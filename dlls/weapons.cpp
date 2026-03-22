/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
/*

===== weapons.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "gamerules.h"
#include "pe_notify.h"
#include "pe_menus.h"
#include "pe_utils.h"
#include "game.h"
#include "../cl_dll/pe_help_topics.h"

extern CGraph	WorldGraph;
extern int gEvilImpulse101;

#define CB_TORSO		(1<<0)
#define CB_ARMS			(1<<1)
#define CB_LEGS			(1<<2)
#define CB_HEAD			(1<<3)
#define CB_NANO			(1<<4)

#define NOT_USED 255

DLL_GLOBAL	short	g_sModelIndexBallsmoke;
DLL_GLOBAL	short	g_sModelIndexLaser;// holds the index for the laser beam
DLL_GLOBAL  const char *g_pModelNameLaser = "sprites/laserbeam.spr";
DLL_GLOBAL	short	g_sModelIndexLaserDot;// holds the index for the laser beam dot
DLL_GLOBAL	short	g_sModelIndexFireball;// holds the index for the fireball
DLL_GLOBAL	short	g_sModelIndexSmoke;// holds the index for the smoke cloud
DLL_GLOBAL	short	g_sModelIndexWExplosion;// holds the index for the underwater explosion
DLL_GLOBAL	short	g_sModelIndexBubbles;// holds the index for the bubbles model
DLL_GLOBAL	short	g_sModelIndexBloodDrop;// holds the sprite index for the initial blood
DLL_GLOBAL	short	g_sModelIndexBloodSpray;// holds the sprite index for splattered blood
DLL_GLOBAL	short	g_sModelIndexBlast;
DLL_GLOBAL	short	g_sModelIndexExplode;

ItemInfo CBasePlayerItem::ItemInfoArray[MAX_WEAPONS];
AmmoInfo CBasePlayerItem::AmmoInfoArray[MAX_AMMO_SLOTS];

extern int gmsgCurWeapon;
extern int gmsgRadar;
extern int gmsgSpecial;

MULTIDAMAGE gMultiDamage;

#define TRACER_FREQ		4			// Tracers fire every fourth bullet


//=========================================================
// MaxAmmoCarry - pass in a name and this function will tell
// you the maximum amount of that type of ammunition that a 
// player can carry.
//=========================================================
int MaxAmmoCarry( int iszName )
{
	for ( int i = 0;  i < MAX_WEAPONS; i++ )
	{
		if ( CBasePlayerItem::ItemInfoArray[i].pszAmmo1 && !strcmp( STRING(iszName), CBasePlayerItem::ItemInfoArray[i].pszAmmo1 ) )
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo1;
		if ( CBasePlayerItem::ItemInfoArray[i].pszAmmo2 && !strcmp( STRING(iszName), CBasePlayerItem::ItemInfoArray[i].pszAmmo2 ) )
			return CBasePlayerItem::ItemInfoArray[i].iMaxAmmo2;
	}

	ALERT( at_console, "MaxAmmoCarry() doesn't recognize '%s'!\n", STRING( iszName ) );
	return -1;
}

	
/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/

//
// ClearMultiDamage - resets the global multi damage accumulator
//
void ClearMultiDamage(void)
{
	gMultiDamage.pEntity = NULL;
	gMultiDamage.amount	= 0;
	gMultiDamage.type = 0;
}


//
// ApplyMultiDamage - inflicts contents of global multi damage register on gMultiDamage.pEntity
//
// GLOBALS USED:
//		gMultiDamage

void ApplyMultiDamage(entvars_t *pevInflictor, entvars_t *pevAttacker )
{
	Vector		vecSpot1;//where blood comes from
	Vector		vecDir;//direction blood should go
	TraceResult	tr;
	
	if ( !gMultiDamage.pEntity )
		return;

	gMultiDamage.pEntity->TakeDamage(pevInflictor, pevAttacker, gMultiDamage.amount, gMultiDamage.type );
}


// GLOBALS USED:
//		gMultiDamage

void AddMultiDamage( entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType)
{
	if ( !pEntity )
		return;
	
	gMultiDamage.type |= bitsDamageType;

	if ( pEntity != gMultiDamage.pEntity )
	{
		ApplyMultiDamage(pevInflictor,pevInflictor); // UNDONE: wrong attacker!
		gMultiDamage.pEntity	= pEntity;
		gMultiDamage.amount		= 0;
	}

	gMultiDamage.amount += flDamage;
}

/*
================
SpawnBlood
================
*/
void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage)
{
	UTIL_BloodDrips( vecSpot, g_vecAttackDir, bloodColor, (int)flDamage );
}


int DamageDecal( CBaseEntity *pEntity, int bitsDamageType )
{
	if ( !pEntity )
		return (DECAL_GUNSHOT1 + RANDOM_LONG(0,4));
	
	return pEntity->DamageDecal( bitsDamageType );
}

void DecalGunshot( TraceResult *pTrace, int iBulletType )
{
	// Is the entity valid
	if ( !UTIL_IsValidEntity( pTrace->pHit ) )
		return;

	if ( VARS(pTrace->pHit)->solid == SOLID_BSP || VARS(pTrace->pHit)->movetype == MOVETYPE_PUSHSTEP )
	{
		CBaseEntity *pEntity = NULL;
		// Decal the wall with a gunshot
		if ( !FNullEnt(pTrace->pHit) )
			pEntity = CBaseEntity::Instance(pTrace->pHit);

		switch( iBulletType )
		{
		case BULLET_PLAYER_FLAME:
		case BULLET_PLAYER_SPRAY:
			UTIL_DecalTrace( pTrace, DamageDecal( pEntity, DMG_BURN ) );
      break;
		//case BULLET_PLAYER_9MM:
		//case BULLET_MONSTER_9MM:
		//case BULLET_PLAYER_MP5:
		//case BULLET_MONSTER_MP5:
		//case BULLET_PLAYER_BUCKSHOT:
		//case BULLET_PLAYER_357:
		default:
			// smoke and decal
			UTIL_GunshotDecalTrace( pTrace, DamageDecal( pEntity, DMG_BULLET ) );
			break;
		/*case BULLET_MONSTER_12MM:
			// smoke and decal
			UTIL_GunshotDecalTrace( pTrace, DamageDecal( pEntity, DMG_BULLET ) );
			break;*/
//		case BULLET_PLAYER_CROWBAR:
		case BULLET_PLAYER_KNIFE:
		//case BULLET_PLAYER_CASE:
			// wall decal
			UTIL_DecalTrace( pTrace, DamageDecal( pEntity, DMG_CLUB ) );
			break;
		}
	}
}



//
// EjectBrass - tosses a brass shell from passed origin at passed velocity
//
void EjectBrass ( const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype )
{
	// FIX: when the player shoots, their gun isn't in the same position as it is on the model other players see.

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
		WRITE_BYTE( TE_MODEL);
		WRITE_COORD( vecOrigin.x);
		WRITE_COORD( vecOrigin.y);
		WRITE_COORD( vecOrigin.z);
		WRITE_COORD( vecVelocity.x);
		WRITE_COORD( vecVelocity.y);
		WRITE_COORD( vecVelocity.z);
		WRITE_ANGLE( rotation );
		WRITE_SHORT( model );
		WRITE_BYTE ( soundtype);
		WRITE_BYTE ( 25 );// 2.5 seconds
	MESSAGE_END();
}


#if 0
// UNDONE: This is no longer used?
void ExplodeModel( const Vector &vecOrigin, float speed, int model, int count )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecOrigin );
		WRITE_BYTE ( TE_EXPLODEMODEL );
		WRITE_COORD( vecOrigin.x );
		WRITE_COORD( vecOrigin.y );
		WRITE_COORD( vecOrigin.z );
		WRITE_COORD( speed );
		WRITE_SHORT( model );
		WRITE_SHORT( count );
		WRITE_BYTE ( 15 );// 1.5 seconds
	MESSAGE_END();
}
#endif


