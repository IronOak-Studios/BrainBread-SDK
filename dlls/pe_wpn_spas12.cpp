#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define SAWED_MAX		30
#define SAWED_CLIP		1

enum sawed_e
{
	SAWED_DEPLOY = 0,
	SAWED_IDLE,
	SAWED_SHOOT,
	SAWED_RELOAD
};

LINK_ENTITY_TO_CLASS( weapon_sawed, CSawed );


//=========================================================
//=========================================================
int CSawed::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

void CSawed::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_sawed"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/sawed/w_sawed.mdl");
	m_iId = WEAPON_SAWED;

	m_iDefaultAmmo = SAWED_CLIP;

	FallInit();// get ready to fall down.
}


void CSawed::Precache( void )
{
	PRECACHE_MODEL("models/sawed/v_sawed.mdl");
	PRECACHE_MODEL("models/sawed/w_sawed.mdl");
	PRECACHE_MODEL("models/sawed/p_sawed.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	//PRECACHE_SOUND ("weapons/sawed.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usSawed = PRECACHE_EVENT( 1, "events/sawed.sc" );
}

int CSawed::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "sawed1";
	p->iMaxAmmo1 = SAWED_MAX;
	p->pszAmmo2 = "sawed2";
	p->iMaxAmmo2 = SAWED_MAX;
	p->iMaxClip = SAWED_CLIP;
	p->iMaxClip2 = SAWED_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SAWED;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CSawed::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
    if( iItemSlot( pPlayer ) == 2 )
	  {
		  if ( pPlayer->ammo_slot3 != pPlayer->ammo_slot3_a )
        pPlayer->ammo_slot3_a = pPlayer->ammo_slot3;
	  }
	  else
	  {
		  if ( pPlayer->ammo_slot4 != pPlayer->ammo_slot4_a )
        pPlayer->ammo_slot4_a = pPlayer->ammo_slot4;
	  }

		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CSawed::Holster( int skiplocal )
{
	m_fInReload = FALSE;
	m_fInReload2 = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( SAWED_HOLSTER );
}

BOOL CSawed::Deploy( )
{
	return DefaultDeploy( "models/sawed/v_sawed.mdl", "models/sawed/p_sawed.mdl", SAWED_DEPLOY, "microuzi" );
}

void CSawed::PrimaryAttack()
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
	/*float flSpread = 0;
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		flSpread = 0.06;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.055;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.02;
	else
		flSpread = 0.03;*/

	vecDir = m_pPlayer->FireBulletsPlayer( 20, vecSrc, vecAiming, Vector( 0.065, 0.04, 0.00  ), 2048, BULLET_PLAYER_SAWED, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
////	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSawed, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.08;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.08;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.06;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CSawed::SecondaryAttack( void )
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
	/*float flSpread = 0;
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		flSpread = 0.06;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.055;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.02;
	else
		flSpread = 0.03;*/

	vecDir = m_pPlayer->FireBulletsPlayer( 35, vecSrc, vecAiming, Vector( 0.095, 0.07, 0.00  ), 2048, BULLET_PLAYER_SAWED, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSawed, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 1, 0 );

	if (!m_iClip2 && m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.08;

	if ( m_flNextSecondaryAttack < UTIL_WeaponTimeBase() )
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.08;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.06;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CSawed::Reload( void )
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

	if( m_iClip || m_iClip2 )
		return;
	//ALERT( at_console, "slot: %d ammo3: %d ammo4: %d\n", iItemSlot( m_pPlayer ), m_pPlayer->ammo_slot3, m_pPlayer->ammo_slot4 );
	//if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	//	return;

	DefaultReload( SAWED_CLIP, SAWED_RELOAD, 0 );
	DefaultReload2( SAWED_CLIP, SAWED_RELOAD, 3.6 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 3.6;
//#endif
}

void CSawed::Reload2( void )
{
	Reload( );
}


void CSawed::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( SAWED_IDLE );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
