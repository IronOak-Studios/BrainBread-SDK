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

===== combat.cpp ========================================================

  functions dealing with damage infliction & death

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "soundent.h"
#include "decals.h"
#include "animation.h"
#include "weapons.h"
#include "func_break.h"
#include "player.h"
#include "game.h"
#include "pe_utils.h"
#include "../cl_dll/pe_help_topics.h"
#include "list"
using namespace std;


extern DLL_GLOBAL Vector		g_vecAttackDir;
extern DLL_GLOBAL int			g_iSkillLevel;

extern Vector VecBModelOrigin( entvars_t* pevBModel );
extern entvars_t *g_pevLastInflictor;
 
extern BOOL g_fGameOver;

list<CBaseEntity*> zombiepool;
list<CBaseEntity*> gibpool;
int zombiecount = 0;
int gibcount = 0;

#define GERMAN_GIB_COUNT		4
#define	HUMAN_GIB_COUNT			6
#define ALIEN_GIB_COUNT			4

#define CB_TORSO		(1<<0)
#define CB_ARMS			(1<<1)
#define CB_LEGS			(1<<2)
#define CB_HEAD			(1<<3)
#define CB_NANO			(1<<4)

// HACKHACK -- The gib velocity equations don't work
void CGib :: LimitVelocity( void )
{
	float length = pev->velocity.Length();

	// ceiling at 1500.  The gib velocity equation is not bounded properly.  Rather than tune it
	// in 3 separate places again, I'll just limit it here.
	if ( length > 1500.0 )
		pev->velocity = pev->velocity.Normalize() * 1500;		// This should really be sv_maxvelocity * 0.75 or something
}


void CGib :: SpawnStickyGibs( entvars_t *pevVictim, Vector vecOrigin, int cGibs )
{
  return;
	int i;

/*	if ( g_Language == LANGUAGE_GERMAN )
	{
		// no sticky gibs in germany right now!
		return; 
	}*/

	for ( i = 0 ; i < cGibs ; i++ )
	{
		CGib *pGib = GetClassPtr( (CGib *)NULL );

		pGib->Spawn( "models/stickygib.mdl" );
		pGib->pev->body = RANDOM_LONG(0,2);

		if ( pevVictim )
		{
			pGib->pev->origin.x = vecOrigin.x + RANDOM_FLOAT( -3, 3 );
			pGib->pev->origin.y = vecOrigin.y + RANDOM_FLOAT( -3, 3 );
			pGib->pev->origin.z = vecOrigin.z + RANDOM_FLOAT( -3, 3 );

			/*
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * (RANDOM_FLOAT ( 0 , 1 ) );
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * (RANDOM_FLOAT ( 0 , 1 ) );
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * (RANDOM_FLOAT ( 0 , 1 ) );
			*/

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT ( -0.15, 0.15 );
			pGib->pev->velocity.y += RANDOM_FLOAT ( -0.15, 0.15 );
			pGib->pev->velocity.z += RANDOM_FLOAT ( -0.15, 0.15 );

			pGib->pev->velocity = pGib->pev->velocity * 900;

			pGib->pev->avelocity.x = RANDOM_FLOAT ( 250, 400 );
			pGib->pev->avelocity.y = RANDOM_FLOAT ( 250, 400 );

			// copy owner's blood color
			pGib->m_bloodColor = (CBaseEntity::Instance(pevVictim))->BloodColor();
		
			if ( pevVictim->health > -50)
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7;
			}
			else if ( pevVictim->health > -200)
			{
				pGib->pev->velocity = pGib->pev->velocity * 2;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 4;
			}

			
			pGib->pev->movetype = MOVETYPE_TOSS;
			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize ( pGib->pev, Vector ( 0, 0 ,0 ), Vector ( 0, 0, 0 ) );
			pGib->SetTouch ( &CGib::StickyGibTouch );
			pGib->SetThink (NULL);
		}
		pGib->LimitVelocity();
	}
}

void CGib :: SpawnHeadGib( entvars_t *pevVictim )
{
  return;
	CGib *pGib = GetClassPtr( (CGib *)NULL );

/*	if ( g_Language == LANGUAGE_GERMAN )
	{
		pGib->Spawn( "models/germangibs.mdl" );// throw one head
		pGib->pev->body = 0;
	}
	else
	{*/
		pGib->Spawn( "models/hgibs.mdl" );// throw one head
		pGib->pev->body = 0;
	//}

	if ( pevVictim )
	{
		pGib->pev->origin = pevVictim->origin + pevVictim->view_ofs;
		
		edict_t		*pentPlayer = FIND_CLIENT_IN_PVS( pGib->edict() );
		
		if ( RANDOM_LONG ( 0, 100 ) <= 5 && pentPlayer )
		{
			// 5% chance head will be thrown at player's face.
			entvars_t	*pevPlayer;

			pevPlayer = VARS( pentPlayer );
			pGib->pev->velocity = ( ( pevPlayer->origin + pevPlayer->view_ofs ) - pGib->pev->origin ).Normalize() * 300;
			pGib->pev->velocity.z += 100;
		}
		else
		{
			pGib->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
		}


		pGib->pev->avelocity.x = RANDOM_FLOAT ( 100, 200 );
		pGib->pev->avelocity.y = RANDOM_FLOAT ( 100, 300 );

		// copy owner's blood color
		pGib->m_bloodColor = (CBaseEntity::Instance(pevVictim))->BloodColor();
	
		if ( pevVictim->health > -50)
		{
			pGib->pev->velocity = pGib->pev->velocity * 0.7;
		}
		else if ( pevVictim->health > -200)
		{
			pGib->pev->velocity = pGib->pev->velocity * 2;
		}
		else
		{
			pGib->pev->velocity = pGib->pev->velocity * 4;
		}
	}
	pGib->LimitVelocity();
}

void CGib :: SpawnRandomGibs( entvars_t *pevVictim, int cGibs, int human )
{
  if( g_fGameOver || MAX_GIBCOUNT <= 0 )
    return;
	int cSplat;

	for ( cSplat = 0 ; cSplat < cGibs ; cSplat++ )
	{
    CGib *pGib = NULL;
    if( !gibpool.size( ) )
    {
      if( gibcount > MAX_GIBCOUNT )
        return;
		  pGib = GetClassPtr( (CGib *)NULL );
      gibcount++;
    }
    else
    {
      pGib = (CGib*)*gibpool.begin( );
      gibpool.pop_front( );
      //ALERT( at_console, "gibpool size: %d/%d\n", gibpool.size( ), gibcount );
    }

/*		if ( g_Language == LANGUAGE_GERMAN )
		{
			pGib->Spawn( "models/germangibs.mdl" );
			pGib->pev->body = RANDOM_LONG(0,GERMAN_GIB_COUNT-1);
		}
		else
		{*/
			if ( human )
			{
				// human pieces
				pGib->Spawn( "models/hgibs.mdl" );
				pGib->pev->body = RANDOM_LONG(1,HUMAN_GIB_COUNT-1);// start at one to avoid throwing random amounts of skulls (0th gib)
			}
			else
			{
				// aliens
				//pGib->Spawn( "models/agibs.mdl" );
				//pGib->pev->body = RANDOM_LONG(0,ALIEN_GIB_COUNT-1);
			}
		//}

		if ( pevVictim )
		{
			// spawn the gib somewhere in the monster's bounding volume
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * (RANDOM_FLOAT ( 0 , 1 ) );
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * (RANDOM_FLOAT ( 0 , 1 ) );
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * (RANDOM_FLOAT ( 0 , 1 ) ) + 1;	// absmin.z is in the floor because the engine subtracts 1 to enlarge the box

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT ( -0.25, 0.25 );
			pGib->pev->velocity.y += RANDOM_FLOAT ( -0.25, 0.25 );
			pGib->pev->velocity.z += RANDOM_FLOAT ( -0.25, 0.25 );

			pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT ( 300, 400 );

			pGib->pev->avelocity.x = RANDOM_FLOAT ( 100, 200 );
			pGib->pev->avelocity.y = RANDOM_FLOAT ( 100, 300 );

			// copy owner's blood color
			pGib->m_bloodColor = (CBaseEntity::Instance(pevVictim))->BloodColor();
			
			if ( pevVictim->health > -50)
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7;
			}
			else if ( pevVictim->health > -200)
			{
				pGib->pev->velocity = pGib->pev->velocity * 1;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 1.3;
			}

			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize ( pGib->pev, Vector( 0 , 0 , 0 ), Vector ( 0, 0, 0 ) );
		}
		pGib->LimitVelocity();
    pGib->SUB_StartFadeOut( );
    /*pGib->pev->nextthink = gpGlobals->time;
    pGib->SetThink ( SUB_FadeOut );*/
  }
}

