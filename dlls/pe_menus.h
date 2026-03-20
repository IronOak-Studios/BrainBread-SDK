void NotifyAll( int nr, int dur = 5, char *message = NULL );
void NotifyTeam( int team, int nr, int dur = -1, char *message = NULL );
void NotifyMid( CBasePlayer *pPlayer, int nr, int dur = 5, char *message = NULL, char *message2 = NULL );
void NotifyMidTeam( int team, int nr, int dur = 5, char *message = NULL, char *message2 = NULL );
void Notify( CBasePlayer *pPlayer, int nr, int dur = -1, char *message = NULL );
void ShowMenu (CBasePlayer *pPlayer, int bitsValidSlots, int nDisplayTime, BOOL fNeedMore, char *pszText);
void ShowMenuAll( int bitsValidSlots, int nDisplayTime, BOOL fNeedMore, char *pszText );
void ShowMenuPE( CBasePlayer *pPlayer );
void ShowBriefing( CBasePlayer *pPlayer, int off = 0 );
//---
void TeamMenu( CBasePlayer *pPlayer );
void ClassMenu( CBasePlayer *pPlayer );
//---
void CalcPoints( CBasePlayer *pPlayer );
void CheckEquip( CBasePlayer *pPlayer );
int IsAvailable( int menu/*, int cat*/, int team, int slot, int oldslot, CBasePlayer *pPlayer = NULL/*, int diff = 0*/ );
int IsAvailable2( int menu/*, int cat*/, int team, int* slot, int oldslot, CBasePlayer *pPlayer = NULL/*, int diff = 0*/ );
void CountDown( CBasePlayer *pPlayer, int dur );