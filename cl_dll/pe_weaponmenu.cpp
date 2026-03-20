#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include <stdio.h>
#include <process.h>
#include "vgui_TeamFortressViewport.h"
#include "..\dlls\pe_weapons_available.h"
#include "..\dlls\pe_weapon_points.h"
#include "demo_api.h"

#include "pe_font.h"
#include "pe_helper.h"

// This piece code has seen too many concept changes
// I don't really like it but it works.

/*void ( F_API *ShowedHelp )( char* sPath, int type ) = NULL;
int ( F_API *NeedToShowHelp )( char* sPath, int type ) = NULL;
void ( F_API *ResetHelp )( char* sPath ) = NULL;
char path[1024] = "";

void ZeroHelp( )
{
	if( ResetHelp && strlen( path ) )
		ResetHelp( path );
}

void DidHelp( int type )
{
	if( ShowedHelp && strlen( path ) )
		ShowedHelp( path, type );
}

bool NeedHelp( int type )
{
	if( !CVAR_GET_FLOAT( "cl_help" ) )
		return false;
	if( NeedToShowHelp && strlen( path ) )
		return ( NeedToShowHelp( path, type ) < 1 ? true : false );
	return true;
}

void Help( int type, char *file )
{
	if( NeedHelp( type ) )
	{
		g_helper->ShowHelp( file );
		DidHelp( type );
	}
}*/

extern int g_sprHUD2;

#define SetBits(flBitVector, bits)		((flBitVector) = (int)(flBitVector) | (bits))
#define ClearBits(flBitVector, bits)	((flBitVector) = (int)(flBitVector) & ~(bits))
#define FBitSet(flBitVector, bit)		((int)(flBitVector) & (bit))

DECLARE_MESSAGE(m_ShowMenuPE, ShowMenuPE) 
//DECLARE_MESSAGE(m_ShowMenuPE, SendData) 

#define TITLE  "------------------------------"
#define TITLE1 "-  P-U-B-L-I-C     E-N-E-M-Y"
#define TITLE2 "------------------------------"
#define H1 "\\t\n\\t\n"
#define H2 "\\t\n\\t\n"//\\t\n\\w-Object-     -Points-\n\\t\n\\t\n\\t\n"

#define END1 "\\t\n\\t\n\\tSelect a weapon again to sell it.\n\\t\n\\tLEFT Mouse: Buy weapon for slot 3\n\\tRIGHT Mouse: Buy weapon for slot 4\n\\t\n\\tPoints: "

#define GUN "\\yPistols Menu - Part "
#define MED "\\yHeavy Weapons Menu - Part "
#define INV "\\yEquipment Menu\n"
#define INVEND "\\t\n\\t\n\\tSelect an item again to sell it.\n\\t\n\\tPoints: "//\\t\n\\wBack to Main-Menu\n\\t\n\\t\n\\tPoints: "
#define BIO "\\yHigh-Tech-Equipment Menu\n"
#define BIOEND "\\t\n\\t\n\\lRIGHT Mouse: Show item info\n\\t\n\\tSelect an item again to sell it.\n\\t\n\\tPoints: "//\\t\n\\wBack to Main-Menu\n\\t\n\\t\n\\tPoints: "

#define bit(a)	(1<<a)

#define C_GREY		1
#define C_GREEN		2
#define C_WHITE		3
#define C_RED		4

#define CURRENT_ITEM (m_iEntryCount+((m_iCat-1)*6)+1)

int iNumCats[10] = { 1, 2, 6, 6, 1, 3, 8, 1, 1, 1 };
int iAddon[4][10] =
{	{0},
	{ 0, 0, 1, 2, 0, 3, 0, 0, 0, 0 },
	{ 0, 0, 5, 0, 6, 7, 0, 0, 0, 0 },
	{ 0, 0, 8, 9, 0, 0, 0, 0, 0, 0 }
 };
int iCyber[4][10] =
{
	{0},
	{ 0, 1, 0, 0, 2, 0, 0, 0, 0, 0 },
	{ 0, 3, 0, 4, 0, 0, 0, 0, 0, 0 },
	{ 0, 5, 0, 0, 0, 0, 0, 0, 0, 0 }
};
int gLoaded = 0;
int iBlink = 0;
float fBlink = 0;

extern int g_Mouse1;
extern int g_Mouse2;
extern int g_Mouse3;
extern int g_Mouse1r;
extern int g_Mouse2r;
extern int g_Mouse3r;
extern int g_MouseX;
extern int g_MouseY;

#define CAT_Y ( 52 + 40 )
#define CAT_X ( 8 )
#define MENU_X ( 24 )
#define MENU_Y ( 77 + 40 )

#define CAT_Y_DRAW ( CAT_Y )
#define CAT_X_DRAW ( CAT_X )
#define MENU_X_DRAW ( MENU_X )
#define MENU_Y_DRAW ( MENU_Y )

int pratio[11][16][11] =
{
	//menu0
	{ { 0+3, 2+3, 4+3, 6+3, 8+3, -1, -1, -1, -1, 11+3 } },
	//menu1
	{
		{ 0, 2, 4, 6, 8, 10, -1, -1, -1, -1 },
		{ 0, 2, 4, 6, 8, -1, -1, -1, -1, -1 }
	},
	//menu2
	{
		{ 1, 3, 5, 7, 9, -1, -1, -1, -1, -1 },
		{ 1, 3,-1,-1,-1, -1, -1, -1, -1, -1 },
		{ 1, 3, 5, 7, 9, -1, -1, -1, -1, -1 },
		{ 3, 8,-1,-1,-1, -1, -1, -1, -1, -1 }
	},
	//menu3
	{
		{ 1, 3, 5, 7, 9, -1, 15, 17, 18, 20 },
		{ 1, 3,-1,-1,-1, -1, 15, 17, 18, 20 },
		{ 1, 3, 5, 7, 9, -1, 15, 17, 18, 20 },
		{ 3, 8,-1,-1,-1, -1, 15, 17, 18, 20 }
	},
	//menu4
	{ { 3, 5, 7, 9, 11, 17, 19, -1, -1, -1, -1 } },
	//menu5
	{ 
		{ 3, 5, 7, 9, 11, -1, -1, -1, -1, -1, -1 },
		{ 3, 5, 7, 9, 11, -1, -1, -1, -1, -1, -1 },
		{ 3, 5, 7, 9, 11, -1, -1, -1, -1, -1, -1 }
	},
	{ { 0 } },
	{ { 0 } },
	{ { 0 } },
	{ { 5, -1, -1, -1, -1, -1, -1, -1, -1, 7 } },
	{
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 },
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, 1 }
	}
};

int MouseOnPoint( int menu, int cat )
{
	int x = g_MouseX, y = g_MouseY, res = 0;
	if( menu == 0 )
		cat = 1;
	
	for( int i = 0; i < 11; i += 1 )
	{
		if( x > MENU_X && x < MENU_X + 220 && y > ( MENU_Y + (12*(pratio[menu][cat-1][i]-3+8)) ) && y < ( MENU_Y + (12*(pratio[menu][cat-1][i]-3+10)) ) )
		{
			if( pratio[menu][cat-1][i] == -1 )
				continue;
			if( i == 10 )
				continue;
			return i+1;
		}
	}
	for( i = 0; i < 5; i += 1 )
	{
		if( x > (int)(CAT_X+((float)i*(255.0f/5.0f))) && x < (int)(CAT_X+((float)(i+1)*(255.0f/5.0f))) && y > CAT_Y && y < CAT_Y + 23 )
		{
			if( menu > 0 )
				gHUD.m_ShowMenuPE.SelectMenuItem( 11 );
			if( i == 2 )
				i = 1;
			gHUD.m_ShowMenuPE.SelectMenuItem( i + 1 );
			return -1;
		}
	}
	int temp = 0;
	if( menu == 1 )
	{
		for( i = 0; i < 3; i += 1 )
		{
			temp = (int)(CAT_X+1+((float)(i+1)*(254.0f/2.0f)));
			temp = ( temp > CAT_X + 254 ) ? CAT_X + 254 : temp;
			if( x > (int)(CAT_X+1+((float)i*(254.0f/2.0f))) && x < temp && y > CAT_Y + 25 && y < CAT_Y + 25 + 20 )
			{
				if( cat > ( i+1 ) )
					gHUD.m_ShowMenuPE.SelectMenuItem( 8 );
				if( cat < ( i+1 ) )
					gHUD.m_ShowMenuPE.SelectMenuItem( 9 );
				return -1;
			}
		}
	}
	else if( ( menu == 2 ) || ( menu == 3 ) )
	{
		for( i = 0; i < 5; i += 1 )
		{
			temp = (int)(CAT_X+1+((float)(i+1)*(254.0f/4.0f)));
			temp = ( temp > CAT_X + 254 ) ? CAT_X + 254 : temp;
			if( x > (int)(CAT_X+1+((float)i*(254.0f/4.0f))) && x < temp && y > CAT_Y + 25 && y < CAT_Y + 25 + 20 )
			{
				gHUD.m_ShowMenuPE.m_iCat = ( i + 1 );
				gHUD.m_ShowMenuPE.ShowMenuNr( );
				return -1;
			}
		}
	}
	else if( menu == 5 )
	{
		for( i = 0; i < 4; i += 1 )
		{
			temp = (int)(CAT_X+1+((float)(i+1)*(254.0f/3.0f)));
			temp = ( temp > CAT_X + 254 ) ? CAT_X + 254 : temp;
			if( x > (int)(CAT_X+1+((float)i*(254.0f/3.0f))) && x < temp && y > CAT_Y + 25 && y < CAT_Y + 25 + 20 )
			{
				gHUD.m_ShowMenuPE.m_iCat = ( i + 1 );
				gHUD.m_ShowMenuPE.ShowMenuNr( );
				return -1;
			}
		}
	}
	if( x > ( CAT_X + 256 + 1 ) && x < ( CAT_X + 256 + 1 + 59 ) && y > ( CAT_Y + 64 + 134 ) && y < ( CAT_Y + 64 + 134 + 31 ) )
	{
		gHUD.m_ShowMenuPE.ResetAll( );
		return -1;
	}

	return -1;
}

