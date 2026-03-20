#ifndef PE_HELPER_H
#define PE_HELPER_H

#include "pe_font.h"
#include "pe_fader.h"
#include "pe_help_topics.h"

#define MAX_HELPTEXT_LEN 1024
#define MAX_SPRITENAME_LEN 128

#define MAX_HELP_TYPES 128
#define MAX_HELP_TIMES CVAR_GET_FLOAT( "cl_help_times" )

struct s_helpitem
{
	int id;

	int startpos[2];
	int pos[2];
	char posfadername[MAX_FADERNAME_LEN];
	cPEFader posFader;

	int max[2];
	int border[2];

	float start;
	float end;
	float delay;

	bool blackbg;
	bool queued;

	int sprite;
	char spritename[MAX_SPRITENAME_LEN];
	char sprfadername[MAX_FADERNAME_LEN];
	cPEFader spriteFader;

	char text[MAX_HELPTEXT_LEN];
	char txtfadername[MAX_FADERNAME_LEN];
	cPEFader textFader;
	s_font *font;

	s_helpitem *prev;
	s_helpitem *next;
};

class cPEHelper
{
	int faderTypes[16];
	char helpInfo[MAX_HELP_TYPES];

	s_helpitem *fitem;
	s_helpitem *litem;

	s_helpitem *RemoveHelp( s_helpitem *rem );
public:
	cPEHelper( );
	~cPEHelper( );
	//s_helpitem *ForceShowHelp( int startdir, bool blackbg, bool queued, int x, int y, int borderx, int bordery, float time, float delay, char *sprite, char *text, s_font *font, char *spritefader = NULL, char *textfader = NULL );
	s_helpitem *ForceShowHelp( char *cfgfile );
	s_helpitem *ShowHelp( int type, char *cfgfile, bool more = false );
	float GetLastTime( );
	bool NeedHelp( int type );
	void DidHelp( int type );
	void RemoveAll( );
	void Draw( float time );
	void Reload( );
	int MsgFunc_Help( const char *pszName, int iSize, void *pbuf );
};

extern cPEHelper *g_helper;

#endif //PE_HELPER_H