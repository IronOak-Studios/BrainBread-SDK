#include "hud.h"
#include "cl_util.h"
#include "string.h"
#include "pe_playlist.h"
/*
extern float g_fFogCol[4];
extern float g_fFogStart;
extern float g_fFogEnd;
extern bool g_bFogOn;

void cPlayList::Clear( )
{
	t_song *del;
	m_pCur = m_pFirst;
	for( int i = 0; i < m_iNumSongs; i++ )
	{
		del = m_pCur;
		SetNext( );
		if( del )
			delete del;
	}
	m_iNumSongs	= 0;
	m_pFirst	= NULL;
	m_pLast		= NULL;
	m_pCur		= NULL;
	m_iCurNum	= 0;
}

void cPlayList::BuildListFromFile( char* cListname )
{
}

// Temporary... Will read multiple filenames from a playlist file
void cPlayList::BuildListFromMap( char* cMapname )
{
	char file[256];
	char list[256];
	char token[4096] = "";
	char *pfile;
	char *pstart;
	char *slash;
	int cnt = 0;

	Clear( );

	file[0] = '\0';

	sprintf( list, "\\maps\\%s.cfg", cMapname );
	
	ConsolePrint( "Loading mapsettings from " );
	ConsolePrint( list );
	ConsolePrint( "\n" );

	pstart = pfile = (char*)gEngfuncs.COM_LoadFile( list, 5, NULL);
  if( pstart )
	  pfile = gEngfuncs.COM_ParseFile( pfile, token );

	while( ( strlen( token ) > 0 ) )
	{
		if( !strcmp( token, "fog_settings" ) )
		{
			while( token[0] != '{' && ( strlen( token ) > 0 ) )
				pfile = gEngfuncs.COM_ParseFile( pfile, token );
	
			while( token[0] != '}' && ( strlen( token ) > 0 ) )
			{
				pfile = gEngfuncs.COM_ParseFile( pfile, token );
				if( token[0] != '}' )
				{
					if( cnt == 0 )
						g_fFogCol[0] = atof(token);
					if( cnt == 1 )
						g_fFogCol[1] = atof(token);
					if( cnt == 2 )
						g_fFogCol[2] = atof(token);
					if( cnt == 3 )
						g_fFogCol[3] = atof(token);
					if( cnt == 4 )
						g_fFogStart = atof(token);
					if( cnt == 5 )
						g_fFogEnd = atof(token);
					if( cnt == 6 )
					{
						g_bFogOn = ( CVAR_GET_FLOAT( "cl_fog" ) ? ( atoi(token) ? true : false ) : false );
						if( g_bFogOn )
							ConsolePrint( "Fog activated.\n" );
						else
							ConsolePrint( "Fog deactivated.\n" );
					}
					cnt++;
				}
			}
		}
		
		pfile = gEngfuncs.COM_ParseFile( pfile, token );
	}
	if( pstart )
		gEngfuncs.COM_FreeFile( pstart );
	

	if( gHUD.m_ShowMenuPE.m_bDllLoaded )
	{
		gHUD.m_ShowMenuPE.ResetMP3s( );
		gHUD.m_ShowMenuPE.GetMP3s( (char*)gEngfuncs.pfnGetGameDirectory( ), list );	
		strcpy( file, gHUD.m_ShowMenuPE.GetMP3Nr( 0 ) );
	}

	if( !strstr( file, "\\" ) )
	{
		sprintf( file, "%s\\Public_Enemy_Soundtrack\\%s", gEngfuncs.pfnGetGameDirectory( ), gHUD.m_ShowMenuPE.GetMP3Nr( 0 ) );
	}
	//ConsolePrint( file );
	if( file[0] != '\0' )
	{
		file[strlen(file)-1] = '\0';
		while( slash = strstr( file, "/" ) )
			*slash = '\\';

		ConsolePrint( "Background music file: " );
		ConsolePrint( file );
		ConsolePrint( "\n" );
	}

	m_pFirst = m_pLast = new t_song;
	strcpy( m_pFirst->cFile, file );
	strcpy( m_pFirst->cTitle, file );
	m_pFirst->iLenght = 1;
	m_pFirst->iNum = m_iCurNum = 0;
	m_pCur = m_pFirst->pNext = m_pFirst->pPrev = m_pFirst;
	m_iNumSongs++;
}

void cPlayList::SetNext( )
{
	m_pCur = m_pCur->pNext;
}

void cPlayList::SetNum( int iNum )
{
}

int cPlayList::GetNum( )
{
	return m_pCur->iNum;
}

int cPlayList::GetLength( )
{
	return m_pCur->iLenght;
}

char* cPlayList::GetFilename( )
{
	return m_pCur->cFile;
}

char* cPlayList::GetTitle( )
{
	return m_pCur->cTitle;
}*/

