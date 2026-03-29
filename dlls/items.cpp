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
/*

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "../cl_dll/pe_help_topics.h"
#include "pe_utils.h"

extern int gmsgItemPickup;

class CBBItem : public CBaseEntity
{
public:
	void	KeyValue(KeyValueData *pkvd ); 
	void	Spawn( void );
	char		type[128];
  int give;
};

class CWorldItem : public CBaseEntity
{
public:
	void	KeyValue(KeyValueData *pkvd ); 
	void	Spawn( void );
	int		m_iType;
};

LINK_ENTITY_TO_CLASS(bb_equipment, CBBItem);

void CBBItem::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wepname"))
	{
		strncpy( type, pkvd->szValue, 128 );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "ammo"))
	{
		give = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CBBItem::Spawn( void )
{
	CBaseEntity *pEntity = NULL;

	pEntity = CBaseEntity::Create( type, pev->origin, pev->angles );

	if (!pEntity)
	{
		ALERT( at_console, "unable to create world_item %s\n", type );
	}
	else
	{
		pEntity->pev->classname = ALLOC_STRING( type );
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
		pEntity->pev->fuser4 = give;
		pEntity->pev->fuser3 = 0;
	}

	REMOVE_ENTITY(edict());
}

/*class CBBAmmo : public CBaseEntity
{
public:
	void	KeyValue(KeyValueData *pkvd ); 
	void	Spawn( void );
	char		type[128];
  int give;
};

LINK_ENTITY_TO_CLASS(bb_ammo, CBBAmmo);

void CBBAmmo::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "wepname"))
	{
		strncpy( type, pkvd->szValue, 128 );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "ammo"))
	{
		give = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CBBAmmo::Spawn( void )
{
	CBaseEntity *pEntity = NULL;

	pEntity = CBaseEntity::Create( type, pev->origin, pev->angles );

	if (!pEntity)
	{
		ALERT( at_console, "unable to create world_item %s\n", type );
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY(edict());
}*/
#define AMMO_GIVE 10
#define AMMO_TOTAL 200
class CBBAmmo : public CBaseEntity
{
  int give;
  char type[256];
  int ammoLeft;
  float nextCharge;

  void KeyValue(KeyValueData *pkvd)
  {
    if (FStrEq(pkvd->szKeyName, "ammo"))
	  {
		  give = atoi(pkvd->szValue);
		  pkvd->fHandled = TRUE;
	  }
  }
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/mapobjects/ammo.mdl");
		pev->movetype = MOVETYPE_NONE;
	  pev->solid = SOLID_TRIGGER;

    TraceResult tr;
		UTIL_TraceLine ( pev->origin, pev->origin - Vector ( 0, 0, 2048 ), dont_ignore_monsters, ENT(pev), &tr );
		pev->origin.z = tr.vecEndPos.z + 1;

    UTIL_SetOrigin( pev, pev->origin );
	  UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN * 3 , Vector( 16 * 3, 16 * 3, 72 ) );

	  SetTouch( NULL );
    //SetUse( NULL );
    //SetThink( Recharge );
    //pev->nextthink = gpGlobals->time + 1;
    SetThink( NULL );
    SetUse( NULL );
    nextCharge = gpGlobals->time + 1;
	}

	void Precache( void )
	{
		PRECACHE_MODEL ("models/mapobjects/ammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}

  void Recharge( )
  {
    ammoLeft = AMMO_TOTAL;
    pev->effects &= ~EF_NODRAW;
    SetThink( NULL );
    SetUse( NULL );
    nextCharge = gpGlobals->time + 1;
  }

	void Touch( CBaseEntity *pOther )
	{ 
    if( !FClassnameIs( pOther->edict( ), "player" ) )
      return;

    CBasePlayer *pp = (CBasePlayer*)pOther;
    if( pp->m_iTeam == 2 )
      return;
    if( !( pp->pev->button & ( IN_USE ) ) || gpGlobals->time < nextCharge || !pp || !pp->m_pActiveItem )
    {
      Help( HELP_AMMO, pp );
      return;
    }
    nextCharge = gpGlobals->time + 1;

    /*if( ammoLeft <= 0 )
    {
      pev->effects |= EF_NODRAW;
      SetUse( NULL );
      SetThink( Recharge );
      pev->nextthink = gpGlobals->time + 30;
      return;
    }*/
	  strcpy( type, STRING(pp->m_pActiveItem->pev->classname) );
    //CBasePlayerItem *item = pp->ItemByName( type );
	int i;
    for( i = 0; i < MAX_WEAPONS; i++ )
      if( CBasePlayerItem::ItemInfoArray[i].pszName && !strcmp( CBasePlayerItem::ItemInfoArray[i].pszName, type ) )
        break;
    ItemInfo ii = CBasePlayerItem::ItemInfoArray[i];
 		bool r1 = ( pOther->GiveAmmo( max( 1, (int)( ii.iMaxClip / 2.0f ) ), (char*)ii.pszAmmo1, ii.iMaxAmmo1 ) != -1 );
		bool r2 = ( pOther->GiveAmmo( max( 1, (int)( ii.iMaxClip2 / 2.0f ) ), (char*)ii.pszAmmo2, ii.iMaxAmmo2 ) != -1 );
    if( r1 || r2 )
		{
      //ammoLeft -= AMMO_GIVE;
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}

	}
};
LINK_ENTITY_TO_CLASS(bb_ammo, CBBAmmo);

/*LINK_ENTITY_TO_CLASS(world_item, CWorldItem);

void CWorldItem::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_iType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CWorldItem::Spawn( void )
{
	CBaseEntity *pEntity = NULL;

	switch (m_iType) 
	{
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create( "item_battery", pev->origin, pev->angles );
		break;
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create( "item_antidote", pev->origin, pev->angles );
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create( "item_security", pev->origin, pev->angles );
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create( "item_suit", pev->origin, pev->angles );
		break;
	}

	if (!pEntity)
	{
		ALERT( at_console, "unable to create world_item %d\n", m_iType );
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY(edict());
}
*/

void CItem::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CItem::ItemTouch);

	if (DROP_TO_FLOOR(ENT(pev)) == 0)
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ), pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove( this );
		return;
	}
}

extern int gEvilImpulse101;

void CItem::ItemTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
	{
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// ok, a player is touching this item, but can he have it?
	if ( !g_pGameRules->CanHaveItem( pPlayer, this ) )
	{
		// no? Ignore the touch.
		return;
	}

	if (MyTouch( pPlayer ))
	{
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );
		
		// player grabbed the item. 
		g_pGameRules->PlayerGotItem( pPlayer, this );
		if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
		{
			Respawn(); 
		}
		else
		{
			UTIL_Remove( this );
		}
	}
	else if (gEvilImpulse101)
	{
		UTIL_Remove( this );
	}
}

CBaseEntity* CItem::Respawn( void )
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin( pev, g_pGameRules->VecItemRespawnSpot( this ) );// blip to whereever you should respawn.

	SetThink ( &CItem::Materialize );
	pev->nextthink = g_pGameRules->FlItemRespawnTime( this ); 
	return this;
}

void CItem::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch( &CItem::ItemTouch );
}

