#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

#define DEAGLE_CLIP			7
#define DEAGLE_MAX_AMMO		35
#define DEAGLE_SLOT			1
#define DEAGLE_POS			0


enum deagle_e
{
	DEAGLE_IDLE1 = 0,
	DEAGLE_DRAW,
	DEAGLE_RELOAD,
	DEAGLE_RELOAD_EMPTY,
	DEAGLE_SHOT,
	DEAGLE_SHOT_EMPTY
};

LINK_ENTITY_TO_CLASS( weapon_deagle, CDeagle );

int CDeagle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = DEAGLE_MAX_AMMO;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = DEAGLE_CLIP;
	p->iFlags = 0;
	p->iSlot = DEAGLE_SLOT;
	p->iPosition = DEAGLE_POS;
	p->iId = m_iId = WEAPON_DEAGLE;
	p->iWeight = 2;

	return 1;
}

int CDeagle::AddToPlayer( CBasePlayer *pPlayer )
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

void CDeagle::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_deagle"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_DEAGLE;
	SET_MODEL(ENT(pev), "models/deagle/w_deagle.mdl");

	m_iDefaultAmmo = DEAGLE_CLIP;

	FallInit();// get ready to fall down.
}


void CDeagle::Precache( void )
{
	PRECACHE_MODEL("models/deagle/v_deagle.mdl");
	PRECACHE_MODEL("models/deagle/w_deagle.mdl");
	PRECACHE_MODEL("models/deagle/p_deagle.mdl");

	PRECACHE_MODEL("models/w_357ammobox.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/357_reload1.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");
	PRECACHE_SOUND ("weapons/deagle.wav");

	m_usFDeagle = PRECACHE_EVENT( 1, "events/deagle.sc" );
}

BOOL CDeagle::Deploy( )
{
	return DefaultDeploy( "models/deagle/v_deagle.mdl", "models/deagle/p_deagle.mdl", DEAGLE_DRAW, "deagle" );
}


void CDeagle::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	//SendWeaponAnim( DEAGLE_HOLSTER );
}

void CDeagle::SecondaryAttack( void )
{ }

void CDeagle::PrimaryAttack()
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
		flSpread = 0.034;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.026;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.009;
	else
		flSpread = 0.014;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFDeagle, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = 0.38;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CDeagle::Reload( void )
{
	if ( m_pPlayer->ammo_slot2 <= 0 )
		return;

	DefaultReload( DEAGLE_CLIP, DEAGLE_RELOAD, 2.6 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.6;
}


void CDeagle::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	iAnim = DEAGLE_IDLE1;
	m_flTimeWeaponIdle = (70.0/30.0);
	SendWeaponAnim( iAnim );
}
