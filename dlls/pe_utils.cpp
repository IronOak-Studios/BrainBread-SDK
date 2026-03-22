#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "game.h"
#include "weapons.h"
#include "player.h"
#include "pe_utils.h"
#include "weapons.h"
#include "gamerules.h"
#include "pe_rules.h"
#include "pe_rules_hacker.h"
#include <time.h>
#include <list>
#include "sqlite/sqlite3.h"
#ifdef WIN32
#include <fstream>
#endif
using namespace std;

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

#define CB_TORSO		(1<<0)
#define CB_ARMS			(1<<1)
#define CB_LEGS			(1<<2)
#define CB_HEAD			(1<<3)
#define CB_NANO			(1<<4)

void uGiveExpToID( char *id, float amount )
{
  for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
    if( !plr || !strlen( STRING( plr->pev->netname ) ) || strcmp( id, GETPLAYERAUTHID( plr->edict( ) ) ) )
			continue;
    plr->GiveExp( amount );
	}
}

extern char *COM_Parse (char *data, char *com_token );
extern unsigned long ElfHash( const char *name );
struct s_xphashitem
{
  long key;
  float value;
};
s_xphashitem *players = NULL;
int playerscount = 0;

int uHashIndex( unsigned long hash )
{
  for( int i = 0; i < playerscount; i++ )
    if( players[i].key == hash )
      return i;
  return -1;
}

int uFreeIndex( )
{
  for( int i = 0; i < playerscount; i++ )
    if( players[i].key == 0 )
      return i;
  return -1;
}

bool uUseSqlite()
{
	const char *sqlite = ".sqlite";
	if (strlen(playerinfofile.string) > strlen(sqlite) && !strcmp(playerinfofile.string + strlen(playerinfofile.string) - strlen(sqlite), sqlite))
		return true;
	return false;
}

bool clearPlayerXp = false;
void uSavePlayerData( )
{
  if( savexp.value == 0.0f )
    return;
	if (uUseSqlite())
		return;
  int length;
  char *plfile; 
  char *playerlist = plfile = (char*)LOAD_FILE_FOR_ME( playerinfofile.string, &length );
  char token[1500];
  char id[128];
  float xp = 0;
  long hash = 0;
  long inittime = 0, curtime = time( NULL );
  int idx = 0, i = 0;
  list<s_xphashitem> plrlist;
  s_xphashitem cur;
  if( playerlist && length )
  {
    playerlist = COM_Parse( playerlist, token );
    inittime = atol( token );
    playerlist = COM_Parse( playerlist, token );
    while( playerlist && length )
    {
      cur.key = atol( token );
      playerlist = COM_Parse( playerlist, token );
      cur.value = atof( token );

      plrlist.push_back( cur );
      playerlist = COM_Parse( playerlist, token );
    }
  }
 	FREE_FILE( plfile );

  if( players )
  {
    delete[] players;
    players = NULL;
  }
  playerscount = plrlist.size( ) + 32;
  players = new s_xphashitem[playerscount];
  memset( players, NULL, playerscount * sizeof(s_xphashitem) );
  list<s_xphashitem>::iterator it = plrlist.begin( );
  int o = 0;
  for( ; it != plrlist.end( ); it++ )
  {
    players[o].key = (*it).key;
    players[o++].value = (*it).value;
  }

  for( i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
    if( !plr || !strlen( STRING( plr->pev->netname ) ) )
			continue;
    strcpy( id, GETPLAYERAUTHID( plr->edict( ) ) );
    hash = ElfHash( id );
    idx = uHashIndex( hash );
    if( idx == -1 )
    {
      idx = uFreeIndex( );
      if( idx == -1 )
        continue;
      players[idx].key = hash;
    }
    if( players[idx].value <= plr->exp )
      players[idx].value = plr->exp;
    else
      plr->GiveExp( players[idx].value );//uGiveExpToID( id, players[idx].value );
	}

  char file[260];
  char *slash;
  GET_GAME_DIR( file );
  strcat( file, "\\" );
  strcat( file, playerinfofile.string );

  if( inittime )
  {
    float diff = ( curtime - inittime ) / 3600.0f;
    if( ( savexp.value > 0 ) && ( diff >= savexp.value ) )
    {
      clearPlayerXp = true;
      inittime = curtime;
    }
  }
  else
    inittime = curtime;

#ifndef WIN32 /// LINUX FILE SAVING ROUTINE

  while( slash = strstr( file, "\\" ) )
    *slash = '/';
  FILE* fl;
  fl = fopen( file, "w" );
  if( !fl )
    return;
  fprintf( fl, "%ld\n", inittime );
  if( !clearPlayerXp )
  {
    for( i = 0; i < playerscount; i++ )
    {
      if( !players[i].key )
        continue;
      fprintf( fl, "%ld %f\n", players[i].key, players[i].value );
    }
  }
  fclose( fl );

#else /// WINDOWS FILE SAVING ROUTINE

  while( slash = strstr( file, "/" ) )
    *slash = '\\';
  ofstream fl;
  fl.open( file, ios::out );
  if( !fl.is_open( ) )
    return;
  char text[256];
  sprintf( text, "%ld\n", inittime );
  fl.write( text, strlen( text ) );
  if( !clearPlayerXp )
  {
    for( i = 0; i < playerscount; i++ )
    {
      if( !players[i].key )
        continue;
      sprintf( text, "%ld %f\n", players[i].key, players[i].value );
      fl.write( text, strlen( text ) );
    }
  }
  fl.close( );

#endif
  //if( clear )
  //  ((cPEHacking*)g_pGameRules)->GoToIntermission( );
}

