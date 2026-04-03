#include "hud.h"
#include "cl_util.h"
#include "string.h"
#include "pe_rain.h"
#include "triangleapi.h"
#include "pm_defs.h"
#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"


#define MAX_PARTICLES_SNOW	(2000*CVAR_GET_FLOAT("cl_weatherdetail"))
#define MAX_PARTICLES_RAIN	(1000*CVAR_GET_FLOAT("cl_weatherdetail"))
#define MAX_PARTICLES_GLOBALSNOW	(3000*CVAR_GET_FLOAT("cl_weatherdetail"))
#define MAX_PARTICLES_GLOBALRAIN	(2000*CVAR_GET_FLOAT("cl_weatherdetail"))
#define PARTICLE_LIFE		 ( m_iType == TYPE_SNOW || m_iType == TYPE_GLOBALSNOW ? 15 : 5 )
#define START_PARTICLES		0

#define ACTIVE_FALL		1
#define	ACTIVE_IMPACT	2

extern int iRainIndex;
extern vec3_t vRainMin[20];
extern vec3_t vRainMax[20];
extern vec3_t iRainType[20];

extern vec3_t		v_origin, v_angles, v_cl_angles, v_sim_org, v_lastAngles;
extern "C" 
{
	void	InterpolateAngles(  float * start, float * end, float * output, float frac );
	void	NormalizeAngles( float * angles );
	float	Distance(const float * v1, const float * v2);
	float	AngleBetweenVectors(  const float * v1,  const float * v2 );
}

void bla( char *format, ... )
{
	va_list		argptr;
	char str[1024];

	va_start( argptr, format );
	vsnprintf( str, sizeof(str), format, argptr );
	va_end( argptr );

	ConsolePrint( str );	
}

cRain::~cRain( )
{
	if( m_pList != NULL )
	{
		delete [] m_pList;
		m_pList = NULL;
	}
}

cRain::cRain( int index, t_partinfo *info )
{
	m_fParticleSideSpread = info->fSideSpread;
	m_iType = info->iType;
	m_iRainIndex = ( index >= 0 && index < 20 ) ? index : 0;
	m_iNumActive = 0;
	m_iActive = 0;
	m_fNextParticle = 0;
	m_fTime = 0;
	m_fTimeDiff = 0;
	m_pFree = NULL;
	m_pList = NULL;
	m_pList = NULL;
	m_fParticleSpeed = info->fSpeed;

 	float distx = vRainMax[m_iRainIndex].x - vRainMin[m_iRainIndex].x;
	float disty = vRainMax[m_iRainIndex].y - vRainMin[m_iRainIndex].y;
  switch( m_iType )
	{
	case TYPE_SNOW:
	case TYPE_GLOBALSNOW:
		m_iSprFall = SPR_Load( "sprites/snow.spr" );
		m_fParticleSizeY = m_fParticleSizeX = info->fSize;
    m_fParticleDelay = 1.0f / ( distx * disty / 4500.0f * ( info->iPartsPerSec / 50.0f ) );	
		break;
	default:
		m_iSprFall = SPR_Load( "sprites/rain.spr" );
		m_fParticleSizeX = 64;
		m_fParticleSizeY = 196;
		m_fParticleDelay = 1.0f / ( distx * disty / 4500.0f * ( info->iPartsPerSec / 50.0f ) );
		break;
	}
  if( gEngfuncs.pfnGetLevelName( ) )
  {
    if( strstr( gEngfuncs.pfnGetLevelName( ), "bladerunner" ) )
    {
 		  m_fParticleSpeed = 900;
      m_fParticleDelay = 1.0f / ( distx * disty / 4500.0f * ( 150 / 50.0f ) );
      m_iMaxParticles = MAX_PARTICLES_RAIN;
    }
    else if( strstr( gEngfuncs.pfnGetLevelName( ), "titanium" ) )
    {
      m_fParticleDelay = ( 1.0 / ((float)info->iPartsPerSec * 2.5f ) );
    }  
  }
  
	m_iSprImpact = SPR_Load( "sprites/impact.spr" );
	//bla( "New: psec: %d, speed: %f, size: %f, spread: %f, type: %d\n", info->iPartsPerSec, info->fSpeed, info->fSize, info->fSideSpread, info->iType );
	//bla( "Set: delay: %f, speed: %f\n", m_fParticleDelay, m_fParticleSpeed );

	
}

