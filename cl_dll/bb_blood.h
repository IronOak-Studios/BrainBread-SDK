#include "pe_fader.h"

typedef struct s_bloodparticle
{
	vec3_t		vOrigin;
	vec3_t		vMove;
	float		fNextMove;
	float		fLifed;
	float		fLifetime;
	float		fLastDist;
	s_bloodparticle	*pNext;
	int			iActive;
	int			iIndex;
	model_s	*sSprite;
	model_s	*sMaskSprite;
	model_s	*sDetailSprite;
	int			iFrame;
	float		fSizeX;
	float		fSizeY;
} t_bloodparticle;

typedef struct s_bloodpartinfo
{
  bool dynnum;
	int		num;
	int		pps;
	int		mode;
	int		falloff;
  int   entidx;
  int   frames;
  int   fps;
  float size[2];
  float side[3];
  bool dynlife;
  float life;
  float partlife;
	char sprite[128];
	char masksprite[128];
	char detailsprite[128];
	char file[128];
	char colfader[MAX_FADERNAME_LEN];
	char sizefader[MAX_FADERNAME_LEN];
	float	speed;
	bool	clip;
	bool	calcangle;
	bool	attachments;
	vec3_t	dir;
	vec3_t	org;
} t_bloodpartinfo;

struct s_bloodlist;
class cBlood
{
public:
	cBlood( t_bloodpartinfo *info );
	virtual ~cBlood( );
	virtual void		Init( );
	static void		KillAll( s_bloodlist *list );
	static cBlood *NewSpray( char *cfgfile, vec3_t origin, vec3_t dir, int entidx = -1, float maxlife = 1, int maxnum = 1 );
	virtual void		Delete( int index, int free = 1 );
	virtual void		StartNew( );
	virtual void		Draw( );
	virtual void		Think( s_bloodlist **it, float fTime );

	float			m_fParticleDelay;
	float			m_fParticleSpeed;
	float			m_fParticleSizeX;
	float			m_fParticleSizeY;
	float			m_fParticleSideSpread[3];

  cl_entity_s *ent;

  vec3_t origin;
  vec3_t dir;

  cPEFader  partcolfade;
  cPEFader  partsizefade;

	model_s		*m_sSpr[5];

  int       mode;

	int				m_iMaxNum;
	int				m_iNum;

	int				m_iNumActive;
	int				m_iActive;
	int				m_iMaxParticles;
	int				m_iType;

	float			m_fMaxLife;
	float			m_fDieTime;
	float			m_fPartLife;

  float			m_fNextParticle;
	float			m_fMaxCheck;
	float			m_fTime;
	float			m_fTimeDiff;

  int fps;
  int frames;
	t_bloodparticle		*m_pFree;
	t_bloodparticle		*m_pList;

  bool clip;
  bool calcangle;
  bool attachments;
  bool lifeinit;
  int curattachent;

  float     (*FallOff)( float time, float param );
};


struct s_bloodlist
{
  cBlood *item;
  s_bloodlist *next;
  s_bloodlist *prev;
};

extern s_bloodlist *gFBlood;
extern s_bloodlist *gLBlood;