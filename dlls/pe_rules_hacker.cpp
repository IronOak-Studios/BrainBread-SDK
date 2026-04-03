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
#include	"voice_gamemgr.h"
#include	"pe_wpn_slotid.h"
#include	"pe_mapobjects.h"
#include	"bb_escape.h"
#include "../cl_dll/pe_help_topics.h"
using namespace::std;

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

LINK_ENTITY_TO_CLASS( bb_custommission, cBBMapMission );

#define ROUND_TIME (roundtime.value)
#define ROUND_DELAY (rounddelay.value)
/*#define ROUND_TIME 0.1
#define ROUND_DELAY 0.1*/

// *shrug* let's get it working as quick&dirty as possible
#define BONUS_PER_ZOMBIE 0.2f
float humanStatsRatio = 1;

//extern hash_map<const char*, float> players;

extern CVoiceGameMgr	g_VoiceGameMgr;
extern DLL_GLOBAL BOOL		g_fGameOver;
extern void UTIL_EdictScreenFade( edict_s *edict, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags );
extern cvar_t timeleft, fragsleft, scoreleft;
extern float g_flIntermissionStartTime;

//extern int gmsgHideSpec;
extern int gmsgAddNotify;
extern int gmsgTeamInfo;
extern int gmsgIntro;
extern int gmsgCounter;
extern int gmsgMates;
extern int gmsgInitHUD;
extern int gmsgHideWeapon;
extern int gmsgClass;
extern int gmsgTeamScore;
extern int gmsgCurWeapon;
extern int gmsgSayText;
extern int gmsgScoreInfo;
extern int gmsgStats;
extern int gmsgLensRef;
extern int gmsgSpecial;
extern int gmsgPlayMusic;
extern int gmsgStopMusic;
extern int gmsgSetVol;
//extern int gmsgFog;
extern int gmsgRain;
extern int gmsgSpectator;
extern int gmsgUpdPoints;
extern int gmsgSmallCnt;
extern int gmsgNotify;
extern int gmsgVGUIMenu;
extern cvar_t mission_timer_detect;
extern float UTIL_MultiManagerTargetDelay( CBaseEntity *pEntity, const char *szTarget );


#define NO_TEAM 0
#define TEAM1	1
#define TEAM2	2

#define ROUND_DURING	0
#define ROUND_START		1

char radio1_text[8][64] = 
{
	{ "Attack the enemy!\n" },
	{ "Cover me!\n" },
	{ "Advance!\n" },
	{ "Stay together!\n" },
	{ "Complete the mission target!\n" },
	{ "Retreat!\n" },
	{ "Incoming!\n" },
	{ "Waiting for orders!\n" }
};
char radio1_sound[8][16] = 
{
	{ "attack" },
	{ "coverme" },
	{ "advance" },
	{ "staytoge" },
	{ "complete" },
	{ "retreat" },
	{ "incoming" },
	{ "waitorde" }
};

char radio2_text[8][64] = 
{
	{ "Cease fire!\n" },
	{ "Open fire!\n" },
	{ "I'm hit!\n" },
	{ "Need assistance!\n" },
	{ "Hold and defend this position!\n" },
	{ "Follow me!\n" },
	{ "Out of ammunition!\n" },
	{ "In position!\n" }
};
char radio2_sound[8][16] = 
{
	{ "ceasef" },
	{ "openfire" },
	{ "im_hit" },
	{ "needassi" },
	{ "holdpos" },
	{ "follow" },
	{ "outofamo" },
	{ "in_pos" }
};

char radio3_text[8][64] = 
{
	{ "Attention!\n" },
	{ "Ambush!\n" },
	{ "Grenade!\n" },
	{ "Sniper!\n" },
	{ "Roger that!\n" },
	{ "Negative!\n" },
	{ "Teammate down!\n" },
	{ "" }
};
char radio3_sound[8][16] = 
{
	{ "attentio" },
	{ "ambush" },
	{ "granade" },
	{ "sniper" },
	{ "roger" },
	{ "negative" },
	{ "matedown" },
	{ "" }
};

#define bit(a) (1<<a)

#define MATE_RAD 7

void cPEHacking::CalcTeamBonus( )
{
	CBaseEntity* ent = NULL;
	CBasePlayer* pl = NULL;
	CBasePlayer* pl2 = NULL;
	int temp;
	
	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		pl = (CBasePlayer *)UTIL_PlayerByIndex (i);
		if ( !pl )
			continue;
	
		temp = pl->m_iNumMates;
		pl->m_iNumMates = 0;

    for( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			pl2 = (CBasePlayer *)UTIL_PlayerByIndex (i);
		
      if ( !pl2
        || !pl2->IsAlive( )
        || ( pl2->m_iTeam != pl->m_iTeam )
        || ( ( pl->pev->origin - pl2->pev->origin ).Length( ) > MATE_RAD * 64 )
        || !strlen( STRING( pl2->pev->netname ) ) )
				continue;
      pl->m_iNumMates++;
    }

		/*ent = UTIL_FindEntityInSphere( NULL, pl->pev->origin, MATE_RAD * 64 );
		while ( ent != NULL )
		{
			if( ent->IsPlayer( ) )
			{
				pl2 = (CBasePlayer*)ent;
				if( pl2 && pl2->IsAlive( ) && pl2->m_iTeam == pl->m_iTeam )
				{
					//pl->m_iMate[pl->m_iNumMates] = i;
					pl->m_iNumMates++;
				}
			}

			ent = UTIL_FindEntityInSphere( ent, pl->pev->origin, MATE_RAD * 64 );
		}*/
    pl->m_iNumMates = max( 0, pl->m_iNumMates - 1 );
		if( !pl->IsAlive( ) )
			pl->m_iNumMates = 0;

		if( pl->m_iNumMates != temp )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgMates, NULL, pl->edict( ) );
				WRITE_BYTE( pl->m_iNumMates );
			MESSAGE_END( );
			if( g_bDbg )
				ALERT( at_logged, "Mates upd.\n" );
		}
	}
}

extern int g_teamplay;
void cPEHacking::CheckRoundEnd( )
{
	m_iRestart = 0;

	// If sv_roundtimelimit changed (e.g. settings.scr applied late), adjust the end time
	if( ROUND_TIME != m_flRoundTimeUsed )
	{
		float elapsed = 0;
		if( m_flRoundTimeUsed > 0 )
			elapsed = m_flRoundTimeUsed - (m_flRoundEndTime - gpGlobals->time);

		m_flRoundTimeUsed = ROUND_TIME;

		if( ROUND_TIME )
			m_flRoundEndTime = gpGlobals->time + (ROUND_TIME - elapsed);
		else
			m_flRoundEndTime = 0;
	}

  CheckAllMissions( );
  if( misComplete || ( oneEscaped && !m_iPlayers[1] ) )
  {
		ALERT( at_logged, "The HUMANS win: Mission completed!\n" );
		NotifyAll( NTM_MISSION );
    AddScore( 1, 20 );
    m_iRestart = 1;
  }
	
	if( ROUND_TIME && m_flRoundEndTime < gpGlobals->time )
	{
    ALERT( at_logged, "ZOMBIES win: The time is up.\n" );
		//RadioAll( "sec_win" );
		AddScore( 2, 10 );
		NotifyAll( NTM_TIMEUP );
		m_iRestart = 2;
		MESSAGE_BEGIN( MSG_ALL, gmsgCounter, NULL );
			WRITE_COORD( 0 );
		MESSAGE_END( );
	}

  if( g_teamplay == 1 )
  {
	  if( misLast && ( !m_iPlayers[1] && ( m_iPlayers[2] > 1 ) ) )//( !m_iPlrAlive[1] && m_iPlayers[1] ) || ( !m_iPlayers[1] && m_iPlayers[2] ) )
	  {
		  ALERT( at_logged, "The ZOMBIES win: The Humans have been wiped out.\n" );
		  //RadioAll( "syn_wins" );
		  NotifyAll( NTM_ALLDEAD );
		  AddScore( 2, 20 );
		  m_iRestart = 2;
	  }
  }
  else
  {
	  if( m_iPlayers[1] == 1 && m_iPlayers[2] )
	  {
			for( int i = 1; i <= MAX_PLAYERS; i++ )
			{
				CBasePlayer *p = (CBasePlayer *)UTIL_PlayerByIndex (i);
			
        if ( !p || !strlen( STRING( p->pev->netname ) ) || p->m_iTeam != 1 )
					continue;
        ALERT( at_logged, "We have a WINNER: Last man standing is %s!\n", STRING( p->pev->netname ) );
        NotifyMidTeam( 1, NTM_CUSTOM, 5, "We have a WINNER: Last man standing is", UTIL_VarArgs( "%s!", STRING( p->pev->netname ) ) );
		    AddScore( 1, 20 );
        p->GiveExp( 1000 );
        p->pev->frags += 20;
		    m_iRestart = 1;
        break;
      }
	  }
  }

	if( m_iRestart )
	{
		ALERT( at_logged, "New round starting...\n" );
		//UpdTeamScore( );
    GoToIntermission( );


		m_flRoundEndTime = gpGlobals->time + ROUND_DELAY;
		m_iRoundStatus = ROUND_START;
    rounds++;
	}
  else if( UpdateCounter( ) ) // alles, das im abstand von 1 sec ausgeführt werden soll, hier hin
	{
    if( nextSave <= gpGlobals->time )
    {
      uSavePlayerData( );
			uSaveAllExp( );
      nextSave = gpGlobals->time + 20;
    }
		//ALERT( at_logged, "Bonus calc.\n" );
		CalcTeamBonus( );
		//ALERT( at_logged, "recount.\n" );
		Recount( );
		//ALERT( at_logged, "maxspeed\n" );
		if( CVAR_GET_FLOAT( "sv_maxspeed" ) < 1000 )
			CVAR_SET_STRING( "sv_maxspeed", "1000" );

		//ALERT( at_logged, "delay spawn\n" );
		//if( gpGlobals->time < ( m_flRoundEndTime - ( ROUND_TIME - 15 ) ) )
		//{
			for( int i = 1; i <= MAX_PLAYERS; i++ )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
			
				if ( !pPlayer || !strlen( STRING( pPlayer->pev->netname ) ) )
					continue;

        if( pPlayer->level != pPlayer->clientlevel || (int)pPlayer->pev->frags != pPlayer->clientFrags || pPlayer->m_iDeaths != pPlayer->clientDeaths )
        {
          MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		        WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
		        WRITE_SHORT( pPlayer->pev->frags );
		        WRITE_SHORT( pPlayer->m_iDeaths );
		        WRITE_BYTE( pPlayer->level );
	        MESSAGE_END( );
          if( pPlayer->m_iTeam == 2 )
            Notify( pPlayer, NTC_ZOMBIE, max( 0.0f, pPlayer->zombieKills ) );
	        
          pPlayer->clientDeaths = pPlayer->m_iDeaths;
          pPlayer->clientFrags = pPlayer->pev->frags;
          pPlayer->clientlevel = pPlayer->level;
        }

				STOP_SOUND(ENT(pPlayer->pev), CHAN_STATIC, "common/null.wav");
			
				if( ( pPlayer->m_sInfo.team != 1 ) && ( pPlayer->m_sInfo.team != 2 ) )
					continue;

				if( pPlayer->IsAlive( ) || pPlayer->m_bNoAutospawn )
					continue;

				//ALERT( at_logged, "wepweg\n" );
				if( pPlayer->pev->weapons )
					pPlayer->RemoveAllItems( TRUE );

        pPlayer->m_bTransform = FALSE;
        if( pPlayer->m_iTeam != pPlayer->m_sInfo.team )
			    SetTeam( pPlayer->m_sInfo.team, pPlayer, 0 );
				
				pPlayer->m_bCanRespawn = TRUE;
				pPlayer->pev->body = 0;
				pPlayer->m_bNoAutospawn = 0;

				//ALERT( at_logged, "notify\n" );
				if( pPlayer->m_iTeam == 1 )
				{
					pPlayer->m_bIsSpecial = FALSE;
					pPlayer->pev->body = 0;
          pPlayer->m_bTransform = FALSE;
					ChangeClass( pPlayer, pPlayer->m_iPrevClass );
					MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
						WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
						WRITE_BYTE( 0 );
					MESSAGE_END( );
				}
				else
				{
					MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
						WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
						WRITE_BYTE( 0 );
					MESSAGE_END( );
					pPlayer->pev->body = 0;
					pPlayer->m_bIsSpecial = FALSE;
					ChangeClass( pPlayer, pPlayer->m_iPrevClass );
					//Notify( pPlayer, NTC_ZOMBIE, max( 0, 5 - ( pPlayer->pev->frags - pPlayer->zombieKills ) ) );
					NotifyMid( pPlayer, NTM_ZOMBIE );
				}
							
				//ALERT( at_logged, "spawn\n" );
				pPlayer->Spawn( );
        PlayerInitMission( pPlayer );
				if( pPlayer->m_iSpawnDelayed )
					pPlayer->StartObserver( );
				else
				{
					pPlayer->BonusPoint( TASK_SPAWN_DELAYED );
					pPlayer->StopObserver( );
					MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
							WRITE_BYTE( ENTINDEX( pPlayer->edict() ) );
							WRITE_BYTE( 0 );
					MESSAGE_END();
				}
				//ALERT( at_logged, "equip\n" );
				if( pPlayer->IsAlive( ) )
					EquipPlayer( pPlayer );
			}
		//}
		//ALERT( at_logged, "balan\n" );
		/*if( !( (int)gpGlobals->time % 4 ) && !m_bBalance && ( ( m_flRoundEndTime - gpGlobals->time )  <= ( ROUND_TIME - 10 ) ) )
		{
			//ALERT( at_logged, "teambalance\n" );
			if( TeamsUneven( ) && autoteam.value )
			{
				NotifyAll( NTM_AUTOBAL );
				m_bBalance = 1;
				ALERT( at_logged, "Teams uneven, forcing autobalance next round.\n" );
			}
			//ALERT( at_logged, "Teams even.\n" );
		}*/
	}

}

