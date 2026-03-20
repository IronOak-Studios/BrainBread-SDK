#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include <stdio.h>
#include "vgui_TeamFortressViewport.h"
extern TeamFortressViewport* gViewPort;

#include "pe_fader.h"
#include "pe_font.h"

#define SMALL_CNT_HEIGHT 35
DECLARE_MESSAGE(m_Counter, Counter) 
DECLARE_MESSAGE(m_Counter, CntDown) 
DECLARE_MESSAGE(m_Counter, SmallCnt) 

extern int g_sprHUD1;
extern int g_iTeamNumber;
extern float fTime; 

cPEFader *progressFader = NULL;
cPEFader *smallProgressFaderGood = NULL;
cPEFader *smallProgressFaderBad = NULL;

CHudCounter::~CHudCounter( )
{
	if( progressFader ) 
		delete progressFader;
	if( smallProgressFaderBad )
		delete smallProgressFaderBad;
	if( smallProgressFaderGood )
		delete smallProgressFaderGood;
}

int CHudCounter::Init(void)
{
	m_bDrawCntDown = FALSE;
	m_bInitCntDown = FALSE;

	HOOK_MESSAGE( Counter ); 
	HOOK_MESSAGE( CntDown ); 
	HOOK_MESSAGE( SmallCnt ); 

	progressFader = new cPEFader( );
	smallProgressFaderGood = new cPEFader( );
	smallProgressFaderBad = new cPEFader( );
	
	gHUD.AddHudElem(this); 

	return 1; 
};

int CHudCounter::VidInit(void)
{
	fTime = 0; 
	m_hSprite = 0;
	memset( m_sCounters, 0, sizeof(m_sCounters) );
	m_iNumCnts = 0;
	m_flRoundTime = 0;
	m_fCntDown = 0;
	m_fStep = 0;
	m_fCntTotal = 0;
	m_HUD_cross = 0;
	m_bDrawCntDown = FALSE;
	m_bInitCntDown = FALSE;
	m_iProgress = 0;
	m_fRStart = 0;
	m_fGStart = 0;
	m_fGStep = 0;

	smallProgressFaderGood->Start( "good", 0, 100 );
	smallProgressFaderBad->Start( "bad", 0, 100 );
	return 1;
};

int CHudCounter::MsgFunc_Counter( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_flRoundTime = READ_COORD();
	m_iFlags |= HUD_ACTIVE;

	return 1;
}

int CHudCounter::MsgFunc_SmallCnt( const char *pszName, int iSize, void *pbuf )
{
	int i = 0;
	char name[128];
	BEGIN_READ( pbuf, iSize );
	sprintf( name, READ_STRING( ) );
	for( i = 0; i < 10; i++ )
		if( !strcmp( name, m_sCounters[i].sName ) && m_sCounters[i].iActive )
		{
			m_sCounters[i].fTotal = READ_COORD( );
			m_sCounters[i].iActive = m_sCounters[i].fTotal > 0 ? true : false;
			m_sCounters[i].fStart = -1;
			return 1;
		}
	for( i = 0; i < 10; i++ )
		if( !m_sCounters[i].iActive )
			break;
	if( i >= 10 )
		return 0;

	sprintf( m_sCounters[i].sName, name );
	m_sCounters[i].fTotal = READ_COORD( );
	m_sCounters[i].iActive = m_sCounters[i].fTotal > 0 ? true : false;
	m_sCounters[i].fStart = -1;

	return 1;
}

int CHudCounter::MsgFunc_CntDown( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_fCntTotal = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	m_bInitCntDown = TRUE;

	return 1;
}

#define PGW (XRES(500)) //Progressbar Width
#define PGS ((ScreenWidth-PGW)/2) //Progressbar X-Pos

