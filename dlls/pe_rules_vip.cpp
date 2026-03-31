// ------------ Public - Enemy Gamerules -- Szenario: DIRTY BUSINESS --------------------
// -------- by Spinator
// ---- pe_rules_vip.cpp
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"pe_menus.h"
#include	"pe_rules.h"
#include	"pe_rules_hacker.h"
#include	"pe_rules_vip.h"
#include	"game.h"
#include	"shake.h"
#include	"pe_utils.h"
#include	"pe_notify.h"
#include	"voice_gamemgr.h"

#define ROUND_TIME (roundtime.value)
#define ROUND_DELAY (rounddelay.value)
/*#define ROUND_TIME 0.1
#define ROUND_DELAY 0.1*/

extern float g_fSatSyn;
extern float g_fSatSec;

extern CVoiceGameMgr	g_VoiceGameMgr;
extern DLL_GLOBAL BOOL		g_fGameOver;
extern void UTIL_EdictScreenFade( edict_s *edict, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags );
extern cvar_t timeleft, fragsleft, scoreleft;
extern float g_flIntermissionStartTime;

extern int gmsgSpecial;
extern int gmsgTeamInfo;
extern int gmsgIntro;
extern int gmsgCounter;
extern int gmsgMates;
extern int gmsgInitHUD;
extern int gmsgHideWeapon;
extern int gmsgClass;
extern int gmsgScoreInfo;
extern int gmsgSpectator;

#define NO_TEAM 0
#define TEAM1	1
#define TEAM2	2

#define ROUND_DURING	0
#define ROUND_START		1

void cPEVip::CheckRoundEnd( )
{
	if( UpdateCounter( ) ) // alles, das im abstand von 1 sec ausgeführt werden soll, hier hin
	{
		CalcTeamBonus( );
		Recount( );
		if( gpGlobals->time < ( m_flRoundEndTime - ( ROUND_TIME - 15 ) ) )
		{
			for( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
			
				if (!pPlayer)
					continue;

				EMIT_SOUND(ENT(pPlayer->pev), CHAN_STATIC, "common/null.wav", 1, ATTN_NORM);
			
				if( ( pPlayer->m_iTeam != 1 ) && ( pPlayer->m_iTeam != 2 ) )
					continue;

				if( pPlayer->IsAlive( ) || pPlayer->m_bNoAutospawn )
					continue;

				if ( pPlayer->pev->weapons )
					pPlayer->RemoveAllItems( TRUE );
				
				pPlayer->m_bCanRespawn = TRUE;
				pPlayer->pev->body = 0;
				pPlayer->m_bNoAutospawn = 0;

				if( pPlayer->m_iTeam == 2 )
				{
					Notify( pPlayer, NTC_KILL_V );
					NotifyMid( pPlayer, NTM_KILL_V );
					pPlayer->m_bIsSpecial = FALSE;
					ChangeClass( pPlayer, pPlayer->m_iPrevClass );
					MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
						WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
						WRITE_BYTE( 0 );
					MESSAGE_END( );
				}
				else
				{
					pPlayer->m_bIsSpecial = FALSE;
					ChangeClass( pPlayer, pPlayer->m_iPrevClass );
					Notify( pPlayer, NTC_V_TEAM );
					NotifyMid( pPlayer, NTM_V_TEAM );
					MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
						WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
						WRITE_BYTE( 0 );
					MESSAGE_END( );
				}
							
				pPlayer->Spawn( );
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

				if( pPlayer->IsAlive( ) )
					EquipPlayer( pPlayer );
			}
		}
		if( !( (int)gpGlobals->time % 4 ) && !m_bBalance && ( ( m_flRoundEndTime - gpGlobals->time )  <= ( ROUND_TIME - 10 ) ) )
		{
			if( TeamsUneven( ) && autoteam.value )
			{
				NotifyAll( NTM_AUTOBAL );
				m_bBalance = 1;
			}
		}
	}

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

	if( m_iVipEscaped )
	{
		AddScore( 1, 20 );
		RadioAll( "sec_win" );
		ALERT( at_logged, "The Security Corps win: The Case is at the target.\n" );
		NotifyAll( NTM_VESC );
		m_iRestart = 1;
	}

	if( m_flRoundEndTime < gpGlobals->time )
	{
		AddScore( 2, 10 );
		RadioAll( "syn_wins" );
		ALERT( at_logged, "The Syndicate wins: The time is up.\n" );
		NotifyAll( NTM_VTIMEUP );
		m_iRestart = 2;
		MESSAGE_BEGIN( MSG_ALL, gmsgCounter, NULL );
			WRITE_LONG( 0 );
		MESSAGE_END( );
	}

	if( !m_iPlrAlive[2] && m_iPlrAlive[1] && m_iPlayers[2] )
	{
		RadioAll( "sec_win" );
		ALERT( at_logged, "The Security Corps win: The Syndicate has been wiped out.\n" );
		AddScore( 1, 10 );
		NotifyAll( NTM_SYWIPED );
		m_iRestart = 1;
	}
	else if( !m_iPlrAlive[1] && m_iPlrAlive[2] && m_iPlayers[1] )
	{
		AddScore( 2, 20 );
		RadioAll( "syn_wins" );
		ALERT( at_logged, "The Syndicate wins: The Security Corps have been wiped out.\n" );
		NotifyAll( NTM_COWIPED );
		m_iRestart = 2;
	}
	else if( !m_iPlrAlive[0] && ( m_iPlayers[1] + m_iPlayers[2] ) )
	{
		RadioAll( "rnddraw" );
		ALERT( at_logged, "Round Draw: All the players are dead.\n" );
		NotifyAll( NTM_ALLDEAD );
		m_iRestart = 3;
	}

	if( m_iRestart )
	{
		ALERT( at_logged, "New round starting...\n" );
		UpdTeamScore( );
		m_flRoundEndTime = gpGlobals->time + ROUND_DELAY;
		m_iRoundStatus = ROUND_START;
	}

}