int giAmmoIndex = 0;

// Precaches the ammo and queues the ammo info for sending to clients
void AddAmmoNameToAmmoRegistry( const char *szAmmoname )
{
	// make sure it's not already in the registry
	unsigned long ahash = ElfHash( szAmmoname );
	for ( int i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		if ( !CBasePlayerItem::AmmoInfoArray[i].pszName)
			continue;

		if( CBasePlayerItem::AmmoInfoArray[i].hash == ahash )
			return;
		//if ( stricmp( CBasePlayerItem::AmmoInfoArray[i].pszName, szAmmoname ) == 0 )
		//	return; // ammo already in registry, just quite
	}


	giAmmoIndex++;
	ASSERT( giAmmoIndex < MAX_AMMO_SLOTS );
	if ( giAmmoIndex >= MAX_AMMO_SLOTS )
		giAmmoIndex = 0;

	CBasePlayerItem::AmmoInfoArray[giAmmoIndex].pszName = szAmmoname;
	CBasePlayerItem::AmmoInfoArray[giAmmoIndex].hash = ElfHash( szAmmoname );
	CBasePlayerItem::AmmoInfoArray[giAmmoIndex].iId = giAmmoIndex;   // yes, this info is redundant
}


// Precaches the weapon and queues the weapon info for sending to clients
void UTIL_PrecacheOtherWeapon( const char *szClassname )
{
	edict_t	*pent;

	pent = CREATE_NAMED_ENTITY( MAKE_STRING( szClassname ) );
	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in UTIL_PrecacheOtherWeapon\n" );
		return;
	}
	
	CBaseEntity *pEntity = CBaseEntity::Instance (VARS( pent ));

	if (pEntity)
	{
		ItemInfo II;
		pEntity->Precache( );
		memset( &II, 0, sizeof II );
		if ( ((CBasePlayerItem*)pEntity)->GetItemInfo( &II ) )
		{
			CBasePlayerItem::ItemInfoArray[II.iId] = II;

			if ( II.pszAmmo1 && *II.pszAmmo1 )
			{
				AddAmmoNameToAmmoRegistry( II.pszAmmo1 );
			}

			if ( II.pszAmmo2 && *II.pszAmmo2 )
			{
				AddAmmoNameToAmmoRegistry( II.pszAmmo2 );
			}

			memset( &II, 0, sizeof II );
		}
	}

	REMOVE_ENTITY(pent);
}

// called by worldspawn
void W_Precache(void)
{
	memset( CBasePlayerItem::ItemInfoArray, 0, sizeof(CBasePlayerItem::ItemInfoArray) );
	memset( CBasePlayerItem::AmmoInfoArray, 0, sizeof(CBasePlayerItem::AmmoInfoArray) );
	giAmmoIndex = 0;

	// custom items...

	// common world objects
	/*UTIL_PrecacheOther( "item_suit" );
	UTIL_PrecacheOther( "item_battery" );
	UTIL_PrecacheOther( "item_antidote" );
	UTIL_PrecacheOther( "item_security" );
	UTIL_PrecacheOther( "item_longjump" );*/
    //UTIL_PrecacheOther( "item_nvg" );
	// + ste0
	PRECACHE_MODEL( "models/rockgibs.mdl" );
	PRECACHE_MODEL( "models/woodgibs.mdl" );
	// - ste0

	//PE
	//PRECACHE_MODEL( "sprites/spark.spr" );
	UTIL_PrecacheOtherWeapon( "weapon_deagle" );
	//UTIL_PrecacheOtherWeapon( "weapon_seburo" );
	UTIL_PrecacheOtherWeapon( "weapon_benelli" );
	UTIL_PrecacheOtherWeapon( "weapon_beretta" );
	UTIL_PrecacheOtherWeapon( "weapon_m16" );
	UTIL_PrecacheOtherWeapon( "weapon_glock" );
	UTIL_PrecacheOtherWeapon( "weapon_knife" );
	UTIL_PrecacheOtherWeapon( "weapon_flame" );
	UTIL_PrecacheOtherWeapon( "bb_objective_item" );
	//UTIL_PrecacheOtherWeapon( "weapon_aug" );
	UTIL_PrecacheOtherWeapon( "weapon_minigun" );
	UTIL_PrecacheOtherWeapon( "weapon_microuzi" );
	UTIL_PrecacheOtherWeapon( "weapon_microuzi_a" );
	UTIL_PrecacheOtherWeapon( "weapon_mp5" );
	UTIL_PrecacheOtherWeapon( "weapon_usp" );
	UTIL_PrecacheOtherWeapon( "weapon_stoner" );
	UTIL_PrecacheOtherWeapon( "weapon_44sw" );
	//UTIL_PrecacheOtherWeapon( "weapon_p226" );
	UTIL_PrecacheOtherWeapon( "weapon_p225" );
	UTIL_PrecacheOtherWeapon( "weapon_beretta_a" );
	UTIL_PrecacheOtherWeapon( "weapon_glock_auto_a" );
	//UTIL_PrecacheOtherWeapon( "weapon_glock_auto" );
	//UTIL_PrecacheOtherWeapon( "weapon_mp5sd" );
	//UTIL_PrecacheOtherWeapon( "weapon_tavor" );
	UTIL_PrecacheOtherWeapon( "weapon_sawed" );
	UTIL_PrecacheOtherWeapon( "weapon_ak47" );
	//UTIL_PrecacheOtherWeapon( "weapon_uspmp" );
	UTIL_PrecacheOtherWeapon( "weapon_hand" );
	//UTIL_PrecacheOtherWeapon( "weapon_spray" );
	UTIL_PrecacheOtherWeapon( "weapon_winchester" );
	UTIL_PrecacheOtherWeapon( "weapon_canister" );

	/*UTIL_PrecacheOther( "mapobject_jaguar" );
	UTIL_PrecacheOther( "mapobject_npc_crow" );*/
	//UTIL_PrecacheOther( "mapobject_bluecar" );
	//UTIL_PrecacheOther( "mapobject_copcar" );
	//UTIL_PrecacheOther( "mapobject_asiancar" );
	//UTIL_PrecacheOther( "mapobject_npc_slavebot" );
	//UTIL_PrecacheOther( "mapobject_npc_bum" );
	//UTIL_PrecacheOther( "monster_apache" );
	/*UTIL_PrecacheOther( "bb_tank" );
	UTIL_PrecacheOther( "bb_escapeair" );*/
	UTIL_PrecacheOther( "monster_barney" );
	UTIL_PrecacheOther( "monster_hgrunt" );
	UTIL_PrecacheOther( "monster_zombie" );

	// hand grenade
	//UTIL_PrecacheOtherWeapon("weapon_handgrenade");

	
	//UTIL_PrecacheOther("laser_spot");


/*#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )
	if ( g_pGameRules->IsDeathmatch() )
	{
		UTIL_PrecacheOther( "weaponbox" );// container for dropped deathmatch weapons
	}
#endif*/

	g_sModelIndexBallsmoke = PRECACHE_MODEL ("sprites/smoke.spr");// fireball
	g_sModelIndexFireball = PRECACHE_MODEL ("sprites/zerogxplode.spr");// fireball
	g_sModelIndexWExplosion = PRECACHE_MODEL ("sprites/WXplo1.spr");// underwater fireball
	g_sModelIndexSmoke = PRECACHE_MODEL ("sprites/wallsmokepuff.spr");// smoke //ste0
	g_sModelIndexBubbles = PRECACHE_MODEL ("sprites/bubble.spr");//bubbles
	g_sModelIndexBloodSpray = PRECACHE_MODEL ("sprites/bloodspray.spr"); // initial blood
	g_sModelIndexBloodDrop = PRECACHE_MODEL ("sprites/blood.spr"); // splattered blood 

	g_sModelIndexBlast = PRECACHE_MODEL ("sprites/white.spr");
	g_sModelIndexExplode = PRECACHE_MODEL ("sprites/fexplo.spr");

	g_sModelIndexLaser = 0;//PRECACHE_MODEL( (char *)g_pModelNameLaser );
	g_sModelIndexLaserDot = 0;//PRECACHE_MODEL("sprites/laserdot.spr");


	// used by explosions
	//PRECACHE_MODEL ("models/grenade.mdl");
	PRECACHE_MODEL ("sprites/explode1.spr");

	//PRECACHE_SOUND ("misc/orbital.wav");
	PRECACHE_SOUND ("weapons/mortarhit.wav");

	PRECACHE_SOUND ("weapons/debris1.wav");// explosion aftermaths
	PRECACHE_SOUND ("weapons/debris2.wav");// explosion aftermaths
	PRECACHE_SOUND ("weapons/debris3.wav");// explosion aftermaths

	PRECACHE_SOUND ("weapons/grenade_hit1.wav");//grenade
	PRECACHE_SOUND ("weapons/grenade_hit2.wav");//grenade
	PRECACHE_SOUND ("weapons/grenade_hit3.wav");//grenade

	PRECACHE_SOUND ("weapons/bullet_hit1.wav");	// hit by bullet
	PRECACHE_SOUND ("weapons/bullet_hit2.wav");	// hit by bullet
	
	PRECACHE_SOUND ("items/weapondrop1.wav");// weapon falls to the ground

}


 

