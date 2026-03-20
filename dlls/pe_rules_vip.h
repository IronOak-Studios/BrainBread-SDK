

class cPEVip : public cPEHacking
{
public:
	cPEVip( );
	virtual void Think( );
	virtual void StartRound( );
	virtual void CheckRoundEnd( );
	virtual void VipEscaped( );
	virtual int  ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[] );
	virtual void ClientDisconnected( edict_t *pClient );
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );

public:
	int		m_iVipEscaped;
	CBasePlayer *m_pVip;
};