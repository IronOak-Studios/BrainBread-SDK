class cPEPointMgr: public CHudBase
{
public:
 ~cPEPointMgr( );
 int Init( );
 int VidInit( );
 int Draw( float time );
 int MsgFunc_UpdPoints( const char *pszName,int iSize, void *pbuf );
 int MsgFunc_Stats( const char *pszName,int iSize, void *pbuf );

public:
	float points;
	int lastAdd;
};