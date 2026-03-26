//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined( GAMESTUDIOMODELRENDERER_H )
#define GAMESTUDIOMODELRENDERER_H
#if defined( _WIN32 )
#pragma once
#endif

/*
====================
CGameStudioModelRenderer

====================
*/
class CGameStudioModelRenderer : public CStudioModelRenderer
{
public:
	CGameStudioModelRenderer( void );

	virtual void Init( void );

	// Override to apply origin fix offset to root bone position so it
	// participates in the engine's sequence crossfade naturally.
	virtual void StudioCalcRotations( float pos[][3], vec4_t *q, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f );

	// Cvar to toggle origin fix table (0 = off, 1 = on)
	cvar_t *m_pCvarModelOriginFix;
};

#endif // GAMESTUDIOMODELRENDERER_H