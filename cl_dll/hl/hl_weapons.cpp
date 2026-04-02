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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

#include "usercmd.h"
#include "entity_state.h"
#include "demo_api.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"

#include "../hud_iface.h"
#include "../com_weapons.h"
#include "../demo.h"
#include "../Exports.h"

#undef PE_CROSS_CALM
#undef PE_CROSS_CALM_SLOW

#define PE_CROSS_CALM (0.004/((85+(dmg*3.75))/100.0f))
#define PE_CROSS_CALM_SLOW (PE_CROSS_CALM*2)
#define PE_CROSS_SHOT_LIMIT 30

#define SERV_CALL
#undef DMG_TIMEBASED
#include "../cl_dll/hud.h"
#undef SERV_CALL
#include "../cl_dll/ammohistory.h"
extern WeaponsResource gWR;
extern int dmg;

#define CB_TORSO		(1<<0)
#define CB_ARMS			(1<<1)
#define CB_LEGS			(1<<2)
#define CB_HEAD			(1<<3)

int g_bDbg = 0;
int g_iShots = 0;
float g_fPunch = 0.01f;
float g_fNextCalm = 0.0f;

extern globalvars_t *gpGlobals;
extern int g_iUser1;

// Pool of client side entities/entvars_t
static entvars_t	ev[ 32 ];
static int			num_ents = 0;

// The entity we'll use to represent the local client
static CBasePlayer	player;

// Local version of game .dll global variables ( time, etc. )
static globalvars_t	Globals; 

static CBasePlayerWeapon *g_pWpns[ 32 ];

float g_flApplyVel = 0.0;
vec3_t previousorigin;

void InitGlobals( )
{
  gpGlobals = &Globals;
}

//----
// PE
//----
CDeagle			g_Deagle;
//CSeburo			g_Seburo;
CBenelli		g_Benelli;
CBeretta		g_Beretta;
CM16			g_M16;
CGlock			g_Glock;
CKnife			g_Knife;
CMinigun		g_Minigun;
CMicrouzi		g_Microuzi;
CMicrouzi_a		g_Microuzi_a;
CMP5			g_Mp5;
CHandGrenade	g_HandGren;
CUsp			g_Usp;
CStoner			g_Stoner;
CSig550			g_Sig550;
//CP226			g_P226;
CP225			g_P225;
CBeretta_a		g_Beretta_a;
CGlock_auto_a	g_Glock_auto_a;
//CGlock_auto		g_Glock_auto;
//CMP5sd			g_MP5sd;
CTavor			g_Tavor;
CSawed			g_Sawed;
CAK47			g_AK47;
CFlame			g_Flame;
//CUspMP			g_UspMP;
CHand			g_Hand;
//CSpray		g_Spray;


/*
======================
AlertMessage

Print debug messages to console
======================
*/
void AlertMessage( ALERT_TYPE atype, char *szFmt, ... )
{
	va_list		argptr;
	static char	string[1024];
	
	va_start (argptr, szFmt);
	vsnprintf (string, sizeof(string), szFmt, argptr);
	va_end (argptr);

	gEngfuncs.Con_Printf( "cl:  " );
	gEngfuncs.Con_Printf( "%s", string );
}

//Returns if it's multiplayer.
//Mostly used by the client side weapons.
bool bIsMultiplayer ( void )
{
	return gEngfuncs.GetMaxClients() == 1 ? 0 : 1;
}
//Just loads a v_ model.
void LoadVModel ( char *szViewModel, CBasePlayer *m_pPlayer )
{
	gEngfuncs.CL_LoadModel( szViewModel, &m_pPlayer->pev->viewmodel );
}

/*
=====================
HUD_PrepEntity

Links the raw entity to an entvars_s holder.  If a player is passed in as the owner, then
we set up the m_pPlayer field.
=====================
*/
void HUD_PrepEntity( CBaseEntity *pEntity, CBasePlayer *pWeaponOwner )
{
	memset( &ev[ num_ents ], 0, sizeof( entvars_t ) );
	pEntity->pev = &ev[ num_ents++ ];

	pEntity->Precache();
	pEntity->Spawn();

	if ( pWeaponOwner )
	{
		ItemInfo info;
		
		((CBasePlayerWeapon *)pEntity)->m_pPlayer = pWeaponOwner;
		
		((CBasePlayerWeapon *)pEntity)->GetItemInfo( &info );

		g_pWpns[ info.iId ] = (CBasePlayerWeapon *)pEntity;
	}
}

/*
=====================
CBaseEntity :: Killed

If weapons code "kills" an entity, just set its effects to EF_NODRAW
=====================
*/
void CBaseEntity :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->effects |= EF_NODRAW;
}

/*
=====================
CBasePlayerWeapon :: DefaultReload
=====================
*/
BOOL CBasePlayerWeapon :: DefaultReload( int iClipSize, int iAnim, float fDelay, int body )
{

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return FALSE;

	int j = min(iClipSize - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);	

	if (j <= 0)
		return FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	//!!UNDONE -- reload sound goes here !!!
	SendWeaponAnim( iAnim, UseDecrement(), body );

	m_fInReload = TRUE;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	return TRUE;
}

BOOL CBasePlayerWeapon :: DefaultReload2( int iClipSize, int iAnim, float fDelay, int body )
{

	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] <= 0)
		return FALSE;

	int j = min(iClipSize - m_iClip2, m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]);	

	if (j <= 0)
		return FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + fDelay;

	//!!UNDONE -- reload sound goes here !!!
	SendWeaponAnim( iAnim, UseDecrement(), body );

	m_fInReload2 = TRUE;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
	return TRUE;
}