void CGib :: SpawnGib( entvars_t *pevVictim, char *name, int human )
{
  if( g_fGameOver || MAX_GIBCOUNT <= 0 )
    return;

  CGib *pGib = NULL;
  if( !gibpool.size( ) )
  {
    if( gibcount > MAX_GIBCOUNT )
      return;
    pGib = GetClassPtr( (CGib *)NULL );
    gibcount++;
  }
  else
  {
    pGib = (CGib*)*gibpool.begin( );
    gibpool.pop_front( );
    //ALERT( at_console, "gibpool size: %d/%d\n", gibpool.size( ), gibcount );
  }

	pGib->Spawn( name );

    if ( pevVictim )
		{
			// spawn the gib somewhere in the monster's bounding volume
			pGib->pev->origin.x = pevVictim->absmin.x + pevVictim->size.x * (RANDOM_FLOAT ( 0 , 1 ) );
			pGib->pev->origin.y = pevVictim->absmin.y + pevVictim->size.y * (RANDOM_FLOAT ( 0 , 1 ) );
			pGib->pev->origin.z = pevVictim->absmin.z + pevVictim->size.z * (RANDOM_FLOAT ( 0 , 1 ) ) + 1;	// absmin.z is in the floor because the engine subtracts 1 to enlarge the box

			// make the gib fly away from the attack vector
			pGib->pev->velocity = g_vecAttackDir * -1;

			// mix in some noise
			pGib->pev->velocity.x += RANDOM_FLOAT ( -0.25, 0.25 );
			pGib->pev->velocity.y += RANDOM_FLOAT ( -0.25, 0.25 );
			pGib->pev->velocity.z += RANDOM_FLOAT ( -0.25, 0.25 );

			pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT ( 300, 400 );

			pGib->pev->avelocity.x = RANDOM_FLOAT ( 100, 200 );
			pGib->pev->avelocity.y = RANDOM_FLOAT ( 100, 300 );

			// copy owner's blood color
			pGib->m_bloodColor = (CBaseEntity::Instance(pevVictim))->BloodColor();
			
			if ( pevVictim->health > -50)
			{
				pGib->pev->velocity = pGib->pev->velocity * 0.7;
			}
			else if ( pevVictim->health > -200)
			{
				pGib->pev->velocity = pGib->pev->velocity * 1;
			}
			else
			{
				pGib->pev->velocity = pGib->pev->velocity * 1.3;
			}

			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize ( pGib->pev, Vector( 0 , 0 , 0 ), Vector ( 0, 0, 0 ) );
		}
		pGib->LimitVelocity();
    pGib->SUB_StartFadeOut( );
    /*pGib->pev->nextthink = gpGlobals->time;
    pGib->SetThink ( SUB_FadeOut );*/
}

BOOL CBaseMonster :: HasHumanGibs( void )
{
	int myClass = Classify();

	if ( myClass == CLASS_HUMAN_MILITARY ||
		 myClass == CLASS_PLAYER_ALLY	||
		 myClass == CLASS_ZOMBIE  ||
		 myClass == CLASS_PLAYER )

		 return TRUE;

	return FALSE;
}


BOOL CBaseMonster :: HasAlienGibs( void )
{
	int myClass = Classify();

	if ( myClass == CLASS_ALIEN_MILITARY ||
		 myClass == CLASS_ALIEN_MONSTER	||
		 myClass == CLASS_ALIEN_PASSIVE  ||
		 myClass == CLASS_INSECT  ||
		 myClass == CLASS_ALIEN_PREDATOR  ||
		 myClass == CLASS_ALIEN_PREY )

		 return TRUE;

	return FALSE;
}


void CBaseMonster::FadeMonster( void )
{
	StopAnimation();
	pev->velocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
	pev->avelocity = g_vecZero;
	pev->animtime = gpGlobals->time;
	pev->effects |= EF_NOINTERP;
	SUB_StartFadeOut();
}

//=========================================================
// GibMonster - create some gore and get rid of a monster's
// model.
//=========================================================
void CBaseMonster :: GibMonster( void )
{
	TraceResult	tr;
	BOOL		gibbed = FALSE;

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM);		

	// only humans throw skulls !!!UNDONE - eventually monsters will have their own sets of gibs
	if ( HasHumanGibs() )
	{
		if ( CVAR_GET_FLOAT("violence_hgibs") != 0 )	// Only the player will ever get here
		{
			//CGib::SpawnHeadGib( pev );
			CGib::SpawnRandomGibs( pev, 2, 1 );	// throw some human gibs.
			CGib::SpawnGib( pev, "models/gibs/upper.mdl", 1 );
      CGib::SpawnGib( pev, "models/gibs/arm1.mdl", 1 );

		}
		gibbed = TRUE;
	}
	else if ( HasAlienGibs() )
	{
		if ( CVAR_GET_FLOAT("violence_agibs") != 0 )	// Should never get here, but someone might call it directly
		{
			CGib::SpawnRandomGibs( pev, 3, 0 );	// Throw alien gibs
		}
		gibbed = TRUE;
	}

	if ( !IsPlayer() )
	{
		if ( gibbed )
		{
			// don't remove players!
			pev->nextthink = gpGlobals->time;
      if( !FClassnameIs( pev, "monster_zombie" ) )
			  SetThink ( &CBaseEntity::SUB_Remove );
      else
        FadeMonster( );
		}
		else
		{
			FadeMonster();
		}
	}
}