void cPEHacking::StartRound( )
{
	if( m_flRoundEndTime > gpGlobals->time )
		return;
  /*if( rounds >= 2 )
    GoToIntermission( );*/
	//ALERT( at_logged, "sr: recount\n" );
  AbortMission( );
  RandomMission( true );

  Recount( );
	//ALERT( at_logged, "sr: autoteam\n" );
	AutoTeam( );

	m_pHacker = NULL;
	//ALERT( at_logged, "sr: resetmap\n" );
	ResetMap( );
	ALERT( at_logged, "Preparing players...\n" );
	m_iTerminalsHacked = 0;
	m_iCountScores = 1;
	m_iHacked1 = 0;
	m_iHacked2 = 0;
	m_iHack1 = 0;
	m_iHack2 = 0;
  misNr = 0;
  misNr = 0;
  misType = 0;
  misHoldoutDuration = 0;
  oneEscaped = false;
#ifdef _DEBUG
  misForceComplete = false;
#endif

  // Pre-neutralize unnamed trigger_once entities that target custom missions,
  // so players can't accidentally fire them before the mission starts.
  if( mission_timer_detect.value )
  {
    for( int m = 0; m < 20 && strlen( missionList[m][0] ); m++ )
    {
      // Skip built-in mission keywords
      if( !strcmp( missionList[m][0], "random" ) ||
          !strcmp( missionList[m][0], "rescue" ) ||
          !strcmp( missionList[m][0], "fred" ) ||
          !strcmp( missionList[m][0], "frags" ) ||
          !strcmp( missionList[m][0], "object" ) )
        continue;

      // Find trigger_once entities targeting this custom mission
      CBaseEntity *pTrigger = NULL;
      while( ( pTrigger = UTIL_FindEntityByClassname( pTrigger, "trigger_once" ) ) != NULL )
      {
        // Only intercept unnamed triggers (no targetname = not triggered externally)
        if( !FStringNull( pTrigger->pev->targetname ) )
          continue;
        if( FStringNull( pTrigger->pev->target ) )
          continue;
        if( !FStrEq( STRING( pTrigger->pev->target ), missionList[m][0] ) )
          continue;

        CBaseDelay *pDelay = static_cast<CBaseDelay*>( pTrigger );
        if( pDelay->m_flDelay > 0 )
        {
          // Disable touch so players can't fire it early
          pTrigger->SetTouch( NULL );
          pTrigger->pev->solid = SOLID_NOT;
        }
      }
    }
  }

  CheckAllMissions( );

	m_flRoundTimeUsed = ROUND_TIME;
	m_flRoundEndTime = gpGlobals->time + ROUND_TIME;
	m_iRoundStatus = ROUND_DURING;

	/*if( !m_iPlayers[1] || !m_iPlayers[2] )
	{
		//ALERT( at_logged, "sr: nocount\n" );
		m_iCountScores = 0;
		NotifyAll( NTM_NOSCORE );
	}*/
	int i;
	for ( i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
	
		if (!pPlayer || !strlen( STRING( pPlayer->pev->netname ) ) )
			continue;
		
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_STATIC, "common/null.wav", 1, ATTN_NORM);
	
		//if( pPlayer->m_sInfo.team == 2 )
		  pPlayer->m_sInfo.team = 1;
    pPlayer->m_bTransform = FALSE;
    pPlayer->zombieKills = 0;
    pPlayer->rehbutton = false;
    pPlayer->m_fTransformTime = 0;
    if( pPlayer->m_iTeam != pPlayer->m_sInfo.team )
			SetTeam( pPlayer->m_sInfo.team, pPlayer, 0 );

    pPlayer->escaped = false;
  		  MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, pPlayer->edict( ) );
			   WRITE_COORD( -1 );
			   WRITE_BYTE( NTM_DONE_HIDE );
		  MESSAGE_END( );
		  MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, pPlayer->edict( ) );
			   WRITE_COORD( -1 );
			   WRITE_BYTE( NTM_REHUMAN_HIDE );
		  MESSAGE_END( );

		
		pPlayer->m_bNoAutospawn = 0;
		if ( pPlayer->m_iTeam == 1 || pPlayer->m_iTeam == 2 )
		{
			//ALERT( at_logged, "sr: spawn\n" );
			if ( pPlayer->pev->weapons )
				pPlayer->RemoveAllItems( TRUE );

			pPlayer->m_bNoAutospawn = 0;
			
			pPlayer->m_bCanRespawn = TRUE;
			pPlayer->pev->body = 0;

			if( pPlayer->m_iTeam == 1 )
			{
				pPlayer->m_bIsSpecial = FALSE;
				pPlayer->pev->body = 0;
				ChangeClass( pPlayer, pPlayer->m_iPrevClass );
				MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
					WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
					WRITE_BYTE( 0 );
				MESSAGE_END( );
			}
			else
			{
				/*if( !m_pHacker && !pPlayer->m_bIsSpecial && i > m_iLastSpec )
				{
					MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
						WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
						WRITE_BYTE( 1 );
					MESSAGE_END( );
					pPlayer->m_bIsSpecial = TRUE;
					pPlayer->pev->body = 1;
					//ChangeClass( pPlayer, 6 );
					//SET_MDL( "hacker" );
					m_pHacker = pPlayer;
					Notify( pPlayer, NTC_HACKER );
					NotifyMid( pPlayer, NTM_HACKER );
					m_iLastSpec = i;
				}
				else
				{*/
					//if( pPlayer->m_bIsSpecial )
					//	SET_MDL( "syndicate1" );
					MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
						WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
						WRITE_BYTE( 0 );
					MESSAGE_END( );
					pPlayer->pev->body = 0;
					pPlayer->m_bIsSpecial = FALSE;
					ChangeClass( pPlayer, pPlayer->m_iPrevClass );
					//Notify( pPlayer, NTC_ZOMBIE, max( 0, 5 - ( pPlayer->pev->frags - pPlayer->zombieKills ) ) );
					NotifyMid( pPlayer, NTM_ZOMBIE );
				//}
			}
						
			pPlayer->Spawn( );
      PlayerInitMission( pPlayer );
			//ALERT( at_logged, "sr: spawned\n" );
			if( pPlayer->m_iSpawnDelayed )
				pPlayer->StartObserver( );
			else
			{
				pPlayer->BonusPoint( TASK_SPAWN );
				if( pPlayer->m_iTeam == m_iRestart )
					pPlayer->BonusPoint( TASK_SPAWN );
				pPlayer->StopObserver( );
                MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
                        WRITE_BYTE( ENTINDEX( pPlayer->edict() ) );
                        WRITE_BYTE( 0 );
                MESSAGE_END();
			}
		}
		else
		{
			pPlayer->Spawn( );
			pPlayer->StartObserver( );
		}
	}
	m_iRestart = -1;
	for ( i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
	
		if (!pPlayer || !strlen( STRING( pPlayer->pev->netname ) ) )
			continue;
		if( pPlayer->m_bCanRespawn )
		{
			/*if( pPlayer->m_iTeam == 2 )
				Radio( "hack", pPlayer, 1 );
			else if( pPlayer->m_iTeam == 1 )
				Radio( "killsyn", pPlayer, 1 );*/
			if( pPlayer->IsAlive( ) )
				EquipPlayer( pPlayer );
		}
	}
	Recount( );
}

/*
	I had some problems where players weren't disconnected correctly
	so I have to call this function quite often to make sure that
	this problem doesn't affect the gameplay.
*/

void cPEHacking::Recount( )
{
	m_iPlayers[0] = 0;
	m_iPlayers[1] = 0;
	m_iPlayers[2] = 0;
	m_iPlrAlive[0] = 0;
	m_iPlrAlive[1] = 0;
	m_iPlrAlive[2] = 0;
	m_iClients = 0;
  humanStatsRatio = 1;

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
	
		if( !pPlayer )
			continue;

		if( strlen( STRING( pPlayer->pev->netname ) ) <= 0 )
		{ //Something went wrong on the player's disconnect
			pPlayer->m_iTeam = 0;
			pPlayer->pev->deadflag = DEAD_DEAD;						
			pPlayer->pev->iuser1 = 1;						
			continue;
		}

		
		if( pPlayer->m_iTeam == 1 )
		{
			m_iClients++;
			m_iPlayers[1]++;
			if( pPlayer->IsAlive( ) )
			{
				m_iPlrAlive[0]++;
				m_iPlrAlive[1]++;
			}
		}
		else if( pPlayer->m_iTeam == 2 )
		{
			m_iClients++;
			m_iPlayers[2]++;
			if( pPlayer->IsAlive( ) )
			{
				m_iPlrAlive[0]++;
				m_iPlrAlive[2]++;
			}
      humanStatsRatio += BONUS_PER_ZOMBIE;
		}
		else
		{
			if( pPlayer->IsPlayer( ) )
			{
				m_iClients++;
				m_iPlayers[0]++;
			}
		}
	}
}

void cPEHacking::EquipPlayer( CBasePlayer *pPlayer )
{
	if( pPlayer->m_bEquiped )
		return;
	pPlayer->m_bEquiped = TRUE;
	int temp;
	const char *last;
	
	if( pPlayer->m_iTeam == 1 )
		temp = WEAPON_KNIFE;
	else
		temp = WEAPON_HAND;
	pPlayer->m_iCurSlot = 1;
	last = uEqPlr( pPlayer, temp );
	pPlayer->m_iPrimaryWeap = temp;

	return;
	
	if( pPlayer->m_bIsSpecial && g_iPERules == 1 )
		pPlayer->GiveNamedItem( "pe_object_htool" );


	pPlayer->m_iCurSlot = 2;

	//pPlayer->m_iPrimaryWeap = 0;
	if( pPlayer->m_sInfo.slot[2] >= 0 && pPlayer->m_sInfo.slot[2] < 35 && slotid[0][pPlayer->m_sInfo.slot[2]] )
	{
		pPlayer->m_pLastItem = pPlayer->ItemByName( last );
		pPlayer->m_iPrimaryWeap = slotid[0][pPlayer->m_sInfo.slot[2]];
		last = uEqPlr( pPlayer, slotid[0][pPlayer->m_sInfo.slot[2]] );
		//ALERT( at_logged, "Primary is now: %d\n", slotid[0][pPlayer->m_sInfo.slot[2]] );
	}

	pPlayer->m_iCurSlot = 3;
	temp = pPlayer->m_sInfo.slot[3];

	for( int i = 0; i < 2; i++ )
	{
		if( i == 1 )
		{
			pPlayer->m_iCurSlot = 4;
			temp = pPlayer->m_sInfo.slot[4];
		}
		//ALERT( at_logged, "Weapon: %d Slot: %d\n", temp, pPlayer->m_iCurSlot );
		if( temp >= 0 && temp < 35 && slotid[1][temp] )
		{
			pPlayer->m_pLastItem = pPlayer->ItemByName( last );
			pPlayer->m_iPrimaryWeap = slotid[1][temp];
			last = uEqPlr( pPlayer, slotid[1][temp] );
			//ALERT( at_logged, "Primary is now: %d\n", slotid[0][pPlayer->m_sInfo.slot[2]] );
		}
	}
	if( pPlayer->m_iPrimaryWeap < 0 || pPlayer->m_iPrimaryWeap >= MAX_WEAPONS )
		pPlayer->m_iPrimaryWeap = 0;
	ItemInfo& II = CBasePlayerItem::ItemInfoArray[pPlayer->m_iPrimaryWeap];
	if( II.iId )
	{
		pPlayer->SelectItem( II.pszName );
		if( !pPlayer->m_pActiveItem ) // In some seldom special cases the commands don't arrive properly
			pPlayer->SelectItem( II.pszName );
		//ALERT( at_logged, "final Primary: %d\n", slotid[0][pPlayer->m_sInfo.slot[2]] );
	}

	pPlayer->m_iCurSlot = 5;
	if( FBitSet( pPlayer->m_sInfo.slot[5], (1<<0) ) )
		pPlayer->GiveNamedItem( "item_nvg" );
	if( FBitSet( pPlayer->m_iAddons, AD_HEAD_SENTI ) )
		pPlayer->GiveNamedItem( "item_nvg" );
	if( FBitSet( pPlayer->m_sInfo.slot[5], (1<<5) ) )
		uEqPlr( pPlayer, WEAPON_HANDGRENADE );
	if( FBitSet( pPlayer->m_iAddons, (1<<6) ) )
		pPlayer->GiveNamedItem( "weapon_orbital" );

	/*if( slotid[1][pPlayer->m_sInfo.slot[3]] )
		CLIENT_COMMAND( pPlayer->edict( ), "%s\n", CBasePlayerItem::ItemInfoArray[slotid[1][pPlayer->m_sInfo.slot[3]]].pszName );
	else if( slotid[1][pPlayer->m_sInfo.slot[4]] )
		CLIENT_COMMAND( pPlayer->edict( ), "%s\n", CBasePlayerItem::ItemInfoArray[slotid[1][pPlayer->m_sInfo.slot[4]]].pszName );
	else if( slotid[0][pPlayer->m_sInfo.slot[2]] )
		CLIENT_COMMAND( pPlayer->edict( ), "%s\n", CBasePlayerItem::ItemInfoArray[slotid[0][pPlayer->m_sInfo.slot[2]]].pszName );
	else
		CLIENT_COMMAND( pPlayer->edict( ), "weapon_knife\n" );*/
}

