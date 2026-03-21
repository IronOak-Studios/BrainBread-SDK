/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//			
//  hud.h
//
// class CHud declaration
//
// CHud handles the message, calculation, and drawing the HUD
//


//#define RGB_YELLOWISH 0x00FF0000 //255,160,0
#define RGB_YELLOWISH 0x00BBBBBB //255,160,0
#define RGB_REDISH 0x00FF1010 //255,160,0
#define RGB_GREENISH 0x0000A000 //0,160,0

#include "wrect.h"
#include "cl_dll.h"
#include "ammo.h"
#include "windows.h"

#define DHN_DRAWZERO 1
#define DHN_2DIGITS  2
#define DHN_3DIGITS  4
#define MIN_ALPHA	 164	

#define DHN_PREZERO  8
#define DHN_4DIGITS  16
#define DHN_5DIGITS  32
#define DHN_6DIGITS  64

#define		HUDELEM_ACTIVE	1

/*extern void ZeroHelp( );
extern void Help( int type, char *file );
extern void DidHelp( int type );
extern bool NeedHelp( int type );*/

typedef struct {
	int x, y;
} POSITION;

enum 
{ 
#ifndef MAX_PLAYERS
  MAX_PLAYERS = 32,
#endif
	MAX_TEAMS = 64,
	MAX_TEAM_NAME = 16,
};

typedef struct {
	unsigned char r,g,b,a;
} RGBA;

typedef struct cvar_s cvar_t;


#define HUD_ACTIVE	1
#define HUD_INTERMISSION 2
#define HUD_NOGLOW 4

#define MAX_PLAYER_NAME_LENGTH		32

#define	MAX_MOTD_LENGTH				1536

//
//-----------------------------------------------------
//
class CHudBase
{
public:
	POSITION  m_pos;
	int   m_type;
	int	  m_iFlags; // active, moving, 
	virtual		~CHudBase() {}
	virtual int Init( void ) {return 0;}
	virtual int VidInit( void ) {return 0;}
	virtual int Draw(float flTime) {return 0;}
	virtual void Think(void) {return;}
	virtual void Reset(void) {return;}
	virtual void InitHUDData( void ) {}		// called every time a server is connected to

};

struct HUDLIST {
	CHudBase	*p;
	HUDLIST		*pNext;
};



//
//-----------------------------------------------------
//
#include "voice_status.h"
#include "hud_spectator.h"


//
//-----------------------------------------------------
//
class CHudAmmo: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void Think(void);
	void Reset(void);
	int DrawWList(float flTime);
	int MsgFunc_CurWeapon(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_WeaponList(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_AmmoX(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf );

	void SlotInput( int iSlot );
	void _cdecl UserCmd_Slot1( void );
	void _cdecl UserCmd_Slot2( void );
	void _cdecl UserCmd_Slot3( void );
	void _cdecl UserCmd_Slot4( void );
	void _cdecl UserCmd_Slot5( void );
	void _cdecl UserCmd_Slot6( void );
	void _cdecl UserCmd_Slot7( void );
	void _cdecl UserCmd_Slot8( void );
	void _cdecl UserCmd_Slot9( void );
	void _cdecl UserCmd_Slot10( void );
	void _cdecl UserCmd_Close( void );
	void _cdecl UserCmd_NextWeapon( void );
	void _cdecl UserCmd_PrevWeapon( void );

private:
	float m_fFade;
	RGBA  m_rgba;
	WEAPON *m_pWeapon;
	int	m_HUD_bucket0;
	int m_HUD_selection;
	int m_iAmmo;

};

//
//-----------------------------------------------------
//

class CHudAmmoSecondary: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void Reset( void );
	int Draw(float flTime);

	int MsgFunc_SecAmmoVal( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_SecAmmoIcon( const char *pszName, int iSize, void *pbuf );

private:
	enum {
		MAX_SEC_AMMO_VALUES = 4
	};

	int m_HUD_ammoicon; // sprite indices
	int m_iAmmoAmounts[MAX_SEC_AMMO_VALUES];
	float m_fFade;
};


#include "health.h"


#define FADE_TIME 100


//
//-----------------------------------------------------
//
class CHudGeiger: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Geiger(const char *pszName, int iSize, void *pbuf);
	
private:
	int m_iGeigerRange;

};

