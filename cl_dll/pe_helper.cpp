#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include "pe_fader.h"
#include "pe_font.h"
#include "pe_helper.h"

#define BORDER_DIST 15

#define START_AUTO 0
#define START_NONE 1
#define START_LEFT 2
#define START_RIGHT 3
#define START_TOP 4
#define START_BOTTOM 5

extern void InitGlobals( );

cPEHelper *g_helper = NULL;

int __MsgFunc_Help( const char *pszName, int iSize, void *pbuf )
{
	return g_helper->MsgFunc_Help( pszName, iSize, pbuf );
}

extern globalvars_t	Globals; 
float reusetime[30];


cPEHelper::cPEHelper( )
{
	memset( faderTypes, 0, sizeof(int) * 16 );
	memset( helpInfo, 0, MAX_HELP_TYPES );
	fitem = NULL;
	litem = NULL;
	HOOK_MESSAGE( Help );
	memset( reusetime, 0, 30 * sizeof(float) );
}

cPEHelper::~cPEHelper( )
{
	s_helpitem *temp;
	while( fitem )
	{
		temp = fitem;
		fitem = fitem->next;
		delete temp;
	}
}

void cPEHelper::RemoveAll( )
{
	s_helpitem *cur;
	while( fitem )
	{
		cur = fitem;
		fitem = fitem->next;
		delete cur;
	}
	litem = fitem = NULL;
}

void cPEHelper::Reload( )
{
	RemoveAll( );
	/*s_helpitem *cur = fitem;
	while( cur )
	{
		cur->sprite = ( strlen( cur->spritename ) ? LoadSprite( cur->spritename ) : -1 );
		cur = cur->next;
	}*/
}

float cPEHelper::GetLastTime( )
{
	s_helpitem *cur = fitem;
	float max = 0;
	while( cur )
	{
		if( cur->end > max )
			max = cur->end;
		cur = cur->next;
	}
	return max;
}

bool cPEHelper::NeedHelp( int type )
{
	if( type < 0 || type >= MAX_HELP_TYPES )
		return true;
	if( !CVAR_GET_FLOAT( "cl_help" ) )
		return false;
	if( helpInfo[type] < MAX_HELP_TIMES )
		return true;
	return false;
}

void cPEHelper::DidHelp( int type )
{
	if( type < 0 || type >= MAX_HELP_TYPES )
		return;
	helpInfo[type]++;
}

s_helpitem *cPEHelper::ShowHelp( int type, char *cfgfile, bool more )
{
	if( !NeedHelp( type ) )
		return NULL;
	if( !more )
		DidHelp( type );
	return ForceShowHelp( cfgfile );
}