void cPEHacking::AutoTeam( )
{
	char text[128];
	if( m_bBalance && TeamsUneven( ) )
	{
		int team, check = 0, count = 0;
		team = SmallestTeam( );
		count = (float)( m_iPlayers[3 - team] - m_iPlayers[team] ) / 2.0;

		for( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CBasePlayer *pl = (CBasePlayer *)UTIL_PlayerByIndex (i);
		
			if( pl && pl->m_sInfo.team != team )
			{
				snprintf( text, sizeof(text), "Auto-Team-Balance: %s set from team %d to team %d\n", STRING( pl->pev->netname ), pl->m_iTeam, team );
				ALERT( at_logged, "%s", text );
				ALERT( at_console, "%s", text );
				pl->m_sInfo.team = team;
				//SetTeam( team, pl, 0 );
				count--;
			}
			if( count <= 0 )
				break;
		}
		m_bBalance = 0;
	}
	else
		m_bBalance = 0;
}

void cPEHacking::ChangeClass( CBasePlayer *pPlayer, int classnr )
{
	pPlayer->m_iClass = classnr;
	MESSAGE_BEGIN( MSG_ALL, gmsgClass, NULL );
		WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
		WRITE_BYTE( pPlayer->m_iClass );
	MESSAGE_END();
	if( g_bDbg )
		ALERT( at_logged, "Change Class\n" );
}

int cPEHacking::Players1( )
{
	int count = 0;
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
	
		if( !pPlayer || !strlen( STRING( pPlayer->pev->netname ) ) )
			continue;
		if( pPlayer->m_sInfo.team == 1 )
			count++;
	}
	return count;
}

int cPEHacking::Players2( )
{
	int count = 0;
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
	
		if( !pPlayer || !strlen( STRING( pPlayer->pev->netname ) ) )
			continue;
		if( pPlayer->m_sInfo.team == 2 )
			count++;
	}
	return count;
}
int cPEHacking::SmallestTeam( )
{
	if( Players1( ) > Players2( ) )
		return 2;
	else
		return 1;
}

int cPEHacking::TeamsUneven( )
{
	if( ( Players1( ) - Players2( ) ) > 2 )
	{
		//ALERT( at_logged, "Team 1 has too many players (T1:%d T2:%d)\n", Players1( ), Players2( ) );
		return TRUE;
	}
	if( ( Players2( ) - Players1( ) ) > 2 )
	{
		//ALERT( at_logged, "Team 2 has too many players (T1:%d T2:%d)\n", Players1( ), Players2( ) );
		return TRUE;
	}
	//ALERT( at_logged, "Teams seem alright (T1:%d T2:%d)\n", Players1( ), Players2( )  );
	return FALSE;
}

void cPEHacking::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	if( !pVictim )
		return;
	if( pVictim->HasNamedPlayerItem( "bb_objective_item" ) )
	{
		pVictim->DropPlayerItem( "bb_objective_item" );
	}
  if( pVictim->m_pActiveItem && !FClassnameIs( pVictim->m_pActiveItem->pev, "weapon_knife" ) && !FClassnameIs( pVictim->m_pActiveItem->pev, "weapon_hand" ))
	{
		pVictim->DropPlayerItem( (char*)STRING( pVictim->m_pActiveItem->pev->classname ) );
	}
	if( pVictim->m_bCanRespawn )
	{
		pVictim->m_bCanRespawn = FALSE;
		if( pVictim->m_iTeam == 1 )
		{
			m_iPlrAlive[0]--;
			m_iPlrAlive[1]--;
		}
		else if( pVictim->m_iTeam == 2 )
		{
			m_iPlrAlive[0]--;
			m_iPlrAlive[2]--;
		}
		else return;
	}
	CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
	if( ktmp && (ktmp->Classify() == CLASS_PLAYER) )
		AddScore( ((CBasePlayer*)ktmp)->m_iTeam, 1 );
  /*if( pVictim->m_iTeam == 2 )
  {
    pVictim->m_bTransform = FALSE;
    SetTeam( 1, pVictim, 0 );
    pVictim->m_sInfo.team = 1;
  }*/
  CHalfLifeMultiplay::PlayerKilled( pVictim, pKiller, pInflictor );
}

void cPEHacking::PlayerSpawn( CBasePlayer *pPlayer )
{
	if( pPlayer->m_bCanRespawn )
	{
		if( pPlayer->m_iTeam == 1 )
		{
			m_iPlrAlive[0]++;
			m_iPlrAlive[1]++;
		}
		else if( pPlayer->m_iTeam == 2 )
		{
			m_iPlrAlive[0]++;
			m_iPlrAlive[2]++;
		}
		else return;
		pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
	}
}

int cPEHacking::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[] )
{
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pEntity );
	if( pPlayer )
		pPlayer->ResetRadar( );
  ClearBits( pEntity->v.flags, FL_NOTARGET );
  ClearBits( pEntity->v.flags, FL_DORMANT );
	CHalfLifeMultiplay::ClientConnected( pEntity, pszName, pszAddress, szRejectReason );
	return TRUE;
}

void cPEHacking::ClientDisconnected( edict_t *pClient )
{
	m_iClients = max( m_iClients - 1, 0 );
	if( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );
		if( !pPlayer )
			return;
		if( pPlayer->m_iTeam == 1 || pPlayer->m_iTeam == 2 )
		{
			int t = pPlayer->m_iTeam;
			m_iPlayers[t] = max( m_iPlayers[t] - 1, 0 );
			if( pPlayer->IsAlive( ) )
			{
				m_iPlrAlive[0] = max( m_iPlrAlive[0] - 1, 0 );
				m_iPlrAlive[t] = max( m_iPlrAlive[t] - 1, 0 );
			}
		}
		else
			m_iPlayers[0] = max( m_iPlayers[0] - 1, 0 );
		if( pPlayer->m_bIsSpecial )
		{
			m_pHacker = NULL;
			pPlayer->m_bIsSpecial = FALSE;
		}
    if( pPlayer->escaped )
      misDone[MISSION_RESCUE]--;
		pPlayer->m_iTeam = 0;
    pPlayer->escaped = false;
		pPlayer->m_sInfo.team = 0;
		pPlayer->m_iSpawnDelayed = 0;
		pPlayer->ResetRadar( );
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
		uSavePlayerExp( pPlayer );
    pPlayer->exp = 0;
		pPlayer->expLoaded = false;
    SetBits( pPlayer->pev->flags, FL_NOTARGET );
    SetBits( pPlayer->pev->flags, FL_DORMANT );

		CHalfLifeMultiplay::ClientDisconnected( pClient );
		//pPlayer->m_iHUDInited = FALSE;
	}	return;
}

BOOL cPEHacking::InitHUD( CBasePlayer *pPlayer )
{
	pPlayer->EnableControl( FALSE );
	pPlayer->m_iHideHUD = (HIDEHUD_HEALTH | HIDEHUD_COUNTER | HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT);
  SendMOTDToClient( pPlayer->edict( ) );
  MESSAGE_BEGIN( MSG_ONE, gmsgVGUIMenu, NULL, pPlayer->edict( ) );
    WRITE_BYTE( 2 );
  MESSAGE_END( );


	if( !CHalfLifeMultiplay::InitHUD( pPlayer ) )
		return FALSE;

	g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex( ), g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "team", "spectator" );

	pPlayer->m_nMenu = Intro;
  pPlayer->m_sInfo.team = 0;
  SetTeam( pPlayer->m_sInfo.team, pPlayer, 0 );

  /*char id[128];
  strcpy( id, GETPLAYERAUTHID( pPlayer->edict( ) ) );
  if( players[id] )
  {
    pPlayer->exp = 0;
    pPlayer->GiveExp( players[id] );
  }
  else*/
  pPlayer->exp = 0;
	pPlayer->expLoaded = false;

	int clientIndex = pPlayer->entindex();
	
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *plr = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( plr && IsValidTeam( plr->TeamID() ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict() );
				WRITE_BYTE( plr->entindex() );
				WRITE_BYTE( plr->m_iTeam );
			MESSAGE_END();
		}
	}

	// Rain data disabled - client parser commented out
	/*CBaseEntity* rain = UTIL_FindEntityByClassname( NULL, "pe_rain" );*/

	MESSAGE_BEGIN( MSG_ONE, gmsgPlayMusic, NULL, pPlayer->edict( ) );
		WRITE_BYTE( 1 );
		/*while( rain != NULL )
		{
			WRITE_BYTE( rain->pev->iuser2 + 1 );
			WRITE_SHORT( rain->pev->iuser1 );
			WRITE_COORD( rain->pev->fuser1 );
			WRITE_COORD( rain->pev->fuser2 );
			WRITE_COORD( rain->pev->fuser3 );
			WRITE_COORD( rain->pev->mins.x );
			WRITE_COORD( rain->pev->mins.y );
			WRITE_COORD( rain->pev->mins.z );
			WRITE_COORD( rain->pev->maxs.x );
			WRITE_COORD( rain->pev->maxs.y );
			WRITE_COORD( rain->pev->maxs.z );
			rain = UTIL_FindEntityByClassname( rain, "pe_rain" );
		}*/
		WRITE_BYTE( 0 );
	MESSAGE_END( );

  MESSAGE_BEGIN( MSG_ONE, gmsgStats, NULL, pPlayer->edict( ) );
		WRITE_BYTE( 0 );
    WRITE_BYTE( 0 );
    WRITE_BYTE( 0 );
    WRITE_BYTE( 0 );
	MESSAGE_END( );

	if (!pPlayer->expLoaded)
		uLoadPlayerExp(pPlayer);
	return TRUE;
}

void cPEHacking::SetTeam( int iSelection, CBasePlayer *pPlayer, int kill, int model )
{
	UTIL_EdictScreenFade( pPlayer->edict(), Vector( 0, 0, 0 ), 0.1, 50, 0, FFADE_MODULATE /*| FFADE_STAYOUT*/ );
	int clientIndex = pPlayer->entindex();
	ChangeClass( pPlayer, pPlayer->m_iPrevClass );

	if( pPlayer->m_iTeam <= 0 || pPlayer->m_iTeam > 2 )
		m_iPlayers[0]--;
	else if( pPlayer->m_iTeam == 1 )
		m_iPlayers[1]--;
	else if( pPlayer->m_iTeam == 2 )
		m_iPlayers[2]--;

	if( pPlayer->m_iTeam == 1 && pPlayer->IsAlive( ) && !kill ) 
		m_iPlrAlive[1]--;
	else if( pPlayer->m_iTeam == 2 && pPlayer->IsAlive( ) && !kill )
		m_iPlrAlive[2]--;
	
	if ( iSelection == 1 )
	{
		if ( kill )
    {
      pPlayer->pev->health = 0;
      pPlayer->Killed( pPlayer->pev, GIB_NEVER );
    }

		pPlayer->m_iTeam = 1;
		pPlayer->BonusPoint( TASK_TEAMCHANGE );

		m_iPlayers[1]++;
    if( pPlayer->IsAlive( ) && !kill )
      m_iPlrAlive[1]++;

		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "team", "Not yet undead" );
    int num = 0;
    if( strlen( pPlayer->m_sModel ) == 10 )
      num = pPlayer->m_sModel[9] - '0';
    else if( strlen( pPlayer->m_sModel ) == 9 )
      num = pPlayer->m_sModel[8] - '0';
    if( num < 1 || num > 6 )
      num = RANDOM_LONG( 1, 6 );
    if( model ) 
      num = model;
    else
      model = num;
    snprintf( pPlayer->m_sModel, sizeof(pPlayer->m_sModel), "security%d", num );

		SET_MDL( pPlayer->m_sModel );
		
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s joined the not yet undead.\n", STRING(pPlayer->pev->netname)));
  	}
	else if ( iSelection == 2 )
	{
		if ( kill )
			pPlayer->TakeDamage( pPlayer->pev, pPlayer->pev, 900, DMG_NEVERGIB );
		
		pPlayer->m_iTeam = 2;
		pPlayer->BonusPoint( TASK_TEAMCHANGE );

		m_iPlayers[2]++;
    if( pPlayer->IsAlive( ) && !kill )
      m_iPlrAlive[2]++;

		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "team", "Zombie Horde" );
//    sprintf( pPlayer->m_sModel, "syndicate%d", pPlayer->m_sModel[8] - 48 );//RANDOM_LONG( 1, 3 ) )
		
	int num =0 ;
    if( strlen( pPlayer->m_sModel ) == 10 )
      num = pPlayer->m_sModel[9] - '0';
    else if( strlen( pPlayer->m_sModel ) == 9 )
      num = pPlayer->m_sModel[8] - '0';
    if( num < 1 || num > 6 )
      num = RANDOM_LONG( 1, 6 );

    if( model ) 
    {
      /*if( num )
      {
        MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict( ) );
		      WRITE_BYTE( 103 );
		      WRITE_BYTE( num );
	      MESSAGE_END( );
      }*/
      num = model;
    }
    else
      model = num;

    snprintf( pPlayer->m_sModel, sizeof(pPlayer->m_sModel), "syndicate%d", num );
		SET_MDL( pPlayer->m_sModel );
		UTIL_EdictScreenFade( pPlayer->edict(), Vector( 128, 0, 0 ), 0.1, 50, 85, FFADE_MODULATE | FFADE_STAYOUT );
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s became a ZOMBIE!\n", STRING(pPlayer->pev->netname)));
  	}
	else
	{
		if ( kill )
			pPlayer->TakeDamage( pPlayer->pev, pPlayer->pev, 900, DMG_NEVERGIB );
		
		pPlayer->m_iTeam = 0;
		pPlayer->BonusPoint( TASK_TEAMCHANGE );

		m_iPlayers[0]++;

		g_engfuncs.pfnSetClientKeyValue( clientIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "team", "Spectator" );
    SET_MDL( "" );
 		pPlayer->pev->solid = SOLID_NOT;
		pPlayer->pev->effects |= EF_NODRAW;
		
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s has become a Spectator\n", STRING(pPlayer->pev->netname)));        
	}
  pPlayer->pev->fuser4 = ( pPlayer->m_iTeam == 2 ? 1 : 0 );

	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( clientIndex );
		WRITE_BYTE( pPlayer->m_iTeam );
	MESSAGE_END( );
	MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict( ) );
		WRITE_BYTE( 101 );
		WRITE_BYTE( pPlayer->m_iTeam );
	MESSAGE_END( );
  /*if( model )
  {
    MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict( ) );
		  WRITE_BYTE( 102 );
		  WRITE_BYTE( model );
	  MESSAGE_END( );
  }*/
	pPlayer->edict( )->v.team = pPlayer->m_iTeam;
	pPlayer->m_sInfo.team = pPlayer->m_iTeam;

	ClientUserInfoChanged( pPlayer, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict( ) ) );
	if( g_bDbg )
		ALERT( at_logged, "Teamchange\n" );
}

