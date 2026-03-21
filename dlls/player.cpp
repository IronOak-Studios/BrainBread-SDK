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
/*

===== player.cpp ========================================================

  functions dealing with the player

*/

#include "extdll.h"
#include "util.h"

#include "cbase.h"
#include "player.h"
#include "trains.h"
#include "nodes.h"
#include "weapons.h"
#include "soundent.h"
#include "monsters.h"
#include "shake.h"
#include "decals.h"
#include "gamerules.h"
#include "game.h"
#include "pe_utils.h"
#include "pe_menus.h"
#include "pe_mapobjects.h"
#include "hltv.h"
#include "pe_notify.h"
#include "pe_rules.h"
#include "pe_rules_hacker.h"
#include "pe_notify.h"
#include "../cl_dll/pe_help_topics.h"

// #define DUCKFIX

extern DLL_GLOBAL ULONG		g_ulModelIndexPlayer;
extern DLL_GLOBAL BOOL		g_fGameOver;
extern DLL_GLOBAL	BOOL	g_fDrawLines;
int gEvilImpulse101;
extern DLL_GLOBAL int		g_iSkillLevel, gDisplayTitle;
#define BASE_SPEED (230.0f*speedratio.value)
#define SPEED_FORMULA ( ( PLR_SPEED(this) * ( pev->fuser4 ? 1.25 : 1 ) / 100.0f ) * BASE_SPEED )


BOOL gInitHUD = TRUE;

extern void CopyToBodyQue(entvars_t* pev);
extern void respawn(entvars_t *pev, BOOL fCopyCorpse);
extern Vector VecBModelOrigin(entvars_t *pevBModel );
extern edict_t *EntSelectSpawnPoint( CBasePlayer *pPlayer );
extern BOOL FEntIsVisible(entvars_t* pev, entvars_t* pevTarget);

s_spawnlist gListCorp[MAX_SPAWN_SPOTS];
s_spawnlist gListSynd[MAX_SPAWN_SPOTS];
s_spawnlist gListBackup[MAX_SPAWN_SPOTS];
// the world node graph
extern CGraph	WorldGraph;

#define TRAIN_ACTIVE	0x80 
#define TRAIN_NEW		0xc0
#define TRAIN_OFF		0x00
#define TRAIN_NEUTRAL	0x01
#define TRAIN_SLOW		0x02
#define TRAIN_MEDIUM	0x03
#define TRAIN_FAST		0x04 
#define TRAIN_BACK		0x05

#define CB_TORSO		(1<<0)
#define CB_ARMS			(1<<1)
#define CB_LEGS			(1<<2)
#define CB_HEAD			(1<<3)
#define CB_NANO			(1<<4)

#define	FLASH_DRAIN_TIME	 1.2 //100 units/3 minutes
#define	FLASH_CHARGE_TIME	 0.2 // 100 units/20 seconds  (seconds per unit)

#define DMG_INFO_DELAY 1.5f
#define CYBERBOMB_DAMAGE 240
#define MATE_DMG_BONUS 0.05

// Global Savedata for player
TYPEDESCRIPTION	CBasePlayer::m_playerSaveData[] = 
{
	DEFINE_FIELD( CBasePlayer, m_flFlashLightTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_iFlashBattery, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonReleased, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, m_rgItems, FIELD_INTEGER, MAX_ITEMS ),
	DEFINE_FIELD( CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER ),

	DEFINE_FIELD( CBasePlayer, m_flTimeStepSound, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flSwimTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flDuckTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flWallJumpTime, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_flSuitUpdate, FIELD_TIME ),
	DEFINE_ARRAY( CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST ),
	DEFINE_FIELD( CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER ),
	DEFINE_ARRAY( CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT ),
	DEFINE_ARRAY( CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT ),
	DEFINE_FIELD( CBasePlayer, m_lastDamageAmount, FIELD_INTEGER ),

	DEFINE_ARRAY( CBasePlayer, m_rgpPlayerItems, FIELD_CLASSPTR, MAX_ITEM_TYPES ),
	DEFINE_FIELD( CBasePlayer, m_pActiveItem, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayer, m_pLastItem, FIELD_CLASSPTR ),
	
	DEFINE_ARRAY( CBasePlayer, m_rgAmmo, FIELD_INTEGER, MAX_AMMO_SLOTS ),
	DEFINE_FIELD( CBasePlayer, m_idrowndmg, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_idrownrestored, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_tSneaking, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_iTrain, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flFallVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iTargetVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponFlash, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_fLongJump, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_fInitHUD, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_tbdPrev, FIELD_TIME ),

	DEFINE_FIELD( CBasePlayer, m_pTank, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_iHideHUD, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iFOV, FIELD_INTEGER ),
	
	//DEFINE_FIELD( CBasePlayer, m_fDeadTime, FIELD_FLOAT ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_fGameHUDInitialized, FIELD_INTEGER ), // only used in multiplayer games
	//DEFINE_FIELD( CBasePlayer, m_flStopExtraSoundTime, FIELD_TIME ),
	//DEFINE_FIELD( CBasePlayer, m_fKnownItem, FIELD_INTEGER ), // reset to zero on load
	//DEFINE_FIELD( CBasePlayer, m_iPlayerSound, FIELD_INTEGER ),	// Don't restore, set in Precache()
	//DEFINE_FIELD( CBasePlayer, m_pentSndLast, FIELD_EDICT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRoomtype, FIELD_FLOAT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flSndRange, FIELD_FLOAT ),	// Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fNewAmmo, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_flgeigerRange, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_flgeigerDelay, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_igeigerRangePrev, FIELD_FLOAT ),	// Don't restore, reset in Precache()
	//DEFINE_FIELD( CBasePlayer, m_iStepLeft, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_ARRAY( CBasePlayer, m_szTextureName, FIELD_CHARACTER, CBTEXTURENAMEMAX ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_chTextureType, FIELD_CHARACTER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fNoPlayerSound, FIELD_BOOLEAN ), // Don't need to restore, debug
	//DEFINE_FIELD( CBasePlayer, m_iUpdateTime, FIELD_INTEGER ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_iClientHealth, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientBattery, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_iClientHideHUD, FIELD_INTEGER ), // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_fWeapon, FIELD_BOOLEAN ),  // Don't restore, client needs reset
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't restore, depends on server message after spawning and only matters in multiplayer
	//DEFINE_FIELD( CBasePlayer, m_vecAutoAim, FIELD_VECTOR ), // Don't save/restore - this is recomputed
	//DEFINE_ARRAY( CBasePlayer, m_rgAmmoLast, FIELD_INTEGER, MAX_AMMO_SLOTS ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_fOnTarget, FIELD_BOOLEAN ), // Don't need to restore
	//DEFINE_FIELD( CBasePlayer, m_nCustomSprayFrames, FIELD_INTEGER ), // Don't need to restore
	
};	


int giPrecacheGrunt = 0;
int gmsgShake = 0;
int gmsgFade = 0;
int gmsgSelAmmo = 0;
int gmsgFlashlight = 0;
int gmsgResetHUD = 0;
int gmsgInitHUD = 0;
int gmsgShowGameTitle = 0;
int gmsgCurWeapon = 0;
int gmsgHealth = 0;
int gmsgDamage = 0;
int gmsgBattery = 0;
int gmsgTrain = 0;
int gmsgLogo = 0;
int gmsgWeaponList = 0;
int gmsgAmmoX = 0;
int gmsgHudText = 0;
int gmsgDeathMsg = 0;
int gmsgScoreInfo = 0;
int gmsgTeamInfo = 0;
int gmsgTeamScore = 0;
int gmsgMOTD = 0;
int gmsgServerName = 0;
int gmsgAmmoPickup = 0;
int gmsgWeapPickup = 0;
int gmsgItemPickup = 0;
int gmsgHideWeapon = 0;
int gmsgSetCurWeap = 0;
int gmsgSayText = 0;
int gmsgTextMsg = 0;
int gmsgSetFOV = 0;
int gmsgShowMenu = 0;
int gmsgGeigerRange = 0;
int gmsgRadar = 0; // by ste0
int gmsgRadarOff = 0; // by ste0
int gmsgNVG = 0;
int gmsgNVGActive = 0;

int gmsgStatusText = 0;
int gmsgStatusValue = 0; 


// Spin
int gmsgSpectator = 0;
int gmsgCounter = 0;
int gmsgIntro = 0;
int gmsgNotify = 0;
int gmsgAddNotify = 0;
int gmsgMates = 0;
int gmsgCntDown = 0;
int gmsgShowMenuPE = 0;
int gmsgClass = 0;
int gmsgBriefing = 0;
int gmsgTargHlth = 0;
int gmsgStats = 0;
int gmsgLensRef = 0;
int gmsgSpecial = 0;
int gmsgRecoilRatio = 0;
int gmsgPlayMusic = 0;
int gmsgSpray = 0;
int gmsgSetVol = 0;
//int gmsgFog = 0;
//int gmsgRain = 0;
int gmsgUpdPoints = 0;
int gmsgPETeam = 0;
int gmsgSmallCnt = 0;
int gmsgHelp = 0;
int gmsgVGUIMenu = 0;

void LinkUserMessages( void )
{
	// Already taken care of?
	if ( gmsgSelAmmo )
	{
		return;
	}

	gmsgSelAmmo = REG_USER_MSG("SelAmmo", sizeof(SelAmmo));
	gmsgGeigerRange = REG_USER_MSG("Geiger", 1);
	gmsgFlashlight = REG_USER_MSG("Flashlight", 2);
	gmsgHealth = REG_USER_MSG( "Health", 2 );
	gmsgDamage = REG_USER_MSG( "Damage", 12 );
	gmsgBattery = REG_USER_MSG( "Battery", 2);
	gmsgTrain = REG_USER_MSG( "Train", 1);
	gmsgHudText = REG_USER_MSG( "HudText", -1 );
	gmsgSayText = REG_USER_MSG( "SayText", -1 );
	gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );
	gmsgWeaponList = REG_USER_MSG("WeaponList", -1);
	gmsgResetHUD = REG_USER_MSG("ResetHUD", 1);		// called every respawn
	gmsgInitHUD = REG_USER_MSG("InitHUD", 0 );		// called every time a new player joins the server
	gmsgShowGameTitle = REG_USER_MSG("GameTitle", 1);
	gmsgDeathMsg = REG_USER_MSG( "DeathMsg", -1 );
	gmsgScoreInfo = REG_USER_MSG( "ScoreInfo", 6 ); // 9
	gmsgMOTD = REG_USER_MSG( "MOTD", -1 );
	gmsgServerName = REG_USER_MSG( "ServerName", -1 );
	gmsgAmmoPickup = REG_USER_MSG( "AmmoPickup", 2 );
	gmsgWeapPickup = REG_USER_MSG( "WeapPickup", 1 );
	gmsgItemPickup = REG_USER_MSG( "ItemPickup", -1 );
	gmsgHideWeapon = REG_USER_MSG( "HideWeapon", 1 );
	gmsgSetFOV = REG_USER_MSG( "SetFOV", 1 );
	gmsgShowMenu = REG_USER_MSG( "ShowMenu", -1 );
	gmsgShake = REG_USER_MSG("ScreenShake", sizeof(ScreenShake));
	gmsgFade = REG_USER_MSG("ScreenFade", sizeof(ScreenFade));
	gmsgAmmoX = REG_USER_MSG("AmmoX", 2);
	// + ste0
	gmsgRadar = REG_USER_MSG ( "Radar", -1);
	gmsgRadarOff = REG_USER_MSG ( "RadarOff", -1);
	// - ste0

	gmsgStatusText = REG_USER_MSG("StatusText", -1);
	gmsgStatusValue = REG_USER_MSG("StatusValue", 3); 


    gmsgNVG = REG_USER_MSG("NVG", 1);
    gmsgNVGActive = REG_USER_MSG("NVGActive", 1);
	// Spin
	gmsgCurWeapon = REG_USER_MSG( "CurWeapon", 5 );
	gmsgSpectator = REG_USER_MSG( "Spectator", 2 );
	gmsgCounter = REG_USER_MSG( "Counter", 2);
	gmsgShowMenuPE = REG_USER_MSG("ShowMenuPE", 1);
	gmsgIntro = REG_USER_MSG("Intro", 2);
	gmsgMates = REG_USER_MSG("Mates", 1);
	gmsgCntDown = REG_USER_MSG("CntDown", 1);
	gmsgNotify = REG_USER_MSG("Notify", -1);
	gmsgAddNotify = REG_USER_MSG("AddNotify", -1 );
	gmsgTeamInfo = REG_USER_MSG( "TeamInfo", 2 );
	gmsgClass = REG_USER_MSG( "Class", 2 );
	gmsgTeamScore = REG_USER_MSG( "TeamScore", 4 );
	gmsgBriefing = REG_USER_MSG( "Briefing", 1 );
	gmsgTargHlth = REG_USER_MSG( "TargHlth", 2 );
	gmsgStats = REG_USER_MSG( "Stats", 4 );
	gmsgLensRef = REG_USER_MSG( "LensRef", -1 );
	gmsgSpecial = REG_USER_MSG( "Special", 2 );
	gmsgRecoilRatio = REG_USER_MSG( "RecoilRatio", 2 );
	gmsgPlayMusic = REG_USER_MSG( "PlayMusic", -1 );
	gmsgSpray = REG_USER_MSG( "Spray", 19 );
	gmsgSetVol = REG_USER_MSG( "SetVol", 2 );
//	gmsgFog = REG_USER_MSG( "Fog", 8 );
	//gmsgRain = REG_USER_MSG( "Rain", -1 );
	gmsgUpdPoints = REG_USER_MSG( "UpdPoints", 4 );
	gmsgPETeam = REG_USER_MSG( "PETeam", 1 );
	gmsgSmallCnt = REG_USER_MSG( "SmallCnt", -1 );
	gmsgHelp = REG_USER_MSG( "Help", 4 );
	gmsgVGUIMenu = REG_USER_MSG( "VGUIMenu", 1 );
  
}

LINK_ENTITY_TO_CLASS( player, CBasePlayer );

// + Spinator
int CountClients( )
{
	int p = 0;
	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer && strlen( STRING( pPlayer->pev->netname ) ) )
			p++;
	}
	return p;
}

static unsigned short FixedUnsigned16( float value, float scale ) //FX
{	
	int output;
	output = value * scale;
	if ( output < 0 )
	output = 0;
	if ( output > 0xFFFF )
		output = 0xFFFF;

	return (unsigned short)output;
}

void UTIL_EdictScreenFadeBuild( ScreenFade &fade, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags )
{
	fade.duration = FixedUnsigned16( fadeTime, 1<<12 ); // 4.12 fixed
	fade.holdTime = FixedUnsigned16( fadeHold, 1<<12 ); // 4.12 fixed
	fade.r = (int)color.x;
	fade.g = (int)color.y;
	fade.b = (int)color.z;
	fade.a = alpha;
	fade.fadeFlags = flags;
}

void UTIL_EdictScreenFadeWrite( const ScreenFade &fade, edict_s *edict )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgFade, NULL, edict ); // use the magic #1 for "one client"
		WRITE_SHORT( fade.duration ); // fade lasts this long
		WRITE_SHORT( fade.holdTime ); // fade lasts this long
		WRITE_SHORT( fade.fadeFlags ); // fade type (in / out)
		WRITE_BYTE( fade.r ); // fade red
		WRITE_BYTE( fade.g ); // fade green
		WRITE_BYTE( fade.b ); // fade blue
		WRITE_BYTE( fade.a ); // fade blue
	MESSAGE_END();
}


void UTIL_EdictScreenFade( edict_s *edict, const Vector &color, float fadeTime, float fadeHold, int alpha, int flags ) //FX
{
	ScreenFade fade;
	UTIL_EdictScreenFadeBuild( fade, color, fadeTime, fadeHold, alpha, flags );
	UTIL_EdictScreenFadeWrite( fade, edict );
}
// - Spinator

void CBasePlayer :: Pain( void )
{
	float	flRndSound;//sound randomizer

	flRndSound = RANDOM_FLOAT ( 0 , 1 ); 
	
	if ( flRndSound <= 0.33 )
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
	else if ( flRndSound <= 0.66 )	
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
}

/* 
 *
 */
Vector VecVelocityForDamage(float flDamage)
{
	Vector vec(RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));

	if (flDamage > -50)
		vec = vec * 0.7;
	else if (flDamage > -200)
		vec = vec * 2;
	else
		vec = vec * 10;
	
	return vec;
}

int TrainSpeed(int iSpeed, int iMax)
{
	float fSpeed, fMax;
	int iRet = 0;

	fMax = (float)iMax;
	fSpeed = iSpeed;

	fSpeed = fSpeed/fMax;

	if (iSpeed < 0)
		iRet = TRAIN_BACK;
	else if (iSpeed == 0)
		iRet = TRAIN_NEUTRAL;
	else if (fSpeed < 0.33)
		iRet = TRAIN_SLOW;
	else if (fSpeed < 0.66)
		iRet = TRAIN_MEDIUM;
	else
		iRet = TRAIN_FAST;

	return iRet;
}

void CBasePlayer :: DeathSound( void )
{
	// water death sounds
	/*
	if (pev->waterlevel == 3)
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE);
		return;
	}
	*/

	// temporarily using pain sounds for death sounds
	switch (RANDOM_LONG(1,5)) 
	{
	case 1: 
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM);
		break;
	case 2: 
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM);
		break;
	case 3: 
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM);
		break;
	}

	// play one of the suit death alarms
	EMIT_GROUPNAME_SUIT(ENT(pev), "HEV_DEAD");
}

// override takehealth
// bitsDamageType indicates type of damage healed. 

int CBasePlayer :: TakeHealth( float flHealth, int bitsDamageType )
{
	return CBaseMonster :: TakeHealth (flHealth, bitsDamageType);

}

Vector CBasePlayer :: GetGunPosition( )
{
//	UTIL_MakeVectors(pev->v_angle);
//	m_HackedGunPos = pev->view_ofs;
	Vector origin;
	
	origin = pev->origin + pev->view_ofs;

	return origin;
}

//=========================================================
// TraceAttack
//=========================================================
#define	HITGROUP_GENERIC	0
#define	HITGROUP_HEAD		1
#define	HITGROUP_CHEST		2
#define	HITGROUP_STOMACH	3
#define HITGROUP_LEFTARM	4	
#define HITGROUP_RIGHTARM	5
#define HITGROUP_LEFTLEG	6
#define HITGROUP_RIGHTLEG	7
char hitgrp[8][64] =
{
	{"generic"},
	{"head"},
	{"chest"},
	{"stomach"},
	{"leftarm"},
	{"rightarm"},
	{"leftleg"},
	{"rightleg"}
};

void CBasePlayer :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	int part = 0;
	BOOL ShouldBleed = TRUE;
	float  flModify = 1.0;
	     
	/*if( flDamage <= 15 )
		flModify /= 4.0;
	else if( flDamage <= 25 )
		flModify /= 3.0;
	else if( flDamage <= 35 )
		flModify /= 2.0;*/

	CBaseEntity *pAttacker = CBaseEntity::Instance(pevAttacker);

	if( this->m_iTeam == pevAttacker->team )
		return;
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker ) )
		return;

	if ( pev->takedamage )
	{
		m_LastHitGroup = ptr->iHitgroup;

		switch ( ptr->iHitgroup )
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			if( m_iArmor[AR_HEAD] > 50 )
			{
				flDamage *= 0.90; // 1.00
				m_iArmor[AR_HEAD] -= ( 50 * flModify );
				ShouldBleed = FALSE;
			}
			else if( m_iArmor[AR_HEAD] > 0 )
			{
				flDamage *= 1.25; // 1.3
				m_iArmor[AR_HEAD] -= ( 50 * flModify );
				ShouldBleed = FALSE;
			}
			else
			{
				flDamage *= 2.5; // 2.8
				ShouldBleed = TRUE;
			}
			if( FBitSet( m_iCyber, CB_HEAD ) )
				flDamage *= 0.75;
			break;
		case HITGROUP_CHEST:
			if( m_iArmor[AR_CHEST] > 75 )
			{
				flDamage *= 0.4; // 0.4
				m_iArmor[AR_CHEST] -= ( 25 * flModify );
				m_iArmor[AR_STOMACH] -= ( 25 * flModify );
				ShouldBleed = FALSE;
			}
			else if( m_iArmor[AR_CHEST] > 50 )
			{
				flDamage *= 0.5; // 0.5
				m_iArmor[AR_CHEST] -= ( 25 * flModify );
				m_iArmor[AR_STOMACH] -= ( 25 * flModify );
				ShouldBleed = FALSE;
			}
			else if( m_iArmor[AR_CHEST] > 25 )
			{
				flDamage *= 0.6; // 0.6
				m_iArmor[AR_CHEST] -= ( 25 * flModify );
				m_iArmor[AR_STOMACH] -= ( 25 * flModify );
				ShouldBleed = FALSE;
			}
			else if( m_iArmor[AR_CHEST] > 0 )
			{
				flDamage *= 0.8; // 0.8
				m_iArmor[AR_CHEST] -= ( 25 * flModify );
				m_iArmor[AR_STOMACH] -= ( 25 * flModify );
				ShouldBleed = FALSE;
			}
			else
			{
				flDamage *= 1.5;
				ShouldBleed = TRUE;
			}
			break;
		case HITGROUP_STOMACH:
			if( m_iArmor[AR_STOMACH] > 75 )
			{
				flDamage *= 0.6; // 0.6
				m_iArmor[AR_CHEST] -= ( 25 * flModify );
				m_iArmor[AR_STOMACH] -= ( 25 * flModify );
				ShouldBleed = FALSE;
			}
			else if( m_iArmor[AR_STOMACH] > 50 )
			{
				flDamage *= 0.7; // 0.7
				m_iArmor[AR_CHEST] -= ( 25 * flModify );
				m_iArmor[AR_STOMACH] -= ( 25 * flModify );
				ShouldBleed = FALSE;
			}
			else if( m_iArmor[AR_STOMACH] > 25 )
			{
				flDamage *= 0.8; // 0.8
				m_iArmor[AR_CHEST] -= ( 25 * flModify );
				m_iArmor[AR_STOMACH] -= ( 25 * flModify );
				ShouldBleed = FALSE;
			}
			else if( m_iArmor[AR_STOMACH] > 0 )
			{
				flDamage *= 1.0; // 1.0
				m_iArmor[AR_CHEST] -= ( 25 * flModify );
				m_iArmor[AR_STOMACH] -= ( 25 * flModify );
				ShouldBleed = FALSE;
			}
			else
			{
				flDamage *= 1.8;
				ShouldBleed = TRUE;
			}
			break;
		case HITGROUP_LEFTARM:
			part = AR_L_ARM;
			break;
		case HITGROUP_RIGHTARM:
			part = AR_R_ARM;
			break;
		case HITGROUP_LEFTLEG:
			part = AR_L_LEG;
			break;
		case HITGROUP_RIGHTLEG:
			part = AR_R_LEG;
			break;
		default:
			break;
		}
		if( part )
		{
			if( m_iArmor[part] > 50 )
			{
				flDamage *= 0.2; // 0.2
				m_iArmor[part] -= ( 50 * flModify );
				ShouldBleed = FALSE;
			}
			else if( m_iArmor[part] > 0 )
			{
				flDamage *= 0.4; // 0.3
				m_iArmor[part] -= ( 50 * flModify );
				ShouldBleed = FALSE;
			}
			else if( m_iArmor[part] <= 0 )
			{
				flDamage *= 0.8; // 0.8
				ShouldBleed = TRUE;
			}
		}

		if( FBitSet( bitsDamageType, DMG_CLUB ) )
			flDamage *= 0.5;

		flDamage -= flDamage * ( (float)m_iNumMates * MATE_DMG_BONUS );

    if( m_iTeam == 2 )
      flDamage *= 0.20;

		if( dmgratio.value > 0 )
			flDamage *= dmgratio.value;
		/*if( strlen( gameplay.string ) > 0 )
		{
			float svdmg = 1.0f;
			if( !stricmp( gameplay.string, "realistic" ) )
				svdmg = 2.0f;
			if( !stricmp( gameplay.string, "action" ) )
				svdmg = 0.5f;
					
			flDamage *= svdmg;
		}*/
		
		ShouldBleed = TRUE;
		if(ShouldBleed)
		{
			//SpawnBlood(ptr->vecEndPos, BLOOD_COLOR_RED, flDamage);// a little surface blood.
      vec3_t org = ptr->vecEndPos + 5 * vecDir.Normalize( );
      MESSAGE_BEGIN( MSG_PVS, gmsgSpray, pev->origin );
      if( flDamage >= 50 )
      {
        WRITE_BYTE( SPRAY_PLRBLOODBIG );
        WRITE_COORD( org.x ); // origin
        WRITE_COORD( org.y );
        WRITE_COORD( org.z );
        WRITE_COORD( 0 ); // dir
        WRITE_COORD( 0 );
	      WRITE_COORD( 1 );
      }
      else
      {
        WRITE_BYTE( SPRAY_PLRBLOOD );
        WRITE_COORD( org.x ); // origin
        WRITE_COORD( org.y );
        WRITE_COORD( org.z );
        WRITE_COORD( vecDir.x ); // dir
        WRITE_COORD( vecDir.y );
        WRITE_COORD( vecDir.z );
      }
        WRITE_COORD( 0 );
	      WRITE_COORD( 1 );
	      WRITE_COORD( 1 );
      MESSAGE_END( );
			TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		}

		//ALERT( at_console, UTIL_VarArgs( "%s(%d) hit %s in %s for %f damage.\n", STRING(pevAttacker->netname), ENTINDEX(ENT(pevAttacker)), STRING(pev->netname), hitgrp[ptr->iHitgroup], flDamage ) );
		
		CBasePlayer *pa = (CBasePlayer*)CBaseEntity::Instance(pevAttacker);

		if( (CBaseEntity::Instance(pevAttacker))->IsPlayer( ) && pa )
			pa->AddDmgDone( ENTINDEX(ENT(pev)), flDamage, ptr->iHitgroup );
		AddDmgReceived( ENTINDEX(ENT(pevAttacker)), flDamage, ptr->iHitgroup );

		if( ptr->iHitgroup == HITGROUP_HEAD &&  pev->health - flDamage <= 0 )
		{
			/*MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BLOODSTREAM );
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z + 28 );
				WRITE_COORD( vecDir.x );
				WRITE_COORD( vecDir.y );
				WRITE_COORD( vecDir.z );
				WRITE_BYTE( 70 );
				WRITE_BYTE( 180 );
			MESSAGE_END();*/
      vec3_t org = ptr->vecEndPos + 2 * vecDir.Normalize( );
      MESSAGE_BEGIN( MSG_PVS, gmsgSpray, pev->origin );
        WRITE_BYTE( SPRAY_PLRBLOODSTREAM );
        WRITE_COORD( org.x ); // origin
        WRITE_COORD( org.y );
        WRITE_COORD( org.z );
        WRITE_COORD( vecDir.x ); // dir
        WRITE_COORD( vecDir.y );
        WRITE_COORD( vecDir.z );
        WRITE_COORD( 0 );
	      WRITE_COORD( 1 );
	      WRITE_COORD( 1 );
      MESSAGE_END( );
		}

		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
		UpdateArmor( );
	}
}

