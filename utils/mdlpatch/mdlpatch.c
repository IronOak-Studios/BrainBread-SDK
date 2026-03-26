/***
*
*	mdlpatch - GoldSrc .mdl root-bone origin patcher
*
*	Applies a per-sequence forward (X) offset to the root bone of a
*	Half-Life studio model.  This bakes the offset into the model data
*	so both server and client see the corrected origin (fixing the
*	hitbox / visual mismatch that occurs with client-only fixes).
*
*	Usage:
*	  mdlpatch <input.mdl> <output.mdl> <default_offset> [seq:offset ...]
*
*	Examples:
*	  mdlpatch fred.mdl   fred.mdl   -20.0  2:-25.0  26:-10.0
*	  mdlpatch zombie.mdl zombie.mdl -15.0  26:0.0
*
*	The default_offset is applied by shifting the root bone's rest
*	position (mstudiobone_t::value[0]).  Per-sequence overrides that
*	differ from the default are handled by adjusting every delta value
*	in the RLE-compressed animation stream for that sequence's root
*	bone X channel.  If a sequence has no animation data for that
*	channel, a minimal RLE entry is injected.
*
***/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Inline subset of engine/studio.h so this tool builds standalone. */

typedef float		vec3_t[3];
typedef unsigned char	byte;

#define IDSTUDIOHEADER	(('T'<<24)+('S'<<16)+('D'<<8)+'I')
#define STUDIO_VERSION	10

#define MAXSEQOVERRIDES 256

typedef struct
{
	int		id;
	int		version;

	char		name[64];
	int		length;

	vec3_t		eyeposition;
	vec3_t		min;
	vec3_t		max;

	vec3_t		bbmin;
	vec3_t		bbmax;

	int		flags;

	int		numbones;
	int		boneindex;

	int		numbonecontrollers;
	int		bonecontrollerindex;

	int		numhitboxes;
	int		hitboxindex;

	int		numseq;
	int		seqindex;

	int		numseqgroups;
	int		seqgroupindex;

	int		numtextures;
	int		textureindex;
	int		texturedataindex;

	int		numskinref;
	int		numskinfamilies;
	int		skinindex;

	int		numbodyparts;
	int		bodypartindex;

	int		numattachments;
	int		attachmentindex;

	int		soundtable;
	int		soundindex;
	int		soundgroups;
	int		soundgroupindex;

	int		numtransitions;
	int		transitionindex;
} studiohdr_t;

typedef struct
{
	char		name[32];
	int		parent;
	int		flags;
	int		bonecontroller[6];
	float		value[6];
	float		scale[6];
} mstudiobone_t;

typedef struct
{
	char		label[32];

	float		fps;
	int		flags;

	int		activity;
	int		actweight;

	int		numevents;
	int		eventindex;

	int		numframes;

	int		numpivots;
	int		pivotindex;

	int		motiontype;
	int		motionbone;
	vec3_t		linearmovement;
	int		automoveposindex;
	int		automoveangleindex;

	vec3_t		bbmin;
	vec3_t		bbmax;

	int		numblends;
	int		animindex;

	int		blendtype[2];
	float		blendstart[2];
	float		blendend[2];
	int		blendparent;

	int		seqgroup;

	int		entrynode;
	int		exitnode;
	int		nodeflags;

	int		nextseq;
} mstudioseqdesc_t;

typedef struct
{
	unsigned short	offset[6];
} mstudioanim_t;

typedef union
{
	struct {
		byte	valid;
		byte	total;
	} num;
	short		value;
} mstudioanimvalue_t;


/* ------------------------------------------------------------------ */

typedef struct
{
	int	seq;
	float	offset;
} seqoverride_t;


