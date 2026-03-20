/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#ifndef PLAYER_H
#define PLAYER_H


#include "pm_materials.h"
#include "effects.h"
#include <list>
using namespace std;


struct s_spawnlist
{
	CBasePlayer* player;
	CBaseEntity* spot;
};

#define POINT_START 30
#define POINT_LIMIT1 40
#define POINT_LIMIT2 45
#define POINT_MAX 50

#define TASK_KILL 1
#define TASK_HACK 2
#define TASK_SAFE 3
#define TASK_SPAWN 4
#define TASK_SPAWN_RESTARTROUND 5
#define TASK_SPAWN_DELAYED 6
#define TASK_TEAMCHANGE 7
#define TASK_VICTIM 8
#define TASK_OVER_LIMIT1 9
#define TASK_OVER_LIMIT2 10
#define TASK_TEAMKILL 11

#undef PE_CROSS_CALM
#undef PE_CROSS_CALM_SLOW

#define PE_CROSS_CALM(a) (0.004/(PLR_DMG(a)/100.0f))
#define PE_CROSS_CALM_SLOW(a) (PE_CROSS_CALM(a)*2)
#define PE_CROSS_SHOT_LIMIT 30

#define MAX_SPAWN_SPOTS 32

extern s_spawnlist gListCorp[MAX_SPAWN_SPOTS];
extern s_spawnlist gListSynd[MAX_SPAWN_SPOTS];
extern s_spawnlist gListBackup[MAX_SPAWN_SPOTS];

#define PLAYER_FATAL_FALL_SPEED		1024// approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED	580// approx 20 feet
#define DAMAGE_FOR_FALL_SPEED		(float) 100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )// damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED		200
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.

//
// Player PHYSICS FLAGS bits
//
#define		PFLAG_ONLADDER		( 1<<0 )
#define		PFLAG_ONSWING		( 1<<0 )
#define		PFLAG_ONTRAIN		( 1<<1 )
#define		PFLAG_ONBARNACLE	( 1<<2 )
#define		PFLAG_DUCKING		( 1<<3 )		// In the process of ducking, but totally squatted yet
#define		PFLAG_USING			( 1<<4 )		// Using a continuous entity
#define		PFLAG_OBSERVER		( 1<<5 )		// player is locked in stationary cam mode. Spectators can move, observers can't.

//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
#define CSUITPLAYLIST	4		// max of 4 suit sentences queued up at any time

#define SUIT_GROUP			TRUE
#define	SUIT_SENTENCE		FALSE

#define	SUIT_REPEAT_OK		0
#define SUIT_NEXT_IN_30SEC	30
#define SUIT_NEXT_IN_1MIN	60
#define SUIT_NEXT_IN_5MIN	300
#define SUIT_NEXT_IN_10MIN	600
#define SUIT_NEXT_IN_30MIN	1800
#define SUIT_NEXT_IN_1HOUR	3600

#define CSUITNOREPEAT		32

#define	SOUND_FLASHLIGHT_ON		"items/flashlight1.wav"
#define	SOUND_FLASHLIGHT_OFF	"items/flashlight1.wav"

#define TEAM_NAME_LENGTH	16

#define AR_HEAD		0
#define AR_CHEST	1
#define AR_STOMACH	2
#define AR_L_ARM	3
#define AR_R_ARM	4
#define AR_R_LEG	5
#define AR_L_LEG	6

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
} PLAYER_ANIM;

#define MAX_ID_RANGE 2048
#define SBAR_STRING_SIZE 128

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_END,
};

#define CHAT_INTERVAL 1.0f

enum _Menu
{
	Intro,
	Menu_Radio1,
	Menu_Radio2,
	Menu_Radio3,

	Menu_Team_Change,
	Menu_Instant,

	Menu_Equip,
	Menu_Hacker,
	Menu_Class,
	Menu_Class_Change,
	Menu_MOTD,
	Menu_VoteMap,
	Menu_VoteVar,
};

typedef struct
{
  vec3_t start;
  vec3_t end;
  float time;
  int dmgtype;
} t_flame_part;