/*
	Take some damage.  
	NOTE: each call to TakeDamage with bitsDamageType set to a time-based damage
	type will cause the damage time countdown to be reset.  Thus the ongoing effects of poison, radiation
	etc are implemented with subsequent calls to TakeDamage using DMG_GENERIC.
*/

#define ARMOR_RATIO	 0.2	// Armor Takes 80% of the damage
#define ARMOR_BONUS  0.5	// Each Point of Armor is work 1/x points of health
void Explosion( entvars_t* pev, float flDamage, float flRadius );

int CBasePlayer :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// have suit diagnose the problem - ie: report damage type
	int bitsDamage = bitsDamageType;
	int ffound = TRUE;
	int fmajor;
	int fcritical;
	int fTookDamage;
	int ftrivial;
	float flRatio;
	float flBonus;
	float flHealthPrev = pev->health;

	flBonus = ARMOR_BONUS;
	flRatio = ARMOR_RATIO;

	if ( ( bitsDamageType & DMG_BLAST ) && g_pGameRules->IsMultiplayer() )
	{
		// blasts damage armor more.
		flBonus *= 2;
	}

	// Already dead
	if ( !IsAlive() )
		return 0;
	// go take the damage first

	
	CBaseEntity *pAttacker = CBaseEntity::Instance(pevAttacker);

	if ( !g_pGameRules->FPlayerCanTakeDamage( this, pAttacker ) )
	{
		// Refuse the damage
		return 0;
	}

	// keep track of amount of damage last sustained
	m_lastDamageAmount = flDamage;

	if( FBitSet( bitsDamageType, DMG_BURN ) )
	{
		//AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
		if( m_fBurning <= gpGlobals->time )
		{
			m_fBurning = gpGlobals->time + 3.8;
			pevFlamer = pevAttacker;
			m_fNextSpread = 0;
			MESSAGE_BEGIN( MSG_PVS, gmsgSpray, pev->origin );
			WRITE_BYTE( SPRAY_BURN );
				WRITE_COORD( 0 ); // origin
				WRITE_COORD( 0 );
				WRITE_COORD( -37 );
				WRITE_COORD( 0 ); // origin
				WRITE_COORD( 0 );
				WRITE_COORD( 1 );
				WRITE_COORD( entindex( ) );
				WRITE_COORD( 1 );
				WRITE_COORD( 1 );
			MESSAGE_END();
		}
	}	
	// this cast to INT is critical!!! If a player ends up with 0.5 health, the engine will get that
	// as an int (zero) and think the player is dead! (this will incite a clientside screentilt, etc)
	fTookDamage = CBaseMonster::TakeDamage(pevInflictor, pevAttacker, (int)flDamage, bitsDamageType);

	// reset damage time countdown for each type of time based damage player just sustained

	{
		for (int i = 0; i < CDMG_TIMEBASED; i++)
			if (bitsDamageType & (DMG_PARALYZE << i))
				m_rgbTimeBasedDamage[i] = 0;
	}

	// tell director about it
	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE ( 9 );	// command length in bytes
		WRITE_BYTE ( DRC_CMD_EVENT );	// take damage event
		WRITE_SHORT( ENTINDEX(this->edict()) );	// index number of primary entity
		WRITE_SHORT( ENTINDEX(ENT(pevInflictor)) );	// index number of secondary entity
		WRITE_LONG( 5 );   // eventflags (priority and flags)
	MESSAGE_END();


	// how bad is it, doc?

	ftrivial = (pev->health > 75 || m_lastDamageAmount < 5);
	fmajor = (m_lastDamageAmount > 25);
	fcritical = (pev->health < 30);

	// handle all bits set in this damage message,
	// let the suit give player the diagnosis

	// UNDONE: add sounds for types of damage sustained (ie: burn, shock, slash )

	// UNDONE: still need to record damage and heal messages for the following types

		// DMG_BURN	
		// DMG_FREEZE
		// DMG_BLAST
		// DMG_SHOCK

	m_bitsDamageType |= bitsDamage; // Save this so we can report it to the client
	m_bitsHUDDamage = -1;  // make sure the damage bits get resent

	while (fTookDamage && (!ftrivial || (bitsDamage & DMG_TIMEBASED)) && ffound && bitsDamage)
	{
		ffound = FALSE;

		if (bitsDamage & DMG_CLUB)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC);	// minor fracture
			bitsDamage &= ~DMG_CLUB;
			ffound = TRUE;
		}
		if (bitsDamage & (DMG_FALL | DMG_CRUSH))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", FALSE, SUIT_NEXT_IN_30SEC);	// major fracture
			else
				SetSuitUpdate("!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC);	// minor fracture
	
			bitsDamage &= ~(DMG_FALL | DMG_CRUSH);
			ffound = TRUE;
		}
		
		if (bitsDamage & DMG_BULLET)
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", FALSE, SUIT_NEXT_IN_30SEC);	// blood loss detected
			//else
			//	SetSuitUpdate("!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC);	// minor laceration
			
			bitsDamage &= ~DMG_BULLET;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_SLASH)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", FALSE, SUIT_NEXT_IN_30SEC);	// major laceration
			else
				SetSuitUpdate("!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC);	// minor laceration

			bitsDamage &= ~DMG_SLASH;
			ffound = TRUE;
		}
		
		if (bitsDamage & DMG_SONIC)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG2", FALSE, SUIT_NEXT_IN_1MIN);	// internal bleeding
			bitsDamage &= ~DMG_SONIC;
			ffound = TRUE;
		}

		if (bitsDamage & (DMG_POISON | DMG_PARALYZE))
		{
			SetSuitUpdate("!HEV_DMG3", FALSE, SUIT_NEXT_IN_1MIN);	// blood toxins detected
			bitsDamage &= ~(DMG_POISON | DMG_PARALYZE);
			ffound = TRUE;
		}

		if (bitsDamage & DMG_ACID)
		{
			SetSuitUpdate("!HEV_DET1", FALSE, SUIT_NEXT_IN_1MIN);	// hazardous chemicals detected
			bitsDamage &= ~DMG_ACID;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_NERVEGAS)
		{
			SetSuitUpdate("!HEV_DET0", FALSE, SUIT_NEXT_IN_1MIN);	// biohazard detected
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = TRUE;
		}

		if (bitsDamage & DMG_RADIATION)
		{
			SetSuitUpdate("!HEV_DET2", FALSE, SUIT_NEXT_IN_1MIN);	// radiation detected
			bitsDamage &= ~DMG_RADIATION;
			ffound = TRUE;
		}
		if (bitsDamage & DMG_SHOCK)
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = TRUE;
		}
	}

	pev->punchangle.x = -2;

	if (fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75) 
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate("!HEV_MED1", FALSE, SUIT_NEXT_IN_30MIN);	// automedic on

		// give morphine shot if not given recently
		SetSuitUpdate("!HEV_HEAL7", FALSE, SUIT_NEXT_IN_30MIN);	// morphine shot
	}
	
	if (fTookDamage && !ftrivial && fcritical && flHealthPrev < 75)
	{

		// already took major damage, now it's critical...
		if (pev->health < 6)
			SetSuitUpdate("!HEV_HLTH3", FALSE, SUIT_NEXT_IN_10MIN);	// near death
		else if (pev->health < 20)
			SetSuitUpdate("!HEV_HLTH2", FALSE, SUIT_NEXT_IN_10MIN);	// health critical
	
		// give critical health warnings
		if (!RANDOM_LONG(0,3) && flHealthPrev < 50)
			SetSuitUpdate("!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN); //seek medical attention
	}

	// if we're taking time based damage, warn about its continuing effects
	if (fTookDamage && (bitsDamageType & DMG_TIMEBASED) && flHealthPrev < 75)
		{
			if (flHealthPrev < 50)
			{
				if (!RANDOM_LONG(0,3))
					SetSuitUpdate("!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN); //seek medical attention
			}
			else
				SetSuitUpdate("!HEV_HLTH1", FALSE, SUIT_NEXT_IN_10MIN);	// health dropping
		}
	FadeOnDMG( );
	if( FBitSet( m_iAddons, AD_TORSO_BOMB ) && ( pev->health <= 20 ) && UTIL_FromChance( 2 ) )
		Explosion( pev, 150, CYBERBOMB_DAMAGE );
	else if( FBitSet( m_iAddons, AD_TORSO_BOMB ) && ( pev->health <= 0 ) )
		Explosion( pev, 150, CYBERBOMB_DAMAGE );

	return fTookDamage;
}

//=========================================================
// PackDeadPlayerItems - call this when a player dies to
// pack up the appropriate weapons and ammo items, and to
// destroy anything that shouldn't be packed.
//
// This is pretty brute force :(
//=========================================================
void CBasePlayer::PackDeadPlayerItems( void )
{
	int iWeaponRules;
	int iAmmoRules;
	int i;
	CBasePlayerWeapon *rgpPackWeapons[ 20 ];// 20 hardcoded for now. How to determine exactly how many weapons we have?
	int iPackAmmo[ MAX_AMMO_SLOTS + 1];
	int iPW = 0;// index into packweapons array
	int iPA = 0;// index into packammo array

	memset(rgpPackWeapons, NULL, sizeof(rgpPackWeapons) );
	memset(iPackAmmo, -1, sizeof(iPackAmmo) );

	// get the game rules 
	iWeaponRules = g_pGameRules->DeadPlayerWeapons( this );
 	iAmmoRules = g_pGameRules->DeadPlayerAmmo( this );

	if ( iWeaponRules == GR_PLR_DROP_GUN_NO && iAmmoRules == GR_PLR_DROP_AMMO_NO )
	{
		// nothing to pack. Remove the weapons and return. Don't call create on the box!
		RemoveAllItems( TRUE );
		return;
	}

// go through all of the weapons and make a list of the ones to pack
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			// there's a weapon here. Should I pack it?
			CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[ i ];

			while ( pPlayerItem )
			{
				switch( iWeaponRules )
				{
				case GR_PLR_DROP_GUN_ACTIVE:
					if ( m_pActiveItem && pPlayerItem == m_pActiveItem )
					{
						// this is the active item. Pack it.
						rgpPackWeapons[ iPW++ ] = (CBasePlayerWeapon *)pPlayerItem;
					}
					break;

				case GR_PLR_DROP_GUN_ALL:
					rgpPackWeapons[ iPW++ ] = (CBasePlayerWeapon *)pPlayerItem;
					break;

				default:
					break;
				}

				pPlayerItem = NULL;//pPlayerItem->m_pNext;
			}
		}
	}

// now go through ammo and make a list of which types to pack.
	if ( iAmmoRules != GR_PLR_DROP_AMMO_NO )
	{
		for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
		{
			if ( m_rgAmmo[ i ] > 0 )
			{
				// player has some ammo of this type.
				switch ( iAmmoRules )
				{
				case GR_PLR_DROP_AMMO_ALL:
					iPackAmmo[ iPA++ ] = i;
					break;

				case GR_PLR_DROP_AMMO_ACTIVE:
					if ( m_pActiveItem && i == m_pActiveItem->PrimaryAmmoIndex() ) 
					{
						// this is the primary ammo type for the active weapon
						iPackAmmo[ iPA++ ] = i;
					}
					else if ( m_pActiveItem && i == m_pActiveItem->SecondaryAmmoIndex() ) 
					{
						// this is the secondary ammo type for the active weapon
						iPackAmmo[ iPA++ ] = i;
					}
					break;

				default:
					break;
				}
			}
		}
	}

// create a box to pack the stuff into.
	CWeaponBox *pWeaponBox = (CWeaponBox *)CBaseEntity::Create( "weaponbox", pev->origin, pev->angles, edict() );

	pWeaponBox->pev->angles.x = 0;// don't let weaponbox tilt.
	pWeaponBox->pev->angles.z = 0;

	pWeaponBox->SetThink( &CWeaponBox::Kill );
	pWeaponBox->pev->nextthink = gpGlobals->time + 120;

// back these two lists up to their first elements
	iPA = 0;
	iPW = 0;

// pack the ammo
	while ( iPackAmmo[ iPA ] != -1 )
	{
		pWeaponBox->PackAmmo( MAKE_STRING( CBasePlayerItem::AmmoInfoArray[ iPackAmmo[ iPA ] ].pszName ), m_rgAmmo[ iPackAmmo[ iPA ] ] );
		iPA++;
	}

// now pack all of the items in the lists
	while ( rgpPackWeapons[ iPW ] )
	{
		// weapon unhooked from the player. Pack it into der box.
		pWeaponBox->PackWeapon( rgpPackWeapons[ iPW ] );

		iPW++;
	}

	pWeaponBox->pev->velocity = pev->velocity * 1.2;// weaponbox has player's velocity, then some.

	RemoveAllItems( TRUE );// now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::RemoveAllItems( BOOL removeSuit )
{
	if (m_pActiveItem)
	{
		ResetAutoaim( );
		m_pActiveItem->Holster( );
		m_pActiveItem = NULL;
	}

	m_pLastItem = NULL;

	int i;
	CBasePlayerItem *pPendingItem;
	for (i = 0; i < MAX_ITEM_TYPES; i++)
	{
		m_pActiveItem = m_rgpPlayerItems[i];
		while (m_pActiveItem)
		{
			pPendingItem = NULL;//m_pActiveItem->m_pNext; 
			m_pActiveItem->Drop( );
			m_pActiveItem = pPendingItem;
		}
		m_rgpPlayerItems[i] = NULL;
	}
	m_pActiveItem = NULL;

	pev->viewmodel		= 0;
	pev->weaponmodel	= 0;
	
	if ( removeSuit )
		pev->weapons = 0;
	else
		pev->weapons &= ~WEAPON_ALLWEAPONS;

	for ( i = 0; i < MAX_AMMO_SLOTS;i++)
		m_rgAmmo[i] = 0;

	UpdateClientData();
	// send Selected Weapon Message to our client
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0);
	MESSAGE_END();
}

/*
 * GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
 *
 * ENTITY_METHOD(PlayerDie)
 */
entvars_t *g_pevLastInflictor;  // Set in combat.cpp.  Used to pass the damage inflictor for death messages.
								// Better solution:  Add as parameter to all Killed() functions.

void CBasePlayer::Killed( entvars_t *pevAttacker, int iGib )
{
	CSound *pSound;

  m_bNoAutospawn = TRUE;
  m_fNextAutospawn = gpGlobals->time + 3;


	m_iCurInfo = 0;
	m_fNextInfo = gpGlobals->time + 1;

	// Holster weapon immediately, to allow it to cleanup
	if ( m_pActiveItem )
		m_pActiveItem->Holster( );

	g_pGameRules->PlayerKilled( this, pevAttacker, pevAttacker );
  
  MESSAGE_BEGIN( MSG_ONE, gmsgSmallCnt, NULL, edict( ) );
		WRITE_STRING( "Zombie transformation" );
		WRITE_COORD( 0 );
	MESSAGE_END( );

	if ( m_pTank != NULL )
	{
		m_pTank->Use( this, this, USE_OFF, 0 );
		m_pTank = NULL;
	}
		if ( m_fNVGActive )
			NVGActive( FALSE ); 
		
		m_fNVG = FALSE; 
		MESSAGE_BEGIN(MSG_ONE, gmsgNVG, NULL, pev);
			WRITE_BYTE( 0 );
        MESSAGE_END( );

	// this client isn't going to be thinking for a while, so reset the sound until they respawn
	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
	{
		if ( pSound )
		{
			pSound->Reset();
		}
	}

	SetAnimation( PLAYER_DIE );
	
	m_iRespawnFrames = 0;

	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes

	pev->deadflag		= DEAD_DYING;
	pev->movetype		= MOVETYPE_TOSS;
	ClearBits( pev->flags, FL_ONGROUND );
	if (pev->velocity.z < 10)
		pev->velocity.z += RANDOM_FLOAT(0,300);

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, FALSE, 0);

	// send "health" update message to zero
	m_iClientHealth = 0;
	MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
		WRITE_SHORT( m_iClientHealth );
	MESSAGE_END( );

	ResetRadar( );
	MESSAGE_BEGIN( MSG_ONE, gmsgRadar, NULL, pev );
		 WRITE_SHORT( 0 );
		 WRITE_BYTE( 3 );
	MESSAGE_END( );

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE(0);
		WRITE_BYTE(0);
		WRITE_BYTE(0XFF);
		WRITE_BYTE(0xFF);
		WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	pev->fov = m_iFOV = m_iClientFOV = 0;

	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE(0);
	MESSAGE_END();


	// UNDONE: Put this in, but add FFADE_PERMANENT and make fade time 8.8 instead of 4.12
	// UTIL_ScreenFade( edict(), Vector(128,0,0), 6, 15, 255, FFADE_OUT | FFADE_MODULATE );

	if ( ( pev->health < -40 && iGib != GIB_NEVER ) || iGib == GIB_ALWAYS )
	{
		pev->solid			= SOLID_NOT;
		GibMonster();	// This clears pev->model
		pev->effects |= EF_NODRAW;
		return;
	}
	//DeathSound();
	
	pev->angles.x = 0;
	pev->angles.z = 0;
	observing = 0;
	Notify( this, 0, 0 );

	SetThink(&CBasePlayer::PlayerDeathThink);
	pev->nextthink = gpGlobals->time + 0.1;
}


// Set the activity based on an event or current state
void CBasePlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;
	float speed;
	char szAnim[64];

	speed = pev->velocity.Length2D();

	if (pev->flags & FL_FROZEN)
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	switch (playerAnim) 
	{
	case PLAYER_JUMP:
		m_IdealActivity = ACT_HOP;
		break;
	
	case PLAYER_SUPERJUMP:
		m_IdealActivity = ACT_LEAP;
		break;
	
	case PLAYER_DIE:
		m_IdealActivity = ACT_DIESIMPLE;
		m_IdealActivity = GetDeathActivity( );
		break;

	case PLAYER_ATTACK1:	
		switch( m_Activity )
		{
		case ACT_HOVER:
		case ACT_SWIM:
		//case ACT_HOP:
		case ACT_LEAP:
		case ACT_DIESIMPLE:
			m_IdealActivity = m_Activity;
			break;
		default:
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}
		break;
	case PLAYER_IDLE:
	case PLAYER_WALK:
		/*if ( !FBitSet( pev->flags, FL_ONGROUND ) && (m_Activity == ACT_HOP || m_Activity == ACT_LEAP) )	// Still jumping
		{
			m_IdealActivity = m_Activity;
		}
		else */if ( pev->waterlevel > 1 )
		{
			if ( speed == 0 )
				m_IdealActivity = ACT_HOVER;
			else
				m_IdealActivity = ACT_SWIM;
		}
		else
		{
			m_IdealActivity = ACT_WALK;
		}
		break;
	}

	switch (m_IdealActivity)
	{
	case ACT_HOVER:
	case ACT_LEAP:
	case ACT_SWIM:
	//case ACT_HOP:
	case ACT_DIESIMPLE:
	default:
		if ( m_Activity == m_IdealActivity)
			return;
		m_Activity = m_IdealActivity;

		animDesired = LookupActivity( m_Activity );
		// Already using the desired animation?
		if (pev->sequence == animDesired)
			return;

		pev->gaitsequence = 0;
		pev->sequence		= animDesired;
		pev->frame			= 0;
		ResetSequenceInfo( );
		return;

	case ACT_HOP:
		if ( m_Activity == m_IdealActivity)
			return;
		m_Activity = m_IdealActivity;
		strcpy( szAnim, "ref_aim_" );
		strcat( szAnim, m_szAnimExtention );
    if( ( m_fTransformTime - 10 ) <= gpGlobals->time && m_bTransform )
    {
		  animDesired = LookupSequence( "morph" );
    }
    else
		  animDesired = LookupSequence( szAnim );
		if (animDesired == -1)
			animDesired = 0;
		m_Activity = ACT_HOP;

		// Already using the desired animation?
		//if (pev->sequence == animDesired)
		//	return;

		pev->sequence		= animDesired;
		animDesired			= LookupActivity( m_Activity );
		pev->gaitsequence	= animDesired;
		pev->frame			= 0;
		ResetSequenceInfo( );
		return;
		/*if (m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished)
		{
			if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
				strcpy( szAnim, "crouch_aim_" );
			else
				strcpy( szAnim, "ref_aim_" );
			strcat( szAnim, m_szAnimExtention );
			animDesired = LookupSequence( szAnim );
			if (animDesired == -1)
				animDesired = 0;
			m_Activity = ACT_HOP;
		}
		else
		{
			animDesired = pev->sequence;
		}
		break;*/

	case ACT_RANGE_ATTACK1:
		if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
			strcpy( szAnim, "crouch_shoot_" );
		else
			strcpy( szAnim, "ref_shoot_" );
		strcat( szAnim, m_szAnimExtention );
    
    if( ( m_fTransformTime - 10 ) <= gpGlobals->time && m_bTransform )
    {
		  animDesired = LookupSequence( "morph" );
    }
    else
      animDesired = LookupSequence( szAnim );
		if (animDesired == -1)
			animDesired = 0;

		if ( pev->sequence != animDesired || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		if (!m_fSequenceLoops)
		{
			pev->effects |= EF_NOINTERP;
		}

		m_Activity = m_IdealActivity;

		pev->sequence		= animDesired;
		ResetSequenceInfo( );
		break;

	case ACT_WALK:
		if (m_Activity != ACT_RANGE_ATTACK1 || m_fSequenceFinished)
		{
			if ( FBitSet( pev->flags, FL_DUCKING ) )	// crouching
				strcpy( szAnim, "crouch_aim_" );
			else
				strcpy( szAnim, "ref_aim_" );
			strcat( szAnim, m_szAnimExtention );
      if( ( m_fTransformTime - 10 ) <= gpGlobals->time && m_bTransform )
      {
		    animDesired = LookupSequence( "morph" );
      }
      else
			  animDesired = LookupSequence( szAnim );
			if (animDesired == -1)
				animDesired = 0;
			m_Activity = ACT_WALK;
		}
		else
		{
      if( ( m_fTransformTime - 10 ) <= gpGlobals->time && m_bTransform )
      {
		    animDesired = LookupSequence( "morph" );
      }
      else
			  animDesired = pev->sequence;
		}
	}

	if ( FBitSet( pev->flags, FL_DUCKING ) )
	{
		if ( speed == 0)
		{
			pev->gaitsequence	= LookupActivity( ACT_CROUCHIDLE );
			// pev->gaitsequence	= LookupActivity( ACT_CROUCH );
		}
		else
		{
			pev->gaitsequence	= LookupActivity( ACT_CROUCH );
		}
	}
	else if( !FBitSet( pev->flags, FL_ONGROUND ) /*&& (m_Activity == ACT_HOP || m_Activity == ACT_LEAP)*/ )
	{
		pev->gaitsequence	= LookupActivity( ACT_HOP );
		m_Activity = ACT_HOP;
	}
	else if ( speed > 160 )
	{
		pev->gaitsequence	= LookupActivity( ACT_RUN );
	}
	else if (speed > 0)
	{
		pev->gaitsequence	= LookupActivity( ACT_WALK );
	}
	else
	{
		// pev->gaitsequence	= LookupActivity( ACT_WALK );
		pev->gaitsequence	= LookupSequence( "deep_idle" );
	}


	// Already using the desired animation?
	if (pev->sequence == animDesired)
		return;

	//ALERT( at_console, "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	pev->sequence		= animDesired;
	pev->frame			= 0;
	ResetSequenceInfo( );
}