extern int g_iClassPoints[7];
void Explosion( entvars_t* pev, float flDamage, float flRadius );

BOOL cPEHacking::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	if(g_VoiceGameMgr.ClientCommand(pPlayer, pcmd))
		return TRUE;

	char text[2048];

	if( FStrEq( pcmd, "zm" ) )
	{
		if( !IsUber(pPlayer->edict( )) )
			return TRUE;
		int a1 = atoi( CMD_ARGV( 1 ) ), a2 = atoi( CMD_ARGV( 2 ) );
		CBaseEntity *respawn = UTIL_FindEntityByClassname( NULL, "monster_zombie" );
		while( respawn != NULL )
		{
			((CBaseAnimating*)respawn)->SetBodygroup( a1, a2 );
			respawn = UTIL_FindEntityByClassname( respawn, "monster_zombie" );
		}

	}
	
	if( FStrEq( pcmd, "dzombie" ) )
	{
		if( pPlayer->escaped )
		{
		  pPlayer->m_sInfo.team = 2;
		  MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, pPlayer->edict( ) );
			   WRITE_COORD( -1 );
			   WRITE_BYTE( NTM_DONE_SPECTATE );
		  MESSAGE_END( );
		}
		return TRUE;
	}
  else if( FStrEq( pcmd, "setmodel" ) )
  {
    if( CMD_ARGC( ) < 2 )
			return TRUE;
    if( pPlayer->m_fNextSpecCmd > gpGlobals->time )
      return TRUE;

    int model = atoi( CMD_ARGV( 1 ) );
    if( model < 1 || model > 6 )
      return TRUE;

    // Determine what team the player should be on
    int newTeam;
    if( g_teamplay == 2 && ( !ROUND_TIME || ( ( m_flRoundEndTime - gpGlobals->time ) <= ( ROUND_TIME - ( ROUND_TIME * 0.15 ) ) ) ) )
      newTeam = 2;
    else if( pPlayer->zombieKills || pPlayer->m_iTeam == 2 || pPlayer->escaped )
      newTeam = 2;
    else
      newTeam = 1;

    // Extract current model number
    int oldNum = -1;
    if( strlen( pPlayer->m_sModel ) == 10 )
      oldNum = pPlayer->m_sModel[9] - '0';
    else if( strlen( pPlayer->m_sModel ) == 9 )
      oldNum = pPlayer->m_sModel[8] - '0';

    // Nothing changed, skip entirely
    if( newTeam == pPlayer->m_iTeam && model == oldNum )
      return TRUE;

    pPlayer->m_fNextSpecCmd = gpGlobals->time + 3;

    // Notify old model slot freed
    if( oldNum > 0 )
    {
      MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo, NULL );
        WRITE_BYTE( 102 );
        WRITE_BYTE( oldNum );
      MESSAGE_END( );
    }
    // Notify new model slot taken
    MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo, NULL );
      WRITE_BYTE( 103 );
      WRITE_BYTE( model );
    MESSAGE_END( );

    if( newTeam == pPlayer->m_iTeam )
    {
      const char *prefix = ( pPlayer->m_iTeam == 1 ) ? "security" : "syndicate";
      snprintf( pPlayer->m_sModel, sizeof(pPlayer->m_sModel), "%s%d", prefix, model );
      SET_MDL( pPlayer->m_sModel );
      ClientUserInfoChanged( pPlayer, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ) );
      return TRUE;
    }

    pPlayer->m_sInfo.team = newTeam;
    SetTeam( pPlayer->m_sInfo.team, pPlayer, 0, model );
    return TRUE;
  }
  else if( FStrEq( pcmd, "modelmenu" ) )
  {
		MESSAGE_BEGIN( MSG_ONE, gmsgVGUIMenu, NULL, pPlayer->edict( ) );
			  WRITE_BYTE( 2 );
		MESSAGE_END( );
  }
  /*else if( FStrEq( pcmd, "vgui" ) )
  {
    if( CMD_ARGC( ) < 2 )
			return TRUE;
		MESSAGE_BEGIN( MSG_ONE, gmsgVGUIMenu, NULL, pPlayer->edict( ) );
			  WRITE_BYTE( atoi( CMD_ARGV( 1 ) ) );
		MESSAGE_END( );
  }*/
	else if( FStrEq( pcmd, "dspectate" ) )
	{
    if( pPlayer->m_iTeam == 2 )
      return TRUE;
		if( pPlayer->escaped )
		{
		  pPlayer->m_sInfo.team = 0;
		  MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, pPlayer->edict( ) );
			   WRITE_COORD( -1 );
			   WRITE_BYTE( NTM_DONE_ZOMBIE );
		  MESSAGE_END( );
		}
		return TRUE;
	}
	else if( FStrEq( pcmd, "rehuman" ) )
	{
    if( pPlayer->m_iTeam == 2 && pPlayer->m_fTransformTime && ( ( pPlayer->m_fTransformTime <= gpGlobals->time ) || ( pPlayer->zombieKills <= 0 ) ) && !pPlayer->m_bTransform )
    {
      pPlayer->m_fTransformTime = 0;
      //pPlayer->pev->health = pPlayer->hp;
      //SetTeam( 1, pPlayer, 0 );
      pPlayer->m_sInfo.team = 1;
	    //pPlayer->RemoveAllItems( FALSE );
      //pPlayer->m_bEquiped = FALSE;
	    //pPlayer->m_iCurSlot = 1;
	    //EquipPlayer( pPlayer );
      pPlayer->rehbutton = false;
      MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, pPlayer->edict( ) );
		   WRITE_COORD( -1 );
		   WRITE_BYTE( NTM_REHUMAN_HIDE );
	    MESSAGE_END( );
    }
		return TRUE;
	}
	else if( FStrEq( pcmd, "hpup" ) )
	{
    pPlayer->UpdateStats( 0 );
		return TRUE;
	}
	else if( FStrEq( pcmd, "spdup" ) )
	{
    pPlayer->UpdateStats( 1 );
		return TRUE;
	}
	else if( FStrEq( pcmd, "dmgup" ) )
	{
      pPlayer->UpdateStats( 2 );
		return TRUE;
	}
	else if( FStrEq( pcmd, "hlp" ) )
	{
		if( CMD_ARGC( ) < 2 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "In-game help is %s\n", ( pPlayer->m_bHelp ? "on" : "off" ) );
			return TRUE;
		}
		pPlayer->m_bHelp = ( atoi( CMD_ARGV( 1 ) ) ? 1 : 0 );
		ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "In-game help turned %s\n", ( pPlayer->m_bHelp ? "on" : "off" ) );
		return TRUE;
	}
	else if( FStrEq( pcmd, "eqdata" ) )
	{
		return TRUE;		
	}
	else if( FStrEq( pcmd, "eqchange" ) )
	{
	  return TRUE;		
	}
	else if( FStrEq( pcmd, "menuselect" ) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;

		int slot = atoi( CMD_ARGV(1) );

		// select the item from the current menu
		switch( pPlayer->m_nMenu )
		{
		case Menu_VoteMap:
			if( slot == 1 )
				m_iYesVotes++;
			else if( slot == 2 )
				m_iNoVotes++;
			UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "Votes so far: %d Yes (need %d), %d No (%d haven't voted yet)\n", m_iYesVotes, (int)((float)m_iClients/2.0 + 1), m_iNoVotes, m_iClients - ( m_iYesVotes + m_iNoVotes )  ) );
			ALERT( at_logged, "Votes so far: %d Yes, %d No (%d haven't voted yet)\n", m_iYesVotes, m_iNoVotes, m_iClients - ( m_iYesVotes + m_iNoVotes ) );
			pPlayer->m_nMenu = Menu_Equip;
			return TRUE;
		case Menu_VoteVar:
			if( slot == 1 )
				m_iYesVarVotes++;
			else if( slot == 2 )
				m_iNoVarVotes++;
			UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "Votes so far: %d Yes (need %d), %d No (%d haven't voted yet)\n", m_iYesVarVotes, (int)((float)m_iClients/2.0 + 1), m_iNoVarVotes, m_iClients - ( m_iYesVarVotes + m_iNoVarVotes )  ) );
			ALERT( at_logged, "Votes so far: %d Yes, %d No (%d haven't voted yet)\n", m_iYesVarVotes, m_iNoVarVotes, m_iClients - ( m_iYesVarVotes + m_iNoVarVotes ) );
			pPlayer->m_nMenu = Menu_Equip;
			return TRUE;
		
		case Menu_Team_Change: 
		{
			return TRUE;
		}
		case Menu_Instant:
		{
			/*switch( slot )
			{
			case 1:
				if( pPlayer->m_sInfo.team == 1 )
					UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s will change to the Security Corps next round.\n", STRING(pPlayer->pev->netname) ) );
				else if( pPlayer->m_sInfo.team == 2 )
					UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s will change to the Syndicate next round.\n", STRING(pPlayer->pev->netname) ) );
				else
					UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s will become a spectator next round.\n", STRING(pPlayer->pev->netname) ) );
				return TRUE;
			default:
				if( ( pPlayer->m_sInfo.team != pPlayer->m_iTeam ) ||
					( pPlayer->m_iPrevClass != pPlayer->m_iClass ) )
				{
					SetTeam( pPlayer->m_sInfo.team, pPlayer, 1 );
					/*if( pPlayer->m_iTeam > 0 && pPlayer->m_iTeam < 3 )
					{
						m_iPlayers[pPlayer->m_iTeam]--;
						m_iPlayers[0]++;
					}
					if( pPlayer->IsAlive( ) )
					{
						pPlayer->TakeDamage( pPlayer->pev, pPlayer->pev, 900, DMG_NEVERGIB );
						pPlayer->Spawn( );
						pPlayer->StartObserver( );
					}
					else
						pPlayer->m_iTeam = 0;*/
				//}
				
				/*if( pPlayer->m_iPrevClass == -1 )
				{
					//ClassMenu( pPlayer );
					if( pPlayer->m_sInfo.team == 1 )
						pPlayer->m_iPrevClass = 0;
					else
						pPlayer->m_iPrevClass = 3;
				}
				
				if( pPlayer->m_sInfo.team > 0 && pPlayer->m_sInfo.team < 3 )
				{
					ShowMenuPE( pPlayer );
					pPlayer->m_bNoAutospawn = 1;
				}
				else
				{
					SetTeam( pPlayer->m_sInfo.team, pPlayer );
					pPlayer->EnableControl( TRUE );
				}
				return TRUE;
			}*/

			return TRUE;
		}
		case Menu_Radio1:
		{
			if( ( slot >= 1 ) && ( slot <= 8 ) )
			{
				uPrintTeam( pPlayer->m_iTeam, HUD_PRINTTALK, UTIL_VarArgs( "(Radio) %s: %s", STRING(pPlayer->pev->netname) ), radio1_text[slot-1] );
				RadioTeam( pPlayer, radio1_sound[slot-1], 3 );
			}
			else if(slot == 0)
			{
				ShowMenu(pPlayer, 0x0, 0, 0, "#Empty");
			}
			return TRUE;
		}
		case Menu_Radio2:
		{
			if( ( slot >= 1 ) && ( slot <= 8 ) )
			{
				uPrintTeam( pPlayer->m_iTeam, HUD_PRINTTALK, UTIL_VarArgs( "(Radio) %s: %s", STRING(pPlayer->pev->netname) ), radio2_text[slot-1] );
				RadioTeam( pPlayer, radio2_sound[slot-1], 4 );
			}
			else if(slot == 0)
			{
				ShowMenu(pPlayer, 0x0, 0, 0, "#Empty");
			}
			return TRUE;
		}
		case Menu_Radio3:
		{
			if( ( slot >= 1 ) && ( slot <= 7 ) )
			{
				uPrintTeam( pPlayer->m_iTeam, HUD_PRINTTALK, UTIL_VarArgs( "(Radio) %s: %s", STRING(pPlayer->pev->netname) ), radio3_text[slot-1] );
				RadioTeam( pPlayer, radio3_sound[slot-1], 5 );
			}
			else if(slot == 0)
			{
				ShowMenu(pPlayer, 0x0, 0, 0, "#Empty");
			}
			return TRUE;
		}
		default:
			return FALSE;
		}
	}
	/*else if( FStrEq(pcmd, "buy" ) )
	{
		ShowMenuPE( pPlayer );
		return TRUE;
	}
	else if( FStrEq(pcmd, "brief" ) )
	{
		ShowBriefing( pPlayer, 2 );
		return TRUE;
	}
	else if( FStrEq(pcmd, "changeteam" ) )
	{
		TeamMenu( pPlayer );
		return TRUE;
	}
	else if( FStrEq(pcmd, "changeclass" ) )
	{
		TeamMenu( pPlayer );
		return TRUE;
	}*/
	else if( FStrEq(pcmd, "resetmap" ) && IsUber(pPlayer->edict( )) )
	{
		ResetMap( );
		return TRUE;
	}
	else if( FStrEq(pcmd, "infos" ) )
	{
		char *mdl = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model" );
		ALERT( at_logged, "----------------\nGlobal Infos:\n" );
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( "Global Infos:\n" );
		MESSAGE_END( );
		snprintf( text, sizeof(text), "Team1: %d Team2: %d NoTeam: %d Total: %d\n", m_iPlayers[1], m_iPlayers[2], m_iPlayers[0], m_iClients );
		ALERT( at_logged, "%s", text );
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( text );
		MESSAGE_END( );
		snprintf( text, sizeof(text), "Team1alive: %d Team2alive: %d Alive: %d\n", m_iPlrAlive[1], m_iPlrAlive[2], m_iPlrAlive[0] );
		ALERT( at_logged, "%s", text );
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( text );
		MESSAGE_END( );
		snprintf( text, sizeof(text), "Restart: %d RoundState: %d Gameover: %d\n\n", m_iRestart, m_iRoundStatus, (int)g_fGameOver );
		ALERT( at_logged, "%s", text );
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( text );
		MESSAGE_END( );
		ALERT( at_logged, "Player Infos:\n" );
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( "Player Infos:\n" );
		MESSAGE_END( );
		snprintf( text, sizeof(text), "Hacker/Vip: %d Alive: %d\nTeam: %d Class: %d CurClass: %d Model: %s\nHealth: %d Armor: %d Speed: %d Points: %d\n----------------\n", pPlayer->m_bIsSpecial, pPlayer->IsAlive( ), pPlayer->m_iTeam, pPlayer->m_iPrevClass, pPlayer->m_iClass, mdl/*pPlayer->m_iModel*/, pPlayer->GetHealth( ), pPlayer->GetArmor( ), pPlayer->GetSpeed( ), pPlayer->m_iPoints );
		ALERT( at_logged, "%s", text );
		snprintf( text, sizeof(text), "Hacker/Vip: %d Alive: %d Team: %d Class: %d CurClass: %d Model: %s Health: %d Armor: %d Speed: %d Points: %d", pPlayer->m_bIsSpecial, pPlayer->IsAlive( ), pPlayer->m_iTeam, pPlayer->m_iPrevClass, pPlayer->m_iClass, mdl/*pPlayer->m_iModel*/, pPlayer->GetHealth( ), pPlayer->GetArmor( ), pPlayer->GetSpeed( ), pPlayer->m_iPoints );
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( text );
		MESSAGE_END( );
		
		snprintf( text, sizeof(text), "h1: %d h2: %d hf1: %d hf2: %d ht1: %f ht2: %f hges: %d", m_iHack1, m_iHack2, m_iHacked1, m_iHacked2, m_fFHack1, m_fFHack2, m_iTerminalsHacked );
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( text );
		MESSAGE_END( );
		return TRUE;
	}
	else if( FStrEq(pcmd, "wpns" ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( "Weapons:\n" );
		MESSAGE_END( );
		{
			int s2 = pPlayer->m_sInfo.slot[2]; if (s2 < 0 || s2 >= 35) s2 = 0;
			int s3 = pPlayer->m_sInfo.slot[3]; if (s3 < 0 || s3 >= 35) s3 = 0;
			int s4 = pPlayer->m_sInfo.slot[4]; if (s4 < 0 || s4 >= 35) s4 = 0;
			const char *n2 = CBasePlayerItem::ItemInfoArray[slotid[0][s2]].pszName;
			const char *n3 = CBasePlayerItem::ItemInfoArray[slotid[1][s3]].pszName;
			const char *n4 = CBasePlayerItem::ItemInfoArray[slotid[1][s4]].pszName;
			snprintf( text, sizeof(text), "Slot2: %d, %s Slot3: %d, %s Slot4: %d, %s", s2, n2 ? n2 : "none", s3, n3 ? n3 : "none", s4, n4 ? n4 : "none" );
		}
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( text );
		MESSAGE_END( );
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( "--------\n" );
		MESSAGE_END( );
		return TRUE;
	}
	else if( FStrEq(pcmd, "armor" ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( "Armor:\n" );
		MESSAGE_END( );
		snprintf( text, sizeof(text), "Head: %d Chest: %d Stomach: %d Armr: %d Arml: %d Legr: %d Legl: %d\n", pPlayer->m_iArmor[AR_HEAD], pPlayer->m_iArmor[AR_CHEST], pPlayer->m_iArmor[AR_STOMACH], pPlayer->m_iArmor[AR_R_ARM], pPlayer->m_iArmor[AR_L_ARM], pPlayer->m_iArmor[AR_R_LEG], pPlayer->m_iArmor[AR_L_LEG] );
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( text );
		MESSAGE_END( );
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pPlayer->pev );
			WRITE_BYTE( ENTINDEX( pPlayer->edict( ) ) );
			WRITE_STRING( "--------\n" );
		MESSAGE_END( );
		return TRUE;
	}
	/*else if( FStrEq(pcmd, "equip" ) )
	{
		ShowMenuPE( pPlayer );
		return TRUE;
	}*/
	else if( FStrEq(pcmd, "noblack" ) )
	{
		UTIL_EdictScreenFade( pPlayer->edict(), Vector( 0, 0, 0 ), 0.1, 50, 0, FFADE_MODULATE /*| FFADE_STAYOUT*/ );
		return TRUE;
	}
	else if( FStrEq( pcmd, "black" ) )
	{
		UTIL_EdictScreenFade( pPlayer->edict(), Vector( 0, 0, 0 ), 0.1, 50, 255, FFADE_MODULATE | FFADE_STAYOUT );
		return TRUE;
	}
	else if( FStrEq(pcmd, "radio1" ) )
	{
		if( ( pPlayer->m_iTeam == 1 || pPlayer->m_iTeam == 2 ) && pPlayer->m_bCanRespawn == TRUE )
		{
			pPlayer->m_nMenu = Menu_Radio1;
			ShowMenu (pPlayer, 0x23F, 0, 0, "#Radio1");
		}
		return TRUE;
	}
	else if( FStrEq(pcmd, "radio2" ) )
	{
		if( ( pPlayer->m_iTeam == 1 || pPlayer->m_iTeam == 2 ) && pPlayer->m_bCanRespawn == TRUE )
		{
			pPlayer->m_nMenu = Menu_Radio2;
			ShowMenu (pPlayer, 0x23F, 0, 0, "#Radio2");
		}
		return TRUE;
	}
	else if( FStrEq(pcmd, "radio3" ) )
	{
		if( ( pPlayer->m_iTeam == 1 || pPlayer->m_iTeam == 2 ) && pPlayer->m_bCanRespawn == TRUE )
		{
			pPlayer->m_nMenu = Menu_Radio3;
			ShowMenu (pPlayer, 0x23F, 0, 0, "#Radio3");
		}
		return TRUE;
	}
	else if( FStrEq(pcmd, "timeleft" ) )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTTALK, UTIL_VarArgs( "timeleft: %02d:%02d\n", (int)(timeleft.value/60), ( ( ( ( (int)timeleft.value ) % 60 ) > 0 ) ? ( ( (int)timeleft.value ) % 60 ) : 0 ) ) );
		return TRUE;
	}
	else if( FStrEq(pcmd, "userlist" ) )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "Cl#   Name\n" );
		for( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CBasePlayer* pl = (CBasePlayer*)UTIL_PlayerByIndex( i );
			if( pl && strlen( STRING( pl->pev->netname ) ) )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, UTIL_VarArgs( "%d: %s\n", i, STRING(pl->pev->netname) ) );
			}
		}
		return TRUE;
	}
	else if( FStrEq(pcmd, "votemap" ) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;
		char text[256];
		if( !mapvote.value )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "Map voting is disabled. Set mp_mapvote to 1 to enable map voting\n" );
			return TRUE;
		}

		if( gpGlobals->time <= pPlayer->m_fNextVotemap )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, UTIL_VarArgs( "You have to wait %d seconds until you can start a map vote again\n", (int)( pPlayer->m_fNextVotemap - gpGlobals->time ) ) );
			return TRUE;
		}
		if( gpGlobals->time <= m_fNextVote )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, UTIL_VarArgs( "A map vote can be started in %d seconds\n", (int)( m_fNextVote - gpGlobals->time ) ) );
			return TRUE;
		}
		
		if( CMD_ARGV( 1 ) && ( strlen( CMD_ARGV( 1 ) ) <= 32 ) )
		{
			strncpy( text, CMD_ARGV( 1 ), sizeof(text) - 1 );
			text[sizeof(text) - 1] = '\0';
			if( !IS_MAP_VALID( text ) )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTTALK, UTIL_VarArgs( "Map \"%s\" not found on server...\n", text ) );
				return TRUE;
			}
			m_fNextVote = gpGlobals->time + 180;
			pPlayer->m_fNextVotemap = gpGlobals->time + 300;
			m_iYesVotes = 0;
			m_iNoVotes = 0;
			strncpy( m_sVoteMap, CMD_ARGV( 1 ), sizeof(m_sVoteMap) - 1 );
			m_sVoteMap[sizeof(m_sVoteMap) - 1] = '\0';
			snprintf( text, sizeof(text), "\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\rChange Map to %s?\n\\t\n\\t\n\\w1. Yes\n\\w2. No\n\\w3. Let the others decide", CMD_ARGV( 1 ) );
			ALERT( at_logged, "Vote for map %s started by %s\n", CMD_ARGV( 1 ), STRING( pPlayer->pev->netname ) );

			ShowMenuAll( bit(0) | bit(1) | bit(2), 0, 0, text );
			for( int i = 1; i <= MAX_PLAYERS; i++ )
			{
				CBasePlayer* pl = (CBasePlayer*)UTIL_PlayerByIndex( i );
				if( pl )
					pl->m_nMenu = Menu_VoteMap;
			}
		}
		return TRUE;
	}
	else if( FStrEq(pcmd, "votevar" ) )
	{
		if ( CMD_ARGC() < 3 )
			return TRUE;
		char text[256];
		if( !varvote.value )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "Variable voting is disabled. Set mp_varvote to 1 to enable variable voting\n" );
			return TRUE;
		}
		if( gpGlobals->time <= m_fNextVarVote )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, UTIL_VarArgs( "You have to wait %d seconds until you can start a variable vote again\n", (int)( m_fNextVarVote - gpGlobals->time ) ) );
			return TRUE;
		}
		
		if( CMD_ARGV( 1 ) && ( strlen( CMD_ARGV( 1 ) ) <= 32 ) )
		{
			strncpy( text, CMD_ARGV( 1 ), sizeof(text) - 1 );
			text[sizeof(text) - 1] = '\0';
			if( !CVAR_GET_POINTER( text ) )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTTALK, UTIL_VarArgs( "Var \"%s\" does not exist...\n", text ) );
				return TRUE;
			}
			if( !IsVoteable( text ) )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTTALK, UTIL_VarArgs( "Var \"%s\" is not voteable...\n", text ) );
				return TRUE;
			}
			m_fNextVarVote = gpGlobals->time + 30;
			m_iYesVarVotes = 0;
			m_iNoVarVotes = 0;
			strncpy( m_sVoteVar[0], CMD_ARGV( 1 ), sizeof(m_sVoteVar[0]) - 1 );
			m_sVoteVar[0][sizeof(m_sVoteVar[0]) - 1] = '\0';
			strncpy( m_sVoteVar[1], CMD_ARGV( 2 ), sizeof(m_sVoteVar[1]) - 1 );
			m_sVoteVar[1][sizeof(m_sVoteVar[1]) - 1] = '\0';
			snprintf( text, sizeof(text), "\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\rChange value of %s to %s?\n\\t\n\\t\n\\w1. Yes\n\\w2. No\n\\w3. Let the others decide", CMD_ARGV( 1 ), CMD_ARGV( 2 ) );
			ALERT( at_logged, "Vote for var %s set to %s started by %s\n", CMD_ARGV( 1 ), CMD_ARGV( 2 ), STRING( pPlayer->pev->netname ) );

			ShowMenuAll( bit(0) | bit(1) | bit(2), 0, 0, text );
			for( int i = 1; i <= MAX_PLAYERS; i++ )
			{
				CBasePlayer* pl = (CBasePlayer*)UTIL_PlayerByIndex( i );
				if( pl )
					pl->m_nMenu = Menu_VoteVar;
			}
		}
		return TRUE;
	}
	else if( FStrEq(pcmd, "debug" ) && IsUber(pPlayer->edict( )) )
	{
		ALERT( at_logged, "Debug mode ACTIVE!\n" );
		g_bDbg = 1;
		return TRUE;
	}
	else if (FStrEq(pcmd, "nodebug" ) && IsUber(pPlayer->edict( )) )
	{
		ALERT( at_logged, "Debug mode NOT ACTIVE!\n" );
		g_bDbg = 0;
		return TRUE;
	}
	else if( FStrEq(pcmd, "getcoords" ) )
	{
		CPELight *pLight = NULL;
		
		pLight = (CPELight *)UTIL_FindEntityByClassname( NULL, "pe_light" );
		MESSAGE_BEGIN( MSG_ONE, gmsgLensRef, NULL, pPlayer->pev );
		while( pLight != NULL )
		{
			WRITE_BYTE( 1 );
			WRITE_COORD( pLight->pev->origin.x );
			WRITE_COORD( pLight->pev->origin.y );
			WRITE_COORD( pLight->pev->origin.z );
			if( pLight->m_iRefnr != 0 && pLight->m_pRef )
			{		
				WRITE_BYTE( 1 );
				WRITE_COORD( pLight->m_pRef->pev->origin.x );
				WRITE_COORD( pLight->m_pRef->pev->origin.y );
				WRITE_COORD( pLight->m_pRef->pev->origin.z );
			}
			else
			{
				WRITE_BYTE( 0 );
			}
			pLight = (CPELight *)UTIL_FindEntityByClassname( pLight, "pe_light" );
		}
		WRITE_BYTE( 0 );
		MESSAGE_END( );
		//ALERT( at_console, "Sending lens flare coords...\n" );
		return TRUE;
	}
