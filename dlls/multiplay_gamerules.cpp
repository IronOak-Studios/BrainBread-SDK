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
//
// teamplay_gamerules.cpp
//
#include    <ctype.h>
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"pe_rules.h"
#include	"pe_rules_hacker.h"
#include	"pe_rules_vip.h"
 
#include	"skill.h"
#include	"game.h"
#include	"items.h"
#include	"voice_gamemgr.h"
#include	"hltv.h"
#include	"pe_utils.h"

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;
extern int gmsgServerName;
extern int gmsgSpectator;

extern int g_teamplay;

#define ITEM_RESPAWN_TIME	30
#define WEAPON_RESPAWN_TIME	20
#define AMMO_RESPAWN_TIME	30

float g_flIntermissionStartTime = 0;

CVoiceGameMgr	g_VoiceGameMgr;

class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool		CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pTalker)
	{
		if( g_pGameRules->PlayerRelationship( pListener, pTalker ) != GR_TEAMMATE )
			return false;
		if( pListener->IsAlive( ) != pTalker->IsAlive( ) )
			return false;

		return true;
	}
};
static CMultiplayGameMgrHelper g_GameMgrHelper;

//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************

CHalfLifeMultiplay :: CHalfLifeMultiplay()
{
	g_VoiceGameMgr.Init(&g_GameMgrHelper, MAX_PLAYERS);

	RefreshSkillData();
	m_flIntermissionEndTime = 0;
	g_flIntermissionStartTime = 0;
	
	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that 
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
	if ( IS_DEDICATED_SERVER() )
	{
		// dedicated server
		char *servercfgfile = (char *)CVAR_GET_STRING( "servercfgfile" );

		if ( servercfgfile && servercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT( at_console, "Executing dedicated server config file\n" );
			snprintf( szCommand, sizeof(szCommand), "exec %s\n", servercfgfile );
			SERVER_COMMAND( szCommand );
		}
	}
	else
	{
		// listen server
		char *lservercfgfile = (char *)CVAR_GET_STRING( "lservercfgfile" );

		if ( lservercfgfile && lservercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT( at_console, "Executing listen server config file\n" );
			snprintf( szCommand, sizeof(szCommand), "exec %s\n", lservercfgfile );
			SERVER_COMMAND( szCommand );
		}
	}
}

BOOL CHalfLifeMultiplay::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	if(g_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
		return TRUE;

	return CGameRules::ClientCommand(pPlayer, pcmd);
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::RefreshSkillData( void )
{
// load all default values
	CGameRules::RefreshSkillData();

// override some values for multiplay.

	// suitcharger
	gSkillData.suitchargerCapacity = 30;

	// Crowbar whack
	gSkillData.plrDmgCrowbar = 25;

	// Glock Round
	gSkillData.plrDmg9MM = 12;

	// 357 Round
	gSkillData.plrDmg357 = 40;

	// MP5 Round
	gSkillData.plrDmgMP5 = 12;

	// M203 grenade
	gSkillData.plrDmgM203Grenade = 100;

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot = 20;// fewer pellets in deathmatch

	// Crossbow
	gSkillData.plrDmgCrossbowClient = 20;

	// RPG
	gSkillData.plrDmgRPG = 120;

	// Egon
	gSkillData.plrDmgEgonWide = 20;
	gSkillData.plrDmgEgonNarrow = 10;

	// Hand Grendade
	gSkillData.plrDmgHandGrenade = 100;

	// Satchel Charge
	gSkillData.plrDmgSatchel = 120;

	// Tripmine
	gSkillData.plrDmgTripmine = 150;

	// hornet
	gSkillData.plrDmgHornet = 10;
}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME		20

extern cvar_t timeleft, fragsleft;

extern cvar_t mp_chattime;

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: Think ( void )
{
	g_VoiceGameMgr.Update(gpGlobals->frametime);

	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	if ( g_fGameOver )   // someone else quit the game already
	{
		// bounds check
		int time = (int)CVAR_GET_FLOAT( "mp_chattime" );
		if ( time < 10 )
			CVAR_SET_STRING( "mp_chattime", "10" );
		else if ( time > MAX_INTERMISSION_TIME )
			CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( MAX_INTERMISSION_TIME ) );

		m_flIntermissionEndTime = g_flIntermissionStartTime + mp_chattime.value;

		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->time )
		{
			if ( m_iEndIntermissionButtonHit  // check that someone has pressed a key, or the max intermission time is over
				|| ( ( g_flIntermissionStartTime + MAX_INTERMISSION_TIME ) < gpGlobals->time) ) 
				ChangeLevel(); // intermission is over
		}

		return;
	}

	float flTimeLimit = timelimit.value * 60;
	float flFragLimit = fraglimit.value;

	time_remaining = (int)(flTimeLimit ? ( flTimeLimit - gpGlobals->time ) : 0);
	
	if ( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
	{
		GoToIntermission();
		return;
	}

	if ( flFragLimit )
	{
		int bestfrags = 9999;
		int remain;

		// check if any player is over the frag limit
		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer && strlen( STRING( pPlayer->pev->netname ) ) && pPlayer->pev->frags >= flFragLimit )
			{
				GoToIntermission();
				return;
			}


			if ( pPlayer )
			{
				remain = flFragLimit - pPlayer->pev->frags;
				if ( remain < bestfrags )
				{
					bestfrags = remain;
				}
			}

		}
		frags_remaining = bestfrags;
	}

	// Updates when frags change
	if ( frags_remaining != last_frags )
	{
		g_engfuncs.pfnCvar_DirectSet( &fragsleft, UTIL_VarArgs( "%i", frags_remaining ) );
	}

	// Updates once per second
	if ( timeleft.value != last_time )
	{
		g_engfuncs.pfnCvar_DirectSet( &timeleft, UTIL_VarArgs( "%i", time_remaining ) );
	}

	last_frags = frags_remaining;
	last_time  = time_remaining;
}


