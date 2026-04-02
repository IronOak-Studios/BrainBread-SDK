#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define SIG550_SLOT		1
#define SIG550_POSITION	1
#define SIG550_MAX		36
#define SIG550_CLIP		6

enum sig550_e
{
  SIG550_DRAW = 0,
	SIG550_IDLE,
	SIG550_SHOOT,
	SIG550_RELOAD
};


LINK_ENTITY_TO_CLASS( weapon_44sw, CSig550 );


//=========================================================
//=========================================================
int CSig550::SecondaryAmmoIndex( void )
{
	return -1;
}

void CSig550::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_44sw"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/44sw/w_44sw.mdl");
	m_iId = WEAPON_SIG550;

	m_iDefaultAmmo = SIG550_CLIP;
	m_bCanZoom = TRUE;

	FallInit();// get ready to fall down.
}


void CSig550::Precache( void )
{
	PRECACHE_MODEL("models/44sw/v_44sw.mdl");
	PRECACHE_MODEL("models/44sw/w_44sw.mdl");
	PRECACHE_MODEL("models/44sw/p_44sw.mdl");

	//m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/44sw.wav");

	PRECACHE_SOUND ("weapons/sw_clipin.wav");
  PRECACHE_SOUND ("weapons/sw_clipout.wav");

	m_usSig550 = PRECACHE_EVENT( 1, "events/sig550.sc" );
}

int CSig550::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "sig550";
	p->iMaxAmmo1 = SIG550_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SIG550_CLIP;
	p->iSlot = SIG550_SLOT;
	p->iPosition = SIG550_POSITION;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SIG550;
	p->iWeight = 2;

	return 1;
}

int CSig550::AddToPlayer( CBasePlayer *pPlayer )
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

void CSig550::Holster( int skiplocal )
{
#ifndef CLIENT_DLL
	m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 0;
#endif
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( SIG550_HOLSTER );
}

BOOL CSig550::Deploy( )
{
	return DefaultDeploy( "models/44sw/v_44sw.mdl", "models/44sw/p_44sw.mdl", SIG550_DRAW, "deagle" );
}


void CSig550::PrimaryAttack()
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
		flSpread = 0.075;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.065;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.025;
	else
		flSpread = 0.035;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_SIG550, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSig550, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.6;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.6;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CSig550::SecondaryAttack( void )
{
/*#ifndef CLIENT_DLL
	if( m_pPlayer->pev->fov != 0 )
		m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 0;
	else
		m_pPlayer->m_iFOV = m_pPlayer->pev->fov = 50;
#endif*/
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CSig550::Reload( void )
{
	if ( m_pPlayer->ammo_slot2 <= 0 )
		return;


	DefaultReload( SIG550_CLIP, SIG550_RELOAD, 4.5 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 4.5;
}


void CSig550::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( SIG550_IDLE );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}