#define AD_TORSO_HEART	(1<<0)
#define AD_TORSO_BOMB	(1<<1)
#define AD_ARMS_FAST	(1<<2)
#define AD_ARMS_SMART	(1<<3)
#define AD_LEGS_ACHIL	(1<<4)
#define AD_HEAD_SENTI	(1<<5)
#define AD_HEAD_ORBITAL	(1<<6)
#define AD_NANO_FACT	(1<<7)
#define AD_NANO_SKIN	(1<<8)

struct s_radarentry
{
	int entindex;
	bool disabled;
	vec3_t origin;
	vec3_t clientorigin;
	int color;
	int clientcolor;

	bool override;
	bool clientoverride;

	entvars_t *pev;

	s_radarentry *prev;
	s_radarentry *next;
};

#define HEALTH_GIVE ( 8 / ( diff.value ? diff.value : 1 ) )
#define SPEED_GIVE ( 2.5 / ( diff.value ? diff.value : 1 ) )
#define DMG_GIVE ( 3.75 / ( diff.value ? diff.value : 1 ) )

#define BASE_HP_VALUE ( 80 / ( diff.value ? diff.value : 1 ) )
#define BASE_SPEED_VALUE ( 90 / ( diff.value ? diff.value : 1 ) )
#define BASE_DMG_VALUE ( 85 / ( diff.value ? diff.value : 1 ) )

#define PLR_SPEED(a) ((BASE_SPEED_VALUE+(a)->speedpnts*SPEED_GIVE)*( (a)->statsRatio ? (a)->statsRatio : 1 ))
#define PLR_HP(a) ((BASE_HP_VALUE+(a)->hppnts*HEALTH_GIVE)*( (a)->statsRatio ? (a)->statsRatio : 1 ))
#define PLR_DMG(a) ((BASE_DMG_VALUE+(a)->dmgpnts*DMG_GIVE)*( (a)->statsRatio ? (a)->statsRatio : 1 ))

class CBasePlayer : public CBaseMonster
{
public:
	CBasePlayer();
	int					random_seed;    // See that is shared between client & server for shared weapons code

	int					m_iPlayerSound;// the index of the sound list slot reserved for this player
	int					m_iTargetVolume;// ideal sound volume. 
	int					m_iWeaponVolume;// how loud the player's weapon is right now.
	int					m_iExtraSoundTypes;// additional classification for this weapon's sound
	int					m_iWeaponFlash;// brightness of the weapon flash
	float				m_flStopExtraSoundTime;
	
	float				m_flFlashLightTime;	// Time until next battery draw/Recharge
	int					m_iFlashBattery;		// Flashlight Battery Draw

	int					m_afButtonLast;
	int					m_afButtonPressed;
	int					m_afButtonReleased;
	
	edict_t			   *m_pentSndLast;			// last sound entity to modify player room type
	float				m_flSndRoomtype;		// last roomtype set by sound entity
	float				m_flSndRange;			// dist from player to sound entity

	float				m_flFallVelocity;
	
	int					m_rgItems[MAX_ITEMS];
	int					m_fKnownItem;		// True when a new item needs to be added
	int					m_fNewAmmo;			// True when a new item has been added

	unsigned int		m_afPhysicsFlags;	// physics flags - set when 'normal' physics should be revisited or overriden
	float				m_fNextSuicideTime; // the time after which the player can next use the suicide command


// these are time-sensitive things that we keep track of
	float				m_flTimeStepSound;	// when the last stepping sound was made
	float				m_flTimeWeaponIdle; // when to play another weapon idle animation.
	float				m_flSwimTime;		// how long player has been underwater
	float				m_flDuckTime;		// how long we've been ducking
	float				m_flWallJumpTime;	// how long until next walljump

	float				m_flSuitUpdate;					// when to play next suit update
	int					m_rgSuitPlayList[CSUITPLAYLIST];// next sentencenum to play for suit update
	int					m_iSuitPlayNext;				// next sentence slot for queue storage;
	int					m_rgiSuitNoRepeat[CSUITNOREPEAT];		// suit sentence no repeat list
	float				m_rgflSuitNoRepeatTime[CSUITNOREPEAT];	// how long to wait before allowing repeat
	int					m_lastDamageAmount;		// Last damage taken
	float				m_tbdPrev;				// Time-based damage timer