/*
===========
TabulateAmmo
This function is used to find and store 
all the ammo we have into the ammo vars.
============
*/
void CBasePlayer::TabulateAmmo()
{
	ammo_9mm = AmmoInventory( GetAmmoIndex( "9mm" ) );
	ammo_slot5 = AmmoInventory( GetAmmoIndex( m_sAmmoSlot[3] ) );
	ammo_slot4 = AmmoInventory( GetAmmoIndex( m_sAmmoSlot[2] ) );
	ammo_slot2_a = AmmoInventory( GetAmmoIndex( m_sAmmo_aSlot[0] ) );
	ammo_slot3_a = AmmoInventory( GetAmmoIndex( m_sAmmo_aSlot[1] ) );
	ammo_slot4_a = AmmoInventory( GetAmmoIndex( m_sAmmo_aSlot[2] ) );
	ammo_slot2 = AmmoInventory( GetAmmoIndex( m_sAmmoSlot[0] ) );
	ammo_slot3 = AmmoInventory( GetAmmoIndex( m_sAmmoSlot[1] ) );
//	ALERT( at_console, "%s: %d, %s: %d, %s: %d\n", m_sAmmoSlot[0], ammo_slot2, m_sAmmoSlot[1], ammo_slot3, m_sAmmoSlot[2], ammo_slot4 );
//	ALERT( at_console, "%s: %d, %s: %d, %s: %d\n", m_sAmmo_aSlot[0], ammo_slot2_a, m_sAmmo_aSlot[1], ammo_slot3_a, m_sAmmo_aSlot[2], ammo_slot4_a );
}


/*
===========
WaterMove
============
*/
#define AIRTIME	12		// lung full of air lasts this many seconds

void CBasePlayer::WaterMove()
{
	int air;

	if (pev->movetype == MOVETYPE_NOCLIP)
		return;

	if (pev->health < 0)
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if (pev->waterlevel != 3) 
	{
		// not underwater
		
		// play 'up for air' sound
		if (pev->air_finished < gpGlobals->time)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM);
		else if (pev->air_finished < gpGlobals->time + 9)
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM);

		pev->air_finished = gpGlobals->time + AIRTIME;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if (m_idrowndmg > m_idrownrestored)
		{
			// set drowning damage bit.  hack - dmg_drownrecover actually
			// makes the time based damage code 'give back' health over time.
			// make sure counter is cleared so we start count correctly.
			
			// NOTE: this actually causes the count to continue restarting
			// until all drowning damage is healed.

			m_bitsDamageType |= DMG_DROWNRECOVER;
			m_bitsDamageType &= ~DMG_DROWN;
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		}

	}
	else
	{	// fully under water
		// stop restoring damage while underwater
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;

		if (pev->air_finished < gpGlobals->time)		// drown!
		{
			if (pev->pain_finished < gpGlobals->time)
			{
				// take drowning damage
				pev->dmg += 1;
				if (pev->dmg > 5)
					pev->dmg = 5;
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), pev->dmg, DMG_DROWN);
				pev->pain_finished = gpGlobals->time + 1;
				
				// track drowning damage, give it back when
				// player finally takes a breath

				m_idrowndmg += pev->dmg;
			} 
		}
		else
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
	}

	if (!pev->waterlevel)
	{
		if (FBitSet(pev->flags, FL_INWATER))
		{       
			ClearBits(pev->flags, FL_INWATER);
		}
		return;
	}
	
	// make bubbles

	air = (int)(pev->air_finished - gpGlobals->time);
	if (!RANDOM_LONG(0,0x1f) && RANDOM_LONG(0,AIRTIME-1) >= air)
	{
		switch (RANDOM_LONG(0,3))
			{
			case 0:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim1.wav", 0.8, ATTN_NORM); break;
			case 1:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim2.wav", 0.8, ATTN_NORM); break;
			case 2:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim3.wav", 0.8, ATTN_NORM); break;
			case 3:	EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_swim4.wav", 0.8, ATTN_NORM); break;
		}
	}

	if (pev->watertype == CONTENT_LAVA)		// do damage
	{
		if (pev->dmgtime < gpGlobals->time)
			TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 10 * pev->waterlevel, DMG_BURN);
	}
	else if (pev->watertype == CONTENT_SLIME)		// do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), 4 * pev->waterlevel, DMG_ACID);
	}
	
	if (!FBitSet(pev->flags, FL_INWATER))
	{
		SetBits(pev->flags, FL_INWATER);
		pev->dmgtime = 0;
	}
}


// TRUE if the player is attached to a ladder
BOOL CBasePlayer::IsOnLadder( void )
{ 
	return ( pev->movetype == MOVETYPE_FLY );
}

void CBasePlayer::PlayerDeathThink(void)
{
	float flForward;

	if (FBitSet(pev->flags, FL_ONGROUND))
	{
		flForward = pev->velocity.Length() - 20;
		if (flForward <= 0)
			pev->velocity = g_vecZero;
		else    
			pev->velocity = flForward * pev->velocity.Normalize();
	}

	/*if ( HasWeapons() )
	{
		// we drop the guns here because weapons that have an area effect and can kill their user
		// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
		// player class sometimes is freed. It's safer to manipulate the weapons once we know
		// we aren't calling into any of their code anymore through the player pointer.
		PackDeadPlayerItems();
	}*/


	if (pev->modelindex && (!m_fSequenceFinished) && (pev->deadflag == DEAD_DYING))
	{
		StudioFrameAdvance( );

		m_iRespawnFrames++;				// Note, these aren't necessarily real "frames", so behavior is dependent on # of client movement commands
		if ( m_iRespawnFrames < 200 )   // Animations should be no longer than this
			return;
	}

	// once we're done animating our death and we're on the ground, we want to set movetype to None so our dead body won't do collisions and stuff anymore
	// this prevents a bug where the dead body would go to a player's head if he walked over it while the dead player was clicking their button to respawn
	if ( pev->movetype != MOVETYPE_NONE && FBitSet(pev->flags, FL_ONGROUND) )
		pev->movetype = MOVETYPE_NONE;

	if (pev->deadflag == DEAD_DYING)
		pev->deadflag = DEAD_DEAD;
	
	StopAnimation();

	pev->effects |= EF_NOINTERP;
	pev->framerate = 0.0;

	BOOL fAnyButtonDown = (pev->button & ~IN_SCORE );
	
	// wait for all buttons released
	if (pev->deadflag == DEAD_DEAD)
	{
		//if (fAnyButtonDown)
		//	return;

		if ( g_pGameRules->FPlayerCanRespawn( this ) )
		{
			m_fDeadTime = gpGlobals->time;
			pev->deadflag = DEAD_RESPAWNABLE;
		}
		
		return;
	}

// if the player has been dead for one second longer than allowed by forcerespawn, 
// forcerespawn isn't on. Send the player off to an intermission camera until they 
// choose to respawn.
	if ( g_pGameRules->IsMultiplayer() && ( gpGlobals->time > (m_fDeadTime + 6) ) && !(m_afPhysicsFlags & PFLAG_OBSERVER) )
	{
		// go to dead camera. 
		// + Spinator
		StartObserver();
		// - Spinator
		//StartDeathCam();
	}
	
// wait for any button down,  or mp_forcerespawn is set and the respawn time is up
	//if (!fAnyButtonDown 
	//	&& !( g_pGameRules->IsMultiplayer() && forcerespawn.value > 0 && (gpGlobals->time > (m_fDeadTime + 5))) )
	//	return;

	pev->button = 0;
	m_iRespawnFrames = 0;

	//ALERT(at_console, "Respawn\n");

	//respawn(pev, !(m_afPhysicsFlags & PFLAG_OBSERVER) );// don't copy a corpse if we're in deathcam.
	StartObserver();
	//pev->nextthink = -1;
}

//=========================================================
// StartDeathCam - find an intermission spot and send the
// player off into observer mode
//=========================================================
void CBasePlayer::StartDeathCam( void )
{
	//edict_t *pSpot, *pNewSpot;
	//int iRand;

	if ( pev->view_ofs == g_vecZero )
	{
		// don't accept subsequent attempts to StartDeathCam()
		return;
	}
	StartObserver( );

}

// 
// PlayerUse - handles USE keypress
//
#define	PLAYER_SEARCH_RADIUS	(float)64

void CBasePlayer::PlayerUse ( void )
{
	// Was use pressed or released?
	if ( ! ((pev->button | m_afButtonPressed | m_afButtonReleased) & IN_USE) )
		return;

	// Hit Use on a train?
	if ( m_afButtonPressed & IN_USE )
	{
		if ( m_pTank != NULL )
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use( this, this, USE_OFF, 0 );
			m_pTank = NULL;
			return;
		}
		else
		{
			if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				return;
			}
			else
			{	// Start controlling the train!
				CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );

				if ( pTrain && !(pev->button & IN_JUMP) && FBitSet(pev->flags, FL_ONGROUND) && (pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) && pTrain->OnControls(pev) )
				{
					m_afPhysicsFlags |= PFLAG_ONTRAIN;
					m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
					m_iTrain |= TRAIN_NEW;
					EMIT_SOUND( ENT(pev), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM);
					return;
				}
			}
		}
	}

	CBaseEntity *pObject = NULL;
	CBaseEntity *pClosest = NULL;
	Vector		vecLOS;
	float flMaxDot = VIEW_FIELD_NARROW;
	float flDot;

	UTIL_MakeVectors ( pev->v_angle );// so we know which way we are facing
	
	while ((pObject = UTIL_FindEntityInSphere( pObject, pev->origin, PLAYER_SEARCH_RADIUS )) != NULL)
	{

		if (pObject->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE))
		{
			// !!!PERFORMANCE- should this check be done on a per case basis AFTER we've determined that
			// this object is actually usable? This dot is being done for every object within PLAYER_SEARCH_RADIUS
			// when player hits the use key. How many objects can be in that area, anyway? (sjb)
			vecLOS = (VecBModelOrigin( pObject->pev ) - (pev->origin + pev->view_ofs));
			
			// This essentially moves the origin of the target to the corner nearest the player to test to see 
			// if it's "hull" is in the view cone
			vecLOS = UTIL_ClampVectorToBox( vecLOS, pObject->pev->size * 0.5 );
			
			flDot = DotProduct (vecLOS , gpGlobals->v_forward);
			if (flDot > flMaxDot )
			{// only if the item is in front of the user
				pClosest = pObject;
				flMaxDot = flDot;
//				ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
			}
//			ALERT( at_console, "%s : %f\n", STRING( pObject->pev->classname ), flDot );
		}
	}
	pObject = pClosest;

	// Found an object
	if (pObject )
	{
		//!!!UNDONE: traceline here to prevent USEing buttons through walls			
		int caps = pObject->ObjectCaps();

		if ( m_afButtonPressed & IN_USE )
			EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM);

		if ( ( (pev->button & IN_USE) && (caps & FCAP_CONTINUOUS_USE) ) ||
			 ( (m_afButtonPressed & IN_USE) && (caps & (FCAP_IMPULSE_USE|FCAP_ONOFF_USE)) ) )
		{
			if ( caps & FCAP_CONTINUOUS_USE )
				m_afPhysicsFlags |= PFLAG_USING;

			pObject->Use( this, this, USE_SET, 1 );
		}
		// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
		else if ( (m_afButtonReleased & IN_USE) && (pObject->ObjectCaps() & FCAP_ONOFF_USE) )	// BUGBUG This is an "off" use
		{
			pObject->Use( this, this, USE_SET, 0 );
		}
	}
	else
	{
		if ( m_afButtonPressed & IN_USE )
			EMIT_SOUND( ENT(pev), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM);
	}
}



void CBasePlayer::Jump()
{
	Vector		vecWallCheckDir;// direction we're tracing a line to find a wall when walljumping
	Vector		vecAdjustedVelocity;
	Vector		vecSpot;
	TraceResult	tr;
	
	if (FBitSet(pev->flags, FL_WATERJUMP))
		return;
	
	if (pev->waterlevel >= 2)
	{
		return;
	}

	// jump velocity is sqrt( height * gravity * 2)

	// If this isn't the first frame pressing the jump button, break out.
	if ( !FBitSet( m_afButtonPressed, IN_JUMP ) )
		return;         // don't pogo stick

	if ( !(pev->flags & FL_ONGROUND) || !pev->groundentity )
	{
		return;
	}

// many features in this function use v_forward, so makevectors now.
	UTIL_MakeVectors (pev->angles);

	// ClearBits(pev->flags, FL_ONGROUND);		// don't stairwalk
	
	SetAnimation( PLAYER_JUMP );

	if ( m_fLongJump &&
		(pev->button & IN_DUCK) &&
		( pev->flDuckTime > 0 ) &&
		pev->velocity.Length() > 50 )
	{
		SetAnimation( PLAYER_SUPERJUMP );
	}

	// If you're standing on a conveyor, add it's velocity to yours (for momentum)
	entvars_t *pevGround = VARS(pev->groundentity);
	if ( pevGround && (pevGround->flags & FL_CONVEYOR) )
	{
		pev->velocity = pev->velocity + pev->basevelocity;
	}
}



// This is a glorious hack to find free space when you've crouched into some solid space
// Our crouching collisions do not work correctly for some reason and this is easier
// than fixing the problem :(
void FixPlayerCrouchStuck( edict_t *pPlayer )
{
	TraceResult trace;

	// Move up as many as 18 pixels if the player is stuck.
	for ( int i = 0; i < 18; i++ )
	{
		UTIL_TraceHull( pPlayer->v.origin, pPlayer->v.origin, dont_ignore_monsters, head_hull, pPlayer, &trace );
		if ( trace.fStartSolid )
			pPlayer->v.origin.z ++;
		else
			break;
	}
}

void CBasePlayer::Duck( )
{
  if (pev->button & IN_DUCK && !pev->fuser4) 
	{
		if ( m_IdealActivity != ACT_LEAP )
		{
			SetAnimation( PLAYER_WALK );
		}
	}
}

//
// ID's player as such.
//
int  CBasePlayer::Classify ( void )
{
	return CLASS_PLAYER;
}


void CBasePlayer::AddPoints( int score, BOOL bAllowNegativeScore )
{
	// Positive score always adds
	if ( score < 0 )
	{
		if ( !bAllowNegativeScore )
		{
			if ( pev->frags < 0 )		// Can't go more negative
				return;
			
			if ( -score > pev->frags )	// Will this go negative?
			{
				score = -pev->frags;		// Sum will be 0
			}
		}
	}

	pev->frags += score;

	/*MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(edict()) );
		WRITE_SHORT( pev->frags );
		WRITE_SHORT( m_iDeaths );
		//WRITE_SHORT( m_iClass );
		//WRITE_SHORT( m_iTeam );
	MESSAGE_END();*/
}


void CBasePlayer::AddPointsToTeam( int score, BOOL bAllowNegativeScore )
{
	int index = entindex();

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && i != index )
		{
			if ( g_pGameRules->PlayerRelationship( this, pPlayer ) == GR_TEAMMATE )
			{
				pPlayer->AddPoints( score, bAllowNegativeScore );
			}
		}
	}
}

//Player ID
void CBasePlayer::InitStatusBar()
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString1[0] = m_SbarString0[0] = 0; 
}

void CBasePlayer::UpdateStatusBar()
{
	if( !IsAlive( ) || pev->iuser1 )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 100 );
			WRITE_STRING( " " );
		MESSAGE_END();

		return;
	}

	int newSBarState[ SBAR_END ];
	char sbuf0[ SBAR_STRING_SIZE ];
	char sbuf1[ SBAR_STRING_SIZE ];

	memset( newSBarState, 0, sizeof(newSBarState) );
	strcpy( sbuf0, m_SbarString0 );
	strcpy( sbuf1, m_SbarString1 );

	// Find an ID Target
	TraceResult tr;
	UTIL_MakeVectors( pev->v_angle + pev->punchangle );
	Vector vecSrc = EyePosition();
	Vector vecEnd = vecSrc + (gpGlobals->v_forward * MAX_ID_RANGE);
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr);

	if (tr.flFraction != 1.0)
	{
		if ( !FNullEnt( tr.pHit ) )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

			if (pEntity->Classify() == CLASS_PLAYER )
			{
				newSBarState[ SBAR_ID_TARGETNAME ] = ENTINDEX( pEntity->edict() );
				//trcpy( sbuf1, "1 %p1\n2 Health: %i2%%\n3 Armor: %i3%%" );
				strcpy( sbuf1, "1 %p1\n2 Health: %i2%\n3 Armor: %i3%" );

				// allies and medics get to see the targets health
				//if ( g_pGameRules->PlayerRelationship( this, pEntity ) == GR_TEAMMATE )
				//{
				newSBarState[ SBAR_ID_TARGETHEALTH ] = ( pEntity->pev->health > 0 ? pEntity->pev->health : 0 );//100 * (pEntity->pev->health / pEntity->pev->max_health);
					newSBarState[ SBAR_ID_TARGETARMOR ] = ( pEntity->pev->armorvalue > 0 ? pEntity->pev->armorvalue : 0 ); //No need to get it % based since 100 it's the max.
				//}
				/*if ( g_pGameRules->PlayerRelationship( this, pEntity ) != GR_TEAMMATE && m_bHelp )
					Help( HELP_SEE_ENEMY, this );*/
				m_flStatusBarDisappearDelay = gpGlobals->time + 2.0;
			}
			/*else if( FClassnameIs( pEntity->pev, "pe_terminal_clip" ) && m_bHelp )
				Help( HELP_SEE_TERMINAL, this );
			/*else if( FClassnameIs( pEntity->pev, "weapon_case" ) && m_bHelp )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgHelp, NULL, pev );
					WRITE_BYTE( HELP_SEE_CASE );
				MESSAGE_END( );
			}
			else if( FClassnameIs( pEntity->pev, "pe_object_htool" ) && m_bHelp )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgHelp, NULL, pev );
					WRITE_BYTE( HELP_SEE_HTOOL );
				MESSAGE_END( );
				ALERT( at_console, "Sending HACKINGTOOL help\n" );
			}*/
		}
		else if ( m_flStatusBarDisappearDelay > gpGlobals->time )
		{
			// hold the values for a short amount of time after viewing the object
			newSBarState[ SBAR_ID_TARGETNAME ] = m_izSBarState[ SBAR_ID_TARGETNAME ];
			newSBarState[ SBAR_ID_TARGETHEALTH ] = m_izSBarState[ SBAR_ID_TARGETHEALTH ];
			newSBarState[ SBAR_ID_TARGETARMOR ] = m_izSBarState[ SBAR_ID_TARGETARMOR ];
		}
	}

	BOOL bForceResend = FALSE;

	if ( strcmp( sbuf0, m_SbarString0 ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( sbuf0 );
		MESSAGE_END();

		strcpy( m_SbarString0, sbuf0 );

		// make sure everything's resent
		bForceResend = TRUE;
	}

	if ( strcmp( sbuf1, m_SbarString1 ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_STRING( sbuf1 );
		MESSAGE_END();

		strcpy( m_SbarString1, sbuf1 );

		// make sure everything's resent
		bForceResend = TRUE;
	}

	// Check values and send if they don't match
	for (int i = 1; i < SBAR_END; i++)
	{
		if ( newSBarState[i] != m_izSBarState[i] || bForceResend )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgStatusValue, NULL, pev );
				WRITE_BYTE( i );
				WRITE_SHORT( newSBarState[i] );
			MESSAGE_END();

			m_izSBarState[i] = newSBarState[i];
		}
	}
}








  
#define CLIMB_SHAKE_FREQUENCY	22	// how many frames in between screen shakes when climbing
#define	MAX_CLIMB_SPEED			200	// fastest vertical climbing speed possible
#define	CLIMB_SPEED_DEC			15	// climbing deceleration rate
#define	CLIMB_PUNCH_X			-7  // how far to 'punch' client X axis when climbing
#define CLIMB_PUNCH_Z			7	// how far to 'punch' client Z axis when climbing

char hitlog[8][64] =
{
	{"head"},
	{"torso"},
	{"arms/legs"},
	{"generic bodypart"},
	{"generic bodypart"},
	{"generic bodypart"},
	{"generic bodypart"},
	{"generic bodypart"}
};

#define REHUMAN_KILLS 5
#define REHUMAN_TIMER 120
extern int g_teamplay;
extern float humanStatsRatio;