void cRain::Init( )
{
	t_particle *init = NULL;
	int i = 0;

	m_iNumActive = 0;
	m_iActive = 0;
	m_fNextParticle = 0;
	m_fTime = 0;
	m_fTimeDiff = 0;
	m_iMaxNum = 0;
	m_fMaxCheck = 17;

  switch( m_iType )
  {
  case TYPE_SNOW:
		m_iMaxParticles = MAX_PARTICLES_SNOW;
    break;
  case TYPE_GLOBALSNOW:
		m_iMaxParticles = MAX_PARTICLES_GLOBALSNOW;
    break;
  case TYPE_RAIN:
		m_iMaxParticles = MAX_PARTICLES_RAIN;
    break;
  case TYPE_GLOBALRAIN:
		m_iMaxParticles = MAX_PARTICLES_GLOBALRAIN;
    break;
  }

	if( m_pList != NULL )
	{
		delete [] m_pList;
		m_pList = NULL;
	}

	m_pList = new t_particle[m_iMaxParticles];
	m_pFree = m_pList;
	for( i = 0; i < m_iMaxParticles; i++ )
	{
		init = m_pList + i;
		memset( init, 0, sizeof(t_particle) );
		if( i < ( m_iMaxParticles - 1 ) )
			init->pNext = m_pList + i + 1;
		else
			init->pNext = NULL;
	}
	m_iActive = 1;
	for( i = 0; i < START_PARTICLES; i++ )
	{
		StartNew( );
	}
}

void cRain::Delete( int index, int free )
{
	if( !m_pList || index < 0 || index >= m_iMaxParticles )
		return;

	t_particle *del = m_pList + index;

	if( !del->iActive )
		return;

	memset( del, 0, sizeof(t_particle) );

	if( !free )
	{
		m_iNumActive--;
		return;
	}
	
	if( !m_pFree )
	{
		m_pFree = del;
		m_iNumActive--;
		return;
	}
	del->pNext = m_pFree;
	m_pFree = del;
	m_iNumActive--;
}

void cRain::StartNew( )
{
	t_particle *start = NULL;
	pmtrace_t tr;
	vec3_t vEnd;
	vec3_t vDir;

	vDir.x = gEngfuncs.pfnRandomFloat( -m_fParticleSpeed*m_fParticleSideSpread, m_fParticleSpeed*m_fParticleSideSpread );
	vDir.y = gEngfuncs.pfnRandomFloat( -m_fParticleSpeed*m_fParticleSideSpread, m_fParticleSpeed*m_fParticleSideSpread );
	vDir.z = -m_fParticleSpeed;
  vDir = vDir.Normalize( );


	if( !m_pFree )
		return;
	
	start = m_pFree;
	if( m_pFree->pNext )
		m_pFree = m_pFree->pNext;
	else
		m_pFree = NULL;

	start->fLastDist = 9000;
	start->vOrigin.z = vRainMax[m_iRainIndex].z;
	start->vOrigin.x = gEngfuncs.pfnRandomFloat( vRainMin[m_iRainIndex].x, vRainMax[m_iRainIndex].x );
	start->vOrigin.y = gEngfuncs.pfnRandomFloat( vRainMin[m_iRainIndex].y, vRainMax[m_iRainIndex].y );

	for( int i = 0; i < 3; i++ )
		vEnd[i] = start->vOrigin[i] + 1000 * vDir[i];

	//tr = *(gEngfuncs.PM_TraceLine( (float *)&start->vOrigin, (float *)&vEnd, 0, 2 /*point sized hull*/, -1 ));
	
	start->vTarget = vEnd;//tr.endpos;
  start->bImpact = true;
	if( start->vTarget.z < v_origin.z - 200 )
  {
    start->bImpact = false;
    start->vTarget.z = v_origin.z - 200;
  }

	
	start->vMove = vDir * m_fParticleSpeed;
	start->fNextMove = 0;
	start->fRemove = m_fTime + PARTICLE_LIFE;
	start->pNext = NULL;
	start->iActive = ACTIVE_FALL;
	start->iSprite = m_iSprFall;
	start->iFrame = 0;
	start->fSizeX = m_fParticleSizeX;
	start->fSizeY = m_fParticleSizeY;
	m_iNumActive++;
}

extern vec3_t ev_punchangle, v_origin;

void cRain::Draw( )
{
	t_particle *check = NULL;

	if( !m_pList )
		return;
	Vector2D dist = vRainMin[m_iRainIndex].Make2D( );
	dist = ( dist + vRainMax[m_iRainIndex].Make2D( ) ) / 2.0f - v_origin.Make2D( );

	if( dist.Length( ) > 2000 )
		return;
	
	vec3_t up, v_forward, v_right, v_up, orig;
	
	AngleVectors( v_angles + ev_punchangle, v_forward, v_right, v_up );

	
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );	
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );

	gEngfuncs.pTriAPI->Color4f( 0.70, 0.70, 0.70 , 0.75 );
	gEngfuncs.pTriAPI->Brightness( 1 );

	if( m_iType == TYPE_RAIN || m_iType == TYPE_GLOBALRAIN )
	{
		up = Vector( 0, 0, 1 );
		gEngfuncs.pTriAPI->Color4f( 0.70, 0.70, 0.70 , 0.25 );
	}
	else
		up = v_up;

	for( int i = 0; i < m_iMaxParticles; i++ )
	{
		check = m_pList + i;
		if( !check->iActive )
			continue;
		if( !gEngfuncs.pTriAPI->SpriteTexture( (struct model_s*)gEngfuncs.GetSpritePointer( check->iSprite ), check->iFrame ) )
			continue;

		 if( m_iType == TYPE_RAIN || m_iType == TYPE_GLOBALRAIN )
		 {
			 if( check->iActive == ACTIVE_IMPACT )
       {
         orig = check->vOrigin;
  			 up = v_up;
       }
       else
       {
         orig = check->vOrigin;
         orig.z += m_fParticleSizeY;
				 up = Vector( 0, 0, 1 );
       }
		 }
     else
       orig = check->vOrigin;


		gEngfuncs.pTriAPI->Begin( TRI_QUADS );
		

		gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
		gEngfuncs.pTriAPI->Vertex3fv( orig - v_right * check->fSizeX + up * check->fSizeY );

		gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
		gEngfuncs.pTriAPI->Vertex3fv( orig + v_right * check->fSizeX + up * check->fSizeY );

		gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
		gEngfuncs.pTriAPI->Vertex3fv( orig + v_right * check->fSizeX - up * check->fSizeY );

		gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
		gEngfuncs.pTriAPI->Vertex3fv( orig - v_right * check->fSizeX - up * check->fSizeY );

		gEngfuncs.pTriAPI->End( );
		
	}
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );

}

