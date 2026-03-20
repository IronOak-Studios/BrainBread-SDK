// ------------ Public - Enemy Gamerules -- Szenario: Hacker --------------------
// -------- by Spinator
// ---- pe_rules_hacker.cpp
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"pe_menus.h"
#include	"pe_rules.h"
#include	"pe_rules_hacker.h"
#include	"game.h"
#include	"shake.h"
#include	"pe_utils.h"
#include	"pe_notify.h"

extern int gmsgCounter;
extern int gmsgRadar;

cPERules::cPERules( )
{
	m_fNextThink = gpGlobals->time;
	m_flUpdateCounter = gpGlobals->time;
	m_flRoundEndTime = gpGlobals->time;
	m_fNextVote = gpGlobals->time + 60;
	m_fNextVarVote = gpGlobals->time + 5;
	m_iYesVotes = 0;
	m_iNoVotes = 0;
	m_iYesVarVotes = 0;
	m_iNoVarVotes = 0;
	m_fResetTime = 0;
	m_bDoReset = 0;
	strcpy( m_sVoteMap, "(null)" );
	strcpy( m_sVoteVar[0], "(null)" );
	strcpy( m_sVoteVar[1], "0" );
//	CVAR_SET_STRING( "sv_maxspeed", "430" );
}

char folder[10][32] = 
{
	{ "" },
	{ "hacking/" },
	{ "vip/" },
	{ "radio1/" },
	{ "radio2/" },
	{ "radio3/" },
	{ "" }
};

void cPERules::Radio( char *szName, CBasePlayer *pPlayer, int iType )
{
	char command[64];

	sprintf( command, "radio/%s%s.wav", folder[iType], szName );
	//CLIENT_COMMAND( pPlayer->edict( ), command );
//	ALERT( at_console, "%s\n", command );
	EMIT_SOUND( ENT(pPlayer->pev), CHAN_VOICE, command, 1, ATTN_NORM);
}

void cPERules::RadioAll( char *szName, int iType )
{
	char command[64];

	sprintf( command, "radio/%s%s.wav", folder[iType], szName );
//	ALERT( at_console, "%s\n", command );
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
	
		if( plr )
			EMIT_SOUND( ENT(plr->pev), CHAN_VOICE, command, 1, ATTN_NORM);
	}
}

void cPERules::RadioTeam( CBasePlayer *pPlayer, char *szName, int iType )
{
	char command[64];
	int o = 0, p = 0;

	sprintf( command, "radio/%s%s.wav", folder[iType], szName );
//	ALERT( at_console, "%s\n", command );
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
	
		if( plr && ( plr->m_iTeam == pPlayer->m_iTeam ) )
		{
			//o = 0;
			//for( p = 1; p < 45; p++ )
			//	if( pPlayer->m_iDraw[p] == pPlayer->entindex( ) )
			//		o = p;
			EMIT_SOUND( ENT(plr->pev), CHAN_VOICE, command, 1, ATTN_NORM );
			//if( ( o >= 45 ) || ( o <= 0 ) )
			//	continue;
			SetColorTeam( plr->pev, plr->m_iTargetVolume, RADAR_BLINK_TEMP );
		}
	}
}

float cPERules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED * 1.25;
} 

int cPERules::UpdateCounter( )
{
	float cur = m_flRoundEndTime - gpGlobals->time + 1;
  if( !(roundtime.value) )
    cur = 0;
	if( (int)cur == (int)m_flUpdateCounter )
		return 0;
	
	MESSAGE_BEGIN( MSG_ALL, gmsgCounter, NULL );
		WRITE_COORD( (int)cur );
	MESSAGE_END( );

	m_flUpdateCounter = cur;
	return 1;
}