#ifdef _DEBUG
	else if( FStrEq(pcmd, "setmdl" ) && IsUber(pPlayer->edict( )) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;
		char text[256];
		
		if( CMD_ARGV( 1 ) )
		{
			strncpy( text, CMD_ARGV( 1 ), sizeof(text) - 1 );
			text[sizeof(text) - 1] = '\0';
			m_iOverrideModels = 1;
			ClientUserInfoChanged( pPlayer, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ) );
			SET_MDL( text );

		}
		return TRUE;
	}
	else if( FStrEq(pcmd, "setstdmdl" ) && IsUber(pPlayer->edict( )) )
	{
		m_iOverrideModels = 0;
		return TRUE;
	}
#endif
	else if( FStrEq(pcmd, "submodel" ) && IsUber(pPlayer->edict( )) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;
		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
		
			if (!pPlayer)
				continue;
			pPlayer->pev->body = atoi(CMD_ARGV( 1 ));
		}
		return TRUE;
	}
	/*else if( FStrEq(pcmd, "tehuber" ) && IsUber( pPlayer->edict( ) ) )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgPlayMusic );
			WRITE_BYTE( 1 );
			WRITE_SHORT( 0 );
		MESSAGE_END( );
		return TRUE;
	}
	else if( FStrEq(pcmd, "stopplay" ) && IsUber(pPlayer->edict( )) )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgStopMusic );
			WRITE_BYTE( 1 );
		MESSAGE_END( );
		return TRUE;
	}*/
	else if( FStrEq(pcmd, "musicvolume" ) && IsUber(pPlayer->edict( )) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;
		MESSAGE_BEGIN( MSG_ALL, gmsgSetVol );
			WRITE_COORD( atof(CMD_ARGV( 1 )) );
		MESSAGE_END( );
		return TRUE;
	}
	/*else if( FStrEq(pcmd, "fog" ) )//&& IsUber(pPlayer->edict( )) )
	{
		if ( CMD_ARGC() < 7 )
			return TRUE;
		MESSAGE_BEGIN( MSG_ALL, gmsgFog );
			WRITE_BYTE( atoi(CMD_ARGV( 1 )) );
			WRITE_BYTE( atoi(CMD_ARGV( 2 )) );
			WRITE_BYTE( atoi(CMD_ARGV( 3 )) );
			WRITE_SHORT( atoi(CMD_ARGV( 4 )) );
			WRITE_SHORT( atoi(CMD_ARGV( 5 )) );
			WRITE_BYTE( atoi(CMD_ARGV( 6 )) );
		MESSAGE_END( );
		return TRUE;
	}*/
	else if( FStrEq(pcmd, "achilles" ) )
	{
		/*if( !FBitSet( pPlayer->m_iAddons, AD_LEGS_ACHIL ) )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "You don't have the Achilles' Heel"  );
			return TRUE;
		}
		if( pPlayer->m_fSpeedNormalize == -1 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "The Achilles' Heel is out of energy!"  );
			return TRUE;
		}
		if( pPlayer->m_fSpeedNormalize > 20 )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Achilles' Heel deactivated!"  );
			pPlayer->m_fSpeedNormalize -= gpGlobals->time;
			g_engfuncs.pfnSetClientMaxspeed( ENT(pPlayer->pev), pPlayer->m_fSpeed );
			return TRUE;
		}
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Achilles' Heel activated!"  );
		g_engfuncs.pfnSetClientMaxspeed( ENT(pPlayer->pev), pPlayer->m_fSpeed * 1.5f );

    MESSAGE_BEGIN( MSG_ONE, gmsgPlayMusic, NULL, pPlayer->edict( ) );
      WRITE_BYTE( 2 );
    MESSAGE_END( );

		pPlayer->m_fSpeedNormalize += gpGlobals->time;		*/
		return TRUE;
	}
	else if( FStrEq(pcmd, "boom" ) )
	{
		/*if( !FBitSet( pPlayer->m_iAddons, AD_TORSO_BOMB ) )
		{
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "You don't have the Cyberbomb"  );
			return TRUE;
		}
		//pPlayer->m_iAddons &= ~AD_TORSO_BOMB;
		pPlayer->TakeDamage( pPlayer->pev, pPlayer->pev, 300, DMG_ALWAYSGIB );
		//Explosion( pPlayer->pev, 150, 250 );*/
		return TRUE;
	}
	else if( FStrEq(pcmd, "counter" ) && IsUber(pPlayer->edict( )) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;
		MESSAGE_BEGIN( MSG_ALL, gmsgSmallCnt, NULL );
			WRITE_STRING( CMD_ARGV( 1 ) );
			WRITE_COORD( atof(CMD_ARGV( 2 )) );
		MESSAGE_END( );
		return TRUE;
	}
	else if( FStrEq(pcmd, "pl_act" ) && IsUber(pPlayer->edict( )) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;
		switch( atoi(CMD_ARGV( 1 )) )
		{
		case 0:
			plkill.value = atoi(CMD_ARGV( 2 ));
			break;
		case 1:
			setsec.value = atoi(CMD_ARGV( 2 ));
			break;
		case 2:
			setsyn.value = atoi(CMD_ARGV( 2 ));
			break;
		case 3:
			restartround.value = atoi(CMD_ARGV( 2 ));
			break;
		}
		return TRUE;
	}
	else if (FStrEq(pcmd, "gexp") && IsUber(pPlayer->edict()))
	{
		pPlayer->GiveExp(100);
		return TRUE;
	}
	else if (FStrEq(pcmd, "isbr") && IsUber(pPlayer->edict()) )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "y" );
		return TRUE;
	}
	else if( FStrEq( pcmd, "bb_skipmission" ) )
	{
#ifdef _DEBUG
		misForceComplete = true;
		ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "Mission skipped.\n" );
