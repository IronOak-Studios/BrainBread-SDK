
#define TYPE_RAIN		1
#define	TYPE_SNOW		2
#define TYPE_GLOBALRAIN		3
#define	TYPE_GLOBALSNOW		4

typedef struct s_particle
{
	vec3_t		vOrigin;
	vec3_t		vTarget;
	vec3_t		vMove;
	float		fNextMove;
	float		fRemove;
	float		fLastDist;
	s_particle	*pNext;
	int			iActive;
	int			iIndex;
	int			iSprite;
	int			iFrame;
	float		fSizeX;
	float		fSizeY;
	bool		bImpact;
} t_particle;

typedef struct s_partinfo
{
	int		iType;
	int		iPartsPerSec;
	float	fSpeed;
	float	fSize;
	float	fSideSpread;
} t_partinfo;

class cRain
{
public:
	cRain( int index, t_partinfo *info );
	virtual ~cRain( );
	virtual void		Init( );
	virtual void		Delete( int index, int free = 1 );
	virtual void		StartNew( );
	virtual void		Draw( );
	virtual void		Think( float fTime );

	float			m_fParticleDelay;
	float			m_fParticleSpeed;
	float			m_fParticleSizeX;
	float			m_fParticleSizeY;
	float			m_fParticleSideSpread;

	int				m_iSprFall;
	int				m_iSprImpact;

	int				m_iMaxNum;

	int				m_iRainIndex;
	int				m_iNumActive;
	int				m_iActive;
	int				m_iMaxParticles;
	int				m_iType;

	float			m_fNextParticle;
	float			m_fMaxCheck;
	float			m_fTime;
	float			m_fTimeDiff;

  int       nexttrace;

  float grndz;
	t_particle		*m_pFree;
	t_particle		*m_pList;
};

class cGlobalRain : public cRain
{
  vec3_t vmax;
public:
  cGlobalRain( int index, t_partinfo *info );
	virtual void  Think( float fTime );
};