void CHudShowMenuPE::ResetAll( )
{
	m_iPoints		= m_iPointsMax;
	m_sInfo.slot[1]	= 0;
	m_sInfo.slot[2]	= 0;
	m_sInfo.slot[3]	= 0;
	m_sInfo.slot[4]	= 0;
	m_sInfo.slot[5]	= 0;
	m_sInfo.slot[6]	= 0;
	ShowMenuNr( );
}

void CHudShowMenuPE::Color( )
{
	int color = 0;
	switch( m_iMenu )
	{
	case 1:
		if( !slot1[m_sInfo.team][CURRENT_ITEM-1] )
			color = C_GREY;
		else if( m_sInfo.slot[2] == CURRENT_ITEM )
			color = C_GREEN;
		else
			color = C_WHITE;
		break;
	case 2:
		if( m_sInfo.slot[3] == CURRENT_ITEM )
			color = C_GREEN;
		else if( m_sInfo.slot[4] == CURRENT_ITEM )
			color = C_RED;
		else if( !slot2[m_sInfo.team][CURRENT_ITEM-1] )
			color = C_GREY;
		else
			color = C_WHITE;
		break;
	case 3:
		if( m_sInfo.slot[4] == CURRENT_ITEM )
			color = C_RED;
		else if( m_sInfo.slot[3] == CURRENT_ITEM )
			color = C_GREEN;
		else if( !slot2[m_sInfo.team][CURRENT_ITEM-1] )
			color = C_GREY;
		else
			color = C_WHITE;
		break;
	case 4:
		if( FBitSet( m_sInfo.slot[5], ( 1 << m_iEntryCount ) ) )
			color = C_GREEN;
		else if( slot4[m_sInfo.team][CURRENT_ITEM-1] )
			color = C_WHITE;
		else
			color = C_GREY;
		break;
	case 5:
		if( FBitSet( m_sInfo.slot[6], ( 1 << ( iCyber[m_iCat][m_iEntryCount+1]-1 ) ) ) )
			color = C_GREEN;
		else if( FBitSet( m_sInfo.slot[1], ( 1 << ( iAddon[m_iCat][m_iEntryCount+1]-1 ) ) ) )
			color = C_GREEN;
		else if( iAddon[m_iCat][m_iEntryCount+1] && !FBitSet( m_sInfo.slot[6], ( 1 << addon[iAddon[m_iCat][m_iEntryCount+1]-1] ) ) )
			color = C_GREY;
		else
			color = C_WHITE;
		break;
	}

	switch( color )
	{
	case C_GREY:
		strcat( m_sMenu, "\\t" );
		break;
	case C_GREEN:
		strcat( m_sMenu, "\\g" );
		break;
	case C_WHITE:
		strcat( m_sMenu, "\\w" );
		break;
	case C_RED:
		strcat( m_sMenu, "\\r" );
		break;
	}

	m_iEntryCount++;
}

#define C1 Color( )
#define C2 Color( )
#define C3 Color( )
#define C4 Color( )

#define MAX_MENU_STRING 512

int CHudShowMenuPE::Init(void)
{
	char file[1024];
	char *offset;
	HOOK_MESSAGE( ShowMenuPE ); 
	//HOOK_MESSAGE( SendData ); 

	gHUD.AddHudElem(this);

	sprintf( file, "%s/eqsave.dll", gEngfuncs.pfnGetGameDirectory( ) );

	while( offset = strstr( file, "/" ) )
		*offset = '\\';

	m_hSaveDll = LoadLibrary( file );

	if( !m_hSaveDll )
	{
		m_bDllLoaded = 0;
		return 0;
	}

	if(	!( 
		//( (FARPROC&)ResetHelp = GetProcAddress( m_hSaveDll, "?ResetHelp@@YGXPAD@Z" ) ) &&
		//( (FARPROC&)ShowedHelp = GetProcAddress( m_hSaveDll, "?ShowedHelp@@YGXPADH@Z" ) ) &&
		//( (FARPROC&)NeedToShowHelp = GetProcAddress( m_hSaveDll, "?NeedToShowHelp@@YGHPADH@Z" ) ) &&
		( (FARPROC&)GetMP3Nr = GetProcAddress( m_hSaveDll, "?GetMP3Nr@@YGPADH@Z" ) ) &&
		( (FARPROC&)GetMP3s = GetProcAddress( m_hSaveDll, "?GetMP3s@@YGHPAD0@Z" ) ) &&
		( (FARPROC&)ResetMP3s = GetProcAddress( m_hSaveDll, "?ResetMP3s@@YGXXZ" ) ) &&
		( (FARPROC&)WriteData = GetProcAddress( m_hSaveDll, "?WriteData@@YGXPADHHHHHHH@Z" ) ) )
		)
	{ 
		FreeLibrary( m_hSaveDll );
		m_bDllLoaded = 0;
		return 0;
	}
	m_bDllLoaded = 1;
	return 1; 
};

int CHudShowMenuPE::VidInit(void)
{
	m_iFlags &= ~HUD_ACTIVE;
	category = 0;
	m_iPoints = 0;
	category = 0;
	iBlink = 0;
	fBlink = 0;


	m_hCSpr[0] = LoadSprite( "sprites/menu/menu1.spr" );
	m_hCSpr[1] = LoadSprite( "sprites/menu/menu2.spr" );
	m_hCSpr[2] = LoadSprite( "sprites/menu/menu3.spr" );

	m_hCSpr[10] = LoadSprite( "sprites/menu/GO.spr" );
	m_hCSpr[12] = LoadSprite( "sprites/menu/bg0.spr" );
	m_hCSpr[13] = LoadSprite( "sprites/menu/bg1.spr" );

	m_iActive = 0;
	gLoaded = 0;
	m_iFlags &= ~HUD_ACTIVE;
	if( gViewPort )
		gViewPort->UpdateCursorState( );
	return 1;
};

void CHudShowMenuPE::Update( int team, int points )
{
	char text[512];
	m_iPointsMax	= points;
	if( team < 3 )
		team = 0;
	else
		team = 3;

	LoadData( team );

	sprintf( text, "eqchange %d %d %d %d %d %d\n",	m_sInfo.slot[2],
													m_sInfo.slot[3],
													m_sInfo.slot[4],
													m_sInfo.slot[5],
													m_sInfo.slot[6],
													m_sInfo.slot[1],
													m_iPoints );
	ClientCmd( text );
}

