
class cBBMapMission : public CPointEntity
{
public:
  void KeyValue( KeyValueData *pkvd )
  {
	  if( FStrEq( pkvd->szKeyName, "topnotify" ) )
	  {
		  strncpy( notify[0], pkvd->szValue, 256 );
		  pkvd->fHandled = TRUE;
	  }	
    else if( FStrEq( pkvd->szKeyName, "midnotifyone" ) )
	  {
		  strncpy( notify[1], pkvd->szValue, 256 );
		  pkvd->fHandled = TRUE;
	  }
    else if( FStrEq( pkvd->szKeyName, "midnotifytwo" ) )
	  {
		  strncpy( notify[2], pkvd->szValue, 256 );
		  pkvd->fHandled = TRUE;
	  }
  }
  void Spawn( )
  {
    pev->solid = SOLID_NOT;
    pev->movetype = MOVETYPE_NONE;
	  pev->effects |= EF_NODRAW;
    complete = false;
    SetUse( NULL );
  }
  void Activate( )
  {
    complete = false;
    //FireTargets( STRING( pev->target ), this, this, USE_TOGGLE, 0 );
    SetUse( &cBBMapMission::Use );
  }
  void Deactivate( )
  {
    complete = false;
    SetUse( NULL );
  }
  void EXPORT Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
  {
    complete = true;
    FireTargets( STRING( pev->message ), this, this, USE_TOGGLE, 0 );
  }
  char notify[3][256];
  bool complete;
};

class cPEHacking : public cPERules
{
public:
	cPEHacking( );
	virtual void Think( );
	virtual void EquipPlayer( CBasePlayer *pPlayer );
	virtual void SetTeam( int iSelection, CBasePlayer *pPlayer, int kill = 1, int model = 0 );
	virtual void CalcTeamBonus( );
	virtual void StartRound( );
	virtual void CheckRoundEnd( );
	virtual void HackedATerminal( int nr, char *name );
	virtual void UnhackedATerminal( int nr, char *name );
	virtual void AutoTeam(  );
	virtual void ChangeClass( CBasePlayer *pPlayer, int classnr );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual BOOL InitHUD( CBasePlayer *pPlayer );
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer );
	virtual int SmallestTeam( );
	virtual int TeamsUneven( );
	virtual void UpdTeamScore( );
	virtual void AddScore( int team, int amount ) { if( m_iCountScores  ) m_iTeamPoints[team] += amount; }
	virtual void RemoveScore( int team, int amount ) { if( m_iCountScores  ) m_iTeamPoints[team] -= amount; }
	virtual int ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[] );
	virtual void ClientDisconnected( edict_t *pClient );
	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd );
	virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual BOOL ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target ) { return FALSE; }
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void Recount( );
	virtual void CheckVars( );

	virtual int Players1( );
	virtual int Players2( );

  void AbortMission( );
  void StartMission( );
  void CheckAllMissions( );
  bool CheckMission( );
  void PlayerInitMission( CBasePlayer *plr );
  int RandomMission( bool reset = false );
  
  float misDone[20];
  float misReq[20];

  char missionList[20][3][32];
  int missionTimes[5];

  bool oneEscaped;

  int misType;
  int misNr;
  float misStart;
  bool misComplete;
  bool misLast;

  bool heliactivated;
  cBBMapMission* curMapMission;
 
  float nextSave;

	int		m_iClients;
	int		m_iPlayers[3];
	int		m_iPlrAlive[3];
	int		m_iRoundStatus;
	int		m_iTeamPoints[3];
	int		m_iHacked1;
	int		m_iHacked2;
	int		m_iTerminalCheck;
	int		m_iOverrideModels;
	int		m_iRestart;
	int		m_iTerminalsHacked;
	int		m_iCountScores;
	int		m_bBalance;
	char	m_sValidMdls[3][3][256];
	int		m_iLastSpec;
	float	m_fFHack1;
	float	m_fFHack2;
	int		m_iHack1;
	int		m_iHack2;
	int		rounds;
	CBasePlayer *m_pHacker;
};