//
//-----------------------------------------------------
//
class CHudTrain: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Train(const char *pszName, int iSize, void *pbuf);

private:
	HSPRITE m_hSprite;
	int m_iPos;

};

//
//-----------------------------------------------------
//
// REMOVED: Vgui has replaced this.
//
/*
class CHudMOTD : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );

	int MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf );

protected:
	static int MOTD_DISPLAY_TIME;
	char m_szMOTD[ MAX_MOTD_LENGTH ];
	float m_flActiveRemaining;
	int m_iLines;
};
*/

//
//-----------------------------------------------------
//
class CHudStatusBar : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	void ParseStatusString( int line_num );

	int MsgFunc_StatusText( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_StatusValue( const char *pszName, int iSize, void *pbuf );

protected:
	enum { 
		MAX_STATUSTEXT_LENGTH = 128,
		MAX_STATUSBAR_VALUES = 8,
		MAX_STATUSBAR_LINES = 2,
	};

	char m_szStatusText[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];  // a text string describing how the status bar is to be drawn
	char m_szStatusBar[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];	// the constructed bar that is drawn
	int m_iStatusValues[MAX_STATUSBAR_VALUES];  // an array of values for use in the status bar

	int m_bReparseString; // set to TRUE whenever the m_szStatusBar needs to be recalculated

	// an array of colors...one color for each line
	float *m_pflNameColors[MAX_STATUSBAR_LINES];
};

//
//-----------------------------------------------------
//
// REMOVED: Vgui has replaced this.
//
/*
class CHudScoreboard: public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int DrawPlayers( int xoffset, float listslot, int nameoffset = 0, char *team = NULL ); // returns the ypos where it finishes drawing
	void UserCmd_ShowScores( void );
	void UserCmd_HideScores( void );
	int MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf );
	void DeathMsg( int killer, int victim );

	int m_iNumTeams;

	int m_iLastKilledBy;
	int m_fLastKillTime;
	int m_iPlayerNum;
	int m_iShowscoresHeld;

	void GetAllPlayersInfo( void );
private:
	struct cvar_s *cl_showpacketloss;

};
*/

struct extra_player_info_t 
{
	short frags;
	short deaths;
	short playerclass;
	short teamnumber;
	short level;
	char teamname[MAX_TEAM_NAME];
};

struct team_info_t 
{
	char name[MAX_TEAM_NAME];
	int points;
	short frags;
	short deaths;
	short ping;
	short packetloss;
	short ownteam;
	short players;
	short level;
	int already_drawn;
	int scores_overriden;
	int teamnumber;
};

extern hud_player_info_t	g_PlayerInfoList[MAX_PLAYERS+1];	   // player info from the engine
extern extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS+1];   // additional player info sent directly to the client dll
extern team_info_t			g_TeamInfo[MAX_TEAMS+1];
extern int					g_IsSpectator[MAX_PLAYERS+1];
extern int					g_IsSpecial[512];


//
//-----------------------------------------------------
//
class CHudDeathNotice : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf );

private:
	int m_HUD_d_skull;  // sprite index of skull icon
};

//
//-----------------------------------------------------
//
class CHudMenu : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	void Reset( void );
	int Draw( float flTime );
	int MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf );

	void SelectMenuItem( int menu_item );

	int m_fMenuDisplayed;
	int m_bitsValidSlots;
	float m_flShutoffTime;
	int m_fWaitingForMore;
};

//
//-----------------------------------------------------
//
class CHudSayText : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_SayText( const char *pszName, int iSize, void *pbuf );
	void SayTextPrint( const char *pszBuf, int iBufSize, int clientIndex = -1 );
	void EnsureTextFitsInOneLineAndWrapIfHaveTo( int line );