static void usage( const char *prog )
{
	fprintf( stderr,
		"Usage: %s <input.mdl> <output.mdl> <default_fwd_offset> [seq:offset ...]\n"
		"\n"
		"Patches the root bone X position in a GoldSrc .mdl file.\n"
		"\n"
		"  default_fwd_offset   Applied to the root bone rest position (all seqs).\n"
		"  seq:offset           Per-sequence override.  The tool computes the\n"
		"                       correction delta relative to the default and\n"
		"                       patches the animation data for that sequence.\n"
		"\n"
		"Examples:\n"
		"  %s fred.mdl   fred.mdl   -20.0  2:-25.0  26:-10.0\n"
		"  %s zombie.mdl zombie.mdl -15.0  26:0.0\n",
		prog, prog, prog );
}


static byte *read_file( const char *path, int *out_size )
{
	FILE *f;
	byte *buf;
	long sz;

	f = fopen( path, "rb" );
	if ( !f )
	{
		fprintf( stderr, "error: cannot open '%s' for reading\n", path );
		return NULL;
	}

	fseek( f, 0, SEEK_END );
	sz = ftell( f );
	fseek( f, 0, SEEK_SET );

	buf = (byte *)malloc( sz );
	if ( !buf )
	{
		fprintf( stderr, "error: out of memory (%ld bytes)\n", sz );
		fclose( f );
		return NULL;
	}

	if ( (long)fread( buf, 1, sz, f ) != sz )
	{
		fprintf( stderr, "error: short read on '%s'\n", path );
		free( buf );
		fclose( f );
		return NULL;
	}

	fclose( f );
	*out_size = (int)sz;
	return buf;
}


static int write_file( const char *path, const byte *buf, int size )
{
	FILE *f;

	f = fopen( path, "wb" );
	if ( !f )
	{
		fprintf( stderr, "error: cannot open '%s' for writing\n", path );
		return 0;
	}

	if ( (int)fwrite( buf, 1, size, f ) != size )
	{
		fprintf( stderr, "error: short write on '%s'\n", path );
		fclose( f );
		return 0;
	}

	fclose( f );
	return 1;
}


/* Returns the index of the first bone with parent == -1, or -1. */
static int find_root_bone( const byte *data, const studiohdr_t *hdr )
{
	const mstudiobone_t *bones;
	int i;

	bones = (const mstudiobone_t *)( data + hdr->boneindex );
	for ( i = 0; i < hdr->numbones; i++ )
	{
		if ( bones[i].parent == -1 )
			return i;
	}
	return -1;
}


/*
 * Add delta_short to every value in the RLE stream for one channel
 * of one bone in one sequence blend.  Returns 0 on success, 1 if
 * any value was clamped, or -1 if offset[channel] == 0 (no data).
 */
static int patch_rle_values( mstudioanim_t *panim, int channel, int numframes, int delta_short )
{
	mstudioanimvalue_t *panimvalue;
	int frame;
	int overflow = 0;

	if ( panim->offset[channel] == 0 )
		return -1;

	panimvalue = (mstudioanimvalue_t *)( (byte *)panim + panim->offset[channel] );

	frame = 0;
	while ( frame < numframes )
	{
		int valid, total, k;

		if ( panimvalue->num.total == 0 )
			break;

		valid = panimvalue->num.valid;
		total = panimvalue->num.total;

		for ( k = 0; k < valid; k++ )
		{
			int newval = panimvalue[1 + k].value + delta_short;
			if ( newval > 32767 || newval < -32768 )
			{
				fprintf( stderr, "  warning: value overflow at frame %d "
					"(value %d + delta %d = %d)\n",
					frame + k, panimvalue[1 + k].value,
					delta_short, newval );
				overflow = 1;
				if ( newval > 32767 ) newval = 32767;
				if ( newval < -32768 ) newval = -32768;
			}
			panimvalue[1 + k].value = (short)newval;
		}

		frame += total;
		panimvalue += 1 + valid;
	}

	return overflow ? 1 : 0;
}


/* Describes one pending RLE injection for a channel with no anim data. */
typedef struct
{
	int	insert_at;	/* byte offset in file where to insert */
	int	anim_off;	/* byte offset of the mstudioanim_t to patch */
	short	delta_val;	/* the value to store */
	int	numframes;	/* frames in the sequence */
} rle_injection_t;


