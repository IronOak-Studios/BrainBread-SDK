#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define MP5_CLIP	30
#define	MP5_MAX		180

enum mp5_e
{
	MP5_IDLE1 = 0,
	MP5_IDLE1B,
	MP5_IDLE2,
	MP5_IDLE2B,
	MP5_DRAW,
	MP5_DRAWB,
	MP5_RELOAD,
	MP5_RELOADB,
	MP5_SHOOT,
	MP5_SHOOTB,
};



LINK_ENTITY_TO_CLASS( weapon_mp5, CMP5 );


//=========================================================
//=========================================================
int CMP5::SecondaryAmmoIndex( void )
{
	return -1;
}

void CMP5::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_mp5");
	Precache( );
	SET_MODEL(ENT(pev), "models/mp5_10/w_mp5_10.mdl");
	m_iId = WEAPON_MP5;

	m_iDefaultAmmo = MP5_CLIP;
	m_iClip2 = 0;
	m_bCanZoom = TRUE;
	FallInit();// get ready to fall down.
}


void CMP5::Precache( void )
{
	PRECACHE_MODEL("models/mp5_10/v_mp5_10.mdl");
	PRECACHE_MODEL("models/mp5_10/w_mp5_10.mdl");
	PRECACHE_MODEL("models/mp5_10/p_mp5_10.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND ("weapons/mp5_10-1.wav");

	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/glauncher2.wav" );

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usMP5 = PRECACHE_EVENT( 1, "events/mp5_10_1.sc" );
}

int CMP5::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "mp5";
	p->iMaxAmmo1 = MP5_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MP5_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CMP5::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CMP5::Deploy( )
{
	if( m_iClip2 )
		return DefaultDeploy( "models/mp5_10/v_mp5_10.mdl", "models/mp5_10/p_mp5_10.mdl", MP5_DRAWB, "mp5" );
	else
		return DefaultDeploy( "models/mp5_10/v_mp5_10.mdl", "models/mp5_10/p_mp5_10.mdl", MP5_DRAW, "mp5" );
}

void CMP5::Holster( int skiplocal )
{
#ifndef CLIENT_DLL
	m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 0;
#endif
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CMP5::PrimaryAttack()
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
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

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

	float flSpread = 0;
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		flSpread = 0.07;//( m_pPlayer->pev->fov == 0 ) ? 0.07 : 0.06;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.06;//( m_pPlayer->pev->fov == 0 ) ? 0.06 : 0.05;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.02;//( m_pPlayer->pev->fov == 0 ) ? 0.02 : 0.01;
	else
		flSpread = 0.03;//( m_pPlayer->pev->fov == 0 ) ? 0.03 : 0.02;
	
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		
	//vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0,0,0), 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
  int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usMP5, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip2 ? 1 : 0 ), 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	//if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
	//	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CMP5::SecondaryAttack( void )
{
	if( !( m_pPlayer->m_afButtonPressed & IN_ATTACK2 ) )
		return;
#ifndef CLIENT_DLL
	if( m_pPlayer->pev->fov != 0 )
		m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 0;
	else
		m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 50;
#endif
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1;
}

void CMP5::Reload( void )
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

#ifndef CLIENT_DLL
	m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 0;
#endif
	if( m_iClip2 )
	{
		DefaultReload( MP5_CLIP, MP5_RELOADB, 3.33 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 3.33;
		m_iClip2 = 0;
	}
	else
	{
		DefaultReload( MP5_CLIP, MP5_RELOAD, 2.33 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.33;
		m_iClip2 = 1;
	}
}


void CMP5::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
}