void CBasePlayer::PreThink(void)
{
  if( g_teamplay == 2 && statsRatio != humanStatsRatio )
  {
    float tmp = pev->health / PLR_HP(this);
    statsRatio = humanStatsRatio;
    pev->health = tmp * PLR_HP(this);
    pev->max_health = PLR_HP(this);

   	g_engfuncs.pfnSetClientMaxspeed( ENT(pev), SPEED_FORMULA );
    MESSAGE_BEGIN( MSG_ONE, gmsgPlayMusic, NULL, edict( ) );
      WRITE_BYTE( 3 );
      WRITE_COORD( SPEED_FORMULA );
    MESSAGE_END( );
  }

  if( pev->fuser4 )
  {
     ClearBits( pev->button, IN_DUCK );
     ClearBits( pev->flags, FL_DUCKING );
     pev->view_ofs[2] = 21;
  }
  if( m_bTransform )
    m_fNextSuicideTime = 0;

  if( g_teamplay == 1 )
  {
    // Enough kills || enough time -> optional rehuman
    if( !rehbutton && m_sInfo.team == 2 && m_fTransformTime && ( ( m_fTransformTime <= gpGlobals->time ) || ( zombieKills <= 0 ) ) && !m_bTransform )
    {
      rehbutton = true;
      MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, edict( ) );
		    WRITE_COORD( 4 );
		    WRITE_BYTE( NTM_REHUMAN );
	    MESSAGE_END( );
    }
    
    // 2 * Enough kills -> force rehuman
    if( m_sInfo.team == 2 && rehbutton && m_fTransformTime && ( ( zombieKills <= -REHUMAN_KILLS ) ) && !m_bTransform )
    {
        //m_fTransformTime = 0;
        m_sInfo.team = 1;
        rehbutton = false;
        MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, edict( ) );
		    WRITE_COORD( -1 );
		    WRITE_BYTE( NTM_REHUMAN_HIDE );
	      MESSAGE_END( );
    }

    /*
    Insta teamchange
    if( m_iTeam == 2 && m_fTransformTime && ( m_fTransformTime <= gpGlobals->time ) && !m_bTransform )
    {
        m_fTransformTime = 0;
        m_sInfo.team = 1;
        rehbutton = false;
        MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, edict( ) );
		    WRITE_COORD( -1 );
		    WRITE_BYTE( NTM_REHUMAN_HIDE );
	      MESSAGE_END( );
        pev->health = 0;
        Killed( pev, GIB_NEVER );
        pev->frags -= 9;
    }*/
  }

  if( m_fNextSuicideTime && m_fNextSuicideTime <= gpGlobals->time )
  {
    pev->health = 0;
    Killed( pev, GIB_NEVER );
    m_fNextSuicideTime = 0;
    pev->frags -= 9;
    /*MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		  WRITE_BYTE( ENTINDEX( edict( ) ) );
		  WRITE_SHORT( pev->frags );
		  WRITE_SHORT( m_iDeaths );
	  MESSAGE_END( );*/
  }

  if( m_bNoAutospawn && m_fNextAutospawn <= gpGlobals->time )
  {
    m_bNoAutospawn = FALSE;
    m_fNextAutospawn = 0;
  }

  while( flame.size( ) > 3 )
    flame.pop_front( );

  if( flame.size( ) )
  {
    if( nextpop <= gpGlobals->time )
    {
      flame.pop_front( );
      nextpop = gpGlobals->time + 0.32;
    }
    else if( nextburn <= gpGlobals->time )
    {
      list<t_flame_part>::iterator i;
      for( i = flame.begin( ); i != flame.end( ); i++ )
      {
          CBaseEntity *ent = UTIL_FindEntityInSphere( NULL, pev->origin, 400 );
          vec3_t dir, dir2;
          //float yaw;
          while( ent != NULL )
		      {
            if( ent->pev->takedamage && ent != this && ent->pev->solid != SOLID_BSP )
            {
              dir = i->end - i->start;
              /*yaw = VecToYaw( ent->pev->origin - i->start ) - UTIL_VecToAngles( dir ).y;
              yaw = MOD( yaw, 360 );
              if( yaw > 180 ) yaw -= 360;
              if( yaw < -180 ) yaw += 360;
              if( abs(yaw) <= 15 )
                ent->TakeDamage( pev, pev, 3, i->dmgtype );*/
			  dir2 = ( ent->pev->origin + Vector( 0, 0, 10 ) ) - i->start;
			  if( DotProduct( dir.Normalize( ), dir2.Normalize( ) ) > 0.96 )
                ent->TakeDamage( pev, pev, 3, i->dmgtype );
            }
			      ent = UTIL_FindEntityInSphere( ent, pev->origin, 400 );
		      }
      }
      nextburn = gpGlobals->time + 0.5;
    }
  }
  if( IsObserver( ) )
  {
	  m_fBurning = 0;
	  m_fNextSpread = 0;
  }
  else if( m_fBurning > gpGlobals->time && m_fNextSpread <= gpGlobals->time )
  {
    CBaseEntity *ent = UTIL_FindEntityInSphere( NULL, pev->origin, 128 );
    int cnt = 0;
    while( ent != NULL && cnt < 5 )
		{
      if( ent->pev->takedamage /*&& !FClassnameIs( ent->pev, "player" )*/ )
      {
        ent->TakeDamage( pevFlamer, pevFlamer, ent == this ? 5 : 2.5, DMG_BURN );
        cnt++;
      }
			ent = UTIL_FindEntityInSphere( ent, pev->origin, 128 );
		}
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "misc/burn.wav", 1, ATTN_NORM );
    m_fNextSpread = gpGlobals->time + 0.5;
  }

  if( m_fTransformTime <= gpGlobals->time && m_bTransform )
  {
    pev->frags -= 10;
	  if( m_pActiveItem )
		  DropPlayerItem( (char*)STRING( m_pActiveItem->pev->classname ) );
    m_bTransform = FALSE;
    pev->health = PLR_HP(this);
    pev->max_health = PLR_HP(this);
    ((cPEHacking*)g_pGameRules)->SetTeam( 2, this, 0 );
    m_sInfo.team = 2;
    DropPlayerItem( "bb_objective_item" );
	  RemoveAllItems( FALSE );
	  m_iCurSlot = 1;
	  uEqPlr( this, WEAPON_HAND );
	  m_iPrimaryWeap = WEAPON_HAND;
    rehbutton = false;
    if( g_teamplay == 1 )
      m_fTransformTime = gpGlobals->time + REHUMAN_TIMER;
    else
      m_fTransformTime = 0;
    zombieKills = REHUMAN_KILLS;
    Notify( this, NTC_ZOMBIE, max( 0.0f, zombieKills ) );
		NotifyMid( this, NTM_ZOMBIE );
    MESSAGE_BEGIN( MSG_ONE, gmsgPlayMusic, NULL, edict( ) );
      WRITE_BYTE( 3 );
      WRITE_COORD( SPEED_FORMULA );
    MESSAGE_END( );
   	g_engfuncs.pfnSetClientMaxspeed( ENT(pev), SPEED_FORMULA );
    UTIL_EdictScreenFade( edict(), Vector( 128, 0, 0 ), 0.1, 50, 85, FFADE_MODULATE | FFADE_STAYOUT );
  }
  if( ( m_fTransformTime - 10 ) <= gpGlobals->time && m_bTransform && !pev->fuser4 )
  {
    pev->health = 200;
    pev->fuser4 = 1;
  }

	if( m_fNextHP && ( m_fNextHP <= gpGlobals->time ) )
	{
		pev->health += 2.5;
		if( pev->health > pev->max_health )
			pev->health = pev->max_health;
		m_fNextHP = gpGlobals->time + 1.5;
	}
	if( m_fNextAF && ( m_fNextAF <= gpGlobals->time ) )
	{
		int part = RANDOM_LONG(0,6);
		m_iArmor[part] += 2;
		if( m_iArmor[part] > m_iArmorMax[part] )
			m_iArmor[part] = m_iArmorMax[part];
		m_fNextAF = gpGlobals->time + 1.5;
	}
	if( m_fNextBullet && ( m_fNextBullet <= gpGlobals->time ) )
	{
		if( m_pActiveItem && strcmp( STRING(m_pActiveItem->pev->classname), "weapon_orbital" ) && strcmp( STRING(m_pActiveItem->pev->classname), "weapon_handgrenade" ) )
		{
			if( m_pActiveItem->pszAmmo1( ) )
			{
				float give = 100;
				if( FBitSet( m_iCyber, CB_ARMS ) )
					give += 20.0;
				if( FBitSet( m_iCyber, CB_TORSO ) )
					give += 30.0;
				if( FBitSet( m_iAddons, AD_NANO_SKIN ) )
					give -= 15.0;
				GiveAmmo( 2, (char *)m_pActiveItem->pszAmmo1( ), (int)( (float)m_pActiveItem->iMaxAmmo1( ) * ( give / 100.0 ) ) );
			}
		}
		m_fNextBullet = gpGlobals->time + 1;
	}

	if ( recoilratio.value != m_fClientRecoilratio )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgRecoilRatio, NULL, pev );
		WRITE_COORD( ( recoilratio.value > 0 ? recoilratio.value : 1 ) );
		MESSAGE_END();

		m_fClientRecoilratio = recoilratio.value;
	}
	//pev->punchangle.x = -0.03f * (float)m_iShots;
	//TraceResult trl;
	//UTIL_MakeVectors(pev->v_angle);
	//ALERT( at_console, "seq: %d  frame: %f anim: %f\n", pev->sequence, pev->frame, pev->animtime );
	
	if( m_iSpawnDelayed && ( m_fSpawn <= gpGlobals->time ) )
	{
		Spawn( );
		if( m_iSpawnDelayed )
			StartObserver( );
		else
			StopObserver( );
		if( IsAlive( ) )
			((cPEHacking*)g_pGameRules)->EquipPlayer( this );
	}
	

	int cinf;


	if( ( m_fViewCheck <= gpGlobals->time ) && ( pev->iuser1 != 3 ) )
	{
		if( m_hObserverTarget )
		{
			pev->iuser2 = ENTINDEX(m_hObserverTarget->edict( ));
			MESSAGE_BEGIN( MSG_ONE, gmsgTargHlth, NULL, edict() );
				WRITE_BYTE( (int)( m_hObserverTarget->pev->health > 0 ? m_hObserverTarget->pev->health : 0 ) );
				WRITE_BYTE( (int)( m_hObserverTarget->pev->armorvalue > 0 ? m_hObserverTarget->pev->armorvalue : 0 ) );
			MESSAGE_END();
		}
		m_fViewCheck = gpGlobals->time + 0.5;
	}


	int buttonsChanged = (m_afButtonLast ^ pev->button);	// These buttons have changed this frame
	
	// Debounced button codes for pressed/released
	// UNDONE: Do we need auto-repeat?
	m_afButtonPressed =  buttonsChanged & pev->button;		// The changed ones still down are "pressed"
	m_afButtonReleased = buttonsChanged & (~pev->button);	// The ones not down are "released"

	g_pGameRules->PlayerThink( this );

	if ( g_fGameOver )
		return;         // intermission or finale

	UTIL_MakeVectors(pev->v_angle);             // is this still used?
	
	ItemPreFrame( );
	WaterMove();

	if ( g_pGameRules && g_pGameRules->FAllowFlashlight() )
		m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_iHideHUD |= HIDEHUD_FLASHLIGHT;


	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();
	
	CheckTimeBasedDamage();

	// + spin
	if( m_flNextRadarCalc <= gpGlobals->time && IsAlive( ) )
	{
		int i = 0;
		s_radarentry *cur = fradar;

		while( cur )
		{
			if( FClassnameIs( cur->pev, "player" ) )
			{
        CBasePlayer *plr = GetClassPtr( (CBasePlayer *)cur->pev );
				if( !plr->IsAlive( ) || ( plr->m_iTeam != m_iTeam && m_iTeam != 2 ) )
				{
					if( !plr->m_bIsSpecial )
						Disable( cur->pev );
				}
			}
			cur = cur->next;
		}

		int dist = FBitSet( m_iAddons, AD_HEAD_SENTI ) ? 950 : 900;

		for( i = 1; i <= MAX_PLAYERS; i++ )
		{
			CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

			if( !plr || !plr->IsAlive( ) || ( plr->m_iTeam == 0 ) )
				continue;
			if( plr->m_bIsSpecial )
			{
				Enable( plr->pev );
				continue;
			}
			if( /*( plr->m_iTeam != m_iTeam ) && ( ( plr->pev->origin - pev->origin ).Length( ) <= dist ) ||*/ m_iTeam == 2 )
			{
				if( ( FBitSet( m_iCyber, CB_HEAD ) && plr->pev->velocity.Length( ) >= 20 ) ||
					FEntIsVisible( pev, plr->pev ) || m_iTeam == 2 )
				Enable( plr->pev );
			}
		}

		UpdateRadar( );
		m_flNextRadarCalc = gpGlobals->time + 0.6;
	}
	// - Spin

    // Observer Tastatus-Steuerung
    if ( IsObserver() )
    {
            Observer_HandleButtons();
            pev->impulse = 0;
			if( m_hObserverTarget != NULL && !m_hObserverTarget->IsAlive( ) )
			{
				m_hObserverTarget = NULL;
				Observer_SetMode( pev->iuser1 );
			}
			if( showdmg.value && ( m_fNextInfo < gpGlobals->time ) && ( ( m_iCurInfo - 32 ) < 32 ) )
			{
				char text[256];
				//ALERT( at_console, "curinfo %d\n", m_iCurInfo );
				if( ( m_iCurInfo < 32 ) && ( m_iCurInfo >= 0 ) )
				{
					cinf = m_iCurInfo;
					//ALERT( at_console, "index %d\n", dmgdone[cinf].index );
					if( !dmgdone[cinf].index )
					{
						m_iCurInfo++;
						m_fNextInfo = 0;
						return;
					}
					sprintf( text, "You did %d damage to %s (", (int)dmgdone[cinf].damage, dmgdone[cinf].name );
					//ClientPrint( pev, HUD_PRINTTALK, UTIL_VarArgs( "You did %d damage to %s:\n", (int)dmgdone[cinf].damage, dmgdone[cinf].name ) );
					for( int i = 0; i < 4; i++ )
					{
						if( !dmgdone[cinf].hitgroup[i] )
							continue;
						//ClientPrint( pev, HUD_PRINTTALK, UTIL_VarArgs( "%d hits in %s\n", dmgdone[cinf].hitgroup[i], hitlog[i] ) );
						if( ( i > 0 ) &&
							( ( i == 1 ) ? dmgdone[cinf].hitgroup[0] : 1 ) &&
							( ( i == 2 ) ? ( dmgdone[cinf].hitgroup[0] || dmgdone[cinf].hitgroup[1] ) : 1  ) )
							strcat( text, ", " );
						strcat( text, UTIL_VarArgs( "%d hits in %s", dmgdone[cinf].hitgroup[i], hitlog[i] ) );
					}
					strcat( text, ")\n" );
					ClientPrint( pev, HUD_PRINTTALK, text );
						
				}
				else if( ( ( m_iCurInfo - 32 ) < 32 ) && ( ( m_iCurInfo - 32 ) >= 0 ) )
				{
					cinf = m_iCurInfo - 32;
					//ALERT( at_console, "index %d\n", dmgdone[cinf].index );
					if( !dmgreceived[cinf].index )
					{
						m_iCurInfo++;
						m_fNextInfo = 0;
						return;
					}
					sprintf( text, "You were hit for %d damage by %s (", (int)dmgreceived[cinf].damage, dmgreceived[cinf].name );
					//ClientPrint( pev, HUD_PRINTTALK, UTIL_VarArgs( "You were hit for %d damage by %s:\n", (int)dmgreceived[cinf].damage, dmgreceived[cinf].name ) );
					for( int i = 0; i < 4; i++ )
					{
						if( !dmgreceived[cinf].hitgroup[i] )
							continue;
						if( ( i > 0 ) &&
							( ( i == 1 ) ? dmgreceived[cinf].hitgroup[0] : 1 ) &&
							( ( i == 2 ) ? ( dmgreceived[cinf].hitgroup[0] || dmgreceived[cinf].hitgroup[1] ) : 1  ) )
							strcat( text, ", " );
						strcat( text, UTIL_VarArgs( "%d hits in %s", dmgreceived[cinf].hitgroup[i], hitlog[i] ) );
						//ClientPrint( pev, HUD_PRINTTALK, UTIL_VarArgs( "%d hits in %s\n", dmgreceived[cinf].hitgroup[i], hitlog[i] ) );
					}
					strcat( text, ")\n" );
					ClientPrint( pev, HUD_PRINTTALK, text );
				}
				
				m_fNextInfo = gpGlobals->time + DMG_INFO_DELAY;
				m_iCurInfo++;
			}
            return;
    }

    if ( m_fNVG )
		NVGUpdate();

	if (pev->deadflag >= DEAD_DYING)
	{
		PlayerDeathThink();
		return;
	}

	// So the correct flags get sent to client asap.
	//
	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
		pev->flags |= FL_ONTRAIN;
	else 
		pev->flags &= ~FL_ONTRAIN;

	// Train speed control
	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
	{
		CBaseEntity *pTrain = CBaseEntity::Instance( pev->groundentity );
		float vel;
		
		if ( !pTrain )
		{
			TraceResult trainTrace;
			// Maybe this is on the other side of a level transition
			UTIL_TraceLine( pev->origin, pev->origin + Vector(0,0,-38), ignore_monsters, ENT(pev), &trainTrace );

			// HACKHACK - Just look for the func_tracktrain classname
			if ( trainTrace.flFraction != 1.0 && trainTrace.pHit )
			pTrain = CBaseEntity::Instance( trainTrace.pHit );


			if ( !pTrain || !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(pev) )
			{
				//ALERT( at_error, "In train mode with no train!\n" );
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = TRAIN_NEW|TRAIN_OFF;
				return;
			}
		}
		else if ( !FBitSet( pev->flags, FL_ONGROUND ) || FBitSet( pTrain->pev->spawnflags, SF_TRACKTRAIN_NOCONTROL ) || (pev->button & (IN_MOVELEFT|IN_MOVERIGHT) ) )
		{
			// Turn off the train if you jump, strafe, or the train controls go dead
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = TRAIN_NEW|TRAIN_OFF;
			return;
		}

		pev->velocity = g_vecZero;
		vel = 0;
		if ( m_afButtonPressed & IN_FORWARD )
		{
			vel = 1;
			pTrain->Use( this, this, USE_SET, (float)vel );
		}
		else if ( m_afButtonPressed & IN_BACK )
		{
			vel = -1;
			pTrain->Use( this, this, USE_SET, (float)vel );
		}

		if (vel)
		{
			m_iTrain = TrainSpeed(pTrain->pev->speed, pTrain->pev->impulse);
			m_iTrain |= TRAIN_ACTIVE|TRAIN_NEW;
		}

	} else if (m_iTrain & TRAIN_ACTIVE)
		m_iTrain = TRAIN_NEW; // turn off train

	if (pev->button & IN_JUMP)
	{
		// If on a ladder, jump off the ladder
		// else Jump
		Jump();
	}


	// If trying to duck, already ducked, or in the process of ducking
	if ((pev->button & IN_DUCK) || FBitSet(pev->flags,FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING) )
		Duck();

	if ( !FBitSet ( pev->flags, FL_ONGROUND ) )
	{
		m_flFallVelocity = -pev->velocity.z;
	}

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	// Clear out ladder pointer
	m_hEnemy = NULL;

	if ( m_afPhysicsFlags & PFLAG_ONBARNACLE )
	{
		pev->velocity = g_vecZero;
	}
}

