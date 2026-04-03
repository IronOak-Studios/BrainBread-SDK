#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include "pe_fader.h"

// Modulus for floating point numbers
#define MOD( a, b ) ( (a) - (int)( (a) / (b) ) * (b) )

int cPEFader::numtypes = 0;
fader_type *cPEFader::ftypes = NULL;
fader_type *cPEFader::ltypes = NULL;
fader_type *cPEFader::curenttype = NULL;
bool cPEFader::initialized = false;

void cPEFader::Init( )
{
  ftypes = NULL;
  if( !CVAR_GET_FLOAT( "cl_fadesys" ) )
    return;

	if( initialized )
		return;
  ConsolePrint( "Loading faders from pe_faders.cfg... " );
	initialized = true;
	char token[1024] = "";
	int r, g, b, a;
	float p, lp = -1;
	char *pstart = (char*)gEngfuncs.COM_LoadFile( "pe_faders.cfg", 5, NULL);
	char *pfile = pstart;
	if( !pfile )
  {
    ConsolePrint( "FAILED, unable to open file.\n" );
		return;
  }
	pfile = gEngfuncs.COM_ParseFile( pfile, token );
	while( pfile && strlen( token ) )
	{
		cPEFader::AddType( token );
		pfile = gEngfuncs.COM_ParseFile( pfile, token );
		if( !pfile || token[0] != '{' )
    {
      ConsolePrint( "FAILED, syntax error. Expecting \"{\".\n" );
	  initialized = false;
		  return;
    }
		pfile = gEngfuncs.COM_ParseFile( pfile, token );
    lp = -1;
		while( pfile && token[0] != '}' )
		{
			r = atoi( token );
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			if( !pfile ) break;
			g = atoi( token );
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			if( !pfile ) break;
			b = atoi( token );
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			if( !pfile ) break;
			a = atoi( token );
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
			if( !pfile ) break;
			p = atof( token );
			if( p <= lp )
			{
				ConsolePrint( "FAILED, syntax error. step percent < previous step percent, missing token?\n" );
				initialized = false;
				return;

			}
			cPEFader::AddStep( r, g, b, a, p );
			lp = p;
			pfile = gEngfuncs.COM_ParseFile( pfile, token );
		}
		if( !pfile ) break;
		pfile = gEngfuncs.COM_ParseFile( pfile, token );
	}
	gEngfuncs.COM_FreeFile( pstart );
  ConsolePrint( "done\n" );
}

void cPEFader::AddType( char *fname )
{
	if( !CVAR_GET_FLOAT( "cl_fadesys" ) )
		return;
	fader_type *nt = new fader_type;
	strncpy( nt->name, fname, MAX_FADERNAME_LEN - 1 );
	nt->name[MAX_FADERNAME_LEN - 1] = '\0';
	numtypes++;
	nt->numsteps = 0;
	nt->fstep = NULL;
	nt->lstep = NULL;
	nt->next = NULL;
  nt->flags = 0;
	curenttype = nt;

	if( !ftypes )
	{
		ltypes = ftypes = nt;
        return;
	}		
	ltypes->next = nt;
	ltypes = nt;
}

void cPEFader::AddStep( char *fname, int r, int g, int b, int a, float percent )
{
	if( !CVAR_GET_FLOAT( "cl_fadesys" ) )
		return;
	fader_type *it = ftypes;
	while( it )
	{
		if( !stricmp( it->name, fname ) )
			break;
		it = it->next;
	}
	if( !it )
		return;
	curenttype = it;
	AddStep( r, g, b, a, percent );
}

void cPEFader::AddStep( int r, int g, int b, int a, float percent )
{
	if( !CVAR_GET_FLOAT( "cl_fadesys" ) )
		return;
	if( !curenttype )
		return;
	fade_step *ns = new fade_step;
	ns->id = curenttype->numsteps++;
	ns->color[0] = r;
	ns->color[1] = g;
	ns->color[2] = b;
	ns->color[3] = a;
	ns->percent = percent;
	ns->prev = NULL;
	ns->next = NULL;

	if( !curenttype->fstep )
	{
		curenttype->lstep = curenttype->fstep = ns;
		return;
	}
	ns->prev = curenttype->lstep;
	curenttype->lstep->next = ns;
	curenttype->lstep = ns;
}

