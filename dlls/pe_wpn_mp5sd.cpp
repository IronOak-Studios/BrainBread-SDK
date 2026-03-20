#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define MP5SD_SLOT		2
#define MP5SD_POSITION	1
#define MP5SD_MAX			150
#define MP5SD_CLIP		30

enum mp5sd_e
{
	MP5SD_IDLE = 0,
	MP5SD_SHOOT,
	MP5SD_DRAW,
	MP5SD_DOWN,
	MP5SD_RELOAD
};


LINK_ENTITY_TO_CLASS( weapon_mp5sd, CMP5sd );


//=========================================================
//=========================================================
int CMP5sd::SecondaryAmmoIndex( void )
{
	return -1;
}

void CMP5sd::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_mp5sd"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/mp5sd/w_mp5sd.mdl");
	m_iId = WEAPON_MP5SD;

	m_iDefaultAmmo = MP5SD_CLIP;
	m_bCanZoom = TRUE;

	FallInit();// get ready to fall down.
}


void CMP5sd::Precache( void )
{
	PRECACHE_MODEL("models/mp5sd/v_mp5sd.mdl");
	PRECACHE_MODEL("models/mp5sd/w_mp5sd.mdl");
	PRECACHE_MODEL("models/mp5sd/p_mp5sd.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/mp5sd.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usMP5sd = PRECACHE_EVENT( 1, "events/mp5sd.sc" );
}

int CMP5sd::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "mp5sd";
	p->iMaxAmmo1 = MP5SD_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MP5SD_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5SD;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CMP5sd::AddToPlayer( CBasePlayer *pPlayer )
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

void CMP5sd::Holster( int skiplocal )
{
#ifndef CLIENT_DLL
	m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 0;
#endif
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( MP5SD_HOLSTER );
}

BOOL CMP5sd::Deploy( )
{
	return DefaultDeploy( "models/mp5sd/v_mp5sd.mdl", "models/mp5sd/p_mp5sd.mdl", MP5SD_DRAW, "mp5sd" );
}


void CMP5sd::PrimaryAttack()
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
		flSpread = 0.07;//( m_pPlayer->pev->fov == 0 ) ? 0.07 : 0.06;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.06;//( m_pPlayer->pev->fov == 0 ) ? 0.06 : 0.05;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.02;//( m_pPlayer->pev->fov == 0 ) ? 0.02 : 0.01;
	else
		flSpread = 0.03;//( m_pPlayer->pev->fov == 0 ) ? 0.03 : 0.02;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_MP5SD, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	//m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usMP5sd, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CMP5sd::SecondaryAttack( void )
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

void CMP5sd::Reload( void )
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
	//ALERT( at_console, "slot: %d ammo3: %d ammo4: %d\n", iItemSlot( m_pPlayer ), m_pPlayer->ammo_slot3, m_pPlayer->ammo_slot4 );
	//if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	//m	return;

	DefaultReload( MP5SD_CLIP, MP5SD_RELOAD, 2.25 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.25;
}


void CMP5sd::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 3 ) )
	{
	case 0:	
	case 1:
	case 2:
		iAnim = MP5SD_IDLE;
		break;
	
	default:
	case 3:
		iAnim = MP5SD_IDLE;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