void CBasePlayer::CheckTimeBasedDamage() 
{
	int i;
	BYTE bDuration = 0;

	static float gtbdPrev = 0.0;

	if (!(m_bitsDamageType & DMG_TIMEBASED))
		return;

	// only check for time based damage approx. every 2 seconds
	if (abs(gpGlobals->time - m_tbdPrev) < 2.0)
		return;
	
	m_tbdPrev = gpGlobals->time;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		// make sure bit is set for damage type
		if (m_bitsDamageType & (DMG_PARALYZE << i))
		{
			switch (i)
			{
			case itbd_Paralyze:
				// UNDONE - flag movement as half-speed
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
//				TakeDamage(pev, pev, NERVEGAS_DAMAGE, DMG_GENERIC);	
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_Poison:
				TakeDamage(pev, pev, POISON_DAMAGE, DMG_GENERIC);
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
//				TakeDamage(pev, pev, RADIATION_DAMAGE, DMG_GENERIC);
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				// NOTE: this hack is actually used to RESTORE health
				// after the player has been drowning and finally takes a breath
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = min(m_idrowndmg - m_idrownrestored, 10);

					TakeHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4;	// get up to 5*10 = 50 points back
				break;
			case itbd_Acid:
//				TakeDamage(pev, pev, ACID_DAMAGE, DMG_GENERIC);
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
//				TakeDamage(pev, pev, SLOWBURN_DAMAGE, DMG_GENERIC);
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
//				TakeDamage(pev, pev, SLOWFREEZE_DAMAGE, DMG_GENERIC);
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (m_rgbTimeBasedDamage[i])
			{
				// use up an antitoxin on poison or nervegas after a few seconds of damage					
				if (((i == itbd_NerveGas) && (m_rgbTimeBasedDamage[i] < NERVEGAS_DURATION)) ||
					((i == itbd_Poison)   && (m_rgbTimeBasedDamage[i] < POISON_DURATION)))
				{
					if (m_rgItems[ITEM_ANTIDOTE])
					{
						m_rgbTimeBasedDamage[i] = 0;
						m_rgItems[ITEM_ANTIDOTE]--;
						SetSuitUpdate("!HEV_HEAL4", FALSE, SUIT_REPEAT_OK);
					}
				}


				// decrement damage duration, detect when done.
				if (!m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					// if we're done, clear damage bits
					m_bitsDamageType &= ~(DMG_PARALYZE << i);	
				}
			}
			else
				// first time taking this damage type - init damage duration
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}

// if in range of radiation source, ping geiger counter

#define GEIGERDELAY 0.25

void CBasePlayer :: UpdateGeigerCounter( void )
{
	BYTE range;

	// delay per update ie: don't flood net with these msgs
	if (gpGlobals->time < m_flgeigerDelay)
		return;

	m_flgeigerDelay = gpGlobals->time + GEIGERDELAY;
		
	// send range to radition source to client

	range = (BYTE) (m_flgeigerRange / 4);

	if (range != m_igeigerRangePrev)
	{
		m_igeigerRangePrev = range;

		MESSAGE_BEGIN( MSG_ONE, gmsgGeigerRange, NULL, pev );
			WRITE_BYTE( range );
		MESSAGE_END();
	}

	// reset counter and semaphore
	if (!RANDOM_LONG(0,3))
		m_flgeigerRange = 1000;

}

/*
================
CheckSuitUpdate

Play suit update if it's time
================
*/

#define SUITUPDATETIME	3.5
#define SUITFIRSTUPDATETIME 0.1

void CBasePlayer::CheckSuitUpdate()
{
	int isentence = 0;
	int isearch = m_iSuitPlayNext;
	
	// Ignore suit updates if no suit
	if ( !(pev->weapons & (1<<WEAPON_SUIT)) )
		return;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();

	//if ( g_pGameRules->IsMultiplayer() )
	//{
		// don't bother updating HEV voice in multiplayer.
		return;
}
 
// add sentence to suit playlist queue. if fgroup is true, then
// name is a sentence group (HEV_AA), otherwise name is a specific
// sentence name ie: !HEV_AA0.  If iNoRepeat is specified in
// seconds, then we won't repeat playback of this word or sentence
// for at least that number of seconds.

void CBasePlayer::SetSuitUpdate(char *name, int fgroup, int iNoRepeatTime)
{
	int i;
	int isentence;
	int iempty = -1;
	
	
	// Ignore suit updates if no suit
	if ( !(pev->weapons & (1<<WEAPON_SUIT)) )
		return;

	if ( g_pGameRules->IsMultiplayer() )
	{
		// due to static channel design, etc. We don't play HEV sounds in multiplayer right now.
		return;
	}

	// if name == NULL, then clear out the queue

	if (!name)
	{
		for (i = 0; i < CSUITPLAYLIST; i++)
			m_rgSuitPlayList[i] = 0;
		return;
	}
	// get sentence or group number
	if (!fgroup)
	{
		isentence = SENTENCEG_Lookup(name, NULL);
		if (isentence < 0)
			return;
	}
	else
		// mark group number as negative
		isentence = -SENTENCEG_GetIndex(name);

	// check norepeat list - this list lets us cancel
	// the playback of words or sentences that have already
	// been played within a certain time.

	for (i = 0; i < CSUITNOREPEAT; i++)
	{
		if (isentence == m_rgiSuitNoRepeat[i])
			{
			// this sentence or group is already in 
			// the norepeat list

			if (m_rgflSuitNoRepeatTime[i] < gpGlobals->time)
				{
				// norepeat time has expired, clear it out
				m_rgiSuitNoRepeat[i] = 0;
				m_rgflSuitNoRepeatTime[i] = 0.0;
				iempty = i;
				break;
				}
			else
				{
				// don't play, still marked as norepeat
				return;
				}
			}
		// keep track of empty slot
		if (!m_rgiSuitNoRepeat[i])
			iempty = i;
	}

	// sentence is not in norepeat list, save if norepeat time was given

	if (iNoRepeatTime)
	{
		if (iempty < 0)
			iempty = RANDOM_LONG(0, CSUITNOREPEAT-1); // pick random slot to take over
		m_rgiSuitNoRepeat[iempty] = isentence;
		m_rgflSuitNoRepeatTime[iempty] = iNoRepeatTime + gpGlobals->time;
	}

	// find empty spot in queue, or overwrite last spot
	
	m_rgSuitPlayList[m_iSuitPlayNext++] = isentence;
	if (m_iSuitPlayNext == CSUITPLAYLIST)
		m_iSuitPlayNext = 0;

	if (m_flSuitUpdate <= gpGlobals->time)
	{
		if (m_flSuitUpdate == 0)
			// play queue is empty, don't delay too long before playback
			m_flSuitUpdate = gpGlobals->time + SUITFIRSTUPDATETIME;
		else 
			m_flSuitUpdate = gpGlobals->time + SUITUPDATETIME; 
	}

}

/*
================
CheckPowerups

Check for turning off powerups

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
================
*/
	static void
CheckPowerups(entvars_t *pev)
{
	if (pev->health <= 0)
		return;

	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes
}


//=========================================================
// UpdatePlayerSound - updates the position of the player's
// reserved sound slot in the sound list.
//=========================================================
void CBasePlayer :: UpdatePlayerSound ( void )
{
	int iBodyVolume;
	int iVolume;
	CSound *pSound;

	pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt :: ClientSoundIndex( edict() ) );

	if ( !pSound )
	{
		ALERT ( at_console, "Client lost reserved sound!\n" );
		return;
	}

	pSound->m_iType = bits_SOUND_NONE;

	// now calculate the best target volume for the sound. If the player's weapon
	// is louder than his body/movement, use the weapon volume, else, use the body volume.
	
	if ( FBitSet ( pev->flags, FL_ONGROUND ) )
	{	
		iBodyVolume = pev->velocity.Length(); 

		// clamp the noise that can be made by the body, in case a push trigger,
		// weapon recoil, or anything shoves the player abnormally fast. 
		if ( iBodyVolume > 512 )
		{
			iBodyVolume = 512;
		}
	}
	else
	{
		iBodyVolume = 0;
	}

	if ( pev->button & IN_JUMP )
	{
		iBodyVolume += 100;
	}

// convert player move speed and actions into sound audible by monsters.
	if ( m_iWeaponVolume > iBodyVolume )
	{
		m_iTargetVolume = m_iWeaponVolume;

		// OR in the bits for COMBAT sound if the weapon is being louder than the player. 
		pSound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		m_iTargetVolume = iBodyVolume;
	}

	// decay weapon volume over time so bits_SOUND_COMBAT stays set for a while
	m_iWeaponVolume -= 250 * gpGlobals->frametime;
	if ( m_iWeaponVolume < 0 )
	{
		iVolume = 0;
	}


	// if target volume is greater than the player sound's current volume, we paste the new volume in 
	// immediately. If target is less than the current volume, current volume is not set immediately to the
	// lower volume, rather works itself towards target volume over time. This gives monsters a much better chance
	// to hear a sound, especially if they don't listen every frame.
	iVolume = pSound->m_iVolume;

	if ( m_iTargetVolume > iVolume )
	{
		iVolume = m_iTargetVolume;
	}
	else if ( iVolume > m_iTargetVolume )
	{
		iVolume -= 250 * gpGlobals->frametime;

		if ( iVolume < m_iTargetVolume )
		{
			iVolume = 0;
		}
	}

	if ( m_fNoPlayerSound )
	{
		// debugging flag, lets players move around and shoot without monsters hearing.
		iVolume = 0;
	}

	if ( gpGlobals->time > m_flStopExtraSoundTime )
	{
		// since the extra sound that a weapon emits only lasts for one client frame, we keep that sound around for a server frame or two 
		// after actual emission to make sure it gets heard.
		m_iExtraSoundTypes = 0;
	}

	if ( pSound )
	{
		pSound->m_vecOrigin = pev->origin;
		pSound->m_iType |= ( bits_SOUND_PLAYER | m_iExtraSoundTypes );
		pSound->m_iVolume = iVolume;
	}

	// keep track of virtual muzzle flash
	m_iWeaponFlash -= 256 * gpGlobals->frametime;
	if (m_iWeaponFlash < 0)
		m_iWeaponFlash = 0;

	//UTIL_MakeVectors ( pev->angles );
	//gpGlobals->v_forward.z = 0;

	// Below are a couple of useful little bits that make it easier to determine just how much noise the 
	// player is making. 
	// UTIL_ParticleEffect ( pev->origin + gpGlobals->v_forward * iVolume, g_vecZero, 255, 25 );
	//ALERT ( at_console, "%d/%d\n", iVolume, m_iTargetVolume );
}


void CBasePlayer::PostThink()
{
	if ( g_fGameOver )
		goto pt_end;         // intermission or finale

	if (!IsAlive())
		goto pt_end;

	// Handle Tank controlling
	if ( m_pTank != NULL )
	{ // if they've moved too far from the gun,  or selected a weapon, unuse the gun
		if ( m_pTank->OnControls( pev ) && !pev->weaponmodel )
		{  
			m_pTank->Use( this, this, USE_SET, 2 );	// try fire the gun
		}
		else
		{  // they've moved off the platform
			m_pTank->Use( this, this, USE_OFF, 0 );
			m_pTank = NULL;
		}
	}

// do weapon stuff
	ItemPostFrame( );

// check to see if player landed hard enough to make a sound
// falling farther than half of the maximum safe distance, but not as far a max safe distance will
// play a bootscrape sound, and no damage will be inflicted. Fallling a distance shorter than half
// of maximum safe distance will make no sound. Falling farther than max safe distance will play a 
// fallpain sound, and damage will be inflicted based on how far the player fell

	if ( (FBitSet(pev->flags, FL_ONGROUND)) && (pev->health > 0) && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD )
	{
		// ALERT ( at_console, "%f\n", m_flFallVelocity );

		if (pev->watertype == CONTENT_WATER)
		{
			// Did he hit the world or a non-moving entity?
			// BUG - this happens all the time in water, especially when 
			// BUG - water has current force
			// if ( !pev->groundentity || VARS(pev->groundentity)->velocity.z == 0 )
				// EMIT_SOUND(ENT(pev), CHAN_BODY, "player/pl_wade1.wav", 1, ATTN_NORM);
		}
		else if ( m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED )
		{// after this point, we start doing damage
			
			float flFallDamage = g_pGameRules->FlPlayerFallDamage( this );

			if( FBitSet( m_iCyber, CB_LEGS ) )
				flFallDamage *= 0.4;

			if ( flFallDamage > pev->health )
			{//splat
				// note: play on item channel because we play footstep landing on body channel
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM);
			}

			if ( flFallDamage > 0 )
			{
				TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), flFallDamage, DMG_FALL ); 
				pev->punchangle.x = 0;
			}
		}

		if ( IsAlive() )
		{
			SetAnimation( PLAYER_WALK );
		}
    }

	if (FBitSet(pev->flags, FL_ONGROUND))
	{		
		if (m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer())
		{
			CSoundEnt::InsertSound ( bits_SOUND_PLAYER, pev->origin, m_flFallVelocity, 0.2 );
			// ALERT( at_console, "fall %f\n", m_flFallVelocity );
		}
		m_flFallVelocity = 0;
	}

	// select the proper animation for the player character	
	if ( IsAlive() )
	{
		if (!pev->velocity.x && !pev->velocity.y)
			SetAnimation( PLAYER_IDLE );
		else if ((pev->velocity.x || pev->velocity.y) && (FBitSet(pev->flags, FL_ONGROUND)))
			SetAnimation( PLAYER_WALK );
		else if (pev->waterlevel > 1)
			SetAnimation( PLAYER_WALK );
	}

	StudioFrameAdvance( );
	CheckPowerups(pev);

	UpdatePlayerSound();

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;

pt_end:
#if defined( CLIENT_WEAPONS )
		// Decay timers on weapons
	// go through all of the weapons and make a list of the ones to pack
	for ( int i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			CBasePlayerItem *pPlayerItem = m_rgpPlayerItems[ i ];
      CBasePlayerItem *pit = m_rgpPlayerItems[ i ];

			while ( pPlayerItem  )
			{
				CBasePlayerWeapon *gun;

				gun = (CBasePlayerWeapon *)pPlayerItem->GetWeaponPtr();
				
				if ( gun && gun->UseDecrement() )
				{
					gun->m_flNextPrimaryAttack		= max( (float)gun->m_flNextPrimaryAttack - (float)gpGlobals->frametime, -1.0f );
					gun->m_flNextSecondaryAttack	= max( (float)gun->m_flNextSecondaryAttack - (float)gpGlobals->frametime, -0.001f );

					if ( gun->m_flTimeWeaponIdle != 1000 )
					{
						gun->m_flTimeWeaponIdle		= max( (float)gun->m_flTimeWeaponIdle - (float)gpGlobals->frametime, -0.001f );
					}

					if ( gun->pev->fuser1 != 1000 )
					{
						gun->pev->fuser1	= max( (float)gun->pev->fuser1 - (float)gpGlobals->frametime, -0.001f );
					}

					// Only decrement if not flagged as NO_DECREMENT
//					if ( gun->m_flPumpTime != 1000 )
				//	{
				//		gun->m_flPumpTime	= max( gun->m_flPumpTime - gpGlobals->frametime, -0.001 );
				//	}
					
				}

				//pPlayerItem = pPlayerItem->m_pNext;
        //break;
        pPlayerItem = NULL;
			}
		}
	}

	m_flNextAttack -= gpGlobals->frametime;
	if ( m_flNextAttack < -0.001 )
		m_flNextAttack = -0.001;
	
	if ( m_flNextAmmoBurn != 1000 )
	{
		m_flNextAmmoBurn -= gpGlobals->frametime;
		
		if ( m_flNextAmmoBurn < -0.001 )
			m_flNextAmmoBurn = -0.001;
	}

	if ( m_flAmmoStartCharge != 1000 )
	{
		m_flAmmoStartCharge -= gpGlobals->frametime;
		
		if ( m_flAmmoStartCharge < -0.001 )
			m_flAmmoStartCharge = -0.001;
	}
	

#else
	return;
#endif
}


// checks if the spot is clear of players
/*BOOL IsSpawnPointValid( CBaseEntity *pPlayer, CBaseEntity *pSpot )
{
	CBaseEntity *ent = NULL;

	if ( !pSpot->IsTriggered( pPlayer ) )
	{
		return FALSE;
	}

	ent = UTIL_FindEntityInSphere( NULL, pSpot->pev->origin, 256 );
	while( ent != NULL )
	{
		// if ent is a client, don't spawn on 'em
		if ( ent->IsPlayer() && ( ent != pPlayer ) )
			return FALSE;
		ent = UTIL_FindEntityInSphere( ent, pSpot->pev->origin, 256 );
	}

	return TRUE;
}*/


DLL_GLOBAL CBaseEntity	*g_pLastSpawn;
DLL_GLOBAL CBaseEntity	*g_pLastSpawnSynd;
DLL_GLOBAL CBaseEntity	*g_pLastSpawnSec;
inline int FNullEnt( CBaseEntity *ent ) { return (ent == NULL) || FNullEnt( ent->edict() ); }

edict_t *EntSelectSpawnPoint( CBasePlayer *pPlayer )
{
	/*s_spawnlist *list = NULL;
	for( int i = 0; i < 33; i++ )
	{
		if( pPlayer->m_iTeam == 1 )
		list = gListCorp;
		else
		list = gListSynd;

		if( ( !list[i].player || list[i].player == pPlayer ) && list[i].spot &&
		( list[i].spot->pev->origin != Vector( 0, 0, 0 ) ) )
		{
			list[i].player = pPlayer;
			if( pPlayer->m_iTeam == 2 )
			{
				CBaseEntity *ent = NULL;
				while ( (ent = UTIL_FindEntityInSphere( ent, list[i].spot->pev->origin, 128 )) != NULL )
				{
					if( !(ent->edict() == pPlayer->edict( )) )
					ent->TakeDamage( VARS(INDEXENT(0)), VARS(INDEXENT(0)), 300, DMG_GENERIC );
				}
			}
			return list[i].spot->edict( );
		}
	}
	return NULL;*/
	s_spawnlist *list = NULL;
	if( pPlayer && ( pPlayer->m_iTeam == 1 || pPlayer->m_iTeam == 0 ) )
		list = gListCorp;
	else
  {
		list = gListSynd;
    if( !list[0].spot )
      list = gListBackup;
  }
	if( !list[0].spot )
		uPrepSpawn( );
 	if( !list[0].spot )
  	return NULL;


	int mx;
	for( mx = 0; mx < MAX_SPAWN_SPOTS; mx++ )
	{
		if( !list[mx].spot )
			break;
	}
	mx--;

	for( int i = 0; i < 30; i++ )
	{
		int cur = RANDOM_LONG( 0, mx );
		if( pPlayer && ( pPlayer->m_iTeam == 1 || pPlayer->m_iTeam == 0 ) )
		{
			CBaseEntity *ent = NULL;
			if( (ent = UTIL_FindEntityInSphere( ent, list[cur].spot->pev->origin, 128 ) ) != NULL )
			{
        if( ent->IsAlive( ) && !(ent->edict() == pPlayer->edict( )) )
					continue;
			}
			return list[cur].spot->edict( );
		}
		else if( !pPlayer || pPlayer->m_iTeam == 2 )
		{
			CBaseEntity *ent = NULL;
			while ( (ent = UTIL_FindEntityInSphere( ent, list[cur].spot->pev->origin, 180 ) ) != NULL )
			{
				if( !pPlayer || !(ent->edict() == pPlayer->edict( )) )
					ent->TakeDamage( VARS(INDEXENT(0)), VARS(INDEXENT(0)), 300, DMG_GENERIC );
			}
			return list[cur].spot->edict( );
		}		
	}
	return NULL;
}
void CBasePlayer::Spawn( void )
{
  statsRatio = 1;
	ResetRadar( );

  if( m_iTeam == 2 )
  {
		UTIL_EdictScreenFade( edict(), Vector( 128, 0, 0 ), 0.1, 50, 85, FFADE_MODULATE | FFADE_STAYOUT );
    pev->fuser4 = 1;
  }
  else
  {
		UTIL_EdictScreenFade( edict(), Vector( 0, 0, 0 ), 0.1, 50, 0, FFADE_MODULATE /*| FFADE_STAYOUT*/ );
    pev->fuser4 = 0;
  }
	int i = 0, temp = m_iClass;

	m_fBurning = 0;
  pickupused.clear( );
	nextpop = 0;
	flame.clear( );
	observing = 0;
	m_bEquiped = FALSE;
	m_fSpread = 0.02;
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	if ( m_pNoise )
	{
		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;
	}
	if ( m_pNoise2 )
	{
		UTIL_Remove( m_pNoise2 );
		m_pNoise2 = NULL;
	}
	m_fStartBeam = 0;

		m_iClass = m_iPrevClass;

	pev->health	= PLR_HP(this);
  pev->max_health = PLR_HP(this);
	
	CountDown( this, 0 );
	
  MESSAGE_BEGIN( MSG_ONE, gmsgPlayMusic, NULL, edict( ) );
      WRITE_BYTE( 3 );
      WRITE_COORD( SPEED_FORMULA );
    MESSAGE_END( );

	for( i = 0; i < 7; i++ )
		m_iArmorMax[i] = m_iArmor[i];

	g_engfuncs.pfnSetClientMaxspeed( ENT(pev), SPEED_FORMULA );
	pev->classname		= MAKE_STRING("player");
	pev->takedamage		= DAMAGE_AIM;
	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_WALK;
	pev->max_health		= pev->health;
	pev->flags		   &= FL_PROXY;	// keep proxy flag sey by engine
	pev->flags		   |= FL_CLIENT;
	pev->air_finished	= gpGlobals->time + 12;
	pev->dmg			= 2;				// initial water damage
	pev->effects		= 0;
	pev->deadflag		= DEAD_NO;
	pev->dmg_take		= 0;
	pev->dmg_save		= 0;
	pev->friction		= 1.0;
	pev->gravity		= 1.0;
	m_bitsHUDDamage		= -1;
	m_bitsDamageType	= 0;
	m_afPhysicsFlags	= 0;
	m_fLongJump			= FALSE;// no longjump module. 
	m_iHideHUD = NULL;
	m_bSpeciallyNotified = 0;
	m_iSlots[0] = 0;
	m_iSlots[1] = 0;
	//m_iHUDInited = TRUE;
	MESSAGE_BEGIN( MSG_ONE, gmsgHideWeapon, NULL, pev );
		WRITE_BYTE( m_iHideHUD );
	MESSAGE_END( );

  if( m_iTeam == 1 )
  {
    m_fTransformTime = 0;
    m_bTransform = FALSE;
    rehbutton = false;

    MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, edict( ) );
      WRITE_COORD( -1 );
      WRITE_BYTE( NTM_REHUMAN_HIDE );
    MESSAGE_END( );
  }


  slot1f = 0;
  slot2f = 0;
  slot3f = 0;

	m_iClientHideHUD = m_iHideHUD;
	
	m_iRespawnFrames = 0;
	pev->renderfx = kRenderFxNone;

	UpdateArmor( );

	SET_VIEW( edict( ), edict( ) );

	m_flNextRadarCalc = gpGlobals->time + 0.1;
	
	MESSAGE_BEGIN( MSG_ONE, gmsgRadarOff, NULL, pev );
		 WRITE_BYTE( 1 );
	MESSAGE_END( );
	InitRadar( );
	EnableTeam( pev, m_iTeam );

	// - ste0
	//if ( m_fNVG )
	//{
		if ( m_fNVGActive )
			NVGActive( FALSE ); 
		
		m_fNVG = FALSE; 
		MESSAGE_BEGIN(MSG_ONE, gmsgNVG, NULL, pev);
			WRITE_BYTE( 0 );
        MESSAGE_END( );

	//}
	m_fNVG = FALSE;
	m_fNVGActive = FALSE;
	m_flInfraredUpdate = gpGlobals->time;

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );

	pev->fov = m_iFOV				= 0;// init field of view.
	m_iClientFOV		= -1; // make sure fov reset is sent

	m_flNextDecalTime	= 0;// let this player decal as soon as he spawns.

	m_flgeigerDelay = gpGlobals->time + 2.0;	// wait a few seconds until user-defined message registrations
												// are recieved by all clients
	
	m_flTimeStepSound	= 0;
	m_iStepLeft = 0;
	m_flFieldOfView		= 0.5;// some monsters use this to determine whether or not the player is looking at them.

	m_bloodColor	= BLOOD_COLOR_RED;
	m_flNextAttack	= UTIL_WeaponTimeBase();
	StartSneaking();

	m_iFlashBattery = 99;
	m_flFlashLightTime = 1; // force first message

// dont let uninitialized value here hurt the player
	m_flFallVelocity = 0;

	g_pGameRules->SetDefaultPlayerTeam( this );
	m_iSpawnDelayed = 0;

	edict_t *pentSpawnSpot = EntSelectSpawnPoint( this );
	if( !pentSpawnSpot && !( pentSpawnSpot = EntSelectSpawnPoint( this ) ) )//g_pGameRules->GetPlayerSpawnSpot( this ) )
	{
		ALERT( at_logged, "NO SPAWN SPOT FOUND!\n" );
		ClientPrint( pev, HUD_PRINTTALK, "No Spawn point fount. You will spawn delayed\n" );
		m_fSpawn = gpGlobals->time + 2;
		m_iSpawnDelayed = 1;
	}
	else
	{
		pev->origin = VARS(pentSpawnSpot)->origin + Vector(0,0,1);
		pev->v_angle  = g_vecZero;
		pev->velocity = g_vecZero;
		pev->angles = VARS(pentSpawnSpot)->angles;
		pev->punchangle = g_vecZero;
		pev->fixangle = TRUE;
		if( pentSpawnSpot->v.target )
			FireTargets( STRING(pentSpawnSpot->v.target), this, this, USE_TOGGLE, 0 );
	}

    SET_MODEL(ENT(pev), "models/player.mdl");
    g_ulModelIndexPlayer = pev->modelindex;
	pev->sequence		= LookupActivity( ACT_IDLE );

	if ( FBitSet(pev->flags, FL_DUCKING) ) 
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);

    pev->view_ofs = VEC_VIEW;
	Precache();
	m_HackedGunPos		= Vector( 0, 32, 0 );

	if ( m_iPlayerSound == SOUNDLIST_EMPTY )
	{
		ALERT ( at_console, "Couldn't alloc player sound slot!\n" );
	}

	m_fNoPlayerSound = FALSE;// normal sound behavior.

	m_pLastItem = NULL;
	//m_fInitHUD = TRUE;
	m_iClientHideHUD = -1;  // force this to be recalculated
	m_fClientRecoilratio = -1;
	m_fWeapon = FALSE;
	m_pClientActiveItem = NULL;
	m_iClientBattery = -1;

	// reset all ammo values to 0
	for ( i = 0; i < MAX_AMMO_SLOTS; i++ )
	{
		m_rgAmmo[i] = 0;
		m_rgAmmoLast[i] = 0;  // client ammo values also have to be reset  (the death hud clear messages does on the client side)
	}

	m_lastx = m_lasty = 0;
	
	m_flNextChatTime = gpGlobals->time;

  InitRadar( );

	g_pGameRules->PlayerSpawn( this );
}

void CBasePlayer::InitRadar( )
{
	int i;
	/*s_radarentry *re = fradar;
	while( re )
	{
		re->clientorigin = re->origin - Vector( 10, 0, 0 );
		re->clientcolor = re->color + 5;
		re = re->next;
	}*/
	for( i = 1; i <= MAX_PLAYERS; i++ ) // Teammembers zum Radar hinzufügen
	{		
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if( plr && ( ( plr->m_iTeam == this->m_iTeam ) || m_bIsSpecial ) && ( plr != this ) && strlen( STRING( plr->pev->netname ) ) )
			Enable( plr->pev );
	}
	EnableTeam( pev, m_iTeam );

	CBaseEntity *pSpot = NULL;
	
	pSpot = UTIL_FindEntityByClassname( NULL, "pe_radar_mark" );
	while (pSpot != NULL)
	{
		Enable(pSpot->pev);
		pSpot = UTIL_FindEntityByClassname( pSpot, "pe_radar_mark" );
	}

	/*pSpot = UTIL_FindEntityByClassname( NULL, "pe_object_case" );
	if( pSpot != NULL )
	{
		Enable(pSpot->pev);
		pSpot = UTIL_FindEntityByClassname( pSpot, "pe_object_case" );
	}
	
	pSpot = UTIL_FindEntityByClassname( NULL, "pe_terminal" );	
	while (pSpot != NULL)
	{
		Enable(pSpot->pev);
		pSpot = UTIL_FindEntityByClassname( pSpot, "pe_terminal" );
	}*/
}


