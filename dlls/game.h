/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

#ifndef GAME_H
#define GAME_H

#include "../cl_dll/bb_sprays.h"

extern void GameDLLInit( void );


extern cvar_t	displaysoundlist;

// multiplayer server rules
extern cvar_t	expperlevel;
extern cvar_t	exppotence;
extern cvar_t	maxlevel;
extern cvar_t	savexp;

extern cvar_t	diff;
extern cvar_t	missions;
extern cvar_t	gameplay;
extern cvar_t	roundtime;
extern cvar_t	rounddelay;

extern cvar_t	playerinfofile;

extern cvar_t	gibcnt;
extern cvar_t	zombiecnt;

extern cvar_t	teamspect;
extern cvar_t	showdmg;
extern cvar_t	autoteam;
extern cvar_t	mapvote;
extern cvar_t	varvote;
extern cvar_t	peversion;
extern cvar_t	recoilratio;
extern cvar_t	speedratio;
extern cvar_t	dmgratio;
extern cvar_t	setsyn;
extern cvar_t	setsec;
extern cvar_t	plkill;
extern cvar_t	restartround;
extern cvar_t	allow_spectators;
extern cvar_t	teamplay;
extern cvar_t	teamscorelimit;
extern cvar_t	fraglimit;
extern cvar_t	timelimit;
extern cvar_t	friendlyfire;
extern cvar_t	falldamage;
extern cvar_t	weaponstay;
extern cvar_t	forcerespawn;
extern cvar_t	flashlight;
extern cvar_t	aimcrosshair;
extern cvar_t	decalfrequency;
extern cvar_t	teamlist;
extern cvar_t	teamoverride;
extern cvar_t	defaultteam;
extern cvar_t	allowmonsters;
extern cvar_t	zombie_behavior;
extern cvar_t	zombie_yawspeed;
extern cvar_t	zombie_lifetime;
extern cvar_t	zombie_drop_height;
extern cvar_t	zombie_slide_angle;

// Engine Cvars
extern cvar_t	*g_psv_gravity;
extern cvar_t	*g_psv_aim;
extern cvar_t	*g_psv_allow_autoaim;
extern cvar_t	*g_footsteps;
extern cvar_t	*g_psv_cheats;

#endif		// GAME_H