static int cmp_injection( const void *a, const void *b )
{
	const rle_injection_t *ia = (const rle_injection_t *)a;
	const rle_injection_t *ib = (const rle_injection_t *)b;
	if ( ia->insert_at < ib->insert_at ) return -1;
	if ( ia->insert_at > ib->insert_at ) return 1;
	return 0;
}


/*
 * Inject minimal RLE entries (4 bytes each: 1 header + 1 value) into the
 * file for sequences whose root bone X channel had no animation data.
 *
 * Each injection is spliced at insert_at.  All absolute byte-offsets in
 * the header, sequence descriptors, bodyparts, models, and meshes are
 * fixed up.  The relative offsets in mstudioanim_t are recomputed by
 * mapping old file positions to new ones through the injection points.
 *
 * Returns the (possibly reallocated) buffer and updates *filesize.
 */
static byte *inject_rle_entries(
	byte *data, int *filesize,
	rle_injection_t *injections, int num_injections )
{
	int i, j;
	int old_size, new_size;
	byte *out;
	studiohdr_t *hdr;

	if ( num_injections == 0 )
		return data;

	qsort( injections, num_injections, sizeof( rle_injection_t ), cmp_injection );

	old_size = *filesize;
	new_size = old_size + num_injections * 4;

	out = (byte *)malloc( new_size );
	if ( !out )
	{
		fprintf( stderr, "error: out of memory for injection buffer\n" );
		return data;
	}

	/* Copy data with injections spliced in */
	{
		int src_pos = 0, dst_pos = 0;
		for ( i = 0; i < num_injections; i++ )
		{
			int copy_len = injections[i].insert_at - src_pos;
			if ( copy_len > 0 )
			{
				memcpy( out + dst_pos, data + src_pos, copy_len );
				dst_pos += copy_len;
				src_pos += copy_len;
			}

			{
				mstudioanimvalue_t entry[2];
				entry[0].num.valid = 1;
				entry[0].num.total = (byte)( injections[i].numframes <= 255
					? injections[i].numframes : 255 );
				entry[1].value = injections[i].delta_val;
				memcpy( out + dst_pos, entry, 4 );
				dst_pos += 4;
			}
		}
		if ( src_pos < old_size )
			memcpy( out + dst_pos, data + src_pos, old_size - src_pos );
	}

	/*
	 * Fix up absolute byte-offsets: every offset >= an injection point
	 * needs to increase by 4 for each injection at or before it.
	 */
	hdr = (studiohdr_t *)out;

	#define FIX_OFFSET( field ) \
		do { \
			int _shift = 0; \
			for ( j = 0; j < num_injections; j++ ) { \
				if ( (field) >= injections[j].insert_at ) \
					_shift += 4; \
				else break; \
			} \
			(field) += _shift; \
		} while (0)

	/* Header offsets */
	FIX_OFFSET( hdr->boneindex );
	FIX_OFFSET( hdr->bonecontrollerindex );
	FIX_OFFSET( hdr->hitboxindex );
	FIX_OFFSET( hdr->seqindex );
	FIX_OFFSET( hdr->seqgroupindex );
	FIX_OFFSET( hdr->textureindex );
	FIX_OFFSET( hdr->texturedataindex );
	FIX_OFFSET( hdr->skinindex );
	FIX_OFFSET( hdr->bodypartindex );
	FIX_OFFSET( hdr->attachmentindex );
	FIX_OFFSET( hdr->soundindex );
	FIX_OFFSET( hdr->soundgroupindex );
	FIX_OFFSET( hdr->transitionindex );
	hdr->length = new_size;

	/* Sequence descriptor offsets */
	{
		mstudioseqdesc_t *pseq = (mstudioseqdesc_t *)( out + hdr->seqindex );
		for ( i = 0; i < hdr->numseq; i++ )
		{
			FIX_OFFSET( pseq[i].animindex );
			FIX_OFFSET( pseq[i].eventindex );
			FIX_OFFSET( pseq[i].pivotindex );
			FIX_OFFSET( pseq[i].automoveposindex );
			FIX_OFFSET( pseq[i].automoveangleindex );
		}
	}

	/* Bodypart / model / mesh offsets */
	{
		typedef struct {
			char name[64]; int nummodels; int base; int modelindex;
		} bodyparts_t;
		typedef struct {
			char name[64]; int type; float boundingradius;
			int nummesh, meshindex, numverts, vertinfoindex, vertindex;
			int numnorms, norminfoindex, normindex, numgroups, groupindex;
		} model_t;
		typedef struct {
			int numtris, triindex, skinref, numnorms, normindex;
		} mesh_t;

		bodyparts_t *bp = (bodyparts_t *)( out + hdr->bodypartindex );
		for ( i = 0; i < hdr->numbodyparts; i++ )
		{
			int m;
			model_t *pmodel;

			FIX_OFFSET( bp[i].modelindex );
			pmodel = (model_t *)( out + bp[i].modelindex );

			for ( m = 0; m < bp[i].nummodels; m++ )
			{
				int mi;
				mesh_t *pmesh;

				FIX_OFFSET( pmodel[m].meshindex );
				FIX_OFFSET( pmodel[m].vertinfoindex );
				FIX_OFFSET( pmodel[m].vertindex );
				FIX_OFFSET( pmodel[m].norminfoindex );
				FIX_OFFSET( pmodel[m].normindex );

				pmesh = (mesh_t *)( out + pmodel[m].meshindex );
				for ( mi = 0; mi < pmodel[m].nummesh; mi++ )
					FIX_OFFSET( pmesh[mi].triindex );
			}
		}
	}

	/*
	 * Set offset[0] for each injected anim_t and fix its other channel
	 * offsets (which are relative and may have shifted).
	 */
	for ( i = 0; i < num_injections; i++ )
	{
		mstudioanim_t *panim;
		int anim_new_pos, data_new_pos, rel;
		int ch;

		/* New position of the anim_t struct */
		anim_new_pos = injections[i].anim_off;
		for ( j = 0; j < num_injections; j++ )
		{
			if ( injections[i].anim_off >= injections[j].insert_at )
				anim_new_pos += 4;
		}

		/* New position of the injected RLE data */
		data_new_pos = injections[i].insert_at;
		for ( j = 0; j < i; j++ )
			data_new_pos += 4;

		rel = data_new_pos - anim_new_pos;
		if ( rel < 0 || rel > 65535 )
		{
			fprintf( stderr, "error: injection offset %d out of unsigned short range\n", rel );
			free( out );
			return data;
		}

		panim = (mstudioanim_t *)( out + anim_new_pos );
		panim->offset[0] = (unsigned short)rel;

		/* Fix other channels' relative offsets */
		for ( ch = 1; ch < 6; ch++ )
		{
			if ( panim->offset[ch] != 0 )
			{
				int target_old = injections[i].anim_off + panim->offset[ch];
				int target_new = target_old;
				for ( j = 0; j < num_injections; j++ )
				{
					if ( target_old >= injections[j].insert_at )
						target_new += 4;
				}
				panim->offset[ch] = (unsigned short)( target_new - anim_new_pos );
			}
		}
	}

	/*
	 * Fix relative offsets in all other anim_t entries whose data
	 * targets crossed an injection boundary.
	 */
	{
		mstudioseqdesc_t *pseq = (mstudioseqdesc_t *)( out + hdr->seqindex );
		for ( i = 0; i < hdr->numseq; i++ )
		{
			int b, q;
			if ( pseq[i].seqgroup != 0 )
				continue;

			for ( q = 0; q < pseq[i].numblends; q++ )
			{
				for ( b = 0; b < hdr->numbones; b++ )
				{
					mstudioanim_t *pa = (mstudioanim_t *)(
						out + pseq[i].animindex )
						+ q * hdr->numbones + b;
					int pa_off_in_new = (int)( (byte *)pa - out );
					int pa_off_old, ch, skip;

					/* Skip entries already handled in the injection loop */
					skip = 0;
					for ( j = 0; j < num_injections && !skip; j++ )
					{
						int inj_anim_new = injections[j].anim_off;
						int jj;
						for ( jj = 0; jj < num_injections; jj++ )
						{
							if ( injections[j].anim_off >= injections[jj].insert_at )
								inj_anim_new += 4;
						}
						if ( pa_off_in_new == inj_anim_new )
							skip = 1;
					}
					if ( skip )
						continue;

					/* Reverse-map to get original file offset of this anim_t */
					pa_off_old = pa_off_in_new;
					for ( j = 0; j < num_injections; j++ )
					{
						int ins_new = injections[j].insert_at;
						int jj;
						for ( jj = 0; jj < j; jj++ )
							ins_new += 4;
						if ( pa_off_in_new >= ins_new )
							pa_off_old -= 4;
					}

					for ( ch = 0; ch < 6; ch++ )
					{
						int target_old, target_new, rel2;
						if ( pa->offset[ch] == 0 )
							continue;

						target_old = pa_off_old + pa->offset[ch];
						target_new = target_old;
						for ( j = 0; j < num_injections; j++ )
						{
							if ( target_old >= injections[j].insert_at )
								target_new += 4;
						}
						rel2 = target_new - pa_off_in_new;
						if ( rel2 < 0 || rel2 > 65535 )
						{
							fprintf( stderr, "error: fixup offset %d out of range "
								"for bone %d ch %d seq %d\n",
								rel2, b, ch, i );
						}
						else
						{
							pa->offset[ch] = (unsigned short)rel2;
						}
					}
				}
			}
		}
	}

	free( data );
	*filesize = new_size;
	return out;

	#undef FIX_OFFSET
}