	float				m_flgeigerRange;		// range to nearest radiation source
	float				m_flgeigerDelay;		// delay per update of range msg to client
	int					m_igeigerRangePrev;
	int					m_iStepLeft;			// alternate left/right foot stepping sound
	char				m_szTextureName[CBTEXTURENAMEMAX];	// current texture name we're standing on
	char				m_chTextureType;		// current texture type

	int					m_idrowndmg;			// track drowning damage taken
	int					m_idrownrestored;		// track drowning damage restored

	int					m_bitsHUDDamage;		// Damage bits for the current fame. These get sent to 
												// the hude via the DAMAGE message
	BOOL				m_fInitHUD;				// True when deferred HUD restart msg needs to be sent
	BOOL				m_fGameHUDInitialized;
	int					m_iTrain;				// Train control position
	BOOL				m_fWeapon;				// Set this to FALSE to force a reset of the current weapon HUD info

	EHANDLE				m_pTank;				// the tank which the player is currently controlling,  NULL if no tank
	float				m_fDeadTime;			// the time at which the player died  (used in PlayerDeathThink())

	BOOL			m_fNoPlayerSound;	// a debugging feature. Player makes no sound if this is true. 
	BOOL			m_fLongJump; // does this player have the longjump module?

	float       m_tSneaking;
	int			m_iUpdateTime;		// stores the number of frame ticks before sending HUD update messages
	int			m_iClientHealth;	// the health currently known by the client.  If this changes, send a new
	int			m_iClientBattery;	// the Battery currently known by the client.  If this changes, send a new
	int			m_iHideHUD;		// the players hud weapon info is to be hidden
	int			m_iClientHideHUD;
	float		m_fClientRecoilratio;
	int			m_iFOV;			// field of view
	int			m_iClientFOV;	// client's known FOV
	// usable player items 
	CBasePlayerItem	*m_rgpPlayerItems[MAX_ITEM_TYPES];
	CBasePlayerItem *m_pActiveItem;
	CBasePlayerItem *m_pClientActiveItem;  // client version of the active item
	CBasePlayerItem *m_pLastItem;
	// shared ammo slots
	int	m_rgAmmo[MAX_AMMO_SLOTS];
	int	m_rgAmmoLast[MAX_AMMO_SLOTS];

	Vector				m_vecAutoAim;
	BOOL				m_fOnTarget;
	int					m_iDeaths;
	float				m_iRespawnFrames;	// used in PlayerDeathThink() to make sure players can always respawn

	int m_lastx, m_lasty;  // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames;// Custom clan logo frames for this player
	float	m_flNextDecalTime;// next time this player can spray a decal

	char m_szTeamName[TEAM_NAME_LENGTH];

	virtual void Spawn( void );
	void Pain( void );

//	virtual void Think( void );
	virtual void Jump( void );
	virtual void Duck( void );
	virtual void PreThink( void );
	virtual void PostThink( void );
	virtual Vector GetGunPosition( void );
	virtual int TakeHealth( float flHealth, int bitsDamageType );
	virtual void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual void	Killed( entvars_t *pevAttacker, int iGib );
	virtual Vector BodyTarget( const Vector &posSrc ) { return Center( ) + pev->view_ofs * RANDOM_FLOAT( 0.5, 1.1 ); };		// position to shoot at
	virtual void StartSneaking( void ) { m_tSneaking = gpGlobals->time - 1; }
	virtual void StopSneaking( void ) { m_tSneaking = gpGlobals->time + 30; }
	virtual BOOL IsSneaking( void ) { return m_tSneaking <= gpGlobals->time; }
	virtual BOOL IsAlive( void ) { return (pev->deadflag == DEAD_NO) && pev->health > 0; }
	virtual BOOL ShouldFadeOnDeath( void ) { return FALSE; }
	virtual	BOOL IsPlayer( void ) { return TRUE; }			// Spectators should return FALSE for this, they aren't "players" as far as game logic is concerned

