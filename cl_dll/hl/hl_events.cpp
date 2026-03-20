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
#include "../hud.h"
#include "../cl_util.h"
#include "event_api.h"

extern "C"
{
// PE
void EV_FireDeagle( struct event_args_s *args  );
void EV_FireSeburo( struct event_args_s *args  );
void EV_FireBenelli( struct event_args_s *args  );
void EV_FireBeretta( struct event_args_s *args  );
void EV_FireM16( struct event_args_s *args  );
void EV_FireGlock( struct event_args_s *args  );
void EV_FireKnife( struct event_args_s *args  );
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

/*
======================
Game_HookEvents

Associate script file name with callback functions.  Callback's must be extern "C" so
 the engine doesn't get confused about name mangling stuff.  Note that the format is
 always the same.  Of course, a clever mod team could actually embed parameters, behavior
 into the actual .sc files and create a .sc file parser and hook their functionality through
 that.. i.e., a scripting system.

That was what we were going to do, but we ran out of time...oh well.
======================
*/
void Game_HookEvents( void )
{
// PE
	gEngfuncs.pfnHookEvent( "events/deagle.sc",					EV_FireDeagle );
	gEngfuncs.pfnHookEvent( "events/seburo.sc",					EV_FireSeburo );
	gEngfuncs.pfnHookEvent( "events/benelli.sc",				EV_FireBenelli );
	gEngfuncs.pfnHookEvent( "events/beretta.sc",				EV_FireBeretta );
	gEngfuncs.pfnHookEvent( "events/m16.sc",					EV_FireM16 );
	gEngfuncs.pfnHookEvent( "events/glock.sc",					EV_FireGlock );
	gEngfuncs.pfnHookEvent( "events/knife.sc",					EV_FireKnife );
	gEngfuncs.pfnHookEvent( "events/aug.sc",					EV_FireAug );
	gEngfuncs.pfnHookEvent( "events/minigun.sc",				EV_FireMinigun );
	gEngfuncs.pfnHookEvent( "events/microuzi.sc",				EV_FireMicrouzi );
	gEngfuncs.pfnHookEvent( "events/microuzi_a.sc",				EV_FireMicrouzi_a );
	gEngfuncs.pfnHookEvent( "events/mp5_10_1.sc",				EV_FireMP5_10 );
	gEngfuncs.pfnHookEvent( "events/usp.sc",					EV_FireUsp );
	gEngfuncs.pfnHookEvent( "events/stoner.sc",					EV_FireStoner );
	gEngfuncs.pfnHookEvent( "events/sig550.sc",					EV_FireSig550 );
	gEngfuncs.pfnHookEvent( "events/p226.sc",					EV_FireP226 );
	gEngfuncs.pfnHookEvent( "events/p225.sc",					EV_FireP225 );
	gEngfuncs.pfnHookEvent( "events/beretta_a.sc",				EV_FireBeretta_a );
	gEngfuncs.pfnHookEvent( "events/glock_auto_a.sc",			EV_FireGlock_auto_a );
	gEngfuncs.pfnHookEvent( "events/glock_auto.sc",				EV_FireGlock_auto );
	gEngfuncs.pfnHookEvent( "events/mp5sd.sc",					EV_FireMP5sd );
	gEngfuncs.pfnHookEvent( "events/tavor.sc",					EV_FireTavor );
	gEngfuncs.pfnHookEvent( "events/sawed.sc",					EV_FireSawed );
	gEngfuncs.pfnHookEvent( "events/ak47.sc",					EV_FireAK47 );
	gEngfuncs.pfnHookEvent( "events/case.sc",					EV_FireCase );
	gEngfuncs.pfnHookEvent( "events/uspmp.sc",					EV_FireUspMP );
	gEngfuncs.pfnHookEvent( "events/hand.sc",					EV_FireHand );
	gEngfuncs.pfnHookEvent( "events/flame.sc",					EV_FireFlame );
	gEngfuncs.pfnHookEvent( "events/spray.sc",					EV_FireSpray );
}