int CHudShowMenuPE::MsgFunc_ShowMenuPE( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	g_helper->RemoveAll( );

	if( m_iActive )
	{
		ClientCmd( "eqdata\n" );
		m_iActive = 0;
		m_iFlags &= ~HUD_ACTIVE;
		if( gViewPort )
			gViewPort->UpdateCursorState( );
		return 1;
	}

	m_sInfo.team	= READ_BYTE( );
	if( m_sInfo.team < 3 )
		m_sInfo.team = 0;
	else
		m_sInfo.team = 3;
//	m_iPointsMax	= READ_BYTE( );
/*	m_sInfo.slot[1]	= READ_BYTE( );
	m_sInfo.slot[2]	= READ_BYTE( );
	m_sInfo.slot[3]	= READ_BYTE( );
	m_sInfo.slot[4]	= READ_BYTE( );
	m_sInfo.slot[5]	= READ_BYTE( );
	m_sInfo.slot[6]	= READ_BYTE( );*/
	LoadData( m_sInfo.team );

	m_iFlags &= ~HUD_ACTIVE;
	m_iFlags |= HUD_ACTIVE;

	m_iActive = 1;

	m_iMenu = 0;
	m_iCat = 0;
	SelectMenuItem( 1 );

	ShowMenuNr( );

/*	g_helper->ShowHelp( HELP_EQUIPMENU, "help/menu_cat.cfg", true );
	g_helper->ShowHelp( HELP_EQUIPMENU, "help/menu_subcat.cfg", true );
	g_helper->ShowHelp( HELP_EQUIPMENU, "help/menu_items.cfg" );*/
	return 1;
}

void CHudShowMenuPE::Think( )
{}

