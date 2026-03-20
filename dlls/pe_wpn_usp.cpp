#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

#define USP_CLIP			15
#define USP_MAX				90
#define USP_SLOT			1
#define USP_POS				3

enum usp_e
{
	USP_IDLE = 0,
	USP_SHOOT,
	USP_SHOOTE,
	USP_DRAW,
	USP_RELOAD,
	USP_RELOADE,
	USP_IDLE2,
	USP_SHOOT2,
	USP_SHOOT2E,
	USP_NTOG,
	USP_GTON,
	USP_RELOAD2,
	USP_RELOAD2E,
};

LINK_ENTITY_TO_CLASS( weapon_usp, CUsp );

int CUsp::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "usp";
	p->iMaxAmmo1 = USP_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = USP_CLIP;
	p->iFlags = 0;
	p->iSlot = USP_SLOT;
	p->iPosition = USP_POS;
	p->iId = m_iId = WEAPON_USP;
	p->iWeight = 2;

	return 1;
}

int CUsp::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CUsp::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_usp"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_USP;
	SET_MODEL(ENT(pev), "models/usp/w_usp.mdl");

	m_iDefaultAmmo = USP_CLIP;

	m_iClip2 = 0;

	FallInit();// get ready to fall down.
}


void CUsp::Precache( void )
{
	PRECACHE_MODEL("models/usp/v_usp.mdl");
	PRECACHE_MODEL("models/usp/w_usp.mdl");
	PRECACHE_MODEL("models/usp/p_usp.mdl");

	PRECACHE_SOUND ("weapons/usp.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usFUsp = PRECACHE_EVENT( 1, "events/usp.sc" );
}

BOOL CUsp::Deploy( )
{
	m_flNextSecondaryAttack = 0;
	m_iClip2 = 0;
	return DefaultDeploy( "models/usp/v_usp.mdl", "models/usp/p_usp.mdl", USP_DRAW, "usp", UseDecrement(), pev->body );
}


void CUsp::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	m_iClip2 = 0;	//SendWeaponAnim( USP_HOLSTER );
}

void CUsp::SecondaryAttack( void )
{
	ResetSequenceInfo( );
#ifndef CLIENT_DLL
	if( m_iClip2 )
		SendWeaponAnim( USP_GTON );
	else
		SendWeaponAnim( USP_NTOG );
#endif

	m_iClip2 = !m_iClip2;
	m_flNextPrimaryAttack = 1;
	m_flNextSecondaryAttack = 1;
}

void CUsp::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if( m_pPlayer->ammo_slot2 > 0 )
			return;

		if (!m_fFireOnEmpty)
			Reload( );
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir;
	float flSpread = 0;
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		flSpread = 0.03;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.022;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.007;
	else
		flSpread = 0.01;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_USP, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFUsp, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, m_iClip2 ? 1 : 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = 0.25;
	m_flNextSecondaryAttack = 0.25;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CUsp::Reload( void )
{
	if ( m_pPlayer->ammo_slot2 <= 0 )
		return;

	if( m_iClip2 )
		DefaultReload( USP_CLIP, USP_RELOAD2, 2.33 );
	else
		DefaultReload( USP_CLIP, USP_RELOAD, 2.33 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.33;
	m_flNextSecondaryAttack = 2.33;
}


void CUsp::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

/*	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	if( m_iGansta )
		iAnim = USP_IDLE2;
	else
		iAnim = USP_IDLE;
	m_flTimeWeaponIdle = (70.0/30.0);
	SendWeaponAnim( iAnim );*/
}