bool _dbError(const char *action, sqlite3 *db = NULL, sqlite3_stmt *stmt = NULL, bool close = true)
{
	ALERT(at_error, "Player EXP DB: Failed to %s (%s)\n", action, sqlite3_errmsg(db));
	sqlite3_finalize(stmt);
	if (db && close)
		sqlite3_close(db);	
	return false;
}

bool _dbOpen(sqlite3 **db)
{
	if (!db)
		return false;
	if (*db)
		return true;

	char file[1024];
	char *slash;
	GET_GAME_DIR(file);
	strncat(file, "\\", 1024);
	strncat(file, playerinfofile.string, 1024);

#ifndef WIN32 /// LINUX
	while (slash = strstr(file, "\\"))
		*slash = '/';
#else /// WINDOWS
	while (slash = strstr(file, "/"))
		*slash = '\\';
#endif

	if (sqlite3_open(file, db) != SQLITE_OK)
		return _dbError("open", *db);
	return true;
}

bool _uCreateExpSchema(sqlite3 *extDb)
{
	sqlite3 *db = extDb;
	if (!db && !_dbOpen(&db))
		return false;

	if (sqlite3_exec(db, "create table player (steamid text primary key, exp double not null, time int not null)", NULL, NULL, NULL) != SQLITE_OK)
		return _dbError("create player table", db);
	if (!extDb)
		sqlite3_close(db);
	return true;
}