//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsMultiplayer( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsDeathmatch( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsCoOp( void )
{
	return gpGlobals->coop;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pWeapon->CanDeploy() )
	{
		// that weapon can't deploy anyway.
		return FALSE;
	}

	if ( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	if ( !pPlayer->m_pActiveItem->CanHolster() )
	{
		// can't put away the active item.
		return FALSE;
	}

	if ( pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight() )
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CHalfLifeMultiplay :: GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{

	CBasePlayerItem *pCheck;
	CBasePlayerItem *pBest;// this will be used in the event that we don't find a weapon in the same category.
	int iBestWeight;
	int i;

	iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	pBest = NULL;

	if ( !pCurrentWeapon->CanHolster() )
	{
		// can't put this gun away right now, so can't switch.
		return FALSE;
	}

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pCheck = pPlayer->m_rgpPlayerItems[ i ];

		while ( pCheck )
		{
			/*if ( pCheck->iWeight() > -1 && pCheck->iWeight() == pCurrentWeapon->iWeight() && pCheck != pCurrentWeapon )
			{
				// this weapon is from the same category. 
				if ( pCheck->CanDeploy() )
				{
					if ( pPlayer->SwitchWeapon( pCheck ) )
					{
						return TRUE;
					}
				}
			}
			else */if ( pCheck->iWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
			{
				//ALERT ( at_console, "Considering %s\n", STRING( pCheck->pev->classname ) );
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted 
				// weapon. 
				if ( pCheck->CanDeploy() )
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pCheck->iWeight();
					pBest = pCheck;
				}
			}

			pCheck = NULL;//pCheck->m_pNext;
		}
	}

	// if we make it here, we've checked all the weapons and found no useable 
	// weapon in the same catagory as the current weapon. 
	
	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
	// at least get the crowbar, but ya never know.
	if ( !pBest )
	{
		return FALSE;
	}

	pPlayer->SwitchWeapon( pBest );

	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay :: ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] )
{
	g_VoiceGameMgr.ClientConnected(pEntity);
	return TRUE;
}

extern int gmsgSayText;
//extern int gmsgGameMode;

void CHalfLifeMultiplay :: UpdateGameMode( CBasePlayer *pPlayer )
{
//	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
//		WRITE_BYTE( 0 );  // game mode none
//	MESSAGE_END();
}

