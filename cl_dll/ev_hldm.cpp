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
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>

#include "r_studioint.h"
#include "com_model.h"
#include "bb_blood.h"

#include "studio.h"
void VectorTransform (const float *in1, float in2[3][4], float *out);
extern engine_studio_api_t IEngineStudio;
extern void InitGlobals( );

static int tracerCount[ 32 ];

static inline int *SafeTracerCount( int idx )
{
	static int dummy = 0;
	if( idx < 1 || idx > 32 )
		return &dummy;
	return &tracerCount[idx - 1];
}

extern "C" char PM_FindTextureType( char *name );

void V_PunchAxis( int axis, float punch );
void VectorAngles( const float *forward, float *angles );

extern cvar_t *cl_lw;
#define SND_CHANGE_PITCH	(1<<7)		// duplicated in protocol.h change sound pitch

int g_iGangsta = 0;
extern "C"
{

// PE
void EV_FireDeagle( struct event_args_s *args  );
void EV_FireSeburo( struct event_args_s *args  );
void EV_FireBenelli( struct event_args_s *args  );
void EV_FireBeretta( struct event_args_s *args  );
void EV_FireM16( struct event_args_s *args  );
void EV_FireGlock( struct event_args_s *args  );
void EV_FireKnife( struct event_args_s *args  ); // Fire the knife lol
void EV_FireAug( struct event_args_s *args  );
void EV_FireMinigun( struct event_args_s *args  );
void EV_FireMicrouzi( struct event_args_s *args  );
void EV_FireMicrouzi_a( struct event_args_s *args  );
void EV_FireMP5_10( struct event_args_s *args  );
void EV_FireUsp( struct event_args_s *args  );
void EV_FireStoner( struct event_args_s *args  );
void EV_FireSig550( struct event_args_s *args  );
void EV_FireP226( struct event_args_s *args  );
void EV_FireP225( struct event_args_s *args  );
void EV_FireBeretta_a( struct event_args_s *args  );
void EV_FireGlock_auto_a( struct event_args_s *args  );
void EV_FireGlock_auto( struct event_args_s *args  );
void EV_FireMP5sd( struct event_args_s *args  );
void EV_FireTavor( struct event_args_s *args  );
void EV_FireSawed( struct event_args_s *args  );
void EV_FireAK47( struct event_args_s *args  );
void EV_FireCase( struct event_args_s *args  );
void EV_FireUspMP( struct event_args_s *args  );
void EV_FireHand( struct event_args_s *args  );
void EV_FireFlame( struct event_args_s *args  );
void EV_FireSpray( struct event_args_s *args  );

void EV_TrainPitchAdjust( struct event_args_s *args );
}

#define VECTOR_CONE_1DEGREES Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES Vector( 0.06105, 0.06105, 0.06105 )	
#define VECTOR_CONE_8DEGREES Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES Vector( 0.17365, 0.17365, 0.17365 )
vec3_t PE_GetAttachment( model_s *mdl, int att )
{
  if( !mdl )
    return Vector( 0, 0, 0 );
	studiohdr_t* pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata( mdl );

	if( !pStudioHeader )
		return Vector( 0, 0, 0 );
  if( pStudioHeader->numattachments <= att )
    return Vector( 0, 0, 0 );

  pStudioHeader->name;
  mstudioattachment_t *pattachment;
	pattachment = (mstudioattachment_t *)((byte *)pStudioHeader + pStudioHeader->attachmentindex);
  return pattachment[att].org;
}

vec3_t PE_MuzzlePos( int idx, int duck )
{
	vec3_t view_ofs;

	cl_entity_t *pEnt = gEngfuncs.GetEntityByIndex( idx );
	if( !pEnt )
	{
		VectorClear( view_ofs );
		return view_ofs;
	}
	model_s * weaponModel = IEngineStudio.GetModelByIndex( pEnt->curstate.weaponmodel );
	studiohdr_t* pStudioHeader = (studiohdr_t *)IEngineStudio.Mod_Extradata(weaponModel);

	VectorClear( view_ofs );

	//view_ofs[2] = DEFAULT_VIEWHEIGHT;

	if ( EV_IsPlayer( idx ) )
	{
		/*if ( EV_IsLocal( idx ) )
		{
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
		}
		else */if ( duck == 1 )
		{
			view_ofs[2] = 8;
		}
	}
	//view_ofs[2] -= DEFAULT_VIEWHEIGHT;

	if( !weaponModel || !pStudioHeader )
		return view_ofs;
  mstudioattachment_t *pattachment;
	pattachment = (mstudioattachment_t *)((byte *)pStudioHeader + pStudioHeader->attachmentindex);

	return pattachment[0].org + view_ofs;
}

void PE_Muzzle( int idx, int type, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, int duck )
{
  vec3_t pos = PE_MuzzlePos( idx, duck );
	pos = origin + pos[0]*forward + pos[1]*right + pos[2]*up;
  gEngfuncs.pEfxAPI->R_MuzzleFlash( (float *)&pos, 1 );
}


// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play
float EV_HLDM_PlayTextureSound( int idx, pmtrace_t *ptr, float *vecSrc, float *vecEnd, int iBulletType )
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	float fvol;
	float fvolbar;
	char *rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	char *pTextureName;
	char texname[ 64 ];
	char szbuffer[ 64 ];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );

	// FIXME check if playtexture sounds movevar is set
	//

	chTextureType = 0;

	// Player or NPC with flesh sound flag
	if ( entity >= 1 && entity <= gEngfuncs.GetMaxClients() )
	{
		// hit body
		chTextureType = CHAR_TEX_FLESH;
	}
	else if ( entity > 0 )
	{
		cl_entity_t *cl_ent = gEngfuncs.GetEntityByIndex( entity );
		if ( cl_ent && ( cl_ent->curstate.eflags & EFLAG_FLESH_SOUND ) )
		{
			chTextureType = CHAR_TEX_FLESH;
		}
	}
	
	if ( chTextureType == 0 && entity == 0 )
	{
		// get texture from entity or world (world is ent(0))
		pTextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );
		
		if ( pTextureName )
		{
			strcpy( texname, pTextureName );
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			{
				pTextureName++;
			}
			
			// '}}'
			strcpy( szbuffer, pTextureName );
			szbuffer[ CBTEXTURENAMEMAX - 1 ] = 0;
				
			// get texture type
			chTextureType = PM_FindTextureType( szbuffer );	
		}
	}
	
	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE: fvol = 0.9;	fvolbar = 0.6;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_METAL: fvol = 0.9; fvolbar = 0.3;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_DIRT:	fvol = 0.9; fvolbar = 0.1;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_VENT:	fvol = 0.5; fvolbar = 0.3;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	case CHAR_TEX_GRATE: fvol = 0.9; fvolbar = 0.5;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	case CHAR_TEX_TILE:	fvol = 0.8; fvolbar = 0.2;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_SLOSH: fvol = 0.9; fvolbar = 0.0;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_WOOD: fvol = 0.9; fvolbar = 0.2;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		fvol = 0.8; fvolbar = 0.2;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_FLESH:
//		if (iBulletType == BULLET_PLAYER_CROWBAR)
		if( iBulletType == BULLET_PLAYER_KNIFE ||
		  iBulletType == BULLET_PLAYER_FLAME ||
		  iBulletType == BULLET_PLAYER_SPRAY )
			return 0.0; // crowbar already makes this sound
		fvol = 1.0;	fvolbar = 0.2;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	}

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong(0,cnt-1)], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong(0,0xf) );
	return fvolbar;
}

char *EV_HLDM_DamageDecal( physent_t *pe )
{
	static char decalname[ 32 ];
	int idx;

	if ( pe->classnumber == 1 )
	{
		idx = gEngfuncs.pfnRandomLong( 0, 2 );
		sprintf( decalname, "{break%i", idx + 1 );
	}
	else if ( pe->rendermode != kRenderNormal )
	{
		sprintf( decalname, "{bproof1" );
	}
	else
	{
		idx = gEngfuncs.pfnRandomLong( 0, 4 );
		sprintf( decalname, "{shot%i", idx + 1 );
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName )
{
	int iRand;
	physent_t *pe;

	gEngfuncs.pEfxAPI->R_BulletImpactParticles( pTrace->endpos );

	iRand = gEngfuncs.pfnRandomLong(0,0x7FFF);
	if ( iRand < (0x7fff/2) )// not every bullet makes a sound.
	{
		switch( iRand % 5)
		{
		case 0:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 1:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 2:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 3:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		case 4:	gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		}
	}

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	// Only decal brush models such as the world etc.
	if (  decalName && decalName[0] && pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot( 
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ), 
				gEngfuncs.pEventAPI->EV_IndexFromTrace( pTrace ), 0, pTrace->endpos, 0 );
		}
	}
}

void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType )
{
	physent_t *pe;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	if ( pe && pe->solid == SOLID_BSP )
	{
		/*switch( iBulletType )
		{
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		default:*/
			// smoke and decal
			EV_HLDM_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ) );
			//gEngfuncs.pEfxAPI->R_SparkStreaks(pTrace->endpos,25,-200,200);
		/*	break;
		}*/
	}
}

int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount )
{
	int tracer = 0;
	int i;
	qboolean player = idx >= 1 && idx <= gEngfuncs.GetMaxClients() ? true : false;

	if ( iTracerFreq != 0 && ( (*tracerCount)++ % iTracerFreq) == 0 )
	{
		vec3_t vecTracerSrc;

		if ( player )
		{
			vec3_t offset( 0, 0, -4 );

			// adjust tracer position for player
			for ( i = 0; i < 3; i++ )
			{
				vecTracerSrc[ i ] = vecSrc[ i ] + offset[ i ] + right[ i ] * 2 + forward[ i ] * 16;
			}
		}
		else
		{
			VectorCopy( vecSrc, vecTracerSrc );
		}
		
		if ( iTracerFreq != 1 )		// guns that always trace also always decal
			tracer = 1;

		/*switch( iBulletType )
		{
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_MONSTER_9MM:
		case BULLET_MONSTER_12MM:
		default:*/
			EV_CreateTracer( vecTracerSrc, end );
		/*	break;
		}*/
	}

	return tracer;
}