TYPEDESCRIPTION	CBasePlayerItem::m_SaveData[] = 
{
	DEFINE_FIELD( CBasePlayerItem, m_pPlayer, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayerItem, m_pNext, FIELD_CLASSPTR ),
	//DEFINE_FIELD( CBasePlayerItem, m_fKnown, FIELD_INTEGER ),Reset to zero on load
	DEFINE_FIELD( CBasePlayerItem, m_iId, FIELD_INTEGER ),
	// DEFINE_FIELD( CBasePlayerItem, m_iIdPrimary, FIELD_INTEGER ),
	// DEFINE_FIELD( CBasePlayerItem, m_iIdSecondary, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CBasePlayerItem, CBaseAnimating );


TYPEDESCRIPTION	CBasePlayerWeapon::m_SaveData[] = 
{
#if defined( CLIENT_WEAPONS )
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_FLOAT ),
#else	// CLIENT_WEAPONS
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_TIME ),
#endif	// CLIENT_WEAPONS
	DEFINE_FIELD( CBasePlayerWeapon, m_iPrimaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iSecondaryAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iClip, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayerWeapon, m_iDefaultAmmo, FIELD_INTEGER ),
//	DEFINE_FIELD( CBasePlayerWeapon, m_iClientClip, FIELD_INTEGER )	 , reset to zero on load so hud gets updated correctly
//  DEFINE_FIELD( CBasePlayerWeapon, m_iClientWeaponState, FIELD_INTEGER ), reset to zero on load so hud gets updated correctly
};

IMPLEMENT_SAVERESTORE( CBasePlayerWeapon, CBasePlayerItem );


void CBasePlayerItem :: SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector(-24, -24, 0);
	pev->absmax = pev->origin + Vector(24, 24, 16); 
}


//=========================================================
// Sets up movetype, size, solidtype for a new weapon. 
//=========================================================
void CBasePlayerItem :: FallInit( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0) );//pointsize until it lands on the ground.
	
	SetTouch( &CBasePlayerItem::DefaultTouch );
	SetThink( &CBasePlayerItem::FallThink );

	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// FallThink - Items that have just spawned run this think
// to catch them when they hit the ground. Once we're sure
// that the object is grounded, we change its solid type
// to trigger and set it in a large box that helps the
// player get it.
//=========================================================
void CBasePlayerItem::FallThink ( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( m_pPlayer )
	{
		// weapon already picked up, stop thinking
		return;
	}

	if ( pev->flags & FL_ONGROUND )
	{
		// clatter if we have an owner (i.e., dropped by someone)
		// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
		if ( !FNullEnt( pev->owner ) )
		{
			int pitch = 95 + RANDOM_LONG(0,29);
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "items/weapondrop1.wav", 1, ATTN_NORM, 0, pitch);	
		}

		// lie flat
		pev->angles.x = 0;
		pev->angles.z = 0;

		Materialize(); 
	}
}

//=========================================================
// Materialize - make a CBasePlayerItem visible and tangible
//=========================================================
void CBasePlayerItem::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	pev->solid = SOLID_TRIGGER;

	UTIL_SetOrigin( pev, pev->origin );// link into world.
	SetTouch (&CBasePlayerItem::DefaultTouch);
	SetThink (NULL);

}

//=========================================================
// AttemptToMaterialize - the item is trying to rematerialize,
// should it do so now or wait longer?
//=========================================================
void CBasePlayerItem::AttemptToMaterialize( void )
{
	float time = g_pGameRules->FlWeaponTryRespawn( this );

	if ( time == 0 )
	{
		Materialize();
		return;
	}

	pev->nextthink = gpGlobals->time + time;
}

//=========================================================
// CheckRespawn - a player is taking this weapon, should 
// it respawn?
//=========================================================
void CBasePlayerItem :: CheckRespawn ( void )
{
	switch ( g_pGameRules->WeaponShouldRespawn( this ) )
	{
	case GR_WEAPON_RESPAWN_YES:
		//Respawn();
		break;
	case GR_WEAPON_RESPAWN_NO:
		return;
		break;
	}
}

//=========================================================
// Respawn- this item is already in the world, but it is
// invisible and intangible. Make it visible and tangible.
//=========================================================
CBaseEntity* CBasePlayerItem::Respawn( void )
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	CBaseEntity *pNewWeapon = CBaseEntity::Create( (char *)STRING( pev->classname ), g_pGameRules->VecWeaponRespawnSpot( this ), pev->angles, pev->owner );

	if ( pNewWeapon )
	{
		pNewWeapon->pev->effects |= EF_NODRAW;// invisible for now
		pNewWeapon->SetTouch( NULL );// no touch
		pNewWeapon->SetThink( &CBasePlayerItem::AttemptToMaterialize );

		DROP_TO_FLOOR ( ENT(pev) );

		// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		pNewWeapon->pev->nextthink = g_pGameRules->FlWeaponRespawnTime( this );
	}
	else
	{
		ALERT ( at_console, "Respawn failed to create %s!\n", STRING( pev->classname ) );
	}

	return pNewWeapon;
}

