#define MOD( a, b ) ( (a) - (int)( (a) / (b) ) * (b) )
#define MAX_GIBCOUNT gibcnt.value
#define MAX_ZOMBIECOUNT zombiecnt.value

void ShowTextAll( char *Text, float HoldTime, int rColor = 255, int gColor = 255, int bColor = 255, float xPos = -1, float yPos = -1, float FadeIn = 0, float FadeOut = 0, int Channel = 2 );
void ShowText( CBasePlayer* pPlayer, char *Text, float HoldTime, int rColor = 255, int gColor = 255, int bColor = 255, float xPos = -1, float yPos = -1, float FadeIn = 0, float FadeOut = 0, int Channel = 2 );
#define SET_MDL( a ) g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex( ), g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", a )
void SetWpnSlot( int id, int slot, CBasePlayer* pPlayer );
void uPrintTeam( int iTeamNr, int msg_dest, const char *msg_name, const char *param1 = NULL );
const char *uEqPlr( CBasePlayer* pPlayer, int wpn, int ammo = 0, int ammo2 = 0 );
void uPlayAll( const char *wave );
void uShake( CBasePlayer *pPlayer, float dur, float freq, float ampl );
int IsUber( edict_t *player );
void Help( int id, CBasePlayer *plr, float param = 0 );
void ForceHelp( int id, CBasePlayer *plr, float param = 0 );
void HelpTeam( int id, int team, float param = 0 );
void ForceHelpTeam( int id, int team, float param = 0 );
void uPrepSpawn( );
float uNearestPlayerDist( vec3_t origin );
void uSavePlayerData( );
bool uLoadPlayerExp(CBasePlayer *plr);
bool uSavePlayerExp(CBasePlayer *plr);
bool uSaveAllExp();
void uCloseExpDb( void );