//=========================================================
// GetDeathActivity - determines the best type of death
// anim to play.
//=========================================================
Activity CBaseMonster :: GetDeathActivity ( void )
{
	Activity	deathActivity;
	BOOL		fTriedDirection;
	float		flDot;
	TraceResult	tr;
	Vector		vecSrc;

	if ( pev->deadflag != DEAD_NO )
	{
		// don't run this while dying.
		return m_IdealActivity;
	}

	vecSrc = Center();

	fTriedDirection = FALSE;
	deathActivity = ACT_DIESIMPLE;// in case we can't find any special deaths to do.

	UTIL_MakeVectors ( pev->angles );
	flDot = DotProduct ( gpGlobals->v_forward, g_vecAttackDir * -1 );

	switch ( m_LastHitGroup )
	{
		// try to pick a region-specific death.
	case HITGROUP_HEAD:
		deathActivity = ACT_DIE_HEADSHOT;
		break;

	case HITGROUP_STOMACH:
		deathActivity = ACT_DIE_GUTSHOT;
		break;

	case HITGROUP_GENERIC:
		// try to pick a death based on attack direction
		fTriedDirection = TRUE;

		if ( flDot > 0.3 )
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if ( flDot <= -0.3 )
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;

	default:
		// try to pick a death based on attack direction
		fTriedDirection = TRUE;

		if ( flDot > 0.3 )
		{
			deathActivity = ACT_DIEFORWARD;
		}
		else if ( flDot <= -0.3 )
		{
			deathActivity = ACT_DIEBACKWARD;
		}
		break;
	}


	// can we perform the prescribed death?
	if ( LookupActivity ( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		// no! did we fail to perform a directional death? 
		if ( fTriedDirection )
		{
			// if yes, we're out of options. Go simple.
			deathActivity = ACT_DIESIMPLE;
		}
		else
		{
			// cannot perform the ideal region-specific death, so try a direction.
			if ( flDot > 0.3 )
			{
				deathActivity = ACT_DIEFORWARD;
			}
			else if ( flDot <= -0.3 )
			{
				deathActivity = ACT_DIEBACKWARD;
			}
		}
	}

	if ( LookupActivity ( deathActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		// if we're still invalid, simple is our only option.
		deathActivity = ACT_DIESIMPLE;
	}

	if ( deathActivity == ACT_DIEFORWARD )
	{
			// make sure there's room to fall forward
			UTIL_TraceHull ( vecSrc, vecSrc + gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr );

			if ( tr.flFraction != 1.0 )
			{
				deathActivity = ACT_DIESIMPLE;
			}
	}

	if ( deathActivity == ACT_DIEBACKWARD )
	{
			// make sure there's room to fall backward
			UTIL_TraceHull ( vecSrc, vecSrc - gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr );

			if ( tr.flFraction != 1.0 )
			{
				deathActivity = ACT_DIESIMPLE;
			}
	}

	return deathActivity;
}

//=========================================================
// GetSmallFlinchActivity - determines the best type of flinch
// anim to play.
//=========================================================
Activity CBaseMonster :: GetSmallFlinchActivity ( void )
{
	Activity	flinchActivity;
	BOOL		fTriedDirection;
	float		flDot;

	fTriedDirection = FALSE;
	UTIL_MakeVectors ( pev->angles );
	flDot = DotProduct ( gpGlobals->v_forward, g_vecAttackDir * -1 );
	
	switch ( m_LastHitGroup )
	{
		// pick a region-specific flinch
	case HITGROUP_HEAD:
		flinchActivity = ACT_FLINCH_HEAD;
		break;
	case HITGROUP_STOMACH:
		flinchActivity = ACT_FLINCH_STOMACH;
		break;
	case HITGROUP_LEFTARM:
		flinchActivity = ACT_FLINCH_LEFTARM;
		break;
	case HITGROUP_RIGHTARM:
		flinchActivity = ACT_FLINCH_RIGHTARM;
		break;
	case HITGROUP_LEFTLEG:
		flinchActivity = ACT_FLINCH_LEFTLEG;
		break;
	case HITGROUP_RIGHTLEG:
		flinchActivity = ACT_FLINCH_RIGHTLEG;
		break;
	case HITGROUP_GENERIC:
	default:
		// just get a generic flinch.
		flinchActivity = ACT_SMALL_FLINCH;
		break;
	}


	// do we have a sequence for the ideal activity?
	if ( LookupActivity ( flinchActivity ) == ACTIVITY_NOT_AVAILABLE )
	{
		flinchActivity = ACT_SMALL_FLINCH;
	}

	return flinchActivity;
}


void CBaseMonster::BecomeDead( void )
{
	pev->takedamage = DAMAGE_YES;// don't let autoaim aim at corpses.
	
	// give the corpse half of the monster's original maximum health. 
	pev->health = pev->max_health / 2;
	pev->max_health = 5; // max_health now becomes a counter for how many blood decals the corpse can place.

	// make the corpse fly away from the attack vector
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_NOT;
  
	//pev->flags &= ~FL_ONGROUND;
	//pev->origin.z += 2;
	//pev->velocity = g_vecAttackDir * -1;
	//pev->velocity = pev->velocity * RANDOM_FLOAT( 300, 400 );
}


BOOL CBaseMonster::ShouldGibMonster( int iGib )
{

		//ALERT( at_console, "%f", pev->health );
	if ( ( iGib == GIB_NORMAL && pev->health < GIB_HEALTH_VALUE ) || ( iGib == GIB_ALWAYS ) )
	{
		//ALERT( at_console, " gibbing!\n" );
		return TRUE;
	}
	
	//ALERT( at_console, " NOT gibbing!\n" );
	return FALSE;
}


void CBaseMonster::CallGibMonster( void )
{
	BOOL fade = FALSE;

	if ( HasHumanGibs() )
	{
		if ( CVAR_GET_FLOAT("violence_hgibs") == 0 )
			fade = TRUE;
	}
	else if ( HasAlienGibs() )
	{
		if ( CVAR_GET_FLOAT("violence_agibs") == 0 )
			fade = TRUE;
	}

	pev->takedamage = DAMAGE_NO;
	pev->solid = SOLID_NOT;// do something with the body. while monster blows up

	if ( fade )
	{
		FadeMonster();
	}
	else
	{
		pev->effects = EF_NODRAW; // make the model invisible.
		GibMonster();
	}

	pev->deadflag = DEAD_DEAD;
	FCheckAITrigger();

	// don't let the status bar glitch for players.with <0 health.
	if (pev->health < -99)
	{
		pev->health = 0;
	}
	
	if ( ShouldFadeOnDeath() && !fade )
		UTIL_Remove(this);
}


/*
============
Killed
============
*/
extern int gmsgDeathMsg;
void CBaseMonster :: Killed( entvars_t *pevAttacker, int iGib )
{
	unsigned int	cCount = 0;
	BOOL			fDone = FALSE;

	if ( HasMemory( bits_MEMORY_KILLED ) )
	{
		if ( ShouldGibMonster( iGib ) )
			CallGibMonster();
		return;
	}

	Remember( bits_MEMORY_KILLED );

	// clear the deceased's sound channels.(may have been firing or reloading when killed)
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM);
	m_IdealMonsterState = MONSTERSTATE_DEAD;
	// Make sure this condition is fired too (TakeDamage breaks out before this happens on death)
	SetConditions( bits_COND_LIGHT_DAMAGE );
	
	if( pevAttacker && pevAttacker->flags & FL_CLIENT )
	{
		const char *killer_weapon_name = "world";
		int killer_index = 0;
		int num = 0;
		killer_index = ENTINDEX(ENT(pevAttacker));
		
		CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( pevAttacker );
		if ( !pPlayer )
			return;
		if ( pPlayer->m_pActiveItem )
		{
			killer_weapon_name = pPlayer->m_pActiveItem->pszName();
		}

		// strip the monster_* or weapon_* from the inflictor's classname
		if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
			killer_weapon_name += 7;
		else if ( strncmp( killer_weapon_name, "monster_", 8 ) == 0 )
			killer_weapon_name += 8;
		else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
			killer_weapon_name += 5;

		if( FClassnameIs( pev, "monster_zombie" ) )
    {
      if( !pev->fuser4 )
			  num = 65;
      else
        num = 67;      
    }
		else if (FClassnameIs(pev, "monster_human_grunt") || FClassnameIs(pev, "monster_hgrunt") || FClassnameIs(pev, "monster_hgrunt_shotgun"))
    {
      pevAttacker->frags += 2;
      CBaseEntity *ent = Instance( pevAttacker );
      if( ent->IsPlayer( ) )
        ((CBasePlayer*)ent)->zombieKills -= 2.5;
      num = 66;
    }
    else if( FClassnameIs( pev, "monster_barney" ) )
    {
      pevAttacker->frags += 2;
      CBaseEntity *ent = Instance( pevAttacker );
      if( ent->IsPlayer( ) )
        ((CBasePlayer*)ent)->zombieKills -= 2.5;
      num = 68;
    }

		if( m_LastHitGroup == 1 )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
				WRITE_BYTE( killer_index );						// the killer
				WRITE_BYTE( num );		// the victim
				WRITE_STRING( "headshot" );
				WRITE_STRING( killer_weapon_name );
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
				WRITE_BYTE( killer_index );						// the killer
				WRITE_BYTE( num );		// the victim
				WRITE_STRING( killer_weapon_name );		// what they were killed by (should this be a string?)
			MESSAGE_END();
		}
	}

	// tell owner ( if any ) that we're dead.This is mostly for MonsterMaker functionality.
	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
	if ( pOwner )
	{
		pOwner->DeathNotice( pev );
	}

	if( ShouldGibMonster( iGib ) )
	{
		CallGibMonster();
		return;
	}
	else if ( pev->flags & FL_MONSTER )
	{
		SetTouch( NULL );
		BecomeDead();
	}
	
	// don't let the status bar glitch for players.with <0 health.
	if (pev->health < -99)
	{
		pev->health = 0;
	}
	
	//pev->enemy = ENT( pevAttacker );//why? (sjb)
	
	m_IdealMonsterState = MONSTERSTATE_DEAD;
}

//
// fade out - slowly fades a entity out, then removes it.
//
// DON'T USE ME FOR GIBS AND STUFF IN MULTIPLAYER! 
// SET A FUTURE THINK AND A RENDERMODE!!
void CBaseEntity :: SUB_StartFadeOut ( void )
{
	if (pev->rendermode == kRenderNormal)
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransTexture;
	}

	//pev->solid = SOLID_NOT;
	pev->avelocity = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.5;
	SetThink ( &CBaseEntity::SUB_FadeOut );
}

