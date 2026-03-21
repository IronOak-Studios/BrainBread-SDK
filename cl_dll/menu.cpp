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
// menu.cpp
//
// generic menu handler
//
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "vgui_TeamFortressViewport.h"
#include "pe_font.h"

#define MAX_MENU_STRING	512
char g_szMenuString[MAX_MENU_STRING];
char g_szPrelocalisedMenuString[MAX_MENU_STRING];

int KB_ConvertString( char *in, char **ppout );

DECLARE_MESSAGE( m_Menu, ShowMenu );

int CHudMenu :: Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( ShowMenu );

	InitHUDData();

	return 1;
}

void CHudMenu :: InitHUDData( void )
{
	m_fMenuDisplayed = 0;
	m_bitsValidSlots = 0;
	Reset();
}

void CHudMenu :: Reset( void )
{
	g_szPrelocalisedMenuString[0] = 0;
	m_fWaitingForMore = FALSE;
}

int CHudMenu :: VidInit( void )
{
	m_fMenuDisplayed = 0;
	m_bitsValidSlots = 0;
	m_iFlags &= ~HUD_ACTIVE;

	Reset();
	return 1;
}

int CHudMenu :: Draw( float flTime )
{
	// check for if menu is set to disappear
	if ( m_flShutoffTime > 0 )
	{
		if ( m_flShutoffTime <= gHUD.m_flTime )
		{  // times up, shutoff
			m_fMenuDisplayed = 0;
			m_iFlags &= ~HUD_ACTIVE;
			return 1;
		}
	}

	// don't draw the menu if the scoreboard is being shown
	if ( gViewPort && gViewPort->IsScoreBoardVisible() )
		return 1;
	
	if ( gViewPort && gViewPort->m_pCurrentMenu && gViewPort->m_pCurrentMenu->IsActive( ) )
		return 1;

	/*if( gHUD.m_ShowMenuPE.m_iActive )
		return 1;*/

	// draw the menu, along the left-hand side of the screen
	g_font->SetFont( FONT_MENU );

	// count the number of newlines
	int nlc = 0;
	int i;
	for ( int i = 0; i < MAX_MENU_STRING && g_szMenuString[i] != '\0'; i++ )
	{
		if ( g_szMenuString[i] == '\n' )
			nlc++;
	}

	// center it
	//int y = (ScreenHeight/2) - ((nlc/2)*12) - 40; // make sure it is above the say text
	//int x = 20;

	int y = 64; // make sure it is above the say text
	int x = 24;

	// FIRO
	int r = 0, g = 0, b = 0;
	i = 0;

	while ( i < MAX_MENU_STRING && g_szMenuString[i] != '\0' )
	{
		// White
		if (g_szMenuString[i] == '\\' && g_szMenuString[i+1] == 'w')
		{
			r = 255;
			g = 255;
			b = 255;
		}
		// Yellow
		else if (g_szMenuString[i] == '\\' && g_szMenuString[i+1] == 'y')
		{
			r = 255;
			g = 255;
			b = 0;
		}
		// Red
		else if (g_szMenuString[i] == '\\' && g_szMenuString[i+1] == 'r')
		{
			r = 192;
			g = 0;
			b = 0;
		}
		// green
		else if (g_szMenuString[i] == '\\' && g_szMenuString[i+1] == 'g')
		{
			r = 0;
			g = 255;
			b = 0;
		}
		// cyan
		else if (g_szMenuString[i] == '\\' && g_szMenuString[i+1] == 'c')
		{
			r = 0;
			g = 255;
			b = 255;
		}
		// Grey
		else if (g_szMenuString[i] == '\\' && g_szMenuString[i+1] == 'q')
		{
			r = 128;
			g = 128;
			b = 128;
		}
		// transparent
		else if (g_szMenuString[i] == '\\' && g_szMenuString[i+1] == 't')
		{
			r = 128;
			g = 128;
			b = 128;
		}
		g_font->DrawString( x, y, g_szMenuString+i+2, r, g, b );
		y += 12;
		
		x = 20;
		// FIRO

		while ( i < MAX_MENU_STRING && g_szMenuString[i] != '\0' && g_szMenuString[i] != '\n' )
			i++;
		if ( g_szMenuString[i] == '\n' )
			i++;
	}
	
	return 1;
}

// selects an item from the menu
void CHudMenu :: SelectMenuItem( int menu_item )
{
	// if menu_item is in a valid slot,  send a menuselect command to the server
	//if ( (menu_item > 0) && (m_bitsValidSlots & (1 << (menu_item-1))) )
	//{
		char szbuf[32];
		sprintf( szbuf, "menuselect %d\n", menu_item );
		ClientCmd( szbuf );

		// remove the menu
		m_fMenuDisplayed = 0;
		m_iFlags &= ~HUD_ACTIVE;
	//}
}


// Message handler for ShowMenu message
// takes four values:
//		short: a bitfield of keys that are valid input
//		char : the duration, in seconds, the menu should stay up. -1 means is stays until something is chosen.
//		byte : a boolean, TRUE if there is more string yet to be received before displaying the menu, FALSE if it's the last string
//		string: menu string to display
// if this message is never received, then scores will simply be the combined totals of the players.
int CHudMenu :: MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf )
{
	char *temp = NULL;

	BEGIN_READ( pbuf, iSize );

	m_bitsValidSlots = READ_SHORT();
	int DisplayTime = READ_CHAR();
	int NeedMore = READ_BYTE();

	if ( DisplayTime > 0 )
		m_flShutoffTime = DisplayTime + gHUD.m_flTime;
	else
		m_flShutoffTime = -1;

	if ( m_bitsValidSlots )
	{
		if ( !m_fWaitingForMore ) // this is the start of a new menu
		{
			strncpy( g_szPrelocalisedMenuString, READ_STRING(), MAX_MENU_STRING );
		}
		else
		{  // append to the current menu string
			strncat( g_szPrelocalisedMenuString, READ_STRING(), MAX_MENU_STRING - strlen(g_szPrelocalisedMenuString) );
		}
		g_szPrelocalisedMenuString[MAX_MENU_STRING-1] = 0;  // ensure null termination (strncat/strncpy does not)

		if ( !NeedMore )
		{  // we have the whole string, so we can localise it now
			strncpy( g_szMenuString, gHUD.m_TextMessage.BufferedLocaliseTextString( g_szPrelocalisedMenuString ), MAX_MENU_STRING );
			g_szMenuString[ MAX_MENU_STRING - 1 ] = 0;

			// Swap in characters
			if ( KB_ConvertString( g_szMenuString, &temp ) )
			{
				strncpy( g_szMenuString, temp, MAX_MENU_STRING );
				g_szMenuString[ MAX_MENU_STRING - 1 ] = 0;
				free( temp );
			}
		}

		m_fMenuDisplayed = 1;
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		m_fMenuDisplayed = 0; // no valid slots means that the menu should be turned off
		m_iFlags &= ~HUD_ACTIVE;
	}

	m_fWaitingForMore = NeedMore;

	return 1;
}