void CBasePlayerItem::DefaultTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
		return;


	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if( pPlayer->m_iTeam != 1 && !FClassnameIs( pev, "weapon_hand" ) )
		return;
  if( pPlayer->m_iTeam == 1 && FClassnameIs( pev, "weapon_hand" ) )
		return;

  if( ( FClassnameIs( pev, "weapon_minigun" ) && pPlayer->dmgpnts < 4 ) ||
	    ( FClassnameIs( pev, "weapon_flame" ) && pPlayer->dmgpnts < 8 ) ||
	    ( FClassnameIs( pev, "weapon_canister" ) && pPlayer->dmgpnts < 10 ) ||
	    ( FClassnameIs( pev, "weapon_handgrenade" ) && pPlayer->dmgpnts < 10 ) ||
      ( FClassnameIs( pev, "weapon_stoner" ) && pPlayer->dmgpnts < 2 ) )
  {
    ForceHelp( HELP_SKILL, pPlayer );
		return;
  }

  m_iDefaultAmmo = (int)pev->fuser4;

	//ALERT( at_console, UTIL_VarArgs( "%s %d", STRING(pev->classname), pPlayer->m_afButtonPressed & ( IN_USE ) ) );
	/*if( !( strcmp( STRING(pev->classname), "weapon_case" ) ) && !( pPlayer->m_afButtonPressed & ( IN_USE ) ) )
	{
		NotifyMid( pPlayer, NTM_USETOPICK, 1 );
		return;
	}*/
	// can I have this?
	if ( !g_pGameRules->CanHavePlayerItem( pPlayer, this ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( this );
		}
    return;
	}
  /*if( iItemSlot( NULL ) == -1 )
  {
    if( pPlayer->slot2f && pPlayer->slot3f )
		    return;
    else if( !pPlayer->slot2f && pPlayer->m_iSlots[1] != m_iId )
    {
      pPlayer->slot2f = TRUE;
      pPlayer->m_iSlots[0] = m_iId;
     	ItemInfo& p = CBasePlayerItem::ItemInfoArray[m_iId];
      pPlayer->m_sAmmoSlot[1] = p.pszAmmo1;
    }
    else if( !pPlayer->slot3f && pPlayer->m_iSlots[0] != m_iId  )
    {
      pPlayer->slot3f = TRUE;
      pPlayer->m_iSlots[1] = m_iId;
     	ItemInfo& p = CBasePlayerItem::ItemInfoArray[m_iId];
      pPlayer->m_sAmmoSlot[2] = p.pszAmmo1;
   }
  }
  else if( iItemSlot( NULL ) == 1 )
  {
    if( pPlayer->slot1f )
      return;
    pPlayer->slot1f = TRUE;
    ItemInfo& p = CBasePlayerItem::ItemInfoArray[m_iId];
    pPlayer->m_sAmmoSlot[0] = p.pszAmmo1;
  }*/

  list<vec3_t>::iterator it;
  for( it = pPlayer->pickupused.begin( ); it != pPlayer->pickupused.end( ); it++ )
  {
    if( *it == pev->origin )
      return;
  }

  if( pPlayer->m_pActiveItem )
		sprintf( pPlayer->m_sLastWeap, "%s", STRING(pPlayer->m_pActiveItem->pev->classname) );
	else
		sprintf( pPlayer->m_sLastWeap, "weapon_knife" );
	if (pOther->AddPlayerItem( this ))
	{
		AttachToPlayer( pPlayer );
    pPlayer->pickupused.push_front( pev->origin );
		/*if( !strcmp( STRING(pev->classname), "weapon_case" ) || !strcmp( STRING(pev->classname), "pe_object_case" ) || !strcmp( STRING(pev->classname), "weaponbox" ) )
		{
						MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
				WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
				WRITE_BYTE( 2 );
			MESSAGE_END( );
			pPlayer->m_bIsSpecial = TRUE;
			DisableAll( "pe_object_case" );
			DisableAll( "weapon_case" );
			DisableAll( "weaponbox" );
		}*/
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);
	//}

	SUB_UseTargets( pOther, USE_TOGGLE, 0 ); // UNDONE: when should this happen?
	if( /*!FClassnameIs( pev, "weapon_knife" ) && !FClassnameIs( pev, "weapon_hand" ) ) */ !pev->fuser3 )
	{ CBaseEntity *ent = Respawn( );//CBaseEntity::Create( (char*)STRING(pev->classname), pev->origin, pev->angles );
  if( ent )
    ent->pev->fuser4 = pev->fuser4;
	}
  }
}

BOOL CanAttack( float attack_time, float curtime, BOOL isPredicted )
{
#if defined( CLIENT_WEAPONS )
	if ( !isPredicted )
#else
	if ( 1 )
#endif
	{
		return ( attack_time <= curtime ) ? TRUE : FALSE;
	}
	else
	{
		return ( attack_time <= 0.0 ) ? TRUE : FALSE;
	}
}