void CHudShowMenuPE::ShowMenuNr( )
{
	char points[64] = "", text[64] = "";
	m_iEntryCount = 0;
	sprintf( points, "%d", m_iPoints );

	category = 0;
	if( m_sInfo.slot[1] )
		SetBits( category, bit(0) );
	else
		ClearBits( category, bit(0) );

	if( m_sInfo.slot[2] )
		SetBits( category, bit(1) );
	else
		ClearBits( category, bit(1) );

	if( m_sInfo.slot[3] )
		SetBits( category, bit(2) );
	else
		ClearBits( category, bit(2) );

	if( m_sInfo.slot[4] )
		SetBits( category, bit(3) );
	else
		ClearBits( category, bit(3) );

	if( m_sInfo.slot[5] )
		SetBits( category, bit(4) );
	else
		ClearBits( category, bit(4) );
	
	if( m_sInfo.slot[6] )
		SetBits( category, bit(5) );
	else
		ClearBits( category, bit(5) );


	switch( m_iMenu )
	{
	case 0:
		{
		strcpy( m_sMenu, "\\t\n\\t\n\\yMain-Equipment Menu\n\\t\n\\w-Sub Menu-                               -Slot-\n\\t\n\\t\n\\t\n" );
		if( FBitSet( category, bit(1) ) )
			strcat( m_sMenu, "\\gPistols/Light Weapons          Slot|2|\n\\t\n" );
		else
			strcat( m_sMenu, "\\wPistols/Light Weapons          Slot|2|\n\\t\n" );
		if( FBitSet( category, bit(2) ) )
			strcat( m_sMenu, "\\gMedium/Heavy  Weapons   Slot|3|\n\\t\n" );
		else
			strcat( m_sMenu, "\\wMedium/Heavy  Weapons   Slot|3|\n\\t\n" );
		if( ( m_sInfo.team == 0 ) || ( m_sInfo.team == 3 ) )
		{
			if( FBitSet( category, bit(3) ) )
				strcat( m_sMenu, "\\gMedium/Heavy  Weapons   Slot|4|\n\\t\n" );
			else
				strcat( m_sMenu, "\\wMedium/Heavy  Weapons   Slot|4|\n\\t\n" );
		}
		else
			strcat( m_sMenu, "\\tMedium/Heavy  Weapons   Slot|4|\n\\t\n" );
		if( FBitSet( category, bit(4) ) )
			strcat( m_sMenu, "\\gEquipment/Items                 Slot|5|\n\\t\n" );
		else
			strcat( m_sMenu, "\\wEquipment/Items                 Slot|5|\n\\t\n" );
	
		if( FBitSet( category, bit(5) ) )
			strcat( m_sMenu, "\\gCyberware\n" );
		else
			strcat( m_sMenu, "\\wCyberware\n" );

		strcat( m_sMenu, "\\t\n\\t\n\\wStart playing\n\\t\n\\t\n\\tPoints: ");
		strcat( m_sMenu, points );
		m_iActive = 1;		
		return;
		}
	case 1:
		{
		strcpy( m_sMenu, H1 );
		strcat( m_sMenu, GUN );
		if( m_iCat == 1 )
		{
			strcat( m_sMenu, "1\n" );
			strcat( m_sMenu, H2 );
			
			C1; strcat( m_sMenu, "Beretta 92FS (3P)\n\\t\n" );
			C1; strcat( m_sMenu, "Beretta 92FS Akimbo(5P)\n\\t\n" );
			C1; strcat( m_sMenu, "H&K USP 9 (5P)\n\\t\n" );
			C1; strcat( m_sMenu, "H&K USP 9 MP (10P)\n\\t\n" );
			C1; strcat( m_sMenu, "Sig P226 (6P)\n\\t\n" );
			C1; strcat( m_sMenu, "Sig P226 MP (12P)\n\\t\n" );
		}
		else
		{
			strcat( m_sMenu, "2\n" );
			strcat( m_sMenu, H2 );

			C1; strcat( m_sMenu, "Sig P225 (4P)\n\\t\n" );
			C1; strcat( m_sMenu, "Desert Eagle (7P)\n\\t\n" );			
			C1;	strcat( m_sMenu, "Glock 18 semi(4P)\n\\t\n" );			
			C1;	strcat( m_sMenu, "Glock 18 auto(8P)\n\\t\n" );			
			C1;	strcat( m_sMenu, "Glock 18 auto Akimbo (12P)\n\\t\n\\t\n\\t\n\\t\n\\t\n" );
		}
		strcat( m_sMenu, END1 );
		strcat( m_sMenu, points );
		m_iActive = 1;
		return;
		}
	case 2:
	case 3:
		{
		strcpy( m_sMenu, H1 );
		strcat( m_sMenu, MED );
		sprintf( text, "%d\n", m_iCat );
		strcat( m_sMenu, text );

		if( m_iCat == 1 )
		{
			strcat( m_sMenu, "\\t-Submachine guns-\n" );
			strcat( m_sMenu, H2 );

			C2;	strcat( m_sMenu, "H&K MP5/10 (14P)\n\\t\n" );
			C2;	strcat( m_sMenu, "H&K MP5 SD6 (13P)\n\\t\n" );
			C2;	strcat( m_sMenu, "Seburo (13P)\n\\t\n" );
			C2;	strcat( m_sMenu, "IMI MicroUzi (9P)\n\\t\n" );
			C2;	strcat( m_sMenu, "IMI MicroUzi Akimbo (13P)\n\\t\n\\t\n\\t\n" );
		}
		else if( m_iCat == 2 )
		{
			strcat( m_sMenu, "\\t-Shotguns-\n" );
			strcat( m_sMenu, H2 );

			C2;	strcat( m_sMenu, "Benelli M3 (14P)\n\\t\n" );
			C2;	strcat( m_sMenu, "Spas12 (17P)\n\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n\\t\n" );
		}
		else if( m_iCat == 3 )
		{
			strcat( m_sMenu, "\\t-Machineguns-\n" );
			strcat( m_sMenu, H2 );

			C2;	strcat( m_sMenu, "AK47 (15P)\n\\t\n" );
			C2;	strcat( m_sMenu, "M16 (15P)\n\\t\n" );
			C2;	strcat( m_sMenu, "Steyr Aug (17P)\n\\t\n" );
			C2;	strcat( m_sMenu, "IMI Tavor (16P)\n\\t\n" );
			C2;	strcat( m_sMenu, "Sig 551 (18P)\n\\t\n\\t\n\\t\n" );
		}
		else
		{
			strcat( m_sMenu, "\\t-SniperRifles & HeavyMachineguns-\n" );
			strcat( m_sMenu, H2 );

			strcat( m_sMenu, "\\t-SniperRifles-\n\\t\n" );
			C2;	strcat( m_sMenu, "Stoner SR-25 (18P)\n\\t\n\\t\n" );

			strcat( m_sMenu, "\\t-HeavyMachineguns-\n\\t\n" );
			C2;	strcat( m_sMenu, "M134 Vulcan Minigun (25P)\n\\t\n\\t\n\\t\n\\t\n" );
		}
		strcat( m_sMenu, END1 );
		strcat( m_sMenu, points );
		m_iActive = 1;
		return;
		}
	case 4:
		{
		strcpy( m_sMenu, H1 );
		strcat( m_sMenu, INV );
		strcat( m_sMenu, H2 );

		strcat( m_sMenu, "\\t-Equipment-\n\\t\n\\t\n" );

		C3;	strcat( m_sMenu, "Nightvision (2P)\n\\t\n" );
		C3;	strcat( m_sMenu, "KevlarVest (6P)\n\\t\n" );
		C3;	strcat( m_sMenu, "Extremity protection (3P)\n\\t\n" );
		C3;	strcat( m_sMenu, "Helmet (4P)\n\\t\n" );
		C3;	strcat( m_sMenu, "Stimpak/Medikit +35HP (5P)\n\\t\n" );
		
		strcat( m_sMenu, "\\t\n\\t-Grenades-\n\\t\n\\t\n" );
		C3;	strcat( m_sMenu, "Multifunctional-Grenades (4P)\n\\t\n" );
		
		strcat( m_sMenu, INVEND );
		strcat( m_sMenu, points );
		m_iActive = 1;
		return;
		}
	case 5:
		{
		strcpy( m_sMenu, H1 );
		strcat( m_sMenu, BIO );
		strcat( m_sMenu, H2 );

		strcat( m_sMenu, "\\t-HighTechEquipment-\n\\t\n\\t\n" );
		if( m_iCat == 1 )
		{
			C4;	strcat( m_sMenu, "Cybertorso (10P)\n\\t\n" );
			C4;	strcat( m_sMenu, "- Cyberheart (6P)\n\\t\n" );
			C4;	strcat( m_sMenu, "- Cyberbomb (4P)\n\\t\n" );
			C4;	strcat( m_sMenu, "Cyberarms (6P)\n\\t\n" );
			C4;	strcat( m_sMenu, "- Fastjack (4P)\n\\t\n" );
			//C4;	strcat( m_sMenu, "- Smartlink (5P)\n\\t\n" );
		}
		else if( m_iCat == 2 )
		{
			C4;	strcat( m_sMenu, "Cyberlegs (6P)\n\\t\n" );
			C4;	strcat( m_sMenu, "- Achilles' heel (3P)\n\\t\n" );
			C4;	strcat( m_sMenu, "Cyberhead (8P)\n\\t\n" );		
			C4;	strcat( m_sMenu, "- Sentinel (2P)\n\\t\n" );
			C4;	strcat( m_sMenu, "- Orbitaldevice (8P)\n\\t\n" );
		}
		else
		{
			C4;	strcat( m_sMenu, "NanoCyberport (3P)\n\\t\n" );
			C4;	strcat( m_sMenu, "- Nano Factory (5P)\n\\t\n" );
			C4;	strcat( m_sMenu, "- Enhanced Skin (4P)\n\\t\n" );		
		}

		strcat( m_sMenu, BIOEND );
		strcat( m_sMenu, points );
		m_iActive = 1;
		return;
		}
	case 10:
		strcpy( m_sMenu, H1 );
		strcat( m_sMenu, H1 );
		strcat( m_sMenu, "\\w\n\\t\n\\rReturn to High-Tech Menu\n" );
		strcat( m_sMenu, H1 );
		switch( m_iCat )
		{
		case 1:
			strcat( m_sMenu, "\\yCyberTorso:\n\\t\n\\w+ 35 armor factor\n\\w+ 30% ammunition\n\\w+ 5% speed\n\\w+ 10% faster reload\n\\w+ 30 HP\n\\w- 50% taken knife damage\n\\t\n\\t\n" );
			break;
		case 2:
			strcat( m_sMenu, "\\yCyberHeart:\n\\t\n\\w+ 20 HP\n\\w+ 2.5 HP every 1.5 seconds\n\\t\n\\t\n" );
			break;
		case 3:
			strcat( m_sMenu, "\\yCyberBomb:\n\\t\n\\wExplodes on the players death\n\\wand damages enemys in a medium\n\\wrange.\n\\t\n\\wAttention: Can explode too\n\\wsoon in some special cases!\n\\t\n\\wCan be triggered, set the\n\\wactivation key in the\n\\wconfiguration menu\n\\t\n\\t\n" );
			break;
		case 4:
			strcat( m_sMenu, "\\yCyberArms:\n\\t\n\\w+ 25 armor factor\n\\w+ 42% accuracy\n\\w+ 200% knife damage\n\\w+ 20% ammunition\n\\w+ 50% faster reload\n\\t\n\\t\n" );
			break;
		case 5:
			strcat( m_sMenu, "\\yFastjack:\n\\t\n\\w+ 50% hacking speed\n\\w+ 75% knife damage\n\\w+ Increased grenade throwing range\n\\t\n\\t\n" );
			break;
		case 7:
			strcat( m_sMenu, "\\yCyberLegs:\n\\t\n\\w+ 25 armor factor\n\\w+ 25% speed\n\\w+ 50% jumpheight\n\\w- 60% fall damage\n\\w- 80% punch on hit\n\\t\n\\t\n" );
			break;
		case 8:
			strcat( m_sMenu, "\\yAchilles' heel:\n\\t\n\\w+ Increases player movement speed\n\\w  for 10 seconds after activation.\n\\t\n\\tSet the activation key in\n\\tthe configuration menu\n\\t\n\\t\n" );
			break;
		case 9:
			strcat( m_sMenu, "\\yCyberHead:\n\\t\n\\w+ 15 armor factor\n\\w+ 33% accuracy\n\\w- 25% head damage\n\\w+ 10 HP\n\\t\n\\t\n" );
			break;
		case 10:
			strcat( m_sMenu, "\\ySentinel:\n\\t\n\\w+ Nighvision\n\\w+ Increased radar range\n\\w+ Targetting (see \"cl_target\"\n\\w  cvar desc. in the manual)\n\\t\n\\t\n" );
			break;
		case 11:
			strcat( m_sMenu, "\\yOrbitalDevice:\n\\t\n\\wShoots a high energetic beam\n\\won the selected ground target.\n\\wUseable once per round.\n\\t\n\\t\n" );
			break;
		case 13:
			strcat( m_sMenu, "\\yNano cyberport:\n\\t\n\\w+ 20 HP\n\\w+ Interface for nano technology\n\\w- Experimental technology\n\\t\n\\t\n" );
			break;
		case 14:
			strcat( m_sMenu, "\\yNano Factory:\n\\t\n\\w+ 2 bullets every second.\n\\w+ 5 armor factor repaired on\n\\wa random part every 1.5 seconds\n\\w- 10% speed\n\\w- 20 HP\n\\t\n\\t\n" );
			break;
		case 15:
			strcat( m_sMenu, "\\yEnhanced Skin:\n\\t\n\\w- 33% explosion damage taken\n\\w- 20% speed\n\\w- 15% ammo\n\\w- 10% accuracy\n\\w+ Invisible to radar\n\\t\n\\t\n" );
			break;
		}
		m_iActive = 1;
		return;
	default:
		g_helper->RemoveAll( );
		m_iActive = 0;
		m_iFlags &= ~HUD_ACTIVE;
		if( gViewPort )
			gViewPort->UpdateCursorState( );
		
		sprintf( text, "eqdata %d %d %d %d %d %d\n",	m_sInfo.slot[2],
														m_sInfo.slot[3],
														m_sInfo.slot[4],
														m_sInfo.slot[5],
														m_sInfo.slot[6],
														m_sInfo.slot[1],
														m_iPoints );
		ClientCmd( text );
		SaveData( m_sInfo.team );
	}	
	return;
}