int CHudCounter::Draw( float flTime )
{
	if( m_bInitCntDown && m_fCntTotal > 0 )
	{
		m_fStep = (float)PGW / ( (float)m_fCntTotal * 10.0f );
		m_fCntDown = flTime + (float)m_fCntTotal;
		m_fGStep = 255.0f / ( ( (float)PGW / 3.0f ) * 2.0f );
		m_bInitCntDown = FALSE;
		m_bDrawCntDown = TRUE;
		progressFader->Start( "good", flTime, m_fCntDown );
	}
	else
		m_bInitCntDown = FALSE;
		
	if( m_iProgress >= PGW )
		m_bDrawCntDown = FALSE;
	m_iProgress = (int)( ( ( (float)m_fCntTotal ) - ( m_fCntDown - flTime ) ) * 10 * m_fStep );


	if( ( gHUD.m_iHideHUDDisplay & HIDEHUD_COUNTER ) )
		return 1;
/*	if( g_iUser1 )
		return 1;*/

	int r, g, b;
	int a = 255;
	int x, y;
	int HealthWidth;
	if( m_flRoundTime <= 5 )
	{
		r = 255; g = 0; b = 0;
	}
	else
	{
		r = 255; g = 255; b = 255;
	}
	g_font->SetFont( FONT_COUNTER );


	HealthWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0large).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0large).left;

	y = 5;

	rect_s rect;
	rect.bottom = 256;
	rect.top = 220;
	rect.right = 221;
	rect.left = 0;
	int rs = 255, gs = 255, bs = 255, as = 255;
	ScaleColors(rs, gs, bs, as );
	SPR_Set( g_sprHUD1, rs, gs, bs );
	SPR_DrawHoles( 0, (int)( (float)ScreenWidth / 2.0 ) - (int)( (float)( rect.right - rect.left ) / 2.0 ), 0, &rect );

	x = (int)( (float)ScreenWidth / 2.0 ) - ( 4 * HealthWidth ) - 4;

	if(m_flRoundTime <= 0)
	{
		x = gHUD.DrawHudNumberLarge(x, y, DHN_3DIGITS | DHN_DRAWZERO, 0, r, g, b);
		g_font->DrawString(x+4, y+5, ":", r, g, b);
		gHUD.DrawHudNumberLarge(x+12, y, DHN_2DIGITS | DHN_PREZERO, 0, r, g, b);
	}
	else
	{
		x = gHUD.DrawHudNumberLarge(x, y, DHN_3DIGITS | DHN_DRAWZERO, (int)m_flRoundTime/60, r, g, b);
		g_font->DrawString(x+4, y+5, ":", r, g, b);
		gHUD.DrawHudNumberLarge(x+12, y, DHN_2DIGITS | DHN_PREZERO, (int)m_flRoundTime%60, r, g, b);
	}

	x += HealthWidth/2;

	int iHeight = gHUD.m_iFontHeight;
	int iWidth = HealthWidth/10;
	float pcent = 0;

	m_iNumCnts = 0;
	for( int i = 0; i < 10; i++ )
	{
		if( !m_sCounters[i].iActive )
			continue;
		if( m_sCounters[i].fStart == -1 )
			m_sCounters[i].fStart = flTime;
		if( flTime > ( m_sCounters[i].fStart + m_sCounters[i].fTotal ) )
			m_sCounters[i].iActive = 0;

		g_font->DrawString( 10, 145 + m_iNumCnts * SMALL_CNT_HEIGHT, m_sCounters[i].sName, 143, 143, 54 );
		UTIL_FillRect( 10, 145 + m_iNumCnts * SMALL_CNT_HEIGHT + 18, XRES(80), 1, 143, 143, 54, 255 );
		UTIL_FillRect( 10, 145 + m_iNumCnts * SMALL_CNT_HEIGHT + 30, XRES(80), 1, 143, 143, 54, 255 );
		UTIL_FillRect( 10, 145 + m_iNumCnts * SMALL_CNT_HEIGHT + 18 + 1, 1, 11, 143, 143, 54, 255 );
		UTIL_FillRect( 10 + XRES(80), 145 + m_iNumCnts * SMALL_CNT_HEIGHT + 18, 1, 13, 143, 143, 54, 255 );
		
		pcent = 100.0f * ( flTime - m_sCounters[i].fStart ) / m_sCounters[i].fTotal;
		if( pcent > 100 )
			pcent = 100;
		if( g_iTeamNumber == 1 )
			smallProgressFaderBad->GetColor( pcent, r, g, b );
		else
			smallProgressFaderGood->GetColor( pcent, r, g, b );
		UTIL_FillRect( 12, 145 + m_iNumCnts * SMALL_CNT_HEIGHT + 20, (XRES(80)-4)/m_sCounters[i].fTotal*(flTime-m_sCounters[i].fStart), 9, r, g, b, 255 );

		m_iNumCnts++;		
	}

	if( m_bDrawCntDown )
	{
		if( m_iProgress == 0 )
			return 1;

		// Rand
		UTIL_FillRect( PGS-4, ScreenHeight * 0.5 - 12-4, PGW+4+4, 1, 255, 255, 255, 255 );
		UTIL_FillRect( PGS-4, ScreenHeight * 0.5 - 12+24+4, PGW+4+4, 1, 255, 255, 255, 255 );
		UTIL_FillRect( PGS-4, ScreenHeight * 0.5 - 12-4, 1, 24+4+4, 255, 255, 255, 255 );
		UTIL_FillRect( PGS+PGW+3, ScreenHeight * 0.5 - 12-4, 1, 24+4+4, 255, 255, 255, 255 );
		
		// Counter
		progressFader->GetColor( flTime, r, g, b );
		UTIL_FillRect( PGS, ScreenHeight * 0.5 - 12, m_iProgress, 24, r, g, b, 255 );

		return 1;
	}
	return 1;
}
