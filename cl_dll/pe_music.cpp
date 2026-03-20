#include "hud.h"
#include "cl_util.h"
#include "demo.h"
#include "demo_api.h"
#include "parsemsg.h"
#include "string.h"
#include "pe_playlist.h"
#include "pe_rain.h"
#include "bb_blood.h"
#include "bb_sprays.h"
#include <list>
using namespace std;

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Music,PlayMusic)
DECLARE_MESSAGE(m_Music,Spray)

DECLARE_MESSAGE(m_Music,SetVol)

extern int g_sprHUD2;
extern bool g_bFogOn;
extern float fTime;
//extern int iRainIndex;
//extern vec3_t vRainMin[20];
//extern vec3_t vRainMax[20];
//extern t_partinfo sRainInfo[20];
//extern cRain *gRain[20];

void COM_FileBase( const char *in, char *out );

list<t_bloodpartinfo> infol;

cPlayList gList;

int CHudMusic::Init( )
{
	//char file[1024];
//	char *offset;

	HOOK_MESSAGE(PlayMusic);
	HOOK_MESSAGE(Spray);

	HOOK_MESSAGE(SetVol);

	CVAR_CREATE( "cl_detailsprites", "1", FCVAR_ARCHIVE );
	CVAR_CREATE( "cl_partsys", "1", FCVAR_ARCHIVE );
	CVAR_CREATE( "cl_helpsys", "1", FCVAR_ARCHIVE );
	CVAR_CREATE( "cl_fadesys", "1", FCVAR_ARCHIVE );
	CVAR_CREATE( "cl_fontsys", "1", FCVAR_ARCHIVE );
  CVAR_CREATE( "cl_lensflare", "1", FCVAR_ARCHIVE );

  CVAR_CREATE( "cl_blood", "3", FCVAR_CLIENTDLL );

	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);

	// FMOD stuff
	/*m_bInited = FALSE;
	m_pSoundStream = NULL;
	m_fVolume = 0;

	sprintf( file, "%s/fmod.dll", gEngfuncs.pfnGetGameDirectory( ) );

	while( offset = strstr( file, "/" ) )
		*offset = '\\';

	m_hDll = LoadLibrary( file );

	if( !m_hDll )
		return 0;

	if(	!(
		( (FARPROC&)InitFmod		= GetProcAddress( m_hDll, "_FSOUND_Init@12" ) ) &&
		( (FARPROC&)CloseFmod		= GetProcAddress( m_hDll, "_FSOUND_Close@0" ) ) &&
		( (FARPROC&)SetOutput		= GetProcAddress( m_hDll, "_FSOUND_SetOutput@4" ) ) &&
		( (FARPROC&)SetDriver		= GetProcAddress( m_hDll, "_FSOUND_SetDriver@4" ) ) &&
		( (FARPROC&)SetVolume		= GetProcAddress( m_hDll, "_FSOUND_SetVolume@8" ) ) &&
		( (FARPROC&)GetLength		= GetProcAddress( m_hDll, "_FSOUND_Stream_GetLength@4" ) ) &&
		( (FARPROC&)GetTime			= GetProcAddress( m_hDll, "_FSOUND_Stream_GetTime@4" ) ) &&
		( (FARPROC&)IsPlaying		= GetProcAddress( m_hDll, "_FSOUND_IsPlaying@4" ) ) &&
		( (FARPROC&)GetVolume		= GetProcAddress( m_hDll, "_FSOUND_GetVolume@4" ) ) &&
		( (FARPROC&)OpenStream		= GetProcAddress( m_hDll, "_FSOUND_Stream_Open@16" ) ) &&
		( (FARPROC&)CloseStream		= GetProcAddress( m_hDll, "_FSOUND_Stream_Close@4" ) ) &&
		( (FARPROC&)PlayStream		= GetProcAddress( m_hDll, "_FSOUND_Stream_Play@8" ) )
		)
	)
	{ 
		FreeLibrary( m_hDll );
		return 0;
	}

	if( !CVAR_GET_FLOAT( "cl_playmusic" ) )
		return 1;
	SetOutput( FSOUND_OUTPUT_DSOUND );
	SetDriver( 0 );
	InitFmod( 44100, 1, FSOUND_INIT_GLOBALFOCUS );
	m_bInited = TRUE;*/
	//---
	return 1;
}

