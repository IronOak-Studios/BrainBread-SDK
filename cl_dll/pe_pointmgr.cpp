#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "pe_helper.h"
#include "pe_notify.h"

DECLARE_MESSAGE(m_PEPointMgr, UpdPoints)
DECLARE_MESSAGE(m_PEPointMgr, Stats)

#define between( x, a, b ) ( ( (x) > (a) ) && ( (x) < (b) ) ) 
#define inside( x, y, a, b, c, d ) ( between(x,a,b) && between(y,c,d) ) 

extern int g_Mouse1r;
extern int g_MouseX;
extern int g_MouseY;

cPEFader *pointFader;
int lvl = 0;
int stats = 0;
int hp = 0;
int spd = 0;
int dmg = 0;
int rehuman = 0;
int spr[2] = { -1, -1 };

#define START_POINTS 0
#define MAX_POINTS 100

#define POINT_X_SIZE 5
#define POINT_Y_SIZE YRES(270)
#define POINT_X_POS ( ScreenWidth - POINT_X_SIZE - 5 )
#define POINT_Y_POS ( ( ScreenHeight - POINT_Y_SIZE ) / 2 )


cPEPointMgr::~cPEPointMgr( )
{
	if( pointFader ) 
		delete pointFader;

}

int cPEPointMgr::Init( )
{
	HOOK_MESSAGE(UpdPoints);
	HOOK_MESSAGE(Stats);

	m_iFlags |= HUD_ACTIVE;

	pointFader = new cPEFader( );
	gHUD.AddHudElem( this );

	points = START_POINTS;
	return TRUE;
}

int done_selection = 0;

int cPEPointMgr::VidInit( )
{
	pointFader->Start( "good", 0, 100 );
  lvl = 0;
  stats = 0;
  hp = 0;
  spd = 0;
  dmg = 0;

  spr[0] = LoadSprite( "sprites/hud/hud3.spr" );
  spr[1] = LoadSprite( "sprites/hud/spectate.spr" );
  rehuman = 0;
  done_selection = 0;
  return TRUE;
}

extern float g_fUser4;
int cPEPointMgr::Draw( float time )
{
	//if( ( gHUD.m_iHideHUDDisplay & HIDEHUD_COUNTER ) )
	//	return 1;
	//if( g_iUser1 )
	//	return 1;
  /*if( g_fUser4 )
  {
     ClearBits( pev->button, IN_DUCK );
     ClearBits( pev->flags, FL_DUCKING );
     pev->view_ofs[2] = 21;
  }*/

  g_font->SetFont( FONT_WEAPONMENU );

  int xpos = XRES(320) - 104;
  int ypos = ScreenHeight - 74;
  rect_s rect;
  int xreh = ScreenWidth - 167;
  int yreh = 0;
  int ydone = ( g_iUser1 ? XRES(32) : 0 );

  // rehuman button
  if( rehuman )
  {
	  rect.left = 24;
	  rect.right = 191;
    rect.top = 37;
    rect.bottom = 98;

	  SPR_Set( spr[0], 255, 255, 255 );
	  SPR_DrawHoles( 0, xreh, yreh, &rect );
  }

  if( done_selection == 1 )
  {
	  rect.left = 40;
	  rect.right = 207;
	rect.top = 97;
	rect.bottom = 158;

	  SPR_Set( spr[1], 255, 255, 255 );
	  SPR_DrawHoles( 0, xreh, ydone, &rect );
  }
  else if( done_selection == 2 )
  {
	  rect.left = 40;
	  rect.right = 207;
	rect.top = 177;
	rect.bottom = 238;

	  SPR_Set( spr[1], 255, 255, 255 );
	  SPR_DrawHoles( 0, xreh, ydone, &rect );
  }  
  
  // Stats anzeige
	rect.left = 24;
	rect.right = 233;
	if( !stats )
    rect.top = 104;
  else
    rect.top = 182;
  rect.bottom = rect.top + 74;

	SPR_Set( spr[0], 255, 255, 255 );
	SPR_DrawHoles( 0, xpos, ypos, &rect );

  // EXP balken
  //SPR_EnableScissor( xpos + 14, ypos + ( 74 - 28 ), (int)( points / 100 * 172 ), 28 );
	rect.left = 38;
	rect.right = rect.left + (int)( points / 100 * 172 );	
  rect.top = 7;
  rect.bottom = rect.top + 14;
  if( rect.left < rect.right )
  {
	  SPR_Set( spr[0], 255, 255, 255 );
	  SPR_DrawHoles( 0, xpos + 14, ypos + 54, &rect );
  }

  //SPR_DisableScissor( );

  // Text
  char text[512];
  //sprintf( text, "lvl %d, stat points %d, hp %f, speed %f%%, dmg %f%%", lvl, stats, hp, spd, dmg );
  
  //sprintf( text, "%d", lvl );  
  //g_font->DrawString( xpos +  58, ypos + 13, text, 140, 140, 140 );
  gHUD.DrawHudNumber( 61, ScreenHeight - 53, DHN_DRAWZERO | DHN_3DIGITS, lvl, 255, 255, 255);
  sprintf( text, "%d", stats );  
  g_font->DrawString( xpos + 150, ypos + 27, text, 140, 140, 140 );
  sprintf( text, "%d", hp );  
  g_font->DrawString( xpos +  33, ypos + 27, text, 140, 140, 140 );
  sprintf( text, "%d%", spd );  
  g_font->DrawString( xpos +  70, ypos + 27, text, 140, 140, 140 );
  sprintf( text, "%d", dmg );  
  g_font->DrawString( xpos + 111, ypos + 27, text, 140, 140, 140 );

  if( g_Mouse1r )
  {
    if( between( g_MouseY, ypos + 20, ypos + 44 ) )
    {
      if(      between( g_MouseX, xpos +  28, xpos +  63 ) )
		  	  ClientCmd( "hpup" );
      else if( between( g_MouseX, xpos +  67, xpos + 102 ) )
		  	  ClientCmd( "spdup" );
      else if( between( g_MouseX, xpos + 107, xpos + 142 ) )
		  	  ClientCmd( "dmgup" );
      g_Mouse1r = FALSE;
    }
    else if( rehuman && between( g_MouseY, yreh, yreh + 61 ) )
    {
      if( between( g_MouseX, xreh, xreh + 167 ) )
      {
        strcpy( gHUD.m_Notify.m_sText1, "BRAAIINNSSS!!! (respawning as human after death)" );
    	  ClientCmd( "rehuman" );
        rehuman = 0;
      }
      g_Mouse1r = FALSE;
    }
    else if( done_selection && between( g_MouseY, ydone, ydone + 61 ) )
    {
      if( between( g_MouseX, xreh, xreh + 167 ) )
      {
		  ClientCmd( ( done_selection == 1 ? "dzombie" : "dspectate" ) );
        done_selection = 0;
      }
      g_Mouse1r = FALSE;
    }
  }

	return TRUE;
}

