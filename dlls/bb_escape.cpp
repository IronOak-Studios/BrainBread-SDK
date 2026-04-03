#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"pe_utils.h"
#include "studio.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"
#include "pe_rules.h"
#include "pe_rules_hacker.h"
#include "list"
using namespace std;

#include "bb_escape.h"

extern int gmsgSpray;

#define ESCAPE_SPEED 150
#define ESCAPE_ROTATE_SPEED 90
#define ESCAPE_ACCEL (150/3.0f)
#define ESCAPE_ROTATE_ACCEL (90/8.0f)

#define ESCAPE_FLY 1
#define ESCAPE_ROTATE 2
#define ESCAPE_IDLE 3

#define ESCAPE_FLY_TELEPORT 2
#define ESCAPE_FLY_DRIVE 1


LINK_ENTITY_TO_CLASS( bb_escapeair, CEscape );

static void CopyMovementProps(entvars_t* dst, const entvars_t* src)
{
  dst->velocity = src->velocity;
  dst->avelocity = src->avelocity;
  dst->origin = src->origin;
  dst->oldorigin = src->oldorigin;
  dst->angles = src->angles;
  dst->speed = src->speed;
  dst->basevelocity = src->basevelocity;
  dst->clbasevelocity = src->clbasevelocity;
}

class CMissionZone : public CBaseEntity
{
public:
  void EXPORT Touch( CBaseEntity *pOther )
  {
	  if( !pOther )
		  return;

    if( FClassnameIs( pOther->pev, "weaponbox" ) && pOther->pev->fuser4 )
    {
      ((cPEHacking*)g_pGameRules)->misDone[MISSION_OBJECT]++;
    }
    
    if( pOther->IsPlayer( ) && ((CBasePlayer*)pOther)->HasNamedPlayerItem( "bb_objective_item" ) )
    {
      ((CBasePlayer*)pOther)->DropPlayerItem( "bb_objective_item" );
       pOther->pev->frags += 5;
      ((CBasePlayer*)pOther)->GiveExp( 300 );
      ((cPEHacking*)g_pGameRules)->misDone[MISSION_OBJECT]++;
    }
  }

  void Spawn( )
  {
	  pev->solid = SOLID_TRIGGER;
	  pev->movetype = MOVETYPE_NONE;
	  pev->effects |= EF_NODRAW;
	  SET_MODEL(ENT(pev), STRING(pev->model));
  }
};
LINK_ENTITY_TO_CLASS( bb_mission_zone, CMissionZone );

extern int gmsgNotify;
void CEscapeZone::Touch( CBaseEntity *pOther )
{
	if( !pOther )
		return;
	if( pOther->IsPlayer( ) /*|| FClassnameIs( pOther->pev, "bb_escapeair" )*/ )
  {
    CBasePlayer *plr = (CBasePlayer*)pOther;
    if( FBitSet( pev->spawnflags, 1 ) && ( plr->m_iTeam == 1 && !plr->escaped ) )
    {
      plr->escaped = true;
      ((cPEHacking*)g_pGameRules)->oneEscaped = true;
		  ((cPEHacking*)g_pGameRules)->SetTeam( 0, plr, 0 );
      plr->StartObserver( );
		  plr->m_iPrevClass = 0;
      plr->m_sInfo.team = 0;
      plr->pev->frags += 15;
  		MESSAGE_BEGIN( MSG_ONE, gmsgNotify, NULL, plr->edict( ) );
			  WRITE_COORD( -1 );
			  WRITE_BYTE( 0x0018 );
		  MESSAGE_END( );
      return;
    }
    //FireTargets( STRING( pev->target ), pOther, this, USE_TOGGLE, 0 );
    entlist.remove( pOther );
    entlist.push_back( pOther );
  }
}

void CEscapeZone::Spawn( )
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SET_MODEL(ENT(pev), STRING(pev->model));
  //SetThink( Think );
  //SetTouch( Touch );
  pev->nextthink = gpGlobals->time;

  //pev->origin = pev->mins + ( pev->maxs - pev->mins ) / 2;
}

