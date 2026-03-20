/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

#ifdef CLIENT_DLL
extern void InitGlobals( );
#endif

#define	HANDGRENADE_SLOT	4
#define	HANDGRENADE_POS		1

#define	HANDGRENADE_PRIMARY_VOLUME		450
#define HANDGRENADE_MAX_CARRY	1
#define HANDGRENADE_MAX_CLIP	WEAPON_NOCLIP
#define HANDGRENADE_DEFAULT_GIVE	1

enum handgrenade_e {
	HANDGRENADE_IDLE = 0,
	//HANDGRENADE_FIDGET,
	HANDGRENADE_PINPULL,
	HANDGRENADE_THROW1,	// toss
//	HANDGRENADE_THROW2,	// medium
//	HANDGRENADE_THROW3,	// hard
//	HANDGRENADE_HOLSTER,
	HANDGRENADE_DRAW
};


LINK_ENTITY_TO_CLASS( weapon_handgrenade, CHandGrenade );
LINK_ENTITY_TO_CLASS( weapon_canister, CHandGrenade );


void CHandGrenade::Spawn( )
{
	Precache( );
 	pev->classname = MAKE_STRING("weapon_canister");
	m_iId = WEAPON_HANDGRENADE;
	SET_MODEL(ENT(pev), "models/canister/w_canister2.mdl");

#ifndef CLIENT_DLL
	pev->dmg = gSkillData.plrDmgHandGrenade;
#endif

	m_flStartThrow = 0;
	m_iDefaultAmmo = HANDGRENADE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

int CHandGrenade::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}


void CHandGrenade::Precache( void )
{
	PRECACHE_MODEL("models/canister/w_canister.mdl");
	PRECACHE_MODEL("models/canister/w_canister2.mdl");
	PRECACHE_MODEL("models/canister/v_canister.mdl");
	PRECACHE_MODEL("models/canister/p_canister.mdl");
}

int CHandGrenade::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "canister";
	p->iMaxAmmo1 = HANDGRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = HANDGRENADE_SLOT;
	p->iPosition = HANDGRENADE_POS;
	p->iId = m_iId = WEAPON_HANDGRENADE;
	p->iWeight = 1;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE | ITEM_FLAG_NOAUTORELOAD;

	return 1;
}

BOOL CHandGrenade::Deploy( )
{
	m_flReleaseThrow = -1;
	m_flStartThrow = 0;
	m_iGangsta = m_pPlayer->m_iLastGrenMode;
	return DefaultDeploy( "models/canister/v_canister.mdl", "models/canister/p_canister.mdl", HANDGRENADE_DRAW, "grenade" );
}

BOOL CHandGrenade::CanHolster( void )
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0 );
}

void CHandGrenade::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
	{
		//SendWeaponAnim( HANDGRENADE_HOLSTER );
	}
	else
	{
		// no more grenades!
		//m_pPlayer->pev->weapons &= ~(1<<WEAPON_HANDGRENADE);
		//SetThink( DestroyItem );
		//pev->nextthink = gpGlobals->time + 0.1;
	}
	m_flStartThrow = 0;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CHandGrenade::SecondaryAttack( )
{
	if( !( m_pPlayer->m_afButtonPressed & IN_ATTACK2 ) )
		return;
	/*if( m_iGangsta == 0 )
	{
		m_iGangsta = 1;
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "1 sec explosion delay"  );
	}
	else if( m_iGangsta == 1 )
	{
		m_iGangsta = 2;
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "2 sec explosion delay"  );
	}
	else if( m_iGangsta == 2 )
	{
		m_iGangsta = 3;
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "3 sec explosion delay"  );
	}
	else if( m_iGangsta == 3 )
	{
		m_iGangsta = 4;
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Sticky grenade"  );
	}
	else if( m_iGangsta == 4 )
	{
		m_iGangsta = 0;
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "Explosion on touch"  );
	}
	//m_flNextPrimaryAttack = 1;
	m_pPlayer->m_iLastGrenMode = m_iGangsta;*/
	m_flNextSecondaryAttack = 0.1;
}