BOOL CHalfLifeMultiplay :: InitHUD( CBasePlayer *pl )
{
	if( !strlen( STRING( pl->pev->netname ) ) && GETPLAYERUSERID( pl->edict( ) ) == -1 && !strlen( GETPLAYERAUTHID( pl->edict( ) ) ) )
		return FALSE;
	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s has joined the game\n", 
		( pl->pev->netname && STRING(pl->pev->netname)[0] != 0 ) ? STRING(pl->pev->netname) : "unconnected" ) );

	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" entered the game\n",  
		STRING( pl->pev->netname ), 
		GETPLAYERUSERID( pl->edict() ),
		GETPLAYERAUTHID( pl->edict() ),
		g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pl->edict() ), "team" ) );

	UpdateGameMode( pl );

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	/*MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
		WRITE_BYTE( ENTINDEX(pl->edict()) );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
		//WRITE_SHORT( 0 );
		//WRITE_SHORT( 0 );
	MESSAGE_END();*/

	//SendMOTDToClient( pl->edict() );

	// loop through all active players and send their score info to the new client
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		// FIXME:  Probably don't need to cast this just to read m_iDeaths
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( plr )
		{
			/*MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
				WRITE_BYTE( i );	// client number
				WRITE_SHORT( plr->pev->frags );
				WRITE_SHORT( plr->m_iDeaths );
				//WRITE_SHORT( plr->m_iClass );
				//WRITE_SHORT( plr->m_iTeam );
			MESSAGE_END();*/
			// Sende den Spectator-Status
			MESSAGE_BEGIN( MSG_ONE, gmsgSpectator, NULL, pl->edict() );
					WRITE_BYTE( i );
					WRITE_BYTE( (plr->pev->iuser1 != 0) );
			MESSAGE_END();
		}
	}

	if ( g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ONE, SVC_INTERMISSION, NULL, pl->edict() );
		MESSAGE_END();
	}
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: ClientDisconnected( edict_t *pClient )
{
	if ( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

		if ( pPlayer )
		{
			CBaseEntity *que = UTIL_FindEntityByClassname( NULL, "bodyque" );
			while ( que != NULL )
			{
				if( que->pev->renderamt == ENTINDEX( pPlayer->edict( ) ) )
					que->pev->effects |= EF_NODRAW;
				que = UTIL_FindEntityByClassname( que, "bodyque" );
			}
			FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" disconnected\n",  
				STRING( pPlayer->pev->netname ), 
				GETPLAYERUSERID( pPlayer->edict() ),
				GETPLAYERAUTHID( pPlayer->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "team" ) );

			pPlayer->RemoveAllItems( TRUE );// destroy all of the players weapons and items
			// Allen Clients mitteilen, dass dieser Client kein Spectator mehr ist
			MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
					WRITE_BYTE( ENTINDEX(pClient) );
					WRITE_BYTE( 0 );
			MESSAGE_END();

			/*CBasePlayer *client = NULL;
			while ( ((client = (CBasePlayer*)UTIL_FindEntityByClassname( client, "player" )) != NULL)
					&& client && (!FNullEnt(client->edict())) )
			{
					if ( !client->pev )
							continue;
					if ( client == pPlayer )
							continue;

					// Wenn ein Spectator diesen Spieler verfolgt hat, soll er sich ein neues Ziel suchen
					if ( client->m_hObserverTarget == pPlayer )
					{
							int iMode = client->pev->iuser1;
							client->pev->iuser1 = 0;
							client->m_hObserverTarget = NULL;
							client->Observer_SetMode( iMode );
					}
			}*/
      for ( int i = 1; i <= MAX_PLAYERS; i++ )
		  {
			  CBasePlayer *client = (CBasePlayer *)UTIL_PlayerByIndex( i );

				if( !client || !client->pev )
					continue;
				if( client == pPlayer )
							continue;
        if( client->m_hObserverTarget == pPlayer )
				{
						int iMode = client->pev->iuser1;
						client->pev->iuser1 = 0;
						client->m_hObserverTarget = NULL;
						client->Observer_SetMode( iMode );
				}
      }
		}
		//FREE_PRIVATE(pClient);
	}
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay :: FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	int iFallDamage = (int)falldamage.value;

	switch ( iFallDamage )
	{
	case 1://progressive
		pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0:// fixed
		return 10;
		break;
	}
} 

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: PlayerThink( CBasePlayer *pPlayer )
{
	if ( g_fGameOver )
	{
		// check for button presses
		if ( pPlayer->m_afButtonPressed & ( IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP ) )
			m_iEndIntermissionButtonHit = TRUE;

		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay :: PlayerSpawn( CBasePlayer *pPlayer )
{
	BOOL		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;

	pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
	
	addDefault = TRUE;

	while ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ))
	{
		pWeaponEntity->Touch( pPlayer );
		addDefault = FALSE;
	}

	if ( addDefault )
	{
		pPlayer->GiveNamedItem( "weapon_crowbar" );
		pPlayer->GiveNamedItem( "weapon_9mmhandgun" );
//		pPlayer->GiveAmmo( 68, "9mm", _9MM_MAX_CARRY );// 4 full reloads
	}
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay :: FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay :: FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

BOOL CHalfLifeMultiplay :: AllowAutoTargetCrosshair( void )
{
	return ( aimcrosshair.value != 0 );
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeMultiplay :: IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}


//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
extern void CopyToBodyQue(entvars_t *pev);
void CHalfLifeMultiplay :: PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	DeathNotice( pVictim, pKiller, pInflictor );

	pVictim->m_iDeaths += 1;

	pVictim->pev->solid = SOLID_NOT;

	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );
	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
	if ( ktmp && (ktmp->Classify() == CLASS_PLAYER) )
		peKiller = (CBasePlayer*)ktmp;

	if ( pVictim->pev == pKiller )  
	{  // killed self
		pKiller->frags -= 1;
	}
	else if ( ktmp && ktmp->IsPlayer() )
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		int frags = IPointsForKill( peKiller, pVictim );
		pKiller->frags += frags;
		if( frags > 0 )
			peKiller->BonusPoint( TASK_KILL );
		else if( frags < 0 )
			peKiller->BonusPoint( TASK_TEAMKILL );
		
		FireTargets( "game_playerkill", ktmp, ktmp, USE_TOGGLE, 0 );
	}
	else
	{  // killed by the world
    if( pVictim->m_iTeam != 2 )
      pVictim->pev->frags -= 2;
	}

	// update the scores
	// killed scores
	/*MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );
		WRITE_SHORT( pVictim->pev->frags );
		WRITE_SHORT( pVictim->m_iDeaths );
		//WRITE_SHORT( pVictim->m_iClass );
		//WRITE_SHORT( pVictim->m_iTeam );
	MESSAGE_END();*/

	// killers score, if it's a player
	CBaseEntity *ep = CBaseEntity::Instance( pKiller );
	if ( ep && ep->Classify() == CLASS_PLAYER )
	{
		CBasePlayer *PK = (CBasePlayer*)ep;

		/*MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( ENTINDEX(PK->edict()) );
			WRITE_SHORT( PK->pev->frags );
			WRITE_SHORT( PK->m_iDeaths );
			//WRITE_SHORT( PK->m_iClass );
			//WRITE_SHORT( PK->m_iTeam );
		MESSAGE_END();*/

		// let the killer paint another decal as soon as he'd like.
		PK->m_flNextDecalTime = gpGlobals->time;
	}
