#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define SEBURO_MAX		100
#define SEBURO_CLIP		32
#define BERETTA_SLOT		1
#define BERETTA_POS			8

enum seburo_e
{
	SEBURO_IDLE = 0,
	SEBURO_SHOOT,
	SEBURO_SHOOTE,
	SEBURO_RELOAD,
	SEBURO_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_seburo, CSeburo );


//=========================================================
//=========================================================
int CSeburo::SecondaryAmmoIndex( void )
{
	return -1;
}

void CSeburo::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_seburo"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_SEBURO;

	m_iDefaultAmmo = SEBURO_CLIP;

	FallInit();// get ready to fall down.
}


void CSeburo::Precache( void )
{
	PRECACHE_MODEL("models/seburocx/v_seburocx.mdl");
	PRECACHE_MODEL("models/seburocx/p_seburocx.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/glock.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usGock_auto = PRECACHE_EVENT( 1, "events/seburo.sc" );
}

int CSeburo::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "seburo";
	p->iMaxAmmo1 = SEBURO_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SEBURO_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SEBURO;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CSeburo::AddToPlayer( CBasePlayer *pPlayer )
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

void CSeburo::Holster( int skiplocal )
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( SEBURO_HOLSTER );
}

BOOL CSeburo::Deploy( )
{
	return DefaultDeploy( "models/seburocx/v_seburocx.mdl", "models/seburocx/p_seburocx.mdl", SEBURO_DRAW, "glock_auto" );
}


void CSeburo::PrimaryAttack()
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
		flSpread = 0.05;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.04;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.011;
	else
		flSpread = 0.021;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_SEBURO, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
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



void CSeburo::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CSeburo::Reload( void )
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

	DefaultReload( SEBURO_CLIP, SEBURO_RELOAD, 2.25 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.25;
}


void CSeburo::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( SEBURO_IDLE );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}

