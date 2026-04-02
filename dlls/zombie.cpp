/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include "studio.h"
#include "game.h"
#include	"gamerules.h"
#include "pe_rules.h"
#include "pe_rules_hacker.h"
#include "player.h"

#define NORMAL 1
#define BRUTAL 10

#define SOUND_DELAY RANDOM_LONG( 15, 22 );

extern int gmsgSpray;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_AE_ATTACK_BOTH		0x03

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs
#define FRED_HEALTH 1000

class CZombie : public CBaseMonster
{
public:
  void Killed( entvars_t *pevAttacker, int iGib );
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	void EXPORT MonsterThink();
	void EXPORT SpawningThink();
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions ( void );
  virtual void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
  virtual BOOL ShouldGibMonster( int iGib );

	float m_flNextFlinch;
  float m_fNextBlood;
  
  float m_fNextSpread;
  float m_fBurning;
	int type;

	// Stuck detection & obstacle avoidance
	Vector m_vecStuckCheckPos;	// Position when stuck timer started
	float  m_flStuckStartTime;	// When the zombie first got stuck (0 = not stuck)
	int    m_iLastAvoidDir;		// Direction of current/last avoidance: 0=none, 1=right, -1=left
	float  m_flAvoidRemaining;	// Sideways distance left in current move (0 = not avoiding)
	Vector m_vecAvoidEnemyPos;	// Enemy pos when m_iLastAvoidDir was set (for 80-unit reset)

	// Small-obstacle clip-through
	float  m_flClipRemaining;	// Distance left to clip through (0 = not clipping)
	Vector m_vecClipDir;		// Direction to clip through (unit vector, 2D)

	float m_fAnimTimeout;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// Obstacle avoidance overrides
	void Move( float flInterval );
	int CheckLocalMove( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );

	// AI override - always retry chase on failure
	Schedule_t *GetScheduleOfType( int Type );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

  bool fountained;

  bool isFred;
  float dieTime;
  bool points_given;
  float nextSound;

  entvars_t *flamer;
	void EXPORT SUB_StartFadeIn()
  {
	  if (pev->rendermode == kRenderNormal)
	  {
		  pev->renderamt = 0;
		  pev->rendermode = kRenderTransTexture;
	  }

	  //pev->solid = SOLID_NOT;
	  pev->velocity = g_vecZero;
	  pev->avelocity = g_vecZero;
	  ClearBits( pev->effects, EF_NOINTERP );

	  pev->nextthink = gpGlobals->time + 0;
	  SetThink ( &CZombie::SUB_FadeIn );
  }

	void EXPORT SUB_FadeIn()
  {
		pev->nextthink = gpGlobals->time + 0.1;
		if ( pev->renderamt < 225 )
			pev->renderamt += 30;
		else
		{
			pev->renderamt = 255;
			pev->rendermode = kRenderNormal;
			SetSequenceByName("spawn");
			if (m_flFrameRate)
				pev->nextthink = gpGlobals->time + 1.5;
			SetThink( &CZombie::SpawningThink );
		}
	}
};

BOOL CZombie::ShouldGibMonster( int iGib )
{
  if( isFred || !gibcnt.value )
		return FALSE;

	if ( ( iGib == GIB_NORMAL && pev->health < GIB_HEALTH_VALUE ) || ( iGib == GIB_ALWAYS ) )
	{
		if( RANDOM_LONG(0,100) <= 50 )
		{
      CGib::SpawnRandomGibs( pev, 2, 1 );
			CGib::SpawnGib( pev, "models/gibs/upper.mdl", 1 );
      CGib::SpawnGib( pev, "models/gibs/arm1.mdl", 1 );
			SetBodygroup( 8, 1 );
			SetBodygroup( 1, 1 );
			SetBodygroup( 2, 1 );
			SetBodygroup( 3, 1 );
			SetBodygroup( 5, 1 );
			SetBodygroup( 6, 1 );
			SetBodygroup( 7, 1 );
			return FALSE;
		}
		else
			return TRUE;
	}
	return FALSE;
}

extern int gmsgScoreInfo;
void CZombie::Killed( entvars_t *pevAttacker, int iGib )
{
  CBaseEntity *ent = Instance( pevAttacker );
  int pnts = 1;
  if( type == BRUTAL )
	  pnts = 2;

  if( ent->IsPlayer( ) && !points_given )
  {
    ((cPEHacking*)g_pGameRules)->misDone[MISSION_FRAGS]++;
    if( !isFred )
    {
      points_given = true;
      pevAttacker->frags += pnts;
    }
  }
  
  if( pev->fuser4 && !points_given )
  {
    pnts = 5;
    ent->GiveExp( 300 );
    ((cPEHacking*)g_pGameRules)->misDone[MISSION_FRED]++;
    DisableAll( pev );
    points_given = true;
    pevAttacker->frags += pnts;
  }

  /*if( ent->IsPlayer( ) )
  {
    MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		  WRITE_BYTE( ENTINDEX( ent->edict( ) ) );
		  WRITE_SHORT( ent->pev->frags );
		  WRITE_SHORT( ((CBasePlayer*)ent)->m_iDeaths );
	  MESSAGE_END( );
  }*/
  CBaseMonster::Killed( pevAttacker, iGib );
  pev->fuser4 = 0;
}