/*#ifndef HLDEMO_BUILD
	if ( pVictim->HasNamedPlayerItem("weapon_satchel") )
	{
		DeactivateSatchels( pVictim );
	}
#endif*/
}

//=========================================================
// Deathnotice. 
//=========================================================
void CHalfLifeMultiplay::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	// Work out what killed the player, and send a message to all clients about it
	CBaseEntity *Killer = CBaseEntity::Instance( pKiller );

	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_index = 0;
	
	// Hack to fix name change
	const char *tau = "tau_cannon";
	const char *gluon = "gluon gun";
	const char *wpns[32] = 
	{
		"Desert Eagle", "Seburo CX", "Benelli M3", "Beretta 92FS",
		"M16", "Glock 18 Semi", "Knife", "Steyr Aug", "M134 Vulcan Minigun",
		"IMI MicroUzi", "IMI MicroUzi akimbo", "H&K MP5/10", "Gastank",
		"H&K USP 9", "Winchester", ".44 SW", "Sig P226", "Sig P225", "Beretta 92FS akimbo",
		"Glock 18 auto akimbo", "Glock 18 auto", "H&K MP5 sd", "IMI Tavor", "sawed off Shotgun", "AK74",
		"Case", "H&K USP MP", "Sig P226 MP", "Orbital Device", "Flammenwerfer S"
	};

	if ( pKiller->flags & FL_CLIENT )
	{
		killer_index = ENTINDEX(ENT(pKiller));
		
		if ( pevInflictor )
		{
			if ( pevInflictor == pKiller )
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( pKiller );
				
				if ( pPlayer && pPlayer->m_pActiveItem )
				{
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
				}
			}
			else
			{
				killer_weapon_name = STRING( pevInflictor->classname );  // it's just that easy
			}
		}
	}
	else
	{
    if( FClassnameIs( pKiller, "monster_human_grunt" ) || FClassnameIs( pKiller, "monster_hgrunt" ) || FClassnameIs( pKiller, "monster_hgrunt_shotgun" ) )
    {
      killer_index = 66;
    }
    else if( FClassnameIs( pKiller, "monster_zombie" ) || FClassnameIs( pKiller, "monster_hgrunt_shotgun" ) )
    {
      if( pKiller->fuser4 )
        killer_index = 67;
      else
        killer_index = 66;
    }
    else if( FClassnameIs( pKiller, "monster_barney" ) )
    {
      killer_index = 68;
    }
    else if( FClassnameIs( pKiller, "bb_tank" ) )
    {
      killer_index = 69;
    }
    else if( FClassnameIs( pKiller, "bb_escapeair" ) )
    {
      killer_index = 70;
    }
  	//killer_weapon_name = STRING( pevInflictor->classname );
		killer_weapon_name = "skull";
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		killer_weapon_name += 7;
	else if ( strncmp( killer_weapon_name, "monster_", 8 ) == 0 )
		killer_weapon_name += 8;
	else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		killer_weapon_name += 5;

	if( pVictim->m_LastHitGroup == 1 )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
			WRITE_BYTE( killer_index );						// the killer
			WRITE_BYTE( ENTINDEX(pVictim->edict()) );		// the victim
			WRITE_STRING( "headshot" );
			WRITE_STRING( killer_weapon_name );
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
			WRITE_BYTE( killer_index );						// the killer
			WRITE_BYTE( ENTINDEX(pVictim->edict()) );		// the victim
			WRITE_STRING( killer_weapon_name );		// what they were killed by (should this be a string?)
		MESSAGE_END();
	}

	// replace the code names with the 'real' names
	if ( !strcmp( killer_weapon_name, "deagle" ) )
		killer_weapon_name = wpns[0];
	else if ( !strcmp( killer_weapon_name, "seburo" ) )
		killer_weapon_name = wpns[1];
	else if ( !strcmp( killer_weapon_name, "benelli" ) )
		killer_weapon_name = wpns[2];
	else if ( !strcmp( killer_weapon_name, "beretta" ) )
		killer_weapon_name = wpns[3];
	else if ( !strcmp( killer_weapon_name, "m16" ) )
		killer_weapon_name = wpns[4];
	else if ( !strcmp( killer_weapon_name, "glock" ) )
		killer_weapon_name = wpns[5];
	else if ( !strcmp( killer_weapon_name, "knife" ) )
		killer_weapon_name = wpns[6];
	else if ( !strcmp( killer_weapon_name, "aug" ) )
		killer_weapon_name = wpns[7];
	else if ( !strcmp( killer_weapon_name, "minigun" ) )
		killer_weapon_name = wpns[8];
	else if ( !strcmp( killer_weapon_name, "microuzi" ) )
		killer_weapon_name = wpns[9];
	else if ( !strcmp( killer_weapon_name, "microuzi_a" ) )
		killer_weapon_name = wpns[10];
	else if ( !strcmp( killer_weapon_name, "mp5" ) )
		killer_weapon_name = wpns[11];
	else if ( !strcmp( killer_weapon_name, "canister" ) )
		killer_weapon_name = wpns[12];
	else if ( !strcmp( killer_weapon_name, "grenade" ) )
		killer_weapon_name = wpns[12];
	else if ( !strcmp( killer_weapon_name, "usp" ) )
		killer_weapon_name = wpns[13];
	else if ( !strcmp( killer_weapon_name, "winchester" ) )
		killer_weapon_name = wpns[14];
	else if ( !strcmp( killer_weapon_name, "44sw" ) )
		killer_weapon_name = wpns[15];
	else if ( !strcmp( killer_weapon_name, "p226" ) )
		killer_weapon_name = wpns[16];
	else if ( !strcmp( killer_weapon_name, "p225" ) )
		killer_weapon_name = wpns[17];
	else if ( !strcmp( killer_weapon_name, "beretta_a" ) )
		killer_weapon_name = wpns[18];
	else if ( !strcmp( killer_weapon_name, "glock_auto_a" ) )
		killer_weapon_name = wpns[19];
	else if ( !strcmp( killer_weapon_name, "glock_auto" ) )
		killer_weapon_name = wpns[20];
	else if ( !strcmp( killer_weapon_name, "mp5sd" ) )
		killer_weapon_name = wpns[21];
	else if ( !strcmp( killer_weapon_name, "tavor" ) )
		killer_weapon_name = wpns[22];
	else if ( !strcmp( killer_weapon_name, "sawed" ) )
		killer_weapon_name = wpns[23];
	else if ( !strcmp( killer_weapon_name, "ak47" ) )
		killer_weapon_name = wpns[24];
	else if ( !strcmp( killer_weapon_name, "case" ) )
		killer_weapon_name = wpns[25];
	else if ( !strcmp( killer_weapon_name, "uspmp" ) )
		killer_weapon_name = wpns[26];
	else if ( !strcmp( killer_weapon_name, "p226mp" ) )
		killer_weapon_name = wpns[27];
	else if ( !strcmp( killer_weapon_name, "orbital" ) )
		killer_weapon_name = wpns[28];
	else if ( !strcmp( killer_weapon_name, "flame" ) )
		killer_weapon_name = wpns[29];

	if ( pVictim->pev == pKiller )  
	{
		// killed self

			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide\n",  
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pVictim->edict() ), "team" ) );		
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",  
				STRING( pKiller->netname ),
				GETPLAYERUSERID( ENT(pKiller) ),
				GETPLAYERAUTHID( ENT(pKiller) ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( ENT(pKiller) ), "team" ),
				STRING( pVictim->pev->netname ),
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pVictim->edict() ), "team" ),
				killer_weapon_name );
	}
	else
	{ 
		// killed by the world

			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide\n",
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ), 
				GETPLAYERAUTHID( pVictim->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pVictim->edict() ), "team" ) );				
	}

	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE ( 9 );	// command length in bytes
		WRITE_BYTE ( DRC_CMD_EVENT );	// player killed
		WRITE_SHORT( ENTINDEX(pVictim->edict()) );	// index number of primary entity
		if (pevInflictor)
			WRITE_SHORT( ENTINDEX(ENT(pevInflictor)) );	// index number of secondary entity
		else
			WRITE_SHORT( ENTINDEX(ENT(pKiller)) );	// index number of secondary entity
		WRITE_LONG( 7 | DRC_FLAG_DRAMATIC);   // eventflags (priority and flags)
	MESSAGE_END();

