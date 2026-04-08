#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"pe_utils.h"
#include "studio.h"
#include "game.h"

extern int gmsgSpray;

#define TANK_SPEED 300
#define TANK_ROTATE_SPEED 60
#define TURRET_ROTATE_SPEED 40
#define TANK_ACCEL (300/3.0f)
#define TANK_ROTATE_ACCEL (60/4.0f)
#define TURRET_ROTATE_SPEED_accel (60.0f)

#define TANK_DRIVE 1
#define TANK_ROTATE 2
#define TANK_IDLE 3

#define TURRET_IDLE 1
#define TURRET_AIM 2
#define TURRET_FIRE 3

#define TANK_SHOT_DELAY 2
#define TANK_SHOOT_ANIM_DURATION 1.0f  // ~30 frames at 30fps


#define TANK_MOVE_TELEPORT 2
#define TANK_MOVE_DRIVE 1

class CTankWP : public CPointEntity
{
public:
	void Spawn( );
	void ReSpawn( );
	void KeyValue( KeyValueData* pkvd );
	float GetDelay( ) { return wait; }
	float GetSpeed( ) { return speed; }
	float GetMoveType( ) { return movetype; }
	
private:
	float	wait;
  float speed;
  int movetype;
  char messagebkp[256];
};
LINK_ENTITY_TO_CLASS( bb_waypoint, CTankWP );

void CTankWP::Spawn( )
{
  CPointEntity::Spawn( );
  strncpy( messagebkp, STRING( pev->message ), sizeof(messagebkp) );
  messagebkp[sizeof(messagebkp) - 1] = '\0';
}

void CTankWP::ReSpawn( )
{
  CPointEntity::ReSpawn( );
  pev->message = MAKE_STRING( messagebkp );
}

void CTankWP::KeyValue( KeyValueData *pkvd )
{
  const char *fu = STRING( pev->targetname );
	if( FStrEq(pkvd->szKeyName, "tankwait" ) )
	{
		wait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "tankspeed" ) )
	{
		speed = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
  else if( FStrEq(pkvd->szKeyName, "tankmove" ) )
	{
		movetype = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else 
		CPointEntity::KeyValue( pkvd );
}

class CTank : public CBaseMonster
{
public:
	void Spawn( void );
	void ReSpawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	
  float CalcYaw( vec3_t vec );
  
  virtual void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	
  void SetObjectCollisionBox( )
	{
		pev->absmin = pev->origin + Vector( -300, -300, 0 );
		pev->absmax = pev->origin + Vector(300, 300, 180 );
	}

	void Drive( );
	void Rotate( );
	void Aim( );
	void Fire( );

  void DoRadiusDamage( float amount );

	void NextTarget( );

	void EXPORT Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT HuntThink( );
	void EXPORT HuntTouch( CBaseEntity *pOther );
	void EXPORT StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

  float speed;
  float rotspeed;
  float gunspeed;
  float reachedspeed;
  float reachedrotspeed;
  float reachedgunspeed;
  float maxspeed;
  float maxrotspeed;
  float maxgunspeed;

  vec3_t m_vecStartPos;
  CBaseEntity *m_pGoalEnt;
  
  Vector m_vecTarget;
	Vector m_posTarget;

	Vector m_vecDesired;
	Vector m_posDesired;

	Vector m_vecGoal;

	Vector m_angGun;
	
  float m_flNextFullThink;
  float startRotate;
  float noDriveTill;
  
  float m_flNextShot;
  float m_flNextDust;
  
  float totalYaw;

  float deltatimeturret;
  float deltatime;

  int baseact;
  int turretact;

  bool shoulddrive;
  bool shouldteleport;

  float m_flSeqResetTime;  // When to reset sequence back to idle after shoot anim

  int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( bb_tank, CTank );

void CTank::DoRadiusDamage( float amount )
{
  CBaseEntity *ent = UTIL_FindEntityInSphere( NULL, pev->origin, 80 );
	while ( ent != NULL )
	{
    if( 1 || FClassnameIs( ent->pev, "monster_zombie" ) )
      ent->TakeDamage( pev, pev, amount, DMG_CRUSH );
		ent = UTIL_FindEntityInSphere( ent, pev->origin, 80 );
	}
}

float CTank::CalcYaw( vec3_t vec )
{
    float res = VecToYaw( vec ) - pev->angles.y;
    res = MOD( res, 360 );
	  if( res > 180 ) res -= 360;
	  if( res < -180 ) res += 360;
    return fabs(res);
}

void CTank::NextTarget( )
{
  if( m_pGoalEnt )
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
  if( !FStringNull( pev->target ) || ( m_pGoalEnt && m_pGoalEnt->pev->target ) )
    m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( ( m_pGoalEnt ? m_pGoalEnt->pev->target : pev->target ) ) );

  if( m_pGoalEnt && FClassnameIs( m_pGoalEnt->pev, "bb_waypoint" ) )
	{
		UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
		m_vecGoal = gpGlobals->v_forward;
    baseact = TANK_ROTATE;
    startRotate = gpGlobals->time;
	  
		m_posDesired = m_pGoalEnt->pev->origin;
    m_vecDesired = ( m_posDesired - pev->origin ).Normalize( );
    totalYaw = sqrt( ( CalcYaw( m_vecDesired ) / 2.0f )  / ( TANK_ROTATE_ACCEL / 2.0f ) );
  }
  else
  {
	  m_posDesired = pev->origin;
    baseact = TANK_IDLE;
  }
}

void CTank::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}

