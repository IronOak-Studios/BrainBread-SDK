#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#ifndef CLIENT_DLL
	#include "pe_utils.h"
#endif


#define PDW_SLOT		2
#define PDW_POSITION	1
#define PDW_MAX			80
#define PDW_CLIP		20

enum pdw_e
{
	PDW_IDLE,
	PDW_IDLE2,
	PDW_IDLE3,
	PDW_RELOAD,
	PDW_DRAW,
	PDW_SHOOT1,
	PDW_SHOOT2,
	PDW_SHOOT3,
	PDW_HOLSTER,
};


LINK_ENTITY_TO_CLASS( weapon_pdw, CPDW );


//=========================================================
//=========================================================
int CPDW::SecondaryAmmoIndex( void )
{
	return -1;
}

void CPDW::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_pdw"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_9mmAR.mdl");
	m_iId = WEAPON_PDW;

	m_iDefaultAmmo = PDW_CLIP;

	FallInit();// get ready to fall down.
}


void CPDW::Precache( void )
{
	PRECACHE_MODEL("models/pdw/v_pdw.mdl");
	PRECACHE_MODEL("models/w_9mmAR.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND ("weapons/pdw-1.wav");// H to the K
	PRECACHE_SOUND ("weapons/pdw-2.wav");// H to the K

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usPDW = PRECACHE_EVENT( 1, "events/pdw.sc" );
	m_usPDW2 = PRECACHE_EVENT( 1, "events/pdw2.sc" );
}

int CPDW::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "pdw";
	p->iMaxAmmo1 = PDW_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = PDW_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_PDW;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CPDW::AddToPlayer( CBasePlayer *pPlayer )
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

void CPDW::Holster( int skiplocal )
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	SendWeaponAnim( PDW_HOLSTER );
}

BOOL CPDW::Deploy( )
{
	return DefaultDeploy( "models/pdw/v_pdw.mdl", "models/p_9mmAR.mdl", PDW_DRAW, "pdw" );
}


void CPDW::PrimaryAttack()
{
#ifndef CLIENT_DLL
	uShake( m_pPlayer, 0.15, 0.07, 0.15 );
#endif
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		//Reload( );
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
		flSpread = 0.009;
	else
		flSpread = 0.018;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_PDW, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;

  int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usPDW, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.10;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.10;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CPDW::SecondaryAttack( void )
{
	//War faul ;)
	PrimaryAttack( );
	PrimaryAttack( );
	PrimaryAttack( );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CPDW::Reload( void )
{
	//ALERT( at_console, "%d, %d, %d\n", iItemSlot( m_pPlayer ), m_pPlayer->ammo_slot3, m_pPlayer->ammo_slot4 );
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

	DefaultReload( PDW_CLIP, PDW_RELOAD, 1.5 );
	//m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 1.5;
}


void CPDW::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	
		iAnim = PDW_IDLE;	
		break;
	case 1:
		iAnim = PDW_IDLE2;
		break;
	
	default:
	case 2:
		iAnim = PDW_IDLE3;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