void cPERules::ResetMap( )
{
	CBaseEntity* respawn = UTIL_FindEntityByClassname( NULL, "func_breakable" );
	int i = 0;

	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "func_breakable" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "multi_manager" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "multi_manager" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "func_train" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "func_train" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "bb_tank" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "bb_tank" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "bb_escapeair" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "bb_escapeair" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "mapobject_zeppelin" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "mapobject_zeppelin" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "pe_light" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "pe_light" );
	}
	
	respawn = UTIL_FindEntityByClassname( NULL, "pe_terminal" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "pe_terminal" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "mapobject_npc_bum" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "mapobject_npc_bum" );
	}
	respawn = UTIL_FindEntityByClassname( NULL, "mapobject_npc_bum1" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "mapobject_npc_bum1" );
	}
	respawn = UTIL_FindEntityByClassname( NULL, "mapobject_npc_bum2" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "mapobject_npc_bum2" );
	}
	respawn = UTIL_FindEntityByClassname( NULL, "mapobject_npc_bum3" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "mapobject_npc_bum3" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "mapobject_copcar" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "mapobject_copcar" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "mapobject_asiancar" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "mapobject_asiancar" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "mapobject_redcar" );
	while ( respawn != NULL )
	{
		respawn->Spawn();
		respawn = UTIL_FindEntityByClassname( respawn, "mapobject_redcar" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "mapobject_bluecar" );
	while ( respawn != NULL )
	{
		respawn->Spawn();
		respawn = UTIL_FindEntityByClassname( respawn, "mapobject_bluecar" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "mapobject_npc_crow" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "mapobject_npc_crow" );
	}

	/*respawn = UTIL_FindEntityByClassname( NULL, "ambient_generic" );
	while ( respawn != NULL )
	{
		((CAmbientGeneric*)respawn)->ToggleUse( NULL, NULL, USE_ON, 100.0 );
		respawn = UTIL_FindEntityByClassname( respawn, "ambient_generic" );
	}*/

	respawn = UTIL_FindEntityByClassname(NULL, "func_door");
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname(respawn, "func_door");
	}

	respawn = UTIL_FindEntityByClassname( NULL, "func_door_rotating" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "func_door_rotating" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "func_button" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "func_button" );
	}
		
	respawn = UTIL_FindEntityByClassname( NULL, "weaponbox" );
  DisableAll("weaponbox");
  DisableAll("monster_zombie");
  while ( respawn != NULL )
	{
		UTIL_Remove( respawn );
		respawn = UTIL_FindEntityByClassname( respawn, "weaponbox" );
	}
	respawn = UTIL_FindEntityByClassname( NULL, "weapon_case" );
	while ( respawn != NULL )
	{
		DisableAll("weapon_case");
		UTIL_Remove( respawn );
		respawn = UTIL_FindEntityByClassname( respawn, "weapon_case" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "pe_object_case" );
	while ( respawn != NULL )
	{
		DisableAll("pe_object_case");
		respawn->Spawn();
		respawn = UTIL_FindEntityByClassname( respawn, "pe_object_case" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "bb_objective_item" );
  DisableAll("bb_objective_item");
	while ( respawn != NULL )
	{
		UTIL_Remove( respawn );
		respawn = UTIL_FindEntityByClassname( respawn, "bb_objective_item" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "pe_objectclip" );
	while ( respawn != NULL )
	{
		respawn->ReSpawn();
		respawn = UTIL_FindEntityByClassname( respawn, "pe_objectclip" );
	}

	respawn = UTIL_FindEntityByClassname( NULL, "bodyque" );
	while ( respawn != NULL )
	{
		respawn->pev->effects |= EF_NODRAW; //REMOVE_ENTITY( respawn->edict( ) );
		respawn = UTIL_FindEntityByClassname( respawn, "bodyque" );
	}	
  
  respawn = UTIL_FindEntityByClassname( NULL, "grenade" );
	while ( respawn != NULL )
	{
		UTIL_Remove( respawn );
		respawn = UTIL_FindEntityByClassname( respawn, "grenade" );
	}

  respawn = UTIL_FindEntityByClassname( NULL, "bb_escapeair" );
  while ( respawn != NULL )
  {
    respawn->pev->fuser4 = FALSE;
    respawn = UTIL_FindEntityByClassname( respawn, "bb_escapeair" );
  }

  respawn = UTIL_FindEntityByClassname( NULL, "monster_hgrunt" );
  while ( respawn != NULL )
  {
    //UTIL_Remove( respawn );
    respawn->pev->health = 0;
    respawn->Killed( respawn->pev, 0 );
    respawn = UTIL_FindEntityByClassname( respawn, "monster_hgrunt" );
  }

  respawn = UTIL_FindEntityByClassname( NULL, "monster_hgrunt_shotgun" );
  while ( respawn != NULL )
  {
    //UTIL_Remove( respawn );
    respawn->pev->health = 0;
    respawn->Killed( respawn->pev, 0 );
    respawn = UTIL_FindEntityByClassname( respawn, "monster_hgrunt_shotgun" );
  }

  respawn = UTIL_FindEntityByClassname( NULL, "monster_zombie" );
  while ( respawn != NULL )
  {
    //UTIL_Remove( respawn );
    respawn->pev->health = 0;
    respawn->Killed( respawn->pev, 0 );
    respawn = UTIL_FindEntityByClassname( respawn, "monster_zombie" );
  }
	uPrepSpawn( );
}

extern char *COM_Parse (char *data, char *com_token );

BOOL cPERules::IsVoteable( char *var )
{
	if( !var || strlen( var ) <= 0 )
		return false;

	int length;
	char token[1500] = "\0";
	char *varlist = (char*)LOAD_FILE_FOR_ME( "voteable_vars.txt", &length );

	if( !varlist || length <= 0 )
	{
		FREE_FILE( varlist );
		return TRUE;
	}

	while( varlist && strlen( varlist ) > 0 )
	{
		varlist = COM_Parse( varlist, token );
		if( strlen( token ) <= 0 )
		{
			FREE_FILE( varlist );
			return FALSE;
		}
		if( !stricmp( token, var ) )
		{
			FREE_FILE( varlist );
			return TRUE;
		}
	}
	FREE_FILE( varlist );
	return FALSE;
}
