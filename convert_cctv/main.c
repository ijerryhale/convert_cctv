//
//  convert.c
//  
//
//  Created by Jerry Hale on 5/14/18.

//	convert ".264" SD video files
//	from FDT 8903 PTZ camera
//	into ".mp4" and ".wav" files
//	http://fdt.us

//	I think this will work on a variety of
//	Chinese made survailance cameras

//	based original work here
//	if this works for you consider
//	hittin' this guys PayPal account

//	https://www.spitzner.org/kkmoon.html

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include <dirent.h>

#include "main.h"

//	find all regular files
//	that have a ".264"
//	file extension
int parse_ext(const struct dirent *dir)
{
	if (!dir) return (0);

	if (dir->d_type == DT_REG)
	{
		const char	*ext = strrchr(dir->d_name,'.');

		if ((!ext) || (ext == dir->d_name))
			return (0);
		else if(strcmp(ext, ".264") == 0)
			return (1);
	 }
	return (0);
}

//	find all of the files in the
//	directory argument that have
//	a file extension of ".264"
//	and convert them to an ".mp4"
//	file and a ".wav" file
int main(int ac, char **av)
{
	if (ac != 2)
        exit(printf("use %s <directory> \n",av[0]));

	struct	dirent **namelist;
	int		n = scandir(av[1], &namelist, parse_ext, alphasort);

	if (n < 0)
	{
		perror("scandir");
		return (1);
	}
	else
	{
		while (n--)
		{
			#if 0
               printf("%s\n", namelist[n]->d_name);
			#else

				char	*buffer,
						*p,
						ccode[5];
				int		count,
						bufsiz,
						abytes;

				char	path[1024] = { 0 };

				strcpy(path, av[1]);
				strcat(path, "/");
				strcat(path, namelist[n]->d_name);

				FILE	*in = fopen(path, "rb");
			
				if (!in) exit(printf("cannot open %s for reading.\n",namelist[n]->d_name));

				/* try to allocate a buffer size of infile */
				fseek(in,0,SEEK_END);
				bufsiz = ftell(in);
				rewind(in);
				if(!(buffer = malloc(bufsiz)))
					exit(printf("unable to allocate %d bytes\n",bufsiz));
				p = buffer;
				/* open output */

				memset(path, 0x00, sizeof(path));
				strcpy(path, av[1]);
				strcat(path, "/");
				strcat(path, namelist[n]->d_name);
				strcat(path, ".mp4");

				FILE	*out = fopen(path,"wb");
			
				if(!out)
					exit(printf("cannot open %s for writing.\n", path));

				memset(path, 0x00, sizeof(path));
				strcpy(path, av[1]);
				strcat(path, "/");
				strcat(path, namelist[n]->d_name);
				strcat(path, ".wav");

				#define CREATE_WAV_FILE
			
				#ifdef CREATE_WAV_FILE
					FILE	*outa = fopen(path, "wb");
			
					if (!outa) exit(printf("cannot open %s for writing.\n", path));

					wav_header	ahead;
					fwrite(&ahead,sizeof(wav_header),1,outa);
				#endif
			
				/* get data */
				count = fread(buffer,1,bufsiz,in);
				abytes = 0;
				if(count != bufsiz)
				{
					printf("could only read %d of %d bytes\n",count,bufsiz);
					exit(0);
				}
				/* Throw first 0x10 bytes of garbage/fileheader plus first videoheader */
				p += 0x10;

				while(p-buffer < bufsiz)
				{
					int	len = 0;
					
					bzero(ccode,5);
					bcopy(p,ccode,4);
					p += 4;
					bcopy(p,&len,4);
					if(!(strncmp((char *)ccode,"HXAF",4)))
					{
						p += 0xc;
						p += 4; // {0x0001, 0x5000} whatever that means, it must go, it's no audio
						len -= 4;
						printf("code: HXAF audio  %d bytes\n",len);
						
						#ifdef CREATE_WAV_FILE
							fwrite(p,1,len,outa);
						#endif

						p += len;
						abytes += len;
						continue;
					}
					else if(!(strncmp((char *)ccode,"HXFI",4)))
					{
						printf("found code HXFI, exit\n");
						break; /* some sort of table follows */
					}
					/* this will break if there's some other ccode ! */
					printf("code: %s video %d bytes\n",ccode,len);
					p += 0xc;
					fwrite(p,1,len,out);
					p += len;
				}

				#ifdef CREATE_WAV_FILE
					/* wav header */
					strncpy(ahead.riff_header,"RIFF",4);
					strncpy(ahead.wave_header,"WAVE",4);
					strncpy(ahead.fmt_header,"fmt ",4);
					strncpy(ahead.data_header,"data",4);

					ahead.fmt_chunk_size = 16;
					ahead.audio_format = 0x6;
					ahead.num_channels = 1;
					ahead.sample_rate = 8000;
					ahead.byte_rate = 8000; // 16 ??
					ahead.sample_alignment = 2;
					ahead.bit_depth = 16;
					ahead.data_bytes = abytes;
					ahead.wav_size = abytes + sizeof(wav_header) - 8;
					fseek(outa,0,SEEK_SET);
					fwrite(&ahead,sizeof(wav_header),1,outa);
			
					fclose(outa);
				#endif
				free(buffer);
				fclose(in);
				fclose(out);
			#endif
	
			free(namelist[n]);
		}

		free(namelist);
	}

    exit(0);

}