friend class CHudSpectator;

private:

	struct cvar_s *	m_HUD_saytext;
	struct cvar_s *	m_HUD_saytext_time;
};

//
//-----------------------------------------------------
//
class CHudBattery: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_Battery(const char *pszName,  int iSize, void *pbuf );
	int	  m_iBat;	
	
private:
	HSPRITE m_hSprite1;
	HSPRITE m_hSprite2;
	wrect_t *m_prc1;
	wrect_t *m_prc2;
	float m_fFade;
	int	  m_iHeight;		// width of the battery innards
};


//
//-----------------------------------------------------
//
class CHudFlashlight: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void Reset( void );
	int MsgFunc_Flashlight(const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_FlashBat(const char *pszName,  int iSize, void *pbuf );
	
private:
	HSPRITE m_hSprite1;
	HSPRITE m_hSprite2;
	HSPRITE m_hBeam;
	wrect_t *m_prc1;
	wrect_t *m_prc2;
	wrect_t *m_prcBeam;
	float m_flBat;	
	int	  m_iBat;	
	int	  m_fOn;
	float m_fFade;
	int	  m_iWidth;		// width of the battery innards
};

//
//-----------------------------------------------------
//
const int maxHUDMessages = 16;
struct message_parms_t
{
	client_textmessage_t	*pMessage;
	float	time;
	int x, y;
	int	totalWidth, totalHeight;
	int width;
	int lines;
	int lineLength;
	int length;
	int r, g, b;
	int text;
	int fadeBlend;
	float charTime;
	float fadeTime;
};

//
//-----------------------------------------------------
//

class CHudTextMessage: public CHudBase
{
public:
	int Init( void );
	static char *LocaliseTextString( const char *msg, char *dst_buffer, int buffer_size );
	static char *BufferedLocaliseTextString( const char *msg );
	char *LookupString( const char *msg_name, int *msg_dest = NULL );
	int MsgFunc_TextMsg(const char *pszName, int iSize, void *pbuf);
};

//
//-----------------------------------------------------
//

class CHudMessage: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_HudText(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_GameTitle(const char *pszName, int iSize, void *pbuf);

	float FadeBlend( float fadein, float fadeout, float hold, float localTime );
	int	XPosition( float x, int width, int lineWidth );
	int YPosition( float y, int height );

	void MessageAdd( const char *pName, float time );
	void MessageAdd(client_textmessage_t * newMessage );
	void MessageDrawScan( client_textmessage_t *pMessage, float time );
	void MessageScanStart( void );
	void MessageScanNextChar( void );
	void Reset( void );

private:
	client_textmessage_t		*m_pMessages[maxHUDMessages];
	float						m_startTime[maxHUDMessages];
	message_parms_t				m_parms;
	float						m_gameTitleTime;
	client_textmessage_t		*m_pGameTitle;

	int m_HUD_title_life;
	int m_HUD_title_half;
	bool m_bEndAfterMessage;
};

//
//-----------------------------------------------------
//
#define MAX_SPRITE_NAME_LENGTH	24

class CHudStatusIcons: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void Reset( void );
	int Draw(float flTime);
	int MsgFunc_StatusIcon(const char *pszName, int iSize, void *pbuf);

	enum { 
		MAX_ICONSPRITENAME_LENGTH = MAX_SPRITE_NAME_LENGTH,
		MAX_ICONSPRITES = 4,
	};

	
	//had to make these public so CHud could access them (to enable concussion icon)
	//could use a friend declaration instead...
	void EnableIcon( char *pszIconName, unsigned char red, unsigned char green, unsigned char blue );
	void DisableIcon( char *pszIconName );

private:

	typedef struct
	{
		char szSpriteName[MAX_ICONSPRITENAME_LENGTH];
		HSPRITE spr;
		wrect_t rc;
		unsigned char r, g, b;
	} icon_sprite_t;

	icon_sprite_t m_IconList[MAX_ICONSPRITES];

};