void CBaseEntity :: SUB_FadeOut ( void  )
{
	if ( pev->renderamt > 7 )
	{
		pev->renderamt -= 7;
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else 
	{
		pev->renderamt = 0;
		pev->nextthink = gpGlobals->time + 0.2;
    if( FClassnameIs( pev, "monster_zombie" ) && !pev->fuser4 )
    {
      SetThink( NULL );
      SetBits( pev->flags, FL_DORMANT );
      ClearBits( pev->flags, FL_MONSTER );
      zombiepool.push_back( this );
      //ALERT( at_logged, "zombies in list: %d/%d\n", zombiepool.size( ), zombiecount );
    }
    else if( FClassnameIs( pev, "gib" ) )
    {
      SetThink( NULL );
      SetBits( pev->flags, FL_DORMANT );
      gibpool.push_back( this );
      //ALERT( at_logged, "gibpool size: %d/%d\n", gibpool.size( ), gibcount );
    }
    else
  	  SetThink ( &CBaseEntity::SUB_Remove );
	}
}

//=========================================================
// WaitTillLand - in order to emit their meaty scent from
// the proper location, gibs should wait until they stop 
// bouncing to emit their scent. That's what this function
// does.
//=========================================================
void CGib :: WaitTillLand ( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	if ( pev->velocity == g_vecZero )
	{
		SetThink (&CBaseEntity::SUB_StartFadeOut);
		pev->nextthink = gpGlobals->time + m_lifeTime;

		// If you bleed, you stink!
		if ( m_bloodColor != DONT_BLEED )
		{
			// ok, start stinkin!
			CSoundEnt::InsertSound ( bits_SOUND_MEAT, pev->origin, 384, 25 );
		}
	}
	else
	{
		// wait and check again in another half second.
		pev->nextthink = gpGlobals->time + 0.5;
	}
}

//
// Gib bounces on the ground or wall, sponges some blood down, too!
//
void CGib :: BounceGibTouch ( CBaseEntity *pOther )
{
	Vector	vecSpot;
	TraceResult	tr;
	
	//if ( RANDOM_LONG(0,1) )
	//	return;// don't bleed everytime

	if (pev->flags & FL_ONGROUND)
	{
		pev->velocity = pev->velocity * 0.9;
		pev->angles.x = 0;
		pev->angles.z = 0;
		pev->avelocity.x = 0;
		pev->avelocity.z = 0;
	}
	else
	{
		if ( /*g_Language != LANGUAGE_GERMAN &&*/ m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED )
		{
			vecSpot = pev->origin + Vector ( 0 , 0 , 8 );//move up a bit, and trace down.
			UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -24 ),  ignore_monsters, ENT(pev), & tr);

			UTIL_BloodDecalTrace( &tr, m_bloodColor );

			m_cBloodDecals--; 
		}

		if ( m_material != matNone && RANDOM_LONG(0,2) == 0 )
		{
			float volume;
			float zvel = fabs(pev->velocity.z);
		
			volume = 0.8 * min(1.0, ((float)zvel) / 450.0);

			CBreakable::MaterialSoundRandom( edict(), (Materials)m_material, volume );
		}
	}
}

//
// Sticky gib puts blood on the wall and stays put. 
//
void CGib :: StickyGibTouch ( CBaseEntity *pOther )
{
	Vector	vecSpot;
	TraceResult	tr;
	
	/*SetThink ( SUB_Remove );
	pev->nextthink = gpGlobals->time + 10;*/

	if ( !FClassnameIs( pOther->pev, "worldspawn" ) )
	{
		pev->nextthink = gpGlobals->time;
		return;
	}

	UTIL_TraceLine ( pev->origin, pev->origin + pev->velocity * 32,  ignore_monsters, ENT(pev), & tr);

	UTIL_BloodDecalTrace( &tr, m_bloodColor );

	pev->velocity = tr.vecPlaneNormal * -1;
	pev->angles = UTIL_VecToAngles ( pev->velocity );
	pev->velocity = g_vecZero; 
	pev->avelocity = g_vecZero;
	pev->movetype = MOVETYPE_NONE;
}

//
// Throw a chunk
//
void CGib :: Spawn( const char *szGibModel )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.75; // deading the bounce a bit
	
	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or rendermode! bad!
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_SLIDEBOX;/// hopefully this will fix the VELOCITY TOO LOW crap
	pev->classname = MAKE_STRING("gib");
  ClearBits( pev->flags, FL_DORMANT );
  ClearBits( pev->effects, EF_NODRAW );

	SET_MODEL(ENT(pev), szGibModel);
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));

	pev->nextthink = gpGlobals->time + 4;
	m_lifeTime = 5;
	SetThink ( &CGib::WaitTillLand );
	SetTouch ( &CGib::BounceGibTouch );

	m_material = matNone;
	m_cBloodDecals = 5;// how many blood decals this gib can place (1 per bounce until none remain). 
}

// take health
int CBaseMonster :: TakeHealth (float flHealth, int bitsDamageType)
{
	if (!pev->takedamage)
		return 0;

	// clear out any damage types we healed.
	// UNDONE: generic health should not heal any
	// UNDONE: time-based damage

	m_bitsDamageType &= ~(bitsDamageType & ~DMG_TIMEBASED);
	
	return CBaseEntity::TakeHealth(flHealth, bitsDamageType);
}