void CBasePlayerWeapon::ItemPostFrame( void )
{
	if ((m_fInReload) && ( m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase() ) )
	{
		// complete the reload. 
		int j = min( iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);	

		// Add them to the clip
		m_iClip += j;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;

		m_pPlayer->TabulateAmmo();

		m_fInReload = FALSE;
		//m_pPlayer->m_iShots = 0;
	}
	if ((m_fInReload2) && ( m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase() ) )
	{
		// complete the reload. 
		int j = min( iMaxClip2( ) - m_iClip2, m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]);	

		// Add them to the clip
		m_iClip2 += j;
		m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] -= j;

		m_pPlayer->TabulateAmmo();

		m_fInReload2 = FALSE;
		//m_pPlayer->m_iShots = 0;
	}


	if( ( m_pPlayer->pev->button & IN_ATTACK ) && !( m_pPlayer->pev->button & IN_ATTACK2 ) )
		lastside = 0;
	else if( ( m_pPlayer->pev->button & IN_ATTACK2 ) && !( m_pPlayer->pev->button & IN_ATTACK ) )
		lastside = 1;
	else if( ( m_pPlayer->pev->button & IN_ATTACK ) && ( m_pPlayer->pev->button & IN_ATTACK2 ) 
		&& lastside == 1 && m_iClip )
		lastside = 1;
	else if( ( m_pPlayer->pev->button & IN_ATTACK ) && ( m_pPlayer->pev->button & IN_ATTACK2 ) 
		&& lastside == 0 && m_iClip2 )
		lastside = 0;


	if( ( lastside == 1 ) && ( m_pPlayer->pev->button & IN_ATTACK2 ) 
		&& CanAttack( m_flNextSecondaryAttack, gpGlobals->time, UseDecrement( ) ) )
	{
		if( ( m_iClip2 == 0 && pszAmmo2( ) ) || ( iMaxClip2( ) == -1 && !m_pPlayer->m_rgAmmo[SecondaryAmmoIndex( )] ) )
		{
			m_fFireOnEmpty = TRUE;
		}

		m_pPlayer->TabulateAmmo( );
		SecondaryAttack( );
		m_pPlayer->m_fNextCalm = gpGlobals->time + PE_CROSS_CALM(m_pPlayer);
		//m_pPlayer->pev->button &= ~IN_ATTACK2;
		lastside = 0;
	}
	if( ( lastside == 0 ) && ( m_pPlayer->pev->button & IN_ATTACK )
		&& CanAttack( m_flNextPrimaryAttack, gpGlobals->time, UseDecrement( ) ) )
	{
		if( ( m_iClip == 0 && pszAmmo1( ) ) || ( iMaxClip( ) == -1 && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex( )] ) )
		{
			m_fFireOnEmpty = TRUE;
		}

		m_pPlayer->TabulateAmmo( );
		PrimaryAttack( );
		m_pPlayer->m_fNextCalm = gpGlobals->time + PE_CROSS_CALM(m_pPlayer);
		lastside = 1;
	}
	else if ( !m_fInReload2 && m_pPlayer->pev->button & IN_RELOAD && iMaxClip() != WEAPON_NOCLIP && !m_fInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
		//m_pPlayer->m_iShots = 0;
	}
	if ( !m_fInReload && m_pPlayer->pev->button & IN_RELOAD && iMaxClip2() != WEAPON_NOCLIP && !m_fInReload2 ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload2();
		//m_pPlayer->m_iShots = 0;
	}
	else if( ( m_iClip == 0 ) && ( m_pPlayer->pev->button & (IN_ATTACK|IN_ATTACK2) ) )
	{
		if( ( m_pPlayer->m_fNextCalm <= gpGlobals->time ) )
		{
			if( m_pPlayer->m_iShots > 0 )
				m_pPlayer->m_iShots -= (int)( ( gpGlobals->time - m_pPlayer->m_fNextCalm ) / PE_CROSS_CALM(m_pPlayer) + 1 );
			m_pPlayer->m_iShots = max( 0, m_pPlayer->m_iShots );
			m_pPlayer->m_fNextCalm = gpGlobals->time + ( m_pPlayer->m_iShots > PE_CROSS_SHOT_LIMIT ? PE_CROSS_CALM(m_pPlayer) : PE_CROSS_CALM_SLOW(m_pPlayer) );
		}
	}
	WeaponIdle( );
	if ( m_pPlayer && !(m_pPlayer->pev->button & (IN_ATTACK|IN_ATTACK2) ) )
	{
		// no fire buttons down
		if( ( m_pPlayer->m_fNextCalm <= gpGlobals->time ) )
		{
			if( m_pPlayer->m_iShots > 0 )
				m_pPlayer->m_iShots -= (int)( ( gpGlobals->time - m_pPlayer->m_fNextCalm ) / PE_CROSS_CALM(m_pPlayer) + 1 );
			m_pPlayer->m_iShots = max( 0, m_pPlayer->m_iShots );
			m_pPlayer->m_fNextCalm = gpGlobals->time + ( m_pPlayer->m_iShots > PE_CROSS_SHOT_LIMIT ? PE_CROSS_CALM(m_pPlayer) : PE_CROSS_CALM_SLOW(m_pPlayer) );
		}

		m_fFireOnEmpty = FALSE;

		if ( !IsUseable() && m_flNextPrimaryAttack < ( UseDecrement() ? 0.0 : gpGlobals->time ) ) 
		{
			// weapon isn't useable, switch.
			if ( !(iFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && g_pGameRules->GetNextBestWeapon( m_pPlayer, this ) )
			{
				m_flNextPrimaryAttack = ( UseDecrement() ? 0.0 : gpGlobals->time ) + 0.3;
				return;
			}
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if ( !m_fInReload2 && m_iClip == 0 && ( SecondaryAmmoIndex() != -1 ? m_iClip2 == 0 : 1 ) && !(iFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < ( UseDecrement() ? 0.0 : gpGlobals->time ) )
			{
				Reload();
				//m_pPlayer->m_iShots = 0;
				//return;
			}
			if ( m_fInSpecialReload && !(iFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < ( UseDecrement() ? 0.0 : gpGlobals->time ) )
			{
				Reload();
				//m_pPlayer->m_iShots = 0;
				//return;
			}
			if ( !m_fInReload && m_iClip2 == 0 && ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] ? m_iClip == 0 : 1 ) && !(iFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextSecondaryAttack < ( UseDecrement() ? 0.0 : gpGlobals->time ) )
			{
				Reload2();
				//m_pPlayer->m_iShots = 0;
				//return;
			}
		}
	}
	
	// catch all
	/*if ( ShouldWeaponIdle() )
	{
		WeaponIdle();
	}*/
	WeaponIdle();
  if( shouldDrop && m_pPlayer )
  {
    m_pPlayer->DropPlayerItem( (char*)STRING(pev->classname) );
    shouldDrop = false;
  }
}

void CBasePlayerItem::DestroyItem( void )
{
	if ( m_pPlayer )
	{
		// if attached to a player, remove. 
		m_pPlayer->RemovePlayerItem( this );
	}

	Kill( );
}

int CBasePlayerItem::AddToPlayer( CBasePlayer *pPlayer )
{
	m_pPlayer = pPlayer;

	return TRUE;
}

void CBasePlayerItem::Drop( void )
{
	SetTouch( NULL );
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + .1;
}

void CBasePlayerItem::Kill( void )
{
	SetTouch( NULL );
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time + .1;
}

void CBasePlayerItem::Holster( int skiplocal /* = 0 */ )
{ 
	m_pPlayer->pev->viewmodel = 0; 
	m_pPlayer->pev->weaponmodel = 0;
}

void CBasePlayerItem::AttachToPlayer ( CBasePlayer *pPlayer )
{
	pev->movetype = MOVETYPE_FOLLOW;
	pev->solid = SOLID_NOT;
	pev->aiment = pPlayer->edict();
	pev->effects = EF_NODRAW; // ??
	pev->modelindex = 0;// server won't send down to clients if modelindex == 0
  if( strlen( STRING(pev->model) ) )
    strcpy( mdl, STRING(pev->model) );
	pev->model = iStringNull;
	pev->owner = pPlayer->edict();
	pev->nextthink = gpGlobals->time + .1;
	SetTouch( NULL );
	SetThink( NULL ); // Clear FallThink so it can't run while attached to player.
}

// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
int CBasePlayerWeapon::AddDuplicate( CBasePlayerItem *pOriginal )
{
	if ( m_iDefaultAmmo )
	{
		return ExtractAmmo( (CBasePlayerWeapon *)pOriginal );
	}
	else
	{
		// a dead player dropped this.
		return ExtractClipAmmo( (CBasePlayerWeapon *)pOriginal );
	}
}


int CBasePlayerWeapon::AddToPlayer( CBasePlayer *pPlayer )
{
	int bResult = CBasePlayerItem::AddToPlayer( pPlayer );

	pPlayer->pev->weapons |= (1<<m_iId);

	if ( !m_iPrimaryAmmoType )
	{
		m_iPrimaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo1() );
		m_iSecondaryAmmoType = pPlayer->GetAmmoIndex( pszAmmo2() );
	}


	if (bResult)
		return AddWeapon( );
	return FALSE;
}

int CBasePlayerWeapon::UpdateClientData( CBasePlayer *pPlayer )
{
	BOOL bSend = FALSE;
	int state = 0;
	if ( pPlayer->m_pActiveItem == this )
	{
		if ( pPlayer->m_fOnTarget )
			state = WEAPON_IS_ONTARGET;
		else
			state = 1;
	}

	// Forcing send of all data!
	if ( !pPlayer->m_fWeapon )
	{
		bSend = TRUE;
	}
	
	// This is the current or last weapon, so the state will need to be updated
	if ( this == pPlayer->m_pActiveItem ||
		 this == pPlayer->m_pClientActiveItem )
	{
		if ( pPlayer->m_pActiveItem != pPlayer->m_pClientActiveItem )
		{
			bSend = TRUE;
		}
	}

	// If the ammo, state, or fov has changed, update the weapon
	if ( m_iClip != m_iClientClip || 
		 m_iClip2 != m_iClientClip2 || 
		 state != m_iClientWeaponState || 
		 pPlayer->m_iFOV != pPlayer->m_iClientFOV )
	{
		bSend = TRUE;
	}

	if ( bSend )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pPlayer->pev );
			WRITE_BYTE( iItemSlot( pPlayer ) );
			WRITE_BYTE( state );
			WRITE_BYTE( m_iId );
			if( m_iId == WEAPON_MINIGUN || m_iId == WEAPON_FLAME || m_iId == WEAPON_SPRAY )
			{
        WRITE_BYTE( m_iClip / 10 );
				WRITE_BYTE( m_iClip % 10 );
				/*if( m_iClip > 128 )
				{
					WRITE_BYTE( 128 );
					WRITE_BYTE( m_iClip-128 );
				}
				else
				{
					WRITE_BYTE( m_iClip );
					WRITE_BYTE( m_iClip2 );
				}*/
			}
			else
			{
				WRITE_BYTE( m_iClip );
				WRITE_BYTE( m_iClip2 );
			}
		MESSAGE_END();

		m_iClientClip2 = m_iClip2;
		m_iClientClip = m_iClip;
		m_iClientWeaponState = state;
		pPlayer->m_fWeapon = TRUE;
	}

	if ( m_pNext )
		m_pNext->UpdateClientData( pPlayer );

	return 1;
}


