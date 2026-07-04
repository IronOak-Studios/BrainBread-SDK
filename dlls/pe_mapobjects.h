class CPELRef : public CBaseEntity
{
public:
	void Spawn()
	{
		Precache( );
		pev->movetype = MOVETYPE_NOCLIP;
		pev->solid = SOLID_NOT;
		//pev->effects |= EF_BRIGHTLIGHT;

		SET_MODEL(ENT(pev), "sprites/laserdot.spr");
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 0;
		UTIL_SetSize( pev, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ) );
		UTIL_SetOrigin( pev, pev->origin );

		pev->takedamage		= DAMAGE_NO;
		pev->health			= 1;
	}
	void ReSpawn()
	{
		Spawn( );
	}
	void Precache( )
	{
		PRECACHE_MODEL("sprites/laserdot.spr");
	}
	void KeyValue( KeyValueData *pkvd )
	{
		if( FStrEq( pkvd->szKeyName, "refnr" ) )
		{
			m_iRefnr = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
	}
	int		m_iRefnr;
};

class CPELight : public CBaseEntity
{
public:
	void Spawn()
	{
		Precache( );
		pev->classname = MAKE_STRING("pe_light");
		pev->movetype = MOVETYPE_NOCLIP;
		pev->solid = SOLID_NOT;
		//pev->effects |= EF_BRIGHTLIGHT;

		SET_MODEL(ENT(pev), "sprites/laserdot.spr");
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 0;
		UTIL_SetSize( pev, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ) );
		UTIL_SetOrigin( pev, pev->origin );

		pev->takedamage		= DAMAGE_NO;
		pev->health			= 1;

		m_pRef = NULL;
		CPELRef *pLightRef = (CPELRef *)UTIL_FindEntityByClassname( NULL, "pe_light_ref" );
		while( pLightRef != NULL )
		{
			if( pLightRef->m_iRefnr == m_iRefnr )
			{
				m_iRef = ENTINDEX( pLightRef->edict( ) );
				m_pRef = pLightRef;
				return;
			}
			pLightRef = (CPELRef *)UTIL_FindEntityByClassname( pLightRef, "pe_light_ref" );
		}
	}
	void ReSpawn()
	{
		Spawn( );
	}
	void Precache( )
	{
		PRECACHE_MODEL("sprites/laserdot.spr");
	}
	void KeyValue( KeyValueData *pkvd )
	{
		if( FStrEq( pkvd->szKeyName, "refpnt" ) )
		{
			m_iRefnr = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
		}
	}
	int		m_iRefnr;
	int		m_iRef;
	CPELRef *m_pRef;

};