/*
=====================
CBasePlayerWeapon :: CanDeploy
=====================
*/
BOOL CBasePlayerWeapon :: CanDeploy( void ) 
{
	BOOL bHasAmmo = 0;

	if ( !pszAmmo1() )
	{
		// this weapon doesn't use ammo, can always deploy.
		return TRUE;
	}

	if ( pszAmmo1() )
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0);
	}
	if ( pszAmmo2() )
	{
		bHasAmmo |= (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] != 0);
	}
	if( ( m_iClip > 0 ) || ( m_iClip2 > 0 ) )
	{
		bHasAmmo |= 1;
	}
	if (!bHasAmmo)
	{
		return FALSE;
	}

	return TRUE;
}

/*
=====================
CBasePlayerWeapon :: DefaultDeploy

=====================
*/
BOOL CBasePlayerWeapon :: DefaultDeploy( char *szViewModel, char *szWeaponModel, int iAnim, char *szAnimExt, int skiplocal, int	body )
{
	if ( !CanDeploy() )
		return FALSE;

	gEngfuncs.CL_LoadModel( szViewModel, &m_pPlayer->pev->viewmodel );
	
	SendWeaponAnim( iAnim, skiplocal, body );

	m_pPlayer->m_flNextAttack = 0.5;
	m_flTimeWeaponIdle = 1.0;
	return TRUE;
}