bool _uLoadPlayerExp(CBasePlayer *plr, sqlite3 *extDb)
{
	if (savexp.value == 0.0f)
		return true;

	sqlite3 *db = extDb;
	if (!db && !_dbOpen(&db))
		return false;

	char id[128];
	strncpy(id, GETPLAYERAUTHID(plr->edict()), 128);

	sqlite3_stmt *stmt = NULL;
	if (sqlite3_prepare_v2(db, "select exp, time from player where steamid=?", -1, &stmt, NULL) != SQLITE_OK)
		return _dbError("prepare load query", db, stmt);

	if (sqlite3_bind_text(stmt, 1, id, strlen(id), NULL) != SQLITE_OK)
		return _dbError("bind load id", db, stmt);

	int step = sqlite3_step(stmt);
	double exp = 0;
	int saveTime = 0;

	if (step == SQLITE_ROW)
	{
		exp = sqlite3_column_double(stmt, 0);
		saveTime = sqlite3_column_int(stmt, 1);
	}
	else if (step != SQLITE_DONE)
		return _dbError("run load query", db, stmt);

	plr->expLoaded = true;
	float age = (time(NULL) - saveTime) / 3600.0f;
	if (savexp.value == -1 || age <= savexp.value)
	{
		plr->GiveExp(exp, true);
		ALERT(at_console, "Loaded exp for %s: %f (age %f)\n", id, exp, age);
	}
	else
		ALERT(at_console, "Exp for %s has expired (age %f)\n", id, age);

	sqlite3_finalize(stmt);
	if (!extDb)
		sqlite3_close(db);
	return true;
}

bool uLoadPlayerExp(CBasePlayer *plr)
{
	if (!uUseSqlite())
		return false;

	return _uLoadPlayerExp(plr, NULL);
}

bool uSaveAllExp()
{
	if (savexp.value == 0.0f)
		return true;

	if (!uUseSqlite())
		return false;

	sqlite3 *db = NULL;
	if (!_dbOpen(&db))
		return false;

	sqlite3_stmt *stmt = NULL;
	if (sqlite3_prepare_v2(db, "insert or replace into player (steamid, exp, time) values(?,?,?)", -1, &stmt, NULL) != SQLITE_OK)
	{
		_dbError("prepare insert statement", db, stmt, false);
		_uCreateExpSchema(db);
		sqlite3_finalize(stmt);
		sqlite3_close(db);
		return false;
	}

	if (sqlite3_exec(db, "begin transaction", NULL, NULL, NULL) != SQLITE_OK)
		return _dbError("start transaction", db);

	char id[128];
	int i = 0;
	for (i = 1; i <= MAX_PLAYERS; i++)
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex(i);
		if (!plr || !strlen(STRING(plr->pev->netname)))
			continue;

		if (!plr->expLoaded)
		{
			_uLoadPlayerExp(plr, db);
			continue;
		}

		strncpy(id, GETPLAYERAUTHID(plr->edict()), 128);
		if (!strlen(id))
			continue;

		if (sqlite3_bind_text(stmt, 1, id, strlen(id), NULL) != SQLITE_OK)
			return _dbError("bind insert id", db, stmt);
		if (sqlite3_bind_double(stmt, 2, (double)plr->exp) != SQLITE_OK)
			return _dbError("bind insert exp", db, stmt);
		if (sqlite3_bind_int(stmt, 3, time(NULL)) != SQLITE_OK)
			return _dbError("bind insert time", db, stmt);

		if (sqlite3_step(stmt) != SQLITE_DONE)
			return _dbError("run insert statement", db, stmt);
		//ALERT(at_console, "Saved exp for player %s: %f\n", id, plr->exp);
		sqlite3_reset(stmt);
	}
	sqlite3_finalize(stmt);

	if (sqlite3_exec(db, "commit transaction", NULL, NULL, NULL) != SQLITE_OK)
		return _dbError("commit transaction", db);

	sqlite3_close(db);
	ALERT(at_console, "Saved exp for all players\n");
	return true;
}

float uNearestPlayerDist( vec3_t origin )
{
  float dist = 999999;
  float cur = 0;
	for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
    if( !plr || !plr->IsAlive( ) )
			continue;
    cur = ( plr->pev->origin - origin ).Length2D( );
		if( cur < dist )
			dist = cur;
	}
  return dist;
}

