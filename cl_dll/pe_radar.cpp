#include "hud.h"
#include "cl_util.h"
#include "hud_scale.h"
#include "parsemsg.h"
#include "pe_notify.h"
#include "triangleapi.h"
#include "pe_helper.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Radar, Radar)
DECLARE_MESSAGE(m_Radar, RadarOff);

extern int g_iTeamNumber;
extern int iTeamColors[5][3];
extern int g_sprHUD2;

int min_radar = 1024;
int max_radar = 0;

int RADAR_CL_RED = 0;
int RADAR_CL_GREEN = 1;
int RADAR_CL_YELLOW = 2;

extern vec3_t		v_origin, v_angles, v_cl_angles, v_sim_org, v_lastAngles;

int CHudRadar::CalcRadar(  )
{
	cl_entity_t	*ent = gEngfuncs.GetLocalPlayer( );
	if( !ent )
		return 1;
	float mycos, mysin;
	int alpha;
	vec3_t angles;
	s_cl_radarentry *cur = fradar;

	alpha = v_angles.y;
	alpha -= 90;
	if (alpha < 0) alpha = 360 + alpha;


	mysin = sin( ( (float)alpha / ( 180.0f / 3.14159265358979323846 ) ) );
	mycos = cos( ( (float)alpha / ( 180.0f / 3.14159265358979323846 ) ) );
	
	int xp, yp, xe, ye, x1, y1, x2, y2;
	xp = yp = xe = ye = x1 = y1 = x2 = y2 = 0;
	
	xp = v_origin.x;
	yp = v_origin.y;
	
	while( cur )
	{
		if( !cur->override && cur->pev )
			cur->origin = cur->pev->origin + Vector( 0, 0, 14 );
		xe = (int)cur->origin.x;
		ye = (int)cur->origin.y;

		x1 = xe - xp;
		y1 = ye - yp;

		x2 = x1 * mycos + y1 * mysin;
		y2 = (0-x1) * mysin + y1 * mycos;

		x1 = x2;	
		y1 = y2;

		x2 = x1 / 30;
		y2 = y1 / 30;

		y2 = y2 * -1;

		if (x2 < -50) x2 = -50;
		if (x2 > 50) x2 = 50;

		if (y2 < -50) y2 = -50;
		if (y2 > 50) y2 = 50;

		x2 = HudScale( x2 + 51 );
		y2 = HudScale( y2 + 51 );
		cur->origin2d[0] = x2;
		cur->origin2d[1] = y2;

		cur = cur->next;
	}	
	return 1;
}

bool showenemy = true;
bool showfriend = true;
bool showrest = true;
int CHudRadar::Init()
{
	m_fFade = 0;
	m_fBlink = 0;
	
	HOOK_MESSAGE(Radar);
	HOOK_MESSAGE(RadarOff);

	CVAR_CREATE( "cl_targets", "", FCVAR_ARCHIVE );


	m_iFlags |= HUD_ACTIVE | HUD_NOGLOW;
	fradar = lradar = NULL;

	gHUD.AddHudElem(this);
	ClearRadar( );

		
	return 1;
}

#define TARGET 3
#define ARROWS 4
#define BRAIN 5
/*#define SPR_TOP 6
#define SPR_BOTTOM 7
#define SPR_LEFT 4
#define SPR_RIGHT 5
#define SPR_TOPLEFT 8
#define SPR_TOPRIGHT 9
#define SPR_BOTTOMLEFT 10
#define SPR_BOTTOMRIGHT 11*/

#define SPR_TOP 5
#define SPR_BOTTOM 0
#define SPR_LEFT 3
#define SPR_RIGHT 4
#define SPR_TOPLEFT 6
#define SPR_TOPRIGHT 7
#define SPR_BOTTOMLEFT 1
#define SPR_BOTTOMRIGHT 2

#define RADAR_XPOS HudScale( 15 )
#define RADAR_YPOS HudScale( 30 )