///// �bernommen... wird aus nem tut sein...
/////
void EV_HLDM_SmokePuff( pmtrace_t *pTrace, float *vecSrc, float *vecEnd )
{
	physent_t *pe;

	// get entity at endpoint
	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	if ( pe && pe->solid == SOLID_BSP )
	{	// if it's a solid wall / entity
		char chTextureType = CHAR_TEX_CONCRETE;
		char *pTextureName;
		char texname[ 64 ];
		char szbuffer[ 64 ];

		// get texture name
		pTextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( pTrace->ent, vecSrc, vecEnd );
		
		if ( pTextureName )
		{
			strcpy( texname, pTextureName );
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if (*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}

			if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
			{
				pTextureName++;
			}
			
			// '}}'
			strcpy( szbuffer, pTextureName );
			szbuffer[ CBTEXTURENAMEMAX - 1 ] = 0;
				
			// get texture type
			chTextureType = PM_FindTextureType( szbuffer );	
		}
		
		bool fDoPuffs = false;
		bool fDoSparks = false;
		int a,r,g,b;

		switch (chTextureType)
		{
			// do smoke puff and eventually add sparks
			case CHAR_TEX_TILE:
			case CHAR_TEX_CONCRETE:
				//fDoSparks = false;//(gEngfuncs.pfnRandomLong(1, 4) == 1);
				fDoPuffs = true;
				a = 128;
				r = 180;
				g = 180;
				b = 180;
				break;

			// don't draw puff, but add sparks often
			case CHAR_TEX_VENT:
			case CHAR_TEX_GRATE: 
			case CHAR_TEX_METAL: 
				fDoSparks = 1;//(gEngfuncs.pfnRandomLong(1, 2) == 1);
				break;
			
			// draw brown puff, but don't do sparks
			case CHAR_TEX_DIRT:	
			case CHAR_TEX_WOOD:
				fDoPuffs = true;
				a = 250;
				r = 97;
				g = 86;
				b = 53;
				break;
			
			// don't do anything if those textures (perhaps add something later...)
			default:
			case CHAR_TEX_GLASS:
			case CHAR_TEX_COMPUTER:
			case CHAR_TEX_SLOSH: 
				break;
		}
				
		if( fDoPuffs )
		{
			vec3_t angles, forward, right, up;

			VectorAngles( pTrace->plane.normal, angles );

			AngleVectors( angles, forward, up, right );
			forward.z = -forward.z;
 			
			// get sprite index
			int  iWallsmoke = gEngfuncs.pEventAPI->EV_FindModelIndex ("sprites/wallsmokepuff.spr");
		
			// create sprite
			TEMPENTITY *pTemp = gEngfuncs.pEfxAPI->R_TempSprite( 
				pTrace->endpos + forward,
				//forward * gEngfuncs.pfnRandomFloat(10, 30) + right * gEngfuncs.pfnRandomFloat(-6, 6) + up * gEngfuncs.pfnRandomFloat(0, 6),
				forward * gEngfuncs.pfnRandomFloat(2, 6) + right * gEngfuncs.pfnRandomFloat(-6, 6) + up * gEngfuncs.pfnRandomFloat(0, 6),
				0.8,
				iWallsmoke,
				//kRenderTransAdd,
				kRenderTransAlpha,
				kRenderFxNone,
				1.0,
				0.3,
				FTENT_ROTATE | FTENT_SPRANIMATE | FTENT_FADEOUT
			);
			TEMPENTITY *pTemp2 = gEngfuncs.pEfxAPI->R_TempSprite( 
				pTrace->endpos,
				//forward * gEngfuncs.pfnRandomFloat(10, 30) + right * gEngfuncs.pfnRandomFloat(-6, 6) + up * gEngfuncs.pfnRandomFloat(0, 6),
				forward * gEngfuncs.pfnRandomFloat(2, 6) + right * gEngfuncs.pfnRandomFloat(-6, 6) + up * gEngfuncs.pfnRandomFloat(0, 6),
				0.8,
				iWallsmoke,
				//kRenderTransAdd,
				kRenderTransAlpha,
				kRenderFxNone,
				1.0,
				0.3,
				FTENT_ROTATE | FTENT_SPRANIMATE | FTENT_FADEOUT
			);

			if(pTemp)
			{	// sprite created successfully, adjust some things
				pTemp->fadeSpeed = 3.5;
				pTemp->entity.curstate.framerate = 20.0;
				pTemp->entity.curstate.renderamt = a;
				pTemp->entity.curstate.rendercolor.r = r;
				pTemp->entity.curstate.rendercolor.g = g;
				pTemp->entity.curstate.rendercolor.b = b;
				pTemp->entity.baseline.origin = forward * 70;
				pTemp->entity.baseline.angles[1] = 0;
				pTemp->entity.baseline.angles[2] = 100;
				pTemp->entity.baseline.angles[3] = 0;
			}
			if(pTemp2)
			{	// sprite created successfully, adjust some things
				pTemp2->fadeSpeed = 3.5;
				pTemp2->entity.curstate.framerate = 20.0;
				pTemp2->entity.curstate.renderamt = a;
				pTemp2->entity.curstate.rendercolor.r = r;
				pTemp2->entity.curstate.rendercolor.g = g;
				pTemp2->entity.curstate.rendercolor.b = b;
				pTemp2->entity.baseline.origin = forward * 50;
				pTemp2->entity.baseline.angles[1] = 0;
				pTemp2->entity.baseline.angles[2] = -100;
				pTemp2->entity.baseline.angles[3] = 0;
			}
		}

		if( fDoSparks )
		{
			// spawn some sparks
			gEngfuncs.pEfxAPI->R_SparkShower( pTrace->endpos );
			//gEngfuncs.pEfxAPI->R_SparkEffect( pTrace->endpos, 20, 40, 60 );
		}
	}
}