void uPrepSpawn( )
{
	int i = 0;
	memset( gListCorp, 0, sizeof(s_spawnlist) * MAX_SPAWN_SPOTS );
	memset( gListSynd, 0, sizeof(s_spawnlist) * MAX_SPAWN_SPOTS );
  memset( gListBackup, 0, sizeof(s_spawnlist) * MAX_SPAWN_SPOTS );
	CBaseEntity *respawn = UTIL_FindEntityByClassname( NULL, "bb_spawn_player" );
	while( i < MAX_SPAWN_SPOTS && respawn != NULL )
	{
		gListCorp[i].spot = respawn;
		gListCorp[i].player = NULL;
		i++;
		respawn = UTIL_FindEntityByClassname( respawn, "bb_spawn_player" );
	}
	i = 0;

	respawn = UTIL_FindEntityByClassname( NULL, "bb_zombie_player" );
	while( i < MAX_SPAWN_SPOTS && respawn != NULL )
	{
    gListSynd[i].spot = respawn;
		gListSynd[i].player = NULL;
		i++;
		respawn = UTIL_FindEntityByClassname( respawn, "bb_zombie_player" );
	}	
  i = 0;

	respawn = UTIL_FindEntityByClassname( NULL, "bb_spawn_zombie" );
	while( i < MAX_SPAWN_SPOTS && respawn != NULL )
	{
    gListBackup[i].spot = respawn;
		gListBackup[i].player = NULL;
		i++;
		respawn = UTIL_FindEntityByClassname( respawn, "bb_spawn_zombie" );
	}	
}

int IsUber( edict_t *player )
{
#ifdef _DEBUG
	 if( !strcmp( GETPLAYERAUTHID( player ), "STEAM_0" ) )
		 return 1;
#endif
	 return 0;
}
void SetWpnSlot( int id, int slot, CBasePlayer* pPlayer )
{ 
	pPlayer->m_iSlots[slot-2] = id;
}

void uPrintTeam( int iTeamNr, int msg_dest, const char *msg_name, const char *param1 )
{
	for (int i = 1; i <= MAX_PLAYERS; i++)
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if( !plr )
			continue;
		if( plr->m_iTeam == iTeamNr )
			ClientPrint( plr->pev, msg_dest, msg_name, param1 );
	}
}

void ShowTextAll( char *Text, float HoldTime, int rColor, int gColor, int bColor, float xPos, float yPos, float FadeIn, float FadeOut, int Channel )
{
		char ShowTextAll[512];
		hudtextparms_t hText;
		
		memset(&hText, 0, sizeof(hText));
		
		hText.channel		= Channel;
		hText.x				= xPos;
		hText.y				= yPos;
		hText.effect		= 0;
		hText.r2			= hText.g2 = hText.b2 = 100;
		hText.a1			= 128;			
		hText.r1			= rColor;
		hText.g1			= gColor;
		hText.b1			= bColor;
		hText.a2			= 100;
		hText.fadeinTime	= FadeIn;	
		hText.fadeoutTime	= FadeOut;
		hText.holdTime		= HoldTime; 
		hText.fxTime		= 0;

		sprintf(ShowTextAll, Text); 

		UTIL_HudMessageAll( hText, ShowTextAll);
}

void ShowText( CBasePlayer* pPlayer, char *Text, float HoldTime, int rColor, int gColor, int bColor, float xPos, float yPos, float FadeIn, float FadeOut, int Channel )
{
		char ShowTextAll[512];
		hudtextparms_t hText;
		
		memset(&hText, 0, sizeof(hText));
		
		hText.channel		= Channel;
		hText.x				= xPos;
		hText.y				= yPos;
		hText.effect		= 0;
		hText.r2			= hText.g2 = hText.b2 = 100;
		hText.a1			= 128;		
		hText.r1			= rColor;
		hText.g1			= gColor;
		hText.b1			= bColor;
		hText.a2			= 100;
		hText.fadeinTime	= FadeIn;	
		hText.fadeoutTime	= FadeOut;
		hText.holdTime		= HoldTime; 
		hText.fxTime		= 0;

		sprintf(ShowTextAll, Text); 

		UTIL_HudMessage( pPlayer, hText, ShowTextAll );
}