void CTank::HuntThink( )
{
	pev->nextthink = gpGlobals->time;// + 0.05;
  if( deltatimeturret )
    deltatimeturret = gpGlobals->time - deltatimeturret;

  if( turretact == TURRET_AIM )
    Aim( );
 
  deltatimeturret = gpGlobals->time;

  if( m_flNextFullThink > gpGlobals->time )
    return;
  m_flNextFullThink = gpGlobals->time + 0.1;

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
    Drive( );
  }

  if( turretact == TURRET_FIRE )
    Fire( );

  // Reset to idle after shoot anim so client detects next fire
  if( m_flSeqResetTime > 0 && gpGlobals->time >= m_flSeqResetTime )
  {
    pev->sequence = 0;
    m_flSeqResetTime = 0;
  }

  deltatime = gpGlobals->time;

  //EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "ambience/tankidle1.wav", 0.5, 0.3, 0, 110 );


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
  else
    turretact = TURRET_IDLE;

  vec3_t prevt = m_vecTarget;
  m_vecTarget = ( m_posTarget - pev->origin ).Normalize( );
  if( m_vecTarget != prevt )
    turretact = TURRET_AIM;

  if( noDriveTill == -1 )
    return;

	float flLength = (pev->origin - m_posDesired).Length();
	
  if( m_pGoalEnt )
	{
		if( flLength < max( ( 0.5f * TANK_ACCEL * ( speed / TANK_ACCEL ) * ( speed / TANK_ACCEL ) ), 64.0f ) )
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
    baseact = TANK_IDLE;
  }

  //ALERT( at_console, "vecdis: %f %f %f\n", m_vecDesired.x, m_vecDesired.y, m_vecDesired.z );
  //ALERT( at_console, "velo: %f %f %f\n", pev->velocity.x, pev->velocity.y, pev->velocity.z );
  //ALERT( at_console, "ang: %f vel: %f\n", pev->angles.y, pev->avelocity.y );
}

void CTank::Aim(  )
{
  if( m_hEnemy == NULL )
    return;
	UTIL_MakeAimVectors( pev->angles );
	Vector posGun, angGun;
	GetAttachment( 1, posGun, angGun );

	Vector vecTarget = (m_posTarget - posGun).Normalize( );

	Vector vecOut;

	vecOut.x = DotProduct( gpGlobals->v_forward, vecTarget );
	vecOut.y = -DotProduct( gpGlobals->v_right, vecTarget );
	vecOut.z = DotProduct( gpGlobals->v_up, vecTarget );

	Vector angles = UTIL_VecToAngles (vecOut);

	if (angles.y > 180)
		angles.y = angles.y - 360;
	if (angles.y < -180)
		angles.y = angles.y + 360;

  if (angles.y > m_angGun.y)
		m_angGun.y = min( angles.y, m_angGun.y + maxgunspeed * deltatimeturret );
	if (angles.y < m_angGun.y)
		m_angGun.y = max( angles.y, m_angGun.y - maxgunspeed * deltatimeturret );

	m_angGun.y = SetBoneController( 0, m_angGun.y );

  Vector posBarrel, angBarrel;
	GetAttachment( 0, posBarrel, angBarrel );
	Vector vecGun = (posBarrel - posGun).Normalize( );

	if( DotProduct( vecGun, vecTarget ) > 0.98 && ( m_flNextShot <= gpGlobals->time ) )
    turretact = TURRET_FIRE;
}

extern void Explosion( entvars_t* pev, float flDamage, float flRadius );
extern void Explosion( vec3_t origin, entvars_t* pev, float flDamage, float flRadius );

void CTank::Fire(  )
{
  Explosion( m_posTarget, pev, 1000, 200 );

  SetSequenceByName( "shoot" );
  pev->animtime = 0;
  pev->frame = 0;
  m_flSeqResetTime = gpGlobals->time + TANK_SHOOT_ANIM_DURATION;
  turretact = TURRET_AIM;
  m_flNextShot = gpGlobals->time + TANK_SHOT_DELAY;
  Vector posGun, angGun;
	GetAttachment( 0, posGun, angGun );
  MESSAGE_BEGIN( MSG_PVS, gmsgSpray, pev->origin );
    WRITE_BYTE( SPRAY_TANK );
    WRITE_COORD( posGun.x ); // origin
    WRITE_COORD( posGun.y);
    WRITE_COORD( posGun.z);
    WRITE_COORD( pev->angles.x + angGun.x ); // origin
    WRITE_COORD( pev->angles.y + angGun.y );
    WRITE_COORD( pev->angles.z + angGun.z );
    WRITE_COORD( 0 );
	  WRITE_COORD( 1 );
	  WRITE_COORD( 1 );
  MESSAGE_END();
}