	virtual BOOL IsNetClient( void ) { return TRUE; }		// Bots should return FALSE for this, they can't receive NET messages
															// Spectators should return TRUE for this
	virtual const char *TeamID( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	void RenewItems(void);
	void PackDeadPlayerItems( void );
	void RemoveAllItems( BOOL removeSuit );
	BOOL SwitchWeapon( CBasePlayerItem *pWeapon );

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void UpdateClientData( void );
	
	static	TYPEDESCRIPTION m_playerSaveData[];

	// Player is moved across the transition by other means
	virtual int		ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void	Precache( void );
	BOOL			IsOnLadder( void );
	BOOL			FlashlightIsOn( void );
	void			FlashlightTurnOn( void );
	void			FlashlightTurnOff( void );
	
	void UpdatePlayerSound ( void );
	void DeathSound ( void );

	int Classify ( void );
	void SetAnimation( PLAYER_ANIM playerAnim );
	void SetWeaponAnimType( const char *szExtention );
	char m_szAnimExtention[32];

	// custom player functions
	virtual void ImpulseCommands( void );
	void CheatImpulseCommands( int iImpulse );

	void StartDeathCam( void );
	void StartObserver( Vector vecPosition, Vector vecViewAngle );

	void AddPoints( int score, BOOL bAllowNegativeScore );
	void AddPointsToTeam( int score, BOOL bAllowNegativeScore );
	BOOL AddPlayerItem( CBasePlayerItem *pItem );
	BOOL RemovePlayerItem( CBasePlayerItem *pItem );
	void DropPlayerItem ( char *pszItemName );
	BOOL HasPlayerItem( CBasePlayerItem *pCheckItem );
	BOOL HasNamedPlayerItem( const char *pszItemName );
	BOOL HasWeapons( void );// do I have ANY weapons?
	void SelectPrevItem( int iItem );
	void SelectNextItem( int iItem );
	void SelectLastItem(void);
	void SelectItem(const char *pstr);
	void ItemPreFrame( void );
	void ItemPostFrame( void );
	void GiveNamedItem( const char *szName );
	void EnableControl(BOOL fControl);

	int  GiveAmmo( int iAmount, char *szName, int iMax );
	void SendAmmoUpdate(void);

	void WaterMove( void );
	void EXPORT PlayerDeathThink( void );
	void PlayerUse( void );

	void CheckSuitUpdate();
	void SetSuitUpdate(char *name, int fgroup, int iNoRepeat);
	void UpdateGeigerCounter( void );
	void CheckTimeBasedDamage( void );

	BOOL FBecomeProne ( void );
	void BarnacleVictimBitten ( entvars_t *pevBarnacle );
	void BarnacleVictimReleased ( void );
	static int GetAmmoIndex(const char *psz);
	int AmmoInventory( int iAmmoIndex );
	int Illumination( void );

	void ResetAutoaim( void );
	Vector GetAutoaimVector( float flDelta  );
	Vector AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta  );

	void ForceClientDllUpdate( void );  // Forces all client .dll specific data to be resent to client.

	void DeathMessage( entvars_t *pevKiller );

	void SetCustomDecalFrames( int nFrames );
	int GetCustomDecalFrames( void );

	void CBasePlayer::TabulateAmmo( void );

	float m_flStartCharge;
	float m_flAmmoStartCharge;
	float m_flPlayAftershock;
	float m_flNextAmmoBurn;// while charging, when to absorb another unit of player's ammo?

	void UpdateRadar( );
	void InitRadar( );
	void ResetRadar( );
	void Enable( entvars_t *entvars );
	void Disable( entvars_t *entvars );
	void SetColor( entvars_t *entvars, int color );
	float m_flNextRadarCalc;
	int m_iNextInit;
	s_radarentry *fradar;
	s_radarentry *lradar;
	entvars_t *m_iCase;

    BOOL		m_fNVG;            
    BOOL		m_fNVGActive; 
	void		NVGUpdate();
    void		NVGActive(BOOL Active);
	float		m_flNVGUpdate;
    float		m_flInfraredUpdate;
    void		NVGCreateInfrared(BOOL fOn);