void cPEVip::StartRound( )
{
	if( m_flRoundEndTime > gpGlobals->time )
		return;
	Recount( );
	AutoTeam( );

	m_pVip = NULL;
	ResetMap( );
	ALERT( at_logged, "Preparing players...\n" );

	m_iVipEscaped = 0;
	m_iCountScores = 1;
	g_fSatSyn =	0;
	g_fSatSec = 0;

	m_flRoundTimeUsed = ROUND_TIME;
	m_flRoundEndTime = gpGlobals->time + ROUND_TIME;
	m_iRoundStatus = ROUND_DURING;

	if( !m_iPlayers[1] || !m_iPlayers[2] )
	{
		m_iCountScores = 0;
		NotifyAll( NTM_NOSCORE );
	}

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
	
		if (!pPlayer)
			continue;
		
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_STATIC, "common/null.wav", 1, ATTN_NORM);
		EMIT_SOUND(ENT(pPlayer->pev), CHAN_VOICE, "common/null.wav", 1, ATTN_NORM);
		if( ( pPlayer->m_iTeam != pPlayer->m_sInfo.team ) && ( pPlayer->m_iTeam != 0 ) )
			SetTeam( pPlayer->m_sInfo.team, pPlayer, 0 );

		pPlayer->m_bNoAutospawn = 0;
		if ( pPlayer->m_iTeam == 1 || pPlayer->m_iTeam == 2 )
		{
			if ( pPlayer->pev->weapons )
				pPlayer->RemoveAllItems( TRUE );

			pPlayer->m_bNoAutospawn = 1;

			pPlayer->pev->body = 0;
			
			pPlayer->m_bCanRespawn = TRUE;

			if( pPlayer->m_iTeam == 2 )
			{
				Notify( pPlayer, NTC_KILL_V );
				NotifyMid( pPlayer, NTM_KILL_V );
				pPlayer->m_bIsSpecial = FALSE;
				ChangeClass( pPlayer, pPlayer->m_iPrevClass );
				MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
					WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
					WRITE_BYTE( 0 );
				MESSAGE_END( );
			}
			else
			{
				pPlayer->m_bIsSpecial = FALSE;
				ChangeClass( pPlayer, pPlayer->m_iPrevClass );
				Notify( pPlayer, NTC_V_TEAM );
				NotifyMid( pPlayer, NTM_V_TEAM );
				MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
					WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
					WRITE_BYTE( 0 );
				MESSAGE_END( );
			}
						
			pPlayer->Spawn( );
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
	m_iRestart = 0;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
	
		if (!pPlayer)
			continue;
		if( pPlayer->m_bCanRespawn )
		{
			/*if( pPlayer->m_iTeam == 2 )
				Radio( "cprevsyn", pPlayer, 2 );
			else */if( pPlayer->m_iTeam == 1 )
				Radio( "bringcas", pPlayer, 2 );

			if( pPlayer->IsAlive( ) )
				EquipPlayer( pPlayer );
		}
	}
	Recount( );
}