void EXPORT CEscapeZone::Think( )
{
  pev->nextthink = gpGlobals->time + 0.2;
  list<CBaseEntity*>::iterator i;
  list<CBaseEntity*> toremove;
  CBaseEntity *ent;
  for( i = entlist.begin( ); i != entlist.end( ); i++ )
  {
    ent = *i;
    if( !ent )
    {
      toremove.push_back( ent );
      continue;
    }
    if( pev->mins.x > ent->pev->origin.x ||
			pev->mins.y > ent->pev->origin.y ||
			pev->mins.z > ent->pev->origin.z ||
			pev->maxs.x < ent->pev->origin.x ||
			pev->maxs.y < ent->pev->origin.y ||
			pev->maxs.z < ent->pev->origin.z )
    {
      toremove.push_back( ent );
			continue;    
    }
  }
  for( i = toremove.begin( ); i != toremove.end( ); i++ )
  {
    entlist.remove( *i );
  }
  numents = entlist.size( );
  //ALERT( at_console, "Num ents in bb_escape: %d\n", numents );
}

LINK_ENTITY_TO_CLASS( bb_escape_final, CEscapeZone );

void CEscape::DoRadiusDamage( float amount )
{
  CBaseEntity *ent = UTIL_FindEntityInSphere( NULL, pev->origin, 200 );
	while ( ent != NULL )
	{
    if( FClassnameIs( ent->pev, "monster_zombie" ) )
      ent->TakeDamage( pev, pev, amount, DMG_CRUSH );
		ent = UTIL_FindEntityInSphere( ent, pev->origin, 200 );
	}
}

float CEscape::CalcYaw( vec3_t vec )
{
    float res = VecToYaw( vec ) - pev->angles.y;
    res = MOD( res, 360 );
	  if( res > 180 ) res -= 360;
	  if( res < -180 ) res += 360;
    return abs(res);
}

void CEscape::NextTarget( bool rescuepoint )
{
  if( m_pGoalEnt && !rescuepoint )
  {
    if ( m_pGoalEnt->pev->message )
	  {
		  FireTargets( STRING( m_pGoalEnt->pev->message ), this, this, USE_TOGGLE, 0 );
		  if( FBitSet( m_pGoalEnt->pev->spawnflags, 2 ) )
			  m_pGoalEnt->pev->message = 0;
	  }

    if ( FBitSet( m_pGoalEnt->pev->spawnflags , SF_TRAIN_WAIT_RETRIGGER ) )
      noDriveTill = -1;

    if( ((CTankWP*)m_pGoalEnt)->GetDelay( ) != 0 )
      noDriveTill = gpGlobals->time + ((CTankWP*)m_pGoalEnt)->GetDelay( );

    if( ((CTankWP*)m_pGoalEnt)->GetSpeed( ) != 0 )
      maxspeed = ((CTankWP*)m_pGoalEnt)->GetSpeed( );    
    
    if( ((CTankWP*)m_pGoalEnt)->GetMoveType( ) == 1 )
      shouldteleport = true;


  }
  if( !rescuepoint )
  {
    if( !FStringNull( pev->target ) || ( m_pGoalEnt && m_pGoalEnt->pev->target ) )
    {
      if( rescueroute && !strcmp( STRING( ( m_pGoalEnt ? m_pGoalEnt->pev->target : pev->target ) ), leavewp ) && !FStringNull( pev->message ) )
      {
        rescueroute = false;
        m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->message ) );
      }
      else
        m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( ( m_pGoalEnt ? m_pGoalEnt->pev->target : pev->target ) ) );
    }
  }
  else
  {
    if( !FStringNull( pev->message ) )
      m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->message ) );
  }

  if( m_pGoalEnt && FClassnameIs( m_pGoalEnt->pev, "bb_waypoint" ) )
	{
		UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
		m_vecGoal = gpGlobals->v_forward;
    baseact = ESCAPE_ROTATE;
    startRotate = gpGlobals->time;
	  
    m_posDesired = m_pGoalEnt->pev->origin;
    m_vecDesired = ( m_posDesired - pev->origin ).Normalize( );
    totalYaw = sqrt( ( CalcYaw( m_vecDesired ) / 2.0f )  / ( ESCAPE_ROTATE_ACCEL / 2.0f ) );
  }
  else
  {
	  m_posDesired = pev->origin;
    baseact = ESCAPE_IDLE;
  }
}

void CEscape::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}

