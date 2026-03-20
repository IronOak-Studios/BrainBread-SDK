#ifndef PE_FADER_H
#define PE_FADER_H

#define TYPE_NOREPEAT (1<<0)

#define MAX_FADERNAME_LEN 32

struct fade_step
{
	int id;
	int color[4];
	float percent;
	fade_step *prev;
	fade_step *next;
};

struct fader_type
{
	char name[MAX_FADERNAME_LEN];
	int numsteps;
	int flags;
	fade_step *fstep;
	fade_step *lstep;
	fader_type *next;
};


class cPEFader
{
	float start;
	float length;
	float nextstep;
	fader_type *type;
	fade_step *step;

	static fader_type *ftypes;
	static fader_type *ltypes;
	static fader_type *curenttype;
	static int numtypes;
public:
	static bool initialized;

	cPEFader( );
	
	void Start( char *fname, float starttime, float endtime );
	void Reset( );
	void GetColor( float time, int &r, int &g, int &b );
	void GetColor( float time, int &r, int &g, int &b, int &a );
	static void AddType( char *fname );
	static void AddStep( int r, int g, int b, int a, float percent );
	static void AddStep( char *fname, int r, int g, int b, int a, float percent );
	static void SetFlags( int flags );
	static void DeleteAll( );
	static void Init( );
};

#endif //PE_FADER_H