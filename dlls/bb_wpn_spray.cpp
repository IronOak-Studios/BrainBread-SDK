#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define SPRAY_SLOT		2
#define SPRAY_POSITION	1
#define SPRAY_MAX			300
#define SPRAY_CLIP		100

enum minigun_e
{
	MINIGUN_IDLE1 = 0,
	MINIGUN_IDLE2,
	MINIGUN_DRAW,
	MINIGUN_SHOOT
};


LINK_ENTITY_TO_CLASS( weapon_spray, CSpray );


//=========================================================
//=========================================================
int CSpray::SecondaryAmmoIndex( void )
{
	return -1;
}

void CSpray::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_spray"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/minigun/w_minigun.mdl");
	m_iId = WEAPON_SPRAY;

	m_iDefaultAmmo = SPRAY_CLIP;

	FallInit();// get ready to fall down.
}


void CSpray::Precache( void )
{
	PRECACHE_MODEL("models/minigun/v_minigun.mdl");
	PRECACHE_MODEL("models/minigun/w_minigun.mdl");
	PRECACHE_MODEL("models/minigun/p_minigun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/minigunfire.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usSpray = PRECACHE_EVENT( 1, "events/spray.sc" );
}

int CSpray::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "spray";
	p->iMaxAmmo1 = SPRAY_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SPRAY_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SPRAY;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CSpray::AddToPlayer( CBasePlayer *pPlayer )
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

void CSpray::Holster( int skiplocal )
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( MINIGUN_HOLSTER );
}

BOOL CSpray::Deploy( )
{
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
	return DefaultDeploy( "models/minigun/v_minigun.mdl", "models/minigun/p_minigun.mdl", MINIGUN_DRAW, "minigun" );
}

extern int gmsgSpray;
void CSpray::PrimaryAttack()
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

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
  Vector vecDir, vecEnd = vecSrc + vecAiming.Normalize( ) * 400;

#ifndef CLIENT_DLL
  /*t_flame_part fp;
  fp.start = vecSrc;
  fp.end = vecEnd;

  fp.time = gpGlobals->time;
  fp.dmgtype = DMG_FREEZE;

  if( !m_pPlayer->flame.size( ) && m_pPlayer->nextpop < ( gpGlobals->time + 2 ) )
    m_pPlayer->nextpop = gpGlobals->time + 0.5;
  m_pPlayer->nextburn = gpGlobals->time;
  m_pPlayer->flame.push_back( fp );*/
#endif
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSpray, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1667;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1667;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();// + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 2, 3 );
}



void CSpray::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
}

void CSpray::Reload( void )
{
  if( m_iClip > 0 )
    return;
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
	//m	return;

	DefaultReload( SPRAY_CLIP, MINIGUN_DRAW, 1.5 );
}


void CSpray::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	//if( RANDOM_LONG( 0, 1 ) )
		SendWeaponAnim( MINIGUN_IDLE1 );
	//else
	//	SendWeaponAnim( MINIGUN_IDLE2 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.7;//UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