int CHudRadar::VidInit()
{

	m_hSprite[0] = LoadSprite( "sprites/radar.spr" );
	m_hSprite[1] = LoadSprite( "sprites/radarp.spr" );
	m_hSprite[2] = LoadSprite( "sprites/radarp2.spr" );
	m_hSprite[3] = LoadSprite( "sprites/target.spr" );
	m_hSprite[4] = LoadSprite( "sprites/arrows.spr" );
	m_hSprite[5] = LoadSprite( "sprites/brain.spr" );
	/*m_hSprite[5] = LoadSprite( "sprites/targetr.spr" );
	m_hSprite[6] = LoadSprite( "sprites/targett.spr" );
	m_hSprite[7] = LoadSprite( "sprites/targetb.spr" );
	m_hSprite[8] = LoadSprite( "sprites/targettl.spr" );
	m_hSprite[9] = LoadSprite( "sprites/targettr.spr" );
	m_hSprite[10] = LoadSprite( "sprites/targetbl.spr" );
	m_hSprite[11] = LoadSprite( "sprites/targetbr.spr" );*/
	ClearRadar( );

	return 1;
}

#define RD_TOP 0
#define RD_BOTTOM (ScreenHeight - HudScale( 48 ))
#define RD_LEFT 0
#define RD_RIGHT (ScreenWidth - HudScale( 48 ))
#define RD_EDGE HudScale( 20 )

int GetSprite( int x, int y )
{
	int spr = 3;
	if( x == RD_LEFT )
	{
		if( y <= RD_TOP + RD_EDGE )
			spr = SPR_TOPLEFT;
		else if( y >= RD_BOTTOM - RD_EDGE )
			spr = SPR_BOTTOMLEFT;
		else
			spr = SPR_LEFT;
	}
	else if( x == RD_RIGHT )
	{
		if( y <= RD_TOP + RD_EDGE )
			spr = SPR_TOPRIGHT;
		else if( y >= RD_BOTTOM - RD_EDGE )
			spr = SPR_BOTTOMRIGHT;
		else
			spr = SPR_RIGHT;
	}
	else if( y == RD_TOP )
	{
		if( x <= RD_LEFT + RD_EDGE )
			spr = SPR_TOPLEFT;
		else if( x >= RD_RIGHT - RD_EDGE )
			spr = SPR_TOPRIGHT;
		else
			spr = SPR_TOP;
	}
	else if( y == RD_BOTTOM )
	{
		if( x <= RD_LEFT + RD_EDGE )
			spr = SPR_BOTTOMLEFT;
		else if( x >= RD_RIGHT - RD_EDGE )
			spr = SPR_BOTTOMRIGHT;
		else
			spr = SPR_BOTTOM;
	}
	else
		spr = -TARGET;
	return spr;
}

