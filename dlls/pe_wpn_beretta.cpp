#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

#define BERETTA_CLIP			15
#define BERETTA_MAX				105
#define BERETTA_SLOT			1
#define BERETTA_POS				1


enum beretta_e
{
	BERETTA_IDLE = 0,
	BERETTA_SHOOT,
	BERETTA_SHOOTE,
	BERETTA_DRAW,
	BERETTA_RELOAD,
	BERETTA_RELOADE,
	BERETTA_IDLE2,
	BERETTA_SHOOT2,
	BERETTA_SHOOT2E,
	BERETTA_NTOG,
	BERETTA_GTON,
	BERETTA_RELOAD2,
	BERETTA_RELOAD2E,
};

LINK_ENTITY_TO_CLASS( weapon_beretta, CBeretta );

int CBeretta::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "beretta";
	p->iMaxAmmo1 = BERETTA_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = BERETTA_CLIP;
	p->iFlags = 0;
	p->iSlot = BERETTA_SLOT;
	p->iPosition = BERETTA_POS;
	p->iId = m_iId = WEAPON_BERETTA;
	p->iWeight = 2;

	return 1;
}

int CBeretta::AddToPlayer( CBasePlayer *pPlayer )
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

void CBeretta::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_beretta"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_BERETTA;
	SET_MODEL(ENT(pev), "models/beretta/w_beretta.mdl");

	m_iDefaultAmmo = BERETTA_CLIP;
	m_iClip2 = 0;

	FallInit();// get ready to fall down.
}


void CBeretta::Precache( void )
{
	PRECACHE_MODEL("models/beretta/v_beretta.mdl");
	PRECACHE_MODEL("models/beretta/w_beretta.mdl");
	PRECACHE_MODEL("models/beretta/p_beretta.mdl");

	PRECACHE_SOUND ("weapons/beretta.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usFBeretta = PRECACHE_EVENT( 1, "events/beretta.sc" );
}

BOOL CBeretta::Deploy( )
{
	m_flNextSecondaryAttack = 0;
	m_iClip2 = 0;
	return DefaultDeploy( "models/beretta/v_beretta.mdl", "models/beretta/p_beretta.mdl", BERETTA_DRAW, "beretta", UseDecrement(), pev->body );
}


void CBeretta::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	//SendWeaponAnim( BERETTA_HOLSTER );
	m_iClip2 = 0;
}

void CBeretta::SecondaryAttack( void )
{
	ResetSequenceInfo( );

#ifndef CLIENT_DLL
	if( m_iClip2 )
		SendWeaponAnim( BERETTA_GTON );
	else
		SendWeaponAnim( BERETTA_NTOG );
#endif

	m_iClip2 = !m_iClip2;
	m_flNextPrimaryAttack = 1;
	m_flNextSecondaryAttack = 1;
/*#if defined( CLIENT_WEAPONS )
	int flags = FEV_NOTHOST;
#else
	int flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFBeretta, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 2 + m_iGangsta, 0 );
*/
}

void CBeretta::PrimaryAttack()
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
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_BERETTA, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFBeretta, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, m_iClip2 ? 1 : 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = 0.25;
	m_flNextSecondaryAttack = 0.25;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CBeretta::Reload( void )
{
	if ( m_pPlayer->ammo_slot2 <= 0 )
		return;

	if( m_iClip2 )
		DefaultReload( BERETTA_CLIP, BERETTA_RELOAD2, 2.37 );
	else
		DefaultReload( BERETTA_CLIP, BERETTA_RELOAD, 2.37 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.37;
	m_flNextSecondaryAttack = 2.37;
}


void CBeretta::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	/*int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	iAnim = BERETTA_IDLE;
	m_flTimeWeaponIdle = (70.0/30.0);
	SendWeaponAnim( iAnim );*/
}