/*
=====================
CBasePlayerWeapon :: PlayEmptySound

=====================
*/
BOOL CBasePlayerWeapon :: PlayEmptySound( void )
{
	if (m_iPlayEmptySound)
	{
		HUD_PlaySound( "weapons/357_cock1.wav", 0.8 );
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}

/*
=====================
CBasePlayerWeapon :: ResetEmptySound

=====================
*/
void CBasePlayerWeapon :: ResetEmptySound( void )
{
	m_iPlayEmptySound = 1;
}

/*
=====================
CBasePlayerWeapon::Holster

Put away weapon
=====================
*/
void CBasePlayerWeapon::Holster( int skiplocal /* = 0 */ )
{ 
	m_fInReload = FALSE; // cancel any reload in progress.
	m_pPlayer->pev->viewmodel = 0; 
}

/*
=====================
CBasePlayerWeapon::SendWeaponAnim

Animate weapon model
=====================
*/
void CBasePlayerWeapon::SendWeaponAnim( int iAnim, int skiplocal, int body )
{
	m_pPlayer->pev->weaponanim = iAnim;
	
	HUD_SendWeaponAnim( iAnim, body, 0 );
}

/*
=====================
CBaseEntity::FireBulletsPlayer

Only produces random numbers to match the server ones.
=====================
*/
extern int g_iShots;
extern float g_fRecoilRatio;
Vector CBaseEntity::FireBulletsPlayer ( ULONG cShots, Vector vecSrc, Vector vecDirShooting, Vector vecSpread, float flDistance, int iBulletType, int iTracerFreq, int iDamage, entvars_t *pevAttacker, int shared_rand )
{
	float x, y, z;

	vecSpread = vecSpread * g_fRecoilRatio;

	for ( ULONG iShot = 1; iShot <= cShots; iShot++ )
	{
		/*if ( pevAttacker == NULL )
		{
			// get circular gaussian spread
			do {
					x = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
					y = RANDOM_FLOAT(-0.5, 0.5) + RANDOM_FLOAT(-0.5, 0.5);
					z = x*x+y*y;
			} while (z > 1);
		}
		else
		{*/
			//Use player's random seed.
			// get circular gaussian spread
			/*if( 0 )
			{
				vecSpread.x *= ( (((float)g_iShots)/140.0f)*(((float)g_iShots)/140.0f) );
				if( FBitSet( gHUD.m_ShowMenuPE.m_sInfo.slot[6], CB_HEAD ) )
				{
					vecSpread.x *= 0.66;
					vecSpread.y *= 0.66;
				}
				if( FBitSet( gHUD.m_ShowMenuPE.m_sInfo.slot[6], CB_ARMS ) )
				{
					vecSpread.x *= 0.58;
					vecSpread.y *= 0.58;
				}
				if( vecSpread.x < (vecSpread.y/4.0f) )
					vecSpread.x = vecSpread.y / 4.0f;
			}*/


			g_iShots += 20;
			x = UTIL_SharedRandomFloat( shared_rand + g_iShots, -(10.0/160.0*g_iShots), (10.0/160.0*g_iShots) );
			y = UTIL_SharedRandomFloat( shared_rand + ( 2 + g_iShots ), (5.0/160.0*g_iShots), (20.0/160.0*g_iShots) );
			g_iShots -= 20;
//			x = UTIL_SharedRandomFloat( shared_rand + g_iShots, -(1/160*g_iShots), (1/160*g_iShots) ) + UTIL_SharedRandomFloat( shared_rand + ( 1 + g_iShots ) , -(1/160*g_iShots), (1/160*g_iShots) );
//			y = UTIL_SharedRandomFloat( shared_rand + ( 2 + g_iShots ), -(0.5/160*g_iShots), (1/160*g_iShots) ) + UTIL_SharedRandomFloat( shared_rand + ( 3 + g_iShots ), -(0.5/160*g_iShots), (1/160*g_iShots) );

		//	x = UTIL_SharedRandomFloat( shared_rand + iShot, -0.5, 0.5 ) + UTIL_SharedRandomFloat( shared_rand + ( 1 + iShot ) , -0.5, 0.5 );
		//	y = UTIL_SharedRandomFloat( shared_rand + ( 2 + iShot ), -0.5, 0.5 ) + UTIL_SharedRandomFloat( shared_rand + ( 3 + iShot ), -0.5, 0.5 );
			z = x * x + y * y;
		//}
			
	}

    return Vector ( x/10.0 * vecSpread.x, y/10.0 * vecSpread.y, 0.0 );
}

/*
=====================
CBasePlayerWeapon::ItemPostFrame

Handles weapon firing, reloading, etc.
=====================
*/
void CBasePlayerWeapon::ItemPostFrame( void )
{
  if( !gpGlobals )
    InitGlobals( );
	if ((m_fInReload) && (m_pPlayer->m_flNextAttack <= 0.0))
	{
#if 0 // FIXME, need ammo on client to make this work right
		// complete the reload. 
		int j = min( iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);	

		// Add them to the clip
		m_iClip += j;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;
#else	
		m_iClip += 10;
#endif
		m_fInReload = FALSE;
		//g_iShots = 0;
	}
	if ((m_fInReload2) && (m_pPlayer->m_flNextAttack <= 0.0))
	{
#if 0 // FIXME, need ammo on client to make this work right
		// complete the reload. 
		int j = min( iMaxClip2() - m_iClip2, m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]);	

		// Add them to the clip
		m_iClip2 += j;
		m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] -= j;
#else	
		m_iClip2 += 10;
#endif
		m_fInReload2 = FALSE;
		//g_iShots = 0;
	}

	
	if( ( m_pPlayer->pev->button & IN_ATTACK ) && !( m_pPlayer->pev->button & IN_ATTACK2 ) )
		lastside = 0;
	else if( ( m_pPlayer->pev->button & IN_ATTACK2 ) && !( m_pPlayer->pev->button & IN_ATTACK ) )
		lastside = 1;
	else if( ( m_pPlayer->pev->button & IN_ATTACK ) && ( m_pPlayer->pev->button & IN_ATTACK2 ) 
		&& lastside == 1 && m_iClip )
		lastside = 1;
	else if( ( m_pPlayer->pev->button & IN_ATTACK ) && ( m_pPlayer->pev->button & IN_ATTACK2 ) 
		&& lastside == 0 && m_iClip2 )
		lastside = 0;

	
	if( ( lastside == 1 ) && ( m_pPlayer->pev->button & IN_ATTACK2 )
		&& ( m_flNextSecondaryAttack <= 0.0 ) )
	{
		if( ( m_iClip2 == 0 && pszAmmo2( ) ) || ( iMaxClip2( ) == -1 && !m_pPlayer->m_rgAmmo[SecondaryAmmoIndex( )] ) )
		{
			m_fFireOnEmpty = TRUE;
		}

		SecondaryAttack( );
		g_fNextCalm = gpGlobals->time + PE_CROSS_CALM;
		lastside = 0;
		//m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	if( ( lastside == 0 ) && ( m_pPlayer->pev->button & IN_ATTACK )
		&& ( m_flNextPrimaryAttack <= 0.0 ) )
	{
		if( ( m_iClip == 0 && pszAmmo1( ) ) || ( iMaxClip( ) == -1 && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex( )] ) )
		{
			m_fFireOnEmpty = TRUE;
		}

		PrimaryAttack( );
		g_fNextCalm = gpGlobals->time + PE_CROSS_CALM;
		lastside = 1;
	}
	else if ( !m_fInReload2 && m_pPlayer->pev->button & IN_RELOAD && iMaxClip() != WEAPON_NOCLIP && !m_fInReload ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
		//g_iShots = 0;
	}
	if ( !m_fInReload && m_pPlayer->pev->button & IN_RELOAD && iMaxClip2() != WEAPON_NOCLIP && !m_fInReload2 ) 
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload2();
		//g_iShots = 0;
	}
	else if( ( m_iClip == 0 ) && ( m_pPlayer->pev->button & (IN_ATTACK|IN_ATTACK2) ) )
	{
		// no fire buttons down
		if( ( player.pev == m_pPlayer->pev ) && ( g_fNextCalm <= gpGlobals->time ) )
		{
			if( g_iShots > 0 )
				g_iShots -= (int)( ( gpGlobals->time - g_fNextCalm ) / PE_CROSS_CALM + 1 );
			g_iShots = max( 0, g_iShots );
			g_fNextCalm = gpGlobals->time + ( g_iShots > PE_CROSS_SHOT_LIMIT ? PE_CROSS_CALM : PE_CROSS_CALM_SLOW );
		}
	}
	WeaponIdle( );
	if ( !(m_pPlayer->pev->button & (IN_ATTACK|IN_ATTACK2) ) )
	{
		// no fire buttons down
		if( ( player.pev == m_pPlayer->pev ) && ( g_fNextCalm <= gpGlobals->time ) )
		{
			if( g_iShots > 0 )
				g_iShots -= (int)( ( gpGlobals->time - g_fNextCalm ) / PE_CROSS_CALM + 1 );
      float bla = dmg;
			g_iShots = max( 0, g_iShots );
			g_fNextCalm = gpGlobals->time + ( g_iShots > PE_CROSS_SHOT_LIMIT ? PE_CROSS_CALM : PE_CROSS_CALM_SLOW );
		}

		m_fFireOnEmpty = FALSE;

		// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
		if ( !m_fInReload2 && m_iClip == 0 && ( SecondaryAmmoIndex() != -1 ? m_iClip2 == 0 : 1 ) && !(iFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack <= 0.0 )
		{
			Reload();
			//g_iShots = 0;
			//return;
		}
		if ( m_fInSpecialReload && !(iFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < 0.0 )
		{
			Reload();
			//g_iShots = 0;
			//return;
		}

		if ( !m_fInReload && m_iClip2 == 0 && ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] ? m_iClip == 0 : 1 ) && !(iFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextSecondaryAttack < 0.0 )
		{
			Reload2();
			//g_iShots = 0;
			//return;
		}

		WeaponIdle( );
		return;
	}
	
	// catch all
	if ( ShouldWeaponIdle() )
	{
		WeaponIdle();
	}
	WeaponIdle();
}

