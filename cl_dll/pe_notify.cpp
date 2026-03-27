#include "hud.h"
#include "cl_util.h"
#include "hud_scale.h"
#include "parsemsg.h"
#include <string.h>
#include <stdio.h>
#include "vgui_TeamFortressViewport.h"
#include "pe_notify.h"
#include "pe_helper.h"

DECLARE_MESSAGE( m_Notify, Notify ) 
DECLARE_MESSAGE( m_Notify, AddNotify ) 
DECLARE_MESSAGE( m_Notify, Mates ) 
//DECLARE_MESSAGE( m_Notify, Plinfo ) 

extern int CL_ButtonBits( int bResetState );

extern int g_sprHUD2;
cPEFader *notifyFader;
cPEFader *pulseFader;

CHudNotify::~CHudNotify( )
{
	if( pulseFader )
		delete pulseFader;
	if( notifyFader )
		delete notifyFader;
}

int CHudNotify::Init(void)
{
	HOOK_MESSAGE( Notify );
	HOOK_MESSAGE( AddNotify );
	HOOK_MESSAGE( Mates );
//	HOOK_MESSAGE( Plinfo );
	//HOOK_MESSAGE( Special );

	strcpy( m_sText1, "" );
	strcpy( m_sText2, "" );
	strcpy( m_sText3, "" );
	m_flDur1 = 0;
	m_flDur2 = 0;

	notifyFader = new cPEFader( );
	pulseFader = new cPEFader( );
	
	gHUD.AddHudElem(this);
	return 1; 
};

int CHudNotify::VidInit(void)
{
	m_iFlags &= ~HUD_ACTIVE;
	m_flDur1 = 0;
	m_flDur2 = 0;

	m_iSpecialSprite = gHUD.GetSpriteIndex( "htool" );
	m_sprMate = gHUD.GetSpriteIndex( "mate" );
	m_iSpecial = 0;
	pulseFader->Start(CVAR_GET_FLOAT("cl_newfont") ? "pulse_solid" : "pulse", 0, 1);

	for( int i = 0; i < ( MAX_PLAYERS + 1 ); i++ )
		g_IsSpecial[i] = 0;

	return 1;
};

int CHudNotify::MsgFunc_Mates( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iMates = READ_BYTE( );
	return 1;
}

int CHudNotify::MsgFunc_Plinfo( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iHlth = READ_BYTE( );
	m_iArmo = READ_BYTE( );
	m_iTeam = READ_BYTE( );
	strcpy( m_sName, READ_STRING( ) );
	return 1;
}

int CHudNotify::MsgFunc_AddNotify( const char *pszName, int iSize, void *pbuf )
{
	char *add;
	BEGIN_READ( pbuf, iSize );

	add = READ_STRING( );
	strcpy( m_sAdditional, add );

	return 1;
}

bool dur1 = false;
bool dur2 = false;
extern int rehuman;
extern int done_selection;

