#ifndef PE_FONT_H
#define PE_FONT_H

#define PE_FONT_START 33
#define PE_FONT_END 122
#define SPACE_WIDTH 5
#define MAX_NAME_LEN 128

////////////
#define FONT_MENU_NAME			"bluehighway"
#define FONT_BRIEFING_NAME		"bluehighway"
#define FONT_COUNTER_NAME		"bluehighway"
#define FONT_INTRO_NAME			"bluehighway"
#define FONT_NOTIFY_MID_NAME	"bluehighway"
#define FONT_NOTIFY_NAME		"bluehighway"
#define FONT_WEAPONMENU_NAME	"bluehighway"
#define FONT_MENU			fntBlueHighway
#define FONT_BRIEFING		fntBlueHighway
#define FONT_COUNTER		fntBlueHighway
#define FONT_INTRO			fntBlueHighway
#define FONT_NOTIFY_MID		fntBlueHighway
#define FONT_NOTIFY			fntBlueHighway
#define FONT_WEAPONMENU		fntBlueHighway

////////////

struct s_font
{
	int charWidth[PE_FONT_END - PE_FONT_START + 1];
	int charXPos[PE_FONT_END - PE_FONT_START + 1];
	int charYPos[PE_FONT_END - PE_FONT_START + 1];
	int height;

	char basename[MAX_NAME_LEN];

	int charSprite;
	int scale;		// asset scale factor (1 = original, 2 = hi-res variant)
	bool ready;

	s_font *next;
};
extern s_font *fntBlueHighway;

class cPEFontMgr
{
	s_font *ffont;
	s_font *lfont;
	s_font *curfont;

public:
	cPEFontMgr( );
	~cPEFontMgr( );
	void SetFont( char *name ) { curfont = AddFont( name ); }
	void SetFont( s_font *fnt ) { curfont = fnt; }
	s_font *AddFont( char *name );
	s_font *GetFont( char *name ) { return AddFont( name ); }
	void LoadFont( char *name );
	void LoadFont( s_font *fnt );
	void Reload( );
	int GetWidth( char chr );
	int GetWidth( char *font, char chr ) { curfont = AddFont( font ); return GetWidth( chr ); }
	int GetWidth( char *string );
	int GetWidth( char *font, char *string ) { return GetWidth( font, string ); }
	int GetWidthML( char *string );
	int GetWidthML( char *font, char *string ) { return GetWidthML( font, string ); }
	int GetHeight( );
	int GetHeight( char *font ) { curfont = AddFont( font ); return GetHeight( ); }
	int GetHeightML( char *string );
	int GetHeightML( char *font, char *string ) { curfont = AddFont( font ); return GetHeightML( string ); }
	int DrawString( int x, int y, char *string, int r, int g, int b, int a = 255 );
	int DrawString( char *font, int x, int y, char *string, int r, int g, int b, int a = 255 ) { curfont = AddFont( font ); return DrawString( x, y, string, r, g, b, a ); }
	int DrawStringML( int x, int y, char *string, int r, int g, int b, int a = 255 );
	int DrawStringML( char *font, int x, int y, char *string, int r, int g, int b, int a = 255 ) { curfont = AddFont( font ); return DrawStringML( x, y, string, r, g, b, a ); }
	int DrawString( int x, int y, char *string );
	int DrawString( char *font, int x, int y, char *string ) { curfont = AddFont( font ); return DrawString( x, y, string ); }
	int DrawStringML( int x, int y, char *string );
	int DrawStringML( char *font, int x, int y, char *string ) { curfont = AddFont( font ); return DrawStringML( x, y, string ); }
protected:
	void DrawChar( int x, int y, char chr, int r, int g, int b );
};

extern cPEFontMgr *g_font;

#endif //PE_FONT_H