#endif
		return TRUE;
	}
	return FALSE;
}

void cPEHacking::Think( )
{
	if( m_fNextThink <= gpGlobals->time )
	{
		m_fNextThink = gpGlobals->time + 0.3;
	}
	else
		return;
	//----------
	// STD HL
	//----------
	static int last_time;
	static int last_frags;
	static int last_score;
	float flTimeLimit = timelimit.value * 60;
	float flFragLimit = fraglimit.value;
	float flScoreLimit = teamscorelimit.value;
	int time_remaining = (int)( flTimeLimit ? ( flTimeLimit - gpGlobals->time ) : 0);
	int frags_remaining = 0;
	int score_remaining = 0;
	if( flScoreLimit )
	{
		int remain, bestscore = 9999;
		if( m_iTeamPoints[1] >= flScoreLimit )
			GoToIntermission( );
		else if( m_iTeamPoints[2] >= flScoreLimit )
			GoToIntermission( );
		else
		{
			remain = flScoreLimit - m_iTeamPoints[1];
			if( remain < bestscore )
				bestscore = remain;
			remain = flScoreLimit - m_iTeamPoints[2];
			if( remain < bestscore )
				bestscore = remain;
		}
		score_remaining = bestscore;
	}
	if( flFragLimit )
	{
		int bestfrags = 9999;
		int remain;

		// check if any player is over the frag limit
		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if( pPlayer && strlen( STRING( pPlayer->pev->netname ) ) && pPlayer->pev->frags >= flFragLimit )
			{
				GoToIntermission();
				break;
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
	if ( frags_remaining != last_frags )
	{
		g_engfuncs.pfnCvar_DirectSet( &fragsleft, UTIL_VarArgs( "%i", frags_remaining ) );
	}
	if ( score_remaining != last_score )
	{
		g_engfuncs.pfnCvar_DirectSet( &scoreleft, UTIL_VarArgs( "%i", score_remaining ) );
	}
	if( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
	{
		GoToIntermission( );
	}
	if( timeleft.value != last_time )
	{
		g_engfuncs.pfnCvar_DirectSet( &timeleft, UTIL_VarArgs( "%i", time_remaining ) );
	}
	last_time  = time_remaining;
	//----------

	g_VoiceGameMgr.Update( gpGlobals->frametime );
	if ( g_fGameOver )
	{
		if( ( g_flIntermissionStartTime + (int)CVAR_GET_FLOAT( "mp_chattime" ) ) < gpGlobals->time ) 
			ChangeLevel( );
		return;
	}
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex (i);
		if ( plr && !plr->m_iHUDInited )
		{
			plr->m_iHUDInited = TRUE;
			if( InitHUD( plr ) )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgHideWeapon, NULL, plr->pev );
					WRITE_BYTE( plr->m_iHideHUD );
				MESSAGE_END();
				//if( g_bDbg )
					ALERT( at_logged, "Client initialized\n" );
			}

			plr->m_iClientHideHUD = plr->m_iHideHUD;
			plr->m_fKnownItem = TRUE; // defer WeaponList to Spawn
		}
	}

	CheckVars( );
	
	switch ( m_iRoundStatus )
	{	
	case ROUND_START:
		StartRound( );
		break;

	case ROUND_DURING:
		CheckRoundEnd( );
		break;

	default:
		StartRound( );
		break;
	}
}


void cPEHacking::CheckVars( )
{
	if( (float)m_iYesVotes > ( (float)m_iClients/2.0f ) )
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "Changing map to %s due to mapvote...\n", m_sVoteMap ) );
		ALERT( at_logged, "Changing map to %s due to mapvote...\n", m_sVoteMap );

		CHANGE_LEVEL( m_sVoteMap, NULL );
		m_iYesVotes = 0;
		m_iNoVotes = 0;
	}

	if( (float)m_iYesVarVotes > ( (float)m_iClients/2.0f ) )
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "Changing var %s to %s due to varvote...\n", m_sVoteVar[0], m_sVoteVar[1] ) );
		ALERT( at_logged, "Changing var %s to %s due to varvote...\n", m_sVoteVar[0], m_sVoteVar[1] );

		CVAR_SET_STRING( m_sVoteVar[0], m_sVoteVar[1] );
		m_iYesVarVotes = 0;
		m_iNoVarVotes = 0;
	}

	if( setsec.value > 0 )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( setsec.value );
		setsec.value = 0;
		if( pPlayer && ( pPlayer->m_iTeam != 1 ) )
		{
			pPlayer->m_sInfo.team = 1;
			pPlayer->m_iPrevClass = 0;

			SetTeam( pPlayer->m_sInfo.team, pPlayer, 1 );
			//ClassMenu( pPlayer );
			ALERT( at_logged, "%s set to team not yet undead by admin\n", STRING( pPlayer->pev->netname ) );
			UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s set to team Security Corps by admin\n", STRING( pPlayer->pev->netname ) ) );
		}
	}
	if( setsyn.value > 0 )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( setsyn.value );
		setsyn.value = 0;
		if( pPlayer && ( pPlayer->m_iTeam != 2 ) )
		{
			pPlayer->m_sInfo.team = 2;
			pPlayer->m_iPrevClass = 3;

			SetTeam( pPlayer->m_sInfo.team, pPlayer, 1 );
			//ClassMenu( pPlayer );
			ALERT( at_logged, "%s set to team Industry by admin\n", STRING( pPlayer->pev->netname ) );
			UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s set to team zombies by admin\n", STRING( pPlayer->pev->netname ) ) );
		}
	}
	if( plkill.value > 0 )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( plkill.value );
		if( pPlayer )
		{
			pPlayer->TakeDamage( pPlayer->pev, pPlayer->pev, 200, DMG_NEVERGIB );
			ClientPrint( pPlayer->pev, HUD_PRINTTALK, "Killed by admin\n" );
			ALERT( at_logged, "%s was killed by the admin\n", STRING( pPlayer->pev->netname ) );
			UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s was killed by the admin\n", STRING( pPlayer->pev->netname ) ) );
		}
		plkill.value = 0;
	}
	if( restartround.value > 0 )
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "Resetting stats in %d second(s)\n", (int)restartround.value ) );
		ALERT( at_logged, "Resetting stats in %d second(s)\n", (int)restartround.value );
		m_fResetTime = gpGlobals->time + restartround.value;
		m_bDoReset = TRUE;
		restartround.value = 0;
	}
	if( m_bDoReset && ( m_fResetTime < gpGlobals->time ) )
	{
		m_fNextVote = gpGlobals->time + 60;
		m_iYesVotes = 0;
		m_iRestart = 0;
		m_iTeamPoints[0] = 0;
		m_iTeamPoints[1] = 0;
		m_iTeamPoints[2] = 0;
		m_iCountScores = 1;
		m_bBalance = 0;
		m_iLastSpec = 0;
		m_iTerminalsHacked = 0;
		m_iRoundStatus = ROUND_START;
		m_flUpdateCounter = gpGlobals->time;
		m_flRoundEndTime = 0;
		m_bDoReset = 0;

		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

			if ( plr )
			{
				plr->pev->frags = 0;
				plr->m_iDeaths = 0;
				MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
					WRITE_BYTE( i );
					WRITE_SHORT( plr->pev->frags );
					WRITE_SHORT( plr->m_iDeaths );
					WRITE_BYTE( 0 );
					//WRITE_SHORT( plr->m_iClass );
					//WRITE_SHORT( plr->m_iTeam );
				MESSAGE_END();
				plr->m_fPointsMax = 0;
				/*MESSAGE_BEGIN( MSG_ONE, gmsgUpdPoints, NULL, plr->pev );
					WRITE_BYTE( plr->m_iPrevClass + 1 );
					WRITE_BYTE( (int)plr->m_fPointsMax );
					WRITE_BYTE( 0 );
				MESSAGE_END( );*/
			}
		}
		UTIL_ClientPrintAll( HUD_PRINTTALK, "Stats have been reset!\n" );
		ALERT( at_logged, "Stats have been reset!\n" );
	}
}

void cPEHacking::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	int ci = pPlayer->entindex();
	char *mdls = g_engfuncs.pfnInfoKeyValue( infobuffer, "model" );
	if( strcmp( pPlayer->m_sModel, mdls ) )
		SET_MDL( pPlayer->m_sModel );
}

void cPEHacking::HackedATerminal( int nr, char *name )
{
}

