//=============================================================================
// observer.cpp
//
#include        "extdll.h"
#include        "util.h"
#include        "cbase.h"
#include        "player.h"
#include        "weapons.h"
#include        "game.h"

extern int gmsgCurWeapon;
extern int gmsgSetFOV;
extern int gmsgTeamInfo;
extern int gmsgSpectator;
extern int gmsgTargHlth;
extern int gmsgPlayMusic;

#define OBS_CHASE_LOCKED	1
#define OBS_CHASE_FREE		2
#define OBS_ROAMING			3
#define OBS_IN_EYE			4

// Spieler wurde zum Spectator, erstmal alles einrichten
// Dies wurde von der player.cpp hierher verschoben.
extern void CopyToBodyQue(entvars_t* pev);
extern edict_t *EntSelectSpawnPoint( CBasePlayer *pPlayer );

void CBasePlayer::StartObserver( )
{	
	//if( pev->deadflag == DEAD_DYING )
	//	return;
				m_iHideHUD = ( HIDEHUD_HEALTH | HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT );
		//else
		//	m_iHideHUD = (HIDEHUD_HEALTH | HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_MONEY);

	if( pev->sequence > 17 )
		CopyToBodyQue( pev );
    m_afPhysicsFlags |= PFLAG_OBSERVER;
    pev->effects = EF_NODRAW;
    pev->view_ofs = g_vecZero;
    //pev->angles = pev->v_angle;
	m_iSpawnDelayed = 0;
	edict_t* pentSpawnSpot = EntSelectSpawnPoint( this );
	if( !pentSpawnSpot && !( pentSpawnSpot = EntSelectSpawnPoint( this ) ) )//g_pGameRules->GetPlayerSpawnSpot( this ) )
	{
		//ALERT( at_logged, "NO SPAWN SPOT FOUND!\n" );
		//ClientPrint( pev, HUD_PRINTTALK, "No observer spawn point found.\n" );
    m_iSpawnDelayed = 0;
	}
	else
	{
		//pev->origin = VARS(pentSpawnSpot)->origin + Vector(0,0,1);
		pev->angles = VARS(pentSpawnSpot)->angles;
	}
    pev->fixangle = TRUE;
    pev->solid = SOLID_NOT;
    pev->takedamage = DAMAGE_NO;
    pev->movetype = MOVETYPE_NONE;
    ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
    ClearBits( pev->flags, FL_DUCKING );
    pev->deadflag = DEAD_DEAD;
	pev->weapons &= ~(1<<WEAPON_SUIT);

    pev->health = 1;
	EnableControl(true);

	pev->iuser1 = 3;
	pev->iuser2 = 0;


	if (observing)
		return;	

	TraceResult tr;
	Vector vecPosition;
	Vector vecViewAngle;

	// Linie nach oben ziehen, von Spielerposition aus
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, 128 ), ignore_monsters, edict(), &tr );

	// Neue Position festlegen
	vecPosition = tr.vecEndPos;
	vecViewAngle = UTIL_VecToAngles( tr.vecEndPos - pev->origin );

	observing = 1;

    // Alle Client-Side-Entities, die dem Spieler
    // zugewiesen wurden, abschalten
        MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
                WRITE_BYTE( TE_KILLPLAYERATTACHMENTS );
                WRITE_BYTE( (BYTE)entindex() );
        MESSAGE_END();

        // Alle Waffen wegstecken
        if (m_pActiveItem)
                m_pActiveItem->Holster( );

        if ( m_pTank != NULL )
        {
                m_pTank->Use( this, this, USE_OFF, 0 );
                m_pTank = NULL;
        }

        // Message Cache des Anzugs löschen,
        // damit wir nicht weiterlabern ;-)
        SetSuitUpdate(NULL, FALSE, 0);

        // Mitteilung an Ammo-HUD, dass der Spieler tot ist
        MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
			WRITE_BYTE(0);
			WRITE_BYTE(0);
			WRITE_BYTE(0XFF);
			WRITE_BYTE(0xFF);
			WRITE_BYTE(0xFF);
        MESSAGE_END();

        // Zoom zurücksetzen
        m_iFOV = m_iClientFOV = 0;
        pev->fov = m_iFOV;
        MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
                WRITE_BYTE(0);
        MESSAGE_END();

        // Ein paar Flags setzen
		//if(m_iTeam == 0)
			m_iHideHUD = ( HIDEHUD_HEALTH | HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT );
		//else
		//	m_iHideHUD = (HIDEHUD_HEALTH | HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_MONEY);

        m_afPhysicsFlags |= PFLAG_OBSERVER;
        pev->effects = EF_NODRAW;
        pev->view_ofs = g_vecZero;
        pev->angles = pev->v_angle = vecViewAngle;
        pev->fixangle = TRUE;
        pev->solid = SOLID_NOT;
        pev->takedamage = DAMAGE_NO;
        pev->movetype = MOVETYPE_NONE;
        ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
        ClearBits( pev->flags, FL_DUCKING );
        pev->deadflag = DEAD_RESPAWNABLE;
		pev->weapons &= ~(1<<WEAPON_SUIT);

        pev->health = 1;
        pev->max_health = 1;
		EnableControl(true);

        // Status-Bar ausschalten
        m_fInitHUD = TRUE;

        // Team Status updaten
        pev->team = m_iTeam; //0;
        MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
                WRITE_BYTE( ENTINDEX(edict()) );
                WRITE_BYTE( m_iTeam );
        MESSAGE_END();
        MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, edict( ) );
                WRITE_BYTE( 101 );
                WRITE_BYTE( m_iTeam );
        MESSAGE_END();

        // Dem Spieler alles wegnehmen
        RemoveAllItems( FALSE );

        // Zur neuen Position bewegen
        UTIL_SetOrigin( pev, vecPosition );

        // Einen Spieler finden, den wir angucken
        m_flNextObserverInput = 0;
        Observer_SetMode(OBS_CHASE_FREE);

        //Allen Clients mitteilen, dass der Spieler jetzt Spectator ist
        MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
                WRITE_BYTE( ENTINDEX( edict() ) );
                WRITE_BYTE( 1 );
        MESSAGE_END();
		pev->iuser1 = 3;
		pev->iuser2 = 0;

      MESSAGE_BEGIN( MSG_ONE, gmsgPlayMusic, NULL, edict( ) );
      WRITE_BYTE( 3 );
      WRITE_COORD( 400 );
    MESSAGE_END( );
	g_engfuncs.pfnSetClientMaxspeed( ENT(pev), 400 );
}

