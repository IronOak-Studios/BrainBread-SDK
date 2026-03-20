#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define M16_SLOT		2
#define M16_POSITION	1
#define M16_MAX			150
#define M16_CLIP		30

enum m16_e
{
	M16_DRAW = 0,
	M16_IDLE,
	M16_IDLE1,
	M16_IDLE2,
	M16_IDLE3,
	M16_SHOOT,
	M16_SHOOTBURST,
	M16_RELOAD
};


LINK_ENTITY_TO_CLASS( weapon_m16, CM16 );


//=========================================================
//=========================================================
int CM16::SecondaryAmmoIndex( void )
{
	return -1;
}

void CM16::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_m16"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/m16/w_m16.mdl");
	m_iId = WEAPON_M16;

	m_iDefaultAmmo = M16_CLIP;

	FallInit();// get ready to fall down.
}


void CM16::Precache( void )
{
	PRECACHE_MODEL("models/m16/v_m16.mdl");
	PRECACHE_MODEL("models/m16/w_m16.mdl");
	PRECACHE_MODEL("models/m16/p_m16.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/m16.wav");
	PRECACHE_SOUND ("weapons/m16burst.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usM16 = PRECACHE_EVENT( 1, "events/m16.sc" );
	m_usM162 = PRECACHE_EVENT( 1, "events/m162.sc" );
}

int CM16::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "m16";
	p->iMaxAmmo1 = M16_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M16_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M16;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CM16::AddToPlayer( CBasePlayer *pPlayer )
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

void CM16::Holster( int skiplocal )
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( M16_HOLSTER );
}

BOOL CM16::Deploy( )
{
	return DefaultDeploy( "models/m16/v_m16.mdl", "models/m16/p_m16.mdl", M16_DRAW, "m16" );
}

int muh = 0;
void CM16::PrimaryAttack()
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
		flSpread = 0.06;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.05;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.01;
	else
		flSpread = 0.02;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_M16, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

  PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usM16, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, muh ? 0 : 1 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.14;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.14;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CM16::SecondaryAttack( void )
{
	//War faul ;)
	PrimaryAttack( );
	PrimaryAttack( );
  muh = 1;
	PrimaryAttack( );
  muh = 0;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.6;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.6;
}

void CM16::Reload( void )
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
	//ALERT( at_console, "slot: %d ammo3: %d ammo4: %d\n", iItemSlot( m_pPlayer ), m_pPlayer->ammo_slot3, m_pPlayer->ammo_slot4 );
	//if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	//m	return;

	DefaultReload( M16_CLIP, M16_RELOAD, 2.91 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.91;
}


void CM16::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 3 ) )
	{
	case 0:	
		iAnim = M16_IDLE;	
		break;
	case 1:
		iAnim = M16_IDLE1;
		break;
	case 2:
		iAnim = M16_IDLE2;
		break;
	
	default:
	case 3:
		iAnim = M16_IDLE3;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