void CTank::Rotate(  )
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
  
  if( yaw <= 25 && reachedrotspeed >= 20 )
    rotspeed = ( flSide < 0 ? 1 : -1 ) * reachedrotspeed * yaw / 25;
  else if( fabs( rotspeed ) < maxrotspeed )
    rotspeed += ( flSide < 0 ? 1 : -1 ) * TANK_ROTATE_ACCEL * deltatime;

  if( reachedrotspeed < fabs( rotspeed ) )
    reachedrotspeed = fabs( rotspeed );
  
	pev->avelocity.y = rotspeed;

  DoRadiusDamage( pev->velocity.Length( ) + 2 );
  //ALERT( at_console, "rotspeed: %f, ht: %f, ct: %f, time: %f\n", rotspeed, totalYaw, cur, tm );
}
extern void VectorInverse (vec3_t v);
void CTank::Drive(  )
{
	UTIL_MakeVectors( pev->angles );
  float flDist = DotProduct( m_posDesired - pev->origin, gpGlobals->v_forward );

  if( shoulddrive && noDriveTill <= gpGlobals->time && noDriveTill > -1 )
    speed += TANK_ACCEL * deltatime;
  else
    speed -= TANK_ACCEL * deltatime;
  speed = max( speed, 0.0f );
  speed = min( speed, maxspeed );

  //ALERT( at_console, "speed: %f, max: %f, dist: %f\n", m_flGoalSpeed, m_flMaxGoalSpeed, flDist );
  pev->velocity = speed * gpGlobals->v_forward;
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

void CTank::HuntTouch( CBaseEntity *pOther )
{
  //if( pOther->Classify( ) != CLASS_NONE )
  //  pOther->TakeDamage( pev, pev, 1000, DMG_BURN );
}

int	CTank :: Classify ( void )
{
	return	CLASS_MACHINE;
}

void CTank :: SetYawSpeed ( void )
{
  pev->yaw_speed = 0;
}

int CTank :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return 0;
}

void CTank::StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CTank::HuntThink );
	SetTouch( &CTank::HuntTouch );
  pev->nextthink = gpGlobals->time + 0.1;
	SetUse( &CTank::Use );
}

void CTank::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
  if( noDriveTill == -1 )
    noDriveTill = 0;
}

void CTank :: Spawn()
{
  Precache( );
	m_vecStartPos = pev->origin;
  ReSpawn( );
}

void CTank :: ReSpawn()
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT(pev), "models/mapobjects/tank/m1a1.mdl" );
	UTIL_SetSize( pev, Vector( -100, -100, 0 ), Vector( 100, 100, 80 ) );
	UTIL_SetOrigin( pev, m_vecStartPos );

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_YES;
	pev->health			= 140;
 	pev->deadflag		= DEAD_NO;

	m_flFieldOfView = VIEW_FIELD_NARROW; // 270 degrees

	pev->sequence = 0;
	// Set framerate manually for client-side animation
	pev->framerate = 1.0;
	pev->frame = 0;

	InitBoneControllers();

	SetThink( &CTank::HuntThink );
	SetTouch( &CTank::HuntTouch );

	pev->nextthink = gpGlobals->time + 1.0;
  m_vecTarget = Vector( 0, 0, 0 );
  turretact = TURRET_IDLE;
  baseact = TANK_IDLE;
  m_flSeqResetTime = 0;
  m_pGoalEnt = NULL;
  pev->fixangle = 0;
  pev->takedamage = DAMAGE_NO;
  maxspeed = TANK_SPEED;
  maxrotspeed = TANK_ROTATE_SPEED;
  maxgunspeed = TURRET_ROTATE_SPEED;
  m_posDesired = pev->origin;
  rotspeed = 0;
  reachedrotspeed = 0;
  speed = 0;
  reachedspeed = 0;
  pev->velocity = Vector( 0, 0, 0 );
  pev->avelocity = Vector( 0, 0, 0 );
  noDriveTill = 0;
  pev->team = 1;
  EMIT_SOUND_DYN( ENT(pev), CHAN_STATIC, "ambience/tankidle1.wav", 1, ATTN_NORM, 0, 100 );
}
void CTank :: Precache()
{
  PRECACHE_MODEL( "models/mapobjects/tank/m1a1.mdl" );
  
  PRECACHE_SOUND( "ambience/tankidle1.wav" );
}