s_helpitem *cPEHelper::ForceShowHelp( char *cfgfile )
{
  if( !CVAR_GET_FLOAT( "cl_helpsys" ) )
    return false;
	char token[1024] = "";
	char *pstart = (char*)gEngfuncs.COM_LoadFile( cfgfile, 5, NULL);
	char *pfile = pstart;
	if( !pfile )
		return NULL;
	
	bool xalign = false, xcenter = false, yalign = false, ycenter = false;
	char font[MAX_NAME_LEN] = "";
	int startdir = START_AUTO;

	s_helpitem *ni = new s_helpitem;
	ni->id = 0;
	ni->end = 0;
	ni->delay = 0;
	ni->start = -1;
	ni->prev = NULL;
	ni->next = NULL;
	ni->blackbg = true;
	ni->queued = false;
	ni->pos[0] = 0;
	ni->pos[1] = 0;
	ni->border[0] = BORDER_DIST;
	ni->border[1] = BORDER_DIST;
	ni->spritename[0] = '\0';
	memset( ni->text, '\0', MAX_HELPTEXT_LEN );
	strcpy( ni->posfadername, "position" );
	strcpy( ni->sprfadername, "stdspr" );
	strcpy( ni->txtfadername, "stdtext" );

	pfile = gEngfuncs.COM_ParseFile( pfile, token );
	while( strlen( token ) )
	{
		if( !strcmp( token, "blackbg" ) )
		{
			// black bg?
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			if( !strcmp( token, "false" ) )
				ni->blackbg = false;
		}
		else if( !strcmp( token, "borderx" ) )
		{
			// Border distance x
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			ni->border[0] = atoi( token );
		}
		else if( !strcmp( token, "font" ) )
		{
			// fontname
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			strncpy( font, token, MAX_NAME_LEN );
		}
		else if( !strcmp( token, "bordery" ) )
		{
			// Border distance x
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			ni->border[1] = atoi( token );
		}
		else if( !strcmp( token, "start" ) )
		{
			// start from where?
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			if( !strcmp( token, "none" ) )
				startdir = START_NONE;
			else if( !strcmp( token, "left" ) )
				startdir = START_LEFT;
			else if( !strcmp( token, "right" ) )
				startdir = START_RIGHT;
			else if( !strcmp( token, "top" ) )
				startdir = START_TOP;
			else if( !strcmp( token, "bottom" ) )
				startdir = START_BOTTOM;
		}
		else if( !strcmp( token, "queue" ) )
		{
			// queue item?
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			if( !strcmp( token, "true" ) )
				ni->queued = true;
		}
		else if( !strcmp( token, "xpos" ) )
		{
			// x pos
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			if( !strcmp( token, "center" ) )
				xcenter = true;
			if( !strcmp( token, "left" ) )
				ni->pos[0] = 0;
			if( !strcmp( token, "right" ) )
				xalign = true;
			else if( token[0] == 'p' )
				ni->pos[0] = (int)( atof( token + 1 ) / 100.0f * ScreenWidth );
			else
				ni->pos[0] = atoi( token );
		}
		else if( !strcmp( token, "ypos" ) )
		{
			// y pos
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			if( !strcmp( token, "center" ) )
				ycenter = true;
			if( !strcmp( token, "top" ) )
				ni->pos[1] = 0;
			if( !strcmp( token, "bottom" ) )
				yalign = true;
			else if( token[0] == 'p' )
				ni->pos[1] = (int)( atof( token + 1 ) / 100.0f * ScreenHeight );
			else
				ni->pos[1] = atoi( token );
		}
		else if( !strcmp( token, "len" ) )
		{
			// length
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			ni->end = atof( token );
		}
		else if( !strcmp( token, "delay" ) )
		{
			// delay
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			ni->delay = atof( token );
		}
		else if( !strcmp( token, "sprite" ) )
		{
			// sprite file
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			strncpy( ni->spritename, token, MAX_SPRITENAME_LEN );
		}
		else if( !strcmp( token, "posfader" ) )
		{
			// position fader
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			strncpy( ni->posfadername, token, MAX_FADERNAME_LEN );
		}
		else if( !strcmp( token, "sprfader" ) )
		{
			// sprite fader
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			strncpy( ni->sprfadername, token, MAX_FADERNAME_LEN );
		}
		else if( !strcmp( token, "txtfader" ) )
		{
			// text fader
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			strncpy( ni->txtfadername, token, MAX_FADERNAME_LEN );
		}
		else if( !strcmp( token, "content" ) )
		{
			// text
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			char *strt = pfile;
			while( strlen( token ) && token[0] != '}' )
				pfile = gEngfuncs.COM_ParseFile( pfile, token );
			if( token[0] == '}' )
			{
				// I just hope the cfg remains DOS formated... ^^
				strncpy( ni->text, strt, min( MAX_HELPTEXT_LEN, (int)pfile - (int)strt - 1 ) );
			}
		}
		else	
		{
			ConsolePrint( "Unknown help cfg file token: " );
			ConsolePrint( token );
			ConsolePrint( "in file " );
			ConsolePrint( cfgfile );
			ConsolePrint( "\n" );
		}
		pfile = gEngfuncs.COM_ParseFile( pfile, token );
	}
	gEngfuncs.COM_FreeFile( pstart );
	
    ni->sprite = ( strlen( ni->spritename ) > 1 ? LoadSprite( ni->spritename ) : -1 );
	if( strlen( font ) )
		ni->font = g_font->AddFont( font );
	else
		ni->font = FONT_BRIEFING;
	if( ni->font && !ni->font->ready )
		g_font->LoadFont( ni->font );

	if( ni->sprite >= 0 ) // Calc dimension
	{
		ni->max[0] = SPR_Width( ni->sprite, 0 ) + 20;
		ni->max[1] = SPR_Height( ni->sprite, 0 );
	}
	else
	{
		ni->max[0] = 0;
		ni->max[1] = 0;
	}
	if( strlen( ni->text ) )
	{
		ni->textFader.Start( ni->txtfadername, ni->start, ni->end );
		g_font->SetFont( ni->font );
		ni->max[0] += g_font->GetWidthML( ni->text );
		ni->max[1] = max( ni->max[1], g_font->GetHeightML( ni->text ) );
	}
	ni->max[0] += 2 * ni->border[0];
	ni->max[1] += 2 * ni->border[1];
	if( xcenter )
		ni->pos[0] = XRES( 320 ) - ni->max[0] / 2;
	else if( xalign )
		ni->pos[0] = XRES( 640 ) - ni->max[0] - ni->border[0];
	if( ycenter )
		ni->pos[1] = YRES( 240 ) - ni->max[1] / 2;
	else if( yalign )
		ni->pos[1] = YRES( 480 ) - ni->max[1] - ni->border[1];

	switch( startdir )
	{
	case START_NONE:
		ni->startpos[0] = ni->pos[0];
		ni->startpos[1] = ni->pos[1];
		break;
	case START_LEFT:
		ni->startpos[0] = -ni->max[0];
		ni->startpos[1] = ni->pos[1];
		break;
	case START_RIGHT:
		ni->startpos[0] = ScreenWidth;
		ni->startpos[1] = ni->pos[1];
		break;
	case START_TOP:
		ni->startpos[0] = ni->pos[0];
		ni->startpos[1] = -ni->max[1];
		break;
	case START_BOTTOM:
		ni->startpos[0] = ni->pos[0];
		ni->startpos[1] = ScreenHeight;
		break;
	default: // Auto will only start at top or bottom coz it's scrolled in faster
	{
		//int px = cur->pos[0] - ScreenWidth / 2;
		int py = ni->pos[1] - ScreenHeight / 2;
		//if( abs( py ) > abs( px ) )
		//{
			ni->startpos[0] = ni->pos[0];
			ni->startpos[1] = ( py > 0 ? ScreenHeight : -ni->max[1] );
		/*}
		else
		{
			cur->startpos[0] = ( px > 0 ? ScreenWidth : -cur->max[0] );
			cur->startpos[1] = cur->pos[1];
		}
		break;*/
	}
	}

	if( !litem )
		litem = fitem = ni;
	else
	{
		litem->next = ni;
		ni->prev = litem;
		litem = ni;
	}
	return ni;
}