int CHudShowMenuPE::Draw( float flTime )
{
	if( !m_iActive )
		return 1;
	
	if ( gViewPort && gViewPort->IsScoreBoardVisible() )
		return 1;

	if( gViewPort )
		gViewPort->UpdateCursorState( );

	if ( gEngfuncs.pDemoAPI->IsPlayingback( ) )
		return 1;

	g_font->SetFont( FONT_WEAPONMENU );


	int nlc = 0, x = g_MouseX, y = g_MouseY, i;
	if( g_Mouse1 )
	{
		x = MouseOnPoint( m_iMenu, m_iCat );
		if( x > -1 )
			SelectMenuItem( x );
	}
	else if( g_Mouse2 && ( m_iMenu == 2 ) )
	{
		m_iMenu++;
		x = MouseOnPoint( m_iMenu, m_iCat );
		if( x > -1 )
			SelectMenuItem( x );
		g_Mouse2 = 0;
		m_iMenu--;
	}
	else if( g_Mouse2 && ( m_iMenu == 5 ) )
	{
		x = MouseOnPoint( m_iMenu, m_iCat );
		m_iMenu = 11;
		if( x > -1 )
			SelectMenuItem( x );
		else
			m_iMenu = 5;
		g_Mouse2 = 0;
	}
	if( g_Mouse1r )
	{
		if( g_MouseX > ( CAT_X + 256 + 2 ) && g_MouseX < ( CAT_X + 256 + 2 + 57 ) && g_MouseY > ( CAT_Y + 64 + 38 ) && g_MouseY < ( CAT_Y + 64 + 38 + 50 ) )
		{
			if( m_iMenu > 0 )
				SelectMenuItem( 11 );
			if( m_iMenu > 0 )
				SelectMenuItem( 11 );
			SelectMenuItem( 10 );
			ShowMenuNr( );
		}
		g_Mouse1r = 0;
	}

	int a = 255;
	int r = 255, g = 255, b = 255;
	ScaleColors( r, g, b, a );
	
	rect_s rect;
	rect.bottom = 256;
	rect.top = 0;
	rect.right = 256;
	rect.left = 0;

	SPR_Set( m_hCSpr[12], r, g, b );
	SPR_DrawHoles( 0, CAT_X_DRAW, CAT_Y_DRAW + 64, &rect );

	SPR_Set( m_hCSpr[13], r, g, b );
	SPR_DrawHoles( 0, CAT_X_DRAW, CAT_Y_DRAW + 254 + 64, &rect );

	rect.bottom = 242;
	rect.top = 182;
	rect.right = 244;
	rect.left = 0;
	SPR_Set( g_sprHUD2, 255, 255, 255 );
	SPR_DrawHoles( 0, CAT_X_DRAW + 384, CAT_Y_DRAW, &rect );
	if( gLoaded > 0 )
	{
		char text[128];
		sprintf( text, "Preset %d loaded...\n", gLoaded );
		g_font->DrawString( CAT_X_DRAW + 384, CAT_Y_DRAW + 68, text, 140, 140, 140 );
	}
	if( gLoaded < 0 )
	{
		char text[128];
		sprintf( text, "Preset %d saved...\n", gLoaded + 10 );
		g_font->DrawString( CAT_X_DRAW + 384, CAT_Y_DRAW + 68, text, 140, 140, 140 );
	}
	if( g_Mouse1 )
	{
		g_MouseX -= ( CAT_X + 384 );
		g_MouseY -= CAT_Y;
		if( ( g_MouseY > 0 ) && ( g_MouseY < 40 ) )
		{
			if( ( g_MouseX > 0 ) && ( g_MouseX < 61 ) )
			{
				LoadData( 1+10+m_sInfo.team );
				ShowMenuNr( );
			}
			if( ( g_MouseX > 60 ) && ( g_MouseX < 122 ) )
			{
				LoadData( 2+10+m_sInfo.team );
				ShowMenuNr( );
			}
			if( ( g_MouseX > 121 ) && ( g_MouseX < 183 ) )
			{
				LoadData( 3+10+m_sInfo.team );
				ShowMenuNr( );
			}
			if( ( g_MouseX > 182 ) && ( g_MouseX < 244 ) )
			{
				LoadData( 4+10+m_sInfo.team );
				ShowMenuNr( );
			}
		}
		else if( ( g_MouseY > 39 ) && ( g_MouseY < 61 ) )
		{
			if( ( g_MouseX > 0 ) && ( g_MouseX < 61 ) )
			{
				SaveData( 1+10+m_sInfo.team );
				ShowMenuNr( );
			}
			if( ( g_MouseX > 60 ) && ( g_MouseX < 122 ) )
			{
				SaveData( 2+10+m_sInfo.team );
				ShowMenuNr( );
			}
			if( ( g_MouseX > 121 ) && ( g_MouseX < 183 ) )
			{
				SaveData( 3+10+m_sInfo.team );
				ShowMenuNr( );
			}
			if( ( g_MouseX > 182 ) && ( g_MouseX < 244 ) )
			{
				SaveData( 4+10+m_sInfo.team );
				ShowMenuNr( );
			}
		}
		g_MouseX += ( CAT_X + 384 );
		g_MouseY += CAT_Y;

		g_Mouse1 = 0;
	}


	switch( m_iMenu )
	{
	case 1:
		rect.top = 64 * ( m_iCat - 1 );
		rect.bottom = 64 * ( m_iCat );
		rect.left = 0;
		rect.right = 256;
		SPR_Set( m_hCSpr[0], r, g, b );
		SPR_DrawHoles( 0, CAT_X_DRAW, CAT_Y_DRAW, &rect );
		break;
	case 2:
		if( m_iCat < 3 )
		{
			rect.top = 128 + 64 * ( m_iCat - 1 );
			rect.bottom = 128 + 64 * ( m_iCat );
			rect.left = 0;
			rect.right = 256;
			SPR_Set( m_hCSpr[0], r, g, b );
			SPR_DrawHoles( 0, CAT_X_DRAW, CAT_Y_DRAW, &rect );
		}
		else
		{
			rect.top = 64 * ( m_iCat - 3 );
			rect.bottom = 64 * ( m_iCat - 2 );
			rect.left = 0;
			rect.right = 256;
			SPR_Set( m_hCSpr[1], r, g, b );
			SPR_DrawHoles( 0, CAT_X_DRAW, CAT_Y_DRAW, &rect );
		}
		break;
	case 4:
		rect.top = 128;
		rect.bottom = 128 + 64;
		rect.left = 0;
		rect.right = 256;
		SPR_Set( m_hCSpr[1], r, g, b );
		SPR_DrawHoles( 0, CAT_X_DRAW, CAT_Y_DRAW, &rect );
		break;
	case 5:
		rect.top = 64 * ( m_iCat - 1 );
		rect.bottom = 64 * ( m_iCat );
		rect.left = 0;
		rect.right = 256;
		SPR_Set( m_hCSpr[2], r, g, b );
		SPR_DrawHoles( 0, CAT_X_DRAW, CAT_Y_DRAW, &rect );
		break;
	case 10:
		int c;
		if( m_iCat > 12 )
			c = 2;
		else if( m_iCat > 6 )
			c = 1;
		else
			c = 0;
		rect.top = 64 * c;
		rect.bottom = 64 * ( c + 1 );
		rect.left = 0;
		rect.right = 256;
		SPR_Set( m_hCSpr[2], r, g, b );
		SPR_DrawHoles( 0, CAT_X_DRAW, CAT_Y_DRAW, &rect );
		break;
	}

	SPR_Set( m_hCSpr[10], r, g, b );
	rect.bottom = 256;
	rect.top = 0;
	rect.right = 256;
	rect.left = 0;
	SPR_DrawHoles( 0, CAT_X_DRAW + 256, CAT_Y_DRAW + 64, &rect );
	
	
	
	r = 255; g = 255; b = 255; a = 128;
	ScaleColors( r, g, b, a );
	
	if( m_iMenu != 1 )
		g_font->DrawString(  CAT_X_DRAW + 6, CAT_Y_DRAW + 6, "Pistols", r, g, b );
	if( m_iMenu != 2 )
		g_font->DrawString(  CAT_X_DRAW + 62, CAT_Y_DRAW + 6, "Heavy Guns", r, g, b );
	if( m_iMenu != 4 )
		g_font->DrawString( CAT_X_DRAW + 160, CAT_Y_DRAW + 6, "Equip", r, g, b );
	if( m_iMenu != 5 )
		g_font->DrawString( CAT_X_DRAW + 210, CAT_Y_DRAW + 6, "Cyber", r, g, b );
	
	r = 0; g = 0; b = 255;
	int r1 = 0, g1 = 255, b1 = 0, a1 = 128;
	int rt = 0, gt = 255, bt = 0, at = 220;
	ScaleColors( r, g, b, a );
	ScaleColors( r1, g1, b1, a1 );
	ScaleColors( rt, gt, bt, at );

	switch( m_iMenu )
	{
	case 1:
		g_font->DrawString(  CAT_X_DRAW + 6, CAT_Y_DRAW + 6, "Pistols", rt, gt, bt );
		if( m_sInfo.slot[2] <= 0 )
			break;

		if( m_sInfo.slot[2] <= 6 )
			UTIL_FillRect( CAT_X_DRAW + 10, CAT_Y_DRAW + 34, 110, 6, r1, g1, b1, a1 );
		else if( m_sInfo.slot[2] <= 12 )
			UTIL_FillRect( CAT_X_DRAW + 133, CAT_Y_DRAW + 34, 110, 6, r1, g1, b1, a1 );
		break;
	case 2:
		g_font->DrawString(  CAT_X_DRAW + 62, CAT_Y_DRAW + 6, "Heavy Guns", rt, gt, bt );
		if( !( m_sInfo.slot[3] <= 0 ) )
		{
		if( m_sInfo.slot[3] <= 6 )
			UTIL_FillRect( CAT_X_DRAW + 7, CAT_Y_DRAW + 34, 49, 6, r1, g1, b1, a1 );
		else if( m_sInfo.slot[3] <= 12 )
			UTIL_FillRect( CAT_X_DRAW + 68, CAT_Y_DRAW + 34, 49, 6, r1, g1, b1, a1 );
		else if( m_sInfo.slot[3] <= 18 )
			UTIL_FillRect( CAT_X_DRAW + 133, CAT_Y_DRAW + 34, 50, 6, r1, g1, b1, a1 );
		else if( m_sInfo.slot[3] <= 24 )
			UTIL_FillRect( CAT_X_DRAW + 194, CAT_Y_DRAW + 34, 51, 6, r1, g1, b1, a1 );
		}
		r1 = 255, g1 = 0, b1 = 0, a1 = 128;
		ScaleColors( r1, g1, b1, a1 );

		if( m_sInfo.slot[4] <= 0 )
			break;
		if( m_sInfo.slot[4] <= 6 )
			UTIL_FillRect( CAT_X_DRAW + 7, CAT_Y_DRAW + 34, 49, 6, r1, g1, b1, a1 );
		else if( m_sInfo.slot[4] <= 12 )
			UTIL_FillRect( CAT_X_DRAW + 68, CAT_Y_DRAW + 34, 49, 6, r1, g1, b1, a1 );
		else if( m_sInfo.slot[4] <= 18 )
			UTIL_FillRect( CAT_X_DRAW + 133, CAT_Y_DRAW + 34, 50, 6, r1, g1, b1, a1 );
		else if( m_sInfo.slot[4] <= 24 )
			UTIL_FillRect( CAT_X_DRAW + 194, CAT_Y_DRAW + 34, 51, 6, r1, g1, b1, a1 );
		break;
	case 4:
		g_font->DrawString( CAT_X_DRAW + 160, CAT_Y_DRAW + 6, "Equip", rt, gt, bt );
		break;
	case 5:
		g_font->DrawString( CAT_X_DRAW + 210, CAT_Y_DRAW + 6, "Cyber", rt, gt, bt );
		if( m_sInfo.slot[6] <= 0 )
			break;

		if( FBitSet( m_sInfo.slot[6], (1<<0) ) || FBitSet( m_sInfo.slot[6], (1<<1) ) )
			UTIL_FillRect( CAT_X_DRAW + 7, CAT_Y_DRAW + 34, 71, 6, r1, g1, b1, a1 );
		if( FBitSet( m_sInfo.slot[6], (1<<2) ) || FBitSet( m_sInfo.slot[6], (1<<3) ) )
			UTIL_FillRect( CAT_X_DRAW + 89, CAT_Y_DRAW + 34, 71, 6, r1, g1, b1, a1 );
		if( FBitSet( m_sInfo.slot[6], (1<<4) ) )
			UTIL_FillRect( CAT_X_DRAW + 171, CAT_Y_DRAW + 34, 71, 6, r1, g1, b1, a1 );
		break;
		break;
	}



	x = MENU_X_DRAW; y = MENU_Y_DRAW;
	int tmpx = 0;
	tmpx = g_font->DrawString( x - 15, y - 69, TITLE, 255, 0, 0 ); // 9,8
	g_font->DrawString( x - 15, y - 57, TITLE1, 255, 0, 0 );// 9,20
	g_font->DrawString( x - 15, y - 45, TITLE2, 255, 0, 0 );// 9,31
	
	g_font->DrawString( tmpx,  y - 69, "-", 255, 0, 0 );
	g_font->DrawString( tmpx, y - 57, "-", 255, 0, 0 );
	g_font->DrawString( tmpx, y - 45, "-", 255, 0, 0 );

	for ( i = 0; i < MAX_MENU_STRING && m_sMenu[i] != '\0'; i++ )
	{
		if ( m_sMenu[i] == '\n' )
			nlc++;
	}

	r = 0; g = 0; b = 0;
	i = 0;

	while ( i < MAX_MENU_STRING && m_sMenu[i] != '\0' )
	{
		// White
		if (m_sMenu[i] == '\\' && m_sMenu[i+1] == 'w')
		{
			r = 255;
			g = 255;
			b = 255;
		}
		// Yellow
		else if (m_sMenu[i] == '\\' && m_sMenu[i+1] == 'y')
		{
			r = 255;
			g = 255;
			b = 0;
		}
		// Red
		else if (m_sMenu[i] == '\\' && m_sMenu[i+1] == 'r')
		{
			r = 255;
			g = 0;
			b = 0;
		}
		// green
		else if (m_sMenu[i] == '\\' && m_sMenu[i+1] == 'g')
		{
			r = 0;
			g = 255;
			b = 0;
		}
		// cyan
		else if (m_sMenu[i] == '\\' && m_sMenu[i+1] == 'c')
		{
			r = 0;
			g = 255;
			b = 255;
		}
		// Grey
		else if (m_sMenu[i] == '\\' && m_sMenu[i+1] == 'q')
		{
			r = 128;
			g = 128;
			b = 128;
		}
		// transparent
		else if (m_sMenu[i] == '\\' && m_sMenu[i+1] == 't')
		{
			r = 128;
			g = 128;
			b = 128;
		}
		// blinking yellow
		else if (m_sMenu[i] == '\\' && m_sMenu[i+1] == 'l')
		{
			if( fBlink <= flTime )
			{
				fBlink = flTime + 0.5;
				iBlink = !iBlink;
			}
			r = iBlink ? 255 : 0;
			g = iBlink ? 255 : 0;
			b = 0;
		}

		g_font->DrawString( x, y + 4, m_sMenu+i+2, r, g, b, 184 );
		y += 12;
		
		x = 20;

		while ( i < MAX_MENU_STRING && m_sMenu[i] != '\0' && m_sMenu[i] != '\n' )
			i++;
		if ( m_sMenu[i] == '\n' )
			i++;
	}
	char text[32];
	int nextx, health, armor, xpos, ypos;

	//if( ScreenHeight <= 480 )
		ypos = CAT_Y_DRAW + 120;
		xpos = CAT_X_DRAW + 384;

	g_font->DrawString( xpos, ypos, "Your Body Values:", 255, 0, 0 );

	//---
	// Health
	//---
	health  = (int)( 100.0f + ( 3.5f * (float)m_iPoints ) );
	health += ( FBitSet( m_sInfo.slot[5], (1<<4) ) ? 35 : 0 );
	health += ( FBitSet( m_sInfo.slot[6], (1<<0) ) ? 30 : 0 );
	health += ( FBitSet( m_sInfo.slot[6], (1<<3) ) ? 10 : 0 );
	health += ( FBitSet( m_sInfo.slot[6], (1<<4) ) ? 20 : 0 );
	health += ( FBitSet( m_sInfo.slot[1], (1<<0) ) ? 20 : 0 );
	health -= ( FBitSet( m_sInfo.slot[1], (1<<7) ) ? 20 : 0 );
	nextx = g_font->DrawString( xpos, ypos + 12, "Hitpoints:", 255, 255, 255 );
	nextx += 15;
	sprintf( text, "%d", ( health <= 240 ? health : 240 ) );
	//text[3] = '\0';
	g_font->DrawString( nextx + 10, ypos + 12, text, 0, 255, 0 );

	//---
	// Armor
	//---
	armor  = ( FBitSet( m_sInfo.slot[5], (1<<3) ) ? 25 : 0 );
	armor += ( FBitSet( m_sInfo.slot[5], (1<<2) ) ? 25 : 0 );
	armor += ( FBitSet( m_sInfo.slot[5], (1<<1) ) ? 50 : 0 );

	armor += ( FBitSet( m_sInfo.slot[6], (1<<0) ) ? (int)(70.0/4.0) : 0 );
	armor += ( FBitSet( m_sInfo.slot[6], (1<<1) ) ? (int)(50.0/16.0) : 0 );
	armor += ( FBitSet( m_sInfo.slot[6], (1<<2) ) ? (int)(50.0/16.0) : 0 );
	armor += ( FBitSet( m_sInfo.slot[6], (1<<3) ) ? (int)(15.0/4.0) : 0 );
	
	g_font->DrawString( xpos, ypos + 24, "Armor:", 255, 255, 255 );

	sprintf( text, "%d", armor );
	g_font->DrawString( nextx + 10, ypos + 24, text, 0, 255, 0 );

	//---
	// Speed
	//---
	strcpy( text, "" );
	float speed = 100;
	speed += ( 1.2f * (float)m_iPoints );
	speed += ( FBitSet( m_sInfo.slot[6], (1<<0) ) ?  5 : 0 );
	speed += ( FBitSet( m_sInfo.slot[6], (1<<2) ) ? 25 : 0 );
	speed -= ( FBitSet( m_sInfo.slot[1], (1<<7) ) ? 10 : 0 );
	speed -= ( FBitSet( m_sInfo.slot[1], (1<<8) ) ? 25 : 0 );

	g_font->DrawString( xpos, ypos + 36, "Speed:", 255, 255, 255 );
	sprintf( text, "%d%%", (int)speed );
	//text[3] = '\0';
	g_font->DrawString( nextx + 10, ypos + 36, text, 0, 255, 0 );

	//---
	// Jumpheight
	//---
	g_font->DrawString( xpos, ypos + 48, "Jumpheight:", 255, 255, 255 );
	sprintf( text, ( FBitSet( m_sInfo.slot[6], (1<<2) ) ? "Improved" : "Normal" ) );
	//text[3] = '\0';
	g_font->DrawString( nextx + 10, ypos + 48, text, 0, 255, 0 );

	//---
	// Accuracy
	//---
	g_font->DrawString( xpos, ypos + 60, "Accuracy:", 255, 255, 255 );
	float acc = 100;
	
	acc += ( FBitSet( m_sInfo.slot[6], (1<<1) ) ? 42 : 0 );
	acc += ( FBitSet( m_sInfo.slot[6], (1<<3) ) ? 33 : 0 );
	acc -= ( FBitSet( m_sInfo.slot[1], (1<<8) ) ? 15 : 0 );

	sprintf( text, "%d%%", (int)acc );
	g_font->DrawString( nextx + 10, ypos + 60, text, 0, 255, 0 );

	return 1;
}

