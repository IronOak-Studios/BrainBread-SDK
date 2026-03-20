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

class CEscape : public CBaseMonster
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

  void KeyValue( KeyValueData *pkvd )
  {
	  if( FStrEq( pkvd->szKeyName, "leavewp" ) )
	  {
		  strcpy( leavewp, pkvd->szValue );
		  pkvd->fHandled = TRUE;
	  }
  }

	void Fly( );
	void Rotate( );

  void DoRadiusDamage( float amount );

	void NextTarget( bool rescuepoint = false );

	void EXPORT Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void ReachedZone( );
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
	
  Vector curDir;
  Vector curDest;
	
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

  CBaseAnimating *part2;
  CBaseAnimating *glassEnt;

  int gruntname;
  bool droped;
  bool rescueroute;

  char leavewp[256];

  CBaseEntity *ezone;

  int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

class CEscapeZone : public CBaseEntity
{
public:
  void Touch( CBaseEntity *pOther );
  void Spawn( );
  void EXPORT Think( );


  list<CBaseEntity*> entlist;
  int numents;
};