bool mapinit = false;
extern s_bloodlist *gFBlood;

int CHudMusic::VidInit( )
{
	//m_bShouldPlay = 0;
	m_bGotCommand = 0;
	m_bPlaying = 0;
	m_iFlags |= HUD_ACTIVE;
	m_fOldVolume = -1;

	// Rain stuff handled here
	fTime = 0;
	//iRainIndex = 0;
  infol.clear( );
  cBlood::KillAll( gFBlood );

	/*for( int i = 0; i < 10; i++ )
	{
		if( gRain[i] )
		{
			delete gRain[i];
			gRain[i] = NULL;
		}
	}*/
	g_bFogOn = false;
	mapinit = false;

	return 1;
}

void CHudMusic::Think( )
{
	int speaking = 0;
	m_fVolume = CVAR_GET_FLOAT( "cl_musicvolume" );
	if( m_bGotCommand )
		m_bShouldPlay = (int)CVAR_GET_FLOAT( "cl_playmusic" );

	for( int i = 0; i < 32; i++ )
		if( GetClientVoiceMgr( )->m_VoicePlayers[i] )
			speaking = 1;
	if( speaking )
		m_fVolume = CVAR_GET_FLOAT( "cl_talkmusicvolume" );

	/*if( !mapinit && gEngfuncs.pfnGetLevelName( ) )
	{
		char map[128];
		COM_FileBase( gEngfuncs.pfnGetLevelName( ), map );
		gList.BuildListFromMap( map );
		mapinit = true;
	}*/

  if( gEngfuncs.pDemoAPI->IsPlayingback( ) )
    return;

	if( m_fVolume != m_fOldVolume )
	{
		if( m_fVolume > 1 )
			m_fVolume = 1;
		if( m_fVolume < 0 )
			m_fVolume = 0;

		//SetVolume( FSOUND_ALL, (int)( m_fVolume * 255.0f ) );
		m_fOldVolume = m_fVolume;
	}

	/*if( !m_bPlaying && m_bShouldPlay )
	{
		if( m_pSoundStream )
		{
			CloseStream( m_pSoundStream );
			m_pSoundStream = NULL;
		}


		if( (int)CVAR_GET_FLOAT( "cl_playmusic" ) != 1 )
		{
			m_bShouldPlay = 0;
			return;
		}

		if( !m_bInited )
		{
			SetOutput( FSOUND_OUTPUT_DSOUND );
			SetDriver( 0 );
			InitFmod( 44100, 1, FSOUND_INIT_GLOBALFOCUS );
			m_bInited = TRUE;
		}


		m_pSoundStream = OpenStream( gList.GetFilename( ), FSOUND_NORMAL | FSOUND_LOOP_NORMAL, 0, 0 );

		if( !m_pSoundStream )
		{
			ConsolePrint( "ERROR: Can't load MP3 file.\n" );
			m_bShouldPlay = 0;
			m_bGotCommand = 0;
			return;
		}

		PlayStream( 0, m_pSoundStream );
		SetVolume( FSOUND_ALL, (int)( m_fVolume * 255.0f ) );
    char text[512];
    sprintf( text, "Playing %s\n\0", gList.GetFilename( ) );
    ConsolePrint( text );
		m_bPlaying = 1;
	}
	if( !m_bShouldPlay && m_bPlaying )
	{
		if( m_pSoundStream )
			CloseStream( m_pSoundStream );
		m_pSoundStream = NULL;
		m_bPlaying = 0;
	}*/
}

int CHudMusic::Draw( float flTime )
{
	return 1; 
}

