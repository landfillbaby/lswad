/*
 * wad.c, wad.h
 * simple routines for manipulating WAD files
 * Copyright (c) 2006 Jon Dowland <jon@alcopop.org>
 * Licensed under the terms of the GNU GPL, version 2
 * see /usr/share/common-licenses/GPL-2 on debian systems
 * or COPYING from the same place you got this file
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>  /* INT_M{AX,IN} */
#include <string.h>
#include <ctype.h>   /* isprint */

typedef unsigned char uchar;

unsigned long fetchint(uchar *p) {
    return (unsigned long)p[0]
        |  (unsigned long)p[1]<<8 
        |  (unsigned long)p[2]<<16 
        |  (unsigned long)p[3]<<24; 
}

struct wad_header {
        char type;
        unsigned long num; /* number of lumps */
        unsigned long dir; /* offset to dir */
};

void wad_header_init(struct wad_header * h, uchar *lump) {
        h->type = lump[0];
	h->num  = fetchint(lump+4);
	h->dir  = fetchint(lump+8);
}

struct wad_dir_entry {
	unsigned long index;
	unsigned long size;
	uchar name[9];
};

void wad_dir_init(struct wad_dir_entry * w, uchar * lump) {
	uchar c;
	w->index = fetchint(lump);
	w->size  = fetchint(lump+4);

	/* sanitize the name */
	strcpy((void *)w->name, (void *)"????????");
	for(c = 8; c < 16; ++c) {
		if(isprint(lump[c])) w->name[c-8] = lump[c];
		else if('\0' == lump[c]) {
			w->name[c-8] = '\0';
			break;
		}
	}
}

/* wad_dir_cmp: comparison function for struct wad_dir_entry */
int wad_dir_cmp(const void *a, const void *b) {
	signed long i;
	struct wad_dir_entry *x = (struct wad_dir_entry *)a;
	struct wad_dir_entry *y = (struct wad_dir_entry *)b;

	i = x->index - y->index;
	if (i > INT_MAX) i = INT_MAX;
	if (i < INT_MIN) i = INT_MIN;

	return (int)i;
}

unsigned long wad_dir_wasted(struct wad_dir_entry * w, unsigned int s) {
	struct wad_dir_entry *current;
	unsigned long wasted = 0, lend = 12;

	/* sort the dictionary in order to count wasted space */
	qsort(w, s, sizeof(struct wad_dir_entry), wad_dir_cmp); 
	for(current = w; current < w+s; ++current) {
		unsigned long wtmp;
		if(current->size > 0) {
			wtmp = current->index - lend;
			if(wtmp > 0)
				printf("%lu bytes wasted at offset %lu\n",
				wtmp,current->index);
			wasted += wtmp;
			lend =    current->index + current->size;
		}
	}
        return wasted;
}
