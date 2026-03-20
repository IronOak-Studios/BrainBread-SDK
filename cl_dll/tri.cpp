//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include <gl/gl.h>
#include <gl/glext.h>
#include "pe_rain.h"
#include "bb_blood.h"
#include "r_studioint.h"

#define DLLEXPORT __declspec( dllexport )

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles( void );
	void DLLEXPORT HUD_DrawTransparentTriangles( void );
};

//#define TEST_IT
#if defined( TEST_IT )

/*
=================
Draw_Triangles

Example routine.  Draws a sprite offset from the player origin.
=================
*/
void Draw_Triangles( void )
{
	cl_entity_t *player;
	vec3_t org;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if ( !player )
		return;

	org = player->origin;

	org.x += 50;
	org.y += 50;

	if (gHUD.m_hsprCursor == 0)
	{
		char sz[256];
		sprintf( sz, "sprites/cursor.spr" );
		gHUD.m_hsprCursor = SPR_Load( sz );
	}

	if ( !gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( gHUD.m_hsprCursor ), 0 ))
	{
		return;
	}
	
	// Create a triangle, sigh
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	// Overload p->color with index into tracer palette, p->packedColor with brightness
	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
	// UNDONE: This gouraud shading causes tracers to disappear on some cards (permedia2)
	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y, org.z );

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

#endif

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles( void )
{

	gHUD.m_Spectator.DrawOverview();
	
#if defined( TEST_IT )
//	Draw_Triangles();
#endif
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/

extern float g_fFogCol[4];
extern float g_fFogStart;
extern float g_fFogEnd;
extern bool g_bFogOn;
extern engine_studio_api_t IEngineStudio;

float fTime = -1;
/*int iRainIndex = 0;
vec3_t vRainMin[20];
vec3_t vRainMax[20];
t_partinfo sRainInfo[20];
cRain *gRain[20];*/
s_bloodlist *gFBlood;
s_bloodlist *gLBlood;

void DLLEXPORT HUD_DrawTransparentTriangles( void )
{
	int i = 0;
	if( fTime == -1 )
		fTime = gEngfuncs.GetClientTime( );

	/*if( iRainIndex )
	{
		if( !gRain[0] )
		{
			for( i = 0; i < iRainIndex; i++ )
      {
        if( sRainInfo[i].iType > TYPE_SNOW ) 
				  gRain[i] = new cGlobalRain( i, &sRainInfo[i] );
        else
          gRain[i] = new cRain( i, &sRainInfo[i] );
      }
		}
		for( i = 0; i < iRainIndex; i++ )
			gRain[i]->Think( gEngfuncs.GetClientTime() - fTime );
		for( i = 0; i < iRainIndex; i++ )
			gRain[i]->Draw( );
	}*/
  s_bloodlist *cur = gFBlood;
  for( ; cur; cur = ( cur ? cur->next : NULL ) )
    cur->item->Think( &cur, gEngfuncs.GetClientTime() - fTime );
  for( cur = gFBlood; cur; cur = cur->next )
    cur->item->Draw( );
	fTime = gEngfuncs.GetClientTime();

	if( IEngineStudio.IsHardware( ) == 2 )
	{
		gEngfuncs.pTriAPI->Fog( g_fFogCol, g_fFogStart, g_fFogEnd, g_bFogOn );
		return;
	}
	if( IEngineStudio.IsHardware( ) != 1 )
		return;

	if( g_bFogOn )
	{
		float col[4];
		col[0] = g_fFogCol[0] / 256.0f;
		col[1] = g_fFogCol[1] / 256.0f;
		col[2] = g_fFogCol[2] / 256.0f;
		col[3] = g_fFogCol[3] / 256.0f;
		glFogi( GL_FOG_MODE, GL_LINEAR );
		glFogfv( GL_FOG_COLOR, col );
		glFogf( GL_FOG_DENSITY, g_fFogStart / g_fFogEnd );
		glHint( GL_FOG_HINT, GL_NICEST );
		glFogf( GL_FOG_START, g_fFogStart ); 
		glFogf( GL_FOG_END, g_fFogEnd ); 
		glEnable( GL_FOG );
	}
	else
		glDisable( GL_FOG );
    //gEngfuncs.pTriAPI->Fog( g_fFogCol, g_fFogStart, g_fFogEnd, g_bFogOn );*/
#if defined( TEST_IT )
//	Draw_Triangles();
#endif
}