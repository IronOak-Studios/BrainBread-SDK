#include "hud.h"
#include "cl_util.h"
// Undefine min/max macros before any C++ standard library or Windows SDK
// headers to prevent macro interference with <cstdlib> and <algorithm>
#undef min
#undef max
#undef fabs
#include "string.h"
#include "bb_blood.h"
#include "triangleapi.h"
#include "pm_defs.h"
#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "r_studioint.h"
#include <gl/gl.h>
#include <gl/glext.h>
#include <list>
using namespace std;
#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

extern engine_studio_api_t IEngineStudio;

#define MAX_PARTICLES	2000
#define MAX_LIFE	2
#define PARTICLE_LIFE		 0.3
#define START_PARTICLES		0

extern vec3_t		v_origin, v_angles, v_cl_angles, v_sim_org, v_lastAngles;
extern "C" 
{
	void	InterpolateAngles(  float * start, float * end, float * output, float frac );
	void	NormalizeAngles( float * angles );
	float	Distance(const float * v1, const float * v2);
	float	AngleBetweenVectors(  const float * v1,  const float * v2 );
}

float FallOffGrav( float time, float param )
{
  return -param * time * time;
}

float FallOffNone( float time, float param )
{
  return 0.0f;
}

float FallOffLin( float time, float param )
{
  return -param * time;
}

extern void bla( char *format, ... );

cBlood::~cBlood( )
{			
  s_bloodlist *cur = gFBlood;
  for( ; cur; cur = cur->next )
    if( cur->item == this )
      break;

  if( !cur )
    return;

  if( gFBlood == cur )
	{
		if( gLBlood == cur )
			gLBlood = NULL;
		gFBlood = cur->next;
		delete cur;
		cur = gFBlood;
	}
	else if( !cur->prev )
	{
		delete cur;
		cur = NULL;
	}
	else
	{
		s_bloodlist *prev = cur->prev;
		prev->next = cur->next;
		if( cur->next )
			cur->next->prev = prev;
		else
			gLBlood = prev;
		delete cur;
		cur = prev->next;
	}
	if( m_pList != NULL )
	{
		delete [] m_pList;
		m_pList = NULL;
	}
}

cBlood::cBlood( t_bloodpartinfo *info )
{
	m_iNumActive = 0;
	m_iActive = 0;
	m_fNextParticle = 0;
	m_fTime = 0;
	m_fTimeDiff = 0;
	m_pFree = NULL;
	m_pList = NULL;
	m_pList = NULL;

  int spridx = 0;
    
  if( strlen( info->sprite ) && ( spridx = SPR_Load( info->sprite ) ) )
    m_sSpr[0] = (model_s*)gEngfuncs.GetSpritePointer( spridx );
  else
    m_sSpr[0] = NULL;

  if( strlen( info->masksprite ) && ( spridx = SPR_Load( info->masksprite ) ) )
    m_sSpr[1] = (model_s*)gEngfuncs.GetSpritePointer( spridx );
  else
    m_sSpr[1] = NULL;
  
  if( strlen( info->detailsprite ) && ( spridx = SPR_Load( info->detailsprite ) ) )
    m_sSpr[2] = (model_s*)gEngfuncs.GetSpritePointer( spridx );
  else
    m_sSpr[2] = NULL;

  m_fParticleSizeX = info->size[0];
  m_fParticleSizeY = info->size[1];
  m_fParticleSpeed = info->speed;
  m_fParticleDelay = 1.0f / info->pps;
  m_iMaxNum = info->num;
  m_iNum = 0;
  m_fPartLife = info->partlife;
  m_fMaxLife = info->life;
  if( m_fMaxLife )
    lifeinit = false;
  else
    lifeinit = true;
  m_fDieTime = -1;
  partcolfade.Start( info->colfader, 0, 1 );
  partsizefade.Start( info->sizefader, 0, 1 );
  clip = info->clip;
  fps = info->fps;
  frames = info->frames;
  calcangle = info->calcangle;
  attachments = info->attachments;
  curattachent = 0;

  if( info->entidx > -1 )
    ent = gEngfuncs.GetEntityByIndex( info->entidx );
  else
    ent = NULL;

  mode = info->mode;

  switch( info->falloff )
  {
  case 1:
    FallOff = FallOffGrav;
    break;
  case 2:
    FallOff = FallOffLin;
    break;
  default:
    FallOff = FallOffNone;
    break;
  }

  m_fParticleSideSpread[0] = info->side[0];
  m_fParticleSideSpread[1] = info->side[1];
  m_fParticleSideSpread[2] = info->side[2];

  origin = info->org;
  dir = info->dir.Normalize( ) * info->speed;

  s_bloodlist *ni = new s_bloodlist;
  memset( ni, 0, sizeof(s_bloodlist) );
  ni->item = this;

  if( !gFBlood )
  {
    gLBlood = gFBlood = ni;
    return;
  }
  gLBlood->next = ni;
  ni->prev = gLBlood;
  gLBlood = ni;

  if( !m_sSpr[0] )
    delete this;
	//bla( "New: psec: %d, speed: %f, size: %f, spread: %f, type: %d\n", info->iPartsPerSec, info->fSpeed, info->fSize, info->fSideSpread, info->iType );
	//bla( "Set: delay: %f, speed: %f\n", m_fParticleDelay, m_fParticleSpeed );	
}