	// Spin
	int					m_iTeam;
	int					m_iClass;
	int					m_iModel;
	int					m_bCanRespawn;
	void				StartObserver( );
	void				StopObserver( void );
    void				Observer_FindNextPlayer( bool bReverse );
    void				Observer_HandleButtons();
    void				Observer_SetMode( int iMode );
    EHANDLE				m_hObserverTarget;
    float				m_flNextObserverInput;
    int					IsObserver() { return pev->iuser1; };
	struct
	{
		int team;
		int slot[7];
	} m_sInfo, m_sInfoBack;
	int					m_iPoints;
	float				m_fPointsMax;
	int					m_iClientPointsMax;
	int					m_iNumMates;
	int					m_iCurNote;
	int					m_iCurMidNote;
	float				m_fCurMidNoteEnd;
	int					m_iMate[24];
	int					m_iCurSlot;
	int					m_iSlots[3];
	const char			*m_sAmmoSlot[4];
	const char			*m_sAmmo_aSlot[4];
	float				m_fCntEndTime;
	int					observing;
	_Menu				m_nMenu;
	int					m_iHUDInited;
	int					m_bIsSpecial;
	float				m_fTaskEnd;
	int					m_bDoneTask;
	int					m_bIsInZone;

	int					m_iArmor[8];
	int					m_iArmorMax[8];
	void				UpdateArmor( );
	int					m_bSpeciallyNotified;
	int					FadeOnDMG( );
	int					m_iPrevClass;
	int					m_bEquiped;
	/*float				speed;
	float				hp;
  float				dmg;*/
	int				  speedpnts;
	int			  	hppnts;
  int		  		dmgpnts;
  int		  		level;
  int		  		clientlevel;

	int					m_iShots;
	float				m_fNextCalm;
	float				m_fSpread;
	float				m_fViewCheck;
	float				m_fDisplayTill;
	int					m_iLastSpot;
	int					m_iFlare[1024];

	int					m_iCyber;
	int					m_iAddons;
	float				m_fNextHP;
	float				m_fNextAF;
	float				m_fNextBullet;

	char				m_sLastWeap[1024];
	int					m_iPrimaryWeap;

	int GetHealth( ) { return pev->health; }
	int GetArmor( ) { return pev->armorvalue; }
	int GetSpeed( ) { return pev->speed; }
	
	//Player ID
	void InitStatusBar( void );
	void UpdateStatusBar( void );
	int m_izSBarState[ SBAR_END ];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[ SBAR_STRING_SIZE ];
	char m_SbarString1[ SBAR_STRING_SIZE ];
	
	float m_flNextChatTime;
	
	void AddDmgDone( int index, float amount, int hitgroup );
	void AddDmgReceived( int from_index, float amount, int hitgroup );

	struct _dmginfo
	{
		int index;
		char name[128];
		float damage;
		int hits;
		int hitgroup[8];
	} dmgdone[32], dmgreceived[32];

	float	m_fNextInfo;
	int		m_iCurInfo;
	virtual int		BloodColor( void ) { return BLOOD_COLOR_RED; }

	void		BonusPoint( int task );

	float		m_fSpawn;
	int			m_iSpawnDelayed;
	float		m_fStartBeam;
	CBeam*		m_pBeam;
	CBeam*		m_pNoise;
	CBeam*		m_pNoise2;
	entvars_t	m_pBeamOrig;
	entvars_t*	m_pOrbital;

	int			m_iBlinks;
	int			m_iMusicOn;
	int			m_bNoAutospawn;
	float		m_fNextAutospawn;
	char		m_sModel[128];

	CBasePlayerItem *ItemByName( const char *pstr );
	int			m_iLastGrenMode;
	BOOL		m_bHelp;
  BOOL    slot1f;
  BOOL    slot2f;
  BOOL    slot3f;

  float m_fTransformTime;
  BOOL m_bTransform;

  list<t_flame_part> flame;
  float nextpop;
  float nextburn;
  float m_fBurning;
  float m_fNextSpread;
  int clientteam;
  list<vec3_t> pickupused;
  entvars_t *pevFlamer;

	void GiveExp( float value, bool load = false );
  void UpdateStats( int type );

  float zombieKills;
  bool escaped;
  bool  rehbutton;
  int clientFrags;
  int clientDeaths;
  float statsRatio;
};

#define AUTOAIM_2DEGREES  0.0348994967025
#define AUTOAIM_5DEGREES  0.08715574274766
#define AUTOAIM_8DEGREES  0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669


extern int	gmsgHudText;
extern BOOL gInitHUD;

#endif // PLAYER_H