/*
=====================
CBasePlayer::SelectItem

  Switch weapons
=====================
*/
void CBasePlayer::SelectItem(const char *pstr)
{
	if (!pstr)
		return;

	CBasePlayerItem *pItem = NULL;

	if (!pItem)
		return;

	
	if (pItem == m_pActiveItem)
		return;

	if (m_pActiveItem)
		m_pActiveItem->Holster( );
	
	m_pLastItem = m_pActiveItem;
	m_pActiveItem = pItem;

	if (m_pActiveItem)
	{
		m_pActiveItem->Deploy( );
	}
}

/*
=====================
CBasePlayer::SelectLastItem

=====================
*/
void CBasePlayer::SelectLastItem(void)
{
	if (!m_pLastItem)
	{
		return;
	}

	if ( m_pActiveItem && !m_pActiveItem->CanHolster() )
	{
		return;
	}

	if (m_pActiveItem)
		m_pActiveItem->Holster( );
	
	CBasePlayerItem *pTemp = m_pActiveItem;
	m_pActiveItem = m_pLastItem;
	m_pLastItem = pTemp;
	m_pActiveItem->Deploy( );
}

/*
=====================
CBasePlayer::Killed

=====================
*/
void CBasePlayer::Killed( entvars_t *pevAttacker, int iGib )
{
	// Holster weapon immediately, to allow it to cleanup
	if ( m_pActiveItem )
		 m_pActiveItem->Holster( );
}

/*
=====================
CBasePlayer::Spawn

=====================
*/
void CBasePlayer::Spawn( void )
{
	if (m_pActiveItem)
		m_pActiveItem->Deploy( );
}

/*
=====================
UTIL_TraceLine

Don't actually trace, but act like the trace didn't hit anything.
=====================
*/
void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
	memset( ptr, 0, sizeof( *ptr ) );
	ptr->flFraction = 1.0;
}

/*
=====================
UTIL_ParticleBox

For debugging, draw a box around a player made out of particles
=====================
*/
void UTIL_ParticleBox( CBasePlayer *player, float *mins, float *maxs, float life, unsigned char r, unsigned char g, unsigned char b )
{
	int i;
	vec3_t mmin, mmax;

	for ( i = 0; i < 3; i++ )
	{
		mmin[ i ] = player->pev->origin[ i ] + mins[ i ];
		mmax[ i ] = player->pev->origin[ i ] + maxs[ i ];
	}

	gEngfuncs.pEfxAPI->R_ParticleBox( (float *)&mmin, (float *)&mmax, 5.0, 0, 255, 0 );
}