int cPEPointMgr::MsgFunc_Stats( const char *pszName,int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
  hp = READ_BYTE( );
  spd = READ_BYTE( );
  dmg = READ_BYTE( );
	stats = READ_BYTE( );
	return TRUE;
}

int cPEPointMgr::MsgFunc_UpdPoints( const char *pszName,int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
  float pnts = READ_COORD( );
	if( pnts <= 100 )
    points = pnts;
	lastAdd = READ_BYTE( );
	stats = READ_BYTE( );
	if( lastAdd < 1 )
		return TRUE;
  else
    lvl = lastAdd;
	s_helpitem *msg = g_helper->ShowHelp( HELP_ALWAYS, "help/level.cfg" );
	if( !msg )
		return TRUE;
	g_font->SetFont( msg->font );
	msg->max[0] -= g_font->GetWidthML( msg->text );
	if( strstr( msg->text, "%d" ) && ( strchr( msg->text, '%' ) == strrchr( msg->text, '%' ) ) )
	{
		char txt[MAX_HELPTEXT_LEN];
		strcpy( txt, msg->text );
		sprintf( msg->text, txt, lastAdd );
	}
	else
		_snprintf( msg->text, MAX_HELPTEXT_LEN, "You reached level %d!", lastAdd );
	
	char *plur;
	if( lastAdd == 1 && ( plur = strstr( msg->text, "points!" ) ) )
	{
		plur[5] = '!';
		plur[6] = '\0';
	}
	msg->max[0] += g_font->GetWidthML( msg->text );
	// yawn, me lazy
	msg = g_helper->ShowHelp( HELP_ALWAYS, "help/bonus.cfg" );
	if( !msg )
		return TRUE;
	g_font->SetFont( msg->font );
	msg->max[0] -= g_font->GetWidthML( msg->text );
	if( strstr( msg->text, "%d" ) && ( strchr( msg->text, '%' ) == strrchr( msg->text, '%' ) ) )
	{
		char txt[MAX_HELPTEXT_LEN];
		strcpy( txt, msg->text );
		sprintf( msg->text, txt, 1 );
	}
	else
		_snprintf( msg->text, MAX_HELPTEXT_LEN, "You received %d spec points!", 1 );
	
	if( 1 == 1 && ( plur = strstr( msg->text, "points!" ) ) )
	{
		plur[5] = '!';
		plur[6] = '\0';
	}
	msg->max[0] += g_font->GetWidthML( msg->text );

  g_helper->ShowHelp( HELP_STATSPOINT, "help/point_stats.cfg" );
	return TRUE;
}