void UTIL_GetWpnAmmo( const char *szClassname )
{

}

const char *uEqPlr( CBasePlayer* pPlayer, int wpn, int ammo, int ammo2 )
{
	float give = 100;
	if( !wpn || wpn > MAX_WEAPONS-1 )
	{
		//ALERT( at_logged, "Weapon %d is invalid\n", wpn );
		return NULL;
	}
	ItemInfo& II = CBasePlayerItem::ItemInfoArray[wpn];
	if ( !II.iId )
	{
		//ALERT( at_logged, "Weapon %d is not in array\n", wpn );
		return NULL;
	}

	pPlayer->GiveNamedItem( II.pszName );
	//pPlayer->SelectItem( II.pszName );
	if( II.pszAmmo1 )
	{
		if( FBitSet( pPlayer->m_iCyber, CB_ARMS ) )
			give += 20.0;
		if( FBitSet( pPlayer->m_iCyber, CB_TORSO ) )
			give += 30.0;
		if( FBitSet( pPlayer->m_iAddons, AD_NANO_SKIN ) )
			give -= 15.0;

		pPlayer->GiveAmmo(
			(int)( (float)( (ammo > 0 ) ? ammo : II.iMaxAmmo1 ) * ( give / 100.0 ) ),
			(char *)II.pszAmmo1,
			(int)( (float)II.iMaxAmmo1 * ( give / 100.0 ) )
			);

		if( ( pPlayer->m_iCurSlot < 5 ) )
			pPlayer->m_sAmmoSlot[pPlayer->m_iCurSlot-2] = II.pszAmmo1;
	}
	if( II.pszAmmo2 )
	{
		pPlayer->GiveAmmo( ( (ammo2 > 0 ) ? ammo2 : II.iMaxAmmo2 ), (char *)II.pszAmmo2, II.iMaxAmmo2 );
		pPlayer->m_sAmmo_aSlot[pPlayer->m_iCurSlot-2] = II.pszAmmo2;
	}
	if( ( pPlayer->m_iCurSlot >= 3 ) && ( pPlayer->m_iCurSlot < 5 ) )
		SetWpnSlot( wpn, pPlayer->m_iCurSlot-1, pPlayer );
	return II.pszName;
}

void uPlayAll( const char *wave )
{
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pl = (CBasePlayer *)UTIL_PlayerByIndex( i );
	
		if( pl )
			CLIENT_COMMAND( pl->edict( ), "play %s\n", wave );
	}
}
extern int gmsgShake;
static unsigned short FixedUnsigned16( float value, float scale )
{
	int output;

	output = value * scale;
	if ( output < 0 )
		output = 0;
	if ( output > 0xFFFF )
		output = 0xFFFF;

	return (unsigned short)output;
}

void uShake( CBasePlayer *pPlayer, float dur, float freq, float ampl )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgShake, NULL, pPlayer->edict() );
		WRITE_SHORT( FixedUnsigned16( ampl, 1<<12 ) );
		WRITE_SHORT( FixedUnsigned16( dur, 1<<12 ) );
		WRITE_SHORT( FixedUnsigned16( freq, 1<<8 ) );
	MESSAGE_END();
}
extern int gmsgHelp;
void Help( int id, CBasePlayer *plr, float param )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgHelp, NULL, plr->pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( id );
		WRITE_COORD( param );
	MESSAGE_END( );
}

void ForceHelp( int id, CBasePlayer *plr, float param )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgHelp, NULL, plr->pev );
		WRITE_BYTE( 1 );
		WRITE_BYTE( id );
		WRITE_COORD( param );
	MESSAGE_END( );
}

void HelpTeam( int id, int team, float param )
{
	for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if( plr && plr->m_iTeam == team )
			Help( id, plr, param );
	}
}


void ForceHelpTeam( int id, int team, float param )
{
	for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if( plr && plr->m_iTeam == team )
			ForceHelp( id, plr, param );
	}
}
