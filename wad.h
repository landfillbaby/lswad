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
#include <string.h>
#include <errno.h>
#include <ctype.h>   /* isprint */
#include <limits.h>  /* INT_M{AX,IN} */

typedef unsigned char uchar;
unsigned long fetchint(uchar *p);

struct wad_header {
        char type;
        unsigned long num; /* number of lumps */
        unsigned long dir; /* offset to dir */
};

void wad_header_init(struct wad_header * h, uchar *lump);

struct wad_dir_entry {
	unsigned long index;
	unsigned long size;
	uchar name[9];
};

void wad_dir_init(struct wad_dir_entry * w, uchar * lump);

/* wad_dir_cmp: comparison function for struct wad_dir_entry */
int wad_dir_cmp(const void *a, const void *b);

unsigned long wad_dir_wasted(struct wad_dir_entry * w, unsigned int s);