s_helpitem *cPEHelper::RemoveHelp( s_helpitem *rem )
{
	if( !rem )
		return NULL;

	if( fitem == rem )
	{
		if( litem == rem )
			litem = NULL;
		fitem = rem->next;
		delete rem;
		return fitem;
	}
	if( !rem->prev )
		delete rem;
	else
	{
		s_helpitem *prev = rem->prev;
		prev->next = rem->next;
		if( rem->next )
			rem->next->prev = prev;
		else
			litem = prev;
		delete rem;
		return prev->next;
	}
	return NULL;
}

void cPEHelper::Draw( float time )
{
	s_helpitem *cur = fitem;
	int x = 0, y = -1000, ox = 0, oy = -1000, r = 0, g = 0, b = 0, maxx = 0, maxy = 0;
	int br, bg, bb, ba;

	while( cur )
	{
		if( cur->start == -1 ) // Init this helpitem
		{
			if( cur->queued ) // If queued, calc new delay
			{
				float lat = GetLastTime( ) - time;
				if( lat > cur->delay )
					cur->delay = lat;
			}
            cur->start = time + cur->delay;
			cur->end += cur->start;
			cur->spriteFader.Start( cur->sprfadername, cur->start, cur->end );
			cur->textFader.Start( cur->txtfadername, cur->start, cur->end );
			cur->posFader.Start( cur->posfadername, cur->start, cur->end ); 
		}
		else if( ( cur->end ) <= time )
		{
			cur = RemoveHelp( cur );
			continue;
		}
		else if( ( cur->start ) > time )
		{
			cur = cur->next;
			continue;
		}
		cur->posFader.GetColor( time, x, y, br, br );
		x = ox = cur->startpos[0] + (int)( (float)x / 100000.0f * ( cur->pos[0] - cur->startpos[0] ) );
		y = oy = cur->startpos[1] + (int)( (float)y / 100000.0f * ( cur->pos[1] - cur->startpos[1] ) );
		cur->spriteFader.GetColor( time, br, bg, bb, ba );
		if( cur->blackbg )
			UTIL_BlackRect(	ox,
							oy,
							cur->max[0],
							cur->max[1] );
		x += cur->border[0];
		y += cur->border[1];

		if( cur->sprite >= 0 )
		{
			rect_s rect;
			rect.top = 0;
			rect.bottom = SPR_Height( cur->sprite, 0 );
			rect.left = 0;
			rect.right = SPR_Width( cur->sprite, 0 );
			r = br; g = bg; b = bb;
			ScaleColors( r, g, b, ba );
			SPR_Set( cur->sprite, r, g, b );
			SPR_Draw( 0, x, y, &rect );
			x += rect.right + 20;
		}
		if( strlen( cur->text ) )
		{
			g_font->SetFont( cur->font );
			cur->textFader.GetColor( time, r, g, b );
			g_font->DrawStringML( x, y, cur->text, r, g, b );
		}

		UTIL_FillRect(	ox,
						oy,
						cur->max[0],
						1,
						br, bg, bb, ba );
		UTIL_FillRect(	ox + cur->max[0],
						oy,
						1,
						cur->max[1] + 1,
						br, bg, bb, ba );
		UTIL_FillRect(	ox,
						oy + cur->max[1],
						cur->max[0],
						1,
						br, bg, bb, ba );
		UTIL_FillRect(	ox,
						oy,
						1,
						cur->max[1],
						br, bg, bb, ba );

		cur = cur->next;
	}
}