//  Print a standard message
	// TODO: make this go direct to console
	return; // just remove for now

	char	szText[ 256 ];

	if ( pKiller->flags & FL_MONSTER )
	{
		// killed by a monster
		snprintf ( szText, sizeof(szText), "%s was killed by a monster.\n", STRING( pVictim->pev->netname ) );
		return;
	}

	if ( pKiller == pVictim->pev )
	{
		snprintf ( szText, sizeof(szText), "%s committed suicide.\n", STRING( pVictim->pev->netname ) );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		snprintf ( szText, sizeof(szText), "%s : %s : %s\n", STRING( pKiller->netname ), killer_weapon_name, STRING( pVictim->pev->netname ) );
	}
	else if ( FClassnameIs ( pKiller, "worldspawn" ) )
	{
		snprintf ( szText, sizeof(szText), "%s fell or drowned or something.\n", STRING( pVictim->pev->netname ) );
	}
	else if ( pKiller->solid == SOLID_BSP )
	{
		snprintf ( szText, sizeof(szText), "%s was mooshed.\n", STRING( pVictim->pev->netname ) );
	}
	else
	{
		snprintf ( szText, sizeof(szText), "%s died mysteriously.\n", STRING( pVictim->pev->netname ) );
	}

	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, szText );

}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeMultiplay :: PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeMultiplay :: FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	if ( weaponstay.value > 0 )
	{
		// make sure it's only certain weapons
		if ( !(pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			return gpGlobals->time + 0;		// weapon respawns almost instantly
		}
	}

	return gpGlobals->time + 0;//WEAPON_RESPAWN_TIME;
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeMultiplay :: FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon && pWeapon->m_iId && (pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD) )
	{
		if ( NUMBER_OF_ENTITIES() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay :: VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeMultiplay :: WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// CanHaveWeapon - returns FALSE if the player is not allowed
// to pick up this weapon
//=========================================================
BOOL CHalfLifeMultiplay::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pItem )
{
	if ( weaponstay.value > 0 )
	{
		if ( pItem->iFlags() & ITEM_FLAG_LIMITINWORLD )
			return CGameRules::CanHavePlayerItem( pPlayer, pItem );

		// check if the player already has this weapon
		for ( int i = 0 ; i < MAX_ITEM_TYPES ; i++ )
		{
			CBasePlayerItem *it = pPlayer->m_rgpPlayerItems[i];

			while ( it != NULL )
			{
				if ( it->m_iId == pItem->m_iId )
				{
					return FALSE;
				}

				it = NULL;//it->m_pNext;
			}
		}
	}

	return CGameRules::CanHavePlayerItem( pPlayer, pItem );
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::ItemShouldRespawn( CItem *pItem )
{
	if ( pItem->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeMultiplay::FlItemRespawnTime( CItem *pItem )
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsAllowedToSpawn( CBaseEntity *pEntity )
{
//	if ( pEntity->pev->flags & FL_MONSTER )
//		return FALSE;

	return TRUE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	/*if ( pAmmo->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_AMMO_RESPAWN_NO;
	}*/

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

//=========================================================
//=========================================================
Vector CHalfLifeMultiplay::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlHealthChargerRechargeTime( void )
{
	return 60;
}


float CHalfLifeMultiplay::FlHEVChargerRechargeTime( void )
{
	return 30;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

edict_t *CHalfLifeMultiplay::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = CGameRules::GetPlayerSpawnSpot( pPlayer );	
	if( !pentSpawnSpot )
		return NULL;
	if ( IsMultiplayer() && pentSpawnSpot->v.target )
	{
		FireTargets( STRING(pentSpawnSpot->v.target), pPlayer, pPlayer, USE_TOGGLE, 0 );
	}

	return pentSpawnSpot;
}


//=========================================================
//=========================================================
int CHalfLifeMultiplay::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// half life deathmatch has only enemies
	return GR_NOTTEAMMATE;
}

BOOL CHalfLifeMultiplay :: PlayFootstepSounds( CBasePlayer *pl, float fvol )
{
	if ( g_footsteps && g_footsteps->value == 0 )
		return FALSE;

	if ( pl->IsOnLadder() || pl->pev->velocity.Length2D() > 220 )
		return TRUE;  // only make step sounds in multiplayer if the player is moving fast enough

	return FALSE;
}

BOOL CHalfLifeMultiplay :: FAllowFlashlight( void ) 
{ 
	return flashlight.value != 0; 
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay :: FAllowMonsters( void )
{
	return ( allowmonsters.value != 0 );
}

//=========================================================
//======== CHalfLifeMultiplay private functions ===========
#define INTERMISSION_TIME		6

extern std::list<CBaseEntity*> zombiepool;
extern std::list<CBaseEntity*> gibpool;
extern int zombiecount;
extern int gibcount;

void CHalfLifeMultiplay :: GoToIntermission( void )
{
	if ( g_fGameOver )
		return;  // intermission has already been triggered, so ignore.

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();

	// bounds check
	int time = (int)CVAR_GET_FLOAT( "mp_chattime" );
	if ( time < 10 )
		CVAR_SET_STRING( "mp_chattime", "10" );
	else if ( time > MAX_INTERMISSION_TIME )
		CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( MAX_INTERMISSION_TIME ) );

	m_flIntermissionEndTime = gpGlobals->time + ( (int)mp_chattime.value );
	g_flIntermissionStartTime = gpGlobals->time;

	g_fGameOver = TRUE;
	m_iEndIntermissionButtonHit = FALSE;

	uSaveAllExp();

  //list<CBaseEntity*>::iterator i;

  zombiepool.clear( );
  zombiecount = 0;
  gibpool.clear( );
  gibcount = 0;
   
  CBaseEntity *ent = UTIL_FindEntityByClassname( NULL, "monster_zombie" );
  while( ent != NULL )
  {
	  ent->UpdateOnRemove();
	  ent->pev->flags |= FL_KILLME;
	  ent->pev->targetname = 0;
    ent->SetThink(&CBaseEntity::SUB_Remove);
    ent = UTIL_FindEntityByClassname( ent, "monster_zombie" );
  }
  ent = UTIL_FindEntityByClassname( NULL, "gib" );
  while( ent != NULL )
  {
	  ent->UpdateOnRemove();
	  ent->pev->flags |= FL_KILLME;
	  ent->pev->targetname = 0;
    ent->SetThink(&CBaseEntity::SUB_Remove);
    ent = UTIL_FindEntityByClassname( ent, "gib" );
  }


  /*for( i = zombiepool.begin( ); i != zombiepool.end( ); i++ )
  {
    ent = *i;
    if( !ent )
      continue;
	  ent->UpdateOnRemove();
	  ent->pev->flags |= FL_KILLME;
	  ent->pev->targetname = 0;
    ent->SetThink(&CBaseEntity::SUB_Remove);
  }
  for( i = gibpool.begin( ); i != gibpool.end( ); i++ )
  {
    ent = *i;
    if( !ent )
      continue;
	  ent->UpdateOnRemove();
	  ent->pev->flags |= FL_KILLME;
	  ent->pev->targetname = 0;
    ent->SetThink(&CBaseEntity::SUB_Remove);
  }*/

}

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s *next;

	char mapname[ 32 ];
	int  minplayers, maxplayers;
	char rulebuffer[ MAX_RULE_BUFFER ];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
} mapcycle_t;

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void DestroyMapCycle( mapcycle_t *cycle )
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if ( p )
	{
		start = p;
		p = p->next;
		while ( p != start )
		{
			n = p->next;
			delete p;
			p = n;
		}
		
		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}

//static char com_token[ 1500 ];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data, char *com_token, int com_token_size )
{
	int             c;
	int             len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			if (len < com_token_size - 1)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		if (len < com_token_size - 1)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
int COM_TokenWaiting( char *buffer )
{
	char *p;

	p = buffer;
	while ( *p && *p!='\n')
	{
		if ( !isspace( *p ) || isalnum( *p ) )
			return 1;

		p++;
	}

	return 0;
}



/*
==============
ReloadMapCycleFile


Parses mapcycle.txt file into mapcycle_t structure
==============
*/
int ReloadMapCycleFile( char *filename, mapcycle_t *cycle )
{
	char szBuffer[ MAX_RULE_BUFFER ];
	char szMap[ 32 ];
	int length;
	char *pFileList;
	char token[1500];
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( filename, &length );
	int hasbuffer;
	mapcycle_item_s *item, *newlist = NULL, *next;

	if ( pFileList && length )
	{
		// the first map name in the file becomes the default
		while ( 1 )
		{
			hasbuffer = 0;
			memset( szBuffer, 0, MAX_RULE_BUFFER );

			pFileList = COM_Parse( pFileList, token, sizeof(token) );
			if ( strlen( token ) <= 0 )
				break;

			strncpy( szMap, token, sizeof( szMap ) );
			szMap[ sizeof( szMap ) - 1 ] = '\0';

			// Any more tokens on this line?
			if ( pFileList && COM_TokenWaiting( pFileList ) )
			{
				pFileList = COM_Parse( pFileList, token, sizeof(token) );
				if ( strlen( token ) > 0 )
				{
					hasbuffer = 1;
					strncpy( szBuffer, token, sizeof( szBuffer ) );
					szBuffer[ sizeof( szBuffer ) - 1 ] = '\0';
				}
			}

			// Check map
			if ( IS_MAP_VALID( szMap ) )
			{
				// Create entry
				char *s;

				item = new mapcycle_item_s;

				strncpy( item->mapname, szMap, sizeof(item->mapname) );
				item->mapname[sizeof(item->mapname) - 1] = '\0';

				item->minplayers = 0;
				item->maxplayers = 0;

				memset( item->rulebuffer, 0, MAX_RULE_BUFFER );

				if ( hasbuffer )
				{
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "minplayers" );
					if ( s && s[0] )
					{
						item->minplayers = atoi( s );
						item->minplayers = max( item->minplayers, 0 );
						item->minplayers = min( item->minplayers, MAX_PLAYERS );
					}
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "maxplayers" );
					if ( s && s[0] )
					{
						item->maxplayers = atoi( s );
						item->maxplayers = max( item->maxplayers, 0 );
						item->maxplayers = min( item->maxplayers, MAX_PLAYERS );
					}

					// Remove keys
					//
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "minplayers" );
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "maxplayers" );

					strncpy( item->rulebuffer, szBuffer, sizeof(item->rulebuffer) );
					item->rulebuffer[sizeof(item->rulebuffer) - 1] = '\0';
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				ALERT( at_console, "Skipping %s from mapcycle, not a valid map\n", szMap );
			}

		}

		FREE_FILE( aFileList );
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while ( item )
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if ( !item )
	{
		return 0;
	}

	while ( item->next )
	{
		item = item->next;
	}
	item->next = cycle->items;
	
	cycle->next_item = item->next;

	return 1;
}

/*
==============
CountPlayers

Determine the current # of active players on the server for map cycling logic
==============
*/
int CountPlayers( void )
{
	int	num = 0;

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( i );

		if ( pEnt && strlen( STRING( pEnt->pev->netname ) ) )
		{
			num = num + 1;
		}
	}

	return num;
}

/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server
 level transition
==============
*/
void ExtractCommandString( char *s, char *szCommand, int szCommand_size )
{
	// Now make rules happen
	char	pkey[512];
	char	value[512];	// use two buffers so compares
								// work without stomping on each other
	char	*o;
	char	*end;
	
	if ( *s == '\\' )
		s++;

	while (1)
	{
		o = pkey;
		end = pkey + sizeof(pkey) - 1;
		while ( *s != '\\' )
		{
			if ( !*s )
				return;
			if ( o < end )
				*o++ = *s++;
			else
				s++;
		}
		*o = 0;
		s++;

		o = value;
		end = value + sizeof(value) - 1;

		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			if ( o < end )
				*o++ = *s++;
			else
				s++;
		}
		*o = 0;

		if ( (int)( strlen( szCommand ) + strlen( pkey ) + strlen( value ) + 3 ) < szCommand_size )
		{
			strcat( szCommand, pkey );
			if ( strlen( value ) > 0 )
			{
				strcat( szCommand, " " );
				strcat( szCommand, value );
			}
			strcat( szCommand, "\n" );
		}

		if (!*s)
			return;
		s++;
	}
}

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
extern char g_sNextMap[32];
void CHalfLifeMultiplay :: ChangeLevel( void )
{
	static char szPreviousMapCycleFile[ 256 ];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[ 1500 ];
	char szRules[ 1500 ];
	int minplayers = 0, maxplayers = 0;
	strcpy( szFirstMapInList, "db_cybercon" );  // the absolute default level is hldm1

	int	curplayers;
	BOOL do_cycle = TRUE;

	// find the map to change to
	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );

	szCommands[ 0 ] = '\0';
	szRules[ 0 ] = '\0';

	curplayers = CountPlayers();


	strncpy( szNextMap, g_sNextMap, sizeof(szNextMap) );
	szNextMap[sizeof(szNextMap) - 1] = '\0';
	//ExtractCommandString( item->rulebuffer, szCommands );
	//strcpy( szRules, item->rulebuffer );

	g_fGameOver = TRUE;

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );
	if ( minplayers || maxplayers )
	{
		ALERT( at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers );
	}
	if ( strlen( szRules ) > 0 )
	{
		ALERT( at_console, "RULES:  %s\n", szRules );
	}
	
	CHANGE_LEVEL( szNextMap, NULL );
	if ( strlen( szCommands ) > 0 )
	{
		SERVER_COMMAND( szCommands );
	}
  if( g_pGameRules )
  {
    delete g_pGameRules;
    g_pGameRules = NULL;
  }
  for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
    if( !pPlayer )
			continue;
		pPlayer->m_iTeam = 0;
    pPlayer->escaped = false;
		pPlayer->m_sInfo.team = 0;
		pPlayer->m_iSpawnDelayed = 0;
		pPlayer->ResetRadar( );
    pPlayer->m_fInitHUD = 0;
    pPlayer->m_fGameHUDInitialized = 0;
    pPlayer->m_iHUDInited = FALSE;
		pPlayer->statsRatio = 1;
    pPlayer->speedpnts = 0;
	  pPlayer->hppnts = 0;
    pPlayer->dmgpnts = 0;
    pPlayer->level = 0;
    pPlayer->clientlevel = -1;
    pPlayer->slot1f = 0;
    pPlayer->slot2f = 0;
    pPlayer->slot3f = 0;
    pPlayer->m_fTransformTime = 0;
    pPlayer->m_bTransform = 0;
    pPlayer->flame.clear( );
    pPlayer->zombieKills = 0;
    pPlayer->rehbutton = false;
    pPlayer->clientFrags = 0;
    pPlayer->clientDeaths = 0;
   	pPlayer->m_fPointsMax = 0;
    pPlayer->exp = 0;
		pPlayer->expLoaded = false;
    SetBits( pPlayer->pev->flags, FL_NOTARGET );
    SetBits( pPlayer->pev->flags, FL_DORMANT );
  }
}

