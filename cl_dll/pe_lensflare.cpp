#include "hud.h"
#include "cl_util.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "pm_defs.h"
#include "vgui_TeamFortressViewport.h"
#include "event_api.h"
#include "demo_api.h"

#include "entity_types.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

extern vec3_t		v_origin, v_angles, v_cl_angles, v_sim_org, v_lastAngles;

#define LENS_FADE_TIME 0.05
#define MAX_LENSFLARES 5 // Up to 24

DECLARE_MESSAGE( m_Lens, LensRef )

int CHudLens::Init( )
{
	HOOK_MESSAGE( LensRef );

	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);

	m_type = 101;

	return 1;
}

float nextcheck = 0;

int CHudLens::VidInit( )
{
	for( int i = 0; i < MAX_LENSFLARES; i++ )
	{
		m_iRef[i] = 0;
		m_iActive[i] = 0;
		m_fRemove[i] = 0;
		m_fActivate[i] = 0;
	}
	m_hSpr[0] = LoadSprite( "sprites/l1.spr" );
	m_hSpr[1] = LoadSprite( "sprites/l2.spr" );
	m_hSpr[2] = LoadSprite( "sprites/l3.spr" );
	m_hSpr[3] = LoadSprite( "sprites/l4.spr" );
	m_hSpr[4] = LoadSprite( "sprites/l5.spr" );
	m_hSpr[5] = LoadSprite( "sprites/l6.spr" );
	m_hSpr[6] = LoadSprite( "sprites/l7.spr" );
	m_hSpr[7] = LoadSprite( "sprites/l8.spr" );
	m_iIndex = MAX_LENSFLARES;
	m_iIndexRef = 0;
  nextcheck = 0;
	return 1;
}

extern float g_iBob;

