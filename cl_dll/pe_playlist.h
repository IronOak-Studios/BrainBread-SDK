
typedef struct s_song
{
	char cFile[128];
	char cTitle[128];
	int iLenght;
	int iNum;
	s_song *pNext;
	s_song *pPrev;
} t_song;

class cPlayList
{
public:
	void		BuildListFromMap( char* cMapname );
	void		BuildListFromFile( char* cListname );
	void		Clear( );

	void		SetNext( );
	void		SetNum( int iNum );
	char*		GetFilename( );
	char*		GetTitle( );
	int			GetLength( );
	int			GetNum( );

	t_song		*m_pFirst;
	t_song		*m_pLast;
	t_song		*m_pCur;
	int			m_iNumSongs;
	int			m_iCurNum;
};