/*
============
TakeDamage

The damage is coming from inflictor, but get mad at attacker
This should be the only function that ever reduces health.
bitsDamageType indicates the type of damage sustained, ie: DMG_SHOCK

Time-based damage: only occurs while the monster is within the trigger_hurt.
When a monster is poisoned via an arrow etc it takes all the poison damage at once.



GLOBALS ASSUMED SET:  g_iSkillLevel
============
*/
extern int gmsgSmallCnt;
int CBaseMonster :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	float	flTake;
	float prevh = pev->health;
	Vector	vecDir;

	if (!pev->takedamage)
		return 0;

	if ( !IsAlive() )
	{
		return DeadTakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	}

	if ( pev->deadflag == DEAD_NO )
	{
		// no pain sound during death animation.
		PainSound();// "Ouch!"
	}

	//!!!LATER - make armor consideration here!
	flTake = flDamage;

	// set damage type sustained
	m_bitsDamageType |= bitsDamageType;

	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	vecDir = Vector( 0, 0, 0 );
	if (!FNullEnt( pevInflictor ))
	{
		CBaseEntity *pInflictor = CBaseEntity :: Instance( pevInflictor );
		if (pInflictor)
		{
			vecDir = ( pInflictor->Center() - Vector ( 0, 0, 10 ) - Center() ).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

	// add to the damage total for clients, which will be sent as a single
	// message at the end of the frame
	// todo: remove after combining shotgun blasts?
	if ( IsPlayer() )
	{
		if ( pevInflictor )
			pev->dmg_inflictor = ENT(pevInflictor);

		pev->dmg_take += flTake;

		// check for godmode or invincibility
		if ( pev->flags & FL_GODMODE )
		{
			return 0;
		}
	}

	// if this is a player, move him around!
	if ( ( !FNullEnt( pevInflictor ) ) && ( pev->movetype == MOVETYPE_WALK /*|| pev->movetype == MOVETYPE_STEP*/ ) && (!pevAttacker || pevAttacker->solid != SOLID_TRIGGER) )
	{
		pev->velocity = pev->velocity + vecDir * ( -DamageForce( pev, flDamage ) * ( pev->fuser4 ? 0.2 : 1 ) );
	}

	// zombies don't damage other zombies
	if( !FNullEnt( pevInflictor ) && pevInflictor->team == 2 && pev->team == 2
		&& FClassnameIs( pevInflictor, "monster_zombie" ) )
	{
		return 0;
	}

	// do the damage
  //if( !IsPlayer( ) || pev->health - flTake > 0 || !FNullEnt( pevInflictor ) && !FClassnameIs( pevInflictor, "monster_zombie" ) )
  if( !IsPlayer( ) || FNullEnt( pevInflictor ) || !FClassnameIs( pevInflictor, "monster_zombie" ) ||
      ( FClassnameIs( pevInflictor, "monster_zombie" ) && pev->team != 2 && pev->health - flTake > 0 ) ||
      ( FClassnameIs( pev, "monster_zombie" ) && pevInflictor->team != 2 ) )
  {
    if( ( FClassnameIs( pev, "monster_barney" ) && pevInflictor->team == 1 ) ||
      ( FClassnameIs( pev, "monster_hgrunt" ) && pevInflictor->team == 1 ) ||
			( FClassnameIs( pev, "monster_human_grunt" ) && pevInflictor->team == 1 ) ||
      ( FClassnameIs( pev, "monster_hgrunt_shotgun" ) && pevInflictor->team == 1 ) )
    {
    }
    else
    {
	    pev->health -= flTake;
      if( pev->health <= 1 && IsPlayer( ) && pev->team == 1 && FClassnameIs( pevInflictor, "monster_zombie" ) )
      {
        //char text[256];
        //sprintf( text, "%s -> Zombie", STRING( pev->netname ) );
        ForceHelp( HELP_START_TRANSFORM, (CBasePlayer*)this );
        MESSAGE_BEGIN( MSG_ONE, gmsgSmallCnt, NULL, edict( ) );
			    WRITE_STRING( "Zombie transformation" );//text );
			    WRITE_COORD( 20 );
		    MESSAGE_END( );
        ((CBasePlayer*)this)->m_fTransformTime = gpGlobals->time + 20;
        ((CBasePlayer*)this)->m_bTransform = TRUE;
	    pev->health = 1;
      }
    }
  }
  else
  {
    if( pev->health != 1 && IsPlayer( ) && pev->team == 1 && !((CBasePlayer*)this)->m_bTransform )
    {
      ForceHelp( HELP_START_TRANSFORM, (CBasePlayer*)this );
      MESSAGE_BEGIN( MSG_ONE, gmsgSmallCnt, NULL, edict( ) );
			  WRITE_STRING( "Zombie transformation" );
			  WRITE_COORD( 20 );
		  MESSAGE_END( );
      ((CBasePlayer*)this)->m_fTransformTime = gpGlobals->time + 20;
      ((CBasePlayer*)this)->m_bTransform = TRUE;
    }
    pev->health = 1;

  }
  if( prevh > 0 )
  {
	  float h2 = max( 0.0f, pev->health );
	  float flActualDmg = prevh - h2;
	  if( pevInflictor )
	  {
      Instance( pevInflictor )->GiveExp( ( IsPlayer( ) ? 2 : 1 ) * flActualDmg );

      // Track round damage for the player who dealt it
      CBaseEntity *pInf = Instance( pevInflictor );
      if( pInf && pInf->IsPlayer() )
        ((CBasePlayer*)pInf)->m_flRoundDamageDealt += flActualDmg;
	  }
  }

	if( ( !FNullEnt( pevInflictor ) ) && ( pev->health <= 0 ) && (!pevAttacker || pevAttacker->solid != SOLID_TRIGGER) )
	{
		pev->velocity = pev->velocity + vecDir * ( -DamageForce( pev, flDamage ) * 3 );
	}
	
	// HACKHACK Don't kill monsters in a script.  Let them break their scripts first
	if ( m_MonsterState == MONSTERSTATE_SCRIPT )
	{
		SetConditions( bits_COND_LIGHT_DAMAGE );
		return 0;
	}

	if ( pev->health <= 0 )
	{
		g_pevLastInflictor = pevInflictor;

		if ( bitsDamageType & DMG_ALWAYSGIB )
		{
			Killed( pevAttacker, GIB_ALWAYS );
		}
		else if ( bitsDamageType & DMG_NEVERGIB )
		{
			Killed( pevAttacker, GIB_NEVER );
		}
		else
		{
			Killed( pevAttacker, GIB_NORMAL );
		}

		g_pevLastInflictor = NULL;

		return 0;
	}

	// react to the damage (get mad)
	if ( (pev->flags & FL_MONSTER) && !FNullEnt(pevAttacker) )
	{
		if ( pevAttacker->flags & (FL_MONSTER | FL_CLIENT) )
		{// only if the attack was a monster or client!
			
			// enemy's last known position is somewhere down the vector that the attack came from.
			if (pevInflictor)
			{
				if (m_hEnemy == NULL || pevInflictor == m_hEnemy->pev || !HasConditions(bits_COND_SEE_ENEMY))
				{
					m_vecEnemyLKP = pevInflictor->origin;
				}
			}
			else
			{
				m_vecEnemyLKP = pev->origin + ( g_vecAttackDir * 64 ); 
			}

			MakeIdealYaw( m_vecEnemyLKP );

			// add pain to the conditions 
			// !!!HACKHACK - fudged for now. Do we want to have a virtual function to determine what is light and 
			// heavy damage per monster class?
			if ( flDamage > 0 )
			{
				SetConditions(bits_COND_LIGHT_DAMAGE);
			}

			if ( flDamage >= 20 )
			{
				SetConditions(bits_COND_HEAVY_DAMAGE);
			}
		}
	}

	return 1;
}

//=========================================================
// DeadTakeDamage - takedamage function called when a monster's
// corpse is damaged.
//=========================================================
int CBaseMonster :: DeadTakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Vector			vecDir;

	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	vecDir = Vector( 0, 0, 0 );
	if (!FNullEnt( pevInflictor ))
	{
		CBaseEntity *pInflictor = CBaseEntity :: Instance( pevInflictor );
		if (pInflictor)
		{
			vecDir = ( pInflictor->Center() - Vector ( 0, 0, 10 ) - Center() ).Normalize();
			vecDir = g_vecAttackDir = vecDir.Normalize();
		}
	}

#if 0// turn this back on when the bounding box issues are resolved.

	pev->flags &= ~FL_ONGROUND;
	pev->origin.z += 1;
	
	// let the damage scoot the corpse around a bit.
	if ( !FNullEnt(pevInflictor) && (pevAttacker->solid != SOLID_TRIGGER) )
	{
		pev->velocity = pev->velocity + vecDir * -DamageForce( pev, flDamage );
	}

#endif

	// kill the corpse if enough damage was done to destroy the corpse and the damage is of a type that is allowed to destroy the corpse.
	if ( bitsDamageType & DMG_GIB_CORPSE )
	{
		if ( pev->health <= flDamage )
		{
			pev->health = -50;
			Killed( pevAttacker, GIB_ALWAYS );
			return 0;
		}
		// Accumulate corpse gibbing damage, so you can gib with multiple hits
		pev->health -= flDamage * 0.1;
	}
	
	return 1;
}


float CBaseMonster :: DamageForce( entvars_t *pev, float damage )
{ 
	float flVolume = pev->size.x * pev->size.y * pev->size.z;
	float force = (flVolume != 0) ? damage * ((32 * 32 * 72.0) / flVolume) * 5 : 0;
	
	if ( force > 1000.0) 
	{
		force = 1000.0;
	}

	return force;
}

//
// RadiusDamage - this entity is exploding, or otherwise needs to inflict damage upon entities within a certain range.
// 
// only damage ents that can clearly be seen by the explosion!

	
void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	if ( flRadius )
		falloff = flDamage / flRadius;
	else
		falloff = 1.0;

	int bInWater = (UTIL_PointContents ( vecSrc ) == CONTENTS_WATER);

	vecSrc.z += 1;// in case grenade is lying on the ground

	if ( !pevAttacker )
		pevAttacker = pevInflictor;

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, flRadius )) != NULL)
	{
    const char *a = STRING( pEntity->pev->classname );
    const char *b = STRING( pevAttacker->classname );
    const char *c = STRING( pevInflictor->classname );
		if ( pEntity->pev->takedamage != DAMAGE_NO && ( ( pEntity->pev->team != pevAttacker->team ) || ( pEntity->pev == pevAttacker ) || ( FClassnameIs( pEntity->pev, "grenade" ) && FClassnameIs( pevInflictor, "grenade" ) ) ) )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{// houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			// blast's don't tavel into or out of water
			/*if (bInWater && pEntity->pev->waterlevel == 0)
				continue;
			if (!bInWater && pEntity->pev->waterlevel == 3)
				continue;*/

			vecSpot = pEntity->BodyTarget( vecSrc );
			
			UTIL_TraceLine ( vecSrc, vecSpot, dont_ignore_monsters, ENT(pevInflictor), &tr );

			//if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() )
			//{// the explosion can 'see' this entity, so hurt them!
			//	if (tr.fStartSolid)
			//	{
					// if we're stuck inside them, fixup the position and distance
			//		tr.vecEndPos = vecSrc;
			//		tr.flFraction = 0.0;
			//	}
				
				// decrease damage for an ent that's farther from the bomb.
				flAdjustedDamage = ( vecSrc - tr.vecEndPos ).Length() * falloff;
				flAdjustedDamage = flDamage - flAdjustedDamage;
			
				if ( flAdjustedDamage < 0 )
				{
					flAdjustedDamage = 0;
				}
			
				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
			/*	if (tr.flFraction != 1.0)
				{
					ClearMultiDamage( );
					pEntity->TraceAttack( pevInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize( ), &tr, bitsDamageType );
					ApplyMultiDamage( pevInflictor, pevAttacker );
				}
				else
				{*/
				pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				//}
			CBasePlayer *pa = (CBasePlayer*)CBaseEntity::Instance(pevAttacker);

			if( pa && pa->IsPlayer( ) )
				pa->AddDmgDone( ENTINDEX(ENT(pEntity->pev)), flAdjustedDamage, 3 );
				if( pEntity->IsPlayer( ) )
					((CBasePlayer*)pEntity)->AddDmgReceived( ENTINDEX(ENT(pevAttacker)), flAdjustedDamage, 3 );
		//	}
		}
	}
}

