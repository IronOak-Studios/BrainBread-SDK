#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include <stdio.h>
#include "vgui_TeamFortressViewport.h"
#include "vgui_spectatorpanel.h"
#include "demo_api.h"

#include "pe_helper.h"

DECLARE_MESSAGE( m_Intro, Intro ) 
extern int CL_ButtonBits( int bResetState );

#define MAX_MENU_STRING	512
char introtext[MAX_MENU_STRING];

int CHudIntro::Init(void)
{
	HOOK_MESSAGE( Intro ); 
	CVAR_CREATE( "cl_showintro", "1", FCVAR_ARCHIVE );

	gHUD.AddHudElem(this);
	return 1; 
};

int CHudIntro::VidInit(void)
{
	m_iFlags &= ~HUD_ACTIVE;
	m_iCurPos = -1;
	m_iActive = 1; 
	strcpy( line[0], "IronOak Studios Presents:" );
	strcpy( line[1], "-P-U-B-L-I-C-     -E-N-E-M-Y-" );
	strcpy( line[2], "The Dark Future" );
	strcpy( line[3], "A Half-Life Modification" );
	g_font->SetFont( FONT_INTRO );
	m_iXPos[0] = XRES( 320 ) - ( g_font->GetWidth( line[0] ) / 2 );
	m_iXPos[1] = XRES( 320 ) - ( g_font->GetWidth( line[1] ) / 2 );
	m_iXPos[2] = XRES( 320 ) - ( g_font->GetWidth( line[2] ) / 2 );
	m_iXPos[3] = XRES( 320 ) - ( g_font->GetWidth( line[3] ) / 2 );
	m_iYPos[0] = YRES( 200 ) - 36;
	m_iYPos[1] = YRES( 200 ) - 36 + 24;
	m_iYPos[2] = YRES( 200 ) - 36 + 36 + 2;
	m_iYPos[3] = YRES( 200 ) - 36 + 72 + 2;
	r[0] = 255;
	r[1] = 255;
	r[2] = 80;
	r[3] = 128;
	g[0] = 255;
	g[1] = 0;
	g[2] = 80;
	g[3] = 128;
	b[0] = 255;
	b[1] = 0;
	b[2] = 255;
	b[3] = 128;

	a[5] = 40;
	a[4] = 70;
	a[3] = 100;
	a[2] = 140;
	a[1] = 180;
	a[0] = 220;
	memset( text, 0, 512 );
	memset( text2, 0, 512 );
	memset( t, 0, 6 * 2 );

	m_flNextDraw = 0;
	actualline = 0;
	return 1;
};

int CHudIntro::MsgFunc_Intro( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_iActive = READ_BYTE( );

	if( m_iActive )
		m_iFlags |= HUD_ACTIVE;
	else
		m_iFlags &= ~HUD_ACTIVE;

	if( m_iActive )
	{
		char text[512];
		snprintf( text, sizeof(text), "hlp %d", ( CVAR_GET_FLOAT( "cl_help" ) ? 1 : 0 ) );
		ClientCmd( text );
		
		g_helper->ShowHelp( HELP_INTRO, "help/intro.cfg" );
	}
	return 1;
}

int CHudIntro::Draw( float flTime )
{
	if( !m_iActive )
		return 1;

	if ( gEngfuncs.pDemoAPI->IsPlayingback( ) )
		return 1;

	int xt = 0, i = 0;
	a[5] = 40;
	a[4] = 70;
	a[3] = 100;
	a[2] = 140;
	a[1] = 180;
	a[0] = 220;
	g_font->SetFont( FONT_INTRO );
	
	if( gViewPort )
		gViewPort->m_pSpectatorPanel->setVisible( false );
	if( !CVAR_GET_FLOAT( "cl_showintro" ) || ( CL_ButtonBits( 0 ) & IN_ATTACK && m_iActive == 1 ) || ( m_iActive == 2 && m_flNextDraw <= flTime ) )
	{
		CL_ButtonBits( 1 );
		g_helper->RemoveAll( );
		m_iFlags &= ~HUD_ACTIVE;
		ClientCmd( "menuselect 2\n" );
		m_iActive = 0;
		if( gViewPort )
			gViewPort->m_pSpectatorPanel->setVisible( true );
		return 1;
	}

	if( m_iCurPos == -1 )
	{
		m_flNextDraw = flTime + 0.75;
		m_iCurPos++;
	}

	if( m_flNextDraw <= flTime )
	{
		if( m_iCurPos < (int)strlen( line[actualline] ) + 5 )
			m_iCurPos++;
		else
		{
			if( actualline < 3 )
			{
				actualline++;
				m_iCurPos = 0;
			}
			else
			{
				m_iActive = 2;
			}
		}

		
		for( i = 0; i <= (m_iCurPos - 6); i++ )
		{
			text[i] = line[actualline][i];
		}
		text[i] = '\0';
		int linelen = strlen( line[actualline] );
		for( int o = 0; o <= 5; o++ )
		{
			int idx = m_iCurPos - 5 + o;
			if( idx >= 0 && idx < linelen )
				t[o][0] = line[actualline][idx];
			else
				t[o][0] = '\0';
			t[o][1] = '\0';
		}

		m_flNextDraw = flTime + 0.05;
		if( m_iActive == 2 )
			m_flNextDraw = flTime + 2.0;

	}
	if( actualline > 0 )
		g_font->DrawString( m_iXPos[0], m_iYPos[0], line[0], r[0], g[0], b[0] );
	if( actualline > 1 )
		g_font->DrawString( m_iXPos[1], m_iYPos[1], line[1], r[1], g[1], b[1] );
	if( actualline > 2 )
		g_font->DrawString( m_iXPos[2], m_iYPos[2], line[2], r[2], g[2], b[2] );
		
	xt = g_font->DrawString( m_iXPos[actualline], m_iYPos[actualline], text, r[actualline], g[actualline], b[actualline] );
	
	int rt = 0, gt = 0, bt = 0, temp;
	for( int o = 0; o <= 5; o++ )
	{
		rt = 0; gt = 255; bt = 0;
		if( ( m_iCurPos - o ) >= 0 )
		{
			g_font->DrawString( xt, m_iYPos[actualline], t[o], rt, gt, bt, a[o] );
			temp = g_font->GetWidth( t[o] );
			xt += g_font->GetWidth( t[o] );
		}
	}

	return 1;
}
