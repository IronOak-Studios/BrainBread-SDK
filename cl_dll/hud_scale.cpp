/***
*
*   hud_scale.cpp
*
*   HUD scaling support for modern resolutions.
*
*   Uses pfnSPR_DrawGeneric with a scissor-rect trick to work around
*   the engine's AdjustSubRect corrupting UV coordinates when width/height
*   are overridden.  We pass prc=NULL so the full sprite is drawn at the
*   scaled size, then use SPR_EnableScissor to clip to the desired sub-rect.
*
****/

#include "hud.h"
#include "cl_util.h"
#include "hud_scale.h"

// ---------------------------------------------------------------------------
// Global scale factor
// ---------------------------------------------------------------------------
float g_flHudScale = 1.0f;

static cvar_t *s_pCvarHudScale = NULL;

void HudScale_Init( void )
{
	// 0 = automatic (ScreenWidth / 1280), >0 = manual multiplier
	s_pCvarHudScale = CVAR_CREATE( "hud_scale", "0", FCVAR_ARCHIVE );
}

void HudScale_Update( void )
{
	float oldScale = g_flHudScale;
	float manual = 0.0f;

	if( s_pCvarHudScale )
		manual = s_pCvarHudScale->value;

	if( manual > 0.0f )
	{
		if( manual < 0.5f )  manual = 0.5f;
		if( manual > 8.0f )  manual = 8.0f;
		g_flHudScale = manual;
	}
	else
	{
		g_flHudScale = (float)ScreenWidth / HUD_BASE_WIDTH;
		if( g_flHudScale < 1.0f )
			g_flHudScale = 1.0f;
	}

	if( g_flHudScale != oldScale )
	{
		gEngfuncs.Con_Printf( "HudScale: %.2f (ScreenWidth=%d, cvar=%.1f)\n",
			g_flHudScale, ScreenWidth, manual );
	}
}

// ---------------------------------------------------------------------------
// Blend-mode constants (passed directly to glBlendFunc by the engine).
// ---------------------------------------------------------------------------
#define GL_BLEND_ONE                 1
#define GL_BLEND_SRC_ALPHA           0x0302
#define GL_BLEND_ONE_MINUS_SRC_ALPHA 0x0303

// ---------------------------------------------------------------------------
// DrawScaledSprite -- core implementation.
//
// Draws the FULL sprite frame (prc=NULL) at the scaled full-sprite size,
// using SPR_EnableScissor to clip to the desired sub-rect region.
// Calls SPR_Set internally so callers don't need to.
// ---------------------------------------------------------------------------
static void DrawScaledSprite( HSPRITE hSprite, int frame, int x, int y,
	const struct rect_s *prc, int blendSrc, int blendDst, int r, int g, int b )
{
	if( !hSprite )
		return;

	int fullW = SPR_Width( hSprite, frame );
	int fullH = SPR_Height( hSprite, frame );

	if( fullW <= 0 || fullH <= 0 )
		return;

	SPR_Set( hSprite, r, g, b );

	int scaledFullW = (int)( fullW * g_flHudScale + 0.5f );
	int scaledFullH = (int)( fullH * g_flHudScale + 0.5f );

	if( prc )
	{
		int clipW = (int)( ( prc->right - prc->left ) * g_flHudScale + 0.5f );
		int clipH = (int)( ( prc->bottom - prc->top ) * g_flHudScale + 0.5f );
		int drawX = x - (int)( prc->left * g_flHudScale + 0.5f );
		int drawY = y - (int)( prc->top * g_flHudScale + 0.5f );

		SPR_EnableScissor( x, y, clipW, clipH );
		gEngfuncs.pfnSPR_DrawGeneric( frame, drawX, drawY, NULL,
			blendSrc, blendDst, scaledFullW, scaledFullH );
		SPR_DisableScissor();
	}
	else
	{
		gEngfuncs.pfnSPR_DrawGeneric( frame, x, y, NULL,
			blendSrc, blendDst, scaledFullW, scaledFullH );
	}
}

inline rect_s SPR_Rect(HSPRITE spr, int frame = 0)
{
	rect_s rc;
	rc.left = 0;
	rc.top = 0;
	rc.right = SPR_Width(spr, frame);
	rc.bottom = SPR_Height(spr, frame);
	return rc;
}

void ScaledSPR_DrawHoles( HSPRITE hSprite, int frame, int x, int y,
	const struct rect_s *prc, int r, int g, int b )
{
	DrawScaledSprite( hSprite, frame, x, y, prc,
		GL_BLEND_SRC_ALPHA, GL_BLEND_ONE_MINUS_SRC_ALPHA, r, g, b );
}

void ScaledSPR_DrawAdditive( HSPRITE hSprite, int frame, int x, int y,
	const struct rect_s *prc, int r, int g, int b )
{
	DrawScaledSprite( hSprite, frame, x, y, prc,
		GL_BLEND_ONE, GL_BLEND_ONE, r, g, b );
}