int CHudLens::Draw( float flTime )
{
  if( (int)CVAR_GET_FLOAT( "cl_lensflare" ) == 0 )
    return TRUE;
	if( gEngfuncs.pDemoAPI->IsPlayingback( ) )
		return TRUE;

	if( !m_iIndexRef )
	{
		ClientCmd( "getcoords\n" );
		m_iIndexRef = 1;
	}
	if( m_iIndexRef == 1 )
		return TRUE;

  if( nextcheck <= flTime )
  {
	  CheckFlares( );
    nextcheck = flTime + 0.05;
  }

	for( int i = 0; i < m_iIndex; i++ )
	{
		if( m_fRemove[i] == -1 )
			m_fRemove[i] = flTime + LENS_FADE_TIME;
		if( m_fActivate[i] == -1 )
	 		m_fActivate[i] = flTime + LENS_FADE_TIME;
		if( ( m_fRemove[i] <= flTime ) && ( m_fRemove[i] > 0 ) )
			m_fRemove[i] = 0;
		if( ( m_fActivate[i] <= flTime ) && ( m_fActivate[i] > 0 ) )
			m_fActivate[i] = 0;
		if( !m_iActive[i] && ( m_fRemove[i] == 0 ) )
			continue;
		vec3_t view_ofs, screen, origin = m_vecLOrigin[i];
		cl_entity_t	*ent = gEngfuncs.GetLocalPlayer( );
		int lx, ly; // licht koordinaten
		int x = (int)( ScreenWidth / 2 ), y = (int)( ScreenHeight / 2 );
		
		if( m_iRef[i] && ent )
		{
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
			origin = /*ent->curstate.origin + view_ofs*/v_origin + dist[i];
			//origin[2] += g_iBob;
		}

		if ( gEngfuncs.pTriAPI->WorldToScreen( origin, screen ) )
			continue;
		screen[0] = XPROJECT(screen[0]);
		screen[1] = YPROJECT(screen[1]);
		lx = (int)( screen[0] );
		ly = (int)( screen[1] );

		if( lx < 0 || lx > ScreenWidth )
			continue;
		if( ly < 0 || ly > ScreenHeight )
			continue;
		int a = 200, borderx = 100, bordery = 80;	
		if( lx < XRES(borderx) )
			a = (int)( ( (float)a / (float)XRES(borderx) ) * lx );
		else
		if( lx > ( ScreenWidth - XRES(borderx) ) )
			a = (int)( ( (float)a / (float)XRES(borderx) ) * ( XRES(borderx) - ( lx - ( ScreenWidth - XRES(borderx) ) ) ) );
		//else
		if( ly < YRES(bordery) )
			a = (int)( ( (float)a / (float)YRES(bordery) ) * ly );
		else
		if( ly > ( ScreenHeight - YRES(bordery) ) )
			a = (int)( ( (float)a / (float)YRES(bordery) ) * ( YRES(bordery) - ( ly - ( ScreenHeight - YRES(bordery) ) ) ) );
		
		if( m_fRemove[i] > 0 )
			a = (int)( ( (float)a / LENS_FADE_TIME ) * ( m_fRemove[i] - flTime ) );
		if( m_fActivate[i] > 0 )
			a = (int)( ( (float)a / LENS_FADE_TIME ) * ( LENS_FADE_TIME - ( m_fActivate[i] - flTime ) ) );

		int vecx = x - lx;
		int vecy = y - ly;
		int r = 255, g = 255, b = 255; // 180 180 180
		ScaleColors( r, g, b, a );

		int vecpx = x + (int)( (float)vecx * (-1.25) );
		int vecpy = y + (int)( (float)vecy * (-1.25) );
		SPR_Set( m_hSpr[3], r, g, b );
		SPR_DrawAdditive( 0, vecpx - (int)( (float)SPR_Width( m_hSpr[3], 0 ) / 2 ), vecpy - (int)( (float)SPR_Height( m_hSpr[3], 0 ) / 2 ), NULL ); //&gHUD.GetSpriteRect( m_hSpr[3] ) );

		vecpx = x + (int)( (float)vecx * (-1) );
		vecpy = y + (int)( (float)vecy * (-1) );
		SPR_Set( m_hSpr[0], r, g, b );
		SPR_DrawAdditive( 0, vecpx - (int)( (float)SPR_Width( m_hSpr[0], 0 ) / 2 ), vecpy - (int)( (float)SPR_Height( m_hSpr[0], 0 ) / 2 ), NULL ); //&gHUD.GetSpriteRect( m_hSpr[0] ) );
		
		vecpx = x + (int)( (float)vecx * (-0.75) );
		vecpy = y + (int)( (float)vecy * (-0.75) );
		SPR_Set( m_hSpr[7], r, g, b );
		SPR_DrawAdditive( 0, vecpx - (int)( (float)SPR_Width( m_hSpr[7], 0 ) / 2 ), vecpy - (int)( (float)SPR_Height( m_hSpr[7], 0 ) / 2 ), NULL ); //&gHUD.GetSpriteRect( m_hSpr[7] ) );

		vecpx = x + (int)( (float)vecx * (-0.5) );
		vecpy = y + (int)( (float)vecy * (-0.5) );
		SPR_Set( m_hSpr[4], r, g, b );
		SPR_DrawAdditive( 0, vecpx - (int)( (float)SPR_Width( m_hSpr[4], 0 ) / 2 ), vecpy - (int)( (float)SPR_Height( m_hSpr[4], 0 ) / 2 ), NULL ); //&gHUD.GetSpriteRect( m_hSpr[4] ) );
		
		vecpx = x + (int)( (float)vecx * (-0.25) );
		vecpy = y + (int)( (float)vecy * (-0.25) );
		SPR_Set( m_hSpr[1], r, g, b );
		SPR_DrawAdditive( 0, vecpx - (int)( (float)SPR_Width( m_hSpr[1], 0 ) / 2 ), vecpy - (int)( (float)SPR_Height( m_hSpr[1], 0 ) / 2 ), NULL ); //&gHUD.GetSpriteRect( m_hSpr[1] ) );

		vecpx = x + (int)( (float)vecx * (-0.15) );
		vecpy = y + (int)( (float)vecy * (-0.15) );
		SPR_Set( m_hSpr[2], r, g, b );
		SPR_DrawAdditive( 0, vecpx - (int)( (float)SPR_Width( m_hSpr[2], 0 ) / 2 ), vecpy - (int)( (float)SPR_Height( m_hSpr[2], 0 ) / 2 ), NULL ); //&gHUD.GetSpriteRect( m_hSpr[2] ) );
		
		vecpx = x + (int)( (float)vecx * (0.5) );
		vecpy = y + (int)( (float)vecy * (0.5) );
		SPR_Set( m_hSpr[5], r, g, b );
		SPR_DrawAdditive( 0, vecpx - (int)( (float)SPR_Width( m_hSpr[5], 0 ) / 2 ), vecpy - (int)( (float)SPR_Height( m_hSpr[5], 0 ) / 2 ), NULL ); //&gHUD.GetSpriteRect( m_hSpr[5] ) );
		
		vecpx = x + (int)( (float)vecx * (0.25) );
		vecpy = y + (int)( (float)vecy * (0.25) );
		SPR_Set( m_hSpr[2], r, g, b );
		SPR_DrawAdditive( 0, vecpx - (int)( (float)SPR_Width( m_hSpr[2], 0 ) / 2 ), vecpy - (int)( (float)SPR_Height( m_hSpr[2], 0 ) / 2 ), NULL ); //&gHUD.GetSpriteRect( m_hSpr[2] ) );

		vecpx = x + (int)( (float)vecx * (1/5.5) );
		vecpy = y + (int)( (float)vecy * (1/5.5) );
		SPR_Set( m_hSpr[3], r, g, b );
		SPR_DrawAdditive( 0, vecpx - (int)( (float)SPR_Width( m_hSpr[3], 0 ) / 2 ), vecpy - (int)( (float)SPR_Height( m_hSpr[3], 0 ) / 2 ), NULL ); //&gHUD.GetSpriteRect( m_hSpr[3] ) );
		
		vecpx = x + (int)( (float)vecx * (1/8) );
		vecpy = y + (int)( (float)vecy * (1/8) );
		SPR_Set( m_hSpr[6], r, g, b );
		SPR_DrawAdditive( 0, vecpx - (int)( (float)SPR_Width( m_hSpr[6], 0 ) / 2 ), vecpy - (int)( (float)SPR_Height( m_hSpr[6], 0 ) / 2 ), NULL ); //&gHUD.GetSpriteRect( m_hSpr[6] ) );
	}


	return 1;
}