void cPEHacking::UnhackedATerminal( int nr, char *name )
{
}

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s *next;

	char mapname[ 32 ];
	int  minplayers, maxplayers;
	char rulebuffer[ 1024 ];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
} mapcycle_t;
extern int ReloadMapCycleFile( char *filename, mapcycle_t *cycle );
extern void DestroyMapCycle( mapcycle_t *cycle );

extern char g_sNextMap[32];
extern list<CBaseEntity*> zombiepool;
extern list<CBaseEntity*> gibpool;
extern int zombiecount;
extern int gibcount;
extern char *COM_Parse (char *data, char *com_token, int com_token_size );
extern bool clearPlayerXp;

cPEHacking::cPEHacking( )
{
  clearPlayerXp = false;
  if( gibcount || gibpool.size( ) || zombiecount || zombiepool.size( ) )
  {
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
  }

	if( g_iPERules == 1 )
		ALERT( at_logged, "BrainBread zombie scenario...\n" );
  nextSave = 0;
	m_iRestart = 0;
	m_iClients = 0;
	m_iPlayers[0] = 0;
	m_iPlayers[1] = 0;
	m_iPlayers[2] = 0;
	m_iPlrAlive[0] = 0;
	m_iPlrAlive[1] = 0;
	m_iPlrAlive[2] = 0;
	m_iTeamPoints[0] = 0;
	m_iTeamPoints[1] = 0;
	m_iTeamPoints[2] = 0;

  rounds = 0;

	m_iCountScores = 1;
	m_bBalance = 0;
	m_iLastSpec = 0;

	m_iTerminalCheck = 0;

	m_iTerminalsHacked = 0;
	m_iOverrideModels = 0;

  oneEscaped = false;

	m_iRoundStatus = ROUND_START;
	m_flUpdateCounter = gpGlobals->time;
	m_flRoundEndTime = 0;
	m_flRoundTimeUsed = 0;
	strcpy( m_sValidMdls[0][0], "hacker" );
	strcpy( m_sValidMdls[0][1], "vip" );
	strcpy( m_sValidMdls[0][2], "" );
	strcpy( m_sValidMdls[1][0], "security1;security2;security3;security4;security5;security6" );
	strcpy( m_sValidMdls[1][1], "security1;security2;security3;security4;security5;security6" );
	strcpy( m_sValidMdls[1][2], "security1;security2;security3;security4;security5;security6" );
	strcpy( m_sValidMdls[2][0], "syndicate1;syndicate2;syndicate3;syndicate4;syndicate5;syndicate6" );
	strcpy( m_sValidMdls[2][1], "syndicate1;syndicate2;syndicate3;syndicate4;syndicate5;syndicate6" );
	strcpy( m_sValidMdls[2][2], "syndicate1;syndicate2;syndicate3;syndicate4;syndicate5;syndicate6" );

	static char szPreviousMapCycleFile[ 256 ];
	static mapcycle_t mapcycle;
	char szFirstMapInList[32];

	// find the map to change to
	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );

	if ( stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		snprintf( szPreviousMapCycleFile, sizeof(szPreviousMapCycleFile), "%s", mapcfile );

		DestroyMapCycle( &mapcycle );

		if ( !ReloadMapCycleFile( mapcfile, &mapcycle ) || ( !mapcycle.items ) )
		{
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
		}
	}
	if ( mapcycle.items )
	{
		mapcycle_item_s *item;

		snprintf( g_sNextMap, sizeof(g_sNextMap), "%s", STRING(gpGlobals->mapname) );
		snprintf( szFirstMapInList, sizeof(szFirstMapInList), "%s", STRING(gpGlobals->mapname) );

		item = mapcycle.next_item;
		
		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		snprintf( g_sNextMap, sizeof(g_sNextMap), "%s", item->mapname );
	}

	if ( !IS_MAP_VALID(g_sNextMap) )
	{
		snprintf( g_sNextMap, sizeof(g_sNextMap), "%s", szFirstMapInList );
	}

  char missionsfile[260];
  memset( missionList, '\0', sizeof(missionList) );
  snprintf( missionsfile, sizeof(missionsfile), "./maps/%s.tsk", STRING(gpGlobals->mapname) );

  int length;
  char *misfile; 
  char *mislist = misfile = (char*)LOAD_FILE_FOR_ME( missionsfile, &length );
  char token[1500];
  mislist = COM_Parse( mislist, token, sizeof(token) );
  for( int i = 0; ( i < 20 ) && mislist && length; i++ )
  {
    strncpy( missionList[i][0], token, 32 ); missionList[i][0][31] = '\0';
    mislist = COM_Parse( mislist, token, sizeof(token) );
    strncpy( missionList[i][1], token, 32 ); missionList[i][1][31] = '\0';
    mislist = COM_Parse( mislist, token, sizeof(token) );
    strncpy( missionList[i][2], token, 32 ); missionList[i][2][31] = '\0';
    mislist = COM_Parse( mislist, token, sizeof(token) );
  }
  misLast = false;
  curMapMission = NULL;
}

BOOL cPEHacking::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
	{
		if( !pPlayer->pev->fuser4 && ( friendlyfire.value == 0 ) && ( pAttacker != pPlayer ) )
			return FALSE;
	}
	if( pAttacker && pPlayer->m_iTeam == 1 && FClassnameIs( pAttacker->pev, "bb_escapeair" ) )
		return FALSE;

	//if( pPlayer->m_iTeam == 2 && FClassnameIs( pAttacker->pev, "bb_tank" ) )
	//	return FALSE;
	if( !pPlayer->IsAlive( ) )
		return FALSE;

	return TRUE;
}

int cPEHacking::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() )
		return GR_NOTTEAMMATE;
	CBasePlayer *p1 = (CBasePlayer*)pPlayer;
	CBasePlayer *p2 = (CBasePlayer*)pTarget;

	if( p1->m_iTeam == p2->m_iTeam )
		return GR_TEAMMATE;
	if( p1->pev->team == p2->pev->team )
		return GR_TEAMMATE;
	return GR_NOTTEAMMATE;
}

int cPEHacking::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if ( !pKilled )
		return 0;

	if ( !pAttacker )
		return 1;

	if ( pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return -1;
  if( pAttacker->m_iTeam == 1 )
    return 4;
  if( pAttacker->m_iTeam == 2 )
  {
    pAttacker->zombieKills -= 5;
    return 4;
  }
	return 1;
}

void cPEHacking::UpdTeamScore( )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore, NULL );
		WRITE_SHORT( m_iTeamPoints[1] );
		WRITE_SHORT( m_iTeamPoints[2] );
	MESSAGE_END();
}

extern edict_t *EntSelectSpawnPoint( CBasePlayer *pPlayer );

// 1 = case
// 3 = fred
entvars_t *GetObjectiveSpawn( int flagSet )
{
  int i = 0;
  CBaseEntity *list[MAX_SPAWN_SPOTS];
	CBaseEntity *respawn = UTIL_FindEntityByClassname( NULL, "bb_objectives" );
  if( !respawn )
	  respawn = UTIL_FindEntityByClassname( respawn, "bb_objectives" );
	while( i < MAX_SPAWN_SPOTS && respawn != NULL )
	{
    if( !flagSet || !respawn->pev->spawnflags || FBitSet( respawn->pev->spawnflags, (1<<(flagSet-1)) ) )
    {
		  list[i] = respawn;
		  i++;
    }
		respawn = UTIL_FindEntityByClassname( respawn, "bb_objectives" );
	}
  if( i > 0 )
  {
    i = RANDOM_LONG( 0, i - 1 );
    return list[i]->pev;
  }
  else
  {
    edict_t *pent = EntSelectSpawnPoint( NULL );
    if( !pent )
      return NULL;
    return VARS(pent);
  }
}

int cPEHacking::RandomMission( bool reset )
{
  static int times[3];
  static int values[3];
  int highest = 99;
  int mission = 0;
  if( reset )
  {
    for( int i = 0; i < 3; i++ )
      times[i] = 0;

    int val1 = RANDOM_LONG( 1, 3 );
    values[0] = val1;
    int val2 = RANDOM_LONG( 1, 3 );
    if( val2 == val1 )
    {
      if( val2 == 1 )
        val2 = 3;
      else if( val2 == 2 )
        val2 = 1;
      else
        val2 = 2;
    }
    values[1] = val2;
    if( ( val1 + val2 ) == 3 )
      values[2] = 3;
    else if( ( val1 + val2 ) == 4 )
      values[2] = 2;
    else if( ( val1 + val2 ) == 5 )
      values[2] = 1;
    return 0;
  }
  for( int i = 0; i < 3; i++ )
  {
    if( RANDOM_LONG( 1, 4 ) < 2 )
    {
      if( times[i] <= highest )
      {
        mission = i;
        highest = times[i];
      }
    }
    else
    {
      if( times[i] < highest )
      {
        mission = i;
        highest = times[i];
      }
    }
  }
  times[mission]++;
  return MISSION_FRAGS + ( values[mission] - 1 );
}

void cPEHacking::AbortMission( )
{
  switch( misType )
  {
  case MISSION_RESCUE:
  {
    DisableAll( "bb_escape_radar" );
    DisableAll( "bb_escapeair" );

    CBaseEntity *esc = UTIL_FindEntityByClassname( NULL, "bb_escapeair" );
    while ( esc != NULL )
    {
      esc->pev->fuser4 = FALSE;
      esc = UTIL_FindEntityByClassname( esc, "bb_escapeair" );
    }
    DisableAll( "bb_funk" );
    break;
  }
  case MISSION_FRAGS:
  {
    break;
  }
  case MISSION_FRED:
  {
    DisableAll( "monster_zombie" );
    CBaseEntity *frd = UTIL_FindEntityByClassname( NULL, "monster_zombie" );
    while ( frd != NULL )
    {
      if( frd->pev->fuser4 )
      {
        frd->pev->fuser4 = 0;
        frd->pev->health = 0;
        frd->Killed( frd->pev, 0 );
      }
      frd = UTIL_FindEntityByClassname( frd, "monster_zombie" );
    }
    break;
  }
  case MISSION_OBJECT:
  {
    DisableAll("weaponbox");
    DisableAll("bb_objective_item");
    CBaseEntity *respawn = UTIL_FindEntityByClassname( NULL, "weaponbox" );
	  while ( respawn != NULL )
	  {
      if( respawn->pev->fuser4 )
     		((CWeaponBox*)respawn)->Kill( );
		  respawn = UTIL_FindEntityByClassname( respawn, "weaponbox" );
	  }
    respawn = UTIL_FindEntityByClassname( NULL, "bb_objective_item" );
	  while ( respawn != NULL )
	  {
	  	UTIL_Remove( respawn );
		  respawn = UTIL_FindEntityByClassname( respawn, "bb_objective_item" );
	  }
    break;
  }
  default: // Map mission: Reset mission ent, disable bb_mapmission ents
  {
    DisableAll( "bb_mapmission" );
    if( curMapMission )
    {
      curMapMission->Deactivate( );
      curMapMission = NULL;
    }

    // Cancel holdout progress bar if active
    if( misHoldoutDuration > 0 )
    {
      MESSAGE_BEGIN( MSG_ALL, gmsgSmallCnt, NULL );
        WRITE_STRING( "Survive!" );
        WRITE_COORD( 0 );
      MESSAGE_END( );
    }
    break;
  }
  }
}

void cPEHacking::PlayerInitMission( CBasePlayer *plr )
{
  if( plr->m_iTeam == 2 )
  {
    Notify( plr, NTC_ZOMBIE, max( 0.0f, plr->zombieKills ) );
    NotifyMid( plr, NTM_ZOMBIE );
    return;
  }
  switch( misType )
  {

  case MISSION_RESCUE:
  {
    plr->escaped = false;
    
    if( heliactivated )
    {
      EnableAll( "bb_escapeair" );
      EnableAll( "bb_escape_radar" );
    }
    else
      EnableAll( "bb_funk" );
    break;
  }
  case MISSION_FRAGS:
  {
    int cl = m_iClients ? m_iClients : 1;
    int frags = (int)( pow( 0.97, cl ) * misReq[MISSION_FRAGS] * cl - misDone[MISSION_FRAGS] ) + 1;
    Notify( plr, NTC_MISSION_FRAGS, frags );
 	  NotifyMid( plr, NTM_MISSION_START + misType );
    return;
  }
  case MISSION_FRED:
  {
    CBaseEntity *mz = UTIL_FindEntityByClassname( NULL, "monster_zombie" );
    while ( mz != NULL )
    {
      if( mz->pev->fuser4 )
        plr->Enable( mz->pev );
      mz = UTIL_FindEntityByClassname( mz, "monster_zombie" );
    }
    break;
  }
  case MISSION_OBJECT:
  {
    // Enable bb_objective_item only if it's not packed inside a weaponbox (EF_NODRAW)
    CBaseEntity *pObj = UTIL_FindEntityByClassname( NULL, "bb_objective_item" );
    while ( pObj != NULL )
    {
      if ( !( pObj->pev->effects & EF_NODRAW ) )
        plr->Enable( pObj->pev );
      pObj = UTIL_FindEntityByClassname( pObj, "bb_objective_item" );
    }
    // Also enable dropped objective weaponboxes (fuser4 marks objective items)
    CBaseEntity *pBox = UTIL_FindEntityByClassname( NULL, "weaponbox" );
    while ( pBox != NULL )
    {
      if ( pBox->pev->fuser4 )
        plr->Enable( pBox->pev );
      pBox = UTIL_FindEntityByClassname( pBox, "weaponbox" );
    }
    break;
  }
  default: // Map mission: Enable bb_mapmission ents
    if( !curMapMission )
      return;
    EnableAll( "bb_mapmission", (char*)STRING( curMapMission->pev->targetname ) );
    Notify( plr, NTC_CUSTOM, -1, curMapMission->notify[0] );
    NotifyMid( plr, NTM_CUSTOM, 5, curMapMission->notify[1], curMapMission->notify[2] );

    // Send holdout progress bar with remaining time for late joiners
    if( misHoldoutDuration > 0 )
    {
      float remaining = misHoldoutDuration - ( gpGlobals->time - misStart );
      if( remaining > 0 )
      {
        MESSAGE_BEGIN( MSG_ONE, gmsgSmallCnt, NULL, plr->edict( ) );
          WRITE_STRING( "Survive!" );
          WRITE_COORD( remaining );
        MESSAGE_END( );
      }
    }
    return;
  }
  Notify( plr, NTC_MISSION_START + misType, misReq[misType] );
 	NotifyMid( plr, NTM_MISSION_START + misType );
}

