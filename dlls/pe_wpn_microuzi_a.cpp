#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define MICROUZI_A_MAX		128
#define MICROUZI_A_CLIP		32

enum microuzi_a_e
{
	MICROUZI_A_IDLE = 0,
	MICROUZI_A_SHOOT_RIGHT,
	MICROUZI_A_SHOOT_LEFT,
	MICROUZI_A_SHOOT_BOTH,
	MICROUZI_A_RELOAD_RIGHT,
	MICROUZI_A_RELOAD_LEFT,
	MICROUZI_A_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_microuzi_a, CMicrouzi_a );


//=========================================================
//=========================================================
int CMicrouzi_a::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

void CMicrouzi_a::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_microuzi_a"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/microuzi/w_microuzi_a.mdl");
	m_iId = WEAPON_MICROUZI_A;

	m_iDefaultAmmo = MICROUZI_A_CLIP;

	FallInit();// get ready to fall down.
}


void CMicrouzi_a::Precache( void )
{
	PRECACHE_MODEL("models/microuzi/v_microuzi_a.mdl");
	PRECACHE_MODEL("models/microuzi/w_microuzi_a.mdl");
	PRECACHE_MODEL("models/microuzi/p_microuzi_a.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/microuzi.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usMicrouzi_a = PRECACHE_EVENT( 1, "events/microuzi_a.sc" );
}

int CMicrouzi_a::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "microuzi_a1";
	p->iMaxAmmo1 = MICROUZI_A_MAX;
	p->pszAmmo2 = "microuzi_a2";
	p->iMaxAmmo2 = MICROUZI_A_MAX;
	p->iMaxClip = MICROUZI_A_CLIP;
	p->iMaxClip2 = MICROUZI_A_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MICROUZI_A;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CMicrouzi_a::AddToPlayer( CBasePlayer *pPlayer )
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

void CMicrouzi_a::Holster( int skiplocal )
{
	m_fInReload = FALSE;
	m_fInReload2 = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( MICROUZI_HOLSTER );
}

BOOL CMicrouzi_a::Deploy( )
{
	return DefaultDeploy( "models/microuzi/v_microuzi_a.mdl", "models/microuzi/p_microuzi_a.mdl", MICROUZI_A_DRAW, "microuzi_a" );
}

void CMicrouzi_a::PrimaryAttack()
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
		flSpread = 0.06;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.055;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.02;
	else
		flSpread = 0.03;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_MICROUZI, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
////	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usMicrouzi_a, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.08;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.08;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.06;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CMicrouzi_a::SecondaryAttack( void )
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
		if( iItemSlot( m_pPlayer ) == 2 )
		{
			if ( m_pPlayer->ammo_slot3_a > 0 )
				return;
		}
		else
		{
			if ( m_pPlayer->ammo_slot4_a > 0 )
				return;
		}
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

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_MICROUZI, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usMicrouzi_a, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 1, 0 );

	if (!m_iClip2 && m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.08;

	if ( m_flNextSecondaryAttack < UTIL_WeaponTimeBase() )
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.08;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.06;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CMicrouzi_a::Reload( void )
{
//#ifndef CLIENT_DLL
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
	//	return;

	DefaultReload( MICROUZI_A_CLIP, MICROUZI_A_RELOAD_LEFT, 2.83 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.83;
//#endif
}

void CMicrouzi_a::Reload2( void )
{
	if( iItemSlot( m_pPlayer ) == 2 )
	{
		if ( m_pPlayer->ammo_slot3_a <= 0 )
			return;
	}
	else
	{
		if ( m_pPlayer->ammo_slot4_a <= 0 )
			return;
	}
	//ALERT( at_console, "slot: %d ammo3: %d ammo4: %d\n", iItemSlot( m_pPlayer ), m_pPlayer->ammo_slot3, m_pPlayer->ammo_slot4 );
	//if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	//	return;

	DefaultReload2( MICROUZI_A_CLIP, MICROUZI_A_RELOAD_RIGHT, 2.83 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.83;
}


void CMicrouzi_a::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( MICROUZI_A_IDLE );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
