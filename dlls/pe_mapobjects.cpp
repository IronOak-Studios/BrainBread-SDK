#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "decals.h"
#include "gamerules.h"
#include "pe_utils.h"
#include "pe_rules.h"
#include "pe_rules_hacker.h"
#include "pe_rules_vip.h"
#include "pe_mapobjects.h"
#include "pe_menus.h"
#include "pe_notify.h"

#define AD_TORSO_HEART	(1<<0)
#define AD_TORSO_BOMB	(1<<1)
#define AD_ARMS_FAST	(1<<2)
#define AD_ARMS_SMART	(1<<3)
#define AD_LEGS_ACHIL	(1<<4)
#define AD_HEAD_SENTI	(1<<5)
#define AD_HEAD_ORBITAL	(1<<6)
#define AD_NANO_FACT	(1<<7)
#define AD_NANO_SKIN	(1<<8)


#define HACK_TIME ( FBitSet( pl->m_iAddons, AD_ARMS_FAST ) ? 3 : 6 )

extern int gmsgShowMenu;
extern int gmsgSpecial;

class CObItem: public CBasePlayerWeapon
{
  int GetItemInfo(ItemInfo *p)
  {
	  p->pszName = "weapon_case";
	  p->pszAmmo1 = NULL;
	  p->iMaxAmmo1 = -1;
	  p->pszAmmo2 = NULL;
	  p->iMaxAmmo2 = -1;
	  p->iMaxClip = -1;
	  p->iSlot = iItemSlot( m_pPlayer );
	  p->iPosition = 0;
	  p->iFlags = 0;
	  p->iId = m_iId = OBJECTIVE_ITEM;
	  p->iWeight = ( iItemSlot( m_pPlayer ) == 2 ? 4 : 3 );

	  return 1;
  }
	void Precache( )
	{
		PRECACHE_MODEL("models/case/w_case.mdl");
	}
	void Spawn( )
	{
    Precache( );
		SET_MODEL(ENT(pev), "models/case/w_case.mdl");
		m_iClip = -1;
		m_iId = OBJECTIVE_ITEM;

    pev->classname = MAKE_STRING("bb_objective_item");

		FallInit( );
	};
	EXPORT void FallInit( )
	{
		pev->movetype = MOVETYPE_TOSS;
		pev->solid = SOLID_BBOX;

		UTIL_SetOrigin( pev, pev->origin );
		UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0) );//pointsize until it lands on the ground.
		
		SetTouch( &CObItem::DefaultTouch );
		SetThink( &CObItem::FallThink );

		pev->nextthink = gpGlobals->time + 0.1;
	}
	EXPORT void FallThink( )
	{
		pev->nextthink = gpGlobals->time + 0.1;

		if ( pev->flags & FL_ONGROUND )
		{
			// clatter if we have an owner (i.e., dropped by someone)
			// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
			if ( !FNullEnt( pev->owner ) )
			{
				int pitch = 95 + RANDOM_LONG(0,29);
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "items/weapondrop1.wav", 1, ATTN_NORM, 0, pitch);	
			}

			// lie flat
			pev->angles.x = 0;
			pev->angles.z = 0;

			Materialize(); 
		}
	}
	EXPORT void Materialize( )
	{
		if ( pev->effects & EF_NODRAW )
		{
			// changing from invisible state to visible.
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
			pev->effects &= ~EF_NODRAW;
			pev->effects |= EF_MUZZLEFLASH;
		}

		pev->solid = SOLID_TRIGGER;

		UTIL_SetOrigin( pev, pev->origin );// link into world.
		SetTouch (&CObItem::DefaultTouch);
		SetThink (NULL);

	}
	EXPORT void DefaultTouch( CBaseEntity *pOther )
	{
		if ( !pOther->IsPlayer() )
			return;

		CBasePlayer *pPlayer = (CBasePlayer *)pOther;
    if( pPlayer->m_iTeam != 1 )
      return;

		if (pOther->AddPlayerItem( this ))
		{
			AttachToPlayer( pPlayer );
			pPlayer->m_bIsSpecial = TRUE;
			DisableAll( "bb_objective_item" );
			EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);
			MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
				WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
				WRITE_BYTE( 1 );
			MESSAGE_END( );
			pPlayer->pev->body = 1;
		}
	}
	int AddToPlayer( CBasePlayer *pPlayer )
	{
		if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
				WRITE_BYTE( OBJECTIVE_ITEM );
			MESSAGE_END();
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( bb_objective_item, CObItem );

/*class CObCase: public CBaseAnimating
{
	void Spawn()
	{
		CBasePlayerWeapon *pNewWeapon = (CBasePlayerWeapon*)CBaseEntity::Create( "weapon_case", pev->origin, pev->angles, NULL );
		if( pNewWeapon )
		pNewWeapon->Spawn( );
		pev->movetype = MOVETYPE_TOSS;
		pev->solid = SOLID_TRIGGER;

		UTIL_SetSize( pev, g_vecZero, g_vecZero );
	};
};
LINK_ENTITY_TO_CLASS( pe_object_case, CObCase );

class CObHTool: public CBasePlayerWeapon
{
	void Precache( )
	{
		PRECACHE_MODEL("models/hacker/w_htool.mdl");
	}
	void Spawn( )
	{
		SET_MODEL(ENT(pev), "models/hacker/w_htool.mdl");
		m_iClip = -1;
		m_iId = ITEM_HTOOL;


		FallInit( );
	};
	EXPORT void FallInit( )
	{
		pev->movetype = MOVETYPE_TOSS;
		pev->solid = SOLID_BBOX;

		UTIL_SetOrigin( pev, pev->origin );
		UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0) );//pointsize until it lands on the ground.
		
		SetTouch( DefaultTouch );
		SetThink( FallThink );

		pev->nextthink = gpGlobals->time + 0.1;
	}
	EXPORT void FallThink( )
	{
		pev->nextthink = gpGlobals->time + 0.1;

		if ( pev->flags & FL_ONGROUND )
		{
			// clatter if we have an owner (i.e., dropped by someone)
			// don't clatter if the gun is waiting to respawn (if it's waiting, it is invisible!)
			if ( !FNullEnt( pev->owner ) )
			{
				int pitch = 95 + RANDOM_LONG(0,29);
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "items/weapondrop1.wav", 1, ATTN_NORM, 0, pitch);	
			}

			// lie flat
			pev->angles.x = 0;
			pev->angles.z = 0;

			Materialize(); 
		}
	}
	EXPORT void Materialize( )
	{
		if ( pev->effects & EF_NODRAW )
		{
			// changing from invisible state to visible.
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
			pev->effects &= ~EF_NODRAW;
			pev->effects |= EF_MUZZLEFLASH;
		}

		pev->solid = SOLID_TRIGGER;

		UTIL_SetOrigin( pev, pev->origin );// link into world.
		SetTouch (DefaultTouch);
		SetThink (NULL);

	}
	EXPORT void DefaultTouch( CBaseEntity *pOther )
	{
		if ( !pOther->IsPlayer() )
			return;

		CBasePlayer *pPlayer = (CBasePlayer *)pOther;

		if (pOther->AddPlayerItem( this ))
		{
			AttachToPlayer( pPlayer );
			pPlayer->m_bIsSpecial = TRUE;
			DisableAll( "pe_object_htool" );
			EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);
			MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
				WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
				WRITE_BYTE( 1 );
			MESSAGE_END( );
			pPlayer->pev->body = 1;
		}
	}
	int AddToPlayer( CBasePlayer *pPlayer )
	{
		if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
				WRITE_BYTE( ITEM_HTOOL );
			MESSAGE_END();
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( pe_object_htool, CObHTool );

//---------------------------------------
// Escape Zone für V.I.P Scenario
//---------------------------------------
LINK_ENTITY_TO_CLASS( pe_escapezone, cPEEscape );

void cPEEscape::Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SET_MODEL(ENT(pev), STRING(pev->model));
}

void cPEEscape::Touch( CBaseEntity *pOther )
{
	if( !pOther )
		return;
	if( !pOther->IsPlayer() )
		return;
	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	
	if( pPlayer->m_iTeam == 1 && pPlayer->m_bIsSpecial && pPlayer->IsAlive( ) )
	{
		((cPEVip*)g_pGameRules)->VipEscaped( );
		pPlayer->BonusPoint( TASK_SAFE );
		pPlayer->m_bIsSpecial = FALSE;
		MESSAGE_BEGIN( MSG_ALL, gmsgSpecial );
			WRITE_BYTE( ENTINDEX(pPlayer->edict( )) );
			WRITE_BYTE( 0 );
		MESSAGE_END( );
	}
}
//---------------------------------------
// Escape Zone für V.I.P Scenario
//---------------------------------------
*/
class cPERain : public CBaseEntity
{
	void Spawn( )
	{
		pev->solid = SOLID_TRIGGER;
		pev->movetype = MOVETYPE_NONE;
		pev->effects |= EF_NODRAW;
		SET_MODEL(ENT(pev), STRING(pev->model));
	}
	void KeyValue( KeyValueData *pkvd )
	{
		if( FStrEq( pkvd->szKeyName, "partsps" ) )
		{
			pev->iuser1 = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if( FStrEq( pkvd->szKeyName, "partspeed" ) )
		{
			pev->fuser1 = atof( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if( FStrEq( pkvd->szKeyName, "partsize" ) )
		{
			pev->fuser2 = atof( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if( FStrEq( pkvd->szKeyName, "partimage" ) )
		{
			pev->iuser2 = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
		else if( FStrEq( pkvd->szKeyName, "partspread" ) )
		{
			pev->fuser3 = atof( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
	}

};
LINK_ENTITY_TO_CLASS( pe_rain, cPERain );

//-------------------------
// Hacker Scenario Targets
//-------------------------
/*LINK_ENTITY_TO_CLASS( pe_terminal, cPEHackerTerminal );


void cPEHackerTerminal::ReSpawn()
{
	pev->renderfx = kRenderFxNone;
	SET_MODEL(ENT(pev), "models/hacker/terminal.mdl" );
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN * 4, VEC_HUMAN_HULL_MAX * 4 );
	UTIL_SetOrigin(pev, pev->origin );
	pev->solid			= SOLID_TRIGGER;
	pev->movetype		= MOVETYPE_TOSS;
	pev->health			= 50;
	pev->classname		= MAKE_STRING( "pe_terminal" );
	
	pev->body			= 0; // gun in holster
	pev->effects		= 0;

	m_bDone = false;
	m_bReached = FALSE;
	m_fHackEnd = gpGlobals->time + 500;

	if( ( m_iTerminal == 0 ) && ((cPEHacking*)g_pGameRules)->m_iTerminalCheck )
		m_iTerminal = 0;
	else
	{
		((cPEHacking*)g_pGameRules)->m_iTerminalCheck = 1;
		m_iTerminal = 1;
	}

	if (m_pClip == NULL)
	{
		//ALERT (at_console, "Erstelle terminal clipzone\n");
		m_pClip = GetClassPtr( ( CBaseEntity *)NULL );
		
		SET_MODEL(ENT(m_pClip->pev), "models/hacker/terminal.mdl" );
		UTIL_SetSize(m_pClip->pev, VEC_HUMAN_HULL_MIN * 2.9 , VEC_HUMAN_HULL_MAX * 2.9 );
		UTIL_SetOrigin (m_pClip->pev,pev->origin);

		m_pClip->pev->classname = MAKE_STRING("pe_terminal_clip");
		m_pClip->pev->solid = SOLID_BBOX;	
		
		m_pClip->pev->effects = EF_NODRAW;
		m_pClip->pev->movetype = MOVETYPE_TOSS;
	}

	//UTIL_EmitAmbientSound(edict(), pev->origin + Vector (0,0,6), "ambience/computal.wav",0.3, ATTN_NORM, 0 , PITCH_NORM);

	SetThink(Think);
}

cPEHackerTerminal::cPEHackerTerminal( )
{
	m_sTerminal[0] = '\0';
}

void cPEHackerTerminal::Spawn( void )
{
	Precache( );

	SET_MODEL(ENT(pev), "models/hacker/terminal.mdl" );
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN * 4, Vector( 16 * 4, 16 * 4, 20 ) );
	UTIL_SetOrigin(pev, pev->origin );
	pev->solid			= SOLID_TRIGGER;
	pev->movetype		= MOVETYPE_TOSS;
	pev->health			= 50;
	pev->classname		= MAKE_STRING( "pe_terminal" );
	
	pev->body			= 0; // gun in holster
	pev->effects		= 0;

	m_bDone = false;
	m_bReached = FALSE;
	m_fHackEnd = gpGlobals->time + 500;

	if( ( m_iTerminal == 0 ) && ((cPEHacking*)g_pGameRules)->m_iTerminalCheck )
		m_iTerminal = 0;
	else
	{
		((cPEHacking*)g_pGameRules)->m_iTerminalCheck = 1;
		m_iTerminal = 1;
	}
	if( strlen(m_sTerminal) <= 0 )
	{
		if( m_iTerminal == 0 )
			snprintf( m_sTerminal, sizeof(m_sTerminal), "PRIMARY" );
		else
			snprintf( m_sTerminal, sizeof(m_sTerminal), "SECONDARY" );
	}

	if (m_pClip == NULL)
	{
		//ALERT (at_console, "Erstelle terminal clipzone\n");
		m_pClip = GetClassPtr( ( CBaseEntity *)NULL );
		
		SET_MODEL(ENT(m_pClip->pev), "models/hacker/terminal.mdl" );
		UTIL_SetSize(m_pClip->pev, VEC_HUMAN_HULL_MIN * 2.9 , Vector( 16 * 2.9, 16 * 2.9, 72 ) );
		UTIL_SetOrigin (m_pClip->pev,pev->origin);

		m_pClip->pev->classname = MAKE_STRING("pe_terminal_clip");
		m_pClip->pev->solid = SOLID_BBOX;	
		
		m_pClip->pev->effects = EF_NODRAW;
		m_pClip->pev->movetype = MOVETYPE_TOSS;
	}

	//UTIL_EmitAmbientSound(edict(), pev->origin + Vector (0,0,6), "ambience/computal.wav",0.3, ATTN_NORM, 0 , PITCH_NORM);

	SetThink(Think);
}

void EXPORT cPEHackerTerminal::Think( )
{
	//ResetSequenceInfo( );
	if( m_pPlayer && m_bReached )
	{
		Vector dist = m_pPlayer->pev->origin - pev->origin;
		//ALERT( at_console, "%d\n", (int)dist.Length( ) );
		if( m_pPlayer->IsPlayer( ) && ( !m_pPlayer->IsAlive( ) || ( (int)dist.Length( ) > 90 ) || m_pPlayer->m_pActiveItem ) )
		{
			//ALERT( at_console, "isp: %d term %d alive %d dist %d item %d\n", m_pPlayer->IsPlayer( ), m_iTerminal, m_pPlayer->IsAlive( ), (int)dist.Length( ), m_pPlayer->m_pActiveItem );
			CountDown( m_pPlayer, 0 );
			m_bReached = FALSE;
			m_fHackEnd = gpGlobals->time + 500;
			m_pPlayer->m_iHideHUD &= ~HIDEHUD_WEAPONS;
			if( m_pWeapon )
			{
				m_pPlayer->m_pActiveItem = m_pWeapon;
				m_pPlayer->m_pActiveItem->Deploy( );
				m_pPlayer->m_pActiveItem->UpdateItemInfo( );
			}
		}
	}
	m_fSequenceLoops = FALSE;
	pev->sequence = LookupSequence("player_inc");
	pev->body = m_bDone ? 1:0;

	pev->nextthink = gpGlobals->time + 1;
}

void cPEHackerTerminal::Precache( )
{
	PRECACHE_MODEL( "models/hacker/terminal.mdl" );
}

void EXPORT cPEHackerTerminal::Touch( CBaseEntity *pOther )
{
	CBasePlayer* pl;
	
	Think( );

	if( !pOther )
		return;
	if( !pOther->IsPlayer( ) )
		return;
	if( ( m_iTerminal == 0 ) && ((cPEHacking*)g_pGameRules)->m_iHacked1 )
		return;
	if( ( m_iTerminal == 1 ) && ((cPEHacking*)g_pGameRules)->m_iHacked2 )
		return;

	m_fSequenceLoops = FALSE;
	pev->sequence = LookupSequence("player_gone");


//	if( !m_bReached )
//	{
		pl = (CBasePlayer*)pOther;
		if( !pl || !pl->IsPlayer( ) )
			return;

	if( !m_bReached )
	{
		m_pPlayer = pl;
	}
	else
	{
		if( m_pPlayer != (CBasePlayer*)pOther )
			return;
	}
	if( m_bReached && ( pl->pev->button & ( IN_USE ) ) )
	{
		if( gpGlobals->time > m_fHackEnd )
		{
			pl->m_iHideHUD &= ~HIDEHUD_WEAPONS;
			CountDown( pl, 0 ); // HACKHACK oder so :) der Countdown ist sonst zu früh fertig, whyever
			if( m_pWeapon )
			{
				pl->m_pActiveItem = m_pWeapon;
				pl->m_pActiveItem->Deploy( );
				pl->m_pActiveItem->UpdateItemInfo( );
			}
			if( pl->m_iTeam == 2 )
			{
				((cPEHacking*)g_pGameRules)->HackedATerminal( m_iTerminal, m_sTerminal );
				pl->BonusPoint( TASK_HACK );
				m_bDone = true;
			}
			else
			{
				((cPEHacking*)g_pGameRules)->UnhackedATerminal( m_iTerminal, m_sTerminal );
				pl->BonusPoint( TASK_HACK );
				m_bDone = false;
			}
		}
	}
	else if( !m_bDone && ( pl->m_iTeam == 2 ) && pl->m_bIsSpecial && ( pl->m_afButtonPressed & ( IN_USE ) ) )
	{
		if( !m_bReached )
		{
			//ALERT( at_console, "term %d team %d speci %d\n", m_iTerminal, pl->m_iTeam, pl->m_bIsSpecial );
			EMIT_SOUND_DYN ( edict(), CHAN_VOICE, "misc/keyboard.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			
			m_fHackEnd = gpGlobals->time + HACK_TIME;
			CountDown( pl, HACK_TIME );
			m_bReached = TRUE;
			if( pl->m_pActiveItem )
				pl->m_pActiveItem->Holster( );
			m_pWeapon = pl->m_pActiveItem;
			pl->m_pActiveItem = NULL;
			pl->pev->viewmodel = 0;
			pl->pev->weaponmodel = 0;
			pl->m_iHideHUD |= HIDEHUD_WEAPONS;
		}
	}
	else if( m_bDone && ( pl->m_iTeam == 1 ) && ( pl->m_afButtonPressed & ( IN_USE ) ) )
	{
		if( ( m_iTerminal == 0 ) && ((cPEHacking*)g_pGameRules)->m_iHacked1 )
			return;
		if( ( m_iTerminal == 1 ) && ((cPEHacking*)g_pGameRules)->m_iHacked2 )
			return;

		if( !m_bReached )
		{
			EMIT_SOUND_DYN ( edict(), CHAN_VOICE, "misc/keyboard.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			
			m_fHackEnd = gpGlobals->time + HACK_TIME;
			CountDown( pl, HACK_TIME );
			m_bReached = TRUE;
			if( pl->m_pActiveItem )
				pl->m_pActiveItem->Holster( );
			m_pWeapon = pl->m_pActiveItem;
			pl->m_pActiveItem = NULL;
			pl->pev->viewmodel = 0;
			pl->pev->weaponmodel = 0;
			pl->m_iHideHUD |= HIDEHUD_WEAPONS;
		}
	}
	else
	{
		if( m_bReached )
		{
			m_pPlayer->m_iHideHUD &= ~HIDEHUD_WEAPONS;
			CountDown( m_pPlayer, 0 );
			if( m_pWeapon )
			{
				m_pPlayer->m_pActiveItem = m_pWeapon;
				m_pPlayer->m_pActiveItem->Deploy( );
				m_pPlayer->m_pActiveItem->UpdateItemInfo( );
			}
		}
		m_bReached = FALSE;
		m_fHackEnd = gpGlobals->time + 500;
		if( !m_bDone && ( pl->m_iTeam == 2 ) && pl->m_bIsSpecial )
		{
			NotifyMid( pl, NTM_HACKIT, 1 );
		}
		else if( m_bDone && ( pl->m_iTeam == 1 ) )
		{
			NotifyMid( pl, NTM_UNHACKIT, 1 );
		}
	}
	pev->nextthink = gpGlobals->time + 1;
}

void cPEHackerTerminal::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "terminal" ) )
	{
		strcpy( m_sTerminal, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
}*/

// + ste0
class CNPCCrow: public CBaseAnimating
{
	int Classify(  ) { return CLASS_NONE; };
	void EXPORT MyThink()
	{
		switch(RANDOM_LONG(0,3))
		{
		case 0:
			pev->sequence = LookupSequence("crow_anim1");
			break;
		case 1:
			pev->sequence = LookupSequence("crow_anim2");
			break;
		case 2:
			pev->sequence = LookupSequence("crow_anim3");
			break;

		}
		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;


		pev->nextthink = gpGlobals->time + RANDOM_LONG(3,5);

	}
	void EXPORT Touch( CBaseEntity *pOther )
	{
		SpawnBlood(pev->origin + Vector (0,0,2),BLOOD_COLOR_RED,40);
		pev->solid = SOLID_NOT;
		pev->effects |= EF_NODRAW;
		pev->health = 0;
	}
	void ReSpawn()
	{
		pev->solid = SOLID_BBOX;
		pev->movetype = MOVETYPE_STEP;
		SET_MODEL ( ENT(pev), "models/mapobjects/crow/crow.mdl" );
		UTIL_SetSize ( pev, Vector ( -2, -2, 0 ), Vector ( 2, 2, 4 ) );
		pev->health = pev->max_health = 1;
		pev->takedamage = DAMAGE_YES;

		pev->sequence = LookupSequence("crow_anim1");
	
		//if (m_fSequenceFinished)
		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->effects = 0;

		pev->nextthink = gpGlobals->time + RANDOM_LONG(2,4);
		SetThink(&CNPCCrow::MyThink);
		SetTouch(&CNPCCrow::Touch);
	};

	void Precache( )
	{
		PRECACHE_MODEL( "models/mapobjects/crow/crow.mdl" );
	};

	void EXPORT Spawn()
	{
    Precache( );
		pev->solid = SOLID_BBOX;
		pev->movetype = MOVETYPE_STEP;
		SET_MODEL ( ENT(pev), "models/mapobjects/crow/crow.mdl" );
		UTIL_SetSize ( pev, Vector ( -2, -2, 0 ), Vector ( 2, 2, 4 ) );
		pev->health = pev->max_health = 1;
		pev->takedamage = DAMAGE_YES;

		pev->sequence = LookupSequence("crow_anim1");
	
		//if (m_fSequenceFinished)
		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->effects = 0;

		pev->nextthink = gpGlobals->time + RANDOM_LONG(2,4);
		SetThink(&CNPCCrow::MyThink);
		SetTouch(&CNPCCrow::Touch);
	};

	virtual int	TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
	{
		if( pev->health < 1 )
			return 1;
		pev->health = 0;
		SpawnBlood(pev->origin + Vector (0,0,2),BLOOD_COLOR_RED,40);
		pev->solid = SOLID_NOT;
		pev->effects |= EF_NODRAW;
		return 1;
	}

};
LINK_ENTITY_TO_CLASS( mapobject_npc_crow, CNPCCrow );

class CNPCSlavebot: public CBaseAnimating // hat zwar nix mit der class hier zu tun aber: HEINO RULEZZZZZZ
{
	float m_fNextSound;
	void EXPORT MyThink()
	{
		switch(RANDOM_LONG(0,5))
		{
		case 0:
			pev->sequence = LookupSequence("slavebot_dance");
			break;
		case 1:
			pev->sequence = LookupSequence("slavebot_dance");
			break;
		case 2:
			pev->sequence = LookupSequence("slavebot_dance");
			break;
		case 3:
			pev->sequence = LookupSequence("slavebot_dance");
			break;
		case 4:
			pev->sequence = LookupSequence("slavebot_smoke");
			ResetSequenceInfo( );
			m_fSequenceLoops = TRUE;
			pev->framerate = 1;


			pev->nextthink = gpGlobals->time + 2;
			return;
			break;
		}
		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;


		pev->nextthink = gpGlobals->time + 2.25;

	}
	void Precache( )
	{
		PRECACHE_MODEL( "models/mapobjects/bum/slavebot.mdl" );
	}

	void EXPORT Spawn()
	{
    Precache( );
		pev->solid = SOLID_BBOX;
		pev->movetype = MOVETYPE_STEP;
		SET_MODEL ( ENT(pev), "models/mapobjects/bum/slavebot.mdl" );
		UTIL_SetSize ( pev, Vector ( -16, -16, 0 ), Vector ( 16, 16, 76 ) );
		pev->health = pev->max_health = 35;
		pev->takedamage = DAMAGE_NO;
		m_fNextSound = 0;

		pev->sequence = LookupSequence("slavebot_dance");

		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->nextthink = gpGlobals->time + 2;
		SetThink(&CNPCSlavebot::MyThink);
	};

	virtual int	TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
	{
		if( pev->health <= 0 )
			return 1;
		SpawnBlood(pev->origin + Vector (0,0,RANDOM_LONG(30,60)),BLOOD_COLOR_RED,10);
		SpawnBlood(pev->origin + Vector (0,0,RANDOM_LONG(30,60)),BLOOD_COLOR_RED,10);
		SpawnBlood(pev->origin + Vector (0,0,RANDOM_LONG(30,60)),BLOOD_COLOR_RED,10);

		pev->solid = SOLID_NOT;
		pev->sequence = LookupSequence("slavebot_smoke");
	
		ResetSequenceInfo( );
		m_fSequenceLoops = FALSE;
		pev->framerate = 1;
		pev->health = 0;
		pev->nextthink = gpGlobals->time + 2;

		return 1;
	}

};
LINK_ENTITY_TO_CLASS( mapobject_npc_slavebot, CNPCSlavebot );

class CNPCBum: public CBaseAnimating // hat zwar nix mit der class hier zu tun aber: HEINO RULEZZZZZZ
{
	int Classify(  ) { return CLASS_NONE; };
	void EXPORT MyThink()
	{
		if (pev->health > 0)
		switch(RANDOM_LONG(0,3))
		{
		case 0:
			pev->sequence = LookupSequence("bum_anim1");
			break;
		case 1:
			pev->sequence = LookupSequence("bum_anim2");
			break;
		case 2:
			pev->sequence = LookupSequence("bum_anim3");
			break;

		}
		else
		{
			pev->sequence = LookupSequence("bum_gone");


		}
		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;


		pev->nextthink = gpGlobals->time + RANDOM_LONG(6,9);

	}
	void ReSpawn()
	{
		pev->solid = SOLID_BBOX;
		pev->movetype = MOVETYPE_STEP;
		if( FClassnameIs( pev, "mapobject_npc_bum" ) )
			SET_MODEL ( ENT(pev), "models/mapobjects/bum/bum1.mdl" );
		else if( FClassnameIs( pev, "mapobject_npc_bum1" ) )
			SET_MODEL ( ENT(pev), "models/mapobjects/bum/bum1.mdl" );
		else if( FClassnameIs( pev, "mapobject_npc_bum2" ) )
			SET_MODEL ( ENT(pev), "models/mapobjects/bum/bum2.mdl" );
		else if( FClassnameIs( pev, "mapobject_npc_bum3" ) )
			SET_MODEL ( ENT(pev), "models/mapobjects/bum/bum3.mdl" );
		UTIL_SetSize ( pev, Vector ( -16, -16, 0 ), Vector ( 16, 16, 76 ) );
		pev->health = pev->max_health = 35;
		pev->takedamage = DAMAGE_YES;

		pev->sequence = LookupSequence("bum_anim1");
	
		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->nextthink = gpGlobals->time + RANDOM_LONG(6,9);
		SetThink(&CNPCBum::MyThink);
	};

	void Precache( )
	{
		PRECACHE_MODEL( "models/mapobjects/bum/bum1.mdl" );
		PRECACHE_MODEL( "models/mapobjects/bum/bum2.mdl" );
		PRECACHE_MODEL( "models/mapobjects/bum/bum3.mdl" );
	}

	void EXPORT Spawn()
	{
    Precache( );
		pev->solid = SOLID_BBOX;
		pev->movetype = MOVETYPE_STEP;
		if( FClassnameIs( pev, "mapobject_npc_bum" ) )
			SET_MODEL ( ENT(pev), "models/mapobjects/bum/bum1.mdl" );
		else if( FClassnameIs( pev, "mapobject_npc_bum1" ) )
			SET_MODEL ( ENT(pev), "models/mapobjects/bum/bum1.mdl" );
		else if( FClassnameIs( pev, "mapobject_npc_bum2" ) )
			SET_MODEL ( ENT(pev), "models/mapobjects/bum/bum2.mdl" );
		else if( FClassnameIs( pev, "mapobject_npc_bum3" ) )
			SET_MODEL ( ENT(pev), "models/mapobjects/bum/bum3.mdl" );
		UTIL_SetSize ( pev, Vector ( -16, -16, 0 ), Vector ( 16, 16, 76 ) );
		pev->health = pev->max_health = 35;
		pev->takedamage = DAMAGE_YES;

		pev->sequence = LookupSequence("bum_anim1");
	
		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->nextthink = gpGlobals->time + RANDOM_LONG(6,9);
		SetThink(&CNPCBum::MyThink);
	};

	virtual int	TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
	{
		if( pev->health <= 0 )
			return 1;
		SpawnBlood(pev->origin + Vector (0,0,RANDOM_LONG(30,60)),BLOOD_COLOR_RED,10);
		SpawnBlood(pev->origin + Vector (0,0,RANDOM_LONG(30,60)),BLOOD_COLOR_RED,10);
		SpawnBlood(pev->origin + Vector (0,0,RANDOM_LONG(30,60)),BLOOD_COLOR_RED,10);

		pev->solid = SOLID_NOT;
		// pev->effects |= EF_NODRAW;
		pev->sequence = LookupSequence("bum_die");
	
		//if (m_fSequenceFinished)
		ResetSequenceInfo( );
		m_fSequenceLoops = FALSE;
		pev->framerate = 1;
		pev->health = 0;
		pev->nextthink = gpGlobals->time + 4.67;

		return 1;
	}

};
LINK_ENTITY_TO_CLASS( mapobject_npc_bum, CNPCBum );
LINK_ENTITY_TO_CLASS( mapobject_npc_bum1, CNPCBum );
LINK_ENTITY_TO_CLASS( mapobject_npc_bum2, CNPCBum );
LINK_ENTITY_TO_CLASS( mapobject_npc_bum3, CNPCBum );
// - ste0


void Explosion( vec3_t origin, entvars_t* pev, float flDamage, float flRadius )
{
	Vector vecSpot = origin;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_SPRITE );
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z );
		WRITE_SHORT( g_sModelIndexExplode );
		WRITE_BYTE( 30 ); // scale * 10
		WRITE_BYTE( 255 ); // brightness
	MESSAGE_END();
	
	// big smoke
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( 250 ); // scale * 10
		WRITE_BYTE( 5  ); // framerate
	MESSAGE_END();

	// blast circle
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( vecSpot.x);
		WRITE_COORD( vecSpot.y);
		WRITE_COORD( vecSpot.z);
		WRITE_COORD( vecSpot.x);
		WRITE_COORD( vecSpot.y);
		WRITE_COORD( vecSpot.z + 1000 ); // reach damage radius over .2 seconds
		WRITE_SHORT( g_sModelIndexBlast );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 4 ); // life
		WRITE_BYTE( 32 );  // width
		WRITE_BYTE( 0 );   // noise
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 192 );   // r, g, b
		WRITE_BYTE( 128 ); // brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);

	RadiusDamage( vecSpot, pev, pev, flDamage, flRadius, CLASS_MACHINE, DMG_BLAST );
}
void Explosion( entvars_t* pev, float flDamage, float flRadius )
{
  	vec3_t vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
    Explosion( vecSpot, pev, flDamage, flRadius );
}
/*class CCopcar: public CBaseAnimating
{
	void EXPORT Think( )
	{
		if( pev->health > 0 )
			pev->sequence = LookupSequence("idle");
		else
			pev->sequence = LookupSequence("crash_idle");

		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->nextthink = gpGlobals->time + 5;

	}
	void ReSpawn( )
	{
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_STEP;
		SET_MODEL ( ENT(pev), "models/mapobjects/cars/copcar.mdl" );
		pev->health = pev->max_health = 500;
		pev->takedamage = DAMAGE_YES;

		pev->sequence = LookupSequence("idle");
	
		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->nextthink = gpGlobals->time + 5;
	};

	void Precache( )
	{
		PRECACHE_MODEL( "models/mapobjects/cars/copcar.mdl" );
	}

	void EXPORT Spawn( )
	{
    Precache( );
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_STEP;
		SET_MODEL ( ENT(pev), "models/mapobjects/cars/copcar.mdl" );
		pev->health = pev->max_health = 500;
		pev->takedamage = DAMAGE_YES;

		pev->sequence = LookupSequence("idle");
	
		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->nextthink = gpGlobals->time + 5;
	};

	virtual int	TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
	{
		if( pev->health <= 0 )
			return 1;
		pev->sequence = LookupSequence("hit");
	
		ResetSequenceInfo( );
		m_fSequenceLoops = FALSE;
		pev->framerate = 1;
		pev->health -= flDamage;
		if( pev->health <= 0 )
		{
			pev->sequence = LookupSequence("crash");
			pev->framerate = 1;
			pev->nextthink = gpGlobals->time + 3;
			Explosion( pev, 20, 200 );

		}
		pev->nextthink = gpGlobals->time + 1;
		return 1;
	}

};
LINK_ENTITY_TO_CLASS( mapobject_copcar, CCopcar );

class CAsiancar: public CBaseAnimating
{
	void EXPORT Think( )
	{
		if( pev->health > 0 )
			pev->sequence = LookupSequence("idle");
		else
			pev->sequence = LookupSequence("crash_idle");

		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->nextthink = gpGlobals->time + 5;
	}
	void ReSpawn( )
	{
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_STEP;
		SET_MODEL ( ENT(pev), "models/mapobjects/cars/asiancar.mdl" );

		Vector mins( -48, -48, 0 ), maxs( 48, 48, 76 );
		pev->health = pev->max_health = 500;
		pev->takedamage = DAMAGE_YES;

	
		pev->sequence = LookupSequence("idle");
		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->nextthink = gpGlobals->time + 5;
	};

	void Precache( )
	{
		PRECACHE_MODEL( "models/mapobjects/cars/asiancar.mdl" );
	}

	void EXPORT Spawn( )
	{
    Precache( );
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_STEP;
		SET_MODEL ( ENT(pev), "models/mapobjects/cars/asiancar.mdl" );

		Vector mins( -48, -48, 0 ), maxs( 48, 48, 76 );
		pev->health = pev->max_health = 500;
		pev->takedamage = DAMAGE_YES;

	
		pev->sequence = LookupSequence("idle");
		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->nextthink = gpGlobals->time + 5;
	};

	virtual int	TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
	{
		if( pev->health <= 0 )
			return 1;
		pev->sequence = LookupSequence("hit");
	
		ResetSequenceInfo( );
		m_fSequenceLoops = FALSE;
		pev->framerate = 1;
		pev->health -= flDamage;
		if( pev->health <= 0 )
		{
			pev->sequence = LookupSequence("crash");
			pev->framerate = 1;
			pev->nextthink = gpGlobals->time + 3;
	
			Explosion( pev, 20, 200 );
		}
		pev->nextthink = gpGlobals->time + 1;

		return 1;
	}

};
LINK_ENTITY_TO_CLASS( mapobject_asiancar, CAsiancar );


class CRedCar: public CBaseAnimating
{
	void EXPORT Think( )
	{
		if( pev->health > 0 )
			pev->sequence = LookupSequence("idle");
		else
			pev->sequence = LookupSequence("crash_idle");

		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->nextthink = gpGlobals->time + 3;
	}
	void EXPORT Spawn( )
	{
		PRECACHE_MODEL( "models/mapobjects/cars/redcar.mdl" );
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_STEP;
		SET_MODEL ( ENT(pev), "models/mapobjects/cars/redcar.mdl" );
		pev->health = pev->max_health = 500;
		pev->takedamage = DAMAGE_YES;

		pev->sequence = LookupSequence("idle");
	
		ResetSequenceInfo( );
		m_fSequenceLoops = TRUE;
		pev->framerate = 1;

		pev->nextthink = gpGlobals->time + 3;
	};

	virtual int	TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
	{
		if( pev->health <= 0 )
			return 1;
		pev->sequence = LookupSequence("hit");
	
		ResetSequenceInfo( );
		m_fSequenceLoops = FALSE;
		pev->framerate = 1;
		pev->health -= flDamage;
		if( pev->health <= 0 )
		{
			pev->sequence = LookupSequence("crash");
			pev->framerate = 1;
			pev->nextthink = gpGlobals->time + 3;

			Explosion( pev, 20, 200 );
		}
		pev->nextthink = gpGlobals->time + 1;

		return 1;
	}

};
LINK_ENTITY_TO_CLASS( mapobject_redcar, CRedCar );*/


class CJaguar: public CBaseAnimating
{
	void Precache( )
	{
		PRECACHE_MODEL( "models/mapobjects/jaguar/jaguar.mdl" );
	};
	void Spawn( )
	{
    Precache( );
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
		SET_MODEL ( ENT(pev), "models/mapobjects/jaguar/jaguar.mdl" );
		pev->takedamage = DAMAGE_NO;
	};

};
LINK_ENTITY_TO_CLASS( mapobject_jaguar, CJaguar );

class CPERadarMark : public CBaseEntity
{
public:
	void EXPORT Spawn()
	{
		pev->classname = MAKE_STRING("pe_radar_mark");
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
	}

};
LINK_ENTITY_TO_CLASS( pe_radar_mark, CPERadarMark);

class CBBFunk : public CBaseEntity
{
public:
	void EXPORT Spawn()
	{
		pev->classname = MAKE_STRING("bb_funk");
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
	}
};
LINK_ENTITY_TO_CLASS(bb_funk,CBBFunk);

class CBBEscapeRadar : public CBaseEntity
{
public:
	void EXPORT Spawn()
	{
		pev->classname = MAKE_STRING("bb_escape_radar");
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
	}
};
LINK_ENTITY_TO_CLASS(bb_escape_radar,CBBEscapeRadar);

class CBBMapmission : public CBaseEntity
{
public:
	void EXPORT Spawn()
	{
		pev->classname = MAKE_STRING("bb_mapmission");
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
	}
};
LINK_ENTITY_TO_CLASS(bb_mapmission,CBBMapmission);

class CBBObjectives : public CBaseEntity
{
public:
	void EXPORT Spawn()
	{
		pev->classname = MAKE_STRING("bb_objectives");
		pev->solid = SOLID_NOT;
		pev->movetype = MOVETYPE_NONE;
	}
};
LINK_ENTITY_TO_CLASS(bb_objectives,CBBObjectives);


LINK_ENTITY_TO_CLASS( pe_light_ref, CPELRef);


LINK_ENTITY_TO_CLASS( pe_light, CPELight);