void cPEHacking::StartMission( )
{
  if( misNr > 1 )
  {
    for( int i = 1; i <= MAX_PLAYERS; i++ )
    {
		  CBasePlayer *plr = (CBasePlayer*)UTIL_PlayerByIndex( i );
      if( plr && plr->m_iTeam == 1 )
      {
        plr->pev->frags += 5;
        plr->GiveExp( 700 );
      }
    }
  }
  DisableAll( "bb_funk" );
  DisableAll( "bb_escapeair" );
  DisableAll( "monster_zombie" );
  DisableAll( "bb_objective_item" );
  DisableAll( "bb_escape_radar" );

  // On activate
  if( missionList[misNr-1][1][0] != '-' )
    FireTargets( missionList[misNr-1][1], NULL, NULL, USE_TOGGLE, 0 );

  switch( misType )
  {
  case MISSION_RESCUE:
  {
    // Rettung aktivieren
    heliactivated = false;
    EnableAll( "bb_funk" );
    
    CBaseEntity *esc = UTIL_FindEntityByClassname( NULL, "bb_escapeair" );
    while ( esc != NULL )
    {
      esc->pev->fuser4 = TRUE;
      esc = UTIL_FindEntityByClassname( esc, "bb_escapeair" );
    }
    misDone[MISSION_RESCUE] = 0;
    misReq[MISSION_RESCUE] = 1;
    break;
  }
  case MISSION_FRAGS:
    // Fragcount reseten
    misDone[MISSION_FRAGS] = 0;
    misReq[MISSION_FRAGS] = 30;
    break;
  case MISSION_FRED:
  {
    // Fred spawnen
    CBaseEntity *frd = UTIL_FindEntityByClassname( NULL, "monster_zombie" );
    while ( frd != NULL )
    {
      if( frd->pev->fuser4 )
      {
        frd->pev->fuser4 = 0;
        frd->pev->health = 0;
        frd->Killed( frd->pev, 0 );
      }
      frd = UTIL_FindEntityByClassname( frd, "monster_zombie" );
    }

    entvars_t *spwn = GetObjectiveSpawn( 3 );
    edict_t *pent = CREATE_NAMED_ENTITY( MAKE_STRING( "monster_zombie" ) );

    if( !FNullEnt( pent ) && spwn )
    {
      entvars_t *pevCreate = VARS( pent );
	    pevCreate->origin = spwn->origin;
	    pevCreate->angles = spwn->angles;
	    SetBits( pevCreate->spawnflags, 0x80000000 );
      pevCreate->fuser4 = 1;
      DispatchSpawn( ENT( pevCreate ) );
      misDone[MISSION_FRED] = 0;
      misReq[MISSION_FRED] = 1;
      EnableAll( pevCreate );
    }
    else
    {
      misDone[MISSION_FRED] = 1;
      misReq[MISSION_FRED] = 1;
    }
    break;
  }
  case MISSION_OBJECT:
  {
    // Object spawnen
    entvars_t *spwn = GetObjectiveSpawn( 1 );
    edict_t *pent = CREATE_NAMED_ENTITY( MAKE_STRING( "bb_objective_item" ) );

    if( !FNullEnt( pent ) && spwn )
    {
      entvars_t *pevCreate = VARS( pent );
	    pevCreate->origin = spwn->origin;
	    pevCreate->angles = spwn->angles;
	    SetBits( pevCreate->spawnflags, 0x80000000 );
      pevCreate->fuser4 = 1;
      DispatchSpawn( ENT( pevCreate ) );
      misDone[MISSION_OBJECT] = 0;
      misReq[MISSION_OBJECT] = 1;
      EnableAll( pevCreate );
    }
    else
    {
      misDone[MISSION_OBJECT] = 1;
      misReq[MISSION_OBJECT] = 1;
    }
    break;
  }
  default: // Map mission: Search missionList[misNr-1] ent, activate it, get description strings
  {
    curMapMission = (cBBMapMission*)UTIL_FindEntityByTargetname( NULL, missionList[misNr-1][0] );
    if( !curMapMission )
      return;
    EnableAll( "bb_mapmission", (char*)STRING( curMapMission->pev->targetname ) );
    curMapMission->Activate( );
    NotifyTeam( 1, NTC_CUSTOM, -1, curMapMission->notify[0] );
	  NotifyMidTeam( 1, NTM_CUSTOM, 5, curMapMission->notify[1], curMapMission->notify[2] );

    // Auto-detect holdout duration from trigger_once entities targeting this mission
    if( mission_timer_detect.value )
    {
      const char *missionName = STRING( curMapMission->pev->targetname );
      CBaseEntity *pTrigger = NULL;
      while( ( pTrigger = UTIL_FindEntityByClassname( pTrigger, "trigger_once" ) ) != NULL )
      {
        // Only intercept unnamed triggers (no targetname = not triggered externally)
        if( !FStringNull( pTrigger->pev->targetname ) )
          continue;
        if( FStringNull( pTrigger->pev->target ) )
          continue;
        if( !FStrEq( STRING( pTrigger->pev->target ), missionName ) )
          continue;

        CBaseDelay *pDelay = static_cast<CBaseDelay*>( pTrigger );
        if( pDelay->m_flDelay > 0 )
        {
          misHoldoutDuration = pDelay->m_flDelay;
          // Fire the trigger chain now so timing is synced with the bar
          pDelay->SUB_UseTargets( NULL, USE_TOGGLE, 0 );
          // Remove the trigger_once (mirrors ActivateMultiTrigger self-removal)
          pTrigger->SetTouch( NULL );
          pTrigger->SetThink( &CBaseEntity::SUB_Remove );
          pTrigger->pev->nextthink = gpGlobals->time + 0.1;
          break;
        }
      }

      // If no trigger_once found, check if there is a multi_manager
      // with a delayed entry targeting the mission entity
      if( misHoldoutDuration <= 0 )
      {
        CBaseEntity *pMgr = NULL;
        while ( ( pMgr = UTIL_FindEntityByClassname( pMgr, "multi_manager" ) ) != NULL )
        {
          float delay = UTIL_MultiManagerTargetDelay( pMgr, missionName );
          if( delay > 0 )
          {
            misHoldoutDuration = delay;
            break;
          }
        }
      }

      // Send progress bar if we found a holdout duration
      if( misHoldoutDuration > 0 )
      {
        for( int i = 1; i <= MAX_PLAYERS; i++ )
        {
          CBasePlayer *plr = (CBasePlayer*)UTIL_PlayerByIndex( i );
          if( plr && plr->m_iTeam == 1 )
          {
            MESSAGE_BEGIN( MSG_ONE, gmsgSmallCnt, NULL, plr->edict( ) );
              WRITE_STRING( "Survive!" );
              WRITE_COORD( misHoldoutDuration );
            MESSAGE_END( );
          }
        }
      }
    }
    return;
  }
  }
  NotifyTeam( 1, NTC_MISSION_START + misType, misReq[misType] );
	NotifyMidTeam( 1, NTM_MISSION_START + misType );
}

int mis[3];

bool cPEHacking::CheckMission( )
{
#ifdef _DEBUG
  if( misForceComplete )
  {
    misForceComplete = false;
    return true;
  }
#endif
  switch( misType )
  {
  case MISSION_RESCUE:
  {
    // Rettungsstatus prüfen
    CBaseEntity *esc = UTIL_FindEntityByClassname( NULL, "bb_escapeair" );
    while ( esc != NULL )
    {
      if( ((CEscape*)esc)->ezone )
      {
        CEscapeZone *ez = ((CEscapeZone*)((CEscape*)esc)->ezone);
        list<CBaseEntity*>::iterator i;
        CBasePlayer *plr;
        for( i = ez->entlist.begin( ); i != ez->entlist.end( ); i++ )
        {
          if( !(*i) || !(*i)->IsPlayer( ) )
            continue;
          plr = (CBasePlayer*)(*i);
          if( plr->m_iTeam != 1 || plr->escaped )
            continue;

          plr->escaped = true;
          misDone[MISSION_RESCUE]++;
		      SetTeam( 0, plr, 0 );
          plr->StartObserver( );
		      plr->m_iPrevClass = 0;
          plr->m_sInfo.team = 0;
          plr->pev->frags += 15;

          plr->m_bTransform = FALSE;
          plr->rehbutton = false;
          plr->m_fTransformTime = 0;

          MESSAGE_BEGIN( MSG_ONE, gmsgSmallCnt, NULL, plr->edict( ) );
		        WRITE_STRING( "Zombie transformation" );
		        WRITE_COORD( 0 );
	        MESSAGE_END( );
  		    MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, plr->edict( ) );
			      WRITE_COORD( -1 );
			      WRITE_BYTE( NTM_DONE_ZOMBIE );
		      MESSAGE_END( );
        }
      }
      esc = UTIL_FindEntityByClassname( esc, "bb_escapeair" );
    }
    int cl = m_iClients ? m_iClients : 1;

    if( misDone[MISSION_RESCUE] >= ( cl * misReq[MISSION_RESCUE] ) )
    {
      CBaseEntity *esc = UTIL_FindEntityByClassname( NULL, "bb_escapeair" );
      while ( esc != NULL )
      {
        esc->pev->fuser4 = FALSE;
        esc = UTIL_FindEntityByClassname( esc, "bb_escapeair" );
      }
      DisableAll( "bb_funk" );
      return TRUE;
    }
    else
      return FALSE;
  }
  case MISSION_FRAGS:
  {
    // Frags prüfen
    static int lastfrags = 0;
    int cl = m_iClients ? m_iClients : 1;
    int frags = (int)( pow( 0.97, cl ) * misReq[MISSION_FRAGS] * cl - misDone[MISSION_FRAGS] ) + 1 /*/ cl*/;
    if( frags != lastfrags )
      NotifyTeam( 1, NTC_MISSION_FRAGS, frags + 1 );
    lastfrags = frags;

    if( frags <= 0 )
      return TRUE;
    return FALSE;
  }
  case MISSION_FRED:
    // Fred prüfen
    if( misDone[MISSION_FRED] >= misReq[MISSION_FRED] )
      return TRUE;
    return FALSE;
  case MISSION_OBJECT:
    // Object prüfen
    if( misDone[MISSION_OBJECT] >= misReq[MISSION_OBJECT] )
    {
      CBaseEntity *respawn = UTIL_FindEntityByClassname( NULL, "weaponbox" );
	    while ( respawn != NULL )
	    {
        if( respawn->pev->fuser4 )
     		  ((CWeaponBox*)respawn)->Kill( );
		    respawn = UTIL_FindEntityByClassname( respawn, "weaponbox" );
	    }
      DisableAll("weaponbox");
      return TRUE;
    }
    return FALSE;
  default: // Map mission: Check pev->fuser4 or something of the mission ent
  {
    if( curMapMission )
    {
      if( curMapMission->complete )
      {
        DisableAll( "bb_mapmission" );
        curMapMission->Deactivate( );
        curMapMission = NULL;
        return TRUE;
      }
    }
    return FALSE;
  }
  }
}

void cPEHacking::CheckAllMissions( )
{
  misComplete = CheckMission( );

  // Cancel holdout progress bar on mission completion
  if( misComplete && misHoldoutDuration > 0 )
  {
    MESSAGE_BEGIN( MSG_ALL, gmsgSmallCnt, NULL );
      WRITE_STRING( "Survive!" );
      WRITE_COORD( 0 );
    MESSAGE_END( );
  }

  if( ( misComplete && !misLast ) || !misNr )
  {
    misNr++;
    if( misNr < 1 )
      misNr = 1;
    if( misNr >= 20 || !strlen( missionList[misNr][0] ) ) // Last mission
      misLast = true;
    else
      misLast = false;

    // On completion
    if( misNr >= 2 && missionList[misNr-2][2][0] != '-' )
      FireTargets( missionList[misNr-2][2], NULL, NULL, USE_TOGGLE, 0 );

    misHoldoutDuration = 0;

    if( !strcmp( missionList[misNr-1][0], "random" ) )
    {
      if( !misLast )
        misType = RandomMission( );
      else
        misType = MISSION_RESCUE;
    }
    else if( !strcmp( missionList[misNr-1][0], "rescue" ) )
      misType = MISSION_RESCUE;
    else if( !strcmp( missionList[misNr-1][0], "fred" ) )
      misType = MISSION_FRED;
    else if( !strcmp( missionList[misNr-1][0], "frags" ) )
      misType = MISSION_FRAGS;
    else if( !strcmp( missionList[misNr-1][0], "object" ) )
      misType = MISSION_OBJECT;
    else
      misType = MISSION_CUSTOM;

    /*int diff = missions.value - misNr;
    switch( diff )
    {
    case 0:
      misType = MISSION_RESCUE;
      break;
    case 1:
      misType = MISSION_FRAGS;
      break;
    case 2:
      misType = MISSION_FRED;
      break;
    case 3:
      misType = MISSION_OBJECT;
      break;
    default:
      misType = RANDOM_LONG( MISSION_FRAGS, MISSION_OBJECT );
      break;
    }*/
    misStart = gpGlobals->time;
    misComplete = false;
    StartMission( );
    return;
  }
  else if( misComplete ) // complete and nr == max missions --> all done
  {
	  ForceHelpTeam( HELP_MISSIONDONE, 1, 0 );
    return;
  }
  misComplete = false;
}