/////
/////

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
extern int g_iShots;
extern vec3_t ev_punchangle;
#define CB_TORSO		(1<<0)
#define CB_ARMS			(1<<1)
#define CB_LEGS			(1<<2)
#define CB_HEAD			(1<<3)
#define CB_NANO			(1<<4)
#define AD_TORSO_HEART	(1<<0)
#define AD_TORSO_BOMB	(1<<1)
#define AD_ARMS_FAST	(1<<2)
#define AD_ARMS_SMART	(1<<3)
#define AD_LEGS_ACHIL	(1<<4)
#define AD_HEAD_SENTI	(1<<5)
#define AD_HEAD_ORBITAL	(1<<6)
#define AD_NANO_FACT	(1<<6)
#define AD_NANO_SKIN	(1<<7)


#define FBitSet(flBitVector, bit)		((int)(flBitVector) & (bit))
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY )
{
	int i;
//	float sprx;
	pmtrace_t tr;
	int iShot;
	int tracer;
	
	/*if( 0 && EV_IsLocal( idx ) )
	{
		flSpreadX *= ( (((float)g_iShots)/140.0f)*(((float)g_iShots)/140.0f) );
		if( FBitSet( gHUD.m_ShowMenuPE.m_sInfo.slot[6], CB_HEAD ) )
		{
			flSpreadX *= 0.66;
			flSpreadY *= 0.66;
		}
		if( FBitSet( gHUD.m_ShowMenuPE.m_sInfo.slot[6], CB_ARMS ) )
		{
			flSpreadX *= 0.58;
			flSpreadY *= 0.58;
		}
		if( flSpreadX < (flSpreadY/4.0f) )
			flSpreadX = flSpreadY / 4.0f;
	} //Spin remove*/

	for ( iShot = 1; iShot <= cShots; iShot++ )	
	{
		vec3_t vecDir, vecEnd;
			
		if ( EV_IsLocal( idx ) )
		{
			if( g_iShots < 160 )
				g_iShots += 20;

			/*if( FBitSet( gHUD.m_ShowMenuPE.m_sInfo.slot[1], AD_NANO_SKIN ) )
			{
				flSpreadX *= 1.15;
				flSpreadY *= 1.15;
			}
			if( FBitSet( gHUD.m_ShowMenuPE.m_sInfo.slot[6], CB_HEAD ) )
			{
				flSpreadX *= 0.66;
				flSpreadY *= 0.66;
			}
			if( FBitSet( gHUD.m_ShowMenuPE.m_sInfo.slot[6], CB_ARMS ) )
			{
				flSpreadX *= 0.58;
				flSpreadY *= 0.58;
			}*/

			//pPlayer->pev->punchangle.x = -pPlayer->m_iShots/60;
			/*if( g_iShots < 2 )
			{
				flSpreadX *= 0.5;
				flSpreadY *= 0.5;
			}*/
			flSpreadX *= 1 + g_iShots/200;
			flSpreadY *= 1 + g_iShots/200;

	//		ev_punchangle[0] = -g_iShots/60;
		}
		float x, y, z;
		//We randomize for the Shotgun.
		if( ( iBulletType == BULLET_PLAYER_SAWED ) ||
		    ( iBulletType == BULLET_PLAYER_BENELLI ) )
		{
			//do {
				x = gEngfuncs.pfnRandomFloat(-0.5,0.5) + gEngfuncs.pfnRandomFloat(-0.5,0.5);
				y = gEngfuncs.pfnRandomFloat(-0.5,0.5) + gEngfuncs.pfnRandomFloat(-0.5,0.5);
				x *= 1 + 160/200;
				y *= 1 + 160/200;
				z = x*x+y*y;
			//} while (z > 1);

			for ( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + x * flSpreadX * right[ i ] + y * flSpreadY * up [ i ];
				vecEnd[i] = vecSrc[ i ] + flDistance * vecDir[ i ];
			}
		}//But other guns already have their spread randomized in the synched spread.
		else
		{

			for ( i = 0 ; i < 3; i++ )
			{
				/*sprx = flSpreadY * ( (((float)g_iShots)/140.0f)*(((float)g_iShots)/140.0f) );
				if( sprx < (flSpreadY/4.0f) )
					sprx = flSpreadY / 4.0f;*/


				vecDir[i] = vecDirShooting[i] + flSpreadX * right[ i ] + flSpreadY * up [ i ];
				vecEnd[i] = vecSrc[ i ] + flDistance * vecDir[ i ];
			}
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	
		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();
	
		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_NORMAL, -1, &tr );

		tracer = EV_HLDM_CheckTracer( idx, vecSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount );

		// do damage, paint decals
		if ( tr.fraction != 1.0 )
		{
			/*switch(iBulletType)
			{
			default:
			case BULLET_PLAYER_9MM:*/		
				
				EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
				EV_HLDM_DecalGunshot( &tr, iBulletType );
				EV_HLDM_SmokePuff( &tr, vecSrc, vecEnd );
			
			/*		break;
			case BULLET_PLAYER_MP5:		
				
				if ( !tracer )
				{
					EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
					EV_HLDM_DecalGunshot( &tr, iBulletType );
					EV_HLDM_SmokePuff( &tr, vecSrc, vecEnd );
				}
				break;
			case BULLET_PLAYER_BUCKSHOT:
				
				EV_HLDM_DecalGunshot( &tr, iBulletType );
				EV_HLDM_SmokePuff( &tr, vecSrc, vecEnd );
			
				break;
			case BULLET_PLAYER_357:
				
				EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
				EV_HLDM_DecalGunshot( &tr, iBulletType );
				EV_HLDM_SmokePuff( &tr, vecSrc, vecEnd );
				
				break;

			}*/
		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

void EV_TrainPitchAdjust( event_args_t *args )
{
	int idx;
	vec3_t origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;
	
	char sz[ 256 ];

	idx = args->entindex;
	
	VectorCopy( args->origin, origin );

	us_params = (unsigned short)args->iparam1;
	stop	  = args->bparam1;

	m_flVolume	= (float)(us_params & 0x003f)/40.0;
	noise		= (int)(((us_params) >> 12 ) & 0x0007);
	pitch		= (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch ( noise )
	{
	case 1: strcpy( sz, "plats/ttrain1.wav"); break;
	case 2: strcpy( sz, "plats/ttrain2.wav"); break;
	case 3: strcpy( sz, "plats/ttrain3.wav"); break; 
	case 4: strcpy( sz, "plats/ttrain4.wav"); break;
	case 5: strcpy( sz, "plats/ttrain6.wav"); break;
	case 6: strcpy( sz, "plats/ttrain7.wav"); break;
	default:
		// no sound
		strcpy( sz, "" );
		return;
	}

	if ( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, sz );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
	}
}

int EV_TFC_IsAllyTeam( int iTeam1, int iTeam2 )
{
	return 0;
}
extern float g_fPunch;

// PE
enum mp5_e
{
	MP5_IDLE1 = 0,
	MP5_IDLE1B,
	MP5_IDLE2,
	MP5_IDLE2B,
	MP5_DRAW,
	MP5_DRAWB,
	MP5_RELOAD,
	MP5_RELOADB,
	MP5_SHOOT,
	MP5_SHOOTB,
};

void EV_FireMP5_10( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if( args->bparam1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( MP5_SHOOTB, 0 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( MP5_SHOOT, 0 );

		//V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -2, 2 ) );
		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/mp5_10-1.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	if ( gEngfuncs.GetMaxClients() > 1 )
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MP5, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
	}
	else
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MP5, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
	}
}

enum deagle_e
{
	DEAGLE_IDLE1 = 0,
	DEAGLE_DRAW,
	DEAGLE_RELOAD,
	DEAGLE_RELOAD_EMPTY,
	DEAGLE_SHOT,
	DEAGLE_SHOT_EMPTY
};

void EV_FireDeagle( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );
	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( DEAGLE_SHOT, 0 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/deagle.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_357, 0, 0, args->fparam1, args->fparam2 );
}

enum seburo_e
{
	SEBURO_IDLE = 0,
	SEBURO_SHOOT,
	SEBURO_SHOOTE,
	SEBURO_RELOAD,
	SEBURO_DRAW
};

void EV_FireSeburo( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SEBURO_SHOOT, 0 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, -8, 2 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/seburocx.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_SEBURO, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum benelli_e {
	BENELLI_IDLE = 0,
	BENELLI_SHOOT,
	BENELLI_INSERT,
	BENELLI_START_RELOAD,
	BENELLI_AFTER_RELOAD,
	BENELLI_DRAW
};

void EV_FireBenelli( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shotgunshell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( BENELLI_SHOOT, 2 );

		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -2, 2 ) );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/benelli.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 12, vecSrc, vecAiming, 2048, BULLET_PLAYER_BENELLI, 0, SafeTracerCount(idx), 0.08716, 0.04362 );
}