void cBlood::Init( )
{
	t_bloodparticle *init = NULL;
	int i = 0;

	m_iNumActive = 0;
	m_iActive = 0;
	m_fNextParticle = 0;
	m_fTime = 0;
	m_fTimeDiff = 0;
	//m_iMaxNum = 0;
	m_fMaxCheck = 17;

	//m_iMaxParticles = MAX_PARTICLES;
  m_iMaxParticles = ( m_iMaxNum ? m_iMaxNum : MAX_PARTICLES );

	if( m_pList != NULL )
	{
		delete [] m_pList;
		m_pList = NULL;
	}

	m_pList = new t_bloodparticle[m_iMaxParticles];
	m_pFree = m_pList;
	for( i = 0; i < m_iMaxParticles; i++ )
	{
		init = m_pList + i;
		memset( init, 0, sizeof(t_bloodparticle) );
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

void cBlood::Delete( int index, int free )
{
	t_bloodparticle *del = NULL;
	del = m_pList + index;

	if( del && !del->iActive )
		return;

	memset( del, 0, sizeof(t_bloodparticle) );

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

extern qboolean EV_IsLocal( int idx );
extern vec3_t PE_GetAttachment( model_s *mdl, int att );

void cBlood::StartNew( )
{
  if( m_iMaxNum && m_iNum >= m_iMaxNum )
    return;
  if( m_iNum >= MAX_PARTICLES )
    return;
	t_bloodparticle *start = NULL;
	pmtrace_t tr;
	vec3_t vEnd;
	vec3_t vDir;
  vec3_t up, forward, right;
  /*vec3_t angles = Vector( 
    gEngfuncs.pfnRandomFloat( -m_fParticleSideSpread[0], m_fParticleSideSpread[0] ),
    gEngfuncs.pfnRandomFloat( -m_fParticleSideSpread[1], m_fParticleSideSpread[1] ),
    gEngfuncs.pfnRandomFloat( -m_fParticleSideSpread[2], m_fParticleSideSpread[2] ) );*/

//  AngleVectors( angles, forward, right, up );
	//vDir = dir.x * forward + dir.y * right + dir.z * up;

  if( ent )
  {
    if( EV_IsLocal( ent->index ) )
      AngleVectors( v_angles, forward, right, up );
    else
      AngleVectors( ent->angles, forward, right, up );
    if( calcangle )
      vDir = Vector( forward.x * dir.x, forward.y * dir.y, forward.z * dir.z );
    else
      vDir = dir;
  }
  else
    vDir = dir;

  vDir = vDir.Normalize( );
 
  vDir.x += gEngfuncs.pfnRandomFloat( -m_fParticleSideSpread[0], m_fParticleSideSpread[0] );
  vDir.y += gEngfuncs.pfnRandomFloat( -m_fParticleSideSpread[1], m_fParticleSideSpread[1] );
  vDir.z += gEngfuncs.pfnRandomFloat( -m_fParticleSideSpread[2], m_fParticleSideSpread[2] );
 
	if( !m_pFree )
		return;
	
	start = m_pFree;
	if( m_pFree->pNext )
		m_pFree = m_pFree->pNext;
	else
		m_pFree = NULL;

  if( ent )
  {
    if( attachments )
    {
      int tmp = curattachent;
      vec3_t attorg;
      curattachent++;
      if( curattachent >= 3 )
        curattachent = 0;
      if( curattachent == tmp )
        return;
      while( origin[curattachent] == -1 )
      {
        curattachent++;
        if( curattachent >= 3 )
          curattachent = 0;
        if( curattachent == tmp )
          return;
      }
      /*attorg = PE_GetAttachment( ent->model, origin[curattachent] );
      float wtf = attorg.z;
      attorg.z = attorg.y + 45;
      attorg.y = wtf;
      attorg.x = -attorg.x;*/
      attorg = ent->attachment[(int)origin[curattachent]];
      start->vOrigin = attorg;//attorg.x * forward + attorg.y * right + attorg.z * up;
    }
    else
      start->vOrigin = ent->origin + origin.x * forward + origin.y * right + origin.z * up;
    if( EV_IsLocal( ent->index ) )
      start->vOrigin.z += 7;
  }
  else
    start->vOrigin = origin;

	/*for( int i = 0; i < 3; i++ )
		vEnd[i] = start->vOrigin[i] + 1000 * dir[i];*/
  start->fLifetime = m_fPartLife;
  if( clip )
  {
    vEnd = start->vOrigin + vDir * m_fParticleSpeed * m_fPartLife;
    tr = *(gEngfuncs.PM_TraceLine( (float *)&start->vOrigin, (float *)&vEnd, 0, 2 /*point sized hull*/, -1 ));
    if( tr.fraction < 1.0 )
      start->fLifetime = m_fPartLife * tr.fraction;
  }

  start->iActive = 1;
	start->vMove = vDir * m_fParticleSpeed;
	start->fNextMove = 0;
	start->fLifed = 0;
	start->pNext = NULL;
	start->sSprite = m_sSpr[0];
	start->sMaskSprite = m_sSpr[1];
	start->sDetailSprite = m_sSpr[2];
	start->iFrame = 0;
	start->fSizeX = m_fParticleSizeX;
	start->fSizeY = m_fParticleSizeY;
	m_iNumActive++;
  m_iNum++;
}

extern vec3_t ev_punchangle, v_origin;

void cBlood::Draw( )
{
	t_bloodparticle *check = NULL;

	if( !m_pList )
		return;

	vec3_t up, v_forward, v_right, v_up, orig;
  int r, g, b, a, c, d, e;
  float x, y;
  bool detailsprites = ( CVAR_GET_FLOAT( "cl_detailsprites" ) ? true : false );
	
	AngleVectors( v_angles + ev_punchangle, v_forward, v_right, v_up );

	
  if( mode == kRenderTransColor || ( mode == kRenderTransTexture && IEngineStudio.IsHardware( ) == 1 ) )
    gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );	
  else
    gEngfuncs.pTriAPI->RenderMode( mode );

	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
  gEngfuncs.pTriAPI->Brightness( 1.0f /*a / 255.0f*/ );


	/*if( m_iType == TYPE_RAIN || m_iType == TYPE_GLOBALRAIN )
	{
		up = Vector( 0, 0, 1 );
		gEngfuncs.pTriAPI->Color4f( 0.70, 0.70, 0.70 , 0.25 );
	}
	else*/
		up = v_up;

	for( int i = 0; i < m_iMaxParticles; i++ )
	{
		check = m_pList + i;
		if( !check->iActive )
			continue;

		 /*if( m_iType == TYPE_RAIN || m_iType == TYPE_GLOBALRAIN )
		 {
         orig = check->vOrigin;
         orig.z += m_fParticleSizeY;
				 up = Vector( 0, 0, 1 );
       }
		 }
     else*/
    orig = check->vOrigin;

    partcolfade.GetColor( ( check->fLifed / check->fLifetime ), r, g, b, a );
  	  gEngfuncs.pTriAPI->Color4f( r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f );
    if( mode == kRenderTransTexture )
	    gEngfuncs.pTriAPI->Brightness( a / 255.0f );
   
    partsizefade.GetColor( ( check->fLifed / check->fLifetime ), c, d, e, e );
    x = c / 100000.0f * check->fSizeX;
    y = d / 100000.0f * check->fSizeY;

    if( IEngineStudio.IsHardware( ) == 1 )
    {
      if( mode == kRenderTransTexture )
      {
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        if( !gEngfuncs.pTriAPI->SpriteTexture( check->sSprite, check->iFrame ) )
          return;
      }
      else if( mode == kRenderTransAdd || !detailsprites )
      {
        glEnable( GL_BLEND );
        glBlendFunc( GL_ONE, GL_ONE );
        if( !gEngfuncs.pTriAPI->SpriteTexture( check->sSprite, check->iFrame ) )
          return;
      }
      else if( mode == kRenderTransColor )
      {
        glEnable( GL_BLEND );
        if( check->sMaskSprite )
        {
	        gEngfuncs.pTriAPI->Brightness( a / 255.0f );
          gEngfuncs.pTriAPI->Color4f( 0, 0, 0, a / 255.0f );
          if( !gEngfuncs.pTriAPI->SpriteTexture( check->sMaskSprite, check->iFrame ) )
            return;
          glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
          glBegin( GL_QUADS );
            glTexCoord2f( 0, 1 );
            glVertex3fv( orig - v_right * x + up * y );
            glTexCoord2f( 1, 1 );
            glVertex3fv( orig + v_right * x + up * y );
            glTexCoord2f( 1, 0 );
            glVertex3fv( orig + v_right * x - up * y );
            glTexCoord2f( 0, 0 );
            glVertex3fv( orig - v_right * x - up * y );
          glEnd( );
        }
        glBlendFunc( GL_ONE, GL_ONE );
        gEngfuncs.pTriAPI->Color4f( r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f );
        if( !gEngfuncs.pTriAPI->SpriteTexture( check->sSprite, check->iFrame ) )
          return;
        if( check->sDetailSprite )
        {
          glBegin( GL_QUADS );
            glTexCoord2f( 0, 1 );
            glVertex3fv( orig - v_right * x + up * y );
            glTexCoord2f( 1, 1 );
            glVertex3fv( orig + v_right * x + up * y );
            glTexCoord2f( 1, 0 );
            glVertex3fv( orig + v_right * x - up * y );
            glTexCoord2f( 0, 0 );
            glVertex3fv( orig - v_right * x - up * y );
          glEnd( );
          gEngfuncs.pTriAPI->Color4f( 0, 0, 0, 1 );
          if( !gEngfuncs.pTriAPI->SpriteTexture( check->sDetailSprite, check->iFrame ) )
            return;
          glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        }
      }
      glBegin( GL_QUADS );
        glTexCoord2f( 0, 1 );
        glVertex3fv( orig - v_right * x + up * y );
        glTexCoord2f( 1, 1 );
        glVertex3fv( orig + v_right * x + up * y );
        glTexCoord2f( 1, 0 );
        glVertex3fv( orig + v_right * x - up * y );
        glTexCoord2f( 0, 0 );
        glVertex3fv( orig - v_right * x - up * y );
      glEnd( );
      glDisable( GL_BLEND );
    }
    else
    {
      if( !gEngfuncs.pTriAPI->SpriteTexture( check->sSprite, check->iFrame ) )
          return;
      gEngfuncs.pTriAPI->Begin( TRI_QUADS );
        gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
        gEngfuncs.pTriAPI->Vertex3fv( orig - v_right * x + up * y );
        gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
        gEngfuncs.pTriAPI->Vertex3fv( orig + v_right * x + up * y );
        gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
        gEngfuncs.pTriAPI->Vertex3fv( orig + v_right * x - up * y );
        gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
        gEngfuncs.pTriAPI->Vertex3fv( orig - v_right * x - up * y );
      gEngfuncs.pTriAPI->End( );
    }
		
	}
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );

}

#define MOD( a, b ) ( (a) - (int)( (a) / (b) ) * (b) )

void cBlood::Think( s_bloodlist **it, float fTime )
{
	t_bloodparticle *check = NULL;
	int i = 0;

	m_fTime += fTime;

	if( ( fTime > 0 ) && !m_iActive )
		Init( );

	if( fTime == 0 )
		return;

	if( !m_pList )
		return;

  if( !lifeinit )
  {
    m_fMaxLife += m_fTime;
    lifeinit = true;
  }

  if( ( m_iMaxNum && m_iNum >= m_iMaxNum && m_iNumActive <= 0 ) || ( m_fMaxLife && m_fMaxLife <= m_fTime ) )
  {
    delete this;
  	*it = gFBlood; // -.-'
    return;
  }

	if( m_fNextParticle <= m_fTime )
	{
		float tmp = max( fTime, m_fParticleDelay );
		for( i = 0; i < (int)( tmp / m_fParticleDelay ); i++ )
			StartNew( );
		m_fNextParticle = m_fTime + m_fParticleDelay;
	}
  vec3_t targ;
  pmtrace_t tr;

	for( i = 0; i < m_iMaxParticles; i++ )
	{
		check = m_pList + i;

		if( !check->iActive )
			continue;

    check->fLifed += fTime;
    
    check->vOrigin = check->vOrigin + check->vMove * fTime + Vector( 0.0f, 0.0f, FallOff( check->fLifed, 9.82 ) );
    if( frames > 1 )
		  check->iFrame = (int)( check->fLifed / ( 1.0f / fps ) ) % frames;
    
  	if( check->fLifed >= check->fLifetime )
		{
			Delete( i );
			continue;
		}
	}
	/*if( m_iNumActive > m_iMaxNum )
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
	}*/
}

extern list<t_bloodpartinfo> infol;

cBlood *cBlood::NewSpray( char *cfgfile, vec3_t origin, vec3_t dir, int entidx, float maxlife, int maxnum )
{
  if( !CVAR_GET_FLOAT( "cl_partsys" ) )
    return NULL;
	int i = 0;
  char file[128];
  snprintf( file, sizeof(file), "%s", cfgfile );
  t_bloodpartinfo info; 

  list<t_bloodpartinfo>::iterator it;
  for( it = infol.begin( ); it != infol.end( ); it++ )
  {
    if( !strcmp( file, &(*it->file) ) )
    {
      memcpy( &info, &(*it), sizeof(t_bloodpartinfo) );
      info.org.x = origin.x;
	    info.org.y = origin.y;
	    info.org.z = origin.z;

      info.dir.x = dir.x;
	    info.dir.y = dir.y;
	    info.dir.z = dir.z;
      if( info.entidx != -1 )
        info.entidx = entidx;
      if( info.dynlife )
        info.life = maxlife;
      if( info.dynnum )
        info.num = maxnum;
      return new cBlood( &info );
    }
  }
  
  info.life = 0;
  info.num = 0;
  info.partlife = 1;
  info.pps = 1;
  info.side[0] = 0;
  info.side[1] = 0;
  info.side[2] = 0;
  info.size[0] = 1;
  info.size[1] = 1;
  info.speed = 1;
  info.mode = kRenderTransAdd;
  info.entidx = -1;
  info.clip = false;
  info.frames = 1;
  info.fps = 1;
  info.calcangle = false;
  info.attachments = false;
  info.dynlife = false;
  info.dynnum = false;
  info.sprite[0] = '\0';
  info.masksprite[0] = '\0';
  info.detailsprite[0] = '\0';
  strcpy( info.sizefader, "null" );
  strcpy( info.colfader, "blood" );
  info.falloff = 3;

  info.org.x = origin.x;
	info.org.y = origin.y;
	info.org.z = origin.z;

  info.dir.x = dir.x;
	info.dir.y = dir.y;
	info.dir.z = dir.z;

  char token[1024] = "";
	char *pstart = (char*)gEngfuncs.COM_LoadFile( file, 5, NULL);
	char *pfile = pstart;
	if( !pfile )
		return NULL;

  snprintf( info.file, sizeof(info.file), "%s", file );
  pfile = gEngfuncs.COM_ParseFile( pfile, token );
	while( strlen( token ) )
	{
		if( !strcmp( token, "pps" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      info.pps = atoi( token );
		}
		else if( !strcmp( token, "num" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      if( !strcmp( token, "dynamic" ) )
      {
        info.dynnum = true;
        info.num = maxnum;
      }
      else
        info.num = atoi( token );
		}
		else if( !strcmp( token, "spd" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      info.speed = atoi( token );
		}
		else if( !strcmp( token, "spr" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      snprintf( info.sprite, sizeof(info.sprite), "%s", token );
		}
		else if( !strcmp( token, "maskspr" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      snprintf( info.masksprite, sizeof(info.masksprite), "%s", token );
		}
		else if( !strcmp( token, "detailspr" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      snprintf( info.detailsprite, sizeof(info.detailsprite), "%s", token );
		}
		else if( !strcmp( token, "size" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      info.size[0] = atof( token );
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      info.size[1] = atof( token );
		}
		else if( !strcmp( token, "side" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      info.side[0] = atof( token );
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      info.side[1] = atof( token );
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      info.side[2] = atof( token );
		}
		else if( !strcmp( token, "maxlife" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      if( !strcmp( token, "dynamic" ) )
      {
        info.dynlife = true;
        info.life = maxlife;
      }
      else
        info.life = atof( token );
		}
		else if( !strcmp( token, "partlife" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      info.partlife = atof( token );
		}
		else if( !strcmp( token, "colfader" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
		  snprintf( info.colfader, sizeof(info.colfader), "%s", token );
		}
		else if( !strcmp( token, "sizefader" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      snprintf( info.sizefader, sizeof(info.sizefader), "%s", token );
		}
		else if( !strcmp( token, "mode" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      if( !strcmp( token, "normal" ) )
        info.mode = kRenderNormal;
      else if( !strcmp( token, "detailed" ) )
        info.mode = kRenderTransColor;
      else if( !strcmp( token, "transtex" ) )
        info.mode = kRenderTransTexture;
      else if( !strcmp( token, "glow" ) )
        info.mode = kRenderGlow;
      else if( !strcmp( token, "transalpha" ) )
        info.mode = kRenderTransAlpha;
      else if( !strcmp( token, "transadd" ) )
        info.mode = kRenderTransAdd;
		}
		else if( !strcmp( token, "falloff" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      if( !strcmp( token, "none" ) )
        info.falloff = 3;
      else if( !strcmp( token, "gravity" ) )
        info.falloff = 1;
      else if( !strcmp( token, "linear" ) )
        info.falloff = 2;
		}
		else if( !strcmp( token, "relative" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      if( !strcmp( token, "ent" ) )
        info.entidx = ( entidx == 0 ? -1 : entidx );
      else if( !strcmp( token, "att" ) )
      {
        info.entidx = ( entidx == 0 ? -1 : entidx );
        info.attachments = true;
      }
		}
		else if( !strcmp( token, "clip" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      if( !strcmp( token, "true" ) )
        info.clip = true;
      else
        info.clip = false;
		}
		else if( !strcmp( token, "frames" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      info.frames = atoi(token);
		}
		else if( !strcmp( token, "fps" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      info.fps = atoi(token);
		}
		else if( !strcmp( token, "calcangle" ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
      if( !strcmp( token, "true" ) )
        info.calcangle = true;
      else
        info.calcangle = false;
		}
		else
		{
			ConsolePrint( "Unknown partsys cfg file token: " );
			ConsolePrint( token );
			ConsolePrint( "in file " );
			ConsolePrint( file );
			ConsolePrint( "\n" );
		}
  	pfile = gEngfuncs.COM_ParseFile( pfile, token );
	}
	gEngfuncs.COM_FreeFile( pstart );

  infol.push_front( info );
  return new cBlood( &info );
}

void cBlood::KillAll( s_bloodlist *bldlist )
{
  s_bloodlist *tmp;
  while( bldlist )
  {
    tmp = bldlist->next;
    delete bldlist;
    bldlist = tmp;
  }
  gFBlood = gLBlood = NULL;
}