void CHudLens::CheckFlares( )
{
	vec3_t orig = v_origin + Vector( 0, 0, 21 );
	vec3_t end;
	pmtrace_t tr;
	bool visible = false;

	for( int i = 0; i < m_iIndex; i++ )
	{
		if( m_iRef[i] )
			end = orig + ( m_vecLOrigin[i] - m_vecRefOrigin[i] ).Normalize() * 4096;
		else
			end = m_vecLOrigin[i];

    tr = *(gEngfuncs.PM_TraceLine( (float *)&orig, (float *)&end, PM_TRACELINE_ANYVISIBLE, 2 /*point sized hull*/, -1 ) );

		// Use PM_PointContents at the hit point to distinguish sky from walls.
		// Sky brush endpos returns CONTENTS_SKY; walls return CONTENTS_EMPTY.
		visible = ( tr.fraction == 1.0f )
			|| ( gEngfuncs.PM_PointContents( tr.endpos, NULL ) == CONTENTS_SKY );

		if( m_iActive[i] && !visible )
		{
			m_fRemove[i] = -1;
			m_fActivate[i] = 0;
		}
		else if( !m_iActive[i] && visible )
		{
			m_fRemove[i] = 0;
			m_fActivate[i] = -1;
		}
		m_iActive[i] = visible ? 1 : 0;
		if( ( m_vecLOrigin[i].x == 0 ) && ( m_vecLOrigin[i].y == 0 ) && ( m_vecLOrigin[i].z == 0 ) )
		{
			char cmd[512];
			sprintf( cmd, "getcoords\n" );
			ClientCmd( cmd );
		}
	}
}
/*	BEGIN_READ( pbuf, iSize );
	m_iIndex = READ_SHORT( );
	m_iIndexRef = READ_SHORT( );
	int act = READ_BYTE( );
	char cmd[256];

	m_pLightRef = (cl_entity_s*)gEngfuncs.GetEntityByIndex( m_iIndexRef );

	for( int i = 0; i < MAX_LENSFLARES; i++ )
	{
		if( m_pLight[i] && ( m_pLight[i]->index == m_iIndex ) )
		{
			if( m_iActive[i] && !act )
			{
				m_fRemove[i] = -1;
				m_fActivate[i] = 0;
			}
			else if( !m_iActive[i] && act )
			{
				m_fRemove[i] = 0;
				m_fActivate[i] = -1;
			}
			m_iActive[i] = act;
			if( ( m_vecLOrigin[i].x == 0 ) && ( m_vecLOrigin[i].y == 0 ) && ( m_vecLOrigin[i].z == 0 ) )
			{
				sprintf( cmd, "getcoord %d\n", m_pLight[i]->index );
				ClientCmd( cmd );
			}
			return 1;
		}
	}
	for( i = 0; i < MAX_LENSFLARES; i++ )
	{
		if( !m_pLight[i] )
		{
			m_pLight[i] = (cl_entity_s*)gEngfuncs.GetEntityByIndex( m_iIndex );
			if( m_pLight[i] )
			{
				if( m_iActive[i] && !act )
				{
					m_fRemove[i] = -1;
					m_fActivate[i] = 0;
				}
				else if( !m_iActive[i] && act )
				{
					m_fRemove[i] = 0;
					m_fActivate[i] = -1;
				}
				m_iActive[i] = act;
				m_vecLOrigin[i] = m_pLight[i]->curstate.origin;
				if( ( m_vecLOrigin[i].x == 0 ) && ( m_vecLOrigin[i].y == 0 ) && ( m_vecLOrigin[i].z == 0 ) )
				{
					sprintf( cmd, "getcoord %d\n", m_pLight[i]->index );
					ClientCmd( cmd );
				}
				if( m_iIndexRef == 0 )
					m_iRef[i] = 0;
				else
				{
					if( m_pLightRef && m_pLight[i] )
					{
						VectorSubtract( m_vecLOrigin[i], m_pLightRef->curstate.origin, dist[i] );
						m_iRef[i] = 1;
					}
					else
						m_iRef[i] = 0;
				}
				break;
			}
			else
				m_iActive[i] = 0; //sollte eigentlich nie der fall sein
		}
	}

	return 1;
}*/


int CHudLens::MsgFunc_LensRef( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int i = 0;

	while( READ_BYTE( ) && i < MAX_LENSFLARES )
	{
		m_vecRefOrigin[i] = Vector( 0, 0, 0 );
		m_iActive[i] = 0;
		m_fRemove[i] = 0;
		m_fActivate[i] = 0;

		m_vecLOrigin[i].x = READ_COORD( );
		m_vecLOrigin[i].y = READ_COORD( );
		m_vecLOrigin[i].z = READ_COORD( );

		if( READ_BYTE( ) )
		{
			m_vecRefOrigin[i].x = READ_COORD( );
			m_vecRefOrigin[i].y = READ_COORD( );
			m_vecRefOrigin[i].z = READ_COORD( );
		}
		if( m_vecRefOrigin[i].Length( ) != 0 )
		{
			VectorSubtract( m_vecLOrigin[i], m_vecRefOrigin[i], dist[i] );
			m_iRef[i] = 1;
		}
		else
			m_iRef[i] = 0;

		i++;
	}
	m_iIndex = i;
	m_iIndexRef = 2;

	return 1;
}


//gEngfuncs.pTriAPI->WorldToScreen(origin,offset);