extern cvar_t *cl_forwardspeed;
extern cvar_t *cl_backspeed;
extern cvar_t *cl_sidespeed;
int CHudMusic::MsgFunc_PlayMusic( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int i = 0;
	//char text[128];
	m_bGotCommand = READ_BYTE( );
  if( m_bGotCommand == 2 && cl_forwardspeed )
  {
    cl_forwardspeed->value = 400;
    cl_backspeed->value = 400;
    cl_sidespeed->value = 400;
    return TRUE;
  }
  if( m_bGotCommand == 3 && cl_forwardspeed )
  {
	float spd = READ_COORD( );
    cl_forwardspeed->value = spd;
    cl_backspeed->value = spd;
    cl_sidespeed->value = spd;
    return TRUE;
  }
  /*ConsolePrint( "Starting particle systems.\n" );

	while( sRainInfo[i].iType = READ_BYTE( ) )
	{
		sRainInfo[i].iPartsPerSec = READ_SHORT( );//800;
		sRainInfo[i].fSpeed = READ_COORD( );//800;
		sRainInfo[i].fSize = READ_COORD( );//1.75;
		sRainInfo[i].fSideSpread = READ_COORD( );//0;
*/
		/*sprintf( text, "%d, %f, %f, %f\n", sRainInfo[i].iPartsPerSec, sRainInfo[i].fSpeed,
			sRainInfo[i].fSize, sRainInfo[i].fSideSpread );
		ConsolePrint( text );*/
/*		vRainMin[i].x = READ_COORD( );
		vRainMin[i].y = READ_COORD( );
		vRainMin[i].z = READ_COORD( );

		vRainMax[i].x = READ_COORD( );
		vRainMax[i].y = READ_COORD( );
		vRainMax[i].z = READ_COORD( );
		i++;
	}*/
	
  /*sRainInfo[i].iType = TYPE_GLOBALRAIN;
	sRainInfo[i].iPartsPerSec = 1;
	sRainInfo[i].fSpeed = 800;
	sRainInfo[i].fSize = 1;
	sRainInfo[i].fSideSpread = 0;

	vRainMin[i] = Vector( 0, 0, 0 );

	vRainMax[i] = Vector( 1800, 1800, 1200 );
	i++;*/

  //iRainIndex = ( CVAR_GET_FLOAT( "cl_weather" ) ? i : 0 );

	return 1;
}

int CHudMusic::MsgFunc_Spray( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
  char file[128] = "";
  //strncpy( file, READ_STRING( ), 128 );
  int type = READ_BYTE( );
  char gre[32];
  switch( (int)CVAR_GET_FLOAT( "cl_blood" ) )
  {
  case 0:
    strcpy( gre, "" );
    break;
  case 1:
    strcpy( gre, "_low" );
    break;
  case 2:
    strcpy( gre, "_med" );
    break;
  default:
    strcpy( gre, "_high" );
    break;
  }

  switch( type )
  {
  case SPRAY_BLOOD:
    sprintf( file, "partsys/blood%s.cfg", gre ); 
    break;
  case SPRAY_BLOODHEAD:
    sprintf( file, "partsys/bloodhead%s.cfg", gre ); 
    break;
  case SPRAY_BURN:
    strcpy( file, "partsys/burn.cfg" ); 
    break;
  case SPRAY_DUST:
    strcpy( file, "partsys/dust.cfg" ); 
    break;
  case SPRAY_TANK:
    strcpy( file, "partsys/tank.cfg" ); 
    break;
  /*case SPRAY_PLRBLOOD:
    sprintf( file, "partsys/plrblood.cfg", gre ); 
    break;
  case SPRAY_PLRBLOODBIG:
    sprintf( file, "partsys/plrbloodbig.cfg", gre ); 
    break;
  case SPRAY_PLRBLOODSTREAM:
    sprintf( file, "partsys/plrbloodstream.cfg", gre ); 
    break;*/
  default:
    return 1;
  }

  vec3_t org, dir;

  org.x = READ_COORD( );
	org.y = READ_COORD( );
	org.z = READ_COORD( );

	dir.x = READ_COORD( );
	dir.y = READ_COORD( );
	dir.z = READ_COORD( );

  int entidx = READ_COORD( );
 
  float life = READ_COORD( );
  
  int num = (int)READ_COORD( );


  cBlood::NewSpray( file, org, dir, entidx, life, num );
	return 1;
}


int CHudMusic::MsgFunc_SetVol( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_fVolume = READ_COORD( );

	if( m_fVolume > 1 )
	{
		if( m_fVolume <= 255 )
			return 1;
		else
			m_fVolume = 255;
		return 1;
	}
	if( m_fVolume < 0 )
		m_fVolume = 0;

	m_fVolume *= 255;
	return 1;
}


CHudMusic::~CHudMusic( )
{
	/*if( !m_hDll )
		return;

  if( gEngfuncs.pDemoAPI->IsPlayingback( ) )
    return;
	CloseFmod( );
	FreeLibrary( m_hDll );*/
}