void CBasePlayer :: Precache( void )
{
	// in the event that the player JUST spawned, and the level node graph
	// was loaded, fix all of the node graph pointers before the game starts.
	
	// !!!BUGBUG - now that we have multiplayer, this needs to be moved!
	if ( WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet )
	{
		if ( !WorldGraph.FSetGraphPointers() )
		{
			ALERT ( at_console, "**Graph pointers were not set!\n");
		}
		else
		{
			ALERT ( at_console, "**Graph Pointers Set!\n" );
		} 
	}

	// SOUNDS / MODELS ARE PRECACHED in ClientPrecache() (game specific)
	// because they need to precache before any clients have connected

	// init geiger counter vars during spawn and each time
	// we cross a level transition

	m_flgeigerRange = 1000;
	m_igeigerRangePrev = 1000;

	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;

	m_iClientBattery = -1;

	m_iTrain = TRAIN_NEW;

	// Make sure any necessary user messages have been registered
	LinkUserMessages();

	m_iUpdateTime = 5;  // won't update for 1/2 a second

	if ( gInitHUD )
		m_fInitHUD = TRUE;
}


int CBasePlayer::Save( CSave &save )
{
	if ( !CBaseMonster::Save(save) )
		return 0;

	return save.WriteFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData) );
}


//
// Marks everything as new so the player will resend this to the hud.
//
void CBasePlayer::RenewItems(void)
{

}


int CBasePlayer::Restore( CRestore &restore )
{
	if ( !CBaseMonster::Restore(restore) )
		return 0;

	int status = restore.ReadFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE(m_playerSaveData) );

	SAVERESTOREDATA *pSaveData = (SAVERESTOREDATA *)gpGlobals->pSaveData;
	// landmark isn't present.
	if ( !pSaveData->fUseLandmark )
	{
		ALERT( at_console, "No Landmark:%s\n", pSaveData->szLandmarkName );

		// default to normal spawn
		edict_t* pentSpawnSpot = EntSelectSpawnPoint( this );
		pev->origin = VARS(pentSpawnSpot)->origin + Vector(0,0,1);
		pev->angles = VARS(pentSpawnSpot)->angles;
	}
	pev->v_angle.z = 0;	// Clear out roll
	pev->angles = pev->v_angle;

	pev->fixangle = TRUE;           // turn this way immediately

// Copied from spawn() for now
	m_bloodColor	= BLOOD_COLOR_RED;

    g_ulModelIndexPlayer = pev->modelindex;

	if ( FBitSet(pev->flags, FL_DUCKING) ) 
	{
		// Use the crouch HACK
		//FixPlayerCrouchStuck( edict() );
		// Don't need to do this with new player prediction code.
		UTIL_SetSize(pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
	}
	else
	{
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	}

	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );

	if ( m_fLongJump )
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "1" );
	}
	else
	{
		g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
	}

	RenewItems();

#if defined( CLIENT_WEAPONS )
	// HACK:	This variable is saved/restored in CBaseMonster as a time variable, but we're using it
	//			as just a counter.  Ideally, this needs its own variable that's saved as a plain float.
	//			Barring that, we clear it out here instead of using the incorrect restored time value.
	m_flNextAttack = UTIL_WeaponTimeBase();
#endif

	return status;
}



void CBasePlayer::SelectNextItem( int iItem )
{
	CBasePlayerItem *pItem;

	pItem = m_rgpPlayerItems[ iItem ];
	
	if (!pItem)
		return;

	if (pItem == m_pActiveItem)
	{
		// select the next one in the chain
		pItem = m_pActiveItem->m_pNext; 
		if (! pItem)
		{
			return;
		}

		CBasePlayerItem *pLast;
		pLast = pItem;
		while (pLast->m_pNext)
			pLast = pLast->m_pNext;

		// relink chain
		pLast->m_pNext = m_pActiveItem;
		m_pActiveItem->m_pNext = NULL;
		m_rgpPlayerItems[ iItem ] = pItem;
	}

	ResetAutoaim( );

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster( );
	}
	
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy( );
		m_pActiveItem->UpdateItemInfo( );
	}
}

CBasePlayerItem *CBasePlayer::ItemByName( const char *pstr )
{
	CBasePlayerItem *pItem = NULL;

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			pItem = m_rgpPlayerItems[i];
	
			while (pItem)
			{
				if (FClassnameIs(pItem->pev, pstr))
					break;
				pItem = NULL;//pItem->m_pNext;
			}
		}

		if (pItem)
			break;
	}

	return pItem;
}

void CBasePlayer::SelectItem(const char *pstr)
{
	if (!pstr)
		return;
	CBasePlayerItem *pItem = NULL;

	for (int i = 0; i < MAX_ITEM_TYPES; i++)
	{
		if (m_rgpPlayerItems[i])
		{
			pItem = m_rgpPlayerItems[i];
	
			while (pItem)
			{
        const char *bla = STRING( pItem->pev->classname );
        if( !strcmp( bla, "weapon_handgrenade" ) )
          pItem->pev->classname = MAKE_STRING( "weapon_canister" );
        else if( !strcmp( bla, "weapon_tavor" ) )
          pItem->pev->classname = MAKE_STRING( "weapon_winchester" );
				if (FClassnameIs(pItem->pev, pstr))
					break;
				pItem = NULL;//pItem->m_pNext;
			}
		}

		if (pItem)
			break;
	}

	if (!pItem)
		return;

	
  if (pItem == m_pActiveItem || !pItem->CanDeploy( ) )
		return;

	ResetAutoaim( );

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster( );
	
	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy( );
		m_pActiveItem->UpdateItemInfo( );
	}
}


void CBasePlayer::SelectLastItem(void)
{
	if( HasNamedPlayerItem( "weapon_case" ) )
		return;
	if (!m_pLastItem)
	{
		if( m_iPrimaryWeap )
		{
			ItemInfo& II = CBasePlayerItem::ItemInfoArray[m_iPrimaryWeap];
			if ( !II.iId )
				return;
			SelectItem( II.pszName );
		}
		return;
	}

	if ( m_pActiveItem && !m_pActiveItem->CanHolster() || !m_pLastItem->CanDeploy( ) )
	{
		return;
	}

	ResetAutoaim( );

	// FIX, this needs to queue them up and delay
	if (m_pActiveItem)
		m_pActiveItem->Holster( );
	
	CBasePlayerItem *pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;
	m_pActiveItem->Deploy( );
	m_pActiveItem->UpdateItemInfo( );
}

//==============================================
// HasWeapons - do I have any weapons at all?
//==============================================
BOOL CBasePlayer::HasWeapons( void )
{
	int i;

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( m_rgpPlayerItems[ i ] )
		{
			return TRUE;
		}
	}

	return FALSE;
}

void CBasePlayer::SelectPrevItem( int iItem )
{
}


const char *CBasePlayer::TeamID( void )
{
	if ( pev == NULL )		// Not fully connected yet
		return "";

	// return their team name
	return m_szTeamName;
}


//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	void	Spawn ( entvars_t *pevOwner );
	void	Think( void );

	virtual int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn ( entvars_t *pevOwner )
{
	pev->origin = pevOwner->origin + Vector ( 0 , 0 , 32 );
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);
	pev->frame = 0;

	pev->nextthink = gpGlobals->time + 0.1;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM);
}

void CSprayCan::Think( void )
{
	TraceResult	tr;	
	int playernum;
	int nFrames;
	CBasePlayer *pPlayer;
	
	pPlayer = (CBasePlayer *)GET_PRIVATE(pev->owner);

	if (pPlayer)
		nFrames = pPlayer->GetCustomDecalFrames();
	else
		nFrames = -1;

	playernum = ENTINDEX(pev->owner);
	
	// ALERT(at_console, "Spray by player %i, %i of %i\n", playernum, (int)(pev->frame + 1), nFrames);

	UTIL_MakeVectors(pev->angles);
	UTIL_TraceLine ( pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, & tr);

	// No customization present.
	if (nFrames == -1)
	{
		UTIL_DecalTrace( &tr, DECAL_LAMBDA6 );
		UTIL_Remove( this );
	}
	else
	{
		UTIL_PlayerDecalTrace( &tr, playernum, pev->frame, TRUE );
		// Just painted last custom frame.
		if ( pev->frame++ >= (nFrames - 1))
			UTIL_Remove( this );
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

class	CBloodSplat : public CBaseEntity
{
public:
	void	Spawn ( entvars_t *pevOwner );
	void	Spray ( void );
};

void CBloodSplat::Spawn ( entvars_t *pevOwner )
{
	pev->origin = pevOwner->origin + Vector ( 0 , 0 , 32 );
	pev->angles = pevOwner->v_angle;
	pev->owner = ENT(pevOwner);

	SetThink ( &CBloodSplat::Spray );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBloodSplat::Spray ( void )
{
	TraceResult	tr;	
	
//	if ( g_Language != LANGUAGE_GERMAN )
//	{
		UTIL_MakeVectors(pev->angles);
		UTIL_TraceLine ( pev->origin, pev->origin + gpGlobals->v_forward * 128, ignore_monsters, pev->owner, & tr);

		UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
//	}
	SetThink ( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

//==============================================



void CBasePlayer::GiveNamedItem( const char *pszName )
{
  if( !pszName || !strlen( pszName ) )
    return;
	edict_t	*pent;

	int istr = MAKE_STRING(pszName);

	pent = CREATE_NAMED_ENTITY(istr);
	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in GiveNamedItem!\n" );
		return;
	}
	VARS( pent )->origin = pev->origin;
	pent->v.spawnflags |= SF_NORESPAWN;

	pent->v.fuser3 = 1;

	DispatchSpawn( pent );
	DispatchTouch( pent, ENT( pev ) );
}



CBaseEntity *FindEntityForward( CBaseEntity *pMe )
{
	TraceResult tr;

	UTIL_MakeVectors(pMe->pev->v_angle);
	UTIL_TraceLine(pMe->pev->origin + pMe->pev->view_ofs,pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 8192,dont_ignore_monsters, pMe->edict(), &tr );
	if ( tr.flFraction != 1.0 && !FNullEnt( tr.pHit) )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
		return pHit;
	}
	return NULL;
}


BOOL CBasePlayer :: FlashlightIsOn( void )
{
	return FBitSet(pev->effects, EF_DIMLIGHT);
}


void CBasePlayer :: FlashlightTurnOn( void )
{
	if ( !g_pGameRules->FAllowFlashlight() )
	{
		return;
	}

  if( m_iTeam == 2 )
    return;

	if ( (pev->weapons & (1<<WEAPON_SUIT)) )
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1.0, ATTN_NORM, 0, PITCH_NORM );
		SetBits(pev->effects, EF_DIMLIGHT);
		MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
		WRITE_BYTE(1);
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();

		m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;

	}
}


void CBasePlayer :: FlashlightTurnOff( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1.0, ATTN_NORM, 0, PITCH_NORM );
    ClearBits(pev->effects, EF_DIMLIGHT);
	MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
	WRITE_BYTE(0);
	WRITE_BYTE(m_iFlashBattery);
	MESSAGE_END();

	m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;

}

/*
===============
ForceClientDllUpdate

When recording a demo, we need to have the server tell us the entire client state
so that the client side .dll can behave correctly.
Reset stuff so that the state is transmitted.
===============
*/
void CBasePlayer :: ForceClientDllUpdate( void )
{
	m_iClientHealth  = -1;
	m_iClientBattery = -1;
	m_iTrain |= TRAIN_NEW;  // Force new train message.
	m_fWeapon = FALSE;          // Force weapon send
	m_fKnownItem = FALSE;    // Force weaponinit messages.
	m_fInitHUD = TRUE;		// Force HUD gmsgResetHUD message
  clientteam = -1;

	// Now force all the necessary messages
	//  to be sent.
	UpdateClientData();
}

/*
============
ImpulseCommands
============
*/
extern float g_flWeaponCheat;

void CBasePlayer::ImpulseCommands( )
{
	TraceResult	tr;// UNDONE: kill me! This is temporary for PreAlpha CDs

	// Handle use events
	PlayerUse();
		
	int iImpulse = (int)pev->impulse;
	switch (iImpulse)
	{
	case 99:
		{

		int iOn;

		if (!gmsgLogo)
		{
			iOn = 1;
			gmsgLogo = REG_USER_MSG("Logo", 1);
		} 
		else 
		{
			iOn = 0;
		}
		
		ASSERT( gmsgLogo > 0 );
		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgLogo, NULL, pev );
			WRITE_BYTE(iOn);
		MESSAGE_END();

		if(!iOn)
			gmsgLogo = 0;
		break;
		}
	case 100:
        // temporary flashlight for level designers
        if ( FlashlightIsOn() )
		{
			FlashlightTurnOff();
		}
        else 
		{
			FlashlightTurnOn();
		}
		break;

	case	201:// paint decal
		
		if ( gpGlobals->time < m_flNextDecalTime )
		{
			// too early!
			break;
		}

		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine ( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), & tr);

		if ( tr.flFraction != 1.0 )
		{// line hit something, so paint a decal
			m_flNextDecalTime = gpGlobals->time + decalfrequency.value;
			CSprayCan *pCan = GetClassPtr((CSprayCan *)NULL);
			pCan->Spawn( pev );
		}

		break;

	default:
		// check all of the cheat impulse commands now
		CheatImpulseCommands( iImpulse );
		break;
	}
	
	pev->impulse = 0;
}

//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands( int iImpulse )
{
#if !defined( HLDEMO_BUILD )
	if ( g_flWeaponCheat == 0.0 )
	{
		return;
	}

	CBaseEntity *pEntity;
	TraceResult tr;

	switch ( iImpulse )
	{
	case 76:
		{
			if (!giPrecacheGrunt)
			{
				giPrecacheGrunt = 1;
				ALERT(at_console, "You must now restart to use Grunt-o-matic.\n");
			}
			else
			{
				UTIL_MakeVectors( Vector( 0, pev->v_angle.y, 0 ) );
				Create("monster_hgrunt", pev->origin + gpGlobals->v_forward * 128, pev->angles);
			}
			break;
		}


	case 101:
		gEvilImpulse101 = TRUE;
		//m_fPointsMax = 50;
    GiveExp( 100000 );
		gEvilImpulse101 = FALSE;
		break;

	case 102:
		// Gibbage!!!
		//CGib::SpawnRandomGibs( pev, 1, 1 );
		break;

	case 103:
		// What the hell are you doing?
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer();
			if ( pMonster )
				pMonster->ReportAIState();
		}
		break;

	case 104:
		// Dump all of the global state varaibles (and global entity names)
//		gGlobalState.DumpGlobals();
		break;

	case	105:// player makes no sound for monsters to hear.
		{
			if ( m_fNoPlayerSound )
			{
				ALERT ( at_console, "Player is audible\n" );
				m_fNoPlayerSound = FALSE;
			}
			else
			{
				ALERT ( at_console, "Player is silent\n" );
				m_fNoPlayerSound = TRUE;
			}
			break;
		}

	case 106:
		// Give me the classname and targetname of this entity.
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			ALERT ( at_console, "Classname: %s", STRING( pEntity->pev->classname ) );
			
			if ( !FStringNull ( pEntity->pev->targetname ) )
			{
				ALERT ( at_console, " - Targetname: %s\n", STRING( pEntity->pev->targetname ) );
			}
			else
			{
				ALERT ( at_console, " - TargetName: No Targetname\n" );
			}

			ALERT ( at_console, "Model: %s\n", STRING( pEntity->pev->model ) );
			if ( pEntity->pev->globalname )
				ALERT ( at_console, "Globalname: %s\n", STRING( pEntity->pev->globalname ) );
		}
		break;

	case 107:
		{
			TraceResult tr;

			edict_t		*pWorld = g_engfuncs.pfnPEntityOfEntIndex( 0 );

			Vector start = pev->origin + pev->view_ofs;
			Vector end = start + gpGlobals->v_forward * 1024;
			UTIL_TraceLine( start, end, ignore_monsters, edict(), &tr );
			if ( tr.pHit )
				pWorld = tr.pHit;
			const char *pTextureName = TRACE_TEXTURE( pWorld, start, end );
			if ( pTextureName )
				ALERT( at_console, "Texture: %s\n", pTextureName );
		}
		break;
	case	195:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_fly", pev->origin, pev->angles);
		}
		break;
	case	196:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_large", pev->origin, pev->angles);
		}
		break;
	case	197:// show shortest paths for entire level to nearest node
		{
			Create("node_viewer_human", pev->origin, pev->angles);
		}
		break;
	case	199:// show nearest node and all connections
		{
			ALERT ( at_console, "%d\n", WorldGraph.FindNearestNode ( pev->origin, bits_NODE_GROUP_REALM ) );
			WorldGraph.ShowNodeConnections ( WorldGraph.FindNearestNode ( pev->origin, bits_NODE_GROUP_REALM ) );
		}
		break;
	case	202:// Random blood splatter
		UTIL_MakeVectors(pev->v_angle);
		UTIL_TraceLine ( pev->origin + pev->view_ofs, pev->origin + pev->view_ofs + gpGlobals->v_forward * 128, ignore_monsters, ENT(pev), & tr);

		if ( tr.flFraction != 1.0 )
		{// line hit something, so paint a decal
			CBloodSplat *pBlood = GetClassPtr((CBloodSplat *)NULL);
			pBlood->Spawn( pev );
		}
		break;
	case	203:// remove creature.
		pEntity = FindEntityForward( this );
		if ( pEntity )
		{
			if ( pEntity->pev->takedamage )
				pEntity->SetThink(&CBaseEntity::SUB_Remove);
		}
		break;
	}
#endif	// HLDEMO_BUILD
}

//
// Add a weapon to the player (Item == Weapon == Selectable Object)
//
int CBasePlayer::AddPlayerItem( CBasePlayerItem *pItem )
{
	CBasePlayerItem *pInsert = NULL;

  if( m_iTeam != 1 && !FClassnameIs( pItem->pev, "weapon_hand" ) )
		return FALSE;

	
  switch( pItem->iItemSlot(this) )
  {
  case 1:
    pInsert = m_rgpPlayerItems[1];
    break;
  case 4:
    pInsert = m_rgpPlayerItems[4];
    break;
  case 5:
    pInsert = m_rgpPlayerItems[5];
    break;
  default:
  case -1:
    if( m_iSlots[0] == pItem->m_iId )
	    pInsert = m_rgpPlayerItems[2];
    else if( m_iSlots[1] == pItem->m_iId )
	    pInsert = m_rgpPlayerItems[3];
    break;
  }

	while (pInsert)
	{
		if (FClassnameIs( pInsert->pev, STRING( pItem->pev->classname) ))
		{
			if (pItem->AddDuplicate( pInsert ))
			{
				g_pGameRules->PlayerGotWeapon ( this, pItem );
				pItem->CheckRespawn();

				// ugly hack to update clip w/o an update clip message
				pInsert->UpdateItemInfo( );
				if (m_pActiveItem)
					m_pActiveItem->UpdateItemInfo( );

				pItem->Kill( );
        return TRUE;
			}
			else if (gEvilImpulse101)
			{
				// FIXME: remove anyway for deathmatch testing
				pItem->Kill( );
			}
			return FALSE;
		}
		pInsert = pInsert->m_pNext;
	}

  if( pItem->iItemSlot( NULL ) == -1 )
  {
    if( slot2f && slot3f )
    {
      if( FClassnameIs( pItem->pev, "bb_objective_item" ) )
      {
        ForceHelp( HELP_NOSPACE, this );
      }
		  return FALSE;
    }
    else if( !slot2f && m_iSlots[1] != pItem->m_iId )
    {
      slot2f = TRUE;
      m_iSlots[0] = pItem->m_iId;
     	ItemInfo& p = CBasePlayerItem::ItemInfoArray[pItem->m_iId];
      m_sAmmoSlot[1] = p.pszAmmo1;
	    m_sAmmo_aSlot[1] = p.pszAmmo2;
    }
    else if( !slot3f && m_iSlots[0] != pItem->m_iId  )
    {
      slot3f = TRUE;
      m_iSlots[1] = pItem->m_iId;
     	ItemInfo& p = CBasePlayerItem::ItemInfoArray[pItem->m_iId];
      m_sAmmoSlot[2] = p.pszAmmo1;
	    m_sAmmo_aSlot[2] = p.pszAmmo2;
   }
  }
  else if( pItem->iItemSlot( NULL ) == 1 )
  {
    if( slot1f )
      return FALSE;
    slot1f = TRUE;
    ItemInfo& p = CBasePlayerItem::ItemInfoArray[pItem->m_iId];
    m_sAmmoSlot[0] = p.pszAmmo1;
	  m_sAmmo_aSlot[0] = p.pszAmmo2;
  }

	if (pItem->AddToPlayer( this ))
	{
		g_pGameRules->PlayerGotWeapon ( this, pItem );
		pItem->CheckRespawn();

		pItem->m_pNext = m_rgpPlayerItems[pItem->iItemSlot(this)];
		m_rgpPlayerItems[pItem->iItemSlot(this)] = pItem;

		// should we switch to this item?
		if ( strcmp( STRING(pItem->pev->classname), "bb_objective_item" ) && g_pGameRules->FShouldSwitchWeapon( this, pItem ) )
		{
			SwitchWeapon( pItem );
		}

		return TRUE;
	}
	else if (gEvilImpulse101)
	{
		// FIXME: remove anyway for deathmatch testing
		pItem->Kill( );
	}
	return FALSE;
}



