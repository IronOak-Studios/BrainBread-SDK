#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define GLOCK_AUTO_A_MAX		128
#define GLOCK_AUTO_A_CLIP		32
#define BERETTA_SLOT			1
#define BERETTA_POS				7

enum glock_auto_a_e
{
	GLOCK_AUTO_A_IDLE = 0,
	GLOCK_AUTO_A_IDLE_LE,
	GLOCK_AUTO_A_IDLE_RE,
	GLOCK_AUTO_A_IDLE_2E,
	GLOCK_AUTO_A_SHOOT_L,
	GLOCK_AUTO_A_SHOOT_R,
	GLOCK_AUTO_A_SHOOT_LE,
	GLOCK_AUTO_A_SHOOT_RE,
	GLOCK_AUTO_A_SHOOT_L_RE,
	GLOCK_AUTO_A_SHOOT_R_LE,
	GLOCK_AUTO_A_SHOOT_LE_RE,
	GLOCK_AUTO_A_SHOOT_RE_LE,
	GLOCK_AUTO_A_SHOOT_BOTH,
	GLOCK_AUTO_A_SHOOT_BOTH2,
	GLOCK_AUTO_A_SHOOT_BOTH_LE,
	GLOCK_AUTO_A_SHOOT_BOTH_RE,
	GLOCK_AUTO_A_SHOOT_BOTH_2E,
	GLOCK_AUTO_A_RELOAD_R,
	GLOCK_AUTO_A_RELOAD_L,
	GLOCK_AUTO_A_RELOAD_R_2E,
	GLOCK_AUTO_A_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_glock_auto_a, CGlock_auto_a );


//=========================================================
//=========================================================
int CGlock_auto_a::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

void CGlock_auto_a::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_glock_auto_a"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/glock/w_glock.mdl");
	m_iId = WEAPON_GLOCK_AUTO_A;

	m_iDefaultAmmo = GLOCK_AUTO_A_CLIP;

	FallInit();// get ready to fall down.
}


void CGlock_auto_a::Precache( void )
{
	PRECACHE_MODEL("models/glock/v_glock_auto_a.mdl");
	PRECACHE_MODEL("models/glock/w_glock.mdl");
	PRECACHE_MODEL("models/glock/p_glock_auto_a.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/glock.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usGlock_auto_a = PRECACHE_EVENT( 1, "events/glock_auto_a.sc" );
}

int CGlock_auto_a::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "glock_auto_a1";
	p->iMaxAmmo1 = GLOCK_AUTO_A_MAX;
	p->pszAmmo2 = "glock_auto_a2";
	p->iMaxAmmo2 = GLOCK_AUTO_A_MAX;
	p->iMaxClip = GLOCK_AUTO_A_CLIP;
	p->iMaxClip2 = GLOCK_AUTO_A_CLIP;
	p->iSlot = BERETTA_SLOT;
	p->iPosition = BERETTA_POS;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK_AUTO_A;
	p->iWeight = 2;

	return 1;
}

int CGlock_auto_a::AddToPlayer( CBasePlayer *pPlayer )
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

void CGlock_auto_a::Holster( int skiplocal )
{
	m_fInReload = FALSE;
	m_fInReload2 = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( GLOCK_AUTO_HOLSTER );
}

BOOL CGlock_auto_a::Deploy( )
{
	return DefaultDeploy( "models/glock/v_glock_auto_a.mdl", "models/glock/p_glock_auto_a.mdl", GLOCK_AUTO_A_DRAW, "glock_auto_a" );
}

void CGlock_auto_a::PrimaryAttack()
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

	// optimized multiplayer. Widened to make it easier to hit a moving player
	float flSpread = 0;
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		flSpread = 0.06;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.055;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.02;
	else
		flSpread = 0.03;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_GLOCK_AUTO_A, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usGlock_auto_a, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.08;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.08;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.06;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CGlock_auto_a::SecondaryAttack( void )
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = 0.15;
		return;
	}

	if (m_iClip2 <= 0)
	{
		if ( m_pPlayer->ammo_slot2_a > 0 )
			return;
/*#ifndef CLIENT_DLL
		if ( m_pPlayer->m_afButtonLast & ( IN_ATTACK ) && CanAttack( m_flNextSecondaryAttack, gpGlobals->time, UseDecrement() ) )
		{
			PrimaryAttack( );
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.06;
			if ( m_flNextSecondaryAttack < UTIL_WeaponTimeBase() )
				m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.06;
			return;
		}
#else
		if ( m_pPlayer->m_afButtonLast & ( IN_ATTACK ) && m_flNextSecondaryAttack <=  UTIL_WeaponTimeBase( ) )
		{
			PrimaryAttack( );
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.06;
			if ( m_flNextSecondaryAttack < UTIL_WeaponTimeBase() )
				m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.06;
			return;
		}
#endif //CLIENT_DLLS*/
		Reload2( );
		PlayEmptySound();
		m_flNextSecondaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip2--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

	// optimized multiplayer. Widened to make it easier to hit a moving player
	float flSpread = 0;
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		flSpread = 0.06;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.055;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.02;
	else
		flSpread = 0.03;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_GLOCK_AUTO_A, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usGlock_auto_a, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 1, 0 );

	if (!m_iClip2 && m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.08;

	if ( m_flNextSecondaryAttack < UTIL_WeaponTimeBase() )
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.08;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.06;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CGlock_auto_a::Reload( void )
{
//#ifndef CLIENT_DLL
	if ( m_pPlayer->ammo_slot2 <= 0 )
		return;
	//ALERT( at_console, "clip %d clipsize %d\n", m_iClip, GLOCK_AUTO_A_CLIP );
	//if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	//	return;

	DefaultReload( GLOCK_AUTO_A_CLIP, GLOCK_AUTO_A_RELOAD_L, 2.0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.0;
//#endif
}

void CGlock_auto_a::Reload2( void )
{
//#ifndef CLIENT_DLL
	if ( m_pPlayer->ammo_slot2_a <= 0 )
		return;

	//ALERT( at_console, "clip %d clipsize %d\n", m_iClip2, GLOCK_AUTO_A_CLIP );
	//if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	//	return;

	DefaultReload2( GLOCK_AUTO_A_CLIP, GLOCK_AUTO_A_RELOAD_R, 2.0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.0;
//#endif
}


void CGlock_auto_a::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( GLOCK_AUTO_A_IDLE );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}

