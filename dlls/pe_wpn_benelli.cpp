#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

// special deathmatch benelli spreads
#define VECTOR_CONE_DM_BENELLI	Vector( 0.08716, 0.04362, 0.00  )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLEBENELLI Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

#define BENELLI_MAX		42
#define BENELLI_CLIP	7

#ifdef CLIENT_DLL
extern void InitGlobals( );
#endif

enum benelli_e {
	BENELLI_IDLE = 0,
	BENELLI_SHOOT,
	BENELLI_INSERT,
	BENELLI_START_RELOAD,
	BENELLI_AFTER_RELOAD,
	BENELLI_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_benelli, CBenelli );

void CBenelli::Spawn( )
{
  pev->classname = MAKE_STRING("weapon_benelli");
	Precache( );
	m_iId = WEAPON_BENELLI;
	SET_MODEL(ENT(pev), "models/benelli/w_benelli.mdl");

	m_iDefaultAmmo = BENELLI_CLIP;

	FallInit();// get ready to fall
}


void CBenelli::Precache( void )
{
	PRECACHE_MODEL("models/benelli/v_benelli.mdl");
	PRECACHE_MODEL("models/benelli/p_benelli.mdl");
	PRECACHE_MODEL("models/benelli/w_benelli.mdl");

	m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// benelli shell

	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/benelli.wav");

	PRECACHE_SOUND ("weapons/reload1.wav");	// benelli reload
	PRECACHE_SOUND ("weapons/reload3.wav");	// benelli reload

//	PRECACHE_SOUND ("weapons/sshell1.wav");	// benelli reload - played on client
//	PRECACHE_SOUND ("weapons/sshell3.wav");	// benelli reload - played on client
	
	PRECACHE_SOUND ("weapons/357_cock1.wav"); // gun empty sound
	PRECACHE_SOUND ("weapons/scock1.wav");	// cock gun

	m_usFBenelli = PRECACHE_EVENT( 1, "events/benelli.sc" );
}

int CBenelli::AddToPlayer( CBasePlayer *pPlayer )
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


int CBenelli::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "benelli";
	p->iMaxAmmo1 = BENELLI_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = BENELLI_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_BENELLI;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}



BOOL CBenelli::Deploy( )
{
	return DefaultDeploy( "models/benelli/v_benelli.mdl", "models/benelli/p_benelli.mdl", BENELLI_DRAW, "benelli" );
}

void CBenelli::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
			return;
		Reload( );
		if (m_iClip == 0)
			PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;

	vecDir = m_pPlayer->FireBulletsPlayer( 12, vecSrc, vecAiming, Vector( 0.065, 0.65, 0.00  ), 2048, BULLET_PLAYER_BENELLI, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFBenelli, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );


	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

#ifdef CLIENT_DLL
    if( !gpGlobals )
      InitGlobals( );
#endif
    if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.5;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.70;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.70;
	m_fInSpecialReload = 0;
}


void CBenelli::SecondaryAttack( void )
{ }


void CBenelli::Reload( void )
{
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	if( m_fInSpecialReload == 3 )
	{
		SendWeaponAnim( BENELLI_AFTER_RELOAD );
				
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
		m_fInSpecialReload = 0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
	}
	if( ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 ) || ( m_iClip == BENELLI_CLIP ) )
		return;



	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim( BENELLI_START_RELOAD );
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.3;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		//if (RANDOM_LONG(0,1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
		//else
		//	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));

		SendWeaponAnim( BENELLI_INSERT );

		//m_flNextReload = UTIL_WeaponTimeBase() + 0.4;
		//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.4;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.4;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;
	}
	if( ( m_iClip == 7 ) || ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 ) )
	{
		m_fInSpecialReload = 3;
	}
}


void CBenelli::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

#ifdef CLIENT_DLL
    if( !gpGlobals )
      InitGlobals( );
#endif
	if ( m_flPumpTime && m_flPumpTime < gpGlobals->time )
	{
		// play pumping sound
		//EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
		m_flPumpTime = 0;
	}

	if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload( );
		}
		else if (m_fInSpecialReload != 0)
		{
			//if (m_iClip != 7 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			//{
				Reload( );
			//}
			//else
			/*{
				// reload debounce has timed out
				SendWeaponAnim( BENELLI_AFTER_RELOAD );
				
				// play cocking sound
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}*/
		}
		else
		{
			int iAnim;
			iAnim = BENELLI_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0/9.0);
			SendWeaponAnim( iAnim );
		}
	}
}

