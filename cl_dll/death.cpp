/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// death notice
//
#include "hud.h"
#include "cl_util.h"
#include "hud_scale.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "vgui_TeamFortressViewport.h"

DECLARE_MESSAGE( m_DeathNotice, DeathMsg );

struct DeathNoticeItem {
	char szKiller[MAX_PLAYER_NAME_LENGTH*2];
	char szVictim[MAX_PLAYER_NAME_LENGTH*2];
	int iHead;	// the index number of the HS sprite, -1 if not HS
	int iId;	// the index number of the associated sprite
	int iSuicide;
	int iTeamKill;
	int iNonPlayerKill;
	float flDisplayTime;
	float *KillerColor;
	float *VictimColor;
};

#define MAX_DEATHNOTICES	4
static int DEATHNOTICE_DISPLAY_TIME = 6;

#define DEATHNOTICE_TOP		32

DeathNoticeItem rgDeathNoticeList[ MAX_DEATHNOTICES + 1 ];

float g_ColorBlue[3]	= { 0.6, 0.8, 1.0 };
float g_ColorRed[3]		= { 1.0, 0.25, 0.25 };
float g_ColorGreen[3]	= { 0.6, 1.0, 0.6 };
float g_ColorYellow[3]	= { 1.0, 0.7, 0.0 };
float g_ColorGrey[3]	= { 0.8, 0.8, 0.8 };

float *GetClientColor( int clientIndex )
{
	switch ( g_PlayerExtraInfo[clientIndex].teamnumber )
	{
	case 1:	return g_ColorBlue;
	case 2: return g_ColorRed;
	case 3: return g_ColorYellow;
	case 4: return g_ColorGreen;
	case 0: return g_ColorYellow;

		default	: return g_ColorGrey;
	}

	return NULL;
}

int CHudDeathNotice :: Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( DeathMsg );

	CVAR_CREATE( "hud_deathnotice_time", "6", 0 );

	return 1;
}


void CHudDeathNotice :: InitHUDData( void )
{
	memset( rgDeathNoticeList, 0, sizeof(rgDeathNoticeList) );
}


int CHudDeathNotice :: VidInit( void )
{
	m_HUD_d_skull = gHUD.GetSpriteIndex( "d_skull" );

	return 1;
}

int CHudDeathNotice :: Draw( float flTime )
{
	int x, y, r, g, b;

	for ( int i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if ( rgDeathNoticeList[i].iId == 0 )
			break;  // we've gone through them all

		if ( rgDeathNoticeList[i].flDisplayTime < flTime )
		{ // display time has expired
			// remove the current item from the list
			memmove( &rgDeathNoticeList[i], &rgDeathNoticeList[i+1], sizeof(DeathNoticeItem) * (MAX_DEATHNOTICES - i) );
			i--;  // continue on the next item;  stop the counter getting incremented
			continue;
		}

		rgDeathNoticeList[i].flDisplayTime = min( rgDeathNoticeList[i].flDisplayTime, gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME );

		// Only draw if the viewport will let me
		if ( gViewPort && gViewPort->AllowedToPrintText() )
		{
			// Draw the death notice
			y = YRES(DEATHNOTICE_TOP) + HudScale( 2 ) + (HudScale( 20 ) * i);  //!!!

			int id = (rgDeathNoticeList[i].iId == -1) ? m_HUD_d_skull : rgDeathNoticeList[i].iId;
			int head = rgDeathNoticeList[i].iHead;
			int sprW = HudScale( gHUD.GetSpriteRect(id).right - gHUD.GetSpriteRect(id).left );
			x = ScreenWidth - ConsoleStringLen(rgDeathNoticeList[i].szVictim) - sprW;
			if( head >= 0 )
				x -= HudScale( gHUD.GetSpriteRect( head ).right - gHUD.GetSpriteRect( head ).left );

			if ( !rgDeathNoticeList[i].iSuicide )
			{
				x -= (HudScale( 5 ) + ConsoleStringLen( rgDeathNoticeList[i].szKiller ) );

				// Draw killers name
				if ( rgDeathNoticeList[i].KillerColor )
					gEngfuncs.pfnDrawSetTextColor( rgDeathNoticeList[i].KillerColor[0], rgDeathNoticeList[i].KillerColor[1], rgDeathNoticeList[i].KillerColor[2] );
				x = HudScale( 5 ) + DrawConsoleString( x, y, rgDeathNoticeList[i].szKiller );
			}

			r = 255;  g = 80;	b = 0;
			if ( rgDeathNoticeList[i].iTeamKill )
			{
				r = 10;	g = 240; b = 10;  // display it in sickly green
			}

			// Draw death weapon
			ScaledSPR_DrawAdditive( gHUD.GetSprite(id), 0, x, y, &gHUD.GetSpriteRect(id), r, g, b );

			x += sprW;
			
			if( head >= 0 )
			{
				int headW = HudScale( gHUD.GetSpriteRect(head).right - gHUD.GetSpriteRect(head).left );
				ScaledSPR_DrawAdditive( gHUD.GetSprite(head), 0, x, y, &gHUD.GetSpriteRect(head), r, g, b );

				x += headW;
			}

			// Draw victims name (if it was a player that was killed)
			if (rgDeathNoticeList[i].iNonPlayerKill == FALSE)
			{
				if ( rgDeathNoticeList[i].VictimColor )
					gEngfuncs.pfnDrawSetTextColor( rgDeathNoticeList[i].VictimColor[0], rgDeathNoticeList[i].VictimColor[1], rgDeathNoticeList[i].VictimColor[2] );
				x = DrawConsoleString( x, y, rgDeathNoticeList[i].szVictim );
			}
		}
	}

	return 1;
}