void CBasePlayerWeapon::SendWeaponAnim( int iAnim, int skiplocal, int body )
{
	if ( UseDecrement() )
		skiplocal = 1;
	else
		skiplocal = 0;

	m_pPlayer->pev->weaponanim = iAnim;

#if defined( CLIENT_WEAPONS )
	if ( skiplocal && ENGINE_CANSKIP( m_pPlayer->edict() ) )
		return;
#endif

	MESSAGE_BEGIN( MSG_ONE, SVC_WEAPONANIM, NULL, m_pPlayer->pev );
		WRITE_BYTE( iAnim );						// sequence number
		WRITE_BYTE( pev->body );					// weaponmodel bodygroup.
	MESSAGE_END();
}

BOOL CBasePlayerWeapon :: AddPrimaryAmmo( int iCount, char *szName, int iMaxClip, int iMaxCarry )
{
	int iIdAmmo;

	if (iMaxClip < 1)
	{
		m_iClip = -1;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName, iMaxCarry );
	}
	else if (m_iClip == 0)
	{
		int i;
		i = min( m_iClip + iCount, iMaxClip ) - m_iClip;
		m_iClip += i;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount - i, szName, iMaxCarry );
	}
	else
	{
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName, iMaxCarry );
	}
	
	// m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = iMaxCarry; // hack for testing

	if (iIdAmmo > 0)
	{
		m_iPrimaryAmmoType = iIdAmmo;
		if (m_pPlayer->HasPlayerItem( this ) )
		{
			// play the "got ammo" sound only if we gave some ammo to a player that already had this gun.
			// if the player is just getting this gun for the first time, DefaultTouch will play the "picked up gun" sound for us.
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
	}

	return iIdAmmo > 0 ? TRUE : FALSE;
}


BOOL CBasePlayerWeapon :: AddSecondaryAmmo( int iCount, char *szName, int iMaxClip, int iMaxCarry )
{
	int iIdAmmo;

	if (iMaxClip < 1)
	{
		m_iClip2 = -1;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName, iMaxCarry );
	}
	else if (m_iClip2 == 0)
	{
		int i;
		i = min( m_iClip2 + iCount, iMaxClip ) - m_iClip2;
		m_iClip2 += i;
		iIdAmmo = m_pPlayer->GiveAmmo( iCount - i, szName, iMaxCarry );
	}
	else
	{
		iIdAmmo = m_pPlayer->GiveAmmo( iCount, szName, iMaxCarry );
	}
	
	// m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = iMaxCarry; // hack for testing

	if (iIdAmmo > 0)
	{
		m_iSecondaryAmmoType = iIdAmmo;
		if (m_pPlayer->HasPlayerItem( this ) )
		{
			// play the "got ammo" sound only if we gave some ammo to a player that already had this gun.
			// if the player is just getting this gun for the first time, DefaultTouch will play the "picked up gun" sound for us.
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
	}
	return iIdAmmo > 0 ? TRUE : FALSE;
}

//=========================================================
// IsUseable - this function determines whether or not a 
// weapon is useable by the player in its current state. 
// (does it have ammo loaded? do I have any ammo for the 
// weapon?, etc)
//=========================================================
BOOL CBasePlayerWeapon :: IsUseable( void )
{
	/*if( ( m_iClip <= 0 ) && ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] <= 0 ) && ( iMaxAmmo1() != -1 ) &&
		( m_iClip2 <= 0 ) && ( m_pPlayer->m_rgAmmo[ SecondaryAmmoIndex() ] <= 0 ) && ( iMaxAmmo2() != -1 ) )
	{
		// clip is empty (or nonexistant) and the player has no more ammo of this type. 
		return FALSE;
	}
	if( ( m_iClip <= 0 ) && ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] <= 0 ) && ( iMaxAmmo1() != -1 ) && ( iMaxAmmo2() == -1 ) )
	{
		// clip is empty (or nonexistant) and the player has no more ammo of this type. 
		return FALSE;
	}*/

	return TRUE;
}

BOOL CBasePlayerWeapon :: CanDeploy( void )
{
	BOOL bHasAmmo = 0;

  if( !FClassnameIs( pev, "weapon_canister" ) && !FClassnameIs( pev, "weapon_handgrenade" ) )
    return TRUE;
	if ( !pszAmmo1() )
	{
		// this weapon doesn't use ammo, can always deploy.
		return TRUE;
	}

	if ( pszAmmo1() )
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0);
	}
	if ( pszAmmo2() )
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] != 0);
	}
	if( (m_iClip > 0 ) || ( m_iClip2 > 0 ) )
	{
		bHasAmmo |= 1;
	}
	if (!bHasAmmo)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CBasePlayerWeapon :: DefaultDeploy( char *szViewModel, char *szWeaponModel, int iAnim, char *szAnimExt, int skiplocal /* = 0 */, int body )
{
	if (!CanDeploy( ))
		return FALSE;

	m_pPlayer->TabulateAmmo();
	m_pPlayer->pev->viewmodel = MAKE_STRING(szViewModel);
	m_pPlayer->pev->weaponmodel = MAKE_STRING(szWeaponModel);
	strcpy( m_pPlayer->m_szAnimExtention, szAnimExt );
	SendWeaponAnim( iAnim, skiplocal, body );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;

	return TRUE;
}


BOOL CBasePlayerWeapon :: DefaultReload( int iClipSize, int iAnim, float fDelay, int body )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return FALSE;

	int j = min(iClipSize - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);	

	if (j <= 0)
		return FALSE;

	if( FBitSet( m_pPlayer->m_iCyber, CB_ARMS ) )
		fDelay *= 0.5;
	if( FBitSet( m_pPlayer->m_iCyber, CB_TORSO ) )
		fDelay *= 0.9;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	//!!UNDONE -- reload sound goes here !!!
	SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0 );

	m_fInReload = TRUE;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	return TRUE;
}

BOOL CBasePlayerWeapon::DefaultReload2( int iClipSize, int iAnim, float fDelay, int body )
{
	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] <= 0)
		return FALSE;

	int j = min(iClipSize - m_iClip2, m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]);	

	if (j <= 0)
		return FALSE;

	if( FBitSet( m_pPlayer->m_iCyber, CB_ARMS ) )
		fDelay *= 0.8;
	if( FBitSet( m_pPlayer->m_iCyber, CB_TORSO ) )
		fDelay *= 0.9;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	//!!UNDONE -- reload sound goes here !!!
	SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0 );

	m_fInReload2 = TRUE;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	return TRUE;
}

BOOL CBasePlayerWeapon :: PlayEmptySound( void )
{
 	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.33;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.33;
	if (m_iPlayEmptySound)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}

void CBasePlayerWeapon :: ResetEmptySound( void )
{
	m_iPlayEmptySound = 1;
}

//=========================================================
//=========================================================
int CBasePlayerWeapon::PrimaryAmmoIndex( void )
{
	return m_iPrimaryAmmoType;
}

//=========================================================
//=========================================================
int CBasePlayerWeapon::SecondaryAmmoIndex( void )
{
	return -1;
}

void CBasePlayerWeapon::Holster( int skiplocal /* = 0 */ )
{ 
	m_fInReload = FALSE; // cancel any reload in progress.
	m_pPlayer->pev->viewmodel = 0; 
	m_pPlayer->pev->weaponmodel = 0;
}

void CBasePlayerAmmo::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CBasePlayerAmmo::DefaultTouch );
}

CBaseEntity* CBasePlayerAmmo::Respawn( void )
{
	pev->effects |= EF_NODRAW;
	SetTouch( NULL );

	UTIL_SetOrigin( pev, g_pGameRules->VecAmmoRespawnSpot( this ) );// move to wherever I'm supposed to repawn.

	SetThink( &CBasePlayerAmmo::Materialize );
	pev->nextthink = g_pGameRules->FlAmmoRespawnTime( this );

	return this;
}

void CBasePlayerAmmo::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch( &CBasePlayerAmmo::DefaultTouch );
}

