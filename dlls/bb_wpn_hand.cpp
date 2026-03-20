#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


#define	HAND_BODYHIT_VOLUME 128
#define	HAND_WALLHIT_VOLUME 512
#define HAND_SLOT		0
#define HAND_POS		1

#define CB_TORSO		(1<<0)
#define CB_ARMS			(1<<1)
#define CB_LEGS			(1<<2)
#define CB_HEAD			(1<<3)
#define CB_NANO			(1<<4)

LINK_ENTITY_TO_CLASS( weapon_hand, CHand );

#ifdef CLIENT_DLL
extern void InitGlobals( );
#endif

enum hand_e {
	HAND_IDLE = 0,
	HAND_ATTACK,
	HAND_ATTACK2,
};


void CHand::Spawn( )
{
  pev->classname = MAKE_STRING("weapon_hand");
	Precache( );
	m_iId = WEAPON_HAND;
  SET_MODEL(ENT(pev), "models/knife/w_knife.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CHand::Precache( void )
{
	PRECACHE_MODEL("models/hand/v_hand.mdl");
	PRECACHE_SOUND("weapons/knife_slash-1.wav");
	PRECACHE_SOUND("weapons/knife_slash-2.wav");
  PRECACHE_SOUND("zombie/claw_strike1.wav");
	PRECACHE_SOUND("zombie/claw_strike2.wav");

	m_usHand = PRECACHE_EVENT ( 1, "events/hand.sc" );
}

int CHand::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = HAND_SLOT;
	p->iPosition = HAND_POS;
	p->iId = WEAPON_HAND;
	p->iWeight = 1;
	return 1;
}



BOOL CHand::Deploy( )
{
	return DefaultDeploy( "models/hand/v_hand.mdl", "", HAND_IDLE, "knife" );
}

void CHand::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( HAND_IDLE );
}


extern void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity );

void CHand::PrimaryAttack()
{
	if (! Swing( 1 ))
	{
		SetThink( &CHand::SwingAgain );
#ifdef CLIENT_DLL
    if( !gpGlobals )
      InitGlobals( );
#endif
    pev->nextthink = gpGlobals->time + 0.1;
	}
}


void CHand::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_HAND );
}


void CHand::SwingAgain( void )
{
	Swing( 0 );
}


int CHand::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );

#ifdef CLIENT_DLL
    if( !gpGlobals )
      InitGlobals( );
#endif
  Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;
  Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usHand, 
	0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0,
	0.0, 0, 0.0 );


	if ( tr.flFraction >= 1.0 )
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
			
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		/*switch( ((m_iSwing++) % 2) + 1 )
		{
		case 0:
			SendWeaponAnim( HAND_ATTACK1HIT ); break;
		case 1:
			SendWeaponAnim( HAND_ATTACK2HIT ); break;
		case 2:
			SendWeaponAnim( HAND_ATTACK3HIT ); break;
		}*/
		SendWeaponAnim( HAND_ATTACK2 );
		

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage( );

		float dmg = 80;

		/*if( m_pPlayer && FBitSet( m_pPlayer->m_iCyber, CB_ARMS ) )
		{
			dmg = 240;
		}

		if ( (m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
		{
			// first swing does full damage
			pEntity->TraceAttack(m_pPlayer->pev, dmg, gpGlobals->v_forward, &tr, DMG_CLUB ); 
		}
		else
		{
			// subsequent swings do half
			pEntity->TraceAttack(m_pPlayer->pev, dmg / 2, gpGlobals->v_forward, &tr, DMG_CLUB ); 
		}	
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );*/
		m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0, 0, 0 ), 64, BULLET_NONE, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,1) )
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "zombie/claw_strike1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "zombie/claw_strike2.wav", 1, ATTN_NORM); break;
				/*case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod3.wav", 1, ATTN_NORM); break;
				*/}
				m_pPlayer->m_iWeaponVolume = HAND_BODYHIT_VOLUME;
				if ( !pEntity->IsAlive() )
				{
					m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
					return TRUE;
				}
				else
					  flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_HAND);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play hand strike
			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "zombie/claw_strike1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "zombie/claw_strike2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * HAND_WALLHIT_VOLUME;
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
		
		SetThink( &CHand::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 1;

		
	}
	return fDidHit;
}