void CHandGrenade::PrimaryAttack( )
{
	//ALERT( at_console, "%f %d\n", m_flStartThrow, ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]  > 0 ) );
	if( ( m_flStartThrow == 0 ) && ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]  > 0 ) )
	{
#ifdef CLIENT_DLL
    if( !gpGlobals )
      InitGlobals( );
#endif
    m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim( HANDGRENADE_PINPULL );
		m_fSequenceLoops = FALSE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
	}
}


void CHandGrenade::WeaponIdle( void )
{
	if( m_pPlayer->pev->button & IN_ATTACK )
		return;
#ifdef CLIENT_DLL
    if( !gpGlobals )
      InitGlobals( );
#endif
    if ( m_flReleaseThrow == 0 && m_flStartThrow )
		 m_flReleaseThrow = gpGlobals->time;

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
	/*if ( ( m_flStartThrow == -1 ) && ( m_pPlayer->m_afButtonReleased & IN_ATTACK ) )
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
		return;
	}*/

	if ( ( m_flStartThrow > 0 ) )
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if ( angThrow.x < 0 )
			angThrow.x = -10 + angThrow.x * ( ( 90 - 10 ) / 90.0 );
		else
			angThrow.x = -10 + angThrow.x * ( ( 90 + 10 ) / 90.0 );

		float flVel = ( 90 - angThrow.x ) * 4;
		if ( flVel > 500 )
			flVel = 500;

		//ALERT( at_console, "%f\n", flVel );
#ifdef CLIENT_DLL
    if( !gpGlobals )
      InitGlobals( );
#endif
    flVel += 0.75 * flVel * ( gpGlobals->time - m_flStartThrow ) / 4;
		flVel *= ( FBitSet( m_pPlayer->m_iAddons, AD_ARMS_FAST ) ? 1.5 : 1 );
		//ALERT( at_console, "%f\n", flVel );

		UTIL_MakeVectors( angThrow );

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + Vector( ( m_pPlayer->pev->velocity.x > 0 ? m_pPlayer->pev->velocity.x : 0 ), ( m_pPlayer->pev->velocity.y > 0 ? m_pPlayer->pev->velocity.y : 0 ), ( m_pPlayer->pev->velocity.z > 0 ? m_pPlayer->pev->velocity.z : 0 ) );

		// alway explode 3 seconds after the pin was pulled
		float time = 1.75;//m_flStartThrow - gpGlobals->time + 3.0;
		if (time < 0)
			time = 0;
		
		/*switch( m_iGangsta )
		{
		case 0:
			CGrenade::ShootContact( m_pPlayer->pev, vecSrc, vecThrow );
			break;
		case 1:
			CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, 1 );
			break;
		case 2:
			CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, 2 );
			break;
		case 3:
			CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, 3 );
			break;
		case 4:
			CGrenade::ShootSticky( m_pPlayer->pev, vecSrc, vecThrow );
			break;
		}*/
    CGrenade::ShootPassive( m_pPlayer->pev, vecSrc, vecThrow, 0 );

		/*if ( flVel < 500 )
		{*/
			SendWeaponAnim( HANDGRENADE_THROW1 );
		/*}
		else if ( flVel < 1000 )
		{
			SendWeaponAnim( HANDGRENADE_THROW2 );
		}
		else
		{
			SendWeaponAnim( HANDGRENADE_THROW3 );
		}*/

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_flReleaseThrow = 1;
		m_flStartThrow = 0;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

		m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;

		if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;// ensure that the animation can finish playing
		}
		return;
	}
	else if( ( m_flReleaseThrow > 0 ) )
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
		{
			SendWeaponAnim( HANDGRENADE_DRAW );
		}
		else
		{
			/*if( m_pPlayer->m_iPrimaryWeap )
			{
				ItemInfo& II = CBasePlayerItem::ItemInfoArray[m_pPlayer->m_iPrimaryWeap];
				if ( !II.iId )
					return;
					m_pPlayer->SelectItem( II.pszName );
			}*/
			//m_pPlayer->SelectLastItem( );
			//RetireWeapon( );
      shouldDrop = true;
      //m_pPlayer->DropPlayerItem( (char*)STRING(pev->classname) );
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		m_flReleaseThrow = -1;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		//if (flRand <= 0.75)
		//{
			iAnim = HANDGRENADE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
		/*}
		else 
		{
			iAnim = HANDGRENADE_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0 / 30.0;
		}*/

		SendWeaponAnim( iAnim );
	}
}




