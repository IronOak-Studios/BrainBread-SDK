#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parsemsg.h"

DECLARE_MESSAGE(m_NVG, NVGActive )
DECLARE_MESSAGE(m_NVG, NVG )

// Done by Grobi and changed by ste0

int CHudNVG::Init(void)
{
	HOOK_MESSAGE( NVGActive );
	HOOK_MESSAGE( NVG );
	m_iNVG = 0;
	m_iOn = 0;
	m_iFlags = 0;
	gHUD.AddHudElem(this);
	srand( (unsigned)time( NULL ) );
	return 1;
}

int CHudNVG::VidInit(void)
{
    
	m_iOn = 0;
	m_hFlackern = LoadSprite("sprites/nvg.spr");
	m_iNVG = 0;
	m_iOn = 0;
	m_iFlags = 0;

    return 1;
}

int CHudNVG::Draw(float fTime)
{
	if (m_iOn)
	{
		int x, y, w, h;
		int frame;
		 SPR_Set(m_hFlackern, 10, 140, 10 );

		frame = (int)(fTime * 15) % SPR_Frames(m_hFlackern);

		w = SPR_Width(m_hFlackern,0);
		h = SPR_Height(m_hFlackern,0);



		for(y = -(rand() % h); y < ScreenHeight; y += h) {
			for(x = -(rand() % w); x < ScreenWidth; x += w) {
				SPR_DrawAdditive( frame,  x, y, NULL );
			}
		}
	}

	return 1;
}

extern int iSentinel;

int CHudNVG::MsgFunc_NVG(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	m_iNVG = READ_BYTE();
	/*if( m_iNVG && ( gHUD.m_ShowMenuPE.m_sInfo.slot[1] & (1<<5) ) )
		iSentinel = 1;
	else
		iSentinel = 0;*/

	m_iFlags |= HUD_ACTIVE;

	m_iOn = 0;

	return 1;
}

int CHudNVG::MsgFunc_NVGActive(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	m_iOn = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;

	return 1;
}