void CBasePlayerAmmo :: DefaultTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() )
	{
		return;
	}
	if( ((CBasePlayer*)pOther)->m_iTeam != 1 )
		return;

	if (AddAmmo( pOther ))
	{
		if ( g_pGameRules->AmmoShouldRespawn( this ) == GR_AMMO_RESPAWN_YES )
		{
			Respawn();
		}
		else
		{
			SetTouch( NULL );
			SetThink(&CBaseEntity::SUB_Remove);
			pev->nextthink = gpGlobals->time + .1;
		}
	}
	else if (gEvilImpulse101)
	{
		// evil impulse 101 hack, kill always
		SetTouch( NULL );
		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time + .1;
	}
}

//=========================================================
// called by the new item with the existing item as parameter
//
// if we call ExtractAmmo(), it's because the player is picking up this type of weapon for 
// the first time. If it is spawned by the world, m_iDefaultAmmo will have a default ammo amount in it.
// if  this is a weapon dropped by a dying player, has 0 m_iDefaultAmmo, which means only the ammo in 
// the weapon clip comes along. 
//=========================================================
int CBasePlayerWeapon::ExtractAmmo( CBasePlayerWeapon *pWeapon )
{
	int			iReturn = 0;

	if ( pszAmmo1() != NULL )
	{
		// blindly call with m_iDefaultAmmo. It's either going to be a value or zero. If it is zero,
		// we only get the ammo in the weapon's clip, which is what we want. 
		iReturn = pWeapon->AddPrimaryAmmo( m_iDefaultAmmo, (char *)pszAmmo1(), iMaxClip(), iMaxAmmo1() );
	}

	if ( pszAmmo2() != NULL )
	{
		//iReturn = pWeapon->AddSecondaryAmmo( 0, (char *)pszAmmo2(), iMaxAmmo2() );
		iReturn = pWeapon->AddSecondaryAmmo( m_iDefaultAmmo, (char *)pszAmmo2(), iMaxClip2(), iMaxAmmo2() );
	}
	m_iDefaultAmmo = 0;

	return iReturn;
}

//=========================================================
// called by the new item's class with the existing item as parameter
//=========================================================
int CBasePlayerWeapon::ExtractClipAmmo( CBasePlayerWeapon *pWeapon )
{

	if ( m_iClip != WEAPON_NOCLIP )
	{
		pWeapon->m_pPlayer->GiveAmmo( m_iClip, (char *)pszAmmo1(), iMaxAmmo1() );
	}

	if ( m_iClip2 != WEAPON_NOCLIP )
	{
		pWeapon->m_pPlayer->GiveAmmo( m_iClip2, (char *)pszAmmo2(), iMaxAmmo2() );
	}

	return TRUE;
}
	
//=========================================================
// RetireWeapon - no more ammo for this gun, put it away.
//=========================================================
void CBasePlayerWeapon::RetireWeapon( void )
{
	// first, no viewmodel at all.
	m_pPlayer->pev->viewmodel = iStringNull;
	m_pPlayer->pev->weaponmodel = iStringNull;
	//m_pPlayer->pev->viewmodelindex = NULL;

	g_pGameRules->GetNextBestWeapon( m_pPlayer, this );
}

//*********************************************************
// weaponbox code:
//*********************************************************

LINK_ENTITY_TO_CLASS( weaponbox, CWeaponBox );

/*TYPEDESCRIPTION	CWeaponBox::m_SaveData[] = 
{
	DEFINE_ARRAY( CWeaponBox, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS ),
	DEFINE_ARRAY( CWeaponBox, m_rgiszAmmo, FIELD_STRING, MAX_AMMO_SLOTS ),
	DEFINE_ARRAY( CWeaponBox, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES ),
	DEFINE_FIELD( CWeaponBox, m_cAmmoTypes, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CWeaponBox, CBaseEntity );*/

//=========================================================
//
//=========================================================
void CWeaponBox::Precache( void )
{
  	//PRECACHE_MODEL("models/w_weaponbox.mdl");
}

//=========================================================
//=========================================================
void CWeaponBox :: KeyValue( KeyValueData *pkvd )
{
	if ( m_cAmmoTypes < MAX_AMMO_SLOTS )
	{
		PackAmmo( ALLOC_STRING(pkvd->szKeyName), atoi(pkvd->szValue) );
		m_cAmmoTypes++;// count this new ammo type.

		pkvd->fHandled = TRUE;
	}
	else
	{
		ALERT ( at_console, "WeaponBox too full! only %d ammotypes allowed\n", MAX_AMMO_SLOTS );
	}
}

//=========================================================
// CWeaponBox - Spawn 
//=========================================================
void CWeaponBox::Spawn( void )
{
	Precache( );

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	//SET_MODEL( ENT(pev), "models/w_weaponbox.mdl");
}

//=========================================================
// CWeaponBox - Kill - the think function that removes the
// box from the world.
//=========================================================
void CWeaponBox::Kill( void )
{
	CBasePlayerItem *pWeapon;
	int i;

	// destroy the weapons
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pWeapon = m_rgpPlayerItems[ i ];

		while ( pWeapon )
		{
			pWeapon->SetThink(&CBaseEntity::SUB_Remove);
			pWeapon->pev->nextthink = gpGlobals->time + 0.1;
			pWeapon = NULL;//pWeapon->m_pNext;
		}
	}

	// remove the box
	UTIL_Remove( this );
}

//=========================================================
// CWeaponBox - Touch: try to add my contents to the toucher
// if the toucher is a player.
//=========================================================
void CWeaponBox::Touch( CBaseEntity *pOther )
{
	if ( !(pev->flags & FL_ONGROUND ) )
	{
		return;
	}

	if ( !pOther->IsPlayer() )
	{
		// only players may touch a weaponbox.
		return;
	}

	if ( !pOther->IsAlive() )
	{
		// no dead guys.
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	int i;

// dole out ammo
	for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
	{
		if ( !FStringNull( m_rgiszAmmo[ i ] ) )
		{
			// there's some ammo of this type. 
			pPlayer->GiveAmmo( m_rgAmmo[ i ], (char *)STRING( m_rgiszAmmo[ i ] ), MaxAmmoCarry( m_rgiszAmmo[ i ] ) );

			//ALERT ( at_console, "Gave %d rounds of %s\n", m_rgAmmo[i], STRING(m_rgiszAmmo[i]) );

			// now empty the ammo from the weaponbox since we just gave it to the player
			m_rgiszAmmo[ i ] = iStringNull;
			m_rgAmmo[ i ] = 0;
		}
	}

// go through my weapons and try to give the usable ones to the player. 
// it's important the the player be given ammo first, so the weapons code doesn't refuse 
// to deploy a better weapon that the player may pick up because he has no ammo for it.
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			CBasePlayerItem *pItem;

			// have at least one weapon in this slot
      CBasePlayerItem *pit = m_rgpPlayerItems[ i ];
      int is = i;
			while ( m_rgpPlayerItems[ i ] && ( is == i || m_rgpPlayerItems[ i ] != pit )  )
			{
				//ALERT ( at_console, "trying to give %s\n", STRING( m_rgpPlayerItems[ i ]->pev->classname ) );

				/*if( !( strcmp( STRING(m_rgpPlayerItems[i]->pev->classname), "weapon_case" ) ) && !( pPlayer->m_afButtonPressed & ( IN_USE ) ) )
				{
					NotifyMid( pPlayer, NTM_USETOPICK, 1 );
					return;
				}
				if( ( pPlayer->m_iTeam == 2 ) && !( strcmp( STRING(m_rgpPlayerItems[i]->pev->classname), "pe_object_htool" ) ) && !( pPlayer->m_afButtonPressed & ( IN_USE ) ) )
				{
					NotifyMid( pPlayer, NTM_USETOHPICK, 1 );
					return;
				}
				else if( ( pPlayer->m_iTeam != 2 ) && !( strcmp( STRING(m_rgpPlayerItems[i]->pev->classname), "pe_object_htool" ) ) )
					return;*/

				pItem = m_rgpPlayerItems[ i ];
				if( pPlayer->m_pActiveItem )
					sprintf( pPlayer->m_sLastWeap, "%s", STRING(pPlayer->m_pActiveItem->pev->classname) );
				else
					sprintf( pPlayer->m_sLastWeap, "weapon_knife" );
				
				if( pPlayer->AddPlayerItem( pItem ) )
				{
					pItem->AttachToPlayer( pPlayer );
          break;
				  //m_rgpPlayerItems[ i ] = m_rgpPlayerItems[ i ]->m_pNext;// unlink this weapon from the box
					/*if( !strcmp( STRING(pItem->pev->classname), "weapon_case" ) )
					{
						MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
							WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
							WRITE_BYTE( 2 );
						MESSAGE_END( );
						pPlayer->m_bIsSpecial = TRUE;
						DisableAll( "pe_object_case" );
						DisableAll( "weapon_case" );
						DisableAll( "weaponbox" );
					}
					if( !strcmp( STRING(pItem->pev->classname), "pe_object_htool" ) )
					{
						MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
							WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
							WRITE_BYTE( 1 );
						MESSAGE_END( );
						pPlayer->pev->body = 1;
						pPlayer->m_bIsSpecial = TRUE;
						DisableAll( "pe_object_htool" );
						DisableAll( "weapon_case" );
						DisableAll( "weaponbox" );
						pPlayer->SelectItem( pPlayer->m_sLastWeap );
					}*/
				}
        else
          return;
			}
		}
	}

  DisableAll( pev );
	EMIT_SOUND( pOther->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
	SetTouch(NULL);
	UTIL_Remove(this);
}

