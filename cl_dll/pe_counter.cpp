#include "hud.h"
#include "cl_util.h"
#include "hud_scale.h"
#include "parsemsg.h"
#include <string.h>
#include <stdio.h>
#include "vgui_TeamFortressViewport.h"
extern TeamFortressViewport* gViewPort;

#include "pe_fader.h"
#include "pe_font.h"

#define SMALL_BAR_HEIGHT HudScale( 13 )
#define SMALL_BAR_GAP    HudScale( 4 )
#define SMALL_CNT_HEIGHT ( g_font->GetHeight() + SMALL_BAR_HEIGHT + SMALL_BAR_GAP )
DECLARE_MESSAGE(m_Counter, Counter) 
DECLARE_MESSAGE(m_Counter, CntDown) 
DECLARE_MESSAGE(m_Counter, SmallCnt) 
DECLARE_MESSAGE(m_Counter, SmallCntAdd)

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
	HOOK_MESSAGE( SmallCntAdd );

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
	memset( m_sCounterPopups, 0, sizeof(m_sCounterPopups) );
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
	char *msgName = READ_STRING( );
	int nameLen = strlen( msgName );
	snprintf( name, sizeof(name), "%s", msgName );
	float total = READ_COORD( );
	int remaining = iSize - nameLen - 1 - 2;
	float initialProgress = remaining >= 2 ? READ_COORD( ) : 0;
	if( initialProgress < 0 )
		initialProgress = 0;
	if( initialProgress > ( total < 0 ? -total : total ) )
		initialProgress = total < 0 ? -total : total;
	for( i = 0; i < 10; i++ )
		if( !strcmp( name, m_sCounters[i].sName ) && m_sCounters[i].iActive )
		{
			m_sCounters[i].bManual = total < 0;
			m_sCounters[i].fTotal = total < 0 ? -total : total;
			m_sCounters[i].iActive = m_sCounters[i].fTotal > 0 ? true : false;
			m_sCounters[i].fStart = gEngfuncs.GetClientTime();
			m_sCounters[i].fProgressOffset = initialProgress;
			m_sCounters[i].fBonusPopupCarry = 0;
			for( int p = 0; p < 12; p++ )
				if( m_sCounterPopups[p].iCounter == i )
					m_sCounterPopups[p].iActive = false;
			return 1;
		}
	for( i = 0; i < 10; i++ )
		if( !m_sCounters[i].iActive )
			break;
	if( i >= 10 )
		return 0;

	snprintf( m_sCounters[i].sName, sizeof(m_sCounters[i].sName), "%s", name );
	m_sCounters[i].bManual = total < 0;
	m_sCounters[i].fTotal = total < 0 ? -total : total;
	m_sCounters[i].iActive = m_sCounters[i].fTotal > 0 ? true : false;
	m_sCounters[i].fStart = gEngfuncs.GetClientTime();
	m_sCounters[i].fProgressOffset = initialProgress;
	m_sCounters[i].fBonusPopupCarry = 0;

	return 1;
}

int CHudCounter::MsgFunc_SmallCntAdd( const char *pszName, int iSize, void *pbuf )
{
	char name[128];
	BEGIN_READ( pbuf, iSize );
	snprintf( name, sizeof(name), "%s", READ_STRING( ) );
	float absoluteProgress = READ_COORD( );
	float popupAmount = READ_COORD( );
	float curTime = gEngfuncs.GetClientTime();

	for( int i = 0; i < 10; i++ )
	{
		if( strcmp( name, m_sCounters[i].sName ) || !m_sCounters[i].iActive )
			continue;

		float progress = absoluteProgress;
		if( progress < 0 )
			progress = 0;
		if( progress > m_sCounters[i].fTotal )
			progress = m_sCounters[i].fTotal;
		m_sCounters[i].fProgressOffset = progress - ( m_sCounters[i].bManual ? 0 : curTime - m_sCounters[i].fStart );

		if( popupAmount > 0 )
		{
			m_sCounters[i].fBonusPopupCarry += popupAmount;
			int popupAmountInt = (int)m_sCounters[i].fBonusPopupCarry;
			if( popupAmountInt > 0 )
			{
				m_sCounters[i].fBonusPopupCarry -= popupAmountInt;

				int popupSlot = -1;
				for( int p = 0; p < 12; p++ )
				{
					if( !m_sCounterPopups[p].iActive )
					{
						popupSlot = p;
						break;
					}
					if( popupSlot < 0 || m_sCounterPopups[p].fStart < m_sCounterPopups[popupSlot].fStart )
						popupSlot = p;
				}

				if( popupSlot >= 0 )
				{
					int seed = (int)( curTime * 100.0f ) + popupSlot * 17 + i * 31 + popupAmountInt * 7;
					m_sCounterPopups[popupSlot].iActive = true;
					m_sCounterPopups[popupSlot].iCounter = i;
					m_sCounterPopups[popupSlot].fStart = curTime;
					m_sCounterPopups[popupSlot].fLife = 0.95f + ( seed % 7 ) * 0.05f;
					m_sCounterPopups[popupSlot].iStartX = HudScale( ( seed % 9 ) - 4 );
					m_sCounterPopups[popupSlot].iStartY = HudScale( ( ( seed / 3 ) % 7 ) - 3 );
					m_sCounterPopups[popupSlot].iDriftX = HudScale( 12 + ( seed % 15 ) );
					m_sCounterPopups[popupSlot].iDriftY = HudScale( 22 + ( ( seed / 5 ) % 17 ) );
					snprintf( m_sCounterPopups[popupSlot].sText, sizeof(m_sCounterPopups[popupSlot].sText), "+%d", popupAmountInt );
				}
			}
		}
		return 1;
	}

	return 1;
}