// Observermodus verlassen
void CBasePlayer::StopObserver( void )
{
        // Spectator-Modus abschalten
        if ( pev->iuser1 || pev->iuser2 )
        {
                // Allen Clients mitteilen, dass der Spieler kein Spectator
                // mehr ist.
                MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
                        WRITE_BYTE( ENTINDEX( edict() ) );
                        WRITE_BYTE( 0 );
                MESSAGE_END();

                pev->iuser1 = pev->iuser2 = 0;
                m_hObserverTarget = NULL;
        		observing = 0;

				m_iHideHUD = 0;
		}
		for( int i = 0; i < 32; i++ )
		{
			dmgdone[i].index = 0;
			dmgreceived[i].index = 0;
			for( int o = 0; o < 8; o++ )
			{
				dmgdone[i].hitgroup[o] = 0;
				dmgreceived[i].hitgroup[o] = 0;
			}
			dmgdone[i].damage = 0;
			dmgreceived[i].damage = 0;
			dmgdone[i].hits	= 0;
			dmgreceived[i].hits	= 0;
		}
}

// Den nächsten Client finden, den der Spieler anschaut
void CBasePlayer::Observer_FindNextPlayer( bool bReverse )
{
	int iStart;
	if( m_hObserverTarget )
		iStart = ENTINDEX( m_hObserverTarget->edict() );
	else
		iStart = ENTINDEX( edict() );
	int iCurrent = iStart;

	m_hObserverTarget = NULL;

	int iDir = bReverse ? -1 : 1;

	do
	{
			iCurrent += iDir;

			// Durch alle Clients loopen
			if( iCurrent > MAX_PLAYERS )
				iCurrent = 1;
			if (iCurrent < 1)
				iCurrent = MAX_PLAYERS;

			CBasePlayer *pEnt = (CBasePlayer*)UTIL_PlayerByIndex( iCurrent );
			if( !pEnt || !pEnt->IsPlayer( ) )
			{
//				ALERT( at_console, "Spectator: no ent\n" );
				continue;
			}
			if( pEnt == this )
			{
	//			ALERT( at_console, "Spectator: ent is this\n" );
				continue;
			}
			// Keine unsichtbaren Spieler oder andere Observer beobachten
			if ( !pEnt->IsAlive( ) )
			{
//				ALERT( at_console, "Spectator: not alive\n" );
				continue;
			}
			if( teamspect.value && ( pEnt->m_iTeam != m_iTeam ) )
				continue;


			m_hObserverTarget = pEnt;
			break;

	} while ( iCurrent != iStart );

	// Ziel gefunden?
	if ( m_hObserverTarget )
	{
		// Ziel in pev speichern damit die Bewegungs-DLL dran kommt
		pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
		MESSAGE_BEGIN( MSG_ONE, gmsgTargHlth, NULL, edict() );
			WRITE_BYTE( (int)( m_hObserverTarget->pev->health > 0 ? m_hObserverTarget->pev->health : 0 ) );
			WRITE_BYTE( (int)( m_hObserverTarget->pev->armorvalue > 0 ? m_hObserverTarget->pev->armorvalue : 0 ) );
		MESSAGE_END();
		// Zum Ziel bewegen
		UTIL_SetOrigin( pev, m_hObserverTarget->pev->origin );

		//ALERT( at_console, "Now Tracking %s\n", STRING( m_hObserverTarget->pev->netname ) );
	}
	else
	{
		ALERT( at_console, "No observer targets.\n" );
	}
}