void cPEVip::VipEscaped( )
{
	if( m_iRoundStatus == ROUND_DURING )
		m_iVipEscaped = 1;
}

cPEVip::cPEVip( )
{
	ALERT( at_logged, "Public-Enemy DirtyCase Scenario...\n" );
	m_iVipEscaped = 0;
	m_pVip = NULL;
}

void cPEVip::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	if( !pVictim )
		return;
	if( pVictim->HasNamedPlayerItem( "weapon_case" ) )
	{
		pVictim->DropPlayerItem( "weapon_case" );
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
	CHalfLifeMultiplay::PlayerKilled( pVictim, pKiller, pInflictor );
}

void cPEVip::PlayerSpawn( CBasePlayer *pPlayer )
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

int cPEVip::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[] )
{
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pEntity );
	if( pPlayer )
		pPlayer->ResetRadar( );
	CHalfLifeMultiplay::ClientConnected( pEntity, pszName, pszAddress, szRejectReason );
	return TRUE;
}

void cPEVip::ClientDisconnected( edict_t *pClient )
{
	m_iClients--;
	if( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );
		if( !pPlayer )
			return;
		if( pPlayer->m_iTeam == 1 )
		{
			m_iPlayers[1]--;
			if( pPlayer->IsAlive( ) )
			{
				m_iPlrAlive[0]--;
				m_iPlrAlive[1]--;
			}
		}
		else if( pPlayer->m_iTeam == 2 )
		{
			m_iPlayers[2]--;
			if( pPlayer->IsAlive( ) )
			{
				m_iPlrAlive[0]--;
				m_iPlrAlive[2]--;
			}
		}
		else
			m_iPlayers[0]--;
		if( pPlayer->m_bIsSpecial )
		{
			m_pVip = NULL;
			pPlayer->m_bIsSpecial = FALSE;
		}
		pPlayer->m_iTeam = 0;
		pPlayer->m_sInfo.team = 0;
		pPlayer->m_iSpawnDelayed = 0;
		pPlayer->ResetRadar( );
		CHalfLifeMultiplay::ClientDisconnected( pClient );
		//pPlayer->m_iHUDInited = FALSE;
	}
	return;
}

void cPEVip::Think( )
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
	int frags_remaining;
	int score_remaining;
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
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if( pPlayer && pPlayer->pev->frags >= flFragLimit )
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
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex (i);
		if ( plr && !plr->m_iHUDInited )
		{
			plr->m_iHUDInited = TRUE;
			InitHUD( plr );
			MESSAGE_BEGIN( MSG_ONE, gmsgHideWeapon, NULL, plr->pev );
				WRITE_BYTE( plr->m_iHideHUD );
			MESSAGE_END();
			//if( g_bDbg )
				ALERT( at_logged, "Client initialized\n" );

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