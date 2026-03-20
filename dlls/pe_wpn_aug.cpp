#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define AUG_SLOT		2
#define AUG_POSITION	1
#define AUG_MAX			150
#define AUG_CLIP		30

enum aug_e
{
	AUG_IDLE = 0,
	AUG_DRAW,
	AUG_RELOAD,
	AUG_SHOT
};


LINK_ENTITY_TO_CLASS( weapon_aug, CAug );


//=========================================================
//=========================================================
int CAug::SecondaryAmmoIndex( void )
{
	return -1;
}

void CAug::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_aug"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_9mmAR.mdl");
	m_iId = WEAPON_AUG;

	m_iDefaultAmmo = AUG_CLIP;
	m_bCanZoom = TRUE;

	FallInit();// get ready to fall down.
}


void CAug::Precache( void )
{
	PRECACHE_MODEL("models/aug/v_aug.mdl");
	PRECACHE_MODEL("models/w_9mmAR.mdl");
	PRECACHE_MODEL("models/aug/p_aug.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/aug.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usAug = PRECACHE_EVENT( 1, "events/aug.sc" );
}

int CAug::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "aug";
	p->iMaxAmmo1 = AUG_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AUG_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AUG;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CAug::AddToPlayer( CBasePlayer *pPlayer )
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

void CAug::Holster( int skiplocal )
{
#ifndef CLIENT_DLL
	m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 0;
#endif
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( AUG_HOLSTER );
}

BOOL CAug::Deploy( )
{
	return DefaultDeploy( "models/aug/v_aug.mdl", "models/aug/p_aug.mdl", AUG_DRAW, "aug" );
}


void CAug::PrimaryAttack()
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

	float flSpread = 0;
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		flSpread = ( m_pPlayer->pev->fov == 0 ) ? 0.07 : 0.06;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = ( m_pPlayer->pev->fov == 0 ) ? 0.06 : 0.05;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = ( m_pPlayer->pev->fov == 0 ) ? 0.02 : 0.01;
	else
		flSpread = ( m_pPlayer->pev->fov == 0 ) ? 0.03 : 0.02;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_AUG, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usAug, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.14;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.14;

	//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( );
}



void CAug::SecondaryAttack( void )
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

void CAug::Reload( void )
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
	//m	return;
#ifndef CLIENT_DLL
	m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 0;
#endif

	DefaultReload( AUG_CLIP, AUG_RELOAD, 3.66 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 3.66;
}


void CAug::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( AUG_IDLE );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
