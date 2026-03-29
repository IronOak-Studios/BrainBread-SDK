#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include <stdio.h>
#include "vgui_TeamFortressViewport.h"
#include "pe_font.h"

DECLARE_MESSAGE( m_Briefing, Briefing ) 
extern int CL_ButtonBits( int bResetState );

// Copied parts of the HL code here

int CHudBriefing::Init(void)
{
	HOOK_MESSAGE( Briefing ); 

	gHUD.AddHudElem(this);
	return 1; 
};

int CHudBriefing::VidInit(void)
{
	m_iFlags &= ~HUD_ACTIVE;
	m_iActive = 0; 
	return 1;
};

int CHudBriefing::MsgFunc_Briefing( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int temp = READ_BYTE( );

	if( temp == 2 )
		m_iActive = !m_iActive;
	else
		m_iActive = temp;

	if( m_iActive )
		m_iFlags |= HUD_ACTIVE;
	else
	{
		m_iFlags &= ~HUD_ACTIVE;
		return 1;
	}
	char level[128];
	strcpy( level , gEngfuncs.pfnGetLevelName( ) );
	char sz[256]; 
	char szTitle[256]; 
	char *ch;
	if (level && level[0])
	{
		strcpy( sz, level );
		ch = strchr( sz, '/' );
		if (!ch)
			ch = strchr( sz, '\\' );
		strcpy( szTitle, ch+1 );
		ch = strchr( szTitle, '.' );
		*ch = '\0';
		*ch = '.';
		strcpy( sz, level );
		ch = strchr( sz, '.' );
		*ch = '\0';
		strcat( sz, ".txt" );
		ch = (char*)gEngfuncs.COM_LoadFile( sz, 5, NULL );
		if( ch )
		{
			if( strlen( ch ) < 2048 )
				strcpy( m_sBriefing, ch );
			else
				strcpy( m_sBriefing, "Map briefing file is too large!");
			gEngfuncs.COM_FreeFile( ch );
		}
		else
			strcpy( m_sBriefing, "No map briefing available...");
	}
	return 1;
}

int CHudBriefing::Draw( float flTime )
{
	if( !m_iActive )
		return 1;
	
	if ( gViewPort && gViewPort->IsScoreBoardVisible() )
		return 1;
	
	if ( gViewPort && gViewPort->m_pCurrentMenu && gViewPort->m_pCurrentMenu->IsActive( ) )
		return 1;

/*	if( gHUD.m_ShowMenuPE.m_iActive )
		return 1;*/

	int x = 0, o, i = 0, y = YRES( 120 );
	char text[1024];
	g_font->SetFont( FONT_BRIEFING );

	if( ( CL_ButtonBits( 0 ) & IN_ATTACK ) && !gHUD.m_PETeam.m_iActive )
	{
		CL_ButtonBits( 1 );
		m_iFlags &= ~HUD_ACTIVE;
		m_iActive = 0;
		return 1;
	}

	while ( i < 1024 && m_sBriefing[i] != '\0' )
	{
		for( o = i; ( m_sBriefing[o] != '\n' ) && ( o < 2048 ) && ( m_sBriefing[o] != '\0' ); o++ ){}

		if( o-i > 0 )
		{
			strncpy( text, m_sBriefing+i, o-i );
			text[o-i] = '\0';

			if( i == 0 )
				g_font->DrawString( ( ScreenWidth / 2 + XRES(140) ) - ( g_font->GetWidth( text ) / 2 ), y, text, 255, 0, 0 );
			else
				g_font->DrawString( ( ScreenWidth / 2 + XRES(140) ) - ( g_font->GetWidth( text ) / 2 ), y, text, 255, 255, 255 );
		}
		y += 12;

		while ( i < 1024 && m_sBriefing[i] != '\0' && m_sBriefing[i] != '\n' )
			i++;
		if ( m_sBriefing[i] == '\n' )
			i++;
	}
	return 1;

}