//
//-----------------------------------------------------
//

/*class CHudPETeam : public CHudBase
{
public:
	int Init( );
	int VidInit( );
	int Draw( float flTime );
	int MsgFunc_PETeam( const char *pszName, int iSize, void *pbuf );

	void SelectMenuItem( int menu_item );

	int m_iActive;
};*/


class CHudNotify : public CHudBase
{
public:
	~CHudNotify( );
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_AddNotify( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Notify( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Mates( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Plinfo( const char *pszName, int iSize, void *pbuf );
	//int MsgFunc_Special( const char *pszName, int iSize, void *pbuf );

public:
	int m_iActive, c[3], m_iMates, m_sprMate, m_i2M;
	float m_flDur1;
	float m_flDur2;
	char m_sText1[1024];
	char m_sText2[1024];
	char m_sText3[1024];
	char m_sAdditional[1024];
	
	int m_iHlth;
	int m_iArmo;
	int m_iTeam;
	int m_iSpecial;
	int m_iSpecialSprite;
	int m_iGamemode;
	char m_sName[128];
};

/*class CHudIntro : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_Intro( const char *pszName, int iSize, void *pbuf );

	int m_iActive;
public:
	int actualline;
	float m_flNextDraw;
	int m_iCurPos;
	int m_iXPos[4], m_iYPos[4];
	char text[512], text2[512], line[4][512];
	int r[4], g[4], b[4], a[6];
	char t[6][2];
};*/

/*class CHudBriefing : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_Briefing( const char *pszName, int iSize, void *pbuf );

	int m_iActive;
public:
	char m_sBriefing[2048];
};*/

class CHudCounter: public CHudBase
{
public:
 ~CHudCounter( );
 int Init( void );
 int VidInit( void );
 int Draw(float flTime);
 int MsgFunc_Counter( const char *pszName,int iSize, void *pbuf );
 int MsgFunc_SmallCnt( const char *pszName,int iSize, void *pbuf );
 int MsgFunc_CntDown( const char *pszName,int iSize, void *pbuf );

public:
 struct _sCounter
 {
	 float	fTotal;
	 float	fStart;
	 char	sName[128];
	 bool	iActive;
 } m_sCounters[10];
 int m_iNumCnts;
 float m_flRoundTime, m_fCntDown, m_fStep, m_fCntTotal;
 HSPRITE m_hSprite;
 int m_HUD_cross;
 int m_bDrawCntDown, m_bInitCntDown, m_iProgress;
 float m_fRStart, m_fGStart, m_fGStep;
};

/*class CHudShowMenuPE: public CHudBase
{
public:
	int Init( void );
	void Think( );
	void LoadData( int nr );
	void SaveData( int nr );
	int VidInit( void );
	int Draw(float flTime);
	int MsgFunc_ShowMenuPE(const char *pszName,int iSize, void *pbuf);
	//int MsgFunc_SendData(const char *pszName,int iSize, void *pbuf);
	void Update( int team, int points );
	void SelectMenuItem( int slot );
	void ResetAll( );
	void ShowMenuNr( );
	int IsAvailable( int slot );
	void CalcPoints( );
	void Color( );
	void ( F_API *WriteData )( char* sPath, int iSaveslot, int addons, int slot1, int slot2, int slot3, int slot4, int slot5 );
	int ( F_API *GetMP3s )( char* sPath, char* sFile );
	char* ( F_API *GetMP3Nr )( int nr );
	void ( F_API *ResetMP3s )( );

	HINSTANCE	m_hSaveDll;
	int	m_bDllLoaded;

	int m_iMenu, m_iCat, m_iActive, m_iPoints, m_iPointsMax;
	int m_hCSpr[20];
	struct
	{
		int team;
		int slot[7];
	} m_sInfo;

	int category, m_iEntryCount;
	char m_sMenu[2048];
};*/

/*class CHudNVG: public CHudBase
{
public:
    int Init( void );
    int VidInit( void );
    int Draw(float flTime);
    int MsgFunc_NVG(const char *pszName, int iSize, void *pbuf);
    int MsgFunc_NVGActive(const char *pszName, int iSize, void *pbuf);

public:
    HSPRITE m_hFlackern;

    int m_iNVG;
    int m_iOn;
    
};*/

class CHudLens : public CHudBase  
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void CheckFlares( );
	int MsgFunc_LensRef(const char *pszName,  int iSize, void *pbuf );

	vec3_t m_vecRefOrigin[24];
	vec3_t m_vecLOrigin[24];
	vec3_t dist[24];

	int m_iIndex;
	int m_iIndexRef;
	int m_iActive[24];
	float m_fRemove[24];
	float m_fActivate[24];

	int m_iRef[24];
	
	int m_hSpr[10];
};

class CHudMusic : public CHudBase  
{
public:
	~CHudMusic( );
	int Init( );
	int VidInit( );
	int Draw( float flTime );
	void Think( );
	int MsgFunc_PlayMusic( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Spray(const char *pszName, int iSize, void *pbuf );
	int MsgFunc_SetVol(const char *pszName, int iSize, void *pbuf );

	// FMOD Funcs
	// General
	/*signed char		( F_API	*InitFmod )( int mixrate, int maxsoftwarechannels, unsigned int flags );
	void			( F_API *CloseFmod )( );
	signed char		( F_API	*SetDriver )( int driver );
	signed char		( F_API *SetOutput )( int outputtype );
	signed char		( F_API *SetVolume )( int channel, int vol );
	// Info
	int				( F_API *GetLength )( FSOUND_STREAM *stream );
	int				( F_API *GetTime )( FSOUND_STREAM *stream );
	signed char		( F_API	*IsPlaying )( int channel );
	int				( F_API	*GetVolume )( int channel );
	// Stream
	FSOUND_STREAM*  ( F_API *OpenStream )( const char *filename, unsigned int mode, int offset, int memlength);
	signed char		( F_API *CloseStream )( FSOUND_STREAM *stream );
	int				( F_API	*PlayStream )( int channel, FSOUND_STREAM *stream );*/
	// ---
public:
	//HINSTANCE		m_hDll;
	//FSOUND_STREAM	*m_pSoundStream;

	int m_bInited;
	int m_bShouldPlay;
	int m_bGotCommand;
	int m_bPlaying;

	float m_fVolume;
	float m_fOldVolume;
};

struct s_cl_radarentry
{
	int entindex;
	vec3_t origin;
	int origin2d[2];
	int color;
	int lastcolor;

	float blinktime;

	cl_entity_s *pev;
	bool override;

	s_cl_radarentry *prev;
	s_cl_radarentry *next;
};

// + ste0
class CHudRadar : public CHudBase  
{
public:
	int Init( void );
	int CalcRadar( );
	int VidInit( void );
	int Draw(float flTime);
	void ClearRadar( void );
	s_cl_radarentry *Enable( int idx );
	void Disable( int idx );
	int MsgFunc_Radar(const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_RadarOff(const char *pszName,  int iSize, void *pbuf );

	//radar_ent_s rad_ent[50];
	
public:
	HSPRITE m_hSprite[15];



	wrect_t *m_prc1;
	wrect_t *m_prc2;

	s_cl_radarentry *fradar;
	s_cl_radarentry *lradar;
		
    int   m_fnrg;
	int	  m_fOn;
	float m_fFade;

	float m_fBlink;
	int	m_iBlink;

	int px;
	int py;
//	int	  m_iWidth;		// width of the battery innards

};

// - ste0
#include "pe_pointmgr.h"

class CHud
{
private:
	HUDLIST						*m_pHudList;
	HSPRITE						m_hsprLogo;
	int							m_iLogo;
	client_sprite_t				*m_pSpriteList;
	int							m_iSpriteCount;
	int							m_iSpriteCountAllRes;
	float						m_flMouseSensitivity;
	int							m_iConcussionEffect; 

public:

	HSPRITE						m_hsprCursor;
	float m_flTime;	   // the current client time
	float m_fOldTime;  // the time at which the HUD was last redrawn
	double m_flTimeDelta; // the difference between flTime and fOldTime
	Vector	m_vecOrigin;
	Vector	m_vecAngles;
	int		m_iKeyBits;
	int		m_iHideHUDDisplay;
	int		m_iFOV;
	int		m_Teamplay;
	int		m_iRes;
	cvar_t  *m_pCvarStealMouse;
	cvar_t	*m_pCvarDraw;

	int m_iFontHeight;
	int DrawHudNumber(int x, int y, int iFlags, int iNumber, int r, int g, int b );
	int DrawHudNumberLarge(int x, int y, int iFlags, int iNumber, int r, int g, int b );
	int DrawHudString(int x, int y, int iMaxX, char *szString, int r, int g, int b );
	int DrawHudStringReverse( int xpos, int ypos, int iMinX, char *szString, int r, int g, int b );
	int DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b );
	int GetNumWidth(int iNumber, int iFlags);

private:
	// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
	// freed in ~CHud()
	HSPRITE *m_rghSprites;	/*[HUD_SPRITE_COUNT]*/			// the sprites loaded from hud.txt
	wrect_t *m_rgrcRects;	/*[HUD_SPRITE_COUNT]*/
	char *m_rgszSpriteNames; /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/

	//struct cvar_s *default_fov;
public:
	HSPRITE GetSprite( int index ) 
	{
		return (index < 0) ? 0 : m_rghSprites[index];
	}

	wrect_t& GetSpriteRect( int index )
	{
		return m_rgrcRects[index];
	}

	
	int GetSpriteIndex( const char *SpriteName );	// gets a sprite index, for use in the m_rghSprites[] array

	CHudAmmo		m_Ammo;
	CHudHealth		m_Health;
	CHudSpectator		m_Spectator;
	CHudGeiger		m_Geiger;
	CHudBattery		m_Battery;
	CHudTrain		m_Train;
	CHudFlashlight	m_Flash;
	CHudMessage		m_Message;
	CHudStatusBar   m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText		m_SayText;
	CHudMenu		m_Menu;
	CHudAmmoSecondary	m_AmmoSecondary;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;
	CHudCounter		m_Counter;
//	CHudShowMenuPE	m_ShowMenuPE;
//	CHudIntro		m_Intro;
//	CHudBriefing	m_Briefing;
	CHudNotify		m_Notify;
//    CHudNVG			m_NVG;
    CHudLens		m_Lens;
	CHudRadar		m_Radar; // by ste0
	CHudMusic		m_Music;
//	CHudPETeam		m_PETeam;
	cPEPointMgr		m_PEPointMgr;

	void Init( void );
	void VidInit( void );
	void Think(void);
	int Redraw( float flTime, int intermission );
	int UpdateClientData( client_data_t *cdata, float time );

	CHud() : m_iSpriteCount(0), m_pHudList(NULL) {}  
	~CHud();			// destructor, frees allocated memory

	// user messages
	int _cdecl MsgFunc_Damage(const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_Logo(const char *pszName,  int iSize, void *pbuf);
	int _cdecl MsgFunc_ResetHUD(const char *pszName,  int iSize, void *pbuf);
	void _cdecl MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_SetFOV(const char *pszName,  int iSize, void *pbuf);
	int  _cdecl MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf );

	// Screen information
	SCREENINFO	m_scrinfo;

	int	m_iWeaponBits;
	int	m_fPlayerDead;
	int m_iIntermission;

	// sprite indexes
	int m_HUD_number_0;
	int m_HUD_number_0large;


	void AddHudElem(CHudBase *p);

	float GetSensitivity();

};

class TeamFortressViewport;

extern CHud gHUD;
extern TeamFortressViewport *gViewPort;

extern int g_iPlayerClass;
extern int g_iTeamNumber;
extern int g_iUser1;
extern int g_iUser2;
extern int g_iUser3;