int CHudNotify::MsgFunc_Notify( const char *pszName, int iSize, void *pbuf )
{
  int string = 0;
  float dr;
	BEGIN_READ( pbuf, iSize );


	dr = READ_COORD( );
	string = READ_BYTE( );


	if( string < 0x0060 )
  {
		m_flDur1 = dr;
    dur1 = true;
  }
  else if( string >= 0x0080 )
  {
    m_flDur2 = -1;
    dur2 = true;
  }

	if( dr == 0 && string < 0x0080 )
		m_iFlags &= ~HUD_ACTIVE;
	else
		m_iFlags |= HUD_ACTIVE;

	switch( string )
	{
	case NTM_HUMAN_ESC:
		strcpy( m_sText2, "The Humans escaped" );
		strcpy( m_sText3, "THE HUMANS WIN" );
		break;
	case NTM_TIMEUP:
		strcpy( m_sText2, "The Humans failed to escape" );
		strcpy( m_sText3, "THE ZOMBIES WIN" );
		break;
	/*case NTM_HACKED1:
		sprintf( m_sText2, "The modem has been installed on the %s terminal!", m_sAdditional );
		strcpy( m_sText3, "It will be hacked in 30 seconds" );
		if( g_iTeamNumber == 1 )
			g_helper->ShowHelp( HELP_HACKED_SEC, "help/hacked_sec.cfg" );
		else
			g_helper->ShowHelp( HELP_HACKED_SYN, "help/hacked_syn.cfg" );
		break;*/
	case NTM_ALLDEAD:
		strcpy( m_sText2, "No more BRAINS..." );
		strcpy( m_sText3, "ROUND DRAW" );
		break;
	case NTM_DRAW:
		strcpy( m_sText2, "No more BRAINS..." );
		strcpy( m_sText3, "ROUND DRAW" );
		break;
	case NTM_ZOMBIE:
		strcpy( m_sText2, "You are a ZOMBIE" );
		strcpy( m_sText3, "Eat BRAINS!! B-R-A-I-N-S!!!");
		g_helper->ShowHelp( HELP_ZOMBIE_SPAWN, "help/task_zombie.cfg" );
		break;
	case NTM_HUMAN:
		strcpy( m_sText2, "You are not yet undead" );
		strcpy( m_sText3, "Try to keep it that way and complete the missions!");
		g_helper->ShowHelp( HELP_HUMAN_SPAWN, "help/task_human.cfg" );
		break;
	case NTC_ZOMBIE:
    if( dr > 0 )
  	  sprintf( m_sText1, "BRAAIINNSSS!! %d more points needed to respawn as human.", (int)dr );
    else if( !rehuman )
  	  strcpy( m_sText1, "BRAAIINNSSS!!! (respawning as human after death)" );
    else
      strcpy( m_sText1, "BRAAIINNSSS!!!" );
		c[0] = 120;
		c[1] = 120;
		c[2] = 255;
		break;
	case NTC_HUMAN:
		strcpy( m_sText1, "Escape from this cursed town!");
		c[0] = 255;
		c[1] = 80;
		c[2] = 80;
		break;
  case NTM_MISSION_RESCUE:
		strcpy( m_sText2, "Use the the radio to order the helicopter" );
		strcpy( m_sText3, "and then try to reach the landing zone.");
		g_helper->ShowHelp( HELP_HUMAN_RESCUE, "help/task_rescue.cfg" );
		break;
	case NTC_MISSION_RESCUE:
		strcpy( m_sText1, "Order the helicopter and wait for it at the escape zone!");
		c[0] = 255;
		c[1] = 80;
		c[2] = 80;
		break;
  case NTM_MISSION_FRAGS:
		strcpy( m_sText2, "Kill the requested amount of zombies" );
		strcpy( m_sText3, "in order to get support of the military");
		g_helper->ShowHelp( HELP_HUMAN_FRAG, "help/task_frag.cfg" );
		break;
	case NTC_MISSION_FRAGS:
		sprintf( m_sText1, "Kill at least %d more zombies!", (int)dr );
		c[0] = 255;
		c[1] = 80;
		c[2] = 80;
		break;
  case NTM_MISSION_FRED:
		strcpy( m_sText2, "Search and kill FRED the mutated zombie" );
		strcpy( m_sText3, "the military needs his DNA");
		g_helper->ShowHelp( HELP_HUMAN_FRED, "help/task_fred.cfg" );
		break;
	case NTC_MISSION_FRED:
		strcpy( m_sText1, "Search and kill FRED the mutated zombie!");
		c[0] = 255;
		c[1] = 80;
		c[2] = 80;
		break;
  case NTM_MISSION_OBJECT:
		strcpy( m_sText2, "Find the case and bring it to a" );
		strcpy( m_sText3, "mission zone.");
		g_helper->ShowHelp( HELP_HUMAN_OBJECT, "help/task_object.cfg" );
		break;
	case NTC_MISSION_OBJECT:
		strcpy( m_sText1, "Find the CASE and bring it to a mission zone!");
		c[0] = 255;
		c[1] = 80;
		c[2] = 80;
		break;
  case NTM_OBJECTRESTORE:
		sprintf( m_sText2, "The case fell into an unreachable position" );
		strcpy( m_sText3, "It will be restored at a random spot" );
		break;
  case NTM_REHUMAN:
		sprintf( m_sText2, "You can now select to respawn as" );
		strcpy( m_sText3, "a human after dieing." );
    rehuman = 1;
		break;
  case NTM_REHUMAN_HIDE:
    rehuman = 0;
		break;
  case NTM_DONE_HIDE:
		done_selection = 0;
		break;
  case NTM_DONE_ZOMBIE:
		sprintf( m_sText2, "You can now select to spawn in as" );
		strcpy( m_sText3, "a zombie for the rest of the round." );
		done_selection = 1;
		break;
  case NTM_DONE_SPECTATE:
		sprintf( m_sText2, "You can now select to spectate" );
		strcpy( m_sText3, "for the rest of the round." );
		done_selection = 2;
		break;
  case NTC_CUSTOM:
		strcpy( m_sText1, READ_STRING( ) );
    c[0] = 255;
		c[1] = 80;
		c[2] = 80;
		break;
  case NTM_CUSTOM:
		strcpy( m_sText2, READ_STRING( ) );
		strcpy( m_sText3, READ_STRING( ) );
		break;

	}
  m_flDur1 = 0;
  if( strlen( m_sText1 ) > 5 && dur1 )
    m_flDur1 += 5;
  else
    m_flDur1 += 2;
  if( strlen( m_sText2 ) > 5 && dur2 )
    m_flDur1 += 5;
  else
    m_flDur1 += 2;
	return 1;
}

