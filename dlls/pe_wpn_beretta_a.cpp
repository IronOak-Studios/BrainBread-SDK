#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

#define BERETTA_A_CLIP			30
#define BERETTA_A_MAX			180
#define BERETTA_A_SLOT			1
#define BERETTA_A_POS			6


enum beretta_a_e
{
	BERETTA_A_IDLE1 = 0,
	BERETTA_A_IDLE2,
	BERETTA_A_IDLE3,
	BERETTA_A_IDLE4,
	BERETTA_A_IDLE5,
	BERETTA_A_IDLE_E,
	BERETTA_A_DRAW,
	BERETTA_A_RELOAD,
	BERETTA_A_SHOOT_L,
	BERETTA_A_SHOOT_R,
	BERETTA_A_SHOOT_LE,
	BERETTA_A_SHOOT_RE,
	BERETTA_A_HOLSTER,
	BERETTA_A_DRAW_EMPTY,
	BERETTA_A_DRAW_SEMIEMPTY
};

LINK_ENTITY_TO_CLASS( weapon_beretta_a, CBeretta_a );

int CBeretta_a::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "beretta_a";
	p->iMaxAmmo1 = BERETTA_A_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = BERETTA_A_CLIP;
	p->iFlags = 0;
	p->iSlot = BERETTA_A_SLOT;
	p->iPosition = BERETTA_A_POS;
	p->iId = m_iId = WEAPON_BERETTA_A;
	p->iWeight = 2;

	return 1;
}

int CBeretta_a::AddToPlayer( CBasePlayer *pPlayer )
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

void CBeretta_a::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_beretta_a"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_BERETTA_A;
	SET_MODEL(ENT(pev), "models/beretta/w_beretta_a.mdl");

	m_iDefaultAmmo = BERETTA_A_CLIP;

	FallInit();// get ready to fall down.
}


void CBeretta_a::Precache( void )
{
	PRECACHE_MODEL("models/beretta/v_beretta_a.mdl");
	PRECACHE_MODEL("models/beretta/w_beretta_a.mdl");
	PRECACHE_MODEL("models/beretta/p_beretta_a.mdl");

	PRECACHE_SOUND ("weapons/beretta_a.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usFBeretta_a = PRECACHE_EVENT( 1, "events/beretta_a.sc" );
}

BOOL CBeretta_a::Deploy( )
{
	if( m_iClip > 1 )
		return DefaultDeploy("models/beretta/v_beretta_a.mdl", "models/beretta/p_beretta_a.mdl", BERETTA_A_DRAW, "beretta_a", UseDecrement(), pev->body );
	
	if( m_iClip < 2 && m_iClip )
		return DefaultDeploy("models/beretta/v_beretta_a.mdl", "models/beretta/p_beretta_a.mdl", BERETTA_A_DRAW_SEMIEMPTY, "beretta_a", UseDecrement(), pev->body );
	return DefaultDeploy("models/beretta/v_beretta_a.mdl", "models/beretta/p_beretta_a.mdl", BERETTA_A_DRAW_EMPTY, "beretta_a", UseDecrement(), pev->body );
}


void CBeretta_a::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	//SendWeaponAnim( BERETTA_A_HOLSTER );
}

void CBeretta_a::SecondaryAttack( void )
{ }

void CBeretta_a::PrimaryAttack()
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
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_BERETTA_A, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFBeretta_a, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, m_iClip < 2 ? 1 : 0, ( m_iClip == 30  ) ? 1 : 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = 0.25;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CBeretta_a::Reload( void )
{
	if ( m_pPlayer->ammo_slot2 <= 0 )
		return;

	DefaultReload( BERETTA_A_CLIP, BERETTA_A_RELOAD, 3.96 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase( ) + 3.96;
}


void CBeretta_a::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	int x;

	x = RANDOM_LONG( 0, 100 );

	if(x <= 70 )
		iAnim = BERETTA_A_IDLE1;
	else if(x <= 80 && x > 70)
		iAnim = BERETTA_A_IDLE2;
	else if(x <= 90 && x > 80)
		iAnim = BERETTA_A_IDLE3;
	else if(x <= 95 && x > 90)
		iAnim = BERETTA_A_IDLE4;
	else if(x <= 100 && x > 95)
		iAnim = BERETTA_A_IDLE5;
	
	if( m_iClip > 1 )
		SendWeaponAnim( iAnim );
	m_flTimeWeaponIdle = (70.0/30.0);
	SendWeaponAnim( iAnim );
}
