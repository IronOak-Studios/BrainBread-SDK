//=============================================================================
//
// Purpose: VGUI panel showing a round summary during intermission.
//          Displays per-player stats: zombie kills, player kills, headshots,
//          damage, XP, escaped. Highlights the MVP (highest damage dealer).
//
//=============================================================================

#include<VGUI_LineBorder.h>

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_SummaryPanel.h"
#include "vgui_helpers.h"

#include <string.h>
#include <stdio.h>

// Column layout
struct SummaryColumnInfo
{
	const char		*title;
	int				weight;		// proportional width weight
	Label::Alignment align;
};

// Column proportions (relative weights, not pixel widths).
// The actual pixel widths are computed at runtime to fill the panel.
static SummaryColumnInfo g_SumColumns[SUMMARY_NUM_COLUMNS] =
{
	{ "Player",		28,		Label::a_west },
	{ "Score",		10,		Label::a_east },
	{ "Kills",		10,		Label::a_east },
	{ "Headshots",	12,		Label::a_east },
	{ "Damage",		14,		Label::a_east },
	{ "XP",			12,		Label::a_east },
	{ "Escaped",	14,		Label::a_center },
};

// Helper: sort entries by frags (score) descending, damage as tiebreaker
static int CompareEntries( const void *a, const void *b )
{
	const RoundPlayerEntry_t *ea = (const RoundPlayerEntry_t *)a;
	const RoundPlayerEntry_t *eb = (const RoundPlayerEntry_t *)b;

	// Use frags from the scoreboard data (matches MVP criteria)
	int fragsA = g_PlayerExtraInfo[ea->index].frags;
	int fragsB = g_PlayerExtraInfo[eb->index].frags;
	if( fragsB != fragsA )
		return fragsB - fragsA;
	return (int)eb->damage - (int)ea->damage;
}