void CZombie::MonsterThink( )
{
  CBaseMonster::MonsterThink( );
  if( dieTime && dieTime <= gpGlobals->time )
  {
    pev->health = 0;
    Killed( pev, 0 );
  }

  if( m_fBurning > gpGlobals->time && m_fNextSpread <= gpGlobals->time )
  {
    CBaseEntity *ent = UTIL_FindEntityInSphere( NULL, pev->origin, 128 );
    int cnt = 0;
    while( ent != NULL && cnt < 5 )
		{
      if( ent->pev->takedamage && !FClassnameIs( ent->pev, "player" ) )
      {
        ent->TakeDamage( flamer, flamer, ent == this ? 2 : 1, 0 );
        cnt++;
      }
			ent = UTIL_FindEntityInSphere( ent, pev->origin, 128 );
		}
    m_fNextSpread = gpGlobals->time + 0.5;
  }
}

void CZombie :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
  if( pevAttacker && pevAttacker->team == 2 && !FClassnameIs( pevAttacker, "monster_zombie" ) )
    return;
	
  if( !ptr )
    return;
	CBaseEntity *be = CBaseEntity::Instance(pevAttacker);
  if( be && be->IsPlayer( ) )
  {
    CBasePlayer *plr = (CBasePlayer*)be;
    flDamage *= ( PLR_DMG(plr) / 100.0f );
  }
  if( pev->takedamage )
	{
		m_LastHitGroup = ptr->iHitgroup;

//    ALERT( at_console, "hb: %d\n", ptr->iHitgroup );
		switch ( ptr->iHitgroup )
		{
		case HITGROUP_GENERIC:
			break;
		case HITGROUP_HEAD:
			if( isFred )
			{
		    flDamage *= 0.2;
			  if( pev->health < ( FRED_HEALTH / 10 ) )
				  SetBodygroup( 5, 3 );
			  else if( pev->health < ( FRED_HEALTH / 5 ) )
				  SetBodygroup( 5, 2 );
			  else if( pev->health < ( FRED_HEALTH / 2 ) )
				  SetBodygroup( 5, 1 );
			}
			else
			{
      if( flDamage >= 100 )
		    SetBodygroup( 6, 1 );
      flDamage *= 0.3;

      if( !GetBodygroup( 6 ) )
      {
        SetBodygroup( 6, 1 );
        CGib::SpawnGib( pev, "models/gibs/head1.mdl", 1 );
      }
      else if( !GetBodygroup( 7 ) && ( pev->health - flDamage ) <= 120 )
      {
        SetBodygroup( 7, 1 );
        //CGib::SpawnGib( pev, "models/gibs/head1.mdl", 1 );
        flDamage = pev->health + 20;
        //pev->health = 0;
      }
      else if( !GetBodygroup( 5 ) && pev->health <= 120 )
      {
        SetBodygroup( 5, 1 );
        //CGib::SpawnGib( pev, "models/gibs/head3.mdl", 1 );
      }
			}
			break;
		case HITGROUP_CHEST:
      if( GetBodygroup( 7 ) + GetBodygroup( 2 ) + GetBodygroup( 1 ) < 3 )
      {
        flDamage *= 0.24;//0.12//gSkillData.monChest;
        break;
      }
      if( !GetBodygroup( 8 ) )
      {
			if( isFred )
			{
			}
			else
			{
        CGib::SpawnGib( pev, "models/gibs/upper.mdl", 1 );
	      SetBodygroup( 8, 1 );
        SetBodygroup( 1, 1 );
        SetBodygroup( 2, 1 );
        SetBodygroup( 3, 1 );
        SetBodygroup( 5, 1 );
        SetBodygroup( 6, 1 );
        SetBodygroup( 7, 1 );
        SetBodygroup( 8, 1 );
        flDamage = 100;
			}
      }
			break;
		case HITGROUP_STOMACH:
      if( !GetBodygroup( 4 ) )
      {
        //CGib::SpawnRandomGibs( pev, 3, 1 );
        SetBodygroup( 4, 1 );
      }
			flDamage *= 0.16;//0.08;//gSkillData.monStomach;
			break;
		case HITGROUP_LEFTARM:
      if( !GetBodygroup( 2 ) )
      {
        SetBodygroup( 2, 1 );
        CGib::SpawnGib( pev, "models/gibs/arm1.mdl", 1 );
      }
      else if( !GetBodygroup( 3 ) )
      {
        SetBodygroup( 3, 1 );
        CGib::SpawnRandomGibs( pev, 1, 1 );
      }
		  flDamage *= 0.05;//*= gSkillData.monArm;
      break;
		case HITGROUP_RIGHTARM:
      if( !GetBodygroup( 1 ) )
      {
        CGib::SpawnGib( pev, "models/gibs/arm2.mdl", 1 );
        SetBodygroup( 1, 1 );
      }
			flDamage *= 0.05;//*= gSkillData.monArm;
			break;
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			flDamage *= 0.06;//gSkillData.monLeg;
			break;
		default:
			break;
		}
		if( !FClassnameIs( pevAttacker, "player" ) )
			flDamage = max( 30.0f, flDamage );

		//SpawnBlood(ptr->vecEndPos, BloodColor(), 50);// a little surface blood.
		TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
    
    //pev->velocity = pev->velocity + vecDir.Normalize( ) * DamageForce( flDamage );
    if( isFred )
      fountained = true;

    if( !fountained && ptr->iHitgroup == HITGROUP_HEAD && ( pev->health - flDamage ) <= 0 )
    {
      /*vec3_t org = ptr->vecEndPos + 5 * vecDir.Normalize( );
      vec3_t angl, forw, rig, up;
      GetBonePosition( (int)showdmg.value, org, angl );
      UTIL_MakeVectorsPrivate( angl, forw, rig, up );
		  MESSAGE_BEGIN( MSG_ALL, gmsgSpray, NULL );
			  WRITE_STRING( "partsys/bloodhead.cfg" );
        WRITE_COORD( org.x ); // origin
			  WRITE_COORD( org.y );
			  WRITE_COORD( org.z );
        //org = org.x * forw + org.y * rig + org.z * up;
        WRITE_COORD( org.x ); // origin
			  WRITE_COORD( org.y );
			  WRITE_COORD( org.z );
 			  WRITE_BYTE( 0 );
			  WRITE_COORD( -1 );
     MESSAGE_END();*/
      MESSAGE_BEGIN( MSG_PVS, gmsgSpray, pev->origin );
			  WRITE_BYTE( SPRAY_BLOODHEAD );
        WRITE_COORD( pev->origin.x ); // origin
			  WRITE_COORD( pev->origin.y );
			  WRITE_COORD( pev->origin.z + 28 + 37 );
        WRITE_COORD( 0 ); // origin
			  WRITE_COORD( 0 );
			  WRITE_COORD( 1 );
			  WRITE_COORD( 0 );
	      WRITE_COORD( 1 );
	      WRITE_COORD( 1 );
      MESSAGE_END();
      fountained = true;
    }
    else if( m_fNextBlood <= gpGlobals->time )
    {
      vec3_t org = ptr->vecEndPos + 5 * vecDir.Normalize( );
		  MESSAGE_BEGIN( MSG_PVS, gmsgSpray, pev->origin );
			  WRITE_BYTE( SPRAY_BLOOD );
        WRITE_COORD( org.x ); // origin
			  WRITE_COORD( org.y );
			  WRITE_COORD( org.z );
			  WRITE_COORD( vecDir.x ); // dir
			  WRITE_COORD( vecDir.y );
			  WRITE_COORD( vecDir.z );
			  WRITE_COORD( 0 );
	      WRITE_COORD( 1 );
	      WRITE_COORD( 1 );
      MESSAGE_END();
      m_fNextBlood = gpGlobals->time + 0.5;
    }

	}
}