/* ------------------------------------------------------------------ */

int main( int argc, char *argv[] )
{
	byte *data;
	int filesize;
	studiohdr_t *hdr;
	mstudiobone_t *bones;
	mstudioseqdesc_t *seqs;
	int root_bone;
	float default_offset;
	seqoverride_t overrides[MAXSEQOVERRIDES];
	int num_overrides = 0;
	rle_injection_t injections[MAXSEQOVERRIDES];
	int num_injections = 0;
	int i, q;

	if ( argc < 4 )
	{
		usage( argv[0] );
		return 1;
	}

	default_offset = (float)atof( argv[3] );

	/* Parse per-sequence overrides */
	for ( i = 4; i < argc; i++ )
	{
		char *colon = strchr( argv[i], ':' );
		if ( !colon )
		{
			fprintf( stderr, "error: bad override format '%s' (expected seq:offset)\n", argv[i] );
			return 1;
		}
		if ( num_overrides >= MAXSEQOVERRIDES )
		{
			fprintf( stderr, "error: too many overrides (max %d)\n", MAXSEQOVERRIDES );
			return 1;
		}
		overrides[num_overrides].seq = atoi( argv[i] );
		overrides[num_overrides].offset = (float)atof( colon + 1 );
		num_overrides++;
	}

	/* Read and validate */
	data = read_file( argv[1], &filesize );
	if ( !data )
		return 1;

	if ( filesize < (int)sizeof( studiohdr_t ) )
	{
		fprintf( stderr, "error: file too small to be a valid .mdl\n" );
		free( data );
		return 1;
	}

	hdr = (studiohdr_t *)data;
	if ( hdr->id != IDSTUDIOHEADER )
	{
		fprintf( stderr, "error: invalid .mdl magic (expected IDST, got 0x%08X)\n", hdr->id );
		free( data );
		return 1;
	}
	if ( hdr->version != STUDIO_VERSION )
	{
		fprintf( stderr, "error: unsupported .mdl version %d (expected %d)\n", hdr->version, STUDIO_VERSION );
		free( data );
		return 1;
	}

	printf( "Model: %s\n", hdr->name );
	printf( "Bones: %d, Sequences: %d\n", hdr->numbones, hdr->numseq );

	root_bone = find_root_bone( data, hdr );
	if ( root_bone < 0 )
	{
		fprintf( stderr, "error: no root bone found (no bone with parent == -1)\n" );
		free( data );
		return 1;
	}

	bones = (mstudiobone_t *)( data + hdr->boneindex );
	printf( "Root bone: %d (\"%s\"), rest X = %.4f, scale X = %.6f\n",
		root_bone, bones[root_bone].name,
		bones[root_bone].value[0], bones[root_bone].scale[0] );

	/* Apply default offset to bone rest position */
	printf( "\nApplying default offset %.2f to root bone rest X position...\n", default_offset );
	bones[root_bone].value[0] += default_offset;
	printf( "  New rest X = %.4f\n", bones[root_bone].value[0] );

	/* Process per-sequence overrides */
	seqs = (mstudioseqdesc_t *)( data + hdr->seqindex );

	for ( i = 0; i < num_overrides; i++ )
	{
		int seq = overrides[i].seq;
		float correction, scale;
		int delta_short;

		if ( seq < 0 || seq >= hdr->numseq )
		{
			fprintf( stderr, "warning: sequence %d out of range (0..%d), skipping\n",
				seq, hdr->numseq - 1 );
			continue;
		}

		correction = overrides[i].offset - default_offset;
		if ( correction == 0.0f )
		{
			printf( "  Seq %d (\"%s\"): offset %.2f same as default, no correction needed.\n",
				seq, seqs[seq].label, overrides[i].offset );
			continue;
		}

		scale = bones[root_bone].scale[0];
		if ( scale == 0.0f )
		{
			fprintf( stderr, "error: root bone X scale is 0, cannot encode delta for seq %d\n", seq );
			continue;
		}

		delta_short = (int)( correction / scale + ( correction > 0 ? 0.5f : -0.5f ) );

		printf( "  Seq %d (\"%s\"): target %.2f, correction %.2f, delta_short %d (scale %.6f)\n",
			seq, seqs[seq].label, overrides[i].offset, correction,
			delta_short, scale );

		if ( seqs[seq].seqgroup != 0 )
		{
			fprintf( stderr, "  warning: sequence %d is in seqgroup %d (demand-loaded), "
				"only seqgroup 0 is supported.  Skipping.\n",
				seq, seqs[seq].seqgroup );
			continue;
		}

		for ( q = 0; q < seqs[seq].numblends; q++ )
		{
			mstudioanim_t *panim;

			panim = (mstudioanim_t *)( data + seqs[seq].animindex )
				+ q * hdr->numbones + root_bone;

			if ( panim->offset[0] != 0 )
			{
				int ret = patch_rle_values( panim, 0, seqs[seq].numframes, delta_short );
				if ( ret == 0 )
					printf( "    Blend %d: patched %d frames in-place.\n",
						q, seqs[seq].numframes );
				else if ( ret > 0 )
					printf( "    Blend %d: patched with overflow warnings.\n", q );
			}
			else
			{
				/* No animation data -- queue an RLE injection */
				int anim_file_off = (int)( (byte *)panim - data );
				int insert_at = seqs[seq].animindex
					+ seqs[seq].numblends * hdr->numbones * (int)sizeof( mstudioanim_t );

				printf( "    Blend %d: no anim data, queuing RLE injection at file offset %d.\n",
					q, insert_at );

				injections[num_injections].insert_at = insert_at;
				injections[num_injections].anim_off = anim_file_off;
				injections[num_injections].delta_val = (short)delta_short;
				injections[num_injections].numframes = seqs[seq].numframes;
				num_injections++;
			}
		}
	}

	/* Apply injections if any */
	if ( num_injections > 0 )
	{
		printf( "\nInjecting %d RLE entries (%d bytes)...\n",
			num_injections, num_injections * 4 );
		data = inject_rle_entries( data, &filesize, injections, num_injections );
		if ( !data )
		{
			fprintf( stderr, "error: injection failed\n" );
			return 1;
		}
		hdr = (studiohdr_t *)data;
	}

	/* Write output */
	printf( "\nWriting %s (%d bytes)...\n", argv[2], filesize );
	if ( !write_file( argv[2], data, filesize ) )
	{
		free( data );
		return 1;
	}

	printf( "Done.\n" );
	free( data );
	return 0;
}