//			flAdjustedDamage = flDamage * ( pow( flRadius - (pEntity->pev->origin - vecSrc).Length( ), 2 ) / pow( flRadius, 2 ) );


void CBaseMonster :: RadiusDamage(entvars_t* pevInflictor, entvars_t*	pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage( pev->origin, pevInflictor, pevAttacker, flDamage, flDamage * 2.5, iClassIgnore, bitsDamageType );
}


void CBaseMonster :: RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage( vecSrc, pevInflictor, pevAttacker, flDamage, flDamage * 2.5, iClassIgnore, bitsDamageType );
}


//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount 
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
//
// Used for many contact-range melee attacks. Bites, claws, etc.
//=========================================================
CBaseEntity* CBaseMonster :: CheckTraceHullAttack( float flDist, int iDamage, int iDmgType )
{
	TraceResult tr;

	if (IsPlayer())
		UTIL_MakeVectors( pev->angles );
	else
		UTIL_MakeAimVectors( pev->angles );

	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist );

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if ( iDamage > 0 )
		{
			pEntity->TakeDamage( pev, pev, iDamage, iDmgType );
		}

		return pEntity;
	}

	return NULL;
}


//=========================================================
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
BOOL CBaseMonster :: FInViewCone ( CBaseEntity *pEntity )
{
	Vector2D	vec2LOS;
	float	flDot;

	UTIL_MakeVectors ( pev->angles );
	
	vec2LOS = ( pEntity->pev->origin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

	if ( flDot > m_flFieldOfView )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// FInViewCone - returns true is the passed vector is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//=========================================================
BOOL CBaseMonster :: FInViewCone ( Vector *pOrigin )
{
	Vector2D	vec2LOS;
	float		flDot;

	UTIL_MakeVectors ( pev->angles );
	
	vec2LOS = ( *pOrigin - pev->origin ).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

	if ( flDot > m_flFieldOfView )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target
//=========================================================
BOOL CBaseEntity :: FVisible ( CBaseEntity *pEntity )
{
	TraceResult tr;
	Vector		vecLookerOrigin;
	Vector		vecTargetOrigin;
	
	if (FBitSet( pEntity->pev->flags, FL_NOTARGET ))
		return FALSE;

	// don't look through water
	if ((pev->waterlevel != 3 && pEntity->pev->waterlevel == 3) 
		|| (pev->waterlevel == 3 && pEntity->pev->waterlevel == 0))
		return FALSE;

	vecLookerOrigin = pev->origin + pev->view_ofs;//look through the caller's 'eyes'
	vecTargetOrigin = pEntity->EyePosition();

	UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, ignore_monsters, ignore_glass, ENT(pev)/*pentIgnore*/, &tr);
	
	if (tr.flFraction != 1.0)
	{
		return FALSE;// Line of sight is not established
	}
	else
	{
		return TRUE;// line of sight is valid.
	}
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target vector
//=========================================================
BOOL CBaseEntity :: FVisible ( const Vector &vecOrigin )
{
	TraceResult tr;
	Vector		vecLookerOrigin;
	
	vecLookerOrigin = EyePosition();//look through the caller's 'eyes'

	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, ENT(pev)/*pentIgnore*/, &tr);
	
	if (tr.flFraction != 1.0)
	{
		return FALSE;// Line of sight is not established
	}
	else
	{
		return TRUE;// line of sight is valid.
	}
}

/*
================
TraceAttack
================
*/
void CBaseEntity::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4;

	if( pev->takedamage )
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );

		int blood = BloodColor();
		
		if ( blood != DONT_BLEED )
		{
			SpawnBlood(vecOrigin, blood, flDamage);// a little surface blood.
			TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		}
	}
}


/*
//=========================================================
// TraceAttack
//=========================================================
void CBaseMonster::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	Vector vecOrigin = ptr->vecEndPos - vecDir * 4;

	ALERT ( at_console, "%d\n", ptr->iHitgroup );


	if ( pev->takedamage )
	{
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );

		int blood = BloodColor();
		
		if ( blood != DONT_BLEED )
		{
			SpawnBlood(vecOrigin, blood, flDamage);// a little surface blood.
		}
	}
}
*/

//=========================================================
// TraceAttack
//=========================================================
void CBaseMonster :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( pev->takedamage )
	{
		m_LastHitGroup = ptr->iHitgroup;

		switch ( ptr->iHitgroup )
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			flDamage *= gSkillData.monHead;
			break;
		case HITGROUP_CHEST:
			flDamage *= gSkillData.monChest;
			break;
		case HITGROUP_STOMACH:
			flDamage *= gSkillData.monStomach;
			break;
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			flDamage *= gSkillData.monArm;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= gSkillData.monLeg;
			break;
		default:
			break;
		}

		SpawnBlood(ptr->vecEndPos, BloodColor(), flDamage);// a little surface blood.
		TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
	}
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.

This version is used by Monsters.
================
*/
void CBaseEntity::FireBullets(ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker )
{
	static int tracerCount;
	TraceResult tr;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;

	if ( pevAttacker == NULL )
		pevAttacker = pev;  // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET | DMG_NEVERGIB;

	for (ULONG iShot = 1; iShot <= cShots; iShot++)
	{
		// get circular gaussian spread
		float x, y, z;
		do {
			x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
			y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
			z = x*x+y*y;
		} while (z > 1);

		Vector vecDir = vecDirShooting +
						x * vecSpread.x * vecRight +
						y * vecSpread.y * vecUp;
		Vector vecEnd;

		vecEnd = vecSrc + vecDir * flDistance;
		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(pev)/*pentIgnore*/, &tr);

		if (iTracerFreq != 0 && (tracerCount++ % iTracerFreq) == 0)
		{
			Vector vecTracerSrc;

			if ( IsPlayer() )
			{// adjust tracer position for player
				vecTracerSrc = vecSrc + Vector ( 0 , 0 , -4 ) + gpGlobals->v_right * 2 + gpGlobals->v_forward * 16;
			}
			else
			{
				vecTracerSrc = vecSrc;
			}
			
			switch( iBulletType )
			{
			case BULLET_MONSTER_MP5:
			case BULLET_MONSTER_9MM:
			case BULLET_MONSTER_12MM:
			default:
				/*MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, vecTracerSrc );
					WRITE_BYTE( TE_TRACER );
					WRITE_COORD( vecTracerSrc.x );
					WRITE_COORD( vecTracerSrc.y );
					WRITE_COORD( vecTracerSrc.z );
					WRITE_COORD( tr.vecEndPos.x );
					WRITE_COORD( tr.vecEndPos.y );
					WRITE_COORD( tr.vecEndPos.z );
				MESSAGE_END();*/
				break;
			}
		}
		// do damage, paint decals
		if (tr.flFraction != 1.0)
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( iDamage )
			{
				pEntity->TraceAttack(pevAttacker, iDamage * diff.value, vecDir, &tr, DMG_BULLET | ((iDamage > 16) ? DMG_ALWAYSGIB : DMG_NEVERGIB) );
				
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
			} 
			else switch(iBulletType)
			{
			default:
			case BULLET_MONSTER_9MM:
				pEntity->TraceAttack(pevAttacker, 35 * diff.value, vecDir, &tr, DMG_BULLET);
				
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );

				break;

			case BULLET_MONSTER_MP5:
				pEntity->TraceAttack(pevAttacker, 35 * diff.value, vecDir, &tr, DMG_BULLET);
				
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );

				break;

			case BULLET_MONSTER_12MM:		
				pEntity->TraceAttack(pevAttacker, 35 * diff.value, vecDir, &tr, DMG_BULLET); 
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
				break;
			
			case BULLET_NONE: // FIX 
				pEntity->TraceAttack(pevAttacker, 20 * diff.value, vecDir, &tr, DMG_CLUB);
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				// only decal glass
				if ( !FNullEnt(tr.pHit) && VARS(tr.pHit)->rendermode != 0)
				{
					UTIL_DecalTrace( &tr, DECAL_GLASSBREAK1 + RANDOM_LONG(0,2) );
				}

				break;
			}
		}
		// make bullet trails
		UTIL_BubbleTrail( vecSrc, tr.vecEndPos, (flDistance * tr.flFraction) / 64.0 );
	}
	ApplyMultiDamage(pev, pevAttacker);
}