int iSentinel = 0;
int CHudRadar::Draw( float flTime )
{
	int r, g, b, x, y, a;

	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;
/*  if( gHUD.m_ShowMenuPE.m_iActive || gHUD.m_PETeam.m_iActive )
		return 1;*/
	if( gHUD.m_Menu.m_fMenuDisplayed )
		return 1;
	
	if (!m_fOn) return 1;

  vec3_t bh, ez = Vector( 0, 0, 0 );
  bool bhdraw = false;

	CalcRadar( );
	
	if( m_fBlink <= flTime )
	{
		m_fBlink = flTime + 0.3;
		m_iBlink = !m_iBlink;
	}

	
	if( g_iTeamNumber == 1 )
	{
		r = iTeamColors[1][0];
		g = iTeamColors[1][1];
		b = iTeamColors[1][2];
	}
	else
	{
		r = iTeamColors[2][0];
		g = iTeamColors[2][1];
		b = iTeamColors[2][2];
	}

	a = 250;

	ScaleColors(r, g, b, a);

	rect_s rect;
	rect.bottom = 139;
	rect.top = 0;
	rect.right = 256;
	rect.left = 131;
	int rs = 255, gs = 255, bs = 255, as = 255;
	ScaleColors(rs, gs, bs, as );
	ScaledSPR_DrawHoles( g_sprHUD2, 0, 0, 0, &rect, rs, gs, bs );

	ScaledSPR_DrawAdditive( m_hSprite[0], 0, RADAR_XPOS, RADAR_YPOS, NULL, r, g, b );

    int x2,y2,isvis,spr,col;
	vec3_t screen, origin;
	s_cl_radarentry *cur = fradar;

	char *targets = CVAR_GET_STRING( "cl_targets" );
	if( strstr( targets, "noenemy" ) )
		showenemy = false;
	else
		showenemy = true;
	if( strstr( targets, "nofriend" ) )
		showfriend = false;
	else
		showfriend = true;
	if( strstr( targets, "noobject" ) )
		showrest = false;
	else
		showrest = true;

	rect.bottom = 8;
	rect.top = 0;
	rect.right = 8;
	rect.left = 0;

	while( cur )
	{
		x2 = RADAR_XPOS - HudScale( 3 );
		y2 = RADAR_YPOS - HudScale( 3 );

		if( cur->blinktime == 1 )
			cur->blinktime = flTime + 2;
		else if( cur->blinktime && cur->blinktime < flTime )
		{
			cur->color = cur->lastcolor;
			cur->blinktime = 0;
		}
		col = cur->color;
		r = g = b = 255;
   	a = 250;
		
		switch( col )
		{
		case RADAR_ENEMY:
			r = 255; g = b = 0;
			break;
		case RADAR_FRIEND:
			r = b = 0; g = 255;
			break;			
		case RADAR_NODRAW:
			r = g = 0; b = 0;
			break;			
		case RADAR_MARK: // pe_radar_mark
			r = g = b = 255;
			break;
		case RADAR_TERMINAL:
			r = g = 0; b = 255;
			break;
		case RADAR_TERMINAL_HACKED:
			r = 255; g = 150; b = 50;
			break;
		case RADAR_OBJECT:
			r = 0; g = 255; b = 255;
			break;
		case RADAR_BLINK:
			r = g = ( m_iBlink ? 255 : 0 ); b = 0;
			break;
		case RADAR_BLINK_TEMP:
			g = ( m_iBlink ? 255 : 0 ); r = b = 0;
			break;
		}

		x2 += cur->origin2d[0];
		y2 += cur->origin2d[1];

		ScaleColors(r, g, b, a);
		ScaledSPR_DrawAdditive( m_hSprite[col == RADAR_BLINK_TEMP ? 2 : 1], 0, x2, y2, &rect, r, g, b );

		if( col != RADAR_BLINK && col != RADAR_TERMINAL && !( g_iTeamNumber == 2 && col == RADAR_ENEMY ) )
		{
			cur = cur->next;
			continue;
		}

		origin = cur->origin;

    if( col == RADAR_TERMINAL )
    {
      bh = origin;
      bhdraw = true;
    }
    else if( bhdraw )
    {
      if( ez.Length( ) == 0 || ( origin - bh ).Length( ) < ( ez - bh ).Length( ) )
        ez = origin;
    }

		isvis = !gEngfuncs.pTriAPI->WorldToScreen( origin, screen );			
		screen[0] = XPROJECT(screen[0]);
		screen[1] = YPROJECT(screen[1]);
		/*char text[256];
		sprintf( text, "say x: %f y: %f\0", screen[0], screen[1], screen[2] );
		ClientCmd( text );*/
    a = 50 + ( 1 - ( origin - v_origin ).Length( ) / 950 ) * 100;
    a = max( a, 50 );
		if( isvis )
		{
			x = ( screen[0] < 0 ? RD_LEFT : ( screen[0] > RD_RIGHT ? RD_RIGHT : (int)screen[0] ) );
			y = ( screen[1] < 0 ? RD_TOP : ( screen[1] > RD_BOTTOM ? RD_BOTTOM : (int)screen[1] ) );
		}
		else
		{
			x = ( screen[0] < 0 ? RD_RIGHT : RD_LEFT  );
			y = ( ScreenHeight - screen[1] < 0 ? RD_TOP : ( ScreenHeight - screen[1] > RD_BOTTOM ?  RD_BOTTOM : ScreenHeight - (int)screen[1] ) );
		}
		spr = GetSprite( x, y );
		if( spr == -TARGET )
		{
      if( g_iTeamNumber == 2 && col == RADAR_ENEMY )
        spr = BRAIN;
      else
        spr = TARGET;
			ScaleColors( r, g, b, a );
			x -= HudScale( SPR_Width( m_hSprite[spr], 0 ) ) / 2;
			y -= HudScale( SPR_Height( m_hSprite[spr], 0 ) ) / 2;
			ScaledSPR_DrawAdditive( m_hSprite[spr], 0, x, y, NULL, r, g, b );
		}
    else
    {
      rect_s arrowrect;
	    arrowrect.bottom = 32;
	    arrowrect.top = 0;
	    arrowrect.right = 32;
	    arrowrect.left = 0;
			ScaleColors( r, g, b, a + 50 );
			ScaledSPR_DrawAdditive( m_hSprite[ARROWS], spr, x, y, &arrowrect, r, g, b );
    }
		cur = cur->next;
	}
  if( bhdraw )
  {
    char text[128];
    sprintf( text, "BlackHawks distance to escapezone is %d meters", (int)( ( bh - ez ).Length( ) / 10 ) );
    g_font->SetFont( FONT_NOTIFY );
		g_font->DrawString( HudScale( 10 ), HudScale( 160 ), text, 0, 255.0f, 0, 180 );
  }
	return 1; 
}

