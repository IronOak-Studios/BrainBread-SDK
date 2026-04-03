#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "vgui_TeamFortressViewport.h"
#include "pe_helper.h"

#define between( x, a, b ) ( ( (x) > (a) ) && ( (x) < (b) ) ) 
#define inside( x, y, a, b, c, d ) ( between(x,a,b) && between(y,c,d) ) 

extern int g_Mouse1r;
extern int g_MouseX;
extern int g_MouseY;

int sprTeam1 = 0;
int sprTeam2 = 0;
int sprTeam3 = 0;
int sprTeam4 = 0;

struct s_items
{
	rect_s area;
	int	slot;
};

s_items g_Items[10] = 
{
	{ { 164, 327,  95, 242 }, 1 },
	{ {  34, 197, 253, 400 }, 2 },
	{ { 198, 289, 451, 478 }, 3 },
	{ {  54, 109, 439, 492 }, 4 },
	{ { 112, 189, 412, 444 }, 9 },
	{ { 0, 0, 0, 0 }, -1 }
};

DECLARE_MESSAGE( m_PETeam, PETeam );

int CHudPETeam::Init( )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( PETeam );
	return 1;
}


int CHudPETeam::VidInit( )
{
	m_iActive = 0;
	m_iFlags &= ~HUD_ACTIVE;

	sprTeam1 = LoadSprite( "sprites/team/team1.spr" );
	sprTeam2 = LoadSprite( "sprites/team/team2.spr" );
	sprTeam3 = LoadSprite( "sprites/team/team3.spr" );
	sprTeam4 = LoadSprite( "sprites/team/team4.spr" );

	return 1;
}

int CHudPETeam::Draw( float flTime )
{
	if( gViewPort )
		gViewPort->UpdateCursorState( );

	if ( gViewPort && gViewPort->IsScoreBoardVisible() )
		return 1;
	
	if ( gViewPort && gViewPort->m_pCurrentMenu && gViewPort->m_pCurrentMenu->IsActive( ) )
		return 1;

	if( gHUD.m_ShowMenuPE.m_iActive )
		return 1;

	if( m_iActive == 2 )
	{
/*		g_helper->ShowHelp( HELP_TEAMMENU, "help/teams_sec.cfg", true );
		g_helper->ShowHelp( HELP_TEAMMENU, "help/teams_syn.cfg" );*/
	}
	m_iActive = 1;

	int y = 0;
	int x = 0;
	rect_s rect;
	rect.top = 0;
	rect.bottom = 256;
	rect.left = 0;
	rect.right = 256;

	SPR_Set( sprTeam1, 255, 255, 255 );
	SPR_DrawHoles( 0, x      , y      , &rect );

	SPR_Set( sprTeam2, 255, 255, 255 );
	SPR_DrawHoles( 0, x + 256, y      , &rect );

	SPR_Set( sprTeam3, 255, 255, 255 );
	SPR_DrawHoles( 0, x      , y + 256, &rect );

	SPR_Set( sprTeam4, 255, 255, 255 );
	SPR_DrawHoles( 0, x + 256, y + 256, &rect );

	if( g_Mouse1r )
	{
		int i = -1;
		while( g_Items[++i].slot != -1 )
			if( inside( g_MouseX, g_MouseY, g_Items[i].area.left, g_Items[i].area.right, g_Items[i].area.top, g_Items[i].area.bottom ) )
				SelectMenuItem( g_Items[i].slot );
		g_Mouse1r = 0;
	}
	return 1;
}

void CHudPETeam::SelectMenuItem( int menu_item )
{
	char szbuf[32];
	snprintf( szbuf, sizeof(szbuf), "menuselect %d\n", menu_item );
	ClientCmd( szbuf );

	m_iActive = 0;
	m_iFlags &= ~HUD_ACTIVE;
	if( gViewPort )
		gViewPort->UpdateCursorState( );
}


int CHudPETeam::MsgFunc_PETeam( const char *pszName, int iSize, void *pbuf )
{
	char *temp = NULL;

	BEGIN_READ( pbuf, iSize );

	m_iActive = 2;
	m_iFlags |= HUD_ACTIVE;
	if( gViewPort )
		gViewPort->UpdateCursorState( );

	return 1;
}
