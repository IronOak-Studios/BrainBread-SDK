/***
*
*   hud_scale.h
*
*   HUD scaling support for modern resolutions.
 *   Base design height is 720 -- the HUD looks correct at 720p
 *   with the existing 640-tier sprite assets.
*
****/

#ifndef HUD_SCALE_H
#define HUD_SCALE_H

#include "wrect.h"
#include "cl_dll.h"

#define HUD_BASE_HEIGHT 720.f

// Global HUD scale factor (ScreenHeight / 720.0 by default, clamped >= 1).
// Updated every frame from the "hud_scale" cvar.
extern float g_flHudScale;

// Register the cvar.  Call once from CHud::Init().
void HudScale_Init( void );

// Recompute g_flHudScale.  Call from CHud::VidInit() and every frame.
void HudScale_Update( void );

// --------------------------------------------------------------------------
// Scaled sprite drawing -- self-contained replacements for SPR_Set + SPR_Draw*.
// These call SPR_Set internally, so callers do NOT need a separate SPR_Set.
// --------------------------------------------------------------------------

void ScaledSPR_DrawHoles( HSPRITE hSprite, int frame, int x, int y,
	const struct rect_s *prc, int r, int g, int b, float assetScale = 1.0f );

void ScaledSPR_DrawAdditive( HSPRITE hSprite, int frame, int x, int y,
	const struct rect_s *prc, int r, int g, int b, float assetScale = 1.0f );

// --------------------------------------------------------------------------
// Scale a value from 720p design space to current screen pixels.
// --------------------------------------------------------------------------

inline int HudScale( int v )
{
	return (int)( v * g_flHudScale + 0.5f );
}

inline float HudScaleF( float v )
{
	return v * g_flHudScale;
}

#endif // HUD_SCALE_H