int CHudNotify::Draw( float flTime )
{
	char text[128];
	if( dur1 || dur2 )
	{
		if( dur1 )
    {
			m_flDur1 = flTime + ( m_flDur1 == -1 ? 999999 : m_flDur1 );
   		notifyFader->Start( CVAR_GET_FLOAT("cl_newfont") ? "mid_solid" : "mid", flTime, m_flDur1 );
    }
		if( dur2 )
      m_flDur2 = flTime + ( m_flDur2 == -1 ? 999999 : m_flDur2 );
		dur1 = false;
    dur2 = false;
	}
	/*if( m_iActive == 255 )
	{
		if( m_flDur1 == -1 )
			m_flDur1 = flTime + 9999999;
		if( m_flDur2 == -1 )
			m_flDur2 = flTime + 9999999;
		m_iActive = 0;
	}*/
	int x = 0, y = ( ScreenHeight / 4 ) - 6;
	int r, g, b, a;

	/*if( gHUD.m_ShowMenuPE.m_iActive )
		return 1;*/

	if( m_iMates )
	{
		//SPR_Set( gHUD.GetSprite( m_sprMate ), 128, 128, 128);
		/*for( int i = 1; i <= m_iMates; i++ )
		{
			SPR_DrawAdditive( 0, ScreenWidth - ( i * 32 ) - 45, 2, &gHUD.GetSpriteRect( m_sprMate ) );
		}*/
		//SPR_DrawHoles( 0, ScreenWidth - 70, 2, &gHUD.GetSpriteRect( m_sprMate ) );
		sprintf( text, "TeamBonus x %d", m_iMates );
		g_font->SetFont( FONT_NOTIFY );
    g_font->DrawString( ScreenWidth - ( g_font->GetWidth( text ) + HudScale( 10 ) ), HudScale( 20 ), text, 0, 255.0f, 0, (int)( 80 + 175.0f / 16.0f * m_iMates ) ); 

	}
	if( g_IsSpecial[gEngfuncs.GetLocalPlayer( )->index] == 1 )
	{
		rect_s rect;
		rect.bottom = 181;
		rect.top = 134;
		rect.right = 65;
		rect.left = 0;
		ScaledSPR_DrawHoles( g_sprHUD2, 0, HudScale( 10 ), ( (float)ScreenHeight / 2.0 ) - ( HudScaleF( (float)( rect.bottom - rect.top ) ) / 2.0 ), &rect, 255, 255, 255 );
	}

	g_font->SetFont( FONT_NOTIFY );
	if( m_flDur2 > flTime )
	{
		pulseFader->GetColor( flTime, r, g, b, a );
		g_font->DrawString( HudScale( 35 ), HudScale( 2 ), m_sText1, c[0], c[1], c[2], a );
	}
	g_font->SetFont( FONT_NOTIFY_MID );

	int col[3] = { 0, 0, 0 };

	if( m_flDur1 > flTime )
	{
		notifyFader->GetColor( flTime, col[0], col[1], col[2] );
		x = ( ScreenWidth / 2 ) - ( g_font->GetWidth( m_sText2 ) / 2 );
		g_font->DrawString( x, y, m_sText2, col[0], col[1], col[2] );
		y += g_font->GetHeight( );
		x = ( ScreenWidth / 2 ) - ( g_font->GetWidth( m_sText3 ) / 2 );
		g_font->DrawString( x, y, m_sText3, col[0], col[1], col[2] );
	}

	if( m_iTeam && ( ( g_iUser1 == 3 ) || !g_iUser1 ) )
	{
		if( g_iTeamNumber == m_iTeam )
		{
			col[0] = 0;
			col[1] = 255;
		}
		else
		{
			col[0] = 255;
			col[1] = 0;
		}
		col[2] = 0;
		int ym = 0, xm = HudScale( 150 );
		if( g_iUser1 == 3 )
		{
			xm = HudScale( 15 );
			ym = YRES(60);
		}
		
		sprintf( text, "%s(%s): %d Health, %d Armor", m_sName, ( m_iTeam == 1 ) ? "alive" : "ZOMBIE", m_iHlth, m_iArmo );
		g_font->DrawString( xm, ScreenHeight - ym - HudScale( 20 ), text, col[0], col[1], col[2], 128 );
	}
	return TRUE;
}