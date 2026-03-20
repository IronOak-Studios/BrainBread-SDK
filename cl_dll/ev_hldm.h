//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined ( EV_HLDMH )
#define EV_HLDMH

// bullet types
typedef	enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_MP5, // mp5
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_SEBURO,
	BULLET_PLAYER_BENELLI,
	BULLET_PLAYER_BERETTA,
	BULLET_PLAYER_M16,
	BULLET_PLAYER_GLOCK,
	BULLET_PLAYER_KNIFE,
	BULLET_PLAYER_AUG,
	BULLET_PLAYER_MINIGUN,
	BULLET_PLAYER_MICROUZI,
	BULLET_PLAYER_MICROUZI_A,
	BULLET_PLAYER_USP,
	BULLET_PLAYER_STONER,
	BULLET_PLAYER_SIG550,
	BULLET_PLAYER_P226,
	BULLET_PLAYER_P225,
	BULLET_PLAYER_BERETTA_A,
	BULLET_PLAYER_GLOCK_AUTO_A,
	BULLET_PLAYER_GLOCK_AUTO,
	BULLET_PLAYER_MP5SD,
	BULLET_PLAYER_TAVOR,
	BULLET_PLAYER_SAWED,
	BULLET_PLAYER_AK47,
	BULLET_PLAYER_FLAME,
	BULLET_PLAYER_USPMP,
	BULLET_PLAYER_HAND,
	BULLET_PLAYER_SPRAY,

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,
} Bullet;

bool	bSide = FALSE;
int		iSideCount = 1;

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName );
void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType );
int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount );
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY );

#endif // EV_HLDMH