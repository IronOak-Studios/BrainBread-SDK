#ifndef SUMMARYPANEL_H
#define SUMMARYPANEL_H

#include<VGUI_Panel.h>
#include<VGUI_Label.h>
#include "../game_shared/vgui_grid.h"

#define SUMMARY_MAX_PLAYERS		32
#define SUMMARY_NUM_COLUMNS		7	// Name, Score, Kills, Headshots, Damage, XP, Escaped

// Round outcome codes (matches m_iRestart on the server)
#define ROUND_OUTCOME_HUMANS	1
#define ROUND_OUTCOME_ZOMBIES	2

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Client-side data for a single player's round stats
//-----------------------------------------------------------------------------
struct RoundPlayerEntry_t
{
	int		index;			// player slot (1-based)
	int		zombieKills;
	int		playerKills;
	int		headshots;
	int		damage;
	int		xpEarned;
	bool	escaped;
};

//-----------------------------------------------------------------------------
// Purpose: Panel displayed during intermission showing per-player round stats
//-----------------------------------------------------------------------------
class SummaryPanel : public Panel
{
public:
	SummaryPanel( int x, int y, int wide, int tall );

	void Initialize( void );
	void Open( void );
	void SetHeader( int outcome, int mvpIndex );
	void AddPlayerEntry( RoundPlayerEntry_t &entry );
	bool HasData( void ) { return m_iNumEntries > 0; }

private:
	void Rebuild( void );
	virtual void paintBackground( void );

	Label		m_TitleLabel;
	Label		m_SubtitleLabel;

	// Column headers
	CGrid		m_HeaderGrid;
	Label		m_HeaderLabels[SUMMARY_NUM_COLUMNS];

	// Player rows (pre-allocated for max players)
	CGrid		m_RowGrids[SUMMARY_MAX_PLAYERS];
	Label		m_RowEntries[SUMMARY_NUM_COLUMNS][SUMMARY_MAX_PLAYERS];

	// Data
	RoundPlayerEntry_t	m_Entries[SUMMARY_MAX_PLAYERS];
	int					m_iNumEntries;
	int					m_iOutcome;		// ROUND_OUTCOME_*
	int					m_iMVPIndex;	// player index (1-based) of the MVP
	bool				m_bHasData;
};

#endif