class CHGrunt;
void CEscape::HuntThink( )
{
	CBaseEntity *pPlayer = NULL;

	pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
	// UNDONE: this needs to send different sounds to every player for multiplayer.	
	if (pPlayer)
	{

		float pitch = DotProduct( pev->velocity - pPlayer->pev->velocity, (pPlayer->pev->origin - pev->origin).Normalize() );

		pitch = (int)(100 + pitch / 50.0);

		if (pitch > 250) 
			pitch = 250;
		if (pitch < 50)
			pitch = 50;
		if (pitch == 100)
			pitch = 101;

		EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "apache/ap_rotor2.wav", 0.25, 0.3, SND_CHANGE_PITCH | SND_CHANGE_VOL, pitch);
	}

	pev->nextthink = gpGlobals->time + 0.1;

  if( deltatime )
    deltatime = gpGlobals->time - deltatime;

  m_vecDesired = ( m_posDesired - pev->origin ).Normalize( );


  if( shouldteleport )
  {
    speed = 0;
    rotspeed = 0;
    pev->velocity = Vector( 0, 0, 0 );
    pev->avelocity = Vector( 0, 0, 0 );
    UTIL_SetOrigin( pev, m_posDesired );
    shouldteleport = false;
  }
  else
  {
    Rotate( );
    Fly( );
  }

  deltatime = gpGlobals->time;

  //EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "ambience/tankidle1.wav", 0.5, 0.3, 0, 110 );

  StudioFrameAdvance( );
  vec3_t tmp = pev->angles;
	pev->angles.y -= 90;

  	Look( 4092 );
	m_hEnemy = BestVisibleEnemy( );


  if( m_hEnemy != NULL )
	{
    //ALERT( at_console, "tank enemy0r!!1: %s\n", STRING( m_hEnemy->pev->classname ) );
		if( FVisible( m_hEnemy ) )
			m_posTarget = m_hEnemy->Center( );
		else
			m_hEnemy = NULL;
	}

  if( pev->velocity.Length( ) < 0.001 )
  {
    bool found = false;
    CBaseEntity* esc = UTIL_FindEntityByClassname( NULL, "bb_escape_final" );
    ezone = NULL;
    while ( esc != NULL )
    {
      /*list<CBaseEntity*> *li = &((CEscapeZone*)esc)->entlist;
      list<CBaseEntity*>::iterator i;
      for( i = li->begin( ); i != li->end( ); i++ )
      {
        if( (*i) == esc )
        {
          found = true;
          break;
        }
      }*/
      if( ( ( esc->pev->mins + ( esc->pev->maxs - esc->pev->mins ) / 2 ) - pev->origin ).Length( ) <= 400 )
      {
        found = true;
        ezone = esc;
        break;
      }	    
      esc = UTIL_FindEntityByClassname( esc, "bb_escape_final" );
    }
    if( found && !droped )
    {		
      edict_t	*pent;
      CBaseEntity *grnt;
      TraceResult tr;

      pent = CREATE_NAMED_ENTITY( gruntname );
      if( pent )
      {
        grnt = Instance( pent );
        UTIL_SetOrigin( grnt->pev, pev->origin - Vector( 200, 200, 150 ) );
        UTIL_TraceLine( grnt->pev->origin, grnt->pev->origin - Vector( 0, 0, 600 ), ignore_monsters, grnt->edict( ), &tr );
        if( tr.flFraction != 1 )
          UTIL_SetOrigin( grnt->pev, tr.vecEndPos );
        SetBits( grnt->pev->spawnflags, SF_MONSTER_FALL_TO_GROUND );
        DispatchSpawn( ENT( grnt->pev ) );
      }
      pent = CREATE_NAMED_ENTITY( gruntname );
      if( pent )
      {
        grnt = Instance( pent );
        UTIL_SetOrigin( grnt->pev, pev->origin + Vector( 200, 200, -150 ) );
        UTIL_TraceLine( grnt->pev->origin, grnt->pev->origin - Vector( 0, 0, 600 ), ignore_monsters, grnt->edict( ), &tr );
        if( tr.flFraction != 1 )
          UTIL_SetOrigin( grnt->pev, tr.vecEndPos );
        SetBits( grnt->pev->spawnflags, SF_MONSTER_FALL_TO_GROUND );
        DispatchSpawn( ENT( grnt->pev ) );
      }
      pent = CREATE_NAMED_ENTITY( gruntname );
      if( pent )
      {
        grnt = Instance( pent );
        UTIL_SetOrigin( grnt->pev, pev->origin - Vector( 200, 0, 150 ) );
        UTIL_TraceLine( grnt->pev->origin, grnt->pev->origin - Vector( 0, 0, 600 ), ignore_monsters, grnt->edict( ), &tr );
        if( tr.flFraction != 1 )
          UTIL_SetOrigin( grnt->pev, tr.vecEndPos );
        SetBits( grnt->pev->spawnflags, SF_MONSTER_FALL_TO_GROUND );
        DispatchSpawn( ENT( grnt->pev ) );
      }
      pent = CREATE_NAMED_ENTITY( gruntname );
      if( pent )
      {
        grnt = Instance( pent );
        UTIL_SetOrigin( grnt->pev, pev->origin - Vector( 0, 200, 150 ) );
        UTIL_TraceLine( grnt->pev->origin, grnt->pev->origin - Vector( 0, 0, 600 ), ignore_monsters, grnt->edict( ), &tr );
        if( tr.flFraction != 1 )
          UTIL_SetOrigin( grnt->pev, tr.vecEndPos );
        SetBits( grnt->pev->spawnflags, SF_MONSTER_FALL_TO_GROUND );
        DispatchSpawn( ENT( grnt->pev ) );
      }

      droped = true;
    }
  }
  else
    droped = false;

	UTIL_MakeAimVectors( pev->angles );
		
	Vector posGun, angGun;
	GetAttachment( 1, posGun, angGun );

	Vector vecTarget = (m_posTarget - posGun).Normalize( );

	Vector vecOut;

	vecOut.x = DotProduct( gpGlobals->v_forward, vecTarget );
	vecOut.y = -DotProduct( gpGlobals->v_right, vecTarget );
	vecOut.z = DotProduct( gpGlobals->v_up, vecTarget );

	Vector angles = UTIL_VecToAngles (vecOut);

	angles.x = -angles.x;
	if (angles.y > 180)
		angles.y = angles.y - 360;
	if (angles.y < -180)
		angles.y = angles.y + 360;
	if (angles.x > 180)
		angles.x = angles.x - 360;
	if (angles.x < -180)
		angles.x = angles.x + 360;

	if (angles.x > m_angGun.x)
		m_angGun.x = min( angles.x, m_angGun.x + 12 );
	if (angles.x < m_angGun.x)
		m_angGun.x = max( angles.x, m_angGun.x - 12 );
	if (angles.y > m_angGun.y)
		m_angGun.y = min( angles.y, m_angGun.y + 12 );
	if (angles.y < m_angGun.y)
		m_angGun.y = max( angles.y, m_angGun.y - 12 );

	m_angGun.y = SetBoneController( 0, m_angGun.y );
	m_angGun.x = SetBoneController( 1, m_angGun.x );

	Vector posBarrel, angBarrel;
	GetAttachment( 0, posBarrel, angBarrel );
	Vector vecGun = (posBarrel - posGun).Normalize( );

	if( ( ( m_posTarget - pev->origin ).Length( ) < 1500 ) && ( m_hEnemy != NULL ) )
	{
		FireBullets( 1, posBarrel, vecTarget, VECTOR_CONE_4DEGREES, 8192, BULLET_PLAYER_MINIGUN, 1 );
    EMIT_SOUND(ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 0.3, 0.3);
	}
  pev->angles = tmp;

  if( glassEnt )
    CopyMovementProps( glassEnt->pev, pev );

  if( part2 )
    CopyMovementProps( part2->pev, pev );


  if( noDriveTill == -1 )
    return;

	float flLength = (pev->origin - m_posDesired).Length();
	
  if( m_pGoalEnt )
	{
		if( flLength < max( ( 0.5f * ESCAPE_ACCEL * ( speed / ESCAPE_ACCEL ) * ( speed / ESCAPE_ACCEL ) ), 64.0f ) )
      NextTarget( );
	}
  else if( m_pGoalEnt == NULL && !FStringNull(pev->target) )
  {
    NextTarget( );
  }
  else
	{
	  m_posDesired = pev->origin;
    m_vecDesired = pev->angles;
    baseact = ESCAPE_IDLE;
  }
}