/*
=====================
UTIL_ParticleBoxes

For debugging, draw boxes for other collidable players
=====================
*/
void UTIL_ParticleBoxes( void )
{
	int idx;
	physent_t *pe;
	cl_entity_t *player;
	vec3_t mins, maxs;
	
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	player = gEngfuncs.GetLocalPlayer();
	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( player->index - 1 );	

	for ( idx = 1; idx < 100; idx++ )
	{
		pe = gEngfuncs.pEventAPI->EV_GetPhysent( idx );
		if ( !pe )
			break;

		if ( pe->info >= 1 && pe->info <= gEngfuncs.GetMaxClients() )
		{
			mins = pe->origin + pe->mins;
			maxs = pe->origin + pe->maxs;

			gEngfuncs.pEfxAPI->R_ParticleBox( (float *)&mins, (float *)&maxs, 0, 0, 255, 2.0 );
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

/*
=====================
UTIL_ParticleLine

For debugging, draw a line made out of particles
=====================
*/
void UTIL_ParticleLine( CBasePlayer *player, float *start, float *end, float life, unsigned char r, unsigned char g, unsigned char b )
{
	gEngfuncs.pEfxAPI->R_ParticleLine( start, end, r, g, b, life );
}

/*
=====================
CBasePlayerWeapon::PrintState

For debugging, print out state variables to log file
=====================
*/
void CBasePlayerWeapon::PrintState( void )
{
  if( !gpGlobals )
    InitGlobals( );
	COM_Log( "c:\\hl.log", "%.4f ", gpGlobals->time );
	COM_Log( "c:\\hl.log", "%.4f ", m_pPlayer->m_flNextAttack );
	COM_Log( "c:\\hl.log", "%.4f ", m_flNextPrimaryAttack );
	COM_Log( "c:\\hl.log", "%.4f ", m_flTimeWeaponIdle - gpGlobals->time);
	COM_Log( "c:\\hl.log", "%i ", m_iClip );
}

/*
=====================
HUD_InitClientWeapons

Set up weapons, player and functions needed to run weapons code client-side.
=====================
*/
void HUD_InitClientWeapons( void )
{
	static int initialized = 0;
	if ( initialized )
		return;

	initialized = 1;

	// Set up pointer ( dummy object )
	gpGlobals = &Globals;

	// Fill in current time ( probably not needed )
	//gpGlobals->time = gEngfuncs.GetClientTime();

	// Fake functions
	g_engfuncs.pfnPrecacheModel		= stub_PrecacheModel;
	g_engfuncs.pfnPrecacheSound		= stub_PrecacheSound;
	g_engfuncs.pfnPrecacheEvent		= stub_PrecacheEvent;
	g_engfuncs.pfnNameForFunction	= stub_NameForFunction;
	g_engfuncs.pfnSetModel			= stub_SetModel;
	g_engfuncs.pfnSetClientMaxspeed = HUD_SetMaxSpeed;

	// Handled locally
	g_engfuncs.pfnPlaybackEvent		= HUD_PlaybackEvent;
	g_engfuncs.pfnAlertMessage		= AlertMessage;

	// Pass through to engine
	g_engfuncs.pfnPrecacheEvent		= gEngfuncs.pfnPrecacheEvent;
	g_engfuncs.pfnRandomFloat		= gEngfuncs.pfnRandomFloat;
	g_engfuncs.pfnRandomLong		= gEngfuncs.pfnRandomLong;

	// Allocate a slot for the local player
	HUD_PrepEntity( &player		, NULL );

	// Allocate slot(s) for each weapon that we are going to be predicting

	// PE
	HUD_PrepEntity( &g_Deagle		, &player );
//	HUD_PrepEntity( &g_Seburo		, &player );
	HUD_PrepEntity( &g_Benelli		, &player );
	HUD_PrepEntity( &g_Beretta		, &player );
	HUD_PrepEntity( &g_M16			, &player );
	HUD_PrepEntity( &g_Glock		, &player );
	HUD_PrepEntity( &g_Knife		, &player );
	HUD_PrepEntity( &g_Minigun		, &player );
	HUD_PrepEntity( &g_Microuzi		, &player );
	HUD_PrepEntity( &g_Microuzi_a	, &player );
	HUD_PrepEntity( &g_Mp5			, &player );
	HUD_PrepEntity( &g_HandGren		, &player );
	HUD_PrepEntity( &g_Usp			, &player );
	HUD_PrepEntity( &g_Stoner		, &player );
	HUD_PrepEntity( &g_Sig550		, &player );
//	HUD_PrepEntity( &g_P226			, &player );
	HUD_PrepEntity( &g_P225			, &player );
	HUD_PrepEntity( &g_Beretta_a	, &player );
	HUD_PrepEntity( &g_Glock_auto_a	, &player );
//	HUD_PrepEntity( &g_Glock_auto	, &player );
//	HUD_PrepEntity( &g_MP5sd		, &player );
	HUD_PrepEntity( &g_Tavor		, &player );
	HUD_PrepEntity( &g_Sawed		, &player );
	HUD_PrepEntity( &g_AK47			, &player );
	HUD_PrepEntity( &g_Flame		, &player );
//	HUD_PrepEntity( &g_UspMP		, &player );
	HUD_PrepEntity( &g_Hand			, &player );
//	HUD_PrepEntity( &g_Spray		, &player );

}

/*
=====================
HUD_GetLastOrg

Retruns the last position that we stored for egon beam endpoint.
=====================
*/
void HUD_GetLastOrg( float *org )
{
	int i;
	
	// Return last origin
	for ( i = 0; i < 3; i++ )
	{
		org[i] = previousorigin[i];
	}
}

/*
=====================
HUD_SetLastOrg

Remember our exact predicted origin so we can draw the egon to the right position.
=====================
*/
void HUD_SetLastOrg( void )
{
	int i;
	
	// Offset final origin by view_offset
	for ( i = 0; i < 3; i++ )
	{
		previousorigin[i] = g_finalstate->playerstate.origin[i] + g_finalstate->client.view_ofs[ i ];
	}
}

/*
=====================
HUD_WeaponsPostThink

Run Weapon firing code on client
=====================
*/
void HUD_WeaponsPostThink( local_state_s *from, local_state_s *to, usercmd_t *cmd, double time, unsigned int random_seed )
{
	int i;
	int buttonsChanged;
	CBasePlayerWeapon *pWeapon = NULL;
	CBasePlayerWeapon *pCurrent;
	weapon_data_t nulldata, *pfrom, *pto;
	static int lasthealth;

	memset( &nulldata, 0, sizeof( nulldata ) );

	// Get current clock
	//gpGlobals->time = time;

	//Lets weapons code use frametime to decrement timers and stuff.
	gpGlobals->frametime = cmd->msec / 1000.0f;

	// Fill in data based on selected weapon
	// FIXME, make this a method in each weapon?  where you pass in an entity_state_t *?
	switch ( from->client.m_iId )
	{
		case WEAPON_MP5:
			pWeapon = &g_Mp5;
			break;

		case WEAPON_HANDGRENADE:
			pWeapon = &g_HandGren;
			break;

			//PE
		case WEAPON_DEAGLE:
			pWeapon = &g_Deagle;
			break;
/*		case WEAPON_SEBURO:
			pWeapon = &g_Seburo;
			break;*/
		case WEAPON_BENELLI:
			pWeapon = &g_Benelli;
			break;
		case WEAPON_BERETTA:
			pWeapon = &g_Beretta;
			break;
		case WEAPON_M16:
			pWeapon = &g_M16;
			break;
		case WEAPON_GLOCK:
			pWeapon = &g_Glock;
			break;
		case WEAPON_KNIFE:
			pWeapon = &g_Knife;
			break;
		case WEAPON_MINIGUN:
			pWeapon = &g_Minigun;
			break;
		case WEAPON_MICROUZI:
			pWeapon = &g_Microuzi;
			break;
		case WEAPON_MICROUZI_A:
			pWeapon = &g_Microuzi_a;
			break;
		case WEAPON_USP:
			pWeapon = &g_Usp;
			break;
		case WEAPON_STONER:
			pWeapon = &g_Stoner;
			break;
		case WEAPON_SIG550:
			pWeapon = &g_Sig550;
			break;
		/*case WEAPON_P226:
			pWeapon = &g_P226;
			break;*/
		case WEAPON_P225:
			pWeapon = &g_P225;
			break;
		case WEAPON_BERETTA_A:
			pWeapon = &g_Beretta_a;
			break;
		case WEAPON_GLOCK_AUTO_A:
			pWeapon = &g_Glock_auto_a;
			break;
		/*case WEAPON_GLOCK_AUTO:
			pWeapon = &g_Glock_auto;
			break;*/
		/*case WEAPON_MP5SD:
			pWeapon = &g_MP5sd;
			break;*/
		case WEAPON_TAVOR:
			pWeapon = &g_Tavor;
			break;
		case WEAPON_SAWED:
			pWeapon = &g_Sawed;
			break;
		case WEAPON_AK47:
			pWeapon = &g_AK47;
			break;
		/*case WEAPON_USPMP:
			pWeapon = &g_UspMP;
			break;*/
		case WEAPON_HAND:
			pWeapon = &g_Hand;
			break;
		/*case WEAPON_SPRAY:
			pWeapon = &g_Spray;
			break;*/

		case WEAPON_FLAME:
			pWeapon = &g_Flame;
			break;
	}

	// Store pointer to our destination entity_state_t so we can get our origin, etc. from it
	//  for setting up events on the client
	g_finalstate = to;

	// If we are running events/etc. go ahead and see if we
	//  managed to die between last frame and this one
	// If so, run the appropriate player killed or spawn function
	if ( g_runfuncs )
	{
		if ( to->client.health <= 0 && lasthealth > 0 )
		{
			player.Killed( NULL, 0 );
			
		}
		else if ( to->client.health > 0 && lasthealth <= 0 )
		{
			player.Spawn();
		}

		lasthealth = to->client.health;
	}

	// We are not predicting the current weapon, just bow out here.
	if ( !pWeapon )
		return;

	for ( i = 0; i < 32; i++ )
	{
		pCurrent = g_pWpns[ i ];
		if ( !pCurrent )
		{
			continue;
		}

		pfrom = &from->weapondata[ i ];
		
		pCurrent->m_fInReload			= pfrom->m_fInReload;
		pCurrent->m_fInSpecialReload	= pfrom->m_fInSpecialReload;
//		pCurrent->m_flPumpTime			= pfrom->m_flPumpTime;
		pCurrent->m_iClip				= pfrom->m_iClip;
		pCurrent->m_flNextPrimaryAttack	= pfrom->m_flNextPrimaryAttack;
		pCurrent->m_flNextSecondaryAttack = pfrom->m_flNextSecondaryAttack;
		pCurrent->m_flTimeWeaponIdle	= pfrom->m_flTimeWeaponIdle;
		pCurrent->pev->fuser1			= pfrom->fuser1;
		pCurrent->m_flStartThrow		= pfrom->fuser2;
		pCurrent->m_flReleaseThrow		= pfrom->fuser3;
		//Spin superjump
		pCurrent->pev->fuser4			= pfrom->fuser4;
		pCurrent->m_chargeReady			= pfrom->iuser1;
		pCurrent->m_fInAttack			= pfrom->iuser2;
		pCurrent->m_fireState			= pfrom->iuser3;
		pCurrent->m_iClip2				= pfrom->iuser4;

		pCurrent->m_iSecondaryAmmoType		= (int)from->client.vuser3[ 2 ];
		pCurrent->m_iPrimaryAmmoType		= (int)from->client.vuser4[ 0 ];
		player.m_rgAmmo[ pCurrent->m_iPrimaryAmmoType ]	= (int)from->client.vuser4[ 1 ];
		player.m_rgAmmo[ pCurrent->m_iSecondaryAmmoType ]	= (int)from->client.vuser4[ 2 ];
	}

	// For random weapon events, use this seed to seed random # generator
	player.random_seed = random_seed;

	// Get old buttons from previous state.
	player.m_afButtonLast = from->playerstate.oldbuttons;

	// Which buttsons chave changed
	buttonsChanged = (player.m_afButtonLast ^ cmd->buttons);	// These buttons have changed this frame
	
	// Debounced button codes for pressed/released
	// The changed ones still down are "pressed"
	player.m_afButtonPressed =  buttonsChanged & cmd->buttons;	
	// The ones not down are "released"
	player.m_afButtonReleased = buttonsChanged & (~cmd->buttons);
	player.pev->v_angle = cmd->viewangles;
	player.pev->origin = from->client.origin;

	// Set player variables that weapons code might check/alter
	player.pev->button = cmd->buttons;

	player.pev->velocity = from->client.velocity;
	player.pev->flags = from->client.flags;

	player.pev->deadflag = from->client.deadflag;
	player.pev->waterlevel = from->client.waterlevel;
	player.pev->maxspeed    = from->client.maxspeed;
	player.pev->fov = from->client.fov;
	player.pev->weaponanim = from->client.weaponanim;
	player.pev->viewmodel = from->client.viewmodel;
	player.m_flNextAttack = from->client.m_flNextAttack;
	player.m_flNextAmmoBurn = from->client.fuser2;
	player.m_flAmmoStartCharge = from->client.fuser3;
	//Spin superjump
	player.pev->fuser4 = from->client.fuser4;

	//Stores all our ammo info, so the client side weapons can use them.
	player.ammo_9mm			= (int)from->client.vuser1[0];
	player.ammo_slot5			= (int)from->client.vuser1[1];
	player.ammo_slot4		= (int)from->client.vuser1[2];
	player.ammo_slot2_a		= (int)from->client.ammo_nails; //is an int anyways...
	player.ammo_slot3_a	= (int)from->client.ammo_shells; 
	player.ammo_slot2		= (int)from->client.ammo_cells;
	player.ammo_slot3		= (int)from->client.vuser2[0];
	player.ammo_slot4_a		= (int)from->client.ammo_rockets;

	
	// Point to current weapon object
	if ( from->client.m_iId )
	{
		player.m_pActiveItem = g_pWpns[ from->client.m_iId ];
	}

	/*if ( player.m_pActiveItem->m_iId == WEAPON_RPG )
	{
		 ( ( CRpg * )player.m_pActiveItem)->m_fSpotActive = (int)from->client.vuser2[ 1 ];
		 ( ( CRpg * )player.m_pActiveItem)->m_cActiveRockets = (int)from->client.vuser2[ 2 ];
	}*/
	
	// Don't go firing anything if we have died.
	// Or if we don't have a weapon model deployed
	if ( ( player.pev->deadflag == DEAD_NO ) && !CL_IsDead() && player.pev->viewmodel )
	{
		if ( player.m_flNextAttack <= 0 )
		{
			pWeapon->ItemPostFrame();
		}
		if( ( pWeapon->m_fInReload || pWeapon->m_fInReload2 ) )
		{
			if( ( g_fNextCalm <= gpGlobals->time ) )
			{
				if( g_iShots > 0 )
					g_iShots -= (int)( ( gpGlobals->time - g_fNextCalm ) / PE_CROSS_CALM + 1 );
				g_iShots = max( 0, g_iShots );
				g_fNextCalm = gpGlobals->time + PE_CROSS_CALM;
			}
		}
	}

	// Assume that we are not going to switch weapons
	to->client.m_iId					= from->client.m_iId;

	// Now see if we issued a changeweapon command ( and we're not dead )
	if ( cmd->weaponselect && ( player.pev->deadflag == DEAD_NO ) && !CL_IsDead() )
	{
		// Switched to a different weapon?
		if ( from->weapondata[ cmd->weaponselect ].m_iId == cmd->weaponselect )
		{
			CBasePlayerWeapon *pNew = g_pWpns[ cmd->weaponselect ];
			if ( pNew && ( pNew != pWeapon ) )
			{
				// Put away old weapon
				if (player.m_pActiveItem)
					player.m_pActiveItem->Holster( );
				
				player.m_pLastItem = player.m_pActiveItem;
				player.m_pActiveItem = pNew;

				// Deploy new weapon
				if (player.m_pActiveItem)
				{
					player.m_pActiveItem->Deploy( );
				}

				// Update weapon id so we can predict things correctly.
				to->client.m_iId = cmd->weaponselect;
			}
		}
	}

	// Copy in results of prediction code
	to->client.viewmodel				= player.pev->viewmodel;
	to->client.fov						= player.pev->fov;
	to->client.weaponanim				= player.pev->weaponanim;
	to->client.m_flNextAttack			= player.m_flNextAttack;
	to->client.fuser2					= player.m_flNextAmmoBurn;
	to->client.fuser3					= player.m_flAmmoStartCharge;
	//Spin superjump
	to->client.fuser4					= player.pev->fuser4;
	to->client.maxspeed					= player.pev->maxspeed;

	//HL Weapons
	to->client.vuser1[0]				= player.ammo_9mm;
	to->client.vuser1[1]				= player.ammo_slot5;
	to->client.vuser1[2]				= player.ammo_slot4;

	to->client.ammo_nails				= player.ammo_slot2_a;
	to->client.ammo_shells				= player.ammo_slot3_a;
	to->client.ammo_cells				= player.ammo_slot2;
	to->client.vuser2[0]				= player.ammo_slot3;
	to->client.ammo_rockets				= player.ammo_slot4_a;

	/*if ( player.m_pActiveItem->m_iId == WEAPON_RPG )
	{
		 from->client.vuser2[ 1 ] = ( ( CRpg * )player.m_pActiveItem)->m_fSpotActive;
		 from->client.vuser2[ 2 ] = ( ( CRpg * )player.m_pActiveItem)->m_cActiveRockets;
	}*/

	// Make sure that weapon animation matches what the game .dll is telling us
	//  over the wire ( fixes some animation glitches )
	if ( g_runfuncs && ( HUD_GetWeaponAnim() != to->client.weaponanim ) )
	{
		int body = 2;

		//Pop the model to body 0.
		/*if ( pWeapon == &g_Tripmine )
			 body = 0;

		//Show laser sight/scope combo
		if ( pWeapon == &g_Python && bIsMultiplayer() )
			 body = 1;*/
		
		// Force a fixed anim down to viewmodel
		HUD_SendWeaponAnim( to->client.weaponanim, body, 1 );
	}

	for ( i = 0; i < 32; i++ )
	{
		pCurrent = g_pWpns[ i ];

		pto = &to->weapondata[ i ];

		if ( !pCurrent )
		{
			memset( pto, 0, sizeof( weapon_data_t ) );
			continue;
		}
	
		pto->m_fInReload				= pCurrent->m_fInReload;
		pto->m_fInSpecialReload			= pCurrent->m_fInSpecialReload;
//		pto->m_flPumpTime				= pCurrent->m_flPumpTime;
		pto->m_iClip					= pCurrent->m_iClip; 
		pto->m_flNextPrimaryAttack		= pCurrent->m_flNextPrimaryAttack;
		pto->m_flNextSecondaryAttack	= pCurrent->m_flNextSecondaryAttack;
		pto->m_flTimeWeaponIdle			= pCurrent->m_flTimeWeaponIdle;
		pto->fuser1						= pCurrent->pev->fuser1;
		pto->fuser2						= pCurrent->m_flStartThrow;
		pto->fuser3						= pCurrent->m_flReleaseThrow;
		//Spin superjump
		pto->fuser4						= pCurrent->pev->fuser4;
		pto->iuser1						= pCurrent->m_chargeReady;
		pto->iuser2						= pCurrent->m_fInAttack;
		pto->iuser3						= pCurrent->m_fireState;
		pto->iuser4						= pCurrent->m_iClip2;

		// Decrement weapon counters, server does this at same time ( during post think, after doing everything else )
		pto->m_flNextReload				-= cmd->msec / 1000.0;
		pto->m_fNextAimBonus			-= cmd->msec / 1000.0;
		pto->m_flNextPrimaryAttack		-= cmd->msec / 1000.0;
		pto->m_flNextSecondaryAttack	-= cmd->msec / 1000.0;
		pto->m_flTimeWeaponIdle			-= cmd->msec / 1000.0;
		pto->fuser1						-= cmd->msec / 1000.0;

		to->client.vuser3[2]				= pCurrent->m_iSecondaryAmmoType;
		to->client.vuser4[0]				= pCurrent->m_iPrimaryAmmoType;
		to->client.vuser4[1]				= player.m_rgAmmo[ pCurrent->m_iPrimaryAmmoType ];
		to->client.vuser4[2]				= player.m_rgAmmo[ pCurrent->m_iSecondaryAmmoType ];

/*		if ( pto->m_flPumpTime != -9999 )
		{
			pto->m_flPumpTime -= cmd->msec / 1000.0;
			if ( pto->m_flPumpTime < -0.001 )
				pto->m_flPumpTime = -0.001;
		}*/

		if ( pto->m_fNextAimBonus < -1.0 )
		{
			pto->m_fNextAimBonus = -1.0;
		}

		if ( pto->m_flNextPrimaryAttack < -1.0 )
		{
			pto->m_flNextPrimaryAttack = -1.0;
		}

		if ( pto->m_flNextSecondaryAttack < -0.001 )
		{
			pto->m_flNextSecondaryAttack = -0.001;
		}

		if ( pto->m_flTimeWeaponIdle < -0.001 )
		{
			pto->m_flTimeWeaponIdle = -0.001;
		}

		if ( pto->m_flNextReload < -0.001 )
		{
			pto->m_flNextReload = -0.001;
		}

		if ( pto->fuser1 < -0.001 )
		{
			pto->fuser1 = -0.001;
		}
	}

	// m_flNextAttack is now part of the weapons, but is part of the player instead
	to->client.m_flNextAttack -= cmd->msec / 1000.0;
	if ( to->client.m_flNextAttack < -0.001 )
	{
		to->client.m_flNextAttack = -0.001;
	}

	to->client.fuser2 -= cmd->msec / 1000.0;
	if ( to->client.fuser2 < -0.001 )
	{
		to->client.fuser2 = -0.001;
	}
	
	to->client.fuser3 -= cmd->msec / 1000.0;
	if ( to->client.fuser3 < -0.001 )
	{
		to->client.fuser3 = -0.001;
	}

	// Store off the last position from the predicted state.
	HUD_SetLastOrg();

	// Wipe it so we can't use it after this frame
	g_finalstate = NULL;
}

/*
=====================
HUD_PostRunCmd

Client calls this during prediction, after it has moved the player and updated any info changed into to->
time is the current client clock based on prediction
cmd is the command that caused the movement, etc
runfuncs is 1 if this is the first time we've predicted this command.  If so, sounds and effects should play, otherwise, they should
be ignored
=====================
*/
void CL_DLLEXPORT HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed )
{
//	RecClPostRunCmd(from, to, cmd, runfuncs, time, random_seed);

	g_runfuncs = runfuncs;

	//Event code depends on this stuff, so always initialize it.
	HUD_InitClientWeapons();

#if defined( CLIENT_WEAPONS )
	if ( cl_lw && cl_lw->value )
	{
		HUD_WeaponsPostThink( from, to, cmd, time, random_seed );
	}
	else
#endif
	{
		to->client.fov = g_lastFOV;
	}

	// All games can use FOV state
	g_lastFOV = to->client.fov;
}

int CBasePlayerWeapon::iItemSlot( CBasePlayer *pPlayer )
{
	return gWR.GetWeapon( m_iId )->iSlot;
}

