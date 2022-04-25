#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "smacker.h"

#define w(p,n) fwrite(p,1,n,fp)

#define LIST w("LIST",4);

#define lu(p) \
{ \
	b[0] = (p & 0x000000FF); \
	b[1] = ((p & 0x0000FF00) >> 8); \
	b[2] = ((p & 0x00FF0000) >> 16); \
	b[3] = ((p & 0xFF000000) >> 24); \
	w(b,4); \
}

#define su(p) \
{ \
	b[0] = (p & 0x00FF); \
	b[1] = ((p & 0xFF00) >> 8); \
	w(b,2); \
}

void process(const char *fn)
{
	FILE *fp = NULL;
	smk s;
	char outfile[256];
	unsigned char b[5];

	unsigned int		i,k;
	int j;
	unsigned long temp_u;

	/* all and video info */
	unsigned long	w, h, f;
	double usf;
	unsigned long total_frame_size;

	/* audio info */
	unsigned char	a_t, a_c[7], a_d[7];
	unsigned long	a_r[7];

	unsigned char num_tracks = 0;
	unsigned char **audio_data[7];
	unsigned long *audio_size[7];
	unsigned long total_audio_size[7] = {0};
	unsigned long total_total_audio_size = 0;

	const unsigned char *pal,*frame;

	unsigned long cur_frame;

	printf("--------\nsmk2avi processing %s...\n",fn);

	/* open the smk file */
	s = smk_open_file(fn,SMK_MODE_DISK);
	if (s == NULL) goto error;

	/* get some info about the file */
	smk_info_all(s, NULL, &f, &usf);
	smk_info_video(s, &w, &h, NULL);
	smk_info_audio(s, &a_t, a_c, a_d, a_r);

	printf("\t\t\twidth: %lu, height: %lu, usec/frame: %lf, frames: %lu\n",w,h,usf,f);

	total_frame_size = w * h * 3;

	/* make 2 passes through the file.
		first one is to pull all the audio tracks only. */
	smk_enable_all(s,a_t);
	for (i = 0; i < 7; i ++)
	{
		if (a_t & (1 << i))
		{
			audio_size[i] = malloc(f * sizeof(unsigned long));
			audio_data[i] = malloc(f * sizeof(unsigned char*));
			num_tracks ++;
		} else {
			audio_size[i] = NULL;
			audio_data[i] = NULL;
		}
	}

	printf("\tAudio processing frame: ");
	smk_first(s);
	for (cur_frame = 0; cur_frame < f; cur_frame ++)
	{
		printf("%lu... ",cur_frame);
			fflush(stdout);
		for (i = 0; i < 7; i ++)
		{
			if (audio_size[i] != NULL)
			{
				audio_size[i][cur_frame] = smk_get_audio_size(s,i);
				total_audio_size[i] += smk_get_audio_size(s,i);
				total_total_audio_size += smk_get_audio_size(s,i);
				audio_data[i][cur_frame] = malloc(audio_size[i][cur_frame]);
				memcpy(audio_data[i][cur_frame],smk_get_audio(s,i),audio_size[i][cur_frame]);
			}
		}
		smk_next(s);
	}
	printf("done!\n");

	smk_enable_all(s,SMK_VIDEO_TRACK);

	sprintf(outfile,"%s.avi",fn);

	fp = fopen(outfile,"wb");

	printf("Writing AVI file...\n");

	// riff header
	w("RIFF",4);
	temp_u = 4 + 4 + (8 + 64 + 124 + (num_tracks * 102)) + 
			8 + (4 + ((total_frame_size + 8) * f) + ( (num_tracks * 8) + total_total_audio_size));
	lu(temp_u);
	w("AVI ",4);

	{
		// avi header list
		LIST;
		temp_u = 8 + 64 + 124 + (num_tracks * 102);
		lu(temp_u);
		w("hdrl",4);

		{
			// avi header
			w("avih",4);
			lu(56);
			{
				lu( (unsigned long)usf ); // microsec per frame
				temp_u = total_frame_size + total_total_audio_size;
				lu( temp_u ); // max bytes per sec
				lu( 1 ); // padding granularity
				lu( 0 ); // flags
				lu( f ); // total frames
				lu( 0 ); // initial frames
				temp_u = num_tracks + 1;
				lu( temp_u ); // streams
				temp_u = total_frame_size + total_total_audio_size;
				lu( temp_u ); // suggested buf size
				lu( w ); // width
				lu( h ); // height
				lu( 0 ); // reserved (0-3)
				lu( 0 );
				lu( 0 );
				lu( 0 );
			}

			// stream list: video stream
puts("Video stream header list");
			LIST
			lu(116);
			w("strl",4);
	
			{
				w("strh",4);
				lu(56);
				{
					w("vids",4); // fourcc type
					w("DIB ",4); // fourcc handler
					lu(0); // flags
					lu(0); // priority + language
					lu(0); // init frames
					lu ((unsigned long)usf); // scale
					lu( 1000000 ); // rate
					lu(0); // start
					temp_u = (unsigned long) ((double)f * 100000.0 / usf);
				lu( temp_u ); // length (time in seconds ?)
					lu(total_frame_size); // suggested bufsize
					lu(-1); // quality
					lu(total_frame_size); // samplesize
					lu(0); // rcFrame
					su(w); su(h); // rcFrame: right, bottom
				}
	
				w("strf",4);
				lu(40);
				{
					lu(40);	// size
					lu(w);	// width
					lu(h);	// height
					su(1); // planes
					su(24); // bpp
					lu(0); // compression
					lu(total_frame_size); // total image frame size
					lu(0); // xpels/meter
					lu(0); // ypels/meter
					lu(0); // colors used
					lu(0); // colors important
				}
			}
	
			// stream list: audio stream(s)
			for (i = 0; i < 7; i++)
			{
				if (audio_size[i] != NULL)
				{
printf("-> Audio header %d, %luhz, %d bits, %d channels\n",i,a_r[i],a_d[i],a_c[i]);
					LIST
					lu(94);
					w("strl",4);
	
					w("strh",4);
					lu(56);
					{
						w("auds",4); // fourcc
						lu(0); // handler (pcm)
						lu(0); // flags
						lu(0); // priority + language
						lu(0); // initial frames
						lu( a_c[i]); // scale
					temp_u = a_r[i] * a_c[i];
					lu( temp_u ); // framerate
						lu(0); // start
					temp_u = (unsigned long) ((double)f * 100000.0 / usf);
					lu( temp_u ); // time in seconds
						temp_u = total_audio_size[i] / a_c[i]; 
					lu( temp_u ); // sugg. buf size
						lu(-1); // quality
						temp_u = total_audio_size[i];
						lu(temp_u); // sample size
						lu(0); // rect ??
						lu(0); // rect ??
					}
		
					w("strf",4);
					lu(18);
					{
						su(1);	// format
						su(a_c[i]); // channels
						lu(a_r[i]); // samples/sec
						temp_u = a_c[i] * a_r[i] * (a_d[i] / 8); // avg bytes/sec
						lu(temp_u);
						temp_u = a_c[i] * (a_d[i] / 8); // avg bytes/sec
						su(temp_u);
						su(a_d[i]);
						su(0);
					}
				}
			}
		}

		smk_first(s);
		printf("\tVideo processing frame:\n");

		// movie data
		LIST
		temp_u = 4 + ((total_frame_size + 8) * f) + ( (num_tracks * 8) + total_total_audio_size);
		lu(temp_u);
		w("movi",4);
	
		for (i = 0; i < f; i ++)
		{
			w("00db",4);
			lu(total_frame_size);

			frame = smk_get_video(s);
			pal = smk_get_palette(s);

			if (frame == NULL || pal == NULL) goto error;
	
			for (j = h - 1; j >= 0; j--)
			{
				for (k = 0; k < w; k++)
				{
					w(&pal[frame[(j * w) + k] * 3 + 2],1);
					w(&pal[frame[(j * w) + k] * 3 + 1],1);
					w(&pal[frame[(j * w) + k] * 3],1);
				}
			}
			printf("%u...",i);
			fflush(stdout);

			smk_next(s);
		}
		printf("done!\n");

		k = 0;
		for (i = 0; i < 7; i++)
		{
			if (audio_size[i] != NULL)
			{
				k ++;
				sprintf((char *)b,"%02uwb",k);
				w(b,4);
				temp_u = total_audio_size[i];
				lu(temp_u);

				for (j = 0; j < (int)f; j++)
				{
					w(audio_data[i][j],audio_size[i][j]);
				}
			}
		}
	}
	
	fclose(fp);	

	smk_close(s);

	printf("done.\n--------\n");
	return;

error:
	if (fp) fclose(fp);	

	smk_close(s);
	printf("!!HAD ERRORS!!\n--------\n");
	return;
	
}

int main (int argc, char *argv[])
{
	int i;
	for (i = 1; i < argc; i ++)
	{
		process(argv[i]);
	}
	return 0;
}