//-----------------------------------------------------------------------------
SummaryPanel::SummaryPanel( int x, int y, int wide, int tall )
	: Panel( x, y, wide, tall ),
	  m_TitleLabel( "" ),
	  m_SubtitleLabel( "" ),
	  m_HeaderGrid()
{
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Scoreboard Title Text" );
	SchemeHandle_t hSmallScheme = pSchemes->getSchemeHandle( "Scoreboard Small Text" );
	SchemeHandle_t hTextScheme  = pSchemes->getSchemeHandle( "Scoreboard Text" );
	Font *titleFont = pSchemes->getFont( hTitleScheme );
	Font *smallFont = pSchemes->getFont( hSmallScheme );
	Font *textFont  = pSchemes->getFont( hTextScheme );

	setBgColor( 0, 0, 0, 96 );

	LineBorder *border = new LineBorder( Color( 60, 60, 60, 128 ) );
	setBorder( border );
	setPaintBorderEnabled( true );

	// --- Title ("MISSION COMPLETE" / "MISSION FAILED" / etc.) ---
	m_TitleLabel.setFont( titleFont );
	m_TitleLabel.setText( "" );
	m_TitleLabel.setBgColor( 0, 0, 0, 255 );
	m_TitleLabel.setFgColor( 255, 220, 0, 0 );
	m_TitleLabel.setContentAlignment( Label::a_center );
	m_TitleLabel.setContentFitted( false );
	m_TitleLabel.setBounds( 0, YRES(4), wide, YRES(22) );
	m_TitleLabel.setParent( this );

	// --- Subtitle (MVP line) ---
	m_SubtitleLabel.setFont( smallFont );
	m_SubtitleLabel.setText( "" );
	m_SubtitleLabel.setBgColor( 0, 0, 0, 255 );
	m_SubtitleLabel.setFgColor( 200, 200, 200, 0 );
	m_SubtitleLabel.setContentAlignment( Label::a_center );
	m_SubtitleLabel.setContentFitted( false );
	m_SubtitleLabel.setBounds( 0, YRES(26), wide, YRES(14) );
	m_SubtitleLabel.setParent( this );

	// --- Compute column widths to fill the panel with padding ---
	int padding = XRES(8);
	int gridW = wide - padding * 2;
	int gridX = padding;

	// Sum up the proportional weights
	int totalWeight = 0;
	for( int i = 0; i < SUMMARY_NUM_COLUMNS; i++ )
		totalWeight += g_SumColumns[i].weight;

	// Calculate actual pixel widths from proportions
	int colWidths[SUMMARY_NUM_COLUMNS];
	int usedW = 0;
	for( int i = 0; i < SUMMARY_NUM_COLUMNS; i++ )
	{
		if( i == SUMMARY_NUM_COLUMNS - 1 )
			colWidths[i] = gridW - usedW;  // last column gets the remainder
		else
			colWidths[i] = ( gridW * g_SumColumns[i].weight ) / totalWeight;
		usedW += colWidths[i];
	}

	// --- Column headers ---
	int headerY = YRES(42);
	m_HeaderGrid.SetDimensions( SUMMARY_NUM_COLUMNS, 1 );
	m_HeaderGrid.SetSpacing( 0, 0 );

	for( int i = 0; i < SUMMARY_NUM_COLUMNS; i++ )
	{
		m_HeaderGrid.SetColumnWidth( i, colWidths[i] );
		m_HeaderGrid.SetEntry( i, 0, &m_HeaderLabels[i] );

		m_HeaderLabels[i].setText( g_SumColumns[i].title );
		m_HeaderLabels[i].setBgColor( 0, 0, 0, 255 );
		m_HeaderLabels[i].setFgColor( 255, 170, 0, 0 );  // orange header text
		m_HeaderLabels[i].setFont( smallFont );
		m_HeaderLabels[i].setContentAlignment( g_SumColumns[i].align );
		m_HeaderLabels[i].setContentFitted( false );
		m_HeaderLabels[i].setSize( colWidths[i], YRES(12) );
	}

	m_HeaderGrid.AutoSetRowHeights();
	m_HeaderGrid.setBounds( gridX, headerY, gridW, m_HeaderGrid.GetRowHeight(0) );
	m_HeaderGrid.setParent( this );
	m_HeaderGrid.setBgColor( 0, 0, 0, 255 );
	m_HeaderGrid.RepositionContents();

	// --- Pre-allocate row grids ---
	int rowY = headerY + YRES(14);
	int rowH = YRES(13);
	for( int row = 0; row < SUMMARY_MAX_PLAYERS; row++ )
	{
		m_RowGrids[row].SetDimensions( SUMMARY_NUM_COLUMNS, 1 );
		m_RowGrids[row].SetSpacing( 0, 0 );

		for( int col = 0; col < SUMMARY_NUM_COLUMNS; col++ )
		{
			m_RowGrids[row].SetColumnWidth( col, colWidths[col] );
			m_RowGrids[row].SetEntry( col, 0, &m_RowEntries[col][row] );

			m_RowEntries[col][row].setText( "" );
			m_RowEntries[col][row].setBgColor( 0, 0, 0, 255 );
			m_RowEntries[col][row].setFgColor( 200, 200, 200, 0 );
			m_RowEntries[col][row].setFont( textFont );
			m_RowEntries[col][row].setContentAlignment( g_SumColumns[col].align );
			m_RowEntries[col][row].setContentFitted( false );
			m_RowEntries[col][row].setSize( colWidths[col], rowH );
		}

		m_RowGrids[row].AutoSetRowHeights();
		m_RowGrids[row].setBounds( gridX, rowY + row * rowH, gridW, rowH );
		m_RowGrids[row].setParent( this );
		m_RowGrids[row].setBgColor( 0, 0, 0, 255 );
		m_RowGrids[row].RepositionContents();
		m_RowGrids[row].setVisible( false );
	}

	m_bHasData = false;
	m_iNumEntries = 0;
	m_iOutcome = 0;
	m_iMVPIndex = 0;

	Initialize();
}