int CHudShowMenuPE::IsAvailable( int slot )
{
	switch( m_iMenu )
	{
	case 1:
		if( ( m_iPoints + points[0][m_sInfo.slot[m_iMenu+1]] - points[0][slot] ) < 0 )
			return FALSE;
		if( !slot1[m_sInfo.team][slot-1] )
			return FALSE;
		break;
	case 2:
	case 3:
		if( ( m_iPoints + points[1][m_sInfo.slot[m_iMenu+1]] - points[1][slot] ) < 0 )
			return FALSE;
		if( !slot2[m_sInfo.team][slot-1] || ( m_iMenu == 2 ? slot == m_sInfo.slot[4] : slot == m_sInfo.slot[3] ) )
			return FALSE;
		break;
	case 4:
		if( ( m_iPoints - points[2][slot-1] ) < 0 )
			return FALSE;
		if( !slot4[m_sInfo.team][slot-1] )
			return FALSE;
		break;
	case 5:
		if( slot < 10 )
		{
			if( ( m_iPoints - points[3][slot-1] ) < 0 )
				return FALSE;
		}
		else
		{
			slot -= 10;
			if( !FBitSet( m_sInfo.slot[6], ( 1 << addon[slot-1] ) ) )
				return FALSE;
			if( ( m_iPoints - points[4][slot-1] ) < 0 )
				return FALSE;
		}
		break;
	case 10:
		if( ( m_iPoints - points[3][slot-1] ) < 0 )
			return FALSE;
		break;
	}

	return TRUE;
}