/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.

This version is used by Players, uses the random seed generator to sync client and server side shots.
================
*/
// Spawning zombies are not linked as SOLID so engine traces miss them.
// Manually test the bullet line against each one via pfnTraceModel.
static void CheckSpawningZombies( const Vector &vecSrc, const Vector &vecEnd, TraceResult &tr )
{
	CBaseEntity *pZ = NULL;
	while( ( pZ = UTIL_FindEntityByClassname( pZ, "monster_zombie" ) ) != NULL )
	{
		if( pZ->pev->takedamage != DAMAGE_AIM || pZ->pev->movetype == MOVETYPE_STEP )
			continue;
		TraceResult ztr;
		UTIL_TraceModel( vecSrc, vecEnd, point_hull, ENT( pZ->pev ), &ztr );
		if( ztr.flFraction < tr.flFraction )
		{
			tr = ztr;
			tr.pHit = ENT( pZ->pev );
		}
	}
}

extern void Explosion( entvars_t* pev, float flDamage, float flRadius );
Vector CBaseEntity::FireBulletsPlayer ( ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker, int shared_rand )
{
	static int tracerCount;
	TraceResult tr;
	Vector vecRight = gpGlobals->v_right;
	Vector vecUp = gpGlobals->v_up;
	float x = .0f, y = .0f, z = .0f;
	int pwall = 0;

	if( recoilratio.value > 0 )
		vecSpread = vecSpread * recoilratio.value;
	if ( pevAttacker == NULL )
		pevAttacker = pev;  // the default attacker is ourselves

	ClearMultiDamage();
	gMultiDamage.type = DMG_BULLET;//| DMG_NEVERGIB;

	if( iBulletType != BULLET_NONE )
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
			WRITE_BYTE( TE_DLIGHT );

			WRITE_COORD( vecSrc.x );    // Position...
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z );
			WRITE_BYTE( 15 );     // Radius, in 10s (therefore, this is 300 units)
        
			WRITE_BYTE( 150 );    // Color: red
			WRITE_BYTE( 120 );    // Color: green
			WRITE_BYTE( 0 );      // Color: blue
            
			WRITE_BYTE( 2  );      // Life
			WRITE_BYTE( 10 );     // Decay rate
		MESSAGE_END();
	}

    // + ste0 (siehe ende der function)