// This message handler may be better off elsewhere
int CHudDeathNotice :: MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );

	int killer = READ_BYTE();
	int victim = READ_BYTE();

	const char *wpns[32] = 
	{
		"  Desert Eagle", "  Seburo CX", "  Benelli M3", "  Beretta 92FS",
		"  M16", "  Glock 18 Semi", "  Knife", "  Steyr Aug", "  M134 Vulcan Minigun",
		"  IMI MicroUzi", "  IMI MicroUzi akimbo", "  H&K MP5/10", "  Gastank",
		"  H&K USP 9", "  Winchester", "  Sig550", "  Sig P226", "  Sig P225", "  Beretta 92FS akimbo",
		"  Glock 18 auto akimbo", "  Glock 18 auto", "  H&K MP5 sd", "  IMI Tavor", "  sawed off Shotgun", "  AK47",
		"  Case", "  H&K USP MP", "  bare fowl hands", "  Orbital Device", "  Flammenwerfer S"
	};
	char killedwith[32];
	int head = 0;

	if ( victim > 0 && victim <= MAX_PLAYERS )
	{
		g_IsSpectator[victim] = 1;
	}

	snprintf( killedwith, sizeof(killedwith), "d_%s", READ_STRING() );
	if( !strcmp( killedwith, "d_headshot" ) )
	{
		head = 1;
		snprintf( killedwith, sizeof(killedwith), "d_%s", READ_STRING() );
	}

	if (gViewPort)
		gViewPort->DeathMsg( killer, victim );

	gHUD.m_Spectator.DeathMessage(victim);

	int i;
	for ( i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if ( rgDeathNoticeList[i].iId == 0 )
			break;
	}
	if ( i == MAX_DEATHNOTICES )
	{ // move the rest of the list forward to make room for this item
		memmove( rgDeathNoticeList, rgDeathNoticeList+1, sizeof(DeathNoticeItem) * MAX_DEATHNOTICES );
		i = MAX_DEATHNOTICES - 1;
	}

	if( gViewPort )
	{
		gViewPort->GetAllPlayersInfo( );
		gViewPort->UpdateOnPlayerInfo( );
	}

	// Get the Killer's name
  if( killer == 65 )
  {
		rgDeathNoticeList[i].KillerColor = g_ColorGrey;
		strncpy( rgDeathNoticeList[i].szKiller, "Z0mBiE\0", MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH-1] = 0;
  } 
  else if( killer == 67 )
  {
		rgDeathNoticeList[i].KillerColor = g_ColorGrey;
		strncpy( rgDeathNoticeList[i].szKiller, "FRED!\0", MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH-1] = 0;
  }
  else if( killer == 66 )
  {
		rgDeathNoticeList[i].KillerColor = g_ColorGrey;
		strncpy( rgDeathNoticeList[i].szKiller, "Marine\0", MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH-1] = 0;
  }
  else if( killer == 68 )
  {
		rgDeathNoticeList[i].KillerColor = g_ColorGrey;
		strncpy( rgDeathNoticeList[i].szKiller, "Barney\0", MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH-1] = 0;
  }
  else if( killer == 69 )
  {
		rgDeathNoticeList[i].KillerColor = g_ColorGrey;
		strncpy( rgDeathNoticeList[i].szKiller, "Tank\0", MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH-1] = 0;
  }
  else if( killer == 70 )
  {
		rgDeathNoticeList[i].KillerColor = g_ColorGrey;
		strncpy( rgDeathNoticeList[i].szKiller, "BlackHawk\0", MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH-1] = 0;
  }
  else if ( killer > 0 && killer <= MAX_PLAYERS )
  {
	  char *killer_name = g_PlayerInfoList[ killer ].name;
	  if ( !killer_name )
	  {
		  killer_name = "";
		  rgDeathNoticeList[i].szKiller[0] = 0;
	  }
    else
	  {
		  rgDeathNoticeList[i].KillerColor = GetClientColor( killer );
		  strncpy( rgDeathNoticeList[i].szKiller, killer_name, MAX_PLAYER_NAME_LENGTH );
		  rgDeathNoticeList[i].szKiller[MAX_PLAYER_NAME_LENGTH-1] = 0;
	  }
  }

	// Get the Victim's name
	char *victim_name = NULL;

  if( victim == 65 )
  {
		rgDeathNoticeList[i].VictimColor = g_ColorGrey;
		strncpy( rgDeathNoticeList[i].szVictim, "Z0mBiE\0", MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH-1] = 0;
  } 
  else if( victim == 67 )
  {
		rgDeathNoticeList[i].VictimColor = g_ColorGrey;
		strncpy( rgDeathNoticeList[i].szVictim, "FRED!\0", MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH-1] = 0;
  }
  else if( victim == 66 )
  {
		rgDeathNoticeList[i].VictimColor = g_ColorGrey;
		strncpy( rgDeathNoticeList[i].szVictim, "Marine\0", MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH-1] = 0;
  }
  else if( victim == 68 )
  {
		rgDeathNoticeList[i].VictimColor = g_ColorGrey;
		strncpy( rgDeathNoticeList[i].szVictim, "Barney\0", MAX_PLAYER_NAME_LENGTH );
		rgDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH-1] = 0;
  }
  else if ( victim > 0 && victim <= MAX_PLAYERS )
  {
    if ( ((char)victim) != -1 )
		  victim_name = g_PlayerInfoList[ victim ].name;
	  if ( !victim_name )
	  {
		  victim_name = "";
		  rgDeathNoticeList[i].szVictim[0] = 0;
	  }
	  else
	  {
		  rgDeathNoticeList[i].VictimColor = GetClientColor( victim );
		  strncpy( rgDeathNoticeList[i].szVictim, victim_name, MAX_PLAYER_NAME_LENGTH );
		  rgDeathNoticeList[i].szVictim[MAX_PLAYER_NAME_LENGTH-1] = 0;
	  }
  }

	// Is it a non-player object kill?
	if ( ((char)victim) == -1 )
	{
		rgDeathNoticeList[i].iNonPlayerKill = TRUE;

		// Store the object's name in the Victim slot (skip the d_ bit)
		strcpy( rgDeathNoticeList[i].szVictim, killedwith+2 );
	}
	else
	{
		if ( killer == victim || killer == 0 )
			rgDeathNoticeList[i].iSuicide = TRUE;

		if ( !strcmp( killedwith, "d_teammate" ) )
			rgDeathNoticeList[i].iTeamKill = TRUE;
	}

	// Find the sprite in the list
	int sprhead = -1, spr = 0;
	spr = gHUD.GetSpriteIndex( killedwith );
	if( spr >= 0 && head )
		sprhead = gHUD.GetSpriteIndex( "d_headshot" );

	rgDeathNoticeList[i].iHead = sprhead;
	rgDeathNoticeList[i].iId = spr;

	DEATHNOTICE_DISPLAY_TIME = CVAR_GET_FLOAT( "hud_deathnotice_time" );
	rgDeathNoticeList[i].flDisplayTime = gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME;

	if (rgDeathNoticeList[i].iNonPlayerKill)
	{
		ConsolePrint( rgDeathNoticeList[i].szKiller );
		ConsolePrint( " killed a " );
		ConsolePrint( rgDeathNoticeList[i].szVictim );
		ConsolePrint( "\n" );
	}
	else
	{
		// record the death notice in the console
		if ( rgDeathNoticeList[i].iSuicide )
		{
			ConsolePrint( rgDeathNoticeList[i].szVictim );

			if ( !strcmp( killedwith, "d_world" ) )
			{
				ConsolePrint( " died" );
			}
			else
			{
				ConsolePrint( " committed suicide\n" );
				return 1;
			}
		}
		else if ( rgDeathNoticeList[i].iTeamKill )
		{
			ConsolePrint( rgDeathNoticeList[i].szKiller );
			ConsolePrint( " killed his teammate " );
			ConsolePrint( rgDeathNoticeList[i].szVictim );
		}
		else
		{
			ConsolePrint( rgDeathNoticeList[i].szKiller );
			ConsolePrint( " killed " );
			ConsolePrint( rgDeathNoticeList[i].szVictim );
		}

		if ( killedwith && *killedwith && (*killedwith > 13 ) && strcmp( killedwith, "d_world" ) && !rgDeathNoticeList[i].iTeamKill )
		{
			if ( !strcmp( killedwith+2, "skull" ) )
			{
				ConsolePrint( "\n" );
				return 1;
			}
			ConsolePrint( " with " );

			if ( !strcmp( killedwith+2, "deagle" ) )
				strcpy( killedwith, wpns[0] );
			else if ( !strcmp( killedwith+2, "seburo" ) )
				strcpy( killedwith, wpns[1] );
			else if ( !strcmp( killedwith+2, "benelli" ) )
				strcpy( killedwith, wpns[2] );
			else if ( !strcmp( killedwith+2, "beretta" ) )
				strcpy( killedwith, wpns[3] );
			else if ( !strcmp( killedwith+2, "m16" ) )
				strcpy( killedwith, wpns[4] );
			else if ( !strcmp( killedwith+2, "glock" ) )
				strcpy( killedwith, wpns[5] );
			else if ( !strcmp( killedwith+2, "knife" ) )
				strcpy( killedwith, wpns[6] );
			else if ( !strcmp( killedwith+2, "aug" ) )
				strcpy( killedwith, wpns[7] );
			else if ( !strcmp( killedwith+2, "minigun" ) )
				strcpy( killedwith, wpns[8] );
			else if ( !strcmp( killedwith+2, "microuzi" ) )
				strcpy( killedwith, wpns[9] );
			else if ( !strcmp( killedwith+2, "microuzi_a" ) )
				strcpy( killedwith, wpns[10] );
			else if ( !strcmp( killedwith+2, "mp5" ) )
				strcpy( killedwith, wpns[11] );
			else if ( !strcmp( killedwith+2, "canister" ) )
				strcpy( killedwith, wpns[12] );
			else if ( !strcmp( killedwith+2, "grenade" ) )
				strcpy( killedwith, wpns[12] );
			else if ( !strcmp( killedwith+2, "usp" ) )
				strcpy( killedwith, wpns[13] );
			else if ( !strcmp( killedwith+2, "winchester" ) )
				strcpy( killedwith, wpns[14] );
			else if ( !strcmp( killedwith+2, "sig550" ) )
				strcpy( killedwith, wpns[15] );
			else if ( !strcmp( killedwith+2, "p226" ) )
				strcpy( killedwith, wpns[16] );
			else if ( !strcmp( killedwith+2, "p225" ) )
				strcpy( killedwith, wpns[17] );
			else if ( !strcmp( killedwith+2, "beretta_a" ) )
				strcpy( killedwith, wpns[18] );
			else if ( !strcmp( killedwith+2, "glock_auto_a" ) )
				strcpy( killedwith, wpns[19] );
			else if ( !strcmp( killedwith+2, "glock_auto" ) )
				strcpy( killedwith, wpns[20] );
			else if ( !strcmp( killedwith+2, "mp5sd" ) )
				strcpy( killedwith, wpns[21] );
			else if ( !strcmp( killedwith+2, "tavor" ) )
				strcpy( killedwith, wpns[22] );
			else if ( !strcmp( killedwith+2, "sawed" ) )
				strcpy( killedwith, wpns[23] );
			else if ( !strcmp( killedwith+2, "ak47" ) )
				strcpy( killedwith, wpns[24] );
			else if ( !strcmp( killedwith+2, "case" ) )
				strcpy( killedwith, wpns[25] );
			else if ( !strcmp( killedwith+2, "uspmp" ) )
				strcpy( killedwith, wpns[26] );
			else if ( !strcmp( killedwith+2, "hand" ) )
				strcpy( killedwith, wpns[27] );
			else if ( !strcmp( killedwith+2, "orbital" ) )
				strcpy( killedwith, wpns[28] );
			else if ( !strcmp( killedwith+2, "flame" ) )
				strcpy( killedwith, wpns[29] );

			ConsolePrint( killedwith+2 ); // skip over the "d_" part
		}

		ConsolePrint( "\n" );
	}

	return 1;
}




