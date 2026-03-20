#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "pe_menus.h"
#include "pe_notify.h"

#ifndef CLIENT_DLL

float g_fSatSyn = 0;
float g_fSatSec = 0;

LINK_ENTITY_TO_CLASS( laser_spot, CLaserSpot );

//=========================================================
//=========================================================
CLaserSpot *CLaserSpot::CreateSpot( void )
{
	CLaserSpot *pSpot = GetClassPtr( (CLaserSpot *)NULL );
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING("laser_spot");

	return pSpot;
}

//=========================================================
//=========================================================
void CLaserSpot::Spawn( void )
{
	Precache( );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/lasersyn.spr");
	UTIL_SetOrigin( pev, pev->origin );
};

//=========================================================
// Suspend- make the laser sight invisible. 
//=========================================================
void CLaserSpot::Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;
	
	SetThink( Revive );
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

//=========================================================
// Revive - bring a suspended laser sight back.
//=========================================================
void CLaserSpot::Revive( void )
{
	pev->effects &= ~EF_NODRAW;

	SetThink( NULL );
}

void CLaserSpot::Precache( void )
{
	PRECACHE_MODEL("sprites/lasersyn.spr");
	PRECACHE_MODEL("sprites/lasersec.spr");
	PRECACHE_MODEL("sprites/orbital.spr");
};

#endif


enum deagle_e
{
	DEAGLE_IDLE1 = 0,
	DEAGLE_DRAW,
	DEAGLE_RELOAD,
	DEAGLE_RELOAD_EMPTY,
	DEAGLE_SHOT,
	DEAGLE_SHOT_EMPTY
};

LINK_ENTITY_TO_CLASS( weapon_orbital, COrbital );

int COrbital::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Orbital";
	p->iMaxAmmo1 = 1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 1;
	p->iFlags = 0;
	p->iSlot = 4;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_ORBITAL;
	p->iWeight = 1;

	return 1;
}

int COrbital::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void COrbital::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_orbital"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_ORBITAL;
//	SET_MODEL(ENT(pev), "models/w_357.mdl");

	m_iDefaultAmmo = 1;

	FallInit();// get ready to fall down.
}


void COrbital::Precache( void )
{
}

BOOL COrbital::Deploy( )
{
#ifndef CLIENT_DLL
	return DefaultDeploy( NULL, NULL, 0, "knife" );
#endif
	return TRUE;
}


void COrbital::Holster( int skiplocal /* = 0 */ )
{
#ifndef CLIENT_DLL
	if( m_pSpot )
		m_pSpot->pev->effects = EF_NODRAW;
#endif
}

void COrbital::SecondaryAttack( void )
{ }

void COrbital::PrimaryAttack()
{
#ifndef CLIENT_DLL
	if( !m_pSpot )
		WeaponIdle( );
	if( !m_pSpot )
		return;
	if (m_iClip <= 0)
			return;

	if (m_iClip <= 0)
			return;

	if( ( m_pPlayer->m_iTeam == 1 ) && ( ( g_fSatSec - gpGlobals->time ) > 0 ))
		return;
	if( ( m_pPlayer->m_iTeam == 2 ) && ( ( g_fSatSyn - gpGlobals->time ) > 0 ))
		return;

	m_iClip--;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_pPlayer->SelectLastItem( );
	m_pPlayer->m_pLastItem = NULL;
	m_pPlayer->m_pOrbital = pev;

	m_pPlayer->m_pBeamOrig = *m_pSpot->pev;

	m_pPlayer->m_fStartBeam = gpGlobals->time + 3;

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex (i);
	
		if (!pPlayer)
			continue;

		CLIENT_COMMAND( pPlayer->edict( ), "speak \"warning satellite activity detected\"\n" );
		NotifyAll( NTM_SAT );
	}
	
	if( m_pSpot )
	{
		m_pSpot->Killed( NULL, GIB_NORMAL );
		m_pSpot = NULL;
	}

	if( m_pPlayer->m_iTeam == 1 )
		g_fSatSec = gpGlobals->time + 15;
	else
		g_fSatSyn = gpGlobals->time + 15;

#endif
}


void COrbital::Reload( void )
{
}


void COrbital::WeaponIdle( void )
{
	if( m_iClip <= 0 )
		return;
#ifndef CLIENT_DLL
	if( !m_pSpot )
	{
		m_pSpot = CLaserSpot::CreateSpot( );
	}
	if( m_pPlayer->m_iTeam == 1 )
		SET_MODEL(ENT(m_pSpot->pev), "sprites/lasersec.spr");
	else
		SET_MODEL(ENT(m_pSpot->pev), "sprites/lasersyn.spr");

	m_pSpot->pev->effects &= ~EF_NODRAW;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecSrc = m_pPlayer->GetGunPosition( );
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;
	UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );
	
	UTIL_SetOrigin( m_pSpot->pev, tr.vecEndPos );
#endif
}
