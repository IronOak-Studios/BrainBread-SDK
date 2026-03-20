

class cPERules : public CHalfLifeMultiplay
{
public:
	cPERules( );
	virtual void ResetMap( );
	virtual BOOL IsVoteable( char *var );
	virtual int UpdateCounter( );
	virtual BOOL IsTeamplay( ) { return TRUE; }
	virtual BOOL IsMultiplayer( ) { return TRUE; }
	virtual BOOL IsDeathmatch( ) { return TRUE; }
	virtual const char *GetGameDescription( ) { return "BrainBread"; }
	virtual void Radio( char *szName, CBasePlayer *pPlayer, int iType = 0 );
	virtual void RadioAll( char *szName, int iType = 0 );
	virtual void RadioTeam( CBasePlayer* pPlayer, char *szName, int iType = 0 );
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
  virtual BOOL FAllowMonsters( ) { return TRUE; }

protected:
	float	m_fNextThink;
	float	m_flUpdateCounter;
	float	m_flRoundEndTime;
	float	m_fNextVote;
	float	m_fNextVarVote;
	float	m_fResetTime;
	int		m_bDoReset;
	int		m_iYesVotes;
	int		m_iNoVotes;
	int		m_iYesVarVotes;
	int		m_iNoVarVotes;
	char	m_sVoteMap[64];
	char	m_sVoteVar[2][64];
};