int CBasePlayer::RemovePlayerItem( CBasePlayerItem *pItem )
{
	if (m_pActiveItem == pItem)
	{
		ResetAutoaim( );
		pItem->Holster( );
		pItem->pev->nextthink = 0;// crowbar may be trying to swing again, etc.
		pItem->SetThink( NULL );
		m_pActiveItem = NULL;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	else if ( m_pLastItem == pItem )
		m_pLastItem = NULL;

	CBasePlayerItem *pPrev = m_rgpPlayerItems[pItem->iItemSlot(this)];

	if (pPrev == pItem)
	{
		m_rgpPlayerItems[pItem->iItemSlot(this)] = pItem->m_pNext;
		return TRUE;
	}
	else
	{
		while (pPrev && pPrev->m_pNext != pItem)
		{
			pPrev = pPrev->m_pNext;
		}
		if (pPrev)
		{
			pPrev->m_pNext = pItem->m_pNext;
			return TRUE;
		}
	}
	return FALSE;
}


//
// Returns the unique ID for the ammo, or -1 if error
//
int CBasePlayer :: GiveAmmo( int iCount, char *szName, int iMax )
{
	if ( !szName )
	{
		// no ammo.
		return -1;
	}

	if ( !g_pGameRules->CanHaveAmmo( this, szName, iMax ) )
	{
		// game rules say I can't have any more of this ammo type.
		return -1;
	}

	int i = 0;

	i = GetAmmoIndex( szName );

	if ( i < 0 || i >= MAX_AMMO_SLOTS )
		return -1;

	int iAdd = min( iCount, iMax - m_rgAmmo[i] );
	if ( iAdd < 1 )
		return i;

	m_rgAmmo[ i ] += iAdd;


	if ( gmsgAmmoPickup )  // make sure the ammo messages have been linked first
	{
		// Send the message that ammo has been picked up
		MESSAGE_BEGIN( MSG_ONE, gmsgAmmoPickup, NULL, pev );
			WRITE_BYTE( GetAmmoIndex(szName) );		// ammo ID
			WRITE_BYTE( iAdd );		// amount
		MESSAGE_END();
	}

	TabulateAmmo();

	return i;
}


/*
============
ItemPreFrame

Called every frame by the player PreThink
============
*/
void CBasePlayer::ItemPreFrame()
{
#if defined( CLIENT_WEAPONS )
    if ( m_flNextAttack > 0 )
#else
    if ( gpGlobals->time < m_flNextAttack )
#endif
	{
		return;
	}

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPreFrame( );
}


/*
============
ItemPostFrame

Called every frame by the player PostThink
============
*/
void CBasePlayer::ItemPostFrame()
{
	static int fInSelect = FALSE;

	// check if the player is using a tank
	if ( m_pTank != NULL )
		return;
	if( m_pActiveItem )
	{
		CBasePlayerWeapon *plw = (CBasePlayerWeapon*)m_pActiveItem;
		if( plw && ( plw->m_fInReload || plw->m_fInReload2 ) )
		{
			if( ( m_fNextCalm <= gpGlobals->time ) )
			{
				if( m_iShots > 0 )
					m_iShots -= (int)( ( gpGlobals->time - m_fNextCalm ) / PE_CROSS_CALM(this) + 1 );
				m_iShots = max( 0, m_iShots );
				m_fNextCalm = gpGlobals->time + PE_CROSS_CALM(this);
			}
		}
	}
	//ALERT( at_console, "%d\n", m_iShots );
#if defined( CLIENT_WEAPONS )
    if ( m_flNextAttack > 0 )
#else
    if ( gpGlobals->time < m_flNextAttack )
#endif
	{
		return;
	}

	ImpulseCommands();

	if (!m_pActiveItem)
		return;

	m_pActiveItem->ItemPostFrame( );
}

int CBasePlayer::AmmoInventory( int iAmmoIndex )
{
	if (iAmmoIndex == -1)
	{
		return -1;
	}

	return m_rgAmmo[ iAmmoIndex ];
}

int CBasePlayer::GetAmmoIndex(const char *psz)
{
	int i;

	if (!psz)
		return -1;

	unsigned long ahash = ElfHash( psz );

	for (i = 1; i < MAX_AMMO_SLOTS; i++)
	{
		if ( !CBasePlayerItem::AmmoInfoArray[i].pszName )
			continue;

		if( CBasePlayerItem::AmmoInfoArray[i].hash == ahash )
			return i;
		//if (stricmp( psz, CBasePlayerItem::AmmoInfoArray[i].pszName ) == 0)
		//	return i;
	}

	return -1;
}

// Called from UpdateClientData
// makes sure the client has all the necessary ammo info,  if values have changed
void CBasePlayer::SendAmmoUpdate(void)
{
	for (int i=0; i < MAX_AMMO_SLOTS;i++)
	{
		if (m_rgAmmo[i] != m_rgAmmoLast[i])
		{
			m_rgAmmoLast[i] = m_rgAmmo[i];

			ASSERT( m_rgAmmo[i] >= 0 );
			ASSERT( m_rgAmmo[i] < 255 );

			// send "Ammo" update message
			MESSAGE_BEGIN( MSG_ONE, gmsgAmmoX, NULL, pev );
				WRITE_BYTE( i );
				WRITE_BYTE( max( min( m_rgAmmo[i], 254 ), 0 ) );  // clamp the value to one byte
			MESSAGE_END();
		}
	}
}

/*
=========================================================
	UpdateClientData

resends any changed player HUD info to the client.
Called every frame by PlayerPreThink
Also called at start of demo recording and playback by
ForceClientDllUpdate to ensure the demo gets messages
reflecting all of the HUD state info.
=========================================================
*/
void CBasePlayer :: UpdateClientData( void )
{
  if( clientteam != m_iTeam )
  {
	  MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, edict() );
		  WRITE_BYTE( 101 );
		  WRITE_BYTE( m_iTeam );
	  MESSAGE_END();
    clientteam = m_iTeam;
  }

	if (m_fInitHUD)
	{
		// Spin - demo recording need this
		//UTIL_EdictScreenFade( edict(), Vector(0,0,0), 0.0, 0.0, 185, FFADE_IN | FFADE_MODULATE ); //Spinator
		m_fInitHUD = FALSE;
		gInitHUD = FALSE;

		MESSAGE_BEGIN( MSG_ONE, gmsgResetHUD, NULL, pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();

		if ( !m_fGameHUDInitialized )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgInitHUD, NULL, pev );
			MESSAGE_END();

			//Spin since it doesn't seem to work properly
			// here we'll do it in our pe_ gamerules
			//g_pGameRules->InitHUD( this );
			m_fGameHUDInitialized = TRUE;
			if ( g_pGameRules->IsMultiplayer() )
			{
				FireTargets( "game_playerjoin", this, this, USE_TOGGLE, 0 );
			}
		}

		FireTargets( "game_playerspawn", this, this, USE_TOGGLE, 0 );

		InitStatusBar();
	}

	if ( m_iHideHUD != m_iClientHideHUD )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgHideWeapon, NULL, pev );
			WRITE_BYTE( m_iHideHUD );
		MESSAGE_END();

		m_iClientHideHUD = m_iHideHUD;
	}

	if ( m_iFOV != m_iClientFOV )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
			WRITE_BYTE( m_iFOV );
		MESSAGE_END();

		// cache FOV change at end of function, so weapon updates can see that FOV has changed
	}

	// HACKHACK -- send the message to display the game title
	if (gDisplayTitle)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgShowGameTitle, NULL, pev );
		WRITE_BYTE( 0 );
		MESSAGE_END();
		gDisplayTitle = 0;
	}

	if (pev->health != m_iClientHealth)
	{
		int iHealth = (int)max( (float)pev->health, 0.0f );  // make sure that no negative health values are sent

		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
			WRITE_SHORT( iHealth );
		MESSAGE_END();

		m_iClientHealth = pev->health;
	}


	if (pev->armorvalue != m_iClientBattery)
	{
		m_iClientBattery = pev->armorvalue;

		ASSERT( gmsgBattery > 0 );
		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgBattery, NULL, pev );
			WRITE_SHORT( (int)pev->armorvalue);
		MESSAGE_END();
	}

	if (pev->dmg_take || pev->dmg_save || m_bitsHUDDamage != m_bitsDamageType)
	{
		// Comes from inside me if not set
		Vector damageOrigin = pev->origin;
		// send "damage" message
		// causes screen to flash, and pain compass to show direction of damage
		edict_t *other = pev->dmg_inflictor;
		if ( other )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(other);
			if ( pEntity )
				damageOrigin = pEntity->Center();
		}

		// only send down damage type that have hud art
		int visibleDamageBits = m_bitsDamageType & DMG_SHOWNHUD;

		MESSAGE_BEGIN( MSG_ONE, gmsgDamage, NULL, pev );
			WRITE_BYTE( pev->dmg_save );
			WRITE_BYTE( pev->dmg_take );
			WRITE_LONG( visibleDamageBits );
			WRITE_COORD( damageOrigin.x );
			WRITE_COORD( damageOrigin.y );
			WRITE_COORD( damageOrigin.z );
		MESSAGE_END();
	
		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;
		
		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}

	// Update Flashlight
	if ((m_flFlashLightTime) && (m_flFlashLightTime <= gpGlobals->time))
	{
		if (FlashlightIsOn())
		{
			if (m_iFlashBattery)
			{
				m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->time;
				//m_iFlashBattery--;
				
				if (!m_iFlashBattery)
					FlashlightTurnOff();
			}
		}
		else
		{
			if (m_iFlashBattery < 100)
			{
				m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->time;
				m_iFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}

		/*MESSAGE_BEGIN( MSG_ONE, gmsgFlashBattery, NULL, pev );
		WRITE_BYTE(m_iFlashBattery);
		MESSAGE_END();*/
	}


	if (m_iTrain & TRAIN_NEW)
	{
		ASSERT( gmsgTrain > 0 );
		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgTrain, NULL, pev );
			WRITE_BYTE(m_iTrain & 0xF);
		MESSAGE_END();

		m_iTrain &= ~TRAIN_NEW;
	}

	//
	// New Weapon?
	//
	if (!m_fKnownItem)
	{
		m_fKnownItem = TRUE;

	// WeaponInit Message
	// byte  = # of weapons
	//
	// for each weapon:
	// byte		name str length (not including null)
	// bytes... name
	// byte		Ammo Type
	// byte		Ammo2 Type
	// byte		bucket
	// byte		bucket pos
	// byte		flags	
	// ????		Icons
		
		// Send ALL the weapon info now
		int i;

		for (i = 0; i < MAX_WEAPONS; i++)
		{
			ItemInfo& II = CBasePlayerItem::ItemInfoArray[i];

			if ( !II.iId )
				continue;

			const char *pszName;
			if (!II.pszName)
				pszName = "Empty";
			else
				pszName = II.pszName;

			MESSAGE_BEGIN( MSG_ONE, gmsgWeaponList, NULL, pev );  
				WRITE_STRING(pszName);			// string	weapon name
				WRITE_BYTE(GetAmmoIndex(II.pszAmmo1));	// byte		Ammo Type
        if( II.iId == WEAPON_MINIGUN || II.iId == WEAPON_FLAME || II.iId == WEAPON_SPRAY )
          WRITE_BYTE( II.iMaxClip / 10 );				// byte     Max Ammo 1
        else
				  WRITE_BYTE(II.iMaxAmmo1);				// byte     Max Ammo 1
				WRITE_BYTE(GetAmmoIndex(II.pszAmmo2));	// byte		Ammo2 Type
        if( II.iId == WEAPON_MINIGUN )
				  WRITE_BYTE( II.iMaxClip % 10 );				// byte     Max Ammo 2
        else
				  WRITE_BYTE(II.iMaxAmmo2);				// byte     Max Ammo 2
  			if( II.iSlot == 2 || II.iSlot == 3 )
				{
					WRITE_BYTE(0);					// byte		bucket
					WRITE_BYTE(0);				// byte		bucket pos
				}
				else
				{
					WRITE_BYTE(II.iSlot);					// byte		bucket
					WRITE_BYTE(II.iPosition);				// byte		bucket pos
				}
				WRITE_BYTE(II.iId);						// byte		id (bit index into pev->weapons)
				WRITE_BYTE(II.iFlags);					// byte		Flags
			MESSAGE_END();
		}
	}


	SendAmmoUpdate();

	// Update all the items
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		if ( m_rgpPlayerItems[i] )  // each item updates it's successors
			m_rgpPlayerItems[i]->UpdateClientData( this );
	}

	// Cache and client weapon change
	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;

	// Update Status Bar
	if ( m_flNextSBarUpdateTime < gpGlobals->time )
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.2;
	}
}


//=========================================================
// FBecomeProne - Overridden for the player to set the proper
// physics flags when a barnacle grabs player.
//=========================================================
BOOL CBasePlayer :: FBecomeProne ( void )
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	return TRUE;
}

//=========================================================
// BarnacleVictimBitten - bad name for a function that is called
// by Barnacle victims when the barnacle pulls their head
// into its mouth. For the player, just die.
//=========================================================
void CBasePlayer :: BarnacleVictimBitten ( entvars_t *pevBarnacle )
{
	TakeDamage ( pevBarnacle, pevBarnacle, pev->health + pev->armorvalue, DMG_SLASH | DMG_ALWAYSGIB );
}

//=========================================================
// BarnacleVictimReleased - overridden for player who has
// physics flags concerns. 
//=========================================================
void CBasePlayer :: BarnacleVictimReleased ( void )
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
}


//=========================================================
// Illumination 
// return player light level plus virtual muzzle flash
//=========================================================
int CBasePlayer :: Illumination( void )
{
	int iIllum = CBaseEntity::Illumination( );

	iIllum += m_iWeaponFlash;
	if (iIllum > 255)
		return 255;
	return iIllum;
}


void CBasePlayer :: EnableControl(BOOL fControl)
{
	if (!fControl)
		pev->flags |= FL_FROZEN;
	else
		pev->flags &= ~FL_FROZEN;

}


#define DOT_1DEGREE   0.9998476951564
#define DOT_2DEGREE   0.9993908270191
#define DOT_3DEGREE   0.9986295347546
#define DOT_4DEGREE   0.9975640502598
#define DOT_5DEGREE   0.9961946980917
#define DOT_6DEGREE   0.9945218953683
#define DOT_7DEGREE   0.9925461516413
#define DOT_8DEGREE   0.9902680687416
#define DOT_9DEGREE   0.9876883405951
#define DOT_10DEGREE  0.9848077530122
#define DOT_15DEGREE  0.9659258262891
#define DOT_20DEGREE  0.9396926207859
#define DOT_25DEGREE  0.9063077870367

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector CBasePlayer :: GetAutoaimVector( float flDelta )
{
	//if (g_iSkillLevel == SKILL_HARD)
	//{
		UTIL_MakeVectors( pev->v_angle + pev->punchangle );
		return gpGlobals->v_forward;
	//}

	Vector vecSrc = GetGunPosition( );
	float flDist = 8192;

	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (1 || g_iSkillLevel == SKILL_MEDIUM)
	{
		m_vecAutoAim = Vector( 0, 0, 0 );
		// flDelta *= 0.5;
	}

	BOOL m_fOldTargeting = m_fOnTarget;
	Vector angles = AutoaimDeflection(vecSrc, flDist, flDelta );

	// update ontarget if changed
	if ( !g_pGameRules->AllowAutoTargetCrosshair() )
		m_fOnTarget = 0;
	else if (m_fOldTargeting != m_fOnTarget)
	{
		m_pActiveItem->UpdateItemInfo( );
	}

	if (angles.x > 180)
		angles.x -= 360;
	if (angles.x < -180)
		angles.x += 360;
	if (angles.y > 180)
		angles.y -= 360;
	if (angles.y < -180)
		angles.y += 360;

	if (angles.x > 25)
		angles.x = 25;
	if (angles.x < -25)
		angles.x = -25;
	if (angles.y > 12)
		angles.y = 12;
	if (angles.y < -12)
		angles.y = -12;


	// always use non-sticky autoaim
	// UNDONE: use sever variable to chose!
	if (0 || g_iSkillLevel == SKILL_EASY)
	{
		m_vecAutoAim = m_vecAutoAim * 0.67 + angles * 0.33;
	}
	else
	{
		m_vecAutoAim = angles * 0.9;
	}

	// m_vecAutoAim = m_vecAutoAim * 0.99;

	// Don't send across network if sv_aim is 0
	if ( g_psv_aim->value != 0 )
	{
		if ( m_vecAutoAim.x != m_lastx ||
			 m_vecAutoAim.y != m_lasty )
		{
			SET_CROSSHAIRANGLE( edict(), -m_vecAutoAim.x, m_vecAutoAim.y );
			
			m_lastx = m_vecAutoAim.x;
			m_lasty = m_vecAutoAim.y;
		}
	}

	// ALERT( at_console, "%f %f\n", angles.x, angles.y );

	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );
	return gpGlobals->v_forward;
}


Vector CBasePlayer :: AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta  )
{
	edict_t		*pEdict = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	CBaseEntity	*pEntity;
	float		bestdot;
	Vector		bestdir;
	edict_t		*bestent;
	TraceResult tr;

	if ( g_psv_aim->value == 0 )
	{
		m_fOnTarget = FALSE;
		return g_vecZero;
	}

	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );

	// try all possible entities
	bestdir = gpGlobals->v_forward;
	bestdot = flDelta; // +- 10 degrees
	bestent = NULL;

	m_fOnTarget = FALSE;

	UTIL_TraceLine( vecSrc, vecSrc + bestdir * flDist, dont_ignore_monsters, edict(), &tr );


	if ( tr.pHit && tr.pHit->v.takedamage != DAMAGE_NO)
	{
		// don't look through water
		if (!((pev->waterlevel != 3 && tr.pHit->v.waterlevel == 3) 
			|| (pev->waterlevel == 3 && tr.pHit->v.waterlevel == 0)))
		{
			if (tr.pHit->v.takedamage == DAMAGE_AIM)
				m_fOnTarget = TRUE;

			return m_vecAutoAim;
		}
	}

	for ( int i = 1; i < gpGlobals->maxEntities; i++, pEdict++ )
	{
		Vector center;
		Vector dir;
		float dot;

		if ( pEdict->free )	// Not in use
			continue;
		
		if (pEdict->v.takedamage != DAMAGE_AIM)
			continue;
		if (pEdict == edict())
			continue;
//		if (pev->team > 0 && pEdict->v.team == pev->team)
//			continue;	// don't aim at teammate
		if ( !g_pGameRules->ShouldAutoAim( this, pEdict ) )
			continue;

		pEntity = Instance( pEdict );
		if (pEntity == NULL)
			continue;

		if (!pEntity->IsAlive())
			continue;

		// don't look through water
		if ((pev->waterlevel != 3 && pEntity->pev->waterlevel == 3) 
			|| (pev->waterlevel == 3 && pEntity->pev->waterlevel == 0))
			continue;

		center = pEntity->BodyTarget( vecSrc );

		dir = (center - vecSrc).Normalize( );

		// make sure it's in front of the player
		if (DotProduct (dir, gpGlobals->v_forward ) < 0)
			continue;

		dot = fabs( DotProduct (dir, gpGlobals->v_right ) ) 
			+ fabs( DotProduct (dir, gpGlobals->v_up ) ) * 0.5;

		// tweek for distance
		dot *= 1.0 + 0.2 * ((center - vecSrc).Length() / flDist);

		if (dot > bestdot)
			continue;	// to far to turn

		UTIL_TraceLine( vecSrc, center, dont_ignore_monsters, edict(), &tr );
		if (tr.flFraction != 1.0 && tr.pHit != pEdict)
		{
			// ALERT( at_console, "hit %s, can't see %s\n", STRING( tr.pHit->v.classname ), STRING( pEdict->v.classname ) );
			continue;
		}

		// don't shoot at friends
		if (IRelationship( pEntity ) < 0)
		{
			if ( !pEntity->IsPlayer() && !g_pGameRules->IsDeathmatch())
				// ALERT( at_console, "friend\n");
				continue;
		}

		// can shoot at this one
		bestdot = dot;
		bestent = pEdict;
		bestdir = dir;
	}

	if (bestent)
	{
		bestdir = UTIL_VecToAngles (bestdir);
		bestdir.x = -bestdir.x;
		bestdir = bestdir - pev->v_angle - pev->punchangle;

		if (bestent->v.takedamage == DAMAGE_AIM)
			m_fOnTarget = TRUE;

		return bestdir;
	}

	return Vector( 0, 0, 0 );
}


void CBasePlayer :: ResetAutoaim( )
{
	if (m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0)
	{
		m_vecAutoAim = Vector( 0, 0, 0 );
		SET_CROSSHAIRANGLE( edict(), 0, 0 );
	}
	m_fOnTarget = FALSE;
}

/*
=============
SetCustomDecalFrames

  UNDONE:  Determine real frame limit, 8 is a placeholder.
  Note:  -1 means no custom frames present.
=============
*/
void CBasePlayer :: SetCustomDecalFrames( int nFrames )
{
	if (nFrames > 0 &&
		nFrames < 8)
		m_nCustomSprayFrames = nFrames;
	else
		m_nCustomSprayFrames = -1;
}

/*
=============
GetCustomDecalFrames

  Returns the # of custom frames this player's custom clan logo contains.
=============
*/
int CBasePlayer :: GetCustomDecalFrames( void )
{
	return m_nCustomSprayFrames;
}


//=========================================================
// DropPlayerItem - drop the named item, or if no name,
// the active item. 
//=========================================================
void CBasePlayer::DropPlayerItem ( char *pszItemName )
{
	/*if ( !g_pGameRules->IsMultiplayer() || (weaponstay.value > 0) )
	{
		// no dropping in single player.
		return;
	}*/

	if ( !strlen( pszItemName ) )
	{
		// if this string has no length, the client didn't type a name!
		// assume player wants to drop the active item.
		// make the string null to make future operations in this function easier
		pszItemName = NULL;
	} 
  if( !strcmp( pszItemName, "weapon_knife" ) || !strcmp( pszItemName, "weapon_hand" ) )
  {
    return;
  }

	CBasePlayerItem *pWeapon;
	int i;

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pWeapon = m_rgpPlayerItems[ i ];

		while ( pWeapon )
		{
			if ( pszItemName )
			{
				// try to match by name. 
				if ( !strcmp( pszItemName, STRING( pWeapon->pev->classname ) ) )
				{
					// match! 
					break;
				}
			}
			else
			{
				// trying to drop active item
				if ( pWeapon == m_pActiveItem )
				{
					// active item!
					break;
				}
			}

			//pWeapon = pWeapon->m_pNext; 
      pWeapon = NULL;
		}

		
		// if we land here with a valid pWeapon pointer, that's because we found the 
		// item we want to drop and hit a BREAK;  pWeapon is the item.
    CWeaponBox *pWeaponBox = NULL;
		if ( pWeapon )
		{
			if( pWeapon->m_iId != OBJECTIVE_ITEM )
				g_pGameRules->GetNextBestWeapon( this, pWeapon );

			UTIL_MakeVectors ( pev->angles ); 

			pev->weapons &= ~(1<<pWeapon->m_iId);// take item off hud

			pWeaponBox = (CWeaponBox *)CBaseEntity::Create( "weaponbox", pev->origin + gpGlobals->v_forward * 10, pev->angles, edict() );
			pWeaponBox->pev->angles.x = 0;
			pWeaponBox->pev->angles.z = 0;
			pWeaponBox->PackWeapon( pWeapon );
			//pWeaponBox->pev->velocity = gpGlobals->v_forward * 1 + gpGlobals->v_forward * 1;
      pWeaponBox->pev->velocity = gpGlobals->v_forward * 300 + gpGlobals->v_forward * 100;
			
			m_fWeapon = 0;

      switch( pWeapon->iItemSlot( this ) )
      {
      case 1:
        slot1f = FALSE;
        break;
      case 2:
        m_iSlots[0] = -1;
        //m_sAmmoSlot[1][0] = '\0';
        slot2f = FALSE;
        break;
      case 3:
        m_iSlots[1] = -1;
        //m_sAmmoSlot[2][0] = '\0';
        slot3f = FALSE;
      }

			switch ( pWeapon->m_iId )
			{
				/*case WEAPON_CASE:
					SET_MODEL( ENT( pWeaponBox->pev ), "models/case/w_case.mdl" );
					m_bIsSpecial = FALSE;
					DisableAll("pe_object_case");
					EnableAll(pWeaponBox->pev);
						MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
						WRITE_BYTE( ENTINDEX(edict( )) );
						WRITE_BYTE( 0 );
					MESSAGE_END( );
					return;*/
				case OBJECTIVE_ITEM:
					SET_MODEL( ENT( pWeaponBox->pev ), "models/case/w_case.mdl" );
					MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
						WRITE_BYTE( ENTINDEX(edict( )) );
						WRITE_BYTE( 0 );
					MESSAGE_END( );
					pev->body = 0;
					m_bIsSpecial = FALSE;
					for ( i = 1; i <= MAX_PLAYERS; i++ )
					{
						CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
					
						if (!pPlayer)
							continue;
						if( m_iTeam != pPlayer->m_iTeam )
							Disable(pev);
						if( this == pPlayer )
							Disable(pev);

					}
					EnableAll( pWeaponBox->pev );
          pWeaponBox->pev->fuser4 = 1;
  				//NotifyTeam( m_iTeam, NTT_HTOOLDROP, 3 );
					return;
			}
      SET_MODEL( ENT( pWeaponBox->pev ), pWeapon->mdl );

			// drop half of the ammo for this weapon.
			int	iAmmoIndex;

			iAmmoIndex = GetAmmoIndex ( pWeapon->pszAmmo1() ); // ???
			
      int total = -1;
			if ( iAmmoIndex != -1 )
			{
        total = 0;
				// this weapon weapon uses ammo, so pack an appropriate amount.
				//if ( pWeapon->iFlags() & ITEM_FLAG_EXHAUSTIBLE )
				//{
					// pack up all the ammo, this weapon is its own ammo type
        total += m_rgAmmo[iAmmoIndex];
				pWeaponBox->PackAmmo( MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[ iAmmoIndex ] );
				m_rgAmmo[ iAmmoIndex ] = 0; 

				/*}
				else
				{
					// pack half of the ammo
					pWeaponBox->PackAmmo( MAKE_STRING(pWeapon->pszAmmo1()), m_rgAmmo[ iAmmoIndex ] / 2 );
					m_rgAmmo[ iAmmoIndex ] /= 2; 
				}*/

			}
			iAmmoIndex = GetAmmoIndex ( pWeapon->pszAmmo2() ); // ???
			
			if ( iAmmoIndex != -1 )
			{
        if( total == -1 )
          total = 0;
        total += m_rgAmmo[iAmmoIndex];
				pWeaponBox->PackAmmo( MAKE_STRING(pWeapon->pszAmmo2()), m_rgAmmo[ iAmmoIndex ] );
				m_rgAmmo[ iAmmoIndex ] = 0;
			}
      if( total != -1 && !total )
        pWeaponBox->Kill( );
			return;// we're done, so stop searching with the FOR loop.
		}
	}
}

