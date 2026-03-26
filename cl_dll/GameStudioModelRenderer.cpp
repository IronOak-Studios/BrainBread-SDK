//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include <assert.h>
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"
#include "Exports.h"

//
// Override the StudioModelRender virtual member functions here to implement custom bone
// setup, blending, etc.
//

// Global engine <-> studio model rendering code interface
extern engine_studio_api_t IEngineStudio;

// The renderer object, created on the stack.
CGameStudioModelRenderer g_StudioRenderer;

//
// Per-model / per-sequence origin fix table.
// Checked in order; first match wins.
// Use sequence -1 to match all sequences for a model.
//
struct s_seqOriginFix
{
	const char *modelSubstring;
	int         sequence;       // -1 = all sequences
	float       forward;        // offset along model's forward axis
	float       right;          // offset along model's right axis
	float       up;             // offset along model's up axis
};

static const s_seqOriginFix g_SeqOriginFixes[] =
{
	// Per-sequence overrides go first (first match wins).
	// { "modelname",  seq,  forward,  right,  up },

	// Whole-model defaults (sequence -1 = all).
	{ "zombie",       26,     0.0f,   0.0f,   0.0f }, // attack
	{ "zombie",       -1,   -15.0f,   0.0f,   0.0f },
	{ "fred",         26,   -10.0f,   0.0f,   0.0f }, // attack
	{ "fred",          2,   -25.0f,   0.0f,   0.0f }, // deep idle
	{ "fred",         -1,   -20.0f,   0.0f,   0.0f },
};

#define NUM_SEQ_ORIGIN_FIXES  (sizeof(g_SeqOriginFixes) / sizeof(g_SeqOriginFixes[0]))

/*
====================
CGameStudioModelRenderer

====================
*/
CGameStudioModelRenderer::CGameStudioModelRenderer( void )
{
	m_pCvarModelOriginFix = NULL;
}

/*
====================
Init

====================
*/
void CGameStudioModelRenderer::Init( void )
{
	// Call base class init first
	CStudioModelRenderer::Init();

	// Toggle for the per-model/per-sequence origin fix table (0 = off, 1 = on).
	m_pCvarModelOriginFix = gEngfuncs.pfnRegisterVariable( "cl_model_origin_fix", "1", 0 );
}

/*
====================
StudioCalcRotations

Override to apply origin fix offset to the root bone position.
Applied here (rather than to the rotation matrix) so the offset
participates in the engine's sequence crossfade when animations change.
====================
*/
void CGameStudioModelRenderer::StudioCalcRotations( float pos[][3], vec4_t *q, mstudioseqdesc_t *pseqdesc, mstudioanim_t *panim, float f )
{
	CStudioModelRenderer::StudioCalcRotations( pos, q, pseqdesc, panim, f );

	if ( !m_pCvarModelOriginFix || m_pCvarModelOriginFix->value == 0.0f )
		return;

	if ( !m_pRenderModel || !m_pRenderModel->name || !m_pStudioHeader )
		return;

	// Derive the sequence index from the descriptor pointer
	mstudioseqdesc_t *pseqbase = (mstudioseqdesc_t *)( (byte *)m_pStudioHeader + m_pStudioHeader->seqindex );
	int seq = (int)( pseqdesc - pseqbase );

	if ( seq < 0 || seq >= m_pStudioHeader->numseq )
		return;

	// Look up offset for this model + sequence
	for ( int i = 0; i < (int)NUM_SEQ_ORIGIN_FIXES; i++ )
	{
		const s_seqOriginFix *fix = &g_SeqOriginFixes[i];

		if ( !strstr( m_pRenderModel->name, fix->modelSubstring ) )
			continue;

		if ( fix->sequence != -1 && fix->sequence != seq )
			continue;

		// Apply offset to the root bone (parent == -1) in model-local space
		mstudiobone_t *pbones = (mstudiobone_t *)( (byte *)m_pStudioHeader + m_pStudioHeader->boneindex );
		for ( int j = 0; j < m_pStudioHeader->numbones; j++ )
		{
			if ( pbones[j].parent == -1 )
			{
				pos[j][0] += fix->forward;
				pos[j][1] += fix->right;
				pos[j][2] += fix->up;
				break;
			}
		}
		break;  // first table match wins
	}
}

////////////////////////////////////
// Hooks to class implementation
////////////////////////////////////

/*
====================
R_StudioDrawPlayer

====================
*/
int R_StudioDrawPlayer( int flags, entity_state_t *pplayer )
{
	return g_StudioRenderer.StudioDrawPlayer( flags, pplayer );
}

/*
====================
R_StudioDrawModel

====================
*/
int R_StudioDrawModel( int flags )
{
	return g_StudioRenderer.StudioDrawModel( flags );
}

/*
====================
R_StudioInit

====================
*/
void R_StudioInit( void )
{
	g_StudioRenderer.Init();
}

// The simple drawing interface we'll pass back to the engine
r_studio_interface_t studio =
{
	STUDIO_INTERFACE_VERSION,
	R_StudioDrawModel,
	R_StudioDrawPlayer,
};

/*
====================
HUD_GetStudioModelInterface

Export this function for the engine to use the studio renderer class to render objects.
====================
*/
int CL_DLLEXPORT HUD_GetStudioModelInterface( int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio )
{
//	RecClStudioInterface(version, ppinterface, pstudio);

	if ( version != STUDIO_INTERFACE_VERSION )
		return 0;

	// Point the engine to our callbacks
	*ppinterface = &studio;

	// Copy in engine helper functions
	memcpy( &IEngineStudio, pstudio, sizeof( IEngineStudio ) );

	// Initialize local variables, etc.
	R_StudioInit();

	// Success
	return 1;
}
