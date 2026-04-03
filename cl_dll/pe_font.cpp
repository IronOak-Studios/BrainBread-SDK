#include "hud.h"
#include "cl_util.h"
#include "hud_scale.h"

#include "pe_font.h"

//typedef int HSPRITE;
extern void ScaleColors( int &r, int &g, int &b, int a );
extern HSPRITE LoadSprite( const char *pszName );

cPEFontMgr *g_font = NULL;

s_font *fntBlueHighway = NULL;
//s_font *fntWepmenu = NULL;

// Scale a font-space value to screen pixels, accounting for the asset's
// native scale so that a 2x sprite at 2x HUD scale produces 1:1 pixels.
static inline int FontScale( const s_font *fnt, int v )
{
	return (int)( v * g_flHudScale / fnt->scale + 0.5f );
}

cPEFontMgr::cPEFontMgr( )
{
	ffont = NULL;
	lfont = NULL;
	curfont = NULL;
	if( !CVAR_GET_FLOAT( "cl_fontsys" ) )
		return;

	FONT_MENU = AddFont( FONT_MENU_NAME );
	FONT_BRIEFING = AddFont( FONT_BRIEFING_NAME );
	FONT_COUNTER = AddFont( FONT_COUNTER_NAME );
	FONT_INTRO = AddFont( FONT_INTRO_NAME );
	FONT_NOTIFY_MID = AddFont( FONT_NOTIFY_MID_NAME );
	FONT_NOTIFY = AddFont( FONT_NOTIFY_NAME );
	FONT_WEAPONMENU = AddFont( FONT_WEAPONMENU_NAME );
	Reload( );
}

void cPEFontMgr::LoadFont( char *name )
{
	if( !CVAR_GET_FLOAT( "cl_fontsys" ) )
		return;
	s_font *fnt = AddFont( name );
    LoadFont( fnt );
}

/*void cPEFontMgr::LoadFont( s_font *fnt )
{
	if( !fnt )
		return;

	char file[512];
	sprintf( file, "sprites/hud/%s.txt\0", fnt->basename );
	char *pstart = (char*)gEngfuncs.COM_LoadFile( file, 5, NULL);
	char *pfile = pstart;
	if( !pfile )
		return;
	char token[1024];
	int line = -1, width, tmp;
	bool nl = false;

	int i = 0;
	pfile = gEngfuncs.COM_ParseFile( pfile, token );
	fnt->height = atoi( token );

	pfile = gEngfuncs.COM_ParseFile( pfile, token );

	while( strlen( token ) > 0 )
	{
		if( i >= 0 && i <= ( PE_FONT_END - PE_FONT_START ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			tmp = atoi( token );
			if( tmp != line )
				nl = true;
			else
				nl = false;
			line = tmp;

			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			width = atoi( token );

			fnt->charXPos[i] = ( !nl ? fnt->charXPos[i-1] + fnt->charWidth[i-1] : 0 );
			fnt->charYPos[i] = line * fnt->height;
			fnt->charWidth[i] = width;
		}
		i++;
		pfile = gEngfuncs.COM_ParseFile( pfile, token );
	}
	sprintf( file, "sprites/hud/%s.spr\0", fnt->basename );
	fnt->charSprite = LoadSprite( file );
	fnt->ready = true;
	gEngfuncs.COM_FreeFile( pstart );
}*/
void cPEFontMgr::LoadFont( s_font *fnt )
{
	if( !CVAR_GET_FLOAT( "cl_fontsys" ) )
		return;
	if( !fnt )
		return;

	char file[512];
	char *pstart = NULL;

	fnt->scale = 1;
	if (CVAR_GET_FLOAT("cl_newfont"))
	{
		snprintf( file, sizeof(file), "sprites/hud/%s_solid.cfg", fnt->basename );
		pstart = (char*)gEngfuncs.COM_LoadFile( file, 5, NULL );
		if (pstart)
			fnt->scale = 2;
	}
	if (!pstart)
	{
		snprintf( file, sizeof(file), "sprites/hud/%s.cfg", fnt->basename );
		pstart = (char*)gEngfuncs.COM_LoadFile( file, 5, NULL );
	}

	char *pfile = pstart;
	if( !pfile )
		return;

	char token[1024];
	bool nl = false;

	int i = 0, endx = 0;
	pfile = gEngfuncs.COM_ParseFile( pfile, token );
	fnt->height = atoi( token );

	pfile = gEngfuncs.COM_ParseFile( pfile, token );

	while( strlen( token ) > 0 )
	{
		if( i >= 0 && i <= ( PE_FONT_END - PE_FONT_START ) )
		{
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			fnt->charYPos[i] = atoi( token );
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			fnt->charXPos[i] = atoi( token );
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			endx = atoi( token );
			fnt->charWidth[i] = endx - fnt->charXPos[i];
		}
		i++;
		pfile = gEngfuncs.COM_ParseFile( pfile, token );
	}

	if( fnt->scale == 2 )
		snprintf( file, sizeof(file), "sprites/hud/%s_solid.spr", fnt->basename );
	else
		snprintf( file, sizeof(file), "sprites/hud/%s.spr", fnt->basename );

	fnt->charSprite = LoadSprite( file );
	fnt->ready = true;
	gEngfuncs.COM_FreeFile( pstart );
}