#define MAX_MOTD_CHUNK	  60
#define MAX_MOTD_LENGTH   1536 // (MAX_MOTD_CHUNK * 4)

void CHalfLifeMultiplay :: SendMOTDToClient( edict_t *client )
{
	// read from the MOTD.txt file
	int length, char_count = 0;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( (char *)CVAR_GET_STRING( "motdfile" ), &length );

	// send the server name
	MESSAGE_BEGIN( MSG_ONE, gmsgServerName, NULL, client );
		WRITE_STRING( CVAR_GET_STRING("hostname") );
	MESSAGE_END();

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while ( pFileList && *pFileList && char_count < MAX_MOTD_LENGTH )
	{
		char chunk[MAX_MOTD_CHUNK+1];
		
		if ( strlen( pFileList ) < MAX_MOTD_CHUNK )
		{
			strncpy( chunk, pFileList, sizeof(chunk) );
			chunk[sizeof(chunk) - 1] = '\0';
		}
		else
		{
			strncpy( chunk, pFileList, MAX_MOTD_CHUNK );
			chunk[MAX_MOTD_CHUNK] = 0;		// strncpy doesn't always append the null terminator
		}

		char_count += strlen( chunk );
		if ( char_count < MAX_MOTD_LENGTH )
			pFileList = aFileList + char_count; 
		else
			*pFileList = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgMOTD, NULL, client );
			WRITE_BYTE( *pFileList ? FALSE : TRUE );	// FALSE means there is still more message to come
			WRITE_STRING( chunk );
		MESSAGE_END();
	}

	FREE_FILE( aFileList );
}

void CHalfLifeMultiplay::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	// No-op for BrainBread
}

