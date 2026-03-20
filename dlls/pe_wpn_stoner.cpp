#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define STONER_MAX		50
#define STONER_CLIP		10

enum stoner_e
{
	STONER_IDLE = 0,
	STONER_DRAW,
	STONER_RELOAD,
	STONER_SHOOT,
};

LINK_ENTITY_TO_CLASS( weapon_stoner, CStoner );


//=========================================================
//=========================================================
int CStoner::SecondaryAmmoIndex( void )
{
	return -1;
}

void CStoner::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_stoner"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/stoner/w_stoner.mdl");
	m_iId = WEAPON_STONER;

	m_iDefaultAmmo = STONER_CLIP;

	FallInit();// get ready to fall down.
}


void CStoner::Precache( void )
{
	PRECACHE_MODEL("models/stoner/v_stoner.mdl");
	PRECACHE_MODEL("models/stoner/w_stoner.mdl");
	PRECACHE_MODEL("models/stoner/p_stoner.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/stoner.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usStoner = PRECACHE_EVENT( 1, "events/stoner.sc" );
}

int CStoner::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "stoner";
	p->iMaxAmmo1 = STONER_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = STONER_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_STONER;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CStoner::AddToPlayer( CBasePlayer *pPlayer )
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

void CStoner::Holster( int skiplocal )
{
#ifndef CLIENT_DLL
	m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 0;
#endif
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( STONER_HOLSTER );
}

BOOL CStoner::Deploy( )
{
	return DefaultDeploy( "models/stoner/v_stoner.mdl", "models/stoner/p_stoner.mdl", STONER_DRAW, "stoner" );
}


void CStoner::PrimaryAttack()
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
		if( iItemSlot( m_pPlayer ) == 2 )
		{
			if ( m_pPlayer->ammo_slot3 > 0 )
				return;
		}
		else
		{
			if ( m_pPlayer->ammo_slot4 > 0 )
				return;
		}
		Reload( );
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}
	float flSpread = 0;
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

	// optimized multiplayer. Widened to make it easier to hit a moving player
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		flSpread = 0.31;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.20;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.002;
	else
		flSpread = 0.02;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_STONER, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usStoner, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75;

	//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( );
}



void CStoner::SecondaryAttack( void )
{
#ifndef CLIENT_DLL
	if( m_pPlayer->pev->fov == 10 )
		m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 0;
	else if( m_pPlayer->pev->fov == 45 )
		m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 10;
	else
		m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 45;
#endif
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CStoner::Reload( void )
{
	if( iItemSlot( m_pPlayer ) == 2 )
	{
		if ( m_pPlayer->ammo_slot3 <= 0 )
			return;
	}
	else
	{
		if ( m_pPlayer->ammo_slot4 <= 0 )
			return;
	}
	//ALERT( at_console, "slot: %d ammo3: %d ammo4: %d\n", iItemSlot( m_pPlayer ), m_pPlayer->ammo_slot3, m_pPlayer->ammo_slot4 );
	//if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	//	return;
#ifndef CLIENT_DLL
	m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 0;
#endif

	DefaultReload( STONER_CLIP, STONER_RELOAD, 4 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 4;
}


void CStoner::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( STONER_IDLE );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