#define MOD( a, b ) ( (a) - (int)( (a) / (b) ) * (b) )

void cRain::Think( float fTime )
{
	t_particle *check = NULL;
	int i = 0;

	m_fTime += fTime;

	if( ( fTime > 0 ) && !m_iActive )
		Init( );

	if( fTime == 0 )
		return;

	if( !m_pList )
		return;

	if( m_fNextParticle <= m_fTime )
	{
		float tmp = max( fTime, m_fParticleDelay );
		int num = (int)min( tmp / m_fParticleDelay, tmp / 0.00125 ); // 800 pps
		if( num > 80 )
			num = 80; // fps < 10
		for( i = 0; i < num; i++ )
			StartNew( );
		m_fNextParticle = m_fTime + m_fParticleDelay;
	}

	for( i = 0; i < m_iMaxParticles; i++ )
	{
		check = m_pList + i;

		if( !check->iActive )
			continue;

		if( check->iActive == ACTIVE_IMPACT )
		{
			if( check->fRemove <= m_fTime )
			{
				Delete( i );
				continue;
			}
			check->iFrame = (int)( (float)( check->fRemove - m_fTime ) / 0.02 ) - 1;
			if( check->iFrame < 0 )
				check->iFrame = 0;
			if( check->iFrame > 4 )
				check->iFrame = 4;
		}
		if( m_iType == TYPE_RAIN )
		{
			//check->iFrame = MOD( m_fTime / 0.2, 2 );
		}

		check->vOrigin = check->vOrigin + check->vMove * fTime;

		if( check->fRemove <= m_fTime )
		{
			Delete( i );
			continue;
		}
		if( Distance( check->vOrigin, check->vTarget ) > check->fLastDist )
		{
      if( m_iType == TYPE_SNOW || m_iType == TYPE_GLOBALSNOW || !check->bImpact )
			{
				Delete( i );
				continue;
			}
			/*switch( gEngfuncs.pfnRandomLong( 1, 3 ) )
			{
			case 1:*/
				check->iActive = ACTIVE_IMPACT;
				check->vOrigin = check->vTarget;
				check->vMove = Vector( 0, 0, 0 );
				check->fRemove = m_fTime + 0.1;
				check->iSprite = m_iSprImpact;
				check->iFrame = 0;
				check->fSizeX = 3;
				check->fSizeY = 3;
			/*	break;
			default:
				Delete( i );
			}*/
		}
		else
			check->fLastDist = Distance( check->vOrigin, check->vTarget );
	}
	if( m_iNumActive > m_iMaxNum )
		m_iMaxNum = m_iNumActive;
	if( ( m_fMaxCheck != 0 ) && ( m_fMaxCheck <= m_fTime ) )
	{
		int tmax = m_iMaxParticles;
		m_iMaxParticles = m_iMaxNum;
		m_fMaxCheck = 0;
		for( int i = m_iMaxNum; i < tmax; i++)
		{
			Delete( i, 0 );
		}
		//bla( "Setting m_iMaxParticles to %d...\n", m_iMaxNum );
	}
}

cGlobalRain::cGlobalRain( int index, t_partinfo *info ) : cRain( index, info )
{
  vmax = vRainMax[m_iRainIndex] = vRainMax[m_iRainIndex] - vRainMin[m_iRainIndex];
  vRainMin[m_iRainIndex] = Vector( 0, 0, 0 );
}

void cGlobalRain::Think( float fTime )
{
  vec3_t half = vmax / 2;

  vRainMin[m_iRainIndex].x =  v_origin.x - half.x;
  vRainMin[m_iRainIndex].y =  v_origin.y - half.y;
  vRainMin[m_iRainIndex].z =  v_origin.z;
  vRainMax[m_iRainIndex].x =  v_origin.x + half.x;
  vRainMax[m_iRainIndex].y =  v_origin.y + half.y;
  vRainMax[m_iRainIndex].z =  v_origin.z + vmax.z;
  cRain::Think( fTime );
}