again:
	// - ste0

	Vector vecDir;
	for ( ULONG iShot = 1; iShot <= cShots; iShot++ )
	{
		if( !pwall && IsPlayer( ) )
		{
			CBasePlayer *pPlayer = (CBasePlayer*) this;
			if( pPlayer->m_iShots < 160 )
				pPlayer->m_iShots += 20;
			//Use player's random seed.
			// get circular gaussian spread
			x = UTIL_SharedRandomFloat( shared_rand + pPlayer->m_iShots, -(10.0/160.0*pPlayer->m_iShots), (10.0/160.0*pPlayer->m_iShots) );
			y = UTIL_SharedRandomFloat( shared_rand + ( 2 + pPlayer->m_iShots ), (5.0/160.0*pPlayer->m_iShots), (20.0/160.0*pPlayer->m_iShots) );
			x /= 10.0;
			y /= 10.0;
		//	x = UTIL_SharedRandomFloat( shared_rand + iShot, -(0.7/160*iShot), (0.7/160*iShot) ) + UTIL_SharedRandomFloat( shared_rand + ( 1 + iShot ) , -(0.7/160*iShot), (0.7/160*iShot) );
		//	y = UTIL_SharedRandomFloat( shared_rand + ( 2 + iShot ), -(0.7/160*iShot), (0.7/160*iShot) ) + UTIL_SharedRandomFloat( shared_rand + ( 3 + iShot ), -(0.7/160*iShot), (0.7/160*iShot) );
			z = x * x + y * y;
		/*}
		if( IsPlayer( ) )
		{
			CBasePlayer *pPlayer = (CBasePlayer*) this;*/
			if( FBitSet( pPlayer->m_iAddons, AD_NANO_SKIN ) )
			{
				x *= 1.10;
				y *= 1.10;
			}
			if( FBitSet( pPlayer->m_iCyber, CB_HEAD ) )
			{
				x *= 0.66;
				y *= 0.66;
			}
			if( FBitSet( pPlayer->m_iCyber, CB_ARMS ) )
			{
				x *= 0.58;
				y *= 0.58;
			}


			/*if( pPlayer->m_iShots < 22 )
			{
				x *= 0.5;
				y *= 0.5;
			}*/
			x *= 1 + pPlayer->m_iShots/200;
			y *= 1 + pPlayer->m_iShots/200;

			if( ( pPlayer->m_iShots > 100 ) && UTIL_FromChance( 66 ) )
			{
				pPlayer->pev->punchangle.x = -x*2;
				//pPlayer->pev->punchangle.y = -y;
			}
			else if( UTIL_FromChance( 33 ) )
			{
				pPlayer->pev->punchangle.x = -x;
				//pPlayer->pev->punchangle.y = -y;
			}
			/*if( iBulletType == BULLET_PLAYER_BENELLI ||
				iBulletType == BULLET_PLAYER_SPAS12 )
			{
				x *= 2;
				y *= 2;
			}*/

		if( ( iBulletType == BULLET_PLAYER_SAWED ) ||
		    ( iBulletType == BULLET_PLAYER_BENELLI ) )
		{
			//do {
        x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
				y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
				x *= 1 + pPlayer->m_iShots/200;
				y *= 1 + pPlayer->m_iShots/200;
				//z = x*x+y*y;
			//} while (z > 1);

		vecDir = vecDirShooting +
						x * vecSpread.x * 0.5 * vecRight +
						y * 0.04 * vecUp;
		}
    else
    {
		vecDir = vecDirShooting +
						x * vecSpread.x * vecRight +
						y * vecSpread.y * vecUp;
    }
    }
		Vector vecEnd;

		vecEnd = vecSrc + vecDir * flDistance;
		UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(pev)/*pentIgnore*/, &tr);
		CheckSpawningZombies( vecSrc, vecEnd, tr );

		// do damage, paint decals
		if (tr.flFraction != 1.0)
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( iDamage )
			{
				pEntity->TraceAttack(pevAttacker, iDamage, vecDir, &tr, DMG_BULLET | ((iDamage > 16) ? DMG_ALWAYSGIB : DMG_NEVERGIB) );
				
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				DecalGunshot( &tr, iBulletType );
			} 
			else switch(iBulletType)
			{
			//PE
			case BULLET_PLAYER_357:		
				pEntity->TraceAttack( pevAttacker, 100, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_SEBURO:		
				pEntity->TraceAttack( pevAttacker, 26, vecDir, &tr, DMG_BULLET );
				break;
			case BULLET_PLAYER_BENELLI:		
				pEntity->TraceAttack( pevAttacker, 26, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_BERETTA:		
				pEntity->TraceAttack( pevAttacker, 55, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_M16:		
				pEntity->TraceAttack( pevAttacker, 52, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_GLOCK:		
				pEntity->TraceAttack( pevAttacker, 45, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_USP:		
				pEntity->TraceAttack( pevAttacker, 55, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_AUG:		
				pEntity->TraceAttack( pevAttacker, 50, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_MINIGUN:		
				pEntity->TraceAttack( pevAttacker, 25, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_MICROUZI:		
				pEntity->TraceAttack( pevAttacker, 30, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_MICROUZI_A:		
				pEntity->TraceAttack( pevAttacker, 22, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_MP5:		
				pEntity->TraceAttack( pevAttacker, 35, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_STONER:		
				pEntity->TraceAttack( pevAttacker, 250, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_SIG550:		
				pEntity->TraceAttack( pevAttacker, 130, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_P226:		
				pEntity->TraceAttack( pevAttacker, 65, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_P225:		
				pEntity->TraceAttack( pevAttacker, 47, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_BERETTA_A:		
				pEntity->TraceAttack( pevAttacker, 55, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_GLOCK_AUTO_A:		
				pEntity->TraceAttack( pevAttacker, 22, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_GLOCK_AUTO:		
				pEntity->TraceAttack( pevAttacker, 26, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_MP5SD:		
				pEntity->TraceAttack( pevAttacker, 35, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_TAVOR:		
				pEntity->TraceAttack( pevAttacker, 200, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_SAWED:		
				pEntity->TraceAttack( pevAttacker, 30, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_AK47:		
				pEntity->TraceAttack( pevAttacker, 55, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_USPMP:		
				pEntity->TraceAttack( pevAttacker, 25, vecDir, &tr, DMG_BULLET ); 
				break;
			case BULLET_PLAYER_HAND:		
				pEntity->TraceAttack( pevAttacker, 30, vecDir, &tr, DMG_BULLET ); 
				break;
      case BULLET_PLAYER_FLAME:
        pEntity->TraceAttack( pevAttacker, 10, vecDir, &tr, DMG_BURN ); 
        break;
      case BULLET_PLAYER_SPRAY:
        pEntity->TraceAttack( pevAttacker, 7, vecDir, &tr, DMG_BURN ); 
        break;
			default:
			case BULLET_NONE: // FIX 
				if( IsPlayer( ) )
				{
					CBasePlayer *pPlayer = (CBasePlayer*) this;
					/*if( FBitSet( pPlayer->m_iAddons, AD_ARMS_FAST ) )
					{
						pEntity->TraceAttack(pevAttacker, 300, vecDir, &tr, DMG_BULLET);
					}
					else if( FBitSet( pPlayer->m_iCyber, CB_ARMS ) )
					{
						pEntity->TraceAttack(pevAttacker, 240, vecDir, &tr, DMG_BULLET);
					}
					else*/
						pEntity->TraceAttack(pevAttacker, 90, vecDir, &tr, DMG_BULLET);
          if( FClassnameIs( pEntity->pev, "monster_zombie" ) ) 
          {
          	UTIL_MakeVectors( pev->v_angle );
            pEntity->pev->velocity = pEntity->pev->velocity + gpGlobals->v_forward * ( CBaseMonster::DamageForce( pEntity->pev, 40 ) * 3 );
          }

				}
				else
					pEntity->TraceAttack(pevAttacker, 80, vecDir, &tr, DMG_BULLET);
				TEXTURETYPE_PlaySound(&tr, vecSrc, vecEnd, iBulletType);
				// only decal glass
				if ( !FNullEnt(tr.pHit) /*&& VARS(tr.pHit)->rendermode != 0*/ )
				{
          if( VARS(tr.pHit)->rendermode != 0 )
					  UTIL_DecalTrace( &tr, DECAL_GLASSBREAK1 + RANDOM_LONG(0,2) );
          if( FBitSet( tr.pHit->v.flags, FL_MONSTER ) && ( tr.pHit->v.takedamage == DAMAGE_YES ) )
            UTIL_BloodDrips( tr.vecEndPos, gpGlobals->v_forward, 0, 0 );
				}

				break;
			}
			//DecalGunshot( &tr, iBulletType );
		}
		// make bullet trails
		UTIL_BubbleTrail( vecSrc, tr.vecEndPos, (flDistance * tr.flFraction) / 64.0 );
	}
	ApplyMultiDamage(pev, pevAttacker);

	// + ste0
	if( UTIL_FromChance(95) && ( VARS( tr.pHit )->solid == SOLID_BSP ) && ( strcmp( STRING(VARS( tr.pHit )->classname), "pe_objectclip" ) ) ) 
	{
		// durch die Wand schießen	
		if( pwall ) // spin - nur einmal durch die wand
			return /*vecDir;//*/Vector( x * vecSpread.x, y * vecSpread.y, 0.0 );
		pwall = 1;

		switch( iBulletType )
		{
			case BULLET_PLAYER_BENELLI:
			case BULLET_PLAYER_GLOCK:
			case BULLET_PLAYER_USP:
			case BULLET_PLAYER_BERETTA:
			case BULLET_PLAYER_MICROUZI:
			case BULLET_PLAYER_MICROUZI_A:
			case BULLET_PLAYER_P226:
			case BULLET_PLAYER_P225:
			case BULLET_PLAYER_BERETTA_A:
			case BULLET_PLAYER_GLOCK_AUTO_A:
			case BULLET_PLAYER_GLOCK_AUTO:
				vecSrc = tr.vecEndPos + gpGlobals->v_forward * 8;
				break;					
			case BULLET_PLAYER_MP5:
			case BULLET_PLAYER_MP5SD:
			case BULLET_PLAYER_SEBURO:
			case BULLET_PLAYER_MINIGUN:
				vecSrc = tr.vecEndPos + gpGlobals->v_forward * 16;
				break;					
			case BULLET_PLAYER_M16:
			case BULLET_PLAYER_AUG:
				vecSrc = tr.vecEndPos + gpGlobals->v_forward * 38;
				break;					
			case BULLET_PLAYER_TAVOR:
			case BULLET_PLAYER_AK47:
			case BULLET_PLAYER_SIG550:
      case BULLET_PLAYER_357:
			case BULLET_PLAYER_STONER:
				vecSrc = tr.vecEndPos + gpGlobals->v_forward * 54;
				break;					

			default:
				return /*vecDir;//*/Vector( x * vecSpread.x, y * vecSpread.y, 0.0 );
				break;
		}
		iDamage *= 0.75;
		iTracerFreq = 0;
		goto again;
	}
	// - ste0

	return /*vecDir;//*/Vector( x * vecSpread.x, y * vecSpread.y, 0.0 );
}

void CBaseEntity :: TraceBleed( float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	//if (BloodColor() == DONT_BLEED)
	//	return;
	
	if (flDamage == 0)
		return;

	if (! (bitsDamageType & (DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB | DMG_MORTAR)))
		return;
	
	// make blood decal on the wall! 
	TraceResult Bloodtr;
	Vector vecTraceDir; 
	float flNoise;
	int cCount;
	int i;

/*
	if ( !IsAlive() )
	{
		// dealing with a dead monster. 
		if ( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return; 
		}
		else
		{
			pev->max_health--;
		}
	}
*/

	if (flDamage < 10)
	{
		flNoise = 0.1;
		cCount = 1;
	}
	else if (flDamage < 25)
	{
		flNoise = 0.2;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}

	for ( i = 0 ; i < cCount ; i++ )
	{
		vecTraceDir = vecDir * -1;// trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * -172, ignore_monsters, ENT(pev), &Bloodtr);

		if ( Bloodtr.flFraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}

//=========================================================
//=========================================================
void CBaseMonster :: MakeDamageBloodDecal ( int cCount, float flNoise, TraceResult *ptr, const Vector &vecDir )
{
	// make blood decal on the wall! 
	TraceResult Bloodtr;
	Vector vecTraceDir; 
	int i;

	if ( !IsAlive() )
	{
		// dealing with a dead monster. 
		if ( pev->max_health <= 0 )
		{
			// no blood decal for a monster that has already decalled its limit.
			return; 
		}
		else
		{
			pev->max_health--;
		}
	}

	for ( i = 0 ; i < cCount ; i++ )
	{
		vecTraceDir = vecDir;

		vecTraceDir.x += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.y += RANDOM_FLOAT( -flNoise, flNoise );
		vecTraceDir.z += RANDOM_FLOAT( -flNoise, flNoise );

		UTIL_TraceLine( ptr->vecEndPos, ptr->vecEndPos + vecTraceDir * 172, ignore_monsters, ENT(pev), &Bloodtr);

		if ( Bloodtr.flFraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}