//=========================================================
// HasPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasPlayerItem( CBasePlayerItem *pCheckItem )
{
	CBasePlayerItem *pItem = m_rgpPlayerItems[pCheckItem->iItemSlot(this)];

	while (pItem)
	{
		if (FClassnameIs( pItem->pev, STRING( pCheckItem->pev->classname) ))
		{
			return TRUE;
		}
		pItem = pItem->m_pNext;
	}

	return FALSE;
}

//=========================================================
// HasNamedPlayerItem Does the player already have this item?
//=========================================================
BOOL CBasePlayer::HasNamedPlayerItem( const char *pszItemName )
{
	CBasePlayerItem *pItem;
	int i;
 
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pItem = m_rgpPlayerItems[ i ];
		
		while (pItem)
		{
			if ( !strcmp( pszItemName, STRING( pItem->pev->classname ) ) )
			{
				return TRUE;
			}
			pItem = NULL;//pItem->m_pNext;
		}
	}

	return FALSE;
}

//=========================================================
// 
//=========================================================
BOOL CBasePlayer :: SwitchWeapon( CBasePlayerItem *pWeapon ) 
{
	if ( !pWeapon->CanDeploy() )
	{
		return FALSE;
	}
	
	ResetAutoaim( );
	
	if (m_pActiveItem)
	{
		m_pActiveItem->Holster( );
	}

	m_pActiveItem = pWeapon;
	pWeapon->Deploy( );

	return TRUE;
}

//=========================================================
// Dead HEV suit prop
//=========================================================
class CDeadHEV : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static const char *m_szPoses[4];
};

const char *CDeadHEV::m_szPoses[] = { "deadback", "deadsitting", "deadstomach", "deadtable" };

void CDeadHEV::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_hevsuit_dead, CDeadHEV );

//=========================================================
// ********** DeadHEV SPAWN **********
//=========================================================
void CDeadHEV :: Spawn( void )
{
	PRECACHE_MODEL("models/player.mdl");
	SET_MODEL(ENT(pev), "models/player.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	pev->body			= 1;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead hevsuit with bad pose\n" );
		pev->sequence = 0;
		pev->effects = EF_BRIGHTFIELD;
	}

	// Corpses have less health
	pev->health			= 8;

	MonsterInitDead();
}


class CStripWeapons : public CPointEntity
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
};

LINK_ENTITY_TO_CLASS( player_weaponstrip, CStripWeapons );

void CStripWeapons :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = NULL;

	if ( pActivator && pActivator->IsPlayer() )
	{
		pPlayer = (CBasePlayer *)pActivator;
	}
	else if ( !g_pGameRules->IsDeathmatch() )
	{
		pPlayer = (CBasePlayer *)CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	}

	if ( pPlayer )
		pPlayer->RemoveAllItems( FALSE );
}

//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission:public CPointEntity
{
	void Spawn( void );
	void Think( void );
};

void CInfoIntermission::Spawn( void )
{
	UTIL_SetOrigin( pev, pev->origin );
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;

	pev->nextthink = gpGlobals->time + 2;// let targets spawn!

}

void CInfoIntermission::Think ( void )
{
	edict_t *pTarget;

	// find my target
	pTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->target) );

	if ( !FNullEnt(pTarget) )
	{
		pev->v_angle = UTIL_VecToAngles( (pTarget->v.origin - pev->origin).Normalize() );
		pev->v_angle.x = -pev->v_angle.x;
	}
}

LINK_ENTITY_TO_CLASS( info_intermission, CInfoIntermission );

void CBasePlayer::UpdateArmor( )
{
	pev->armorvalue	= (int)( (float)( (float)m_iArmor[AR_HEAD] / 4.0f ) +
								(float)( (float)m_iArmor[AR_CHEST] / 4.0f ) +
								(float)( (float)m_iArmor[AR_STOMACH] / 4.0f ) +
								(float)( ( (float)m_iArmor[AR_L_ARM] +
								(float)m_iArmor[AR_R_ARM] +
								(float)m_iArmor[AR_L_LEG] +
								(float)m_iArmor[AR_R_LEG] ) / 16 ) );
	m_iClientBattery = -1;
}

int CBasePlayer::FadeOnDMG( )
{
	if( pev->health > 0 && m_iTeam != 2 )
	  UTIL_EdictScreenFade( edict( ), Vector( 128, 0, 0 ), 0.7, 0.1, 96, FFADE_IN | FFADE_MODULATE );
	return 1;
}

void CBasePlayer::NVGActive(BOOL Active)
{
	//if (!m_fNVG)
	//	return;

    if( Active && !m_fNVGActive )
	{
        MESSAGE_BEGIN(MSG_ONE, gmsgNVGActive, NULL, pev);
			WRITE_BYTE( 1 );
		MESSAGE_END( );
		m_fNVGActive = TRUE;
		
	 
    } 
	else if( !Active && m_fNVGActive )
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgNVGActive, NULL, pev);
			WRITE_BYTE( 0 );
		MESSAGE_END( );
		m_fNVGActive = FALSE;
		NVGCreateInfrared( FALSE );
    }
}
void CBasePlayer::NVGCreateInfrared( BOOL fOn )  //THX to Prefect
{
    int r, g, b, life;

    // set up some variables
    if( fOn )
	{
        r = 255; g = 255; b = 255;
        life = 255;

        m_flInfraredUpdate = gpGlobals->time + 20.0;
    }
	else
	{
        r = g = b = 0;
        life = 0;

        m_flInfraredUpdate = gpGlobals->time;
    }
	//EMIT_SOUND(edict(), CHAN_WEAPON, "buttons/blip1.wav", 0.8, ATTN_NORM );
    // ... this is it!!
    MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, pev->origin, pev );
        WRITE_BYTE(TE_ELIGHT);
        WRITE_SHORT(ENTINDEX(edict()));    // entity to follow

        WRITE_COORD(pev->origin.x);    // original origin
        WRITE_COORD(pev->origin.y);
        WRITE_COORD(pev->origin.z);

        WRITE_COORD( 2048 );    // radius

        WRITE_BYTE( r );    // color
        WRITE_BYTE( g );
        WRITE_BYTE( b );
    
        WRITE_BYTE( life );    // life
        WRITE_COORD( 0 );    // decay
    MESSAGE_END();
}



void CBasePlayer::NVGUpdate ()
{
	m_flNVGUpdate = gpGlobals->time;
    if( m_fNVGActive && m_flInfraredUpdate < gpGlobals->time )
	{
        NVGCreateInfrared(TRUE);
	}
}

int RadColor( entvars_t* radar, CBasePlayer *pPlayer )
{
	if( !radar || !pPlayer )
		return 0;

	if( FClassnameIs( radar, "player" ) )
	{
		CBasePlayer *Plr = GetClassPtr( (CBasePlayer *)radar );
		if( Plr->m_bIsSpecial )
			return ( ( g_iPERules != 1 ) ? RADAR_BLINK : RADAR_OBJECT ); // vip oda hacker
		if( Plr->m_iTeam == pPlayer->m_iTeam )
				return RADAR_FRIEND;
		if( Plr->m_iTeam != pPlayer->m_iTeam )
				return RADAR_ENEMY;
	}
	if( FClassnameIs( radar, "pe_radar_mark" ) )
		return RADAR_MARK;
	if(
		FClassnameIs( radar, "weapon_case" ) ||
		FClassnameIs( radar, "bb_objective_item" ) ||
		FClassnameIs( radar, "bb_funk" ) ||
		FClassnameIs( radar, "weaponbox" ) ||
		FClassnameIs( radar, "bb_escape_radar" ) ||
		FClassnameIs( radar, "bb_mapmission" ) ||
		FClassnameIs( radar, "monster_zombie" )
		)
		return RADAR_BLINK;
  if( FClassnameIs( radar, "bb_escapeair" ) )
    return RADAR_TERMINAL;
  //if( FClassnameIs( radar, "bb_escape_radar" ) )
   // return RADAR_TERMINAL_HACKED;


	/*if( FClassnameIs( radar, "pe_terminal" ) )
	{			
		if( GetClassPtr( (cPEHackerTerminal *)radar)->m_bDone )
			return RADAR_TERMINAL_HACKED;
		else if( GetClassPtr( (cPEHackerTerminal *)radar)->m_bReached )
			return RADAR_BLINK;
		return RADAR_TERMINAL;
	}*/
	return RADAR_NODRAW;
}

void CBasePlayer::ResetRadar( )
{
	s_radarentry *cur;
	while( fradar )
	{
		cur = fradar;
		fradar = fradar->next;
		delete cur;
	}
	lradar = fradar = NULL;
	MESSAGE_BEGIN( MSG_ONE, gmsgRadar, NULL, pev );
    WRITE_SHORT( 0 );
		WRITE_BYTE( 3 );
	MESSAGE_END( );

}

void CBasePlayer::UpdateRadar( )
{	
	s_radarentry *cur = fradar;
	int idx;
	while( cur )
	{
		cur->override = true;
		if( cur->entindex > 0 && cur->entindex <= MAX_PLAYERS && FEntIsVisible( pev, cur->pev ) )
			cur->override = false;

		if( cur->override )
			cur->origin = cur->pev->origin;
		
		if( cur->disabled )
		{
			idx = cur->entindex;
			//ALERT( at_console, "Delete: %d, %s\n", cur->entindex, STRING(cur->pev->netname) );

			if( fradar == cur )
			{
				if( lradar == cur )
					lradar = NULL;
				fradar = cur->next;
				delete cur;
				cur = fradar;
			}
			else if( !cur->prev )
			{
				delete cur;
				cur = NULL;
			}
			else
			{
				s_radarentry *prev = cur->prev;
				prev->next = cur->next;
				if( cur->next )
					cur->next->prev = prev;
				else
					lradar = prev;
				delete cur;
				cur = prev->next;
			}
			MESSAGE_BEGIN( MSG_ONE, gmsgRadar, NULL, pev );
				WRITE_SHORT( idx );
				WRITE_BYTE( 0 );
			MESSAGE_END( );
			continue;
		}
		if( cur->override != cur->clientoverride || cur->origin != cur->clientorigin )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgRadar, NULL, pev );
				WRITE_SHORT( cur->entindex );
				WRITE_BYTE( 1 );
				WRITE_COORD( cur->origin.x );
				WRITE_COORD( cur->origin.y );
				WRITE_COORD( cur->origin.z );
				WRITE_BYTE( cur->override ? 1 : 0 );
			MESSAGE_END( );
			cur->clientorigin = cur->origin;
			cur->clientoverride = cur->override;
		}
		cur->color = RadColor( cur->pev, this );
		if( cur->color != cur->clientcolor )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgRadar, NULL, pev );
				WRITE_SHORT( cur->entindex );
				WRITE_BYTE( 2 );
				WRITE_BYTE( cur->color );
			MESSAGE_END( );
			cur->clientcolor = cur->color;
		}
		cur = cur->next;
	}
}

void CBasePlayer::Enable( entvars_t *entvars )
{	
	if( !entvars )
		return;
	if( entvars == pev )
		return;
	if( CBaseEntity::Instance(entvars)->IsPlayer( ) &&
		FBitSet( ((CBasePlayer*)CBaseEntity::Instance(entvars))->m_iAddons, AD_NANO_SKIN ) &&
		!( ((CBasePlayer*)CBaseEntity::Instance(entvars))->m_bIsSpecial ) )
		return;
	
	s_radarentry *nr = fradar;
	while( nr )
	{
		if( nr->pev == entvars )
			break;
		nr = nr->next;
	}
	if( nr )
	{
		nr->disabled = false;
		nr->color = RadColor( entvars, this );
		//ALERT( at_console, "Quick enable: %d, %s\n", nr->entindex, STRING(nr->pev->netname) );
		return;
	}

	nr = new s_radarentry;
	memset( nr, 0, sizeof(s_radarentry) );

	nr->entindex = ENTINDEX(ENT(entvars));
	nr->color = RadColor( entvars, this );
	nr->origin = entvars->origin;
	nr->pev = entvars;
	nr->disabled = false;
	nr->override = ( nr->color == RADAR_OBJECT ? true : false );
	//ALERT( at_console, "Enable: %d, %s\n", nr->entindex, STRING(nr->pev->netname) );

	if( !fradar )
	{
		fradar = lradar = nr;
	}
	else
	{
		lradar->next = nr;
		nr->prev = lradar;
		lradar = nr;
	}
}

void CBasePlayer::SetColor( entvars_t *entvars, int color )
{	
	if( !entvars )
		return;
	
	s_radarentry *nr = fradar;
	while( nr )
	{
		if( nr->pev == entvars )
			break;
		nr = nr->next;
	}
	if( !nr )
		return;
	nr->color = color;
}

void EnableAll( entvars_t *entvars )
{	
	if( !entvars )
		return;
  if( FBitSet( entvars->flags, FL_KILLME ) )
    return;

	for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
	
		if( !pPlayer )
			continue;

		if( entvars != pPlayer->pev )
			pPlayer->Enable( entvars );
	}
}

void SetColorAll( entvars_t *entvars, int color )
{	
	if( !entvars )
		return;

	for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
	
		if( !pPlayer )
			continue;

		if( entvars != pPlayer->pev )
			pPlayer->SetColor( entvars, color );
	}
}


void EnableTeam( entvars_t *entvars, int team )
{	
	if( !entvars )
		return;

	for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
	
		if( !pPlayer || pPlayer->m_iTeam != team )
			continue;

		if( entvars != pPlayer->pev )
			pPlayer->Enable( entvars );
	}
}

void SetColorTeam( entvars_t *entvars, int team, int color )
{	
	if( !entvars )
		return;

	for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
	
		if( !pPlayer || pPlayer->m_iTeam != team )
			continue;

		if( entvars != pPlayer->pev )
			pPlayer->SetColor( entvars, color );
	}
}

void CBasePlayer::Disable( entvars_t *entvars )
{	
	if( !entvars )
		return;

	s_radarentry *rem = fradar;
	while( rem )
	{
		if( rem->pev == entvars )
			break;
		rem = rem->next;
	}
	if( !rem )
		return;
	//ALERT( at_console, "Disable: %d, %s\n", rem->entindex, STRING(rem->pev->netname) );
	rem->disabled = true;
}

void DisableAll( entvars_t *entvars )
{	
	if( !entvars )
		return;

	for( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
	
		if( !pPlayer )
			continue;

		if( entvars != pPlayer->pev )
			pPlayer->Disable( entvars );
	}
}

void DisableAll( const char* name )
{	
	if( !name || !strlen( name ) )
		return;
	
	CBaseEntity* pEnt = UTIL_FindEntityByClassname( NULL, name );
	while( pEnt != NULL)
	{
		DisableAll( pEnt->pev );
		pEnt = UTIL_FindEntityByClassname( pEnt, name );
	}
}

void EnableAll( const char* name, char *target )
{	
	if( !name || !strlen( name ) )
		return;

  CBaseEntity* pEnt = UTIL_FindEntityByClassname( NULL, name );
	while( pEnt != NULL)
	{
    if( !target || !strcmp( target, STRING( pEnt->pev->target ) ) )
		  EnableAll( pEnt->pev );
		pEnt = UTIL_FindEntityByClassname( pEnt, name );
	}
}

void SetColorAll( const char* name, int color )
{	
	if( !name || !strlen( name ) )
		return;
	
	CBaseEntity* pEnt = UTIL_FindEntityByClassname( NULL, name );
	while( pEnt != NULL)
	{
		SetColorAll( pEnt->pev, color );
		pEnt = UTIL_FindEntityByClassname( pEnt, name );
	}
}

void CBasePlayer::AddDmgDone( int index, float amount, int hitgroup )
{
	int i = 0;
	for( i = 0; i < 32; i++ )
	{
		if( ( dmgdone[i].index == index ) || ( dmgdone[i].index == 0 ) )
			break;
	}

	if( !VARS(INDEXENT(index)) || ( VARS(INDEXENT(index)) == pev ) )
		return;

	if( strlen(STRING( VARS(INDEXENT(index))->netname )) > 0 )
		strcpy( dmgdone[i].name, STRING( VARS(INDEXENT(index))->netname ) );
	else
		return;//strcpy( dmgdone[i].name, STRING( VARS(INDEXENT(index))->classname ) );
	dmgdone[i].index = index;
	dmgdone[i].damage += amount;
	dmgdone[i].hits++;
	switch( hitgroup )
	{
	case 1:
		dmgdone[i].hitgroup[0]++;
		break;
	case 2:
	case 3:
		dmgdone[i].hitgroup[1]++;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		dmgdone[i].hitgroup[2]++;
		break;
	default:
		dmgdone[i].hitgroup[3]++;
		break;
	}
}


void CBasePlayer::AddDmgReceived( int from_index, float amount, int hitgroup )
{
	int i = 0;
	
	for( i = 0; i < 32; i++ )
	{
		if( ( dmgreceived[i].index == from_index ) || ( dmgreceived[i].index == 0 ) )
			break;
	}
	if( !VARS(INDEXENT(from_index)) || ( VARS(INDEXENT(from_index)) == pev ) )
		return;

	if( strlen(STRING( VARS(INDEXENT(from_index))->netname )) > 0 )
		strcpy( dmgreceived[i].name, STRING( VARS(INDEXENT(from_index))->netname ) );
	else
		return;//strcpy( dmgreceived[i].name, STRING( VARS(INDEXENT(from_index))->classname ) );
	dmgreceived[i].index = from_index;
	dmgreceived[i].damage += amount;
	dmgreceived[i].hits++;

	switch( hitgroup )
	{
	case 1:
		dmgreceived[i].hitgroup[0]++;
		break;
	case 2:
	case 3:
		dmgreceived[i].hitgroup[1]++;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		dmgreceived[i].hitgroup[2]++;
		break;
	default:
		dmgreceived[i].hitgroup[3]++;
		break;
	}
}

#define POINT_BONUS_KILL					0.50f
#define POINT_BONUS_HACK					1.00f
#define POINT_BONUS_SAFE					2.00f
#define POINT_BONUS_SPAWN					0.50f
#define POINT_BONUS_SPAWN_DELAYED			0.25f
#define POINT_BONUS_SPAWN_RESTARTROUND		0.35f
#define POINT_PENALTY_OVER_LIMIT1		  (-0.50f)
#define POINT_PENALTY_OVER_LIMIT2		  (-1.00f)
#define POINT_PENALTY_DEATH				  (-0.50f)
#define POINT_PENALTY_TEAMCHANGE		  (-0.25f)

void CBasePlayer::BonusPoint( int task )
{
}

/*int level[15] =
{
  0,
	1000,
	3000,
	7000,
	15000,
  25000,
  45000,
  60000,
 1000000,
 1700000,
};*/
/*int levelExp[32];/* =
{
  0,
  1000,
	2000,
	4000,
	6000,
	8000,
  10000,
  12000,
  14000,
  16000,
  18000,
  20000,
  22000,
  24000
};*/
#define EXP_FOR_LEVEL( a ) ( pow( a, exppotence.value ) * expperlevel.value )
#define LEVEL_FOR_EXP( a ) ( pow( a / expperlevel.value, 1 / exppotence.value ) )
/*#define EXP_FOR_LEVEL( a ) ( a * expperlevel.value )
#define LEVEL_FOR_EXP( a ) ( (float)a / expperlevel.value )*/

/*int getLvl( float xp )
{
  /*if( !levelExp[1] )
  {
    for( int i = 0; i < 23; i++ )
      levelExp[i] = 2500 * i;
    levelExp[i] = 0;
  }*/
	/*for( int i = 0; i < ( maxlevel.value - 1 ); i++ )
		if( xp >= EXP_FOR_LEVEL( i ) && xp < EXP_FOR_LEVEL( i + 1 ) )
			return i;
		return maxlevel.value;
}*/

void CBasePlayer::GiveExp( float value, bool load )
{
	if (!load)
	{
		if (g_fGameOver)
			return;
		if (m_pActiveItem && FClassnameIs(m_pActiveItem->pev, "weapon_flame"))
			value /= 15;
		if (m_iTeam == 2)
			value *= 0.5;
	}
	int lvl = (int)LEVEL_FOR_EXP( exp );
  if( !lvl )
    m_fPointsMax = 0;
	exp += value;
	int lvl2 = (int)LEVEL_FOR_EXP( exp );
  bool lvlup = false;
  if( lvl != lvl2 )
  {
		pev->health += PLR_HP(this) / 2;
		pev->health = min( pev->health, PLR_HP(this) );
    lvlup = true;
    m_fPointsMax += lvl2 - lvl;
    level = lvl2;
  }
  //float curexp = EXP_FOR_LEVEL( lvl2 ), nextexp = EXP_FOR_LEVEL( lvl2 + 1 );
  //float percent = ( 100 * ( exp - curexp ) / ( nextexp - curexp ) );
	MESSAGE_BEGIN( MSG_ONE, gmsgUpdPoints, NULL, pev );
		WRITE_COORD( ( LEVEL_FOR_EXP( exp ) - lvl2 ) * 100 );//percent );
    WRITE_BYTE( lvlup ? lvl2 : 0 );
    WRITE_BYTE( (int)m_fPointsMax );
	MESSAGE_END( );
}

#define MAX_HEALTH 280
#define MAX_SPEED 175
#define MAX_DMG 250

void CBasePlayer::UpdateStats( int type )
{
  if( m_fPointsMax - 1 < 0 )
    return;

  switch( type )
  {
  case 0:
  {
    float tmp = pev->health / PLR_HP(this);
    if( ( PLR_HP(this) + HEALTH_GIVE ) > MAX_HEALTH )
      return;
    hppnts++;
    pev->health = tmp * PLR_HP(this);
    pev->max_health = PLR_HP(this);
    ForceHelp( HELP_HPPOINT, this );
    break;
  }
  case 1:
    if( ( PLR_SPEED(this) + SPEED_GIVE ) > MAX_SPEED )
      return;
    speedpnts++;
   	g_engfuncs.pfnSetClientMaxspeed( ENT(pev), SPEED_FORMULA );
    MESSAGE_BEGIN( MSG_ONE, gmsgPlayMusic, NULL, edict( ) );
      WRITE_BYTE( 3 );
      WRITE_COORD( SPEED_FORMULA );
    MESSAGE_END( );
    ForceHelp( HELP_SPEEDPOINT, this );
    break;
  case 2:
     if( ( PLR_DMG(this) + DMG_GIVE ) > MAX_DMG )
      return;
    dmgpnts++;
    ForceHelp( HELP_SKILLPOINT, this );
    break;
  }

  m_fPointsMax--;

	MESSAGE_BEGIN( MSG_ONE, gmsgStats, NULL, pev );
		WRITE_BYTE( hppnts );
    WRITE_BYTE( speedpnts );
    WRITE_BYTE( dmgpnts );
    WRITE_BYTE( (int)m_fPointsMax );
	MESSAGE_END( );
}

CBasePlayer::CBasePlayer()
	: CBaseMonster()
{
	m_iTeam = 0;
	escaped = false;
	m_sInfo.team = 0;
	m_iSpawnDelayed = 0;
	m_fInitHUD = 0;
	m_fGameHUDInitialized = 0;
	m_iHUDInited = FALSE;
	statsRatio = 1;
	speedpnts = 0;
	hppnts = 0;
	dmgpnts = 0;
	level = 0;
	clientlevel = -1;
	slot1f = 0;
	slot2f = 0;
	slot3f = 0;
	m_fTransformTime = 0;
	m_bTransform = 0;
	flame.clear();
	zombieKills = 0;
	rehbutton = false;
	clientFrags = 0;
	clientDeaths = 0;
	m_fPointsMax = 0;
	exp = 0;
	expLoaded = false;
}