//-----------------------------------------------------------------------------
void SummaryPanel::Initialize( void )
{
	m_bHasData = false;
	m_iNumEntries = 0;
	m_iOutcome = 0;
	m_iMVPIndex = 0;

	m_TitleLabel.setText( "" );
	m_SubtitleLabel.setText( "" );

	for( int row = 0; row < SUMMARY_MAX_PLAYERS; row++ )
	{
		m_RowGrids[row].setVisible( false );
		for( int col = 0; col < SUMMARY_NUM_COLUMNS; col++ )
			m_RowEntries[col][row].setText( "" );
	}
}

//-----------------------------------------------------------------------------
void SummaryPanel::SetHeader( int outcome, int mvpIndex )
{
	m_iOutcome = outcome;
	m_iMVPIndex = mvpIndex;
	m_iNumEntries = 0;
	m_bHasData = false;
}

//-----------------------------------------------------------------------------
void SummaryPanel::AddPlayerEntry( RoundPlayerEntry_t &entry )
{
	if( m_iNumEntries >= SUMMARY_MAX_PLAYERS )
		return;

	m_Entries[m_iNumEntries] = entry;
	m_iNumEntries++;
}

//-----------------------------------------------------------------------------
void SummaryPanel::Open( void )
{
	// Finalize whatever data we have
	if( !m_bHasData && m_iNumEntries > 0 )
	{
		qsort( m_Entries, m_iNumEntries, sizeof(RoundPlayerEntry_t), CompareEntries );
		m_bHasData = true;
	}
	if( m_bHasData )
		Rebuild();
	setVisible( true );
}