// Tastatur-Eingaben für den Observermodus...
void CBasePlayer::Observer_HandleButtons()
{
	// Mouse-Clicks verlangsamen
	if ( m_flNextObserverInput > gpGlobals->time )
		return;

	// Springen wechselt zwischen den Modi
	if ( m_afButtonPressed & IN_JUMP )
	{
		if ( pev->iuser1 == OBS_ROAMING )
			Observer_SetMode( OBS_CHASE_LOCKED );
		else if ( pev->iuser1 == OBS_CHASE_LOCKED )
			Observer_SetMode( OBS_CHASE_FREE );
		else if ( pev->iuser1 == OBS_CHASE_FREE )
			Observer_SetMode( OBS_IN_EYE );
		else if ( pev->iuser1 == OBS_IN_EYE )
			Observer_SetMode( teamspect.value ? OBS_CHASE_LOCKED : OBS_ROAMING );

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack wechselt zum nächsten Spieler
	if ( m_afButtonPressed & IN_ATTACK && pev->iuser1 != OBS_ROAMING )
	{
		Observer_FindNextPlayer( false );

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}

	// Attack2 wechselt zum vorherigen Spieler
	if ( m_afButtonPressed & IN_ATTACK2 && pev->iuser1 != OBS_ROAMING )
	{
		Observer_FindNextPlayer( true );

		m_flNextObserverInput = gpGlobals->time + 0.2;
	}
}

// Versuche, Observer-Modus zu wechseln
void CBasePlayer::Observer_SetMode( int iMode )
{
        // Abbrechen, wenn wir bereits im gewünschten Modus sind
        if ( ( m_hObserverTarget != NULL ) && ( iMode == pev->iuser1 ) )
                return;

        // Wechsle zu Roaming?
        if ( iMode == OBS_ROAMING )
        {
                // MOD AUTOREN: Wenn ihr kein Roaming-Modus in eurem Mod wollt,
                //              dann brecht hier einfach ab
                pev->iuser1 = OBS_ROAMING;
                pev->iuser2 = 0;

                ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode3" );
                pev->maxspeed = 320;
                return;
        }

        // Wechsle zu ChaseLock?
        if ( iMode == OBS_CHASE_LOCKED )
        {
                // Sicherstellen, dass wir ein Ziel haben
                if ( m_hObserverTarget == NULL )
                        Observer_FindNextPlayer( false );

                if (m_hObserverTarget)
                {
                        pev->iuser1 = OBS_CHASE_LOCKED;
                        pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
                        ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode1" );
						MESSAGE_BEGIN( MSG_ONE, gmsgTargHlth, NULL, edict() );
							WRITE_BYTE( (int)( m_hObserverTarget->pev->health > 0 ? m_hObserverTarget->pev->health : 0 ) );
							WRITE_BYTE( (int)( m_hObserverTarget->pev->armorvalue > 0 ? m_hObserverTarget->pev->armorvalue : 0 ) );
						MESSAGE_END();
                       pev->maxspeed = 0;
                }
                else
                {
                        ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
                        Observer_SetMode(OBS_ROAMING);
                }

                return;
        }

        // Wechsle zu ChaseFree?
        if ( iMode == OBS_CHASE_FREE )
        {
                // Sicherstellen, dass wir ein Ziel haben
                if ( m_hObserverTarget == NULL )
                        Observer_FindNextPlayer( false );

                if (m_hObserverTarget)
                {
                        pev->iuser1 = OBS_CHASE_FREE;
                        pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
						MESSAGE_BEGIN( MSG_ONE, gmsgTargHlth, NULL, edict() );
							WRITE_BYTE( (int)( m_hObserverTarget->pev->health > 0 ? m_hObserverTarget->pev->health : 0 ) );
							WRITE_BYTE( (int)( m_hObserverTarget->pev->armorvalue > 0 ? m_hObserverTarget->pev->armorvalue : 0 ) );
						MESSAGE_END();
                        ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode2" );
                        pev->maxspeed = 0;
                }
                else
                {
                        ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
                        Observer_SetMode(OBS_ROAMING);
                }

                return;
        }


        if ( iMode == OBS_IN_EYE )
        {
                // Sicherstellen, dass wir ein Ziel haben
                if ( m_hObserverTarget == NULL )
                        Observer_FindNextPlayer( false );

                if (m_hObserverTarget)
                {
                        pev->iuser1 = OBS_IN_EYE;
                        pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
						MESSAGE_BEGIN( MSG_ONE, gmsgTargHlth, NULL, edict() );
							WRITE_BYTE( (int)( m_hObserverTarget->pev->health > 0 ? m_hObserverTarget->pev->health : 0 ) );
							WRITE_BYTE( (int)( m_hObserverTarget->pev->armorvalue > 0 ? m_hObserverTarget->pev->armorvalue : 0 ) );
						MESSAGE_END();
                        ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode4" );
                        pev->maxspeed = 0;
                }
                else
                {
                        ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
                        Observer_SetMode(OBS_ROAMING);
                }

                return;
        }
}

