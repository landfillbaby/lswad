/*
   wadfs: a read-only filesystem for accessing doom WAD files
   Copyright (c) 2006 Jon Dowland <jon@alcopop.org>

   based on the FUSE "hello" example by Miklos Szeredi
   Copyright (C) 2001-2005 Miklos Szeredi <miklos@szeredi.hu>

   This program can be distributed under the terms of the
   GNU GPL, version 2. On debian systems, see
	      /usr/share/common-licenses/GPL-2
   or see COPYING from where you get this file
*/

#define FUSE_USE_VERSION 22
#include "wad.h"
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct wad_dir_entry *waddir;
static struct wad_header header;
static time_t atime, ctime, mtime;
static FILE *wad;

static int wadfs_getattr(const char *path, struct stat *stbuf) {
  struct wad_dir_entry *d;

  memset(stbuf, 0, sizeof(struct stat));
  if(strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }
  for(d = waddir; d < waddir + header.num; ++d) {
    if(0 == strcmp(path + 1, d->name)) {
      stbuf->st_mode = S_IFREG | 0444;
      stbuf->st_nlink = 1;
      stbuf->st_size = d->size;
      stbuf->st_mtime = mtime;
      stbuf->st_atime = atime;
      stbuf->st_ctime = ctime;
      return 0;
    }
  }
  return -ENOENT;
}

static int wadfs_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler) {
  struct wad_dir_entry *d;

  if(strcmp(path, "/") != 0) { return -ENOENT; }

  filler(h, ".", 0, 0);
  filler(h, "..", 0, 0);
  for(d = waddir; d < waddir + header.num; ++d) { filler(h, d->name, 0, 0); }

  return 0;
}

static int wadfs_open(const char *path, struct fuse_file_info *fi) {
  struct wad_dir_entry *d;

  for(d = waddir; d < waddir + header.num; ++d) {
    if('/' == *path && strcmp(path + 1, d->name) == 0) {
      if((fi->flags & 3) != O_RDONLY) return -EACCES;
      return 0;
    }
  }
  return -ENOENT;
}

static int wadfs_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi) {
  struct wad_dir_entry *d;
  size_t len;
  (void)fi;

  if(0 == size) { return 0; }
  for(d = waddir; d < waddir + header.num; ++d) {
    if('/' == *path && strcmp(path + 1, d->name) == 0) {
      len = d->size;
      if(offset < len) {
	if(offset + size > len) { size = len - offset; }
	if(0 == fseek(wad, d->index + offset, SEEK_SET)) {
	  fread(buf, size, 1, wad);
	}
      } else {
	size = 0;
      }

      return size;
    }
  }
  return -ENOENT;
}

static struct fuse_operations wadfs_oper = {
    .getattr = wadfs_getattr,
    .getdir = wadfs_getdir,
    .open = wadfs_open,
    .read = wadfs_read,
};

int main(int argc, char *argv[]) {
  uchar hbuf[12];
  struct wad_dir_entry *current;
  char *wadfile;
  struct stat stat;
  int ret;

  if(argc < 3) {
    fprintf(stderr, "usage: %s wadfile [fuseargs] mountpoint\n", argv[0]);
    exit(1);
  }
  wadfile = argv[1];
  argv[1] = argv[0];
  argv++;
  argc--;

  /* open the wad and find the directory */
  if(NULL == (wad = fopen(wadfile, "rb"))) {
    fprintf(stderr, "cannot open %s: %s\n", wadfile, strerror(errno));
    exit(EXIT_FAILURE);
  }
  if(1 != fread(hbuf, 12, 1, wad)) {
    fprintf(stderr, "couldn't read WAD header from %s\n", wadfile);
    exit(EXIT_FAILURE);
  }
  if(0 != strncmp("WAD", (const char *)(hbuf + 1), 3)) {
    fprintf(stderr, "%s is not a WAD file.\n", wadfile);
    exit(EXIT_FAILURE);
  }
  wad_header_init(&header, hbuf);
  if('I' != header.type && 'P' != header.type) {
    fprintf(stderr, "%s is not a WAD file.\n", wadfile);
    exit(EXIT_FAILURE);
  }
  if(0 != fseek(wad, header.dir, SEEK_SET)) {
    fprintf(stderr, "cannot seek to %lu: %s\n", header.dir, strerror(errno));
    exit(EXIT_FAILURE);
  }
#define WADDIRSIZE (unsigned long)sizeof(struct wad_dir_entry) * header.num
  waddir = (struct wad_dir_entry *)malloc(WADDIRSIZE);
  if(NULL == waddir) {
    fprintf(stderr, "error: could not allocate %lu bytes\n", (WADDIRSIZE));
    return 1;
  }
#undef WADDIRSIZE

  for(current = waddir; current < waddir + header.num; ++current) {
    uchar dbuf[16];

    if(1 != fread(dbuf, 16, 1, wad)) {
      fprintf(stderr, "cannot read directory: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    wad_dir_init(current, dbuf);
  }
  /* fstat stuff */
  if(0 == fstat(fileno(wad), &stat)) {
    atime = stat.st_atime;
    mtime = stat.st_mtime;
    ctime = stat.st_ctime;
  }

  /* now for the fuse magic */
  ret = fuse_main(argc, argv, &wadfs_oper);
  fclose(wad);
  return ret;
}