extern globalvars_t *gpGlobals;
int cPEHelper::MsgFunc_Help( const char *pszName, int iSize, void *pbuf )
{
	int type = 0, force = 0;
	float param = 0;
	BEGIN_READ( pbuf, iSize );
	force = READ_BYTE( );
	type = READ_BYTE( );
	param = READ_COORD( );
	s_helpitem *hlp = NULL;

  if( !gpGlobals )
    InitGlobals( );

  if( force )
    force = -1;
  else
    force = type;
  if( reusetime[type] > gpGlobals->time )
	  return FALSE;

	switch( type )
	{
	/*case HELP_SEE_ENEMY:
		hlp = ShowHelp( force, "help/enemy.cfg" );
		break;*/
	case HELP_SKILL:
		hlp = ShowHelp( force, "help/skill.cfg" );
		break;
	case HELP_NOSPACE:
		hlp = ShowHelp( force, "help/nospace.cfg" );
		break;
/*	case HELP_SEE_TERMINAL:
		if( g_iTeamNumber == 1 )
			ShowHelp( force, "help/see_term_sec.cfg" );
		else
			ShowHelp( force, "help/see_term_syn.cfg" );
		break;
	case HELP_SEE_CASE:
		if( g_iTeamNumber == 1 )
			ShowHelp( force, "help/see_case_sec.cfg" );
		else
			ShowHelp( force, "help/see_case_syn.cfg" );
		break;
	case HELP_SEE_HTOOL:
		if( g_iTeamNumber == 1 )
			ShowHelp( force, "help/see_htool_sec.cfg" );
		else
			ShowHelp( force, "help/see_htool_syn.cfg" );
		break;*/
	case HELP_START_TRANSFORM:
		hlp = ShowHelp( force, "help/start_transform.cfg" );
		break;
	case HELP_MISSIONDONE:
		hlp = ShowHelp( force, "help/mission_done.cfg" );
		break;
	case HELP_STATSPOINT:
		hlp = ShowHelp( force, "help/point_stats.cfg" );
		break;
	case HELP_SKILLPOINT:
		hlp = ShowHelp( force, "help/point_skill.cfg" );
		break;
	case HELP_HPPOINT:
		hlp = ShowHelp( force, "help/point_hp.cfg" );
		break;
	case HELP_SPEEDPOINT:
		hlp = ShowHelp( force, "help/point_speed.cfg" );
		break;
	case HELP_AMMO:
		hlp = ShowHelp( force, "help/ammo.cfg" );
		break;
	}
	if( hlp )
		reusetime[type] = gpGlobals->time + hlp->end * 1.2;

	return 1;	
}