LINK_ENTITY_TO_CLASS( monster_zombie, CZombie );

const char *CZombie::pAttackHitSounds[] = 
{
	"zombie/hit.wav",
	"zombie/hit2.wav",
	"zombie/hit3.wav",
  "zombie/hit4.wav",
};

const char *CZombie::pAttackMissSounds[] = 
{
	"zombie/hit.wav",
	"zombie/hit2.wav",
	"zombie/hit3.wav",
  "zombie/hit4.wav",
};

const char *CZombie::pAttackSounds[] = 
{
	"zombie/hit.wav",
	"zombie/hit2.wav",
	"zombie/hit3.wav",
  "zombie/hit4.wav",
};

const char *CZombie::pIdleSounds[] = 
{
	"zombie/hummmmm4.wav",
	"zombie/hummmmmmm2.wav",
  "zombie/hunger.wav",
	"zombie/hummmmmmm3.wav",
	"zombie/huummmmm.wav",
  "zombie/hunger2.wav",
  "zombie/huummmmm.wav",
};

const char *CZombie::pAlertSounds[] = 
{
	"zombie/braaains.wav",
	"zombie/feeed.wav",
  "zombie/hunger.wav",
  "zombie/flesh2.wav",
  "zombie/join.wav",
  "zombie/brains.wav",
  "zombie/hunger2.wav",
  "zombie/flesh.wav",
  "zombie/hungers.wav",
  "zombie/mustfeed.wav",
	"zombie/brain.wav",
  "zombie/join2.wav",
  "zombie/hummmmm4.wav",
  "zombie/hunger2s.wav",
  "zombie/brains2.wav",
  "zombie/huummmmm.wav",
};