void CHudShowMenuPE::SelectMenuItem( int slot )
{
	gLoaded = 0;

	switch( m_iMenu )
	{
	case 0:
		if( slot == 10 )
		{
			m_iMenu = -1;
			break;
		}
		if( m_iMenu == slot )
			return;
		if( slot == 3 && m_sInfo.team != 0 && m_sInfo.team != 3  )
			return;
		m_iMenu = slot;
		m_iCat = 1;
		break;
	case 1:
		switch( slot )
		{
		case 7:
			m_iPoints += points[0][m_sInfo.slot[2]];
			m_sInfo.slot[2] = 0;
			break;
		case 8:
			if( m_iCat > 1 )
				m_iCat--;
			else
				return;
			break;
		case 9:
			m_iCat++;
			if( m_iMenu && ( m_iCat > iNumCats[1] ) )
				m_iCat = iNumCats[1];
			break;
		case 11:
			m_iMenu = 0;
			m_iCat = 0;
			break;
		case 10:
		case 0:
			m_iMenu = -1;
			m_iCat = 0;
			break;
		default:
			if( ( slot + ( 6 * ( m_iCat - 1 ) ) ) == m_sInfo.slot[2] )
			{
				m_iPoints += points[0][m_sInfo.slot[2]];
				m_sInfo.slot[2] = 0;
				break;
			}
			else if( IsAvailable( slot + ( 6 * ( m_iCat - 1 ) ) ) )
			{
				m_iPoints += points[0][m_sInfo.slot[2]];

				m_sInfo.slot[2] = slot + ( 6 * ( m_iCat - 1 ) );
	
				m_iPoints -= points[0][m_sInfo.slot[2]];
			}
			else
				return;
			break;

		}
		break;
	case 2:
	case 3:
		switch( slot )
		{
		case 7:
			if( m_iMenu == 1 )
				m_iPoints += points[0][m_sInfo.slot[m_iMenu+1]];
			else
				m_iPoints += points[1][m_sInfo.slot[m_iMenu+1]];
		
			m_sInfo.slot[m_iMenu+1] = 0;
			break;
		case 8:
			if( m_iCat > 1 )
				m_iCat--;
			else
				return;
			break;
		case 9:
			m_iCat++;
			if( m_iMenu && ( m_iCat > iNumCats[m_iMenu] ) )
				m_iCat = iNumCats[m_iMenu];
			break;
		case 11:
			m_iMenu = 0;
			m_iCat = 0;
			break;
		case 10:
		case 0:
			m_iMenu = -1;
			m_iCat = 0;
			break;
		default:
			if( ( slot + ( 6 * ( m_iCat - 1 ) ) ) == m_sInfo.slot[m_iMenu+1] )
			{
				if( m_iMenu == 1 )
					m_iPoints += points[0][m_sInfo.slot[m_iMenu+1]];
				else
					m_iPoints += points[1][m_sInfo.slot[m_iMenu+1]];

				m_sInfo.slot[m_iMenu+1] = 0;
				break;
			}
			else if( IsAvailable( slot + ( 6 * ( m_iCat - 1 ) ) ) )
			{
				if( m_iMenu == 1 )
					m_iPoints += points[0][m_sInfo.slot[m_iMenu+1]];
				else
					m_iPoints += points[1][m_sInfo.slot[m_iMenu+1]];

				m_sInfo.slot[m_iMenu+1] = slot + ( 6 * ( m_iCat - 1 ) );
	
				if( m_iMenu == 1 )
					m_iPoints -= points[0][m_sInfo.slot[m_iMenu+1]];
				else
					m_iPoints -= points[1][m_sInfo.slot[m_iMenu+1]];
			}
			else
				return;
			break;

		}
		break;
	case 4:
		switch( slot )
		{
		case 11:
			m_iMenu = 0;
			m_iCat = 0;
			break;
		case 0:
		case 10:
			m_iMenu = -1;
			m_iCat = 0;
			break;
		default:
			if( FBitSet( m_sInfo.slot[m_iMenu+1], (1<<(slot-1)) ) )
			{
				ClearBits( m_sInfo.slot[m_iMenu+1], (1<<(slot-1)) );
				m_iPoints += points[m_iMenu-2][slot-1];
			}
			else if( IsAvailable( slot ) )
			{
				m_sInfo.slot[m_iMenu+1] |= (1<<(slot-1));
				m_iPoints -= points[m_iMenu-2][slot-1];
			}
			break;
		}
		break;
	case 5:
		switch( slot )
		{
		case 0:
		case 10:
			m_iMenu = -1;
			m_iCat = 0;
			break;
		case 11:
			m_iMenu = 0;
			m_iCat = 0;
			break;
		case 8:
			if( m_iCat > 1 )
				m_iCat--;
			else
				return;
			break;
		case 9:
			m_iCat++;
			if( m_iMenu && ( m_iCat > iNumCats[5] ) )
				m_iCat = iNumCats[5];
			break;
		default:
			if( iAddon[m_iCat][slot] )
			{
				if( FBitSet( m_sInfo.slot[1], (1<<(iAddon[m_iCat][slot]-1)) ) )
				{
					ClearBits( m_sInfo.slot[1], (1<<(iAddon[m_iCat][slot]-1)) );
					m_iPoints += points[4][iAddon[m_iCat][slot]-1];
					break;
				}
				else if( IsAvailable( iAddon[m_iCat][slot] + 10 ) )
				{
					m_sInfo.slot[1] |= (1<<(iAddon[m_iCat][slot]-1));
					m_iPoints -= points[4][iAddon[m_iCat][slot]-1];
				}
			}
			else if( iCyber[m_iCat][slot] )
			{
				if( FBitSet( m_sInfo.slot[6], (1<<(iCyber[m_iCat][slot]-1)) ) )
				{
					ClearBits( m_sInfo.slot[6], (1<<(iCyber[m_iCat][slot]-1)) );
					m_iPoints += points[3][iCyber[m_iCat][slot]-1];
					slot++;
					while( iAddon[m_iCat][slot] )
					{
						if( FBitSet( m_sInfo.slot[1], (1<<(iAddon[m_iCat][slot]-1)) ) )
						{
							ClearBits( m_sInfo.slot[1], (1<<(iAddon[m_iCat][slot]-1)) );
							m_iPoints += points[4][iAddon[m_iCat][slot]-1];
						}
						slot++;
					}
					break;
				}
				else if( IsAvailable( iCyber[m_iCat][slot] ) )
				{
					m_sInfo.slot[6] |= (1<<(iCyber[m_iCat][slot]-1));
					m_iPoints -= points[3][iCyber[m_iCat][slot]-1];
				}
			}

			/*m_iMenu = 10;
			m_iCat = slot;*/
			break;
		}
		break;
	case 10:
		switch( slot )
		{
		case 11:
			m_iMenu = 0;
			m_iCat = 0;
			break;
		case 0:
		case 10:
			m_iMenu = 5;
			if( m_iCat > 12 )
				m_iCat = 3;
			else if( m_iCat > 6 )
				m_iCat = 2;
			else
				m_iCat = 1;
			break;
		}
		break;
	case 11:
		switch( slot )
		{
		case 11:
			m_iMenu = 0;
			m_iCat = 0;
			break;
		case 0:
		case 10:
			m_iMenu = 5;
			m_iCat = 1;
			break;
		default:
			if( !points[3][slot-1] && !points[4][slot-1] )
				return;
			m_iMenu = 10;
			m_iCat = (m_iCat-1)*6 + slot;
			break;
		}
		break;
	}
	ShowMenuNr( );
}

