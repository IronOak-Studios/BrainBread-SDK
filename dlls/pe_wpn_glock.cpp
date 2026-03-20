#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

#define GLOCK_CLIP			18
#define GLOCK_MAX			108
#define GLOCK_SLOT			1
#define GLOCK_POS			2

enum glock_e
{
	GLOCK_IDLE = 0,
	GLOCK_SHOOT,
	GLOCK_SHOOT2,
	GLOCK_SHOOT3,
	GLOCK_SHOOT_BURST,
	GLOCK_SHOOT_LAST,
	GLOCK_RELOAD,
	GLOCK_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_glock, CGlock );

int CGlock::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "glock";
	p->iMaxAmmo1 = GLOCK_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_CLIP;
	p->iFlags = 0;
	p->iSlot = GLOCK_SLOT;
	p->iPosition = GLOCK_POS;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = 2;

	return 1;
}

int CGlock::AddToPlayer( CBasePlayer *pPlayer )
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

void CGlock::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_glock"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_GLOCK;
	SET_MODEL(ENT(pev), "models/glock/w_glock.mdl");

	m_iDefaultAmmo = GLOCK_CLIP;

	FallInit();// get ready to fall down.
}


void CGlock::Precache( void )
{
	PRECACHE_MODEL("models/glock/v_glock.mdl");
	PRECACHE_MODEL("models/glock/w_glock.mdl");
	PRECACHE_MODEL("models/glock/p_glock.mdl");

	PRECACHE_SOUND ("weapons/glock.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usFGlock = PRECACHE_EVENT( 1, "events/glock.sc" );
}

BOOL CGlock::Deploy( )
{
	return DefaultDeploy( "models/glock/v_glock.mdl", "models/glock/p_glock.mdl", GLOCK_DRAW, "glock", UseDecrement(), pev->body );
}


void CGlock::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	//SendWeaponAnim( GLOCK_HOLSTER );
}

void CGlock::SecondaryAttack( void )
{
/*	PrimaryAttack( );
	PrimaryAttack( );
	PrimaryAttack( );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
	SendWeaponAnim( GLOCK_SHOOT_BURST );*/
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if( m_pPlayer->ammo_slot2 > 0 )
			return;

		if (!m_fFireOnEmpty)
			Reload( );
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir;
	float flSpread = 0;
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		flSpread = 0.03;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.022;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.007;
	else
		flSpread = 0.01;

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_GLOCK, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFGlock, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );
	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	m_pPlayer->m_iShots += 15;

	if(	m_iClip > 0 )
	{
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_GLOCK, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFGlock, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );
		m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
		m_pPlayer->m_iShots += 15;
		m_iClip--;
	}
	if(	m_iClip > 0 )
	{
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_GLOCK, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFGlock, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );
		m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
		m_pPlayer->m_iShots += 15;
		m_iClip--;
	}



	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextSecondaryAttack = 0.65;
	m_flNextPrimaryAttack = 0.65;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CGlock::PrimaryAttack()
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
		if( m_pPlayer->ammo_slot2 > 0 )
			return;

		if (!m_fFireOnEmpty)
			Reload( );
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir;
	float flSpread = 0;
	if ( !FBitSet( m_pPlayer->pev->flags, FL_ONGROUND ) )
		flSpread = 0.03;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.022;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.007;
	else
		flSpread = 0.01;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_GLOCK, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFGlock, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 1, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = 0.25;
	m_flNextSecondaryAttack = 0.25;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CGlock::Reload( void )
{
	if ( m_pPlayer->ammo_slot2 <= 0 )
		return;

	DefaultReload( GLOCK_CLIP, GLOCK_RELOAD, 2.17 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.17;
}


void CGlock::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	iAnim = GLOCK_IDLE;
	m_flTimeWeaponIdle = (70.0/30.0);
	SendWeaponAnim( iAnim );
}