int CHudCounter::MsgFunc_CntDown( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_fCntTotal = READ_BYTE();
	m_fCntDown = gEngfuncs.GetClientTime() + (float)m_fCntTotal;
	m_iFlags |= HUD_ACTIVE;
	m_bInitCntDown = TRUE;
	m_bDrawCntDown = m_fCntTotal > 0 ? TRUE : FALSE;
	if( m_bDrawCntDown )
		progressFader->Start( "good", gEngfuncs.GetClientTime(), m_fCntDown );

	return 1;
}

#define PGW (XRES(500)) //Progressbar Width
#define PGS ((ScreenWidth-PGW)/2) //Progressbar X-Pos

int CHudCounter::Draw( float flTime )
{
	float curTime = gEngfuncs.GetClientTime();

	if( m_bInitCntDown && m_fCntTotal > 0 )
	{
		m_fStep = (float)PGW / ( (float)m_fCntTotal * 10.0f );
		m_fGStep = 255.0f / ( ( (float)PGW / 3.0f ) * 2.0f );
		m_bInitCntDown = FALSE;
	}
	else
		m_bInitCntDown = FALSE;
		
	if( m_iProgress >= PGW )
		m_bDrawCntDown = FALSE;
	m_iProgress = (int)( ( ( (float)m_fCntTotal ) - ( m_fCntDown - curTime ) ) * 10 * m_fStep );


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


	HealthWidth = HudScale( gHUD.GetSpriteRect(gHUD.m_HUD_number_0large).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0large).left );

	y = HudScale( 5 );

	rect_s rect;
	rect.bottom = 248;
	rect.top = 220;
	rect.right = 182;
	rect.left = 28;
	int rs = 255, gs = 255, bs = 255, as = 255;
	ScaleColors(rs, gs, bs, as );
	int bgWidth = HudScale( rect.right - rect.left );
	ScaledSPR_DrawHoles( g_sprHUD1, 0, ScreenWidth / 2 - bgWidth / 2, 0, &rect, rs, gs, bs );

	int visibleWidth = 2 * HealthWidth + HudScale( 12 );
	x = ScreenWidth / 2 - visibleWidth / 2 - 2 * HealthWidth;

	if(m_flRoundTime <= 0)
	{
		x = gHUD.DrawHudNumberLarge(x, y, DHN_3DIGITS | DHN_DRAWZERO, 0, r, g, b);
		g_font->DrawString(x + HudScale( 4 ), y + HudScale( 5 ), ":", r, g, b);
		gHUD.DrawHudNumberLarge(x + HudScale( 12 ), y, DHN_2DIGITS | DHN_PREZERO, 0, r, g, b);
	}
	else
	{
		x = gHUD.DrawHudNumberLarge(x, y, DHN_3DIGITS | DHN_DRAWZERO, (int)m_flRoundTime/60, r, g, b);
		g_font->DrawString(x + HudScale( 4 ), y + HudScale( 5 ), ":", r, g, b);
		gHUD.DrawHudNumberLarge(x + HudScale( 12 ), y, DHN_2DIGITS | DHN_PREZERO, (int)m_flRoundTime%60, r, g, b);
	}

	x += HealthWidth/2;

	int iHeight = HudScale( gHUD.m_iFontHeight );
	int iWidth = HealthWidth/10;
	float pcent = 0;

	m_iNumCnts = 0;
	for( int i = 0; i < 10; i++ )
	{
		if( !m_sCounters[i].iActive )
			continue;
		if( m_sCounters[i].fStart < 0 )
			m_sCounters[i].fStart = curTime;
		float progress = ( m_sCounters[i].bManual ? 0 : curTime - m_sCounters[i].fStart ) + m_sCounters[i].fProgressOffset;
		if( !m_sCounters[i].bManual && progress > m_sCounters[i].fTotal )
			m_sCounters[i].iActive = 0;
		if( progress < 0 )
			progress = 0;
		if( progress > m_sCounters[i].fTotal )
			progress = m_sCounters[i].fTotal;

		int cy = HudScale( 145 ) + m_iNumCnts * SMALL_CNT_HEIGHT;
		int cx = HudScale( 10 );
		int cw = XRES(80);
		int fontH = g_font->GetHeight();
		int barTop = cy + fontH;
		int r = 143, g = 143, b = 54;
		if (CVAR_GET_FLOAT("cl_newfont"))
		{
			r = g = 255;
			// progress bar background
			gEngfuncs.pfnFillRGBABlend(cx, barTop, cw, SMALL_BAR_HEIGHT, 0, 0, 0, 180);
		}
		g_font->DrawString( cx, cy, m_sCounters[i].sName, r, g, b );

		UTIL_FillRect( cx, barTop, cw, HudScale( 1 ), r, g, b, 255 );
		UTIL_FillRect( cx, barTop + SMALL_BAR_HEIGHT - HudScale( 1 ), cw, HudScale( 1 ), r, g, b, 255 );
		UTIL_FillRect( cx, barTop + HudScale( 1 ), HudScale( 1 ), SMALL_BAR_HEIGHT - HudScale( 2 ), r, g, b, 255 );
		UTIL_FillRect( cx + cw, barTop, HudScale( 1 ), SMALL_BAR_HEIGHT, r, g, b, 255 );
		
		pcent = 100.0f * progress / m_sCounters[i].fTotal;
		if( pcent > 100 )
			pcent = 100;
		if( !m_sCounters[i].bManual && g_iTeamNumber == 1 && strstr( m_sCounters[i].sName, "ombie" ) )
			smallProgressFaderBad->GetColor( pcent, r, g, b );
		else
			smallProgressFaderGood->GetColor( pcent, r, g, b );
		UTIL_FillRect( cx + HudScale( 2 ), barTop + HudScale( 2 ), (cw - HudScale( 4 ))/m_sCounters[i].fTotal*progress, SMALL_BAR_HEIGHT - HudScale( 4 ), r, g, b, 255 );

		for( int p = 0; p < 12; p++ )
		{
			if( !m_sCounterPopups[p].iActive || m_sCounterPopups[p].iCounter != i )
				continue;

			float life = curTime - m_sCounterPopups[p].fStart;
			if( life >= m_sCounterPopups[p].fLife )
			{
				m_sCounterPopups[p].iActive = false;
				continue;
			}

			float frac = life / m_sCounterPopups[p].fLife;
			int alpha = (int)( ( 1.0f - frac ) * 255.0f );
			int px = cx + cw + HudScale( 10 ) + m_sCounterPopups[p].iStartX + (int)( m_sCounterPopups[p].iDriftX * frac );
			int py = barTop - HudScale( 5 ) + m_sCounterPopups[p].iStartY - (int)( m_sCounterPopups[p].iDriftY * frac );
			g_font->DrawString( px + HudScale( 1 ), py + HudScale( 1 ), m_sCounterPopups[p].sText, 0, 0, 0, alpha / 2 );
			g_font->DrawString( px, py, m_sCounterPopups[p].sText, 255, 220, 64, alpha );
		}

		m_iNumCnts++;		
	}

	if( m_bDrawCntDown )
	{
		if( m_iProgress == 0 )
			return 1;

		// Rand
		int ph = HudScale( 24 );
		int pp = HudScale( 4 );
		int py = (int)(ScreenHeight * 0.5f) - ph / 2;
		UTIL_FillRect( PGS - pp, py - pp, PGW + pp * 2, HudScale( 1 ), 255, 255, 255, 255 );
		UTIL_FillRect( PGS - pp, py + ph + pp, PGW + pp * 2, HudScale( 1 ), 255, 255, 255, 255 );
		UTIL_FillRect( PGS - pp, py - pp, HudScale( 1 ), ph + pp * 2, 255, 255, 255, 255 );
		UTIL_FillRect( PGS + PGW + pp - HudScale( 1 ), py - pp, HudScale( 1 ), ph + pp * 2, 255, 255, 255, 255 );
		
		// Counter
		progressFader->GetColor( curTime, r, g, b );
		UTIL_FillRect( PGS, py, m_iProgress, ph, r, g, b, 255 );

		return 1;
	}
	return 1;
}
