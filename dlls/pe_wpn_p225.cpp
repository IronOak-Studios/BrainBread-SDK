#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

#define P225_CLIP			9
#define P225_MAX			54
#define P225_SLOT			1
#define P225_POS			1

enum p225_e
{
	P225_IDLE = 0,
	P225_SHOOT,
	P225_SHOOTE,
	P225_DRAW,
	P225_RELOAD,
	P225_RELOADE,
	P225_IDLE2,
	P225_SHOOT2,
	P225_SHOOT2E,
	P225_NTOG,
	P225_GTON,
	P225_RELOAD2,
	P225_RELOAD2E,
};

LINK_ENTITY_TO_CLASS( weapon_p225, CP225 );

int CP225::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "p225";
	p->iMaxAmmo1 = P225_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = P225_CLIP;
	p->iFlags = 0;
	p->iSlot = P225_SLOT;
	p->iPosition = P225_POS;
	p->iId = m_iId = WEAPON_P225;
	p->iWeight = 2;

	return 1;
}

int CP225::AddToPlayer( CBasePlayer *pPlayer )
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

void CP225::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_p225"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_P225;
	SET_MODEL(ENT(pev), "models/p225/w_p225.mdl");

	m_iDefaultAmmo = P225_CLIP;
	m_iClip2 = 0;

	FallInit();// get ready to fall down.
}


void CP225::Precache( void )
{
	PRECACHE_MODEL("models/p225/v_p225.mdl");
	PRECACHE_MODEL("models/p225/w_p225.mdl");
	PRECACHE_MODEL("models/p225/p_p225.mdl");

	PRECACHE_SOUND ("weapons/p225.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usFP225 = PRECACHE_EVENT( 1, "events/p225.sc" );
}

BOOL CP225::Deploy( )
{
	m_flNextSecondaryAttack = 0;
	m_iClip2 = 0;
	return DefaultDeploy( "models/p225/v_p225.mdl", "models/p225/p_p225.mdl", P225_DRAW, "p225", UseDecrement(), pev->body );
}


void CP225::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	//SendWeaponAnim( P225_HOLSTER );
	m_iClip2 = 0;
}

void CP225::SecondaryAttack( void )
{
	ResetSequenceInfo( );
#ifndef CLIENT_DLL
	if( m_iClip2 )
		SendWeaponAnim( P225_GTON );
	else
		SendWeaponAnim( P225_NTOG );
#endif

	m_iClip2 = !m_iClip2;
	m_flNextPrimaryAttack = 1;
	m_flNextSecondaryAttack = 1;
}

void CP225::PrimaryAttack()
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
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_P225, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFP225, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, m_iClip2 ? 1 : 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = 0.25;
	m_flNextSecondaryAttack = 0.25;
	//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( );
}


void CP225::Reload( void )
{
	if ( m_pPlayer->ammo_slot2 <= 0 )
		return;

	if( m_iClip2 )
		DefaultReload( P225_CLIP, P225_RELOAD2, 2.37 );
	else
		DefaultReload( P225_CLIP, P225_RELOAD, 2.37 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.37;
	m_flNextSecondaryAttack = 2.37;
}


void CP225::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	/*int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	iAnim = P225_IDLE;
	m_flTimeWeaponIdle = (70.0/30.0);
	SendWeaponAnim( iAnim );*/
}


