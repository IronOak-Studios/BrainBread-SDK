#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"pe_rules.h"
#include	"pe_rules_hacker.h"
#include	"game.h"
#include	"pe_menus.h"
#include	"pe_weapons_available.h"
#include	"pe_weapon_points.h"
#include	"shake.h"

extern void UTIL_EdictScreenFade( edict_s *edict, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags );

#define bit(a) (1<<a)

extern int gmsgShowMenu;
extern int gmsgShowMenuPE;
extern int gmsgNotify;
extern int gmsgCntDown;
extern int gmsgBriefing;
extern int gmsgPETeam;
int g_iClassPoints[8] = { 30, 25, 20, 30, 25, 20, 30, 30 };

/*void ShowBriefing( CBasePlayer *pPlayer, int off )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgBriefing, NULL, pPlayer->pev );
	WRITE_BYTE( (off?(off==2?2:0):1) );
	MESSAGE_END( );
}*/
void CountDown( CBasePlayer *pPlayer, int dur )
{
	//pPlayer->m_fCntEndTime = gpGlobals->time + dur;

	if( !pPlayer )
		MESSAGE_BEGIN( MSG_ALL, gmsgCntDown, NULL );
	else
		MESSAGE_BEGIN( MSG_ONE, gmsgCntDown, NULL, pPlayer->pev );

		WRITE_BYTE( dur );
	MESSAGE_END( );
}

void Notify( CBasePlayer *pPlayer, int nr, int dur, char *message )
{
	//if( pPlayer->m_iCurNote == nr )
	//	return;
	MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, pPlayer->pev );
		WRITE_COORD( dur );
		WRITE_BYTE( nr );
  if( message )
    WRITE_STRING( message );
	MESSAGE_END( );
	pPlayer->m_iCurNote = nr;
}

void NotifyMid( CBasePlayer *pPlayer, int nr, int dur, char *message, char *message2 )
{
	if( pPlayer->m_iCurMidNote == nr && pPlayer->m_fCurMidNoteEnd > gpGlobals->time )
		return;
    MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, pPlayer->pev );
		WRITE_COORD( dur );
		WRITE_BYTE( nr );
  if( message )
  {
    WRITE_STRING( message );
    WRITE_STRING( ( message2 ? message2 : "" ) );
  }
	MESSAGE_END( );
	pPlayer->m_iCurMidNote = nr;
	pPlayer->m_fCurMidNoteEnd = gpGlobals->time + dur;
}

void NotifyMidTeam( int team, int nr, int dur, char *message, char *message2 )
{
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
	
		if (!pPlayer)
			continue;
		if( pPlayer->m_iTeam != team )
			continue;

		MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, pPlayer->pev );
			WRITE_COORD( dur );
			WRITE_BYTE( nr );
    if( message )
    {
      WRITE_STRING( message );
      WRITE_STRING( ( message2 ? message2 : "" ) );
    }
    MESSAGE_END( );
    pPlayer->m_iCurMidNote = nr;
	  pPlayer->m_fCurMidNoteEnd = gpGlobals->time + dur;
	}
}

void NotifyAll( int nr, int dur, char *message )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgNotify, NULL );
		WRITE_COORD( dur );
		WRITE_BYTE( nr );
  if( message )
    WRITE_STRING( message );
	MESSAGE_END( );
}

void NotifyTeam( int team, int nr, int dur, char *message )
{
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
	
		if (!pPlayer)
			continue;
		if( pPlayer->m_iTeam != team )
			continue;

		MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, pPlayer->pev );
			WRITE_COORD( dur );
			WRITE_BYTE( nr );
    if( message )
      WRITE_STRING( message );
		MESSAGE_END( );
	}
}

void ShowMenu (CBasePlayer *pPlayer, int bitsValidSlots, int nDisplayTime, BOOL fNeedMore, char *pszText)
{
    MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, pPlayer->pev);
        WRITE_SHORT( bitsValidSlots);
        WRITE_CHAR( nDisplayTime );
        WRITE_BYTE( fNeedMore );
        WRITE_STRING (pszText);
    MESSAGE_END();
}

void ShowMenuAll( int bitsValidSlots, int nDisplayTime, BOOL fNeedMore, char *pszText )
{
    MESSAGE_BEGIN( MSG_ALL, gmsgShowMenu, NULL );
        WRITE_SHORT( bitsValidSlots);
        WRITE_CHAR( nDisplayTime );
        WRITE_BYTE( fNeedMore );
        WRITE_STRING (pszText);
    MESSAGE_END();
}
