/*
   lswad: a simple wad entry lister
   Copyright (c) 2006 Jon Dowland <jon@alcopop.org>

   This program can be distributed under the terms of the
   GNU GPL, version 2. On debian systems, see
              /usr/share/common-licenses/GPL-2
   or see COPYING from where you get this file
*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "wad.h"

int main(int argc, char **argv) {
	FILE *wad;
	struct wad_header header;
	uchar hbuf[12];
	struct wad_dir_entry *current, *waddir;
	unsigned long wasted;

	char print_header = 1;
	char calc_wasted = 1;
	int aoffset; /* offset into args for the file */
	
	if(argc < 2) {
		fprintf(stderr, "usage: %s [flags] wad\n"
				"flags:\n"
				"\t-w\tdo not calculate wasted space\n"
				"\t-h\tsuppress table header\n"
				,argv[0]);
		return 1;
	}
	for(aoffset = 1; aoffset < argc-1; ++aoffset) {
		char *c = argv[aoffset]; 
		/* assumption: strlen(*c) >= 1 */
		if('-' != *c) break;
		/* assumption: each arg is null-terminated */
		for(c = argv[aoffset]+1; *c != '\0'; ++c) {
			switch(*c) {
				case 'h': print_header = 0; break;
				case 'w': calc_wasted = 0; break;
				default:
					fprintf(stderr, "unknown flag '%c'\n",*c);
					return 1;
			}
		}
	}

	if(NULL == (wad = fopen(argv[aoffset],"rb"))) {
		fprintf(stderr,"cannot open %s: %s\n",argv[aoffset],strerror(errno));
		return 1;
	}
	if(1 != fread(hbuf,12,1,wad)) {
		fprintf(stderr,"couldn't read WAD header from %s\n",argv[aoffset]);
		return 1;
	}
	if(0!=strncmp("WAD",(const char *)(hbuf+1),3)) {
		fprintf(stderr,"%s is not a WAD file.\n",argv[aoffset]);
		return 1;
	}
	wad_header_init(&header,hbuf); 
	if(header.type != 'I' && header.type != 'P') {
		fprintf(stderr,"%s is not a WAD file.\n",argv[aoffset]);
		return 1;
	}
	if(print_header)
		printf("%cWAD containing %lu lumps\n"
			"directory at offset %lu (size %lu bytes)\n",
			hbuf[0],header.num,header.dir,16*header.num);

	if(0 != fseek(wad,header.dir,SEEK_SET)) {
		fprintf(stderr, "cannot seek to %lu: %s\n",
				header.dir, strerror(errno));
		return 1;
	}

#	define WADDIRSIZE (unsigned long)sizeof(struct wad_dir_entry)*header.num
	waddir = (struct wad_dir_entry *)malloc(WADDIRSIZE);
	if(NULL == waddir) {
		fprintf(stderr, "error: could not allocate %lu bytes\n", (WADDIRSIZE));
		return 1;
	}
#	undef WADDIRSIZE

	if(print_header) printf("    name\t    size\t   index\n");
	for(current = waddir; current < waddir+header.num; ++current) {
		uchar dbuf[16];

		if(1 != fread(dbuf,16,1,wad)) {
			fprintf(stderr, "cannot read directory: %s\n",
					strerror(errno));
			return 1;
		}

		wad_dir_init(current,dbuf);
		printf("%8s\t%8lu\t%8lu\n", current->name,current->size,current->index);
	}
	fclose(wad);

	if(calc_wasted) {
		/* report wasted space */
		wasted = wad_dir_wasted(waddir, header.num);
		if(wasted > 0)
			printf("%s: %lu wasted byte%s\n",
				argv[aoffset],wasted,wasted==1?"":"s");
	}
}