enum beretta_e
{
	BERETTA_IDLE = 0,
	BERETTA_SHOOT,
	BERETTA_SHOOTE,
	BERETTA_DRAW,
	BERETTA_RELOAD,
	BERETTA_RELOADE,
	BERETTA_IDLE2,
	BERETTA_SHOOT2,
	BERETTA_SHOOT2E,
	BERETTA_NTOG,
	BERETTA_GTON,
	BERETTA_RELOAD2,
	BERETTA_RELOAD2E,
};

void EV_FireBeretta( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	/*if( args->bparam1 > 1 )
	{
		g_iGangsta = args->bparam1 - 2;
		return;
	}*/

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );
	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if( args->bparam1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( BERETTA_SHOOT2, 0 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( BERETTA_SHOOT, 0 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/beretta.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_BERETTA, 0, 0, args->fparam1, args->fparam2 );
}

enum m16_e
{
	M16_DRAW = 0,
	M16_IDLE,
	M16_IDLE1,
	M16_IDLE2,
	M16_IDLE3,
	M16_SHOOT,
	M16_SHOOTBURST,
	M16_RELOAD
};

void EV_FireM16( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
    if( !args->bparam2 )
		  gEngfuncs.pEventAPI->EV_WeaponAnimation( M16_SHOOTBURST, 2 );
    else
		  gEngfuncs.pEventAPI->EV_WeaponAnimation( M16_SHOOT, 2 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/m16.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_M16, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum glock_e
{
	GLOCK_IDLE = 0,
	GLOCK_SHOOT,
	GLOCK_SHOOT2,
	GLOCK_SHOOT3,
	GLOCK_SHOOT_BURST,
	GLOCK_SHOOT_LAST,
	GLOCK_RELOAD,
	GLOCK_DRAW
};

void EV_FireGlock( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );
	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if( args->bparam1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( GLOCK_SHOOT  + gEngfuncs.pfnRandomLong(0,2), 0 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( GLOCK_SHOOT_BURST, 0 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, -4, 0, -2 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	if( args->bparam1 )
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glock.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );
	else
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glock1.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, ( args->bparam1 ? 1 : 3 ), vecSrc, vecAiming, 8192, BULLET_PLAYER_GLOCK, 0, 0, args->fparam1, args->fparam2 );
}

//int g_iSwing;
enum knife_e {
	KNIFE_IDLE = 0,
	KNIFE_HIT1,
	KNIFE_HIT2,
};

//Only predict the miss sounds, hit sounds are still played 
//server side, so players don't get the wrong idea.
void EV_FireKnife( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	
	//Play Swing sound
	if( gEngfuncs.pfnRandomLong( 0, 1 ) )
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/knife_slash-1.wav", 1, ATTN_NORM, 0, PITCH_NORM); 
	else
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/knife_slash-2.wav", 1, ATTN_NORM, 0, PITCH_NORM); 

	if ( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation( KNIFE_HIT1 + gEngfuncs.pfnRandomLong( 0, 1 ), 0 );

		/*switch( (g_iSwing++) % 3 )
		{
			case 0:
				gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK1MISS, 1 ); break;
			case 1:
				gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK2MISS, 1 ); break;
			case 2:
				gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK3MISS, 1 ); break;
		}*/
	//	gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK3MISS, 0 );
	}
}

enum aug_e
{
	AUG_IDLE = 0,
	AUG_DRAW,
	AUG_RELOAD,
	AUG_SHOT
};

void EV_FireAug( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( AUG_SHOT, 2 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/aug.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_AUG, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum minigun_e
{
	MINIGUN_IDLE1 = 0,
	MINIGUN_IDLE2,
	MINIGUN_DRAW,
	MINIGUN_SHOOT
};

void EV_FireMinigun( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MINIGUN_SHOOT, 2 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 0, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, -2, -5, -6 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/minigunfire.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );

	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MINIGUN, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum microuzi_e
{
	MICROUZI_DRAW = 0,
	MICROUZI_IDLE,
	MICROUZI_SHOOT,
	MICROUZI_RELOAD,
};

void EV_FireMicrouzi( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MICROUZI_SHOOT, 2 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/microuzi.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MICROUZI, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );


}

enum microuzi_a_e
{
	MICROUZI_A_IDLE = 0,
	MICROUZI_A_SHOOT_RIGHT,
	MICROUZI_A_SHOOT_LEFT,
	MICROUZI_A_SHOOT_BOTH,
	MICROUZI_A_RELOAD_RIGHT,
	MICROUZI_A_RELOAD_LEFT,
	MICROUZI_A_DRAW
};

void EV_FireMicrouzi_a( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if( args->bparam1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( MICROUZI_A_SHOOT_RIGHT, 0 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( MICROUZI_A_SHOOT_LEFT, 0 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	if( args->bparam1 )
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, -3, 2, 0 );
	else
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, -3, -20, 0 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/microuzi.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MICROUZI_A, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum usp_e
{
	USP_IDLE = 0,
	USP_SHOOT,
	USP_SHOOTE,
	USP_DRAW,
	USP_RELOAD,
	USP_RELOADE,
	USP_IDLE2,
	USP_SHOOT2,
	USP_SHOOT2E,
	USP_NTOG,
	USP_GTON,
	USP_RELOAD2,
	USP_RELOAD2E,
};

void EV_FireUsp( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );
	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if( args->bparam1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( USP_SHOOT2, 0 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( USP_SHOOT, 0 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/usp.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_USP, 0, 0, args->fparam1, args->fparam2 );
}

enum stoner_e
{
	STONER_IDLE = 0,
	STONER_DRAW,
	STONER_RELOAD,
	STONER_SHOOT,
};

void EV_FireStoner( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		//EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( STONER_SHOOT, 2 );

		//V_PunchAxis( 0, /*gEngfuncs.pfnRandomFloat( -5, 5 )*/-2 );
		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	//else
	//	PE_Muzzle( idx, 1, origin, forward, right, up );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/stoner.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_STONER, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum sig550_e
{
  SIG550_DRAW = 0,
	SIG550_IDLE,
	SIG550_SHOOT,
	SIG550_RELOAD
};

void EV_FireSig550( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SIG550_SHOOT, 2 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	//EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/44sw.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_SIG550, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum p226_e
{
	P226_IDLE = 0,
	P226_SHOOT,
	P226_SHOOTE,
	P226_DRAW,
	P226_RELOAD,
	P226_RELOADE,
	P226_IDLE2,
	P226_SHOOT2,
	P226_SHOOT2E,
	P226_NTOG,
	P226_GTON,
	P226_RELOAD2,
	P226_RELOAD2E,
};

void EV_FireP226( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );
	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if( args->bparam1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( P226_SHOOT2, 0 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( P226_SHOOT, 0 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/p226.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_P226, 0, 0, args->fparam1, args->fparam2 );
}

enum p225_e
{
	P225_IDLE = 0,
	P225_SHOOT,
	P225_SHOOTE,
	P225_DRAW,
	P225_RELOAD,
	P225_RELOADE,
	P225_IDLE2,
	P225_SHOOT2,
	P225_SHOOT2E,
	P225_NTOG,
	P225_GTON,
	P225_RELOAD2,
	P225_RELOAD2E,
};

void EV_FireP225( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );
	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if( args->bparam1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( P225_SHOOT2, 0 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( P225_SHOOT, 0 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/p225.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_P225, 0, 0, args->fparam1, args->fparam2 );
}

enum beretta_a_e
{
	BERETTA_A_IDLE1 = 0,
	BERETTA_A_IDLE2,
	BERETTA_A_IDLE3,
	BERETTA_A_IDLE4,
	BERETTA_A_IDLE5,
	BERETTA_A_IDLE_E,
	BERETTA_A_DRAW,
	BERETTA_A_RELOAD,
	BERETTA_A_SHOOT_L,
	BERETTA_A_SHOOT_R,
	BERETTA_A_SHOOT_LE,
	BERETTA_A_SHOOT_RE,
	BERETTA_A_HOLSTER,
	BERETTA_A_DRAW_EMPTY,
	BERETTA_A_DRAW_SEMIEMPTY
};

void EV_FireBeretta_a( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );
	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if( args->bparam2 )
		bSide = false;

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if ( !bSide )
		{
			//if( args->bparam1 )
			//	gEngfuncs.pEventAPI->EV_WeaponAnimation( BERETTA_A_SHOOT_RE, 0 );
			//else
				gEngfuncs.pEventAPI->EV_WeaponAnimation( BERETTA_A_SHOOT_R, 0 );

			bSide = TRUE;
		}
		else
		{
			//if( args->bparam1 )
			//	gEngfuncs.pEventAPI->EV_WeaponAnimation( BERETTA_A_SHOOT_LE, .0 );
			//else
			//{
				gEngfuncs.pEventAPI->EV_WeaponAnimation( BERETTA_A_SHOOT_L, .0 );
			//}

			bSide = FALSE;
			//bODir = TRUE;
		
			for ( int i = 0; i < 3; i++ )
			{
				origin[i]  -= right[i] * ( 12 );
			}
		}

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	if( bSide )
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, -2 );
	else
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, 0, -10, 0 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/beretta_a.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_BERETTA_A, 0, 0, args->fparam1, args->fparam2 );
}

enum glock_auto_a_e
{
	GLOCK_AUTO_A_IDLE = 0,
	GLOCK_AUTO_A_IDLE_LE,
	GLOCK_AUTO_A_IDLE_RE,
	GLOCK_AUTO_A_IDLE_2E,
	GLOCK_AUTO_A_SHOOT_L,
	GLOCK_AUTO_A_SHOOT_R,
	GLOCK_AUTO_A_SHOOT_LE,
	GLOCK_AUTO_A_SHOOT_RE,
	GLOCK_AUTO_A_SHOOT_L_RE,
	GLOCK_AUTO_A_SHOOT_R_LE,
	GLOCK_AUTO_A_SHOOT_LE_RE,
	GLOCK_AUTO_A_SHOOT_RE_LE,
	GLOCK_AUTO_A_SHOOT_BOTH,
	GLOCK_AUTO_A_SHOOT_BOTH2,
	GLOCK_AUTO_A_SHOOT_BOTH_LE,
	GLOCK_AUTO_A_SHOOT_BOTH_RE,
	GLOCK_AUTO_A_SHOOT_BOTH_2E,
	GLOCK_AUTO_A_RELOAD_R,
	GLOCK_AUTO_A_RELOAD_L,
	GLOCK_AUTO_A_RELOAD_R_2E,
	GLOCK_AUTO_A_DRAW
};

void EV_FireGlock_auto_a( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		if( args->bparam1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( GLOCK_AUTO_A_SHOOT_R, 0 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( GLOCK_AUTO_A_SHOOT_L, .0 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	if( args->bparam1 )
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, -4, 0, -2 );
	else
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, -4, -20, -2 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glock.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_GLOCK_AUTO_A, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum glock_auto_e
{
	GLOCK_AUTO_IDLE = 0,
	GLOCK_AUTO_SHOOT,
	GLOCK_AUTO_SHOOTE,
	GLOCK_AUTO_RELOAD,
	GLOCK_AUTO_DRAW
};

void EV_FireGlock_auto( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( GLOCK_AUTO_SHOOT, 0 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4, -4, 0, -2 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glock.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_GLOCK_AUTO, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum mp5sd_e
{
	MP5SD_IDLE = 0,
	MP5SD_SHOOT,
	MP5SD_DRAW,
	MP5SD_DOWN,
	MP5SD_RELOAD
};

void EV_FireMP5sd( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		//EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MP5SD_SHOOT, 2 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	//else
	//	PE_Muzzle( idx, 1, origin, forward, right, up );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/mp5sd.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MP5SD, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum tavor_e
{
	TAVOR_DEPLOY = 0,
	TAVOR_IDLE,
	TAVOR_SHOOT1,
	TAVOR_STARTRELOAD,
	TAVOR_RELOAD,
	TAVOR_STOPRELOAD,
};

void EV_FireTavor( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		//EV_MuzzleFlash();
    gEngfuncs.pEventAPI->EV_WeaponAnimation( TAVOR_SHOOT1, 2 );

		//V_PunchAxis( 0, /*gEngfuncs.pfnRandomFloat( -5, 5 )*/-2 );
		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	//else
	//	PE_Muzzle( idx, 1, origin, forward, right, up );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	//gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/stoner.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_STONER, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum sawed_e {
	SAWED_DEPLOY = 0,
	SAWED_IDLE,
	SAWED_SHOOT,
	SAWED_RELOAD
};

void EV_FireSawed( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	//shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shotgunshell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SAWED_SHOOT, 2 );

		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -2, 2 ) );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 32, -12, 6 );

	//EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHOTSHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/spas12.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 12, vecSrc, vecAiming, 2048, BULLET_PLAYER_SAWED, 0, SafeTracerCount(idx), 0.08716, 0.04362 );
}

enum ak47_e
{
	AK47_IDLE = 0,
	AK47_SHOOT,
	AK47_RELOAD,
	AK47_DRAW
};

void EV_FireAK47( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( AK47_SHOOT, 2 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/ak47.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_AK47, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum case_e {
	CASE_IDLE = 0,
	CASE_DRAW,
	CASE_HOLSTER,
	CASE_ATTACKMISS,
	CASE_ATTACKHIT
};

int g_iSwing;

void EV_FireCase( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	
	//Play Swing sound
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM); 

	if ( EV_IsLocal( idx ) )
	{
		//gEngfuncs.pEventAPI->EV_WeaponAnimation( KNIFE_SCHLAG, 0 );

		/*switch( (g_iSwing++) % 3 )
		{
			case 0:
				gEngfuncs.pEventAPI->EV_WeaponAnimation ( CASE_ATTACK1MISS, 1 ); break;
			case 1:
				gEngfuncs.pEventAPI->EV_WeaponAnimation ( CASE_ATTACK2, 1 ); break;
			case 2:
				gEngfuncs.pEventAPI->EV_WeaponAnimation ( CASE_ATTACK3, 1 ); break;
		}*/
		gEngfuncs.pEventAPI->EV_WeaponAnimation ( CASE_ATTACKMISS, 1 );
		g_iSwing++;
		//gEngfuncs.pEventAPI->EV_WeaponAnimation ( CROWBAR_ATTACK3MISS, 0 );
	}
}

enum uspmp_e
{
	USPMP_IDLE = 0,
	USPMP_SHOOT,
	USPMP_SHOOTE,
	USPMP_RELOAD,
	USPMP_DRAW
};

void EV_FireUspMP( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( USPMP_SHOOT, 0 );

		V_PunchAxis( 0, -g_fPunch * (float)g_iShots );
	}
	else
		PE_Muzzle( idx, 1, origin, forward, right, up, args->ducking );

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/usp.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_USPMP, 2, SafeTracerCount(idx), args->fparam1, args->fparam2 );
}

enum hand_e
{
	HAND_IDLE = 0,
	HAND_ATTACK,
	HAND_ATTACK2,
};

void EV_FireHand( event_args_t *args )
{
	int idx;
	Vector origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );

	if( gEngfuncs.pfnRandomLong( 0, 1 ) )
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "zombie/hit.wav", 1, ATTN_NORM, 0, PITCH_NORM); 
	else
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "zombie/hit2.wav", 1, ATTN_NORM, 0, PITCH_NORM); 

	if ( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation( HAND_ATTACK + gEngfuncs.pfnRandomLong( 0, 1 ), 0 );
	}
}

enum flame_e
{
	FLAME_IDLE = 0,
	FLAME_FIRESTART,
	FLAME_FIRE,
	FLAME_FIRESTOP,
};
extern float flaming;
extern globalvars_t *gpGlobals;

void EV_FireFlame( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	
  vecSrc = PE_MuzzlePos( idx, args->ducking );
	if ( EV_IsLocal( idx ) )
	{
		if( flaming == 0 )
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation( FLAME_FIRESTART, 2 );
      if( !gpGlobals )
        InitGlobals( );
		  flaming = gpGlobals->time + 1.0;
			//char text[512];
			//sprintf( text, "starting %f %f\n", flaming, gpGlobals->time );
			//ConsolePrint( text );
		}
		else if( flaming > gpGlobals->time && ( flaming - gpGlobals->time ) <= 0.2 )
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation( FLAME_FIRE, 2 );
      if( !gpGlobals )
        InitGlobals( );
			flaming = gpGlobals->time + 0.5;
			//char text[512];
			//sprintf( text, "during %f %f\n", flaming, gpGlobals->time );
			//ConsolePrint( text );
		}
		//cBlood::NewSpray( "partsys/smoke.cfg", vecSrc /*- origin/*Vector( 0, 0, 13 )*/, Vector( 0, 0, 1 ), idx );
	}
  cBlood::NewSpray( "partsys/flame.cfg", vecSrc - Vector( 0, 0, -5 ), Vector( 1, 1, 1 ), idx );
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/flame.wav", 2, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	VectorCopy( forward, vecAiming );
}

void EV_FireSpray( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	
  vecSrc = PE_MuzzlePos( idx, args->ducking );
	if ( EV_IsLocal( idx ) )
	{
		//gEngfuncs.pEventAPI->EV_WeaponAnimation( MINIGUN_SHOOT, 2 );
    cBlood::NewSpray( "partsys/smoke.cfg", vecSrc /*- origin/*Vector( 0, 0, 13 )*/, Vector( 0, 0, 1 ), idx );
	}
  cBlood::NewSpray( "partsys/spray.cfg", vecSrc /*- origin/*Vector( 0, 0, 13 )*/, Vector( 1, 1, 1 ), idx );

	VectorCopy( forward, vecAiming );
}
