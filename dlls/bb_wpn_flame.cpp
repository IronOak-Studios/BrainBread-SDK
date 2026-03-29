#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define FLAME_SLOT		2
#define FLAME_POSITION	1
#define FLAME_MAX			99
#define FLAME_CLIP		100

enum flame_e
{
	FLAME_IDLE = 0,
	FLAME_FIRESTART,
	FLAME_FIRE,
	FLAME_FIRESTOP,
};


LINK_ENTITY_TO_CLASS( weapon_flame, CFlame );


//=========================================================
//=========================================================
int CFlame::SecondaryAmmoIndex( void )
{
	return -1;
}

void CFlame::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_flame"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/flame/w_flamethrower.mdl");
	m_iId = WEAPON_FLAME;

	m_iDefaultAmmo = FLAME_CLIP;

	FallInit();// get ready to fall down.
}


void CFlame::Precache( void )
{
	PRECACHE_MODEL("models/flame/v_flamethrower.mdl");
	PRECACHE_MODEL("models/flame/w_flamethrower.mdl");
	PRECACHE_MODEL("models/flame/p_flamethrower.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/flame.wav");
	PRECACHE_SOUND ("misc/burn.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usFlame = PRECACHE_EVENT( 1, "events/flame.sc" );
}

int CFlame::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "flame";
	p->iMaxAmmo1 = FLAME_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = FLAME_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_FLAME;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CFlame::AddToPlayer( CBasePlayer *pPlayer )
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

void CFlame::Holster( int skiplocal )
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( MINIGUN_HOLSTER );
}

BOOL CFlame::Deploy( )
{
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
	return DefaultDeploy( "models/flame/v_flamethrower.mdl", "models/flame/p_flamethrower.mdl", FLAME_IDLE, "ak47" );
}

extern int gmsgSpray;
void CFlame::PrimaryAttack()
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
  t_flame_part fp;
  fp.start = vecSrc;
  fp.end = vecEnd;

  fp.time = gpGlobals->time;
  fp.dmgtype = DMG_BURN;

  if( !m_pPlayer->flame.size( ) /*&& m_pPlayer->nextpop < ( gpGlobals->time + 2 )*/ )
    m_pPlayer->nextpop = gpGlobals->time + 0.32;
  else
    m_pPlayer->nextburn = gpGlobals->time;
  m_pPlayer->flame.push_back( fp );
  if( m_iClip <= 0 )
  {
    m_pPlayer->flame.clear( );
    m_pPlayer->nextpop = 0;
  }
#endif
	
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFlame, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1667;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1667;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();// + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 2, 3 );
}



void CFlame::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
}

void CFlame::Reload( void )
{
  //if( m_iClip > 0 )
    return;
	/*if( iItemSlot( m_pPlayer ) == 2 )
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

	DefaultReload( FLAME_CLIP, FLAME_IDLE, 1.5 );*/
}

#ifdef CLIENT_DLL
float flaming = 0;
extern globalvars_t *gpGlobals;
#include "../cl_dll/wrect.h"
typedef int (*pfnUserMsgHook)(const char *pszName, int iSize, void *pbuf);
#include "../engine/cdll_int.h"
#include "../dlls/cdll_dll.h"
#include "../common/event_api.h"
extern cl_enginefunc_t gEngfuncs;
#endif

void CFlame::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	#ifdef CLIENT_DLL
	if( flaming && flaming <= gpGlobals->time )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation( FLAME_FIRESTOP, 2 );
		//char text[512];
		//sprintf( text, "stopping %f %f\n", flaming, gpGlobals->time );
		//gEngfuncs.pfnConsolePrint( text );
		flaming = 0;
	}
	#endif


	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	//if( RANDOM_LONG( 0, 1 ) )
		//SendWeaponAnim( FLAME_IDLE );
	//else
	//	SendWeaponAnim( MINIGUN_IDLE2 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.7;//UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