void cPEFader::SetFlags( int flags )
{
	if( !curenttype )
		return;
	curenttype->flags = flags;
}

void cPEFader::DeleteAll( )
{
	fader_type *itt = ftypes;
	fader_type *delt = NULL;
	fade_step *its = NULL;
	fade_step *dels = NULL;
	while( itt )
	{
		its = itt->fstep;
		while( its )
		{
			dels = its;
			its = its->next;
			delete dels;
		}
		delt = itt;
		itt = itt->next;
		delete delt;
	}
	ftypes = ltypes = NULL;
	numtypes = 0;
  initialized = false;
}


cPEFader::cPEFader( )
{
	Reset( );
}

void cPEFader::Reset( )
{
	type = NULL;
	start = 0;
	length = 0;
	step = NULL;
	nextstep = 0;
}

void cPEFader::Start( char *fname, float starttime, float endtime )
{
	type = ftypes;
	while( type )
	{
		if( !stricmp( type->name, fname ) )
			break;
		type = type->next;
	}
	if( !type )
		return;

	start = starttime;
	length = endtime - starttime;
	step = type->fstep;
	if( !step )
		return;
	nextstep = start + length * ( step->percent / 100.0f );
}

void cPEFader::GetColor( float time, int &r, int &g, int &b )
{
	int a;
	GetColor( time, r, g, b, a );
	ScaleColors( r, g, b, a );
}

void cPEFader::GetColor( float time, int &r, int &g, int &b, int &a )
{
	if( !type || time < start || !length )
		return;
	/*if( time > ( start + length ) )
	{
		if( type->flags & TYPE_NOREPEAT )
		{
			r = step->color[0];
			g = step->color[1];
			b = step->color[2];
			a = step->color[3];
			return;
		}
		start = time;
		//step = type->fstep;
		//nextstep = -1;
	}
  /*if( step->next && time > step->next->percent || !step->next && time > type->fstep->percent )
  {
    step = type->fstep;
    nextstep = start + length * ( step->percent / 100.0f );
  }
  while( nextstep <= time )
	{
		if( !step->next )
			step = type->fstep->next;
		else
			step = step->next;
		if( !step )
			return;
		nextstep = start + length * ( step->percent / 100.0f );
	}*/
  step = type->fstep;
  float curpercent = ( time - start ) / length * 100.0f;
  if( !( type->flags & TYPE_NOREPEAT ) )
  {
    while( curpercent > 100.0f )
      curpercent -= 100.0f;
  }
  else
  {
	  if( curpercent > 100.0f )
		  curpercent = 100.0f;
  }

  if( curpercent != 100.0f )
  {
    while( step->percent <= curpercent )
	  {
		  if( !step->next )
			  step = type->fstep->next;
		  else
			  step = step->next;
		  if( !step )
			  return;
	  }
  }
  else
    step = type->lstep;

	if( !step->prev )
		return;
	float steppercent = step->percent - step->prev->percent;
	curpercent -= step->prev->percent;
	
	/*char text[512];
	sprintf( text, "stepp:%f, curp:%f\n", steppercent, curpercent );
	ConsolePrint( text );*/

	if( steppercent <= 0 )
		steppercent = 1;
    r = (int)step->prev->color[0];
    g = (int)step->prev->color[1];
    b = (int)step->prev->color[2];
    a = (int)step->prev->color[3];

	r += (int)( ( step->color[0] - step->prev->color[0] ) / steppercent * curpercent );
	g += (int)( ( step->color[1] - step->prev->color[1] ) / steppercent * curpercent );
	b += (int)( ( step->color[2] - step->prev->color[2] ) / steppercent * curpercent );
	a += (int)( ( step->color[3] - step->prev->color[3] ) / steppercent * curpercent );
}
