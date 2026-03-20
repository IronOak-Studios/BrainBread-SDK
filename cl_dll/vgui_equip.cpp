#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"

extern int g_Mouse1;
extern int g_Mouse2;
extern int g_Mouse3;
extern int g_Mouse1r;
extern int g_Mouse2r;
extern int g_Mouse3r;
extern int g_MouseX;
extern int g_MouseY;
class CMyInputHandler : public InputSignal
{
public:
	bool bPressed;

	CMyInputHandler()
	{
	}

	virtual void cursorMoved(int x,int y,Panel* panel)
	{
		g_MouseX = x;
		g_MouseY = y;
	}
	virtual void cursorEntered(Panel* panel) {}
	virtual void cursorExited(Panel* panel) {}
	virtual void mousePressed(MouseCode code,Panel* panel) 
	{
		if( code == MOUSE_LEFT )
			g_Mouse1 = 1;
		if( code == MOUSE_RIGHT )
			g_Mouse2 = 1;
		if( code == MOUSE_MIDDLE )
			g_Mouse3 = 1;
		if( code == MOUSE_LEFT )
			g_Mouse1r = 0;
		if( code == MOUSE_RIGHT )
			g_Mouse2r = 0;
		if( code == MOUSE_MIDDLE )
			g_Mouse3r = 0;
	}
	virtual void mouseReleased(MouseCode code,Panel* panel)
	{
		if( code == MOUSE_LEFT )
			g_Mouse1r = 1;
		if( code == MOUSE_RIGHT )
			g_Mouse2r = 1;
		if( code == MOUSE_MIDDLE )
			g_Mouse3r = 1;
	}

	virtual void mouseDoublePressed(MouseCode code,Panel* panel) {}
	virtual void mouseWheeled(int delta,Panel* panel) {}
	virtual void keyPressed(KeyCode code,Panel* panel) {}
	virtual void keyTyped(KeyCode code,Panel* panel) {}
	virtual void keyReleased(KeyCode code,Panel* panel) {}
	virtual void keyFocusTicked(Panel* panel) {}
};

CEquip :: CEquip(int iTrans, int iRemoveMe, int x, int y, int wide, int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	addInputSignal( new CMyInputHandler() );

	/*CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hSmallScheme = pSchemes->getSchemeHandle("Scoreboard Text");

	Font *smallfont = pSchemes->getFont(hSmallScheme);
	m_pBt1 = new CommandButton( "Slot1", 8, 12, 54, 23 );
	m_pBt1->setFont(smallfont);
	m_pBt1->setContentAlignment( vgui::Label::a_center );
	m_pBt1->setBorder( new LineBorder( 1, Color(255, 255, 0, 180) ) );
	m_pBt1->setParent(this);
	m_pBt2 = new CommandButton( "Slot2", 8 + 54, 12, 54, 23 );
	m_pBt2->setFont(smallfont);
	m_pBt2->setContentAlignment( vgui::Label::a_center );
	m_pBt2->setBorder( new LineBorder( 1, Color(255, 255, 0, 180) ) );
	m_pBt2->setParent(this);
	m_pBt3 = new CommandButton( "Slot3", 8 + 54 * 2, 12, 54, 23 );
	m_pBt3->setFont(smallfont);
	m_pBt3->setContentAlignment( vgui::Label::a_center );
	m_pBt3->setBorder( new LineBorder( 1, Color(255, 255, 0, 180) ) );
	m_pBt3->setParent(this);
	m_pBt4 = new CommandButton( "Slot4", 8 + 54 * 3, 12, 54, 23 );
	m_pBt4->setFont(smallfont);
	m_pBt4->setContentAlignment( vgui::Label::a_center );
	m_pBt4->setBorder( new LineBorder( 1, Color(255, 255, 0, 180) ) );
	m_pBt4->setParent(this);
	m_pBt5 = new CommandButton( "Slot5", 8 + 54 * 4, 12, 54, 23 );
	m_pBt5->setFont(smallfont);
	m_pBt5->setContentAlignment( vgui::Label::a_center );
	m_pBt5->setBorder( new LineBorder( 1, Color(255, 255, 0, 180) ) );
	m_pBt5->setParent(this);
	m_pPanel1 = new CTransparentPanel( 255, 9, 34, 270, 500 );
	m_pPanel1->setParent(this);
	m_pPanel1->setBorder( new LineBorder( 1, Color(255, 170, 0, 0) ) );
	m_pPanel2 = new CTransparentPanel( 0, 9, 34, 53, 1 );
	m_pPanel2->setParent(this);
	m_pPanel2->setBorder( new LineBorder( 1, Color( 0, 0, 0, 0) ) );*/
	//m_pPanel2 = new CTransparentPanel(255, 8 + 54, 12, 54, 23 );
	//m_pPanel2->setParent(this);
	//m_pPanel2->setBorder( new LineBorder( 1, Color(255, 0, 0, 180) ) );
	//m_pPanel = new CTransparentPanel(255, 102, ScreenHeight * 0.5 - 18, (ScreenWidth-198), 36 );
	//m_pPanel->setParent(this);
	//m_pPanel->setBorder( new LineBorder( 1, Color(255, 0, 0, 128) ) );
}