void CEscape::Rotate(  )
{
  float yaw = CalcYaw( m_vecDesired );

  if( yaw <= 2 )
  {
    pev->avelocity.y = 0;
    rotspeed = 0;
    reachedrotspeed = 0;
    shoulddrive = true;
    startRotate = 0;
    return;
  }
  else if( startRotate )
    shoulddrive = false;
  else if( yaw <= 10 )
    return;

  UTIL_MakeAimVectors( pev->angles );
	float flSide = DotProduct( m_vecDesired, gpGlobals->v_right );
  
  if( yaw <= 40 && reachedrotspeed >= 20 )
    rotspeed = ( flSide < 0 ? 1 : -1 ) * reachedrotspeed * yaw / 40;
  else if( abs( rotspeed ) < maxrotspeed )
    rotspeed += ( flSide < 0 ? 1 : -1 ) * ESCAPE_ROTATE_ACCEL * deltatime;

  if( reachedrotspeed < abs( rotspeed ) )
    reachedrotspeed = abs( rotspeed );
  
	pev->avelocity.y = rotspeed;

  DoRadiusDamage( pev->velocity.Length( ) + 2 );
  //ALERT( at_console, "rotspeed: %f, ht: %f, ct: %f, time: %f\n", rotspeed, totalYaw, cur, tm );
}
extern void VectorInverse (vec3_t v);
void CEscape::Fly(  )
{
	UTIL_MakeVectors( pev->angles );
  float flDist = DotProduct( m_posDesired - pev->origin, gpGlobals->v_forward );

  if( pev->fuser4 && noDriveTill && noDriveTill <= gpGlobals->time && noDriveTill > -1 )
  {   
    ((cPEHacking*)g_pGameRules)->heliactivated = false;
    EnableAll( "bb_funk" );
    DisableAll( "bb_escape_radar" );
    DisableAll( pev );
  }

  if( shoulddrive && noDriveTill <= gpGlobals->time && noDriveTill > -1 && m_pGoalEnt )
  {
      noDriveTill = 0;
   		curDest = m_posDesired;
  }

  if( shoulddrive && noDriveTill <= gpGlobals->time && noDriveTill > -1 )
  {
    noDriveTill = 0;
    speed += ESCAPE_ACCEL * deltatime;
  }
  else
    speed -= ESCAPE_ACCEL * deltatime;
  speed = max( speed, 0.0f );
  speed = min( speed, maxspeed );

  curDir = ( curDest - pev->origin ).Normalize( );
  //ALERT( at_console, "speed: %f, max: %f, dist: %f\n", m_flGoalSpeed, m_flMaxGoalSpeed, flDist );
  pev->velocity = speed * gpGlobals->v_forward + Vector( 0, 0, speed * curDir.z );
  //ALERT( at_console, "rotspd: %f spd: %f yaw: %f drv: %s, hlt: %f\n", rotspeed, speed, CalcYaw( m_vecDesired ), shoulddrive ? "true" : "false", ( 0.5f * TANK_ACCEL * ( speed / TANK_ACCEL ) * ( speed / TANK_ACCEL ) ) );

  float len = pev->velocity.Length( );
  DoRadiusDamage( len / 10 + 2 );

  if( len < 20 || m_flNextDust > gpGlobals->time )
    return;
  m_flNextDust = gpGlobals->time + 0.4;

  MESSAGE_BEGIN( MSG_PVS, gmsgSpray, pev->origin );
    WRITE_BYTE( SPRAY_DUST );
    WRITE_COORD( 2 );
    WRITE_COORD( 3 );
    WRITE_COORD( -1 );
    WRITE_COORD( -1 );
    WRITE_COORD( -1 );
    WRITE_COORD( 1 );
    WRITE_COORD( entindex( ) );
	  WRITE_COORD( 1 );
	  WRITE_COORD( 1 );
  MESSAGE_END( );
}