//-----------------------------------------------------------------------------
void SummaryPanel::Rebuild( void )
{
	// Refresh player info from engine
	gViewPort->GetAllPlayersInfo();

	// Set title based on outcome
	switch( m_iOutcome )
	{
	case ROUND_OUTCOME_HUMANS:
		m_TitleLabel.setFgColor( 100, 255, 100, 0 );  // green
		m_TitleLabel.setText( "MISSION COMPLETE" );
		break;
	case ROUND_OUTCOME_ZOMBIES:
		m_TitleLabel.setFgColor( 255, 80, 80, 0 );    // red
		m_TitleLabel.setText( "MISSION FAILED" );
		break;
	default:
		m_TitleLabel.setFgColor( 200, 200, 200, 0 );  // grey
		m_TitleLabel.setText( "ROUND OVER" );
		break;
	}

	// Set MVP subtitle
	if( m_iMVPIndex > 0 && m_iMVPIndex <= MAX_PLAYERS )
	{
		hud_player_info_t pi;
		GetPlayerInfo( m_iMVPIndex, &pi );
		if( pi.name && pi.name[0] )
		{
			char mvpText[256];
			sprintf( mvpText, "MVP: %s", pi.name );
			m_SubtitleLabel.setText( mvpText );
			m_SubtitleLabel.setFgColor( 255, 220, 50, 0 );  // gold
		}
		else
		{
			m_SubtitleLabel.setText( "" );
		}
	}
	else
	{
		m_SubtitleLabel.setText( "" );
	}

	int values[SUMMARY_NUM_COLUMNS][SUMMARY_MAX_PLAYERS];

	// Fill in rows
	for( int row = 0; row < SUMMARY_MAX_PLAYERS; row++ )
	{
		if( row >= m_iNumEntries )
		{
			m_RowGrids[row].setVisible( false );
			continue;
		}

		RoundPlayerEntry_t &e = m_Entries[row];
		bool isMVP = ( e.index == m_iMVPIndex );

		// Determine row text color
		int r, g, b;
		if( isMVP )
		{
			r = 255; g = 220; b = 50;  // gold for MVP
		}
		else
		{
			r = 200; g = 200; b = 200; // default light grey
		}

		// Get player name from engine
		char nameStr[64];
		hud_player_info_t pi;
		GetPlayerInfo( e.index, &pi );
		if( pi.name && pi.name[0] )
		{
			if( isMVP )
				sprintf( nameStr, "* %s", pi.name );
			else
				strncpy( nameStr, pi.name, sizeof(nameStr) - 1 );
			nameStr[sizeof(nameStr) - 1] = '\0';
		}
		else
		{
			sprintf( nameStr, "Player %d", e.index );
		}

		char buf[32];

		// Column 0: Name
		m_RowEntries[0][row].setText( nameStr );
		m_RowEntries[0][row].setFgColor( r, g, b, 0 );

		// Column 1: Score (frags from scoreboard data)
		values[1][row] = (int)g_PlayerExtraInfo[e.index].frags;
		sprintf( buf, "%d", values[1][row] );
		m_RowEntries[1][row].setText( buf );
		m_RowEntries[1][row].setFgColor( r, g, b, 0 );

		// Column 2: Kills (zombie NPCs + zombie players)
		values[2][row] = e.zombieKills;
		sprintf( buf, "%d", e.zombieKills );
		m_RowEntries[2][row].setText( buf );
		m_RowEntries[2][row].setFgColor( r, g, b, 0 );

		// Column 3: Headshots
		values[3][row] = e.headshots;
		sprintf( buf, "%d", e.headshots );
		m_RowEntries[3][row].setText( buf );
		m_RowEntries[3][row].setFgColor( r, g, b, 0 );

		// Column 4: Damage
		if( e.damage >= 1000 )
			sprintf( buf, "%d.%dk", e.damage / 1000, (e.damage % 1000) / 100 );
		else
			sprintf( buf, "%d", (int)e.damage );
		values[4][row] = (int)e.damage;
		m_RowEntries[4][row].setText( buf );
		m_RowEntries[4][row].setFgColor( r, g, b, 0 );

		// Column 5: XP earned (this round only)
		if( e.xpEarned >= 1000 )
			sprintf( buf, "%d.%dk", e.xpEarned / 1000, (e.xpEarned % 1000) / 100 );
		else
			sprintf( buf, "%d", (int)e.xpEarned );
		values[5][row] = (int)e.xpEarned;
		m_RowEntries[5][row].setText( buf );
		m_RowEntries[5][row].setFgColor( r, g, b, 0 );

		// Column 6: Escaped
		m_RowEntries[6][row].setText( e.escaped ? "Yes" : "-" );
		if( e.escaped )
			m_RowEntries[6][row].setFgColor( 100, 255, 100, 0 );
		else
			m_RowEntries[6][row].setFgColor( 150, 150, 150, 0 );

		m_RowGrids[row].setVisible( true );
	}

	// Highlight column leaders in bright white (columns 1-5: Score, Kills, Headshots, Damage, XP)
	for( int col = 1; col <= 5; col++ )
	{
		// Find the max value for this column
		int maxVal = 0;
		int maxCount = 0;
		for( int i = 0; i < m_iNumEntries; i++ )
		{
			int val = values[col][i];
			if( val > maxVal )
			{
				maxVal = val;
				maxCount = 1;
			}
			else if( val == maxVal )
				maxCount++;
		}

		// Only highlight if max > 0 and not everyone is tied
		if( maxVal > 0 && maxCount < m_iNumEntries )
		{
			for( int i = 0; i < m_iNumEntries; i++ )
			{
				if( values[col][i] == maxVal )
					m_RowEntries[col][i].setFgColor( 255, 255, 255, 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
void SummaryPanel::paintBackground( void )
{
	int wide, tall;
	getSize( wide, tall );

	// Semi-transparent dark background
	drawSetColor( 0, 0, 0, 96 );
	drawFilledRect( 0, 0, wide, tall );

	// Thin separator line under the header (matches grid padding)
	int pad = XRES(8);
	int headerBottom = YRES(42) + YRES(12) + 1;
	drawSetColor( 100, 100, 100, 128 );
	drawFilledRect( pad, headerBottom, wide - pad, headerBottom + 1 );
}