//=========================================================
// CWeaponBox - PackWeapon: Add this weapon to the box
//=========================================================
BOOL CWeaponBox::PackWeapon( CBasePlayerItem *pWeapon )
{
	// is one of these weapons already packed in this box?
	if ( HasWeapon( pWeapon ) )
	{
		return FALSE;// box can only hold one of each weapon type
	}

	if ( pWeapon->m_pPlayer )
	{
		if ( !pWeapon->m_pPlayer->RemovePlayerItem( pWeapon ) )
		{
			// failed to unhook the weapon from the player!
			return FALSE;
		}
	}

	int iWeaponSlot = 0;
	if ( pWeapon->m_pPlayer )
		iWeaponSlot = pWeapon->iItemSlot( pWeapon->m_pPlayer );
	
	if ( m_rgpPlayerItems[ iWeaponSlot ] )
	{
		// there's already one weapon in this slot, so link this into the slot's column
		pWeapon->m_pNext = m_rgpPlayerItems[ iWeaponSlot ];	
		m_rgpPlayerItems[ iWeaponSlot ] = pWeapon;
	}
	else
	{
		// first weapon we have for this slot
		m_rgpPlayerItems[ iWeaponSlot ] = pWeapon;
		pWeapon->m_pNext = NULL;	
	}

	pWeapon->pev->spawnflags |= SF_NORESPAWN;// never respawn
	pWeapon->pev->movetype = MOVETYPE_NONE;
	pWeapon->pev->solid = SOLID_NOT;
	pWeapon->pev->effects = EF_NODRAW;
	pWeapon->pev->modelindex = 0;
	pWeapon->pev->model = iStringNull;
	pWeapon->pev->owner = edict();
	pWeapon->SetThink( NULL );// crowbar may be trying to swing again, etc.
	pWeapon->SetTouch( NULL );
	pWeapon->m_pPlayer = NULL;

	//ALERT ( at_console, "packed %s\n", STRING(pWeapon->pev->classname) );

	return TRUE;
}

//=========================================================
// CWeaponBox - PackAmmo
//=========================================================
BOOL CWeaponBox::PackAmmo( int iszName, int iCount )
{
	int iMaxCarry;

	if ( FStringNull( iszName ) )
	{
		// error here
		ALERT ( at_console, "NULL String in PackAmmo!\n" );
		return FALSE;
	}
	
	iMaxCarry = MaxAmmoCarry( iszName );

	if ( iMaxCarry != -1 && iCount > 0 )
	{
		//ALERT ( at_console, "Packed %d rounds of %s\n", iCount, STRING(iszName) );
		GiveAmmo( iCount, (char *)STRING( iszName ), iMaxCarry );
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CWeaponBox - GiveAmmo
//=========================================================
int CWeaponBox::GiveAmmo( int iCount, char *szName, int iMax, int *pIndex/* = NULL*/ )
{
	int i;

	for (i = 1; i < MAX_AMMO_SLOTS && !FStringNull( m_rgiszAmmo[i] ); i++)
	{
		if (stricmp( szName, STRING( m_rgiszAmmo[i])) == 0)
		{
			if (pIndex)
				*pIndex = i;

			int iAdd = min( iCount, iMax - m_rgAmmo[i]);
			if (iCount == 0 || iAdd > 0)
			{
				m_rgAmmo[i] += iAdd;

				return i;
			}
			return -1;
		}
	}
	if (i < MAX_AMMO_SLOTS)
	{
		if (pIndex)
			*pIndex = i;

		m_rgiszAmmo[i] = MAKE_STRING( szName );
		m_rgAmmo[i] = iCount;

		return i;
	}
	ALERT( at_console, "out of named ammo slots\n");
	return i;
}

//=========================================================
// CWeaponBox::HasWeapon - is a weapon of this type already
// packed in this box?
//=========================================================
BOOL CWeaponBox::HasWeapon( CBasePlayerItem *pCheckItem )
{
	CBasePlayerItem *pItem;
	if( pCheckItem->m_pPlayer )
		pItem = m_rgpPlayerItems[pCheckItem->iItemSlot( pCheckItem->m_pPlayer )];
	else
		pItem = m_rgpPlayerItems[0];

	while (pItem)
	{
		if (FClassnameIs( pItem->pev, STRING( pCheckItem->pev->classname) ))
		{
			return TRUE;
		}
		pItem = pItem->m_pNext;
	}

	return FALSE;
}

//=========================================================
// CWeaponBox::IsEmpty - is there anything in this box?
//=========================================================
BOOL CWeaponBox::IsEmpty( void )
{
	int i;

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			return FALSE;
		}
	}

	for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
	{
		if ( !FStringNull( m_rgiszAmmo[ i ] ) )
		{
			// still have a bit of this type of ammo
			return FALSE;
		}
	}

	return TRUE;
}

//=========================================================
//=========================================================
void CWeaponBox::SetObjectCollisionBox( void )
{
	pev->absmin = pev->origin + Vector(-16, -16, 0);
	pev->absmax = pev->origin + Vector(16, 16, 16); 
}

int CBasePlayerWeapon::iItemSlot( CBasePlayer *pPlayer )
{
	if( !pPlayer )
		return -1;
	if( pPlayer->m_iSlots[0] == m_iId )
		return 2;
	else if( pPlayer->m_iSlots[1] == m_iId )
		return 3;

	return -1;
}

void CBasePlayerWeapon::PrintState( void )
{
	ALERT( at_console, "primary:  %f\n", m_flNextPrimaryAttack );
	ALERT( at_console, "idle   :  %f\n", m_flTimeWeaponIdle );

//	ALERT( at_console, "nextrl :  %f\n", m_flNextReload );
//	ALERT( at_console, "nextpum:  %f\n", m_flPumpTime );

//	ALERT( at_console, "m_frt  :  %f\n", m_fReloadTime );
	ALERT( at_console, "m_finre:  %i\n", m_fInReload );
//	ALERT( at_console, "m_finsr:  %i\n", m_fInSpecialReload );

	ALERT( at_console, "m_iclip:  %i\n", m_iClip );
}
 