void CHudRadar::ClearRadar( )
{
	s_cl_radarentry *cur;
	while( fradar )
	{
		cur = fradar;
		fradar = fradar->next;
		delete cur;
	}
	lradar = fradar = NULL;

	m_fFade = 0;
	m_fBlink = 0;
	m_fOn = 1;
	px=py=0;
	return;
}

void CHudRadar::Disable( int idx )
{	
	s_cl_radarentry *cur = fradar;
	while( cur )
	{
		if( cur->entindex == idx )
		{
			if( fradar == cur )
			{
				if( lradar == cur )
					lradar = NULL;
				fradar = cur->next;
				delete cur;
			}
			else if( !cur->prev )
			{
				delete cur;
			}
			else
			{
				s_cl_radarentry *prev = cur->prev;
				prev->next = cur->next;
				if( cur->next )
					cur->next->prev = prev;
				else
					lradar = prev;
				delete cur;
			}
			return;
		}
		cur = cur->next;
	}
}

s_cl_radarentry *CHudRadar::Enable( int idx )
{	
	s_cl_radarentry *nr = fradar;
	while( nr )
	{
		if( nr->entindex == idx )
			break;
		nr = nr->next;
	}
	if( nr )
		return nr;

	nr = new s_cl_radarentry;
	memset( nr, 0, sizeof(s_cl_radarentry) );
	nr->entindex = idx;
	if( !fradar )
	{
		fradar = lradar = nr;
	}
	else
	{
		lradar->next = nr;
		nr->prev = lradar;
		lradar = nr;
	}
	return nr;
}

int CHudRadar::MsgFunc_Radar(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int idx = READ_SHORT( );
	int mode = READ_BYTE( );
	s_cl_radarentry *cur;
	switch( mode )
	{
	case 0: //remove
		Disable( idx );
		break;
	case 1: //update origin
		cur = Enable( idx );
		cur->origin.x = READ_COORD( );
		cur->origin.y = READ_COORD( );
		cur->origin.z = READ_COORD( );
		if( idx > 0 && idx <= gEngfuncs.GetMaxClients( ) )
		{
			cur->pev = gEngfuncs.GetEntityByIndex( idx );
			cur->origin = cur->origin + Vector( 0, 0, 14 );
		}
		cur->override = ( READ_BYTE( ) ? true : false );
		break;
	case 2: //update color
		cur = Enable( idx );
		if( cur->blinktime )
			cur->lastcolor = READ_BYTE( );
		else
		{
			cur->lastcolor = cur->color;
			cur->color = READ_BYTE( );
		}
		if( cur->color == RADAR_BLINK_TEMP )
			cur->blinktime = 1;
		break;
	case 3: // reset
		ClearRadar( );
		break;
	}

	return 1;
}


int CHudRadar::MsgFunc_RadarOff(const char *pszName,  int iSize, void *pbuf )
{
	//ClearRadar( );
	BEGIN_READ( pbuf, iSize );
	m_fOn = READ_BYTE();
	m_fOn = TRUE; // :)
	return 1;
}

