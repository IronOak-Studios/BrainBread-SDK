#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define GLOCK_AUTO_MAX		128
#define GLOCK_AUTO_CLIP		32
#define BERETTA_SLOT		1
#define BERETTA_POS			8

enum glock_auto_e
{
	GLOCK_AUTO_IDLE = 0,
	GLOCK_AUTO_SHOOT,
	GLOCK_AUTO_SHOOTE,
	GLOCK_AUTO_RELOAD,
	GLOCK_AUTO_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_glock_auto, CGlock_auto );


//=========================================================
//=========================================================
int CGlock_auto::SecondaryAmmoIndex( void )
{
	return -1 ;
}

void CGlock_auto::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_glock_auto"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/glock/w_glock_auto.mdl");
	m_iId = WEAPON_GLOCK_AUTO;

	m_iDefaultAmmo = GLOCK_AUTO_CLIP;

	FallInit();// get ready to fall down.
}


void CGlock_auto::Precache( void )
{
	PRECACHE_MODEL("models/glock/v_glock_auto.mdl");
	PRECACHE_MODEL("models/glock/w_glock_auto.mdl");
	PRECACHE_MODEL("models/glock/p_glock_auto.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/glock.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usGock_auto = PRECACHE_EVENT( 1, "events/glock_auto.sc" );
}

int CGlock_auto::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "glock_auto";
	p->iMaxAmmo1 = GLOCK_AUTO_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_AUTO_CLIP;
	p->iSlot = BERETTA_SLOT;
	p->iPosition = BERETTA_POS;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK_AUTO;
	p->iWeight = 2;

	return 1;
}

int CGlock_auto::AddToPlayer( CBasePlayer *pPlayer )
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

void CGlock_auto::Holster( int skiplocal )
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( GLOCK_AUTO_HOLSTER );
}

BOOL CGlock_auto::Deploy( )
{
	return DefaultDeploy( "models/glock/v_glock_auto.mdl", "models/glock/p_glock_auto.mdl", GLOCK_AUTO_DRAW, "glock_auto" );
}


void CGlock_auto::PrimaryAttack()
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
		if ( m_pPlayer->ammo_slot2 > 0 )
			return;
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
		flSpread = 0.06;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.05;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.015;
	else
		flSpread = 0.025;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_GLOCK_AUTO, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usGock_auto, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.06;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.06;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CGlock_auto::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CGlock_auto::Reload( void )
{
	if ( m_pPlayer->ammo_slot2 <= 0 )
		return;
	//ALERT( at_console, "slot: %d ammo3: %d ammo4: %d\n", iItemSlot( m_pPlayer ), m_pPlayer->ammo_slot3, m_pPlayer->ammo_slot4 );
	//if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	//	return;

	DefaultReload( GLOCK_AUTO_CLIP, GLOCK_AUTO_RELOAD, 2.25 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.25;
}


void CGlock_auto::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( GLOCK_AUTO_IDLE );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}