s_font *cPEFontMgr::AddFont( char *name )
{
	if( !ffont )
	{
		lfont = ffont = new s_font;
		memset( ffont, 0, sizeof(s_font) );
		strncpy( ffont->basename, name, MAX_NAME_LEN - 1 );
		ffont->basename[MAX_NAME_LEN - 1] = '\0';
		return ffont;
	}
	s_font *it = ffont;
	while( it )
	{
		if( !strcmp( name, it->basename ) )
			break;
		it = it->next;
	}
	if( it )
		return it;
	it = new s_font;
	memset( it, 0, sizeof(s_font) );
	strncpy( it->basename, name, MAX_NAME_LEN - 1 );
	it->basename[MAX_NAME_LEN - 1] = '\0';
	lfont->next = it;
	lfont = it;
	return it;
}

void cPEFontMgr::Reload( )
{
	s_font *it = ffont;
	while( it )
	{
		LoadFont( it );
		it = it->next;
	}
}

int cPEFontMgr::GetWidth( char chr )
{
	if( !curfont || !curfont->ready )
		return 0;

	if( (int)chr < PE_FONT_START || (int)chr > PE_FONT_END )
		return HudScale( SPACE_WIDTH );
	return FontScale( curfont, curfont->charWidth[ chr - PE_FONT_START ] );
}

int cPEFontMgr::GetWidth( char *string )
{
	int need = 0;
	for ( ; *string != 0 && *string != '\n'; string++ )
		need += GetWidth( *string );
	return need;
}

int cPEFontMgr::GetWidthML( char *string )
{
	int maxneed = 0, need = 0;
	for ( ; *string != 0; string++ )
	{
		if( *string == '\n' )
		{
			if( need > maxneed )
				maxneed = need;
			need = 0;
			continue;
		}
		need += GetWidth( *string );
	}
	return max( maxneed, need );
}

void cPEFontMgr::DrawChar( int x, int y, char chr, int r, int g, int b )
{
	if( !curfont || !curfont->ready )
		return;

	if( (int)chr < PE_FONT_START || (int)chr > PE_FONT_END )
		return;
    
	chr -= PE_FONT_START;

	rect_s rect;
	rect.top = curfont->charYPos[chr];
	rect.bottom = rect.top + curfont->height;
	rect.left = curfont->charXPos[chr];
	rect.right = rect.left + curfont->charWidth[chr] + 1;
	
	if (curfont->scale > 1)
		ScaledSPR_DrawHoles( curfont->charSprite, 0, x, y, &rect, r, g, b, (float)curfont->scale );
	else
		ScaledSPR_DrawAdditive(curfont->charSprite, 0, x, y, &rect, r, g, b, (float)curfont->scale);
}

int cPEFontMgr::DrawString( int x, int y, char *string, int r, int g, int b, int a )
{
	if( a < 255 )
		ScaleColors( r, g, b, a );
	for ( ; *string != 0 && *string != '\n'; string++ )
	{
		DrawChar( x, y, *string, r, g, b );
		x = x + GetWidth( *string );	
	}
	return x;
}

int cPEFontMgr::DrawStringML( int x, int y, char *string, int r, int g, int b, int a )
{
	if( !curfont || !curfont->ready )
		return x;
	if( a < 255 )
		ScaleColors( r, g, b, a );
	int curx = x;
	for ( ; *string != 0 ; string++ )
	{
		if( *string == '\n' )
		{
			y += FontScale( curfont, curfont->height );
			curx = x;
			continue;
		}
		DrawChar( curx, y, *string, r, g, b );
		curx = curx + GetWidth( *string );	
	}
	return x;
}

void ChrToColor( char chr, int &r, int &g, int &b )
{
	if( chr == 'w')
	{
		r = 255;
		g = 255;
		b = 255;
	}
	// Yellow
	else if( chr == 'y')
	{
		r = 255;
		g = 255;
		b = 0;
	}
	// Red
	else if( chr == 'r')
	{
		r = 255;
		g = 0;
		b = 0;
	}
	// green
	else if( chr == 'g')
	{
		r = 0;
		g = 255;
		b = 0;
	}
	// cyan
	else if( chr == 'c')
	{
		r = 0;
		g = 255;
		b = 255;
	}
	// Grey
	else if( chr == 'q')
	{
		r = 128;
		g = 128;
		b = 128;
	}
	// transparent
	else if( chr == 't')
	{
		r = 128;
		g = 128;
		b = 128;
	}
}

int cPEFontMgr::DrawString( int x, int y, char *string )
{
	int r = 0, g = 0, b = 0;
	for ( ; *string != 0 && *string != '\n'; string++ )
	{
		if( *string == '\\' )
		{
			if( *(string + 1) == 0 )
				break;
			ChrToColor( *(++string), r, g, b );
			continue;
		}
		DrawChar( x, y, *string, r, g, b );
		x = x + GetWidth( *string );	
	}
	return x;
}

int cPEFontMgr::DrawStringML( int x, int y, char *string )
{
	if( !curfont || !curfont->ready )
		return x;
	int curx = x, r = 0, g = 0, b = 0;
	for ( ; *string != 0 ; string++ )
	{
		if( *string == '\\' )
		{
			if( *(string + 1) == 0 )
				break;
			ChrToColor( *(++string), r, g, b );
			continue;
		}

		if( *string == '\n' )
		{
			y += FontScale( curfont, curfont->height );
			curx = x;
			continue;
		}
		DrawChar( curx, y, *string, r, g, b );
		curx = curx + GetWidth( *string );	
	}
	return x;
}

int cPEFontMgr::GetHeight( )
{
	if( !curfont || !curfont->ready )
		return 0;
	return FontScale( curfont, curfont->height );
}

int cPEFontMgr::GetHeightML( char *string )
{
	if( !curfont || !curfont->ready )
		return 0;
	int count = 0;
	for ( ; *string != 0; string++ )
	{
		if( *string == '\n' )
			count++;
	}
	count++;
	return count * FontScale( curfont, curfont->height );
}
