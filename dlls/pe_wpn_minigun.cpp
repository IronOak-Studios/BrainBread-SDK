#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define MINIGUN_SLOT		2
#define MINIGUN_POSITION	1
#define MINIGUN_MAX			0
#define MINIGUN_CLIP		600

enum minigun_e
{
	MINIGUN_IDLE1 = 0,
	MINIGUN_IDLE2,
	MINIGUN_DRAW,
	MINIGUN_SHOOT
};


LINK_ENTITY_TO_CLASS( weapon_minigun, CMinigun );


//=========================================================
//=========================================================
int CMinigun::SecondaryAmmoIndex( void )
{
	return -1;
}

void CMinigun::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_minigun"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/minigun/w_minigun.mdl");
	m_iId = WEAPON_MINIGUN;

	m_iDefaultAmmo = MINIGUN_CLIP;

	FallInit();// get ready to fall down.
}


void CMinigun::Precache( void )
{
	PRECACHE_MODEL("models/minigun/v_minigun.mdl");
	PRECACHE_MODEL("models/minigun/w_minigun.mdl");
	PRECACHE_MODEL("models/minigun/p_minigun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/minigunfire.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usMinigun = PRECACHE_EVENT( 1, "events/minigun.sc" );
}

int CMinigun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "minigun";
	p->iMaxAmmo1 = MINIGUN_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MINIGUN_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MINIGUN;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CMinigun::AddToPlayer( CBasePlayer *pPlayer )
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

void CMinigun::Holster( int skiplocal )
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( MINIGUN_HOLSTER );
}

BOOL CMinigun::Deploy( )
{
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
	return DefaultDeploy( "models/minigun/v_minigun.mdl", "models/minigun/p_minigun.mdl", MINIGUN_DRAW, "minigun" );
}


void CMinigun::PrimaryAttack()
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


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

	// optimized multiplayer. Widened to make it easier to hit a moving player
	float flSpread = 0;
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		flSpread = 0.045;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.035;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.008;
	else
		flSpread = 0.015;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_MINIGUN, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;

#ifndef CLIENT_DLL
/*	int zv = m_pPlayer->pev->velocity.z;
	if( m_pPlayer->pev->velocity.Length2D( ) > 100 )
		m_pPlayer->pev->velocity = ( m_pPlayer->pev->velocity / 2 ) + vecAiming * ( -m_pPlayer->DamageForce( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) ? 2 : 7 ) * ( m_pPlayer->pev->fuser4 ? 0.2 : 1 ) );
	else
		m_pPlayer->pev->velocity = m_pPlayer->pev->velocity + vecAiming * ( -m_pPlayer->DamageForce( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) ? 2 : 7 ) * ( m_pPlayer->pev->fuser4 ? 0.2 : 1 ) );
	//m_pPlayer->pev->velocity.z = zv;*/
#endif
	
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usMinigun, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.05;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.05;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();// + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 2, 3 );
}



void CMinigun::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
}

void CMinigun::Reload( void )
{
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

	DefaultReload( MINIGUN_CLIP, MINIGUN_DRAW, 0 );*/
}


void CMinigun::WeaponIdle( void )
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