void CEscape::HuntTouch( CBaseEntity *pOther )
{
  //if( pOther->Classify( ) != CLASS_NONE )
  //  pOther->TakeDamage( pev, pev, 1000, DMG_BURN );
}

int	CEscape :: Classify ( void )
{
	return	CLASS_MACHINE;
}

void CEscape :: SetYawSpeed ( void )
{
  pev->yaw_speed = 0;
}

int CEscape :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return 0;
}

void CEscape::StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CEscape::HuntThink );
	SetTouch( &CEscape::HuntTouch );
  pev->nextthink = gpGlobals->time + 0.1;
	SetUse( &CEscape::Use );
}

void CEscape::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
  if( pev->fuser4 )
  {
    if( noDriveTill == -1 )
      noDriveTill = 0;
    rescueroute = true;

    NextTarget( );
    EnableAll( pev );
    EnableAll( "bb_escape_radar" );
    DisableAll( "bb_funk" );
    ((cPEHacking*)g_pGameRules)->heliactivated = true;
  }
}

void CEscape :: Spawn()
{
  Precache( );
  gruntname = ALLOC_STRING( "monster_hgrunt" );
	m_vecStartPos = pev->origin;
  ReSpawn( );
}

void CEscape :: ReSpawn()
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT(pev), "models/mapobjects/blackhawk/bh1.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	UTIL_SetOrigin( pev, m_vecStartPos );

  if( glassEnt == NULL )
	{
		glassEnt = GetClassPtr( ( CBaseAnimating *)NULL );
		
		SET_MODEL( ENT(glassEnt->pev), "models/mapobjects/blackhawk/bh3.mdl" );
		//UTIL_SetSize( glassEnt->pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
		UTIL_SetOrigin( glassEnt->pev, pev->origin );

		glassEnt->pev->classname = MAKE_STRING( "bb_escapeair_glass" );
		glassEnt->pev->solid = SOLID_NOT;	
		
		glassEnt->pev->movetype = MOVETYPE_NOCLIP;

    glassEnt->pev->rendermode = kRenderTransAlpha;
		glassEnt->pev->rendercolor.x = 255;
		glassEnt->pev->rendercolor.y = 128;
		glassEnt->pev->rendercolor.z = 128;
		glassEnt->pev->renderamt = 128;
		glassEnt->pev->renderfx = kRenderFxNone;

    glassEnt->pev->maxspeed = pev->maxspeed;
    glassEnt->pev->speed = pev->speed;
    glassEnt->pev->yaw_speed = pev->yaw_speed;
    glassEnt->pev->pitch_speed = pev->pitch_speed;
	glassEnt->pev->sequence = LookupSequence("idle");
	glassEnt->ResetSequenceInfo( );
  }

  if( part2 == NULL )
	{
		part2 = GetClassPtr( ( CBaseAnimating *)NULL );
		
		SET_MODEL( ENT(part2->pev), "models/mapobjects/blackhawk/bh2.mdl" );
		//UTIL_SetSize( part2->pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
		UTIL_SetOrigin( part2->pev, pev->origin );

		part2->pev->classname = MAKE_STRING( "bb_escapeair_part" );
		part2->pev->solid = SOLID_NOT;	
		
		part2->pev->movetype = MOVETYPE_NOCLIP;

    part2->pev->maxspeed = pev->maxspeed;
    part2->pev->speed = pev->speed;
    part2->pev->yaw_speed = pev->yaw_speed;
    part2->pev->pitch_speed = pev->pitch_speed;
	
  }

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_YES;
	pev->health			= 140;
 	pev->deadflag		= DEAD_NO;

	m_flFieldOfView = VIEW_FIELD_NARROW; // 270 degrees

	pev->sequence = 0;
	ResetSequenceInfo( );
	pev->frame = RANDOM_LONG(0, 0xFF);

	InitBoneControllers();

	SetThink( &CEscape::HuntThink );
	SetTouch( &CEscape::HuntTouch );

	pev->nextthink = gpGlobals->time + 1.0;
  m_vecTarget = Vector( 0, 0, 0 );
  baseact = ESCAPE_IDLE;
  m_pGoalEnt = NULL;
  pev->fixangle = 0;
  pev->takedamage = DAMAGE_NO;
  maxspeed = ESCAPE_SPEED;
  maxrotspeed = ESCAPE_ROTATE_SPEED;
  m_posDesired = pev->origin;
  rotspeed = 0;
  reachedrotspeed = 0;
  speed = 0;
  reachedspeed = 0;
  pev->velocity = Vector( 0, 0, 0 );
  pev->avelocity = Vector( 0, 0, 0 );
  noDriveTill = 0;
}
void CEscape :: Precache()
{
  PRECACHE_MODEL( "models/mapobjects/blackhawk/bh1.mdl" );
  PRECACHE_MODEL( "models/mapobjects/blackhawk/bh2.mdl" );
  PRECACHE_MODEL( "models/mapobjects/blackhawk/bh3.mdl" );

  PRECACHE_SOUND( "apache/ap_rotor2.wav" );
  PRECACHE_SOUND( "turret/tu_fire1.wav" );
}