void CHudShowMenuPE::SaveData( int nr )
{
	if( !m_bDllLoaded )
		return;
	
	char file[256];
	char *slash;
	sprintf( file, "%s", gEngfuncs.pfnGetGameDirectory( ) );
	while( slash = strstr( file, "/" ) )
		*slash = '\\';

	if( nr > 10 )
		gLoaded = ( nr - 20 ) - m_sInfo.team;
	else
		gLoaded = 0;

	WriteData( file, nr, m_sInfo.slot[2], m_sInfo.slot[3], m_sInfo.slot[4], m_sInfo.slot[5], m_sInfo.slot[6], m_sInfo.slot[1] );
}

void CHudShowMenuPE::CalcPoints( )
{
	int i = 0, sum = 0;
	m_iPoints = m_iPointsMax;
	m_iPoints -= points[0][m_sInfo.slot[2]] + points[1][m_sInfo.slot[3]] + points[1][m_sInfo.slot[4]];
	for( i = 0; i < 16; i++ )
	{
		sum += FBitSet( m_sInfo.slot[5], bit(i) ) ? points[2][i] : 0;
		sum += FBitSet( m_sInfo.slot[6], bit(i) ) ? points[3][i] : 0;
		sum += FBitSet( m_sInfo.slot[1], bit(i) ) ? points[4][i] : 0;
	}
	m_iPoints -= sum;
}

int IsAvailable2( int menu, int team, int* slot, int point, int cyber )
{
	int temp = point, i = 0;
	switch( menu )
	{
	case 4:
		for( i = 0; i < 7; i++ )
		{
			if( !FBitSet( *slot, (1<<i) ) )
				continue;

			temp -= points[2][i];
			/*char text[512];

			if( temp < 0 )
			{
				sprintf( text, "say too few points for %d\n", i );
				ClientCmd( text );

				return FALSE;
			}
			/*if( !slot4[team-1][i] )
			{
				sprintf( text, "say %d not available\n", i );
				ClientCmd( text );

				return FALSE;
			}*/
			if( temp < 0 )
			{
				temp += points[2][i];
				ClearBits( *slot, (1<<i) );
			}
		}
		break;
	case 5:
		for( i = 0; i < 8; i++ )
		{
			if( !FBitSet( *slot, (1<<i) ) )
				continue;
				
			temp -= points[3][i];

			if( temp < 0 )
			{
				temp += points[3][i];
				ClearBits( *slot, (1<<i) );
			}
		}
		break;
	case 6:
		for( i = 0; i < 8; i++ )
		{
			if( !FBitSet( *slot, (1<<i) ) )
				continue;
				
			temp -= points[4][i];
			
			if( temp < 0 )
			{
				temp += points[4][i];
				ClearBits( *slot, (1<<i) );
			}
			if( !FBitSet( cyber, ( 1 << addon[i] ) ) )
				ClearBits( *slot, (1<<i) );
		}
		break;
	}

	return TRUE;
}

void CHudShowMenuPE::LoadData( int nr )
{
	char token[1024];
	char filet[64];
	int menu, slot = 0;

	sprintf( filet, "SAVE\\pe_equip.t%02d", nr );

	char *pstart = (char*)gEngfuncs.COM_LoadFile( filet, 5, NULL);
	char *pfile = pstart;
	if( !pfile )
			return;

	if( nr > 10 )
		gLoaded = ( nr - 10 ) - m_sInfo.team;
	else
		gLoaded = 0;

	m_sInfo.slot[1] = 0;
	m_sInfo.slot[2] = 0;
	m_sInfo.slot[3] = 0;
	m_sInfo.slot[4] = 0;
	m_sInfo.slot[5] = 0;
	m_sInfo.slot[6] = 0;
	
	//pfile = gEngfuncs.COM_ParseFile( pfile, token );

	menu = m_iMenu;
	m_iPoints = m_iPointsMax;
	
	pfile = gEngfuncs.COM_ParseFile( pfile, token );
	m_iMenu = 1;
	m_sInfo.slot[2] = ( IsAvailable( atoi(token) ) ? atoi(token) : 0 );
	CalcPoints( );

	pfile = gEngfuncs.COM_ParseFile( pfile, token );
	m_iMenu = 2;
	m_sInfo.slot[3] = ( IsAvailable( atoi(token) ) ? atoi(token) : 0 );
	CalcPoints( );

	pfile = gEngfuncs.COM_ParseFile( pfile, token );
	m_iMenu = 3;
	m_sInfo.slot[4] = ( IsAvailable( atoi(token) ) ? atoi(token) : 0 );
	CalcPoints( );

	pfile = gEngfuncs.COM_ParseFile( pfile, token );
	m_iMenu = 4;
	slot = atoi(token);
	m_sInfo.slot[5] = ( IsAvailable2( 4, m_sInfo.team, &slot, m_iPoints, 0 ) ? slot : 0 );
	CalcPoints( );

	pfile = gEngfuncs.COM_ParseFile( pfile, token );
	m_iMenu = 5;
	slot = atoi(token);
	m_sInfo.slot[6] = ( IsAvailable2( 5, m_sInfo.team, &slot, m_iPoints, 0 ) ? slot : 0 );
	CalcPoints( );

	pfile = gEngfuncs.COM_ParseFile( pfile, token );
	slot = atoi(token);
	m_sInfo.slot[1] = ( IsAvailable2( 6, m_sInfo.team, &slot, m_iPoints, m_sInfo.slot[6] ) ? slot : 0 );
	CalcPoints( );
	gEngfuncs.COM_FreeFile( pstart );

	m_iMenu = menu;
}
