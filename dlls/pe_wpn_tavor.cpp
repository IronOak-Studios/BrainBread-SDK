#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#define TAVOR_MAX			42
#define TAVOR_CLIP		7

#ifdef CLIENT_DLL
extern void InitGlobals( );
#endif

enum tavor_e
{
	TAVOR_DEPLOY = 0,
	TAVOR_IDLE,
	TAVOR_SHOOT1,
	TAVOR_STARTRELOAD,
	TAVOR_RELOAD,
	TAVOR_STOPRELOAD,
};


LINK_ENTITY_TO_CLASS( weapon_tavor, CTavor );
LINK_ENTITY_TO_CLASS( weapon_winchester, CTavor );


//=========================================================
//=========================================================
int CTavor::SecondaryAmmoIndex( void )
{
	return -1;
}

void CTavor::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_winchester"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/winchester/w_winchester.mdl");
	m_iId = WEAPON_TAVOR;

	m_iDefaultAmmo = TAVOR_CLIP;

	FallInit();// get ready to fall down.
}


void CTavor::Precache( void )
{
	PRECACHE_MODEL("models/winchester/v_winchester.mdl");
	PRECACHE_MODEL("models/winchester/w_winchester.mdl");
	PRECACHE_MODEL("models/winchester/p_winchester.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	m_usTavor = PRECACHE_EVENT( 1, "events/tavor.sc" );
}

int CTavor::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "tavor";
	p->iMaxAmmo1 = TAVOR_MAX;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = TAVOR_CLIP;
	p->iSlot = iItemSlot( m_pPlayer );
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_TAVOR;
	p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	return 1;
}

int CTavor::AddToPlayer( CBasePlayer *pPlayer )
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

void CTavor::Holster( int skiplocal )
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
//	SendWeaponAnim( TAVOR_HOLSTER );
}

BOOL CTavor::Deploy( )
{
	return DefaultDeploy( "models/winchester/v_winchester.mdl", "models/winchester/p_winchester.mdl", TAVOR_DEPLOY, "ak47" );
}


void CTavor::PrimaryAttack()
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
		flSpread = 0.31;
	else if (m_pPlayer->pev->velocity.Length2D() > 170)
		flSpread = 0.18;
	else if ( FBitSet( m_pPlayer->pev->flags, FL_DUCKING ) )
		flSpread = 0.002;
	else
		flSpread = 0.03;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_TAVOR, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
//	m_pPlayer->pev->punchangle.x = -m_pPlayer->m_fSpread * (float)m_pPlayer->m_iShots;
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usTavor, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

#ifdef CLIENT_DLL
    if( !gpGlobals )
      InitGlobals( );
#endif
    if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.5;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.12;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.12;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.70;
	m_fInSpecialReload = 0;

}



void CTavor::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.38;
}

void CTavor::Reload( void )
{
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	if( m_fInSpecialReload == 3 )
	{
    SendWeaponAnim( TAVOR_STOPRELOAD );
				
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
		m_fInSpecialReload = 0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.6;
	}
	if( ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 ) || ( m_iClip == TAVOR_CLIP ) )
		return;



	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim( TAVOR_STARTRELOAD );
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.34;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.34;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.34;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.34;
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

    SendWeaponAnim( TAVOR_RELOAD );

		//m_flNextReload = UTIL_WeaponTimeBase() + 0.4;
		//m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.4;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.57;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.57;
	}
  if( ( m_iClip == TAVOR_CLIP ) || ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 ) )
	{
		m_fInSpecialReload = 3;
	}
}


void CTavor::WeaponIdle( void )
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
			iAnim = TAVOR_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (20.0/9.0);
			SendWeaponAnim( iAnim );
		}
	}
}
