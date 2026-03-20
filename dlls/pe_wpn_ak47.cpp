#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define AK47_SLOT		2
#define AK47_POSITION	1
#define AK47_MAX			150
#define AK47_CLIP		30

enum ak47_e
{
	AK47_IDLE = 0,
	AK47_SHOOT,
	AK47_RELOAD,
	AK47_DRAW
};


LINK_ENTITY_TO_CLASS( weapon_ak47, CAK47 );


//=========================================================
//=========================================================
int CAK47::SecondaryAmmoIndex( void )
{
	return -1;
}

void CAK47::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_ak47"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/ak47/w_ak47.mdl");
	m_iId = WEAPON_AK47;

	m_iDefaultAmmo = AK47_CLIP * 5;

	FallInit();// get ready to fall down.
}


void CAK47::Precache( void )
{
	PRECACHE_MODEL("models/ak47/v_ak47.mdl");
	PRECACHE_MODEL("models/ak47/p_ak47.mdl");
	PRECACHE_MODEL("models/ak47/w_ak47.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND ("weapons/ak47.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usAK47 = PRECACHE_EVENT( 1, "events/ak47.sc" );
}

int CAK47::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "ak47";
	p->iMaxAmmo1 = AK47_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AK47_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AK47;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CAK47::AddToPlayer( CBasePlayer *pPlayer )
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

void CAK47::Holster( int skiplocal )
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( AK47_HOLSTER );
}

BOOL CAK47::Deploy( )
{
	return DefaultDeploy( "models/ak47/v_ak47.mdl", "models/ak47/p_ak47.mdl", AK47_DRAW, "ak47" );
}

extern int gmsgSpray;
void CAK47::PrimaryAttack()
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
		flSpread = 0.09;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.08;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.028;
	else
		flSpread = 0.038;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_AK47, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usAK47, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.11;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.11;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}



void CAK47::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CAK47::Reload( void )
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

	DefaultReload( AK47_CLIP, AK47_RELOAD, 2.3 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 2.3;
}


void CAK47::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 3 ) )
	{
	case 0:	
	case 1:
	case 2:
		iAnim = AK47_IDLE;
		break;
	
	default:
	case 3:
		iAnim = AK47_IDLE;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
