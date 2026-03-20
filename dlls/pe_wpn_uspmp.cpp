#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define USPMP_MAX		120
#define USPMP_CLIP		30
#define BERETTA_SLOT		1
#define BERETTA_POS			8

enum uspmp_e
{
	USPMP_IDLE = 0,
	USPMP_SHOOT,
	USPMP_SHOOTE,
	USPMP_RELOAD,
	USPMP_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_uspmp, CUspMP );


//=========================================================
//=========================================================
int CUspMP::SecondaryAmmoIndex( void )
{
	return -1;
}

void CUspMP::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_uspmp"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/uspmp/w_uspmp.mdl");
	m_iId = WEAPON_USPMP;

	m_iDefaultAmmo = USPMP_CLIP;

	FallInit();// get ready to fall down.
}


void CUspMP::Precache( void )
{
	PRECACHE_MODEL("models/uspmp/v_uspmp.mdl");
	PRECACHE_MODEL("models/uspmp/w_uspmp.mdl");
	PRECACHE_MODEL("models/uspmp/p_uspmp.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/glock.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usUspMP = PRECACHE_EVENT( 1, "events/uspmp.sc" );
}

int CUspMP::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uspmp";
	p->iMaxAmmo1 = USPMP_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = USPMP_CLIP;
	p->iSlot = BERETTA_SLOT;
	p->iPosition = BERETTA_POS;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_USPMP;
	p->iWeight = 2;

	return 1;
}

int CUspMP::AddToPlayer( CBasePlayer *pPlayer )
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

void CUspMP::Holster( int skiplocal )
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( USPMP_HOLSTER );
}

BOOL CUspMP::Deploy( )
{
	return DefaultDeploy( "models/uspmp/v_uspmp.mdl", "models/uspmp/p_uspmp.mdl", USPMP_DRAW, "glock_auto" );
}


void CUspMP::PrimaryAttack()
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

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_USPMP, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usUspMP, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.06;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.06;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CUspMP::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CUspMP::Reload( void )
{
	if ( m_pPlayer->ammo_slot2 <= 0 )
		return;
	//ALERT( at_console, "slot: %d ammo3: %d ammo4: %d\n", iItemSlot( m_pPlayer ), m_pPlayer->ammo_slot3, m_pPlayer->ammo_slot4 );
	//if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	//	return;

	DefaultReload( USPMP_CLIP, USPMP_RELOAD, 2.25 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.25;
}


void CUspMP::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( USPMP_IDLE );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}