const char *CZombie::pPainSounds[] = 
{
	"zombie/harrrrr.wav",
  "zombie/hummm5.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CZombie :: Classify ( void )
{
	return	CLASS_ZOMBIE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CZombie :: SetYawSpeed ( void )
{
	int ys;

	ys = zombie_yawspeed.value ? (int)zombie_yawspeed.value : 10;

#if 0
	switch ( m_Activity )
	{
	}
#endif

	pev->yaw_speed = ys;
}

int CZombie :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 30% damage from bullets
	/*if ( bitsDamageType == DMG_BULLET )
  {
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage ) * 5;
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}*/

	// HACK HACK -- until we fix this.
  if( FBitSet( bitsDamageType, DMG_BURN ) )
  {
    //AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
    if( m_fBurning <= gpGlobals->time )
    {
      m_fBurning = gpGlobals->time + 3.8;
      MESSAGE_BEGIN( MSG_PVS, gmsgSpray, pev->origin );
			  WRITE_BYTE( SPRAY_BURN );
        WRITE_COORD( 0 ); // origin
			  WRITE_COORD( 0 );
			  WRITE_COORD( 0 );
        WRITE_COORD( 0 ); // origin
			  WRITE_COORD( 0 );
			  WRITE_COORD( 1 );
        WRITE_COORD( entindex( ) );
	      WRITE_COORD( 1 );
	      WRITE_COORD( 1 );
      MESSAGE_END();
      flamer = pevAttacker;
    }
  }

	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CZombie :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CZombie :: AlertSound( void )
{
  if( nextSound > gpGlobals->time )
    return;
  nextSound = gpGlobals->time + SOUND_DELAY;
  /*if( RANDOM_LONG( 0, 100 ) < 30 )
    return;*/
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CZombie :: IdleSound( void )
{
  if( nextSound > gpGlobals->time )
    return;
  nextSound = gpGlobals->time + SOUND_DELAY;
	int pitch = 100 + RANDOM_LONG(-5,5);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CZombie :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombie :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
  float dmg;
  
  if( isFred )
  {
    dmg = 80;
  }
  else
  {
    if( type == BRUTAL )
	    dmg = 40;
    else
	    dmg = 25;
    if( GetBodygroup( 1 ) )
      dmg /= 1.6;
    if( GetBodygroup( 2 ) )
      dmg /= 1.4;
    if( GetBodygroup( 3 ) )
      dmg /= 1.2;
  }
	if( !diff.value )
		diff.value = 0.01;
  dmg *= diff.value;
	switch( pEvent->event )
	{
		case ZOMBIE_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash right!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 96, dmg, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
          pHurt->pev->punchangle.z = -18 * ( GetBodygroup( 1 ) ? 0.5 : 1 ) * ( GetBodygroup( 2 ) ? 0.3 : 1 );
					pHurt->pev->punchangle.x = 5 * ( GetBodygroup( 1 ) ? 0.5 : 1 ) * ( GetBodygroup( 2 ) ? 0.3 : 1 );
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case ZOMBIE_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash left!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 96, dmg, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 18 * ( GetBodygroup( 1 ) ? 0.5 : 1 ) * ( GetBodygroup( 2 ) ? 0.3 : 1 );
					pHurt->pev->punchangle.x = 5 * ( GetBodygroup( 1 ) ? 0.5 : 1 ) * ( GetBodygroup( 2 ) ? 0.3 : 1 );
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case ZOMBIE_AE_ATTACK_BOTH:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 96, dmg, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = 5 * ( GetBodygroup( 1 ) ? 0.5 : 1 ) * ( GetBodygroup( 2 ) ? 0.3 : 1 );
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CZombie :: Spawn()
{
  //Precache( );
	m_fBurning = 0;

  if( pev->fuser4 )
    isFred = true;
  else
    isFred = false;

	if( RANDOM_LONG( 1, 10 ) < ((cPEHacking*)g_pGameRules)->m_iClients )
		type = BRUTAL;
	else
		type = NORMAL;
  points_given = false;
  pev->velocity = Vector( 0, 0, 0 );
  pev->avelocity = g_vecZero;

  if( isFred )
  {
    SetBits( pev->spawnflags, SF_MONSTER_FADECORPSE );
    SET_MODEL(ENT(pev), "models/fred.mdl");
    UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN * 2, VEC_HUMAN_HULL_MAX * 2 );
  }
  else
  {
    if( zombie_lifetime.value > 0 )
      dieTime = gpGlobals->time + RANDOM_LONG( (int)(zombie_lifetime.value * 0.75), (int)zombie_lifetime.value );
    else
      dieTime = 0;
    char model[32];
    int nr = RANDOM_LONG( 1, 6 );
    /*if( nr == 7 )
      nr = RANDOM_LONG( 1, 6 );*/                         //1 2 3 4 5 6
    sprintf( model, "models/zombie%d.mdl", nr % 3 + 1 );  //1 2 3 1 2 3
    pev->skin = nr % 2;                                   //1 0 1 0 1 0
    /*switch( RANDOM_LONG( 1, 4 ) )
    {
    case 1:
      SET_MODEL(ENT(pev), "models/zombie.mdl");
      break;
    case 2:
      SET_MODEL(ENT(pev), "models/zombie2.mdl");
      pev->skin = RANDOM_LONG( 0, 99 ) % 3;
      break;
    case 3:
      SET_MODEL(ENT(pev), "models/zombie3.mdl");
      pev->skin = RANDOM_LONG( 0, 100 ) % 4;
      if( pev->skin == 3 )
        pev->skin = ( RANDOM_LONG( 0, 100 ) < 65 ? RANDOM_LONG( 0, 100 ) % 2 : 3 );
      break;
    case 4:
      SET_MODEL(ENT(pev), "models/zombie4.mdl");
      pev->skin = RANDOM_LONG( 0, 99 ) % 3;
      break;
    }*/
    SET_MODEL( ENT(pev), model );

    switch( RANDOM_LONG( 1, 8 ) )
    {
    case 1:
      SetBodygroup( 1, 1 );
      break;
    case 2:
      SetBodygroup( 2, 1 );
      break;
    case 3:
      SetBodygroup( 4, 1 );
      break;
    default:
      break;
    }
 	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
 }

  m_fNextBlood = 0;
  m_fBurning = 0;

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= isFred ? MOVETYPE_TOSS : MOVETYPE_NONE;
	m_bloodColor		= BLOOD_COLOR_RED;
  if( isFred )
    pev->health = FRED_HEALTH * max( ((cPEHacking*)g_pGameRules)->m_iClients, 1 );
  else
    pev->health			= ( type == BRUTAL ? RANDOM_LONG( 170, 210 ) : RANDOM_LONG( 120, 160 ) );//gSkillData.zombieHealth;
	if( !diff.value )
		diff.value = 0.01;
  pev->health *= diff.value;
	pev->renderamt = 0;
	pev->rendermode = kRenderTransTexture;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE;//0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP | bits_CAP_CLIMB | bits_CAP_SQUAD | bits_CAP_HEAR | bits_CAP_DUCK | bits_CAP_JUMP;
  pev->team = 2;
  fountained = false;
  flamer = NULL;
  SetBits( pev->flags, FL_MONSTER );
  ClearBits( pev->flags, FL_DORMANT );
  ClearBits( pev->effects, EF_NODRAW );
  SetBits( pev->effects, EF_NOINTERP );
  for( int i = 0; i < 9; i++ )
    SetBodygroup( i, 0 );

  SetSequenceByName ( "spawnfade" );
  SetThink( &CZombie::SUB_StartFadeIn );
  pev->nextthink = gpGlobals->time + 0.1;

  nextSound = gpGlobals->time + SOUND_DELAY;

  m_flLastYawTime = 0;
  m_vecStuckCheckPos = pev->origin;
  m_flStuckStartTime = 0;
  m_iLastAvoidDir = 0;
  m_flAvoidRemaining = 0;
  m_vecAvoidEnemyPos = Vector( 0, 0, 0 );
  m_flClipRemaining = 0;
  m_vecClipDir = Vector( 0, 0, 0 );
  m_fAnimTimeout = gpGlobals->time + 4;

  /*MESSAGE_BEGIN( MSG_PVS, gmsgSpray, pev->origin );
		WRITE_BYTE( SPRAY_BURN );
    WRITE_COORD( pev->origin.x ); // origin
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 28 + 37 );
    WRITE_COORD( 0 ); // origin
		WRITE_COORD( 0 );
		WRITE_COORD( 1 );
		WRITE_COORD( 0 );
	  WRITE_COORD( 1 );
	  WRITE_COORD( 1 );
  MESSAGE_END();*/
}

void CZombie::SpawningThink( )
{
	if( !m_fSequenceFinished && m_flFrameRate > 0 && m_fAnimTimeout > gpGlobals->time )
	{
		StudioFrameAdvance();
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	// Clear area before materializing
	Vector mins = pev->origin + pev->mins;
	Vector maxs = pev->origin + pev->maxs;
	CBaseEntity *pList[32];
	int count = UTIL_EntitiesInBox( pList, 32, mins, maxs, FL_CLIENT | FL_MONSTER );
	if( count )
	{
		bool damaged = false;
		for( int i = 0; i < count; i++ )
		{
			CBaseEntity *pEnt = pList[i];
			if( !pEnt || !pEnt->IsAlive() || pEnt == this )
				continue;

			// ~10 damage per second (called every 0.1s)
			pEnt->TakeDamage(VARS(INDEXENT(0)), VARS(INDEXENT(0)), 1, DMG_GENERIC );
			damaged = true;
		}
		if (damaged)
		{
			pev->nextthink = gpGlobals->time + 0.1;
			return;
		}
	}

	pev->movetype = MOVETYPE_STEP;
 	MonsterInit();
}

//=========================================================
// Move - zombie override with obstacle avoidance.
//
// When stuck for 1s, tries clip-through first (small
// obstacles the head can see over), then random sideway
// moves.
//=========================================================
void CZombie::Move( float flInterval )
{
	// Old behavior: just use base class movement
	if ( zombie_behavior.value == 0 )
	{
		CBaseMonster::Move( flInterval );
		return;
	}

	// --- Executing a clip (obstacle or ledge) ---
	// Slides the zombie forward at walk speed, bypassing engine
	// collision. FL_ONGROUND is cleared so gravity applies if the
	// zombie ends up over empty air (ledge drop).
	if ( m_flClipRemaining > 0 )
	{
		if ( !FRouteClear() && m_movementGoal != MOVEGOAL_NONE )
		{
			MakeIdealYaw( m_Route[ m_iRouteIndex ].vecLocation );
			ChangeYaw( pev->yaw_speed );
		}
		if ( m_IdealActivity != m_movementActivity )
			m_IdealActivity = m_movementActivity;

		float flDist = m_flGroundSpeed * pev->framerate * flInterval;
		if ( flDist > m_flClipRemaining )
			flDist = m_flClipRemaining;

		UTIL_SetOrigin( pev, pev->origin + m_vecClipDir * flDist );
		pev->flags &= ~FL_ONGROUND;
		m_flClipRemaining -= flDist;

		if ( m_flClipRemaining <= 0 )
		{
			ALERT( at_aiconsole, "zombie clip: finished at (%.0f %.0f %.0f)\n", pev->origin.x, pev->origin.y, pev->origin.z );
			m_flClipRemaining = 0;
			m_vecStuckCheckPos = pev->origin;
			m_flStuckStartTime = 0;
			if ( m_hEnemy != NULL )
				m_vecEnemyLKP = m_hEnemy->pev->origin;
			FRefreshRoute();
		}
		return;
	}

	// --- Executing a sideway move ---
	// Slides the zombie perpendicular to its goal direction to get
	// around an obstacle. Uses engine movement (UTIL_MoveToOrigin)
	// so collision still applies. Aborts early if the path to the
	// enemy becomes clear. Direction alternates each attempt.
	if ( m_flAvoidRemaining > 0 )
	{
		if ( FRouteClear() || m_movementGoal == MOVEGOAL_NONE )
		{
			ALERT( at_aiconsole, "zombie sidestep: aborted (no route)\n" );
			m_flAvoidRemaining = 0;
			CBaseMonster::Move( flInterval );
			return;
		}

		Vector vecToGoal = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin );
		vecToGoal.z = 0;
		float flGoalDist = vecToGoal.Length();
		if ( flGoalDist < 1.0 )
		{
			m_flAvoidRemaining = 0;
			CBaseMonster::Move( flInterval );
			return;
		}
		vecToGoal = vecToGoal / flGoalDist;

		Vector vecUp( 0, 0, 1 );
		Vector vecPerp = CrossProduct( vecToGoal, vecUp );
		Vector vecSideDir = vecPerp * (float)m_iLastAvoidDir;

		MakeIdealYaw( m_Route[ m_iRouteIndex ].vecLocation );
		ChangeYaw( pev->yaw_speed );

		if ( m_IdealActivity != m_movementActivity )
			m_IdealActivity = m_movementActivity;

		float flDist = m_flGroundSpeed * pev->framerate * flInterval;
		Vector vecSideTarget = pev->origin + vecSideDir * 64;

		float flTotal = flDist;
		float flStep;
		while ( flTotal > 0.001 )
		{
			flStep = min( 16.0f, flTotal );
			UTIL_MoveToOrigin( ENT( pev ), vecSideTarget, flStep, MOVE_NORMAL );
			flTotal -= flStep;
		}

		m_flAvoidRemaining -= flDist;

		/*
		// Abort early if path to enemy is now clear
		if (m_hEnemy != NULL)
		{
			TraceResult trFwd;
			UTIL_TraceLine( pev->origin, m_hEnemy->pev->origin, dont_ignore_monsters, ENT( pev ), &trFwd );
			if ( trFwd.flFraction > 0.9 )
			{
				ALERT( at_aiconsole, "zombie sidestep: path clear, aborting early (%.0f remaining)\n", m_flAvoidRemaining );
				m_flAvoidRemaining = 0;
			}
		}
		*/

		if ( m_flAvoidRemaining <= 0 )
		{
			ALERT( at_aiconsole, "zombie sidestep: finished at (%.0f %.0f %.0f)\n", pev->origin.x, pev->origin.y, pev->origin.z );
			m_flAvoidRemaining = 0;
			m_vecStuckCheckPos = pev->origin;
			m_flStuckStartTime = 0;

			if ( m_hEnemy != NULL )
				m_vecEnemyLKP = m_hEnemy->pev->origin;
			FRefreshRoute();
		}

		return;
	}

	// --- Normal movement and stuck detection ---
	// Each frame we try normal base class movement. If the zombie
	// hasn't moved more than 16 units from its stuck checkpoint,
	// a timer starts. After 0.5s of being stuck we stop pushing
	// into the wall (prevents oscillation). After 1.0s we attempt
	// a clip-through or sideway move.

	// Reset avoidance direction if enemy moved significantly
	if ( m_iLastAvoidDir != 0 && m_hEnemy != NULL )
	{
		if ( ( m_hEnemy->pev->origin - m_vecAvoidEnemyPos ).Length2D() > 80 )
			m_iLastAvoidDir = 0;
	}

	// After 0.5s stuck, stop pushing into the wall
	if ( m_flStuckStartTime > 0 && gpGlobals->time - m_flStuckStartTime > 0.5 )
	{
		if ( !FRouteClear() )
		{
			MakeIdealYaw( m_Route[ m_iRouteIndex ].vecLocation );
			ChangeYaw( pev->yaw_speed );

			if ( m_IdealActivity != m_movementActivity )
				m_IdealActivity = m_movementActivity;
		}
	}
	else
	{
		CBaseMonster::Move( flInterval );
	}

	if ( m_movementGoal == MOVEGOAL_NONE || FRouteClear() )
	{
		m_flStuckStartTime = 0;
		return;
	}

	// Check if we've moved enough to reset the stuck timer
	float flDistFromStuckPos = ( pev->origin - m_vecStuckCheckPos ).Length2D();

	if ( flDistFromStuckPos > 16.0 )
	{
		m_vecStuckCheckPos = pev->origin;
		m_flStuckStartTime = 0;
		return;
	}

	if ( m_flStuckStartTime == 0 )
	{
		m_flStuckStartTime = gpGlobals->time;
		m_vecStuckCheckPos = pev->origin;
		return;
	}

	if ( gpGlobals->time - m_flStuckStartTime < 1.0 )
		return;

	// --- Stuck for 1s: try clip/ledge, then sideway move ---
	ALERT( at_aiconsole, "zombie stuck: 1s at (%.0f %.0f %.0f)\n", pev->origin.x, pev->origin.y, pev->origin.z );

	Vector vecToGoal = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin );
	vecToGoal.z = 0;
	float flGoalDist = vecToGoal.Length();
	if ( flGoalDist < 1.0 )
		return;
	vecToGoal = vecToGoal / flGoalDist;

	// --- Small-obstacle clip-through / ledge drop (behavior >= 2) ---
	// We first check if the path is clear at about head height, then
	// we do a reverse trace to see if there is an obstacle thin enough
	// to clip through. If no obstacle is found we assume it's a ledge.
	// In both cases we then check for a reasonable drop after the
	// clip (configurable height).
	if ( zombie_behavior.value >= 2 && !isFred ) do
	{
		float flHalfZ = 36;	// human_hull half-height
		Vector vecEye = pev->origin + Vector( 0, 0, pev->size.z * 0.75f );

		TraceResult trHead;
		UTIL_TraceLine( vecEye, vecEye + vecToGoal * 80, ignore_monsters, ENT( pev ), &trHead );

		// Can't see past the obstacle
		if ( trHead.flFraction != 1.0 || trHead.fStartSolid || trHead.fAllSolid )
		{
			ALERT( at_aiconsole, "zombie clip: head blocked (frac %.2f)\n", trHead.flFraction );
			break;
		}

		Vector vecFar  = pev->origin + vecToGoal * 45 + Vector( 0, 0, flHalfZ);
		Vector vecNear = pev->origin + Vector( 0, 0, flHalfZ);

		TraceResult trRev;
		UTIL_TraceHull( vecFar, vecNear, dont_ignore_monsters, human_hull, ENT( pev ), &trRev );

		// Reverse trace is stuck in a solid
		if ( trRev.fStartSolid || trRev.fAllSolid )
		{
			ALERT( at_aiconsole, "zombie clip: reverse trace stuck in solid\n" );
			break;
		}

		Vector vecExit;
		bool bLedge = false;

		if ( trRev.flFraction < 1.0 )
		{
			// Found obstacle and valid exit position
			vecExit = trRev.vecEndPos;
		}
		else
		{
			// No obstacle, move to coyote position
			vecExit = pev->origin + vecToGoal * 45 + Vector( 0, 0, flHalfZ);
			bLedge = true;
		}

		// Check ground at the actual end of the clip path (4 units past exit)
		Vector vecClipEnd = vecExit + vecToGoal * 4;
		float flDropDist = max( 36.0f, zombie_drop_height.value );
		TraceResult trFloor;
		UTIL_TraceHull( vecClipEnd, vecClipEnd - Vector( 0, 0, flDropDist ), dont_ignore_monsters, human_hull, ENT( pev ), &trFloor );

		if ( trFloor.flFraction == 1.0 || trFloor.fAllSolid || ( bLedge && trFloor.flFraction == 0 ) )
		{
			ALERT( at_aiconsole, "zombie clip: no ground below (frac %.2f, allsolid %d, traced %.0f)\n", trFloor.flFraction, trFloor.fAllSolid, flDropDist );
			break;
		}

		// Avoid hurt triggers and other zombies (prevent stacking)
		CBaseEntity *pHit = CBaseEntity::Instance( trFloor.pHit );
		if ( pHit && ( FClassnameIs( pHit->pev, "trigger_hurt" ) ||
					   FClassnameIs( pHit->pev, "monster_zombie" ) ) )
		{
			ALERT( at_aiconsole, "zombie clip: blocked by %s below\n", STRING( pHit->pev->classname ) );
			break;
		}

		// Don't drop down if target is not below
		float flDrop = pev->origin.z - trFloor.vecEndPos.z + flHalfZ;
		if ( bLedge && flDrop > 18 && m_Route[m_iRouteIndex].vecLocation.z >= pev->origin.z )
		{
			ALERT( at_aiconsole, "zombie clip: ledge drop %.0f but goal not below (goal z %.0f, origin z %.0f)\n", flDrop, m_Route[ m_iRouteIndex ].vecLocation.z, pev->origin.z );
			break;
		}

		float flDist = ( vecClipEnd - pev->origin ).Length2D();
		ALERT( at_aiconsole, "zombie %s: dist %.0f, drop %.0f/%.0f, hit %s\n", bLedge ? "ledge" : "clip", flDist, flDrop, flDropDist, pHit ? STRING( pHit->pev->classname ) : "world" );

		m_flClipRemaining = flDist;
		m_vecClipDir = vecToGoal;
		m_vecStuckCheckPos = pev->origin;
		m_flStuckStartTime = 0;
		return;

	} while ( 0 );

	// --- Initiate sideway move ---
	// Clip/ledge failed or not available. Slide perpendicular to
	// the goal direction to get around the obstacle. Direction
	// alternates each attempt; first time prefers the side closer
	// to the enemy. Steps back 10 units first to pull off the wall.
	Vector vecUp( 0, 0, 1 );
	Vector vecPerp = CrossProduct( vecToGoal, vecUp );

	int iDir;
	if ( m_iLastAvoidDir != 0 )
	{
		iDir = -m_iLastAvoidDir;
	}
	else if ( m_hEnemy != NULL )
	{
		Vector vecToEnemy = ( m_hEnemy->pev->origin - pev->origin );
		vecToEnemy.z = 0;
		float flDot = DotProduct( vecPerp, vecToEnemy );
		iDir = ( flDot >= 0 ) ? 1 : -1;
	}
	else
	{
		iDir = RANDOM_LONG( 0, 1 ) ? 1 : -1;
	}

	m_iLastAvoidDir = iDir;

	if ( m_hEnemy != NULL )
	{
		m_vecAvoidEnemyPos = m_hEnemy->pev->origin;
		m_vecEnemyLKP = m_hEnemy->pev->origin;
	}

	Vector vecBackTarget = pev->origin - vecToGoal * 10;
	UTIL_MoveToOrigin( ENT( pev ), vecBackTarget, 10.0, MOVE_NORMAL );

	FRefreshRoute();

	m_flAvoidRemaining = (float)RANDOM_LONG( 66, 196 );
	m_vecStuckCheckPos = pev->origin;
	m_flStuckStartTime = 0;

	ALERT( at_aiconsole, "zombie sidestep: started, dir %s, dist %.0f\n", iDir > 0 ? "right" : "left", m_flAvoidRemaining );
}

//=========================================================
// CheckLocalMove - zombie override that always bypasses
// obstacle detection so route-building always succeeds.
// Actual obstacle avoidance is handled by the custom
// Move() logic which detects stuck zombies and moves
// them sideways around obstacles.
//=========================================================
int CZombie::CheckLocalMove( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	// Old behavior: no override, handled by zombie hack in CBaseMonster::CheckLocalMove
	if ( zombie_behavior.value == 0 )
		return CBaseMonster::CheckLocalMove( vecStart, vecEnd, pTarget, pflDist );

	// New behavior: always force VALID so route-building succeeds with direct paths
	int iResult = CBaseMonster::CheckLocalMove( vecStart, vecEnd, pTarget, pflDist );

	if ( iResult == LOCALMOVE_INVALID )
	{
		iResult = LOCALMOVE_VALID;
	}

	return iResult;
}

//=========================================================
// GetScheduleOfType - zombie override that prevents the
// zombie from ever idling or giving up when it has an
// enemy. On any failure, immediately retry chasing.
//=========================================================
Schedule_t *CZombie::GetScheduleOfType( int Type )
{
	// Old behavior: no override
	if ( zombie_behavior.value == 0 )
		return CBaseMonster::GetScheduleOfType( Type );

	// New behavior: never give up chasing
	switch ( Type )
	{
	case SCHED_CHASE_ENEMY_FAILED:
	case SCHED_FAIL:
		// Zombies never give up - if we have an enemy, chase again immediately
		if ( m_hEnemy != NULL )
			return CBaseMonster::GetScheduleOfType( SCHED_CHASE_ENEMY );
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie :: Precache()
{
	int i;

	PRECACHE_MODEL("models/fred.mdl");

	PRECACHE_MODEL( "models/zombie1.mdl" );
	PRECACHE_MODEL( "models/zombie2.mdl" );
	PRECACHE_MODEL( "models/zombie3.mdl" );
	/*PRECACHE_MODEL( "models/zombie4.mdl" );
	PRECACHE_MODEL( "models/zombie5.mdl" );
	PRECACHE_MODEL( "models/zombie6.mdl" );*/

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CZombie::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if (m_Activity == ACT_MELEE_ATTACK1)
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
		else
#endif			
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;
	
}
