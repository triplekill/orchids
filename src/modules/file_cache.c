/**
 ** @file file_cache.c
 ** File cache
 ** 
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 ** 
 ** @version 0.1.0
 ** @ingroup output
 ** 
 ** @date  Started on: Fri Mar 12 10:48:03 2004
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
/* #include <sys/resource.h> */
#include <sys/utsname.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <utime.h>
#include <limits.h>

#include "orchids.h"

#include "file_cache.h"
#include "file_cache_priv.h"

static char *cache_gc_prefix_g = NULL;
static char *cache_gc_dir_g = NULL;


static int
cache_gc_select(
#ifndef BSD_SCANDIR
	const
#endif
	 struct dirent *d)
{
  return (!strncmp(cache_gc_prefix_g, d->d_name, strlen(cache_gc_prefix_g)));
}


static int
cache_gc_compar(
#ifndef BSD_SCANDIR
	const struct dirent **a, const struct dirent **b
#define REPA (*a)
#define REPB (*b)
#else
	const void *ap, const void *bp
#define REPA (*(const struct dirent **)ap)
#define REPB (*(const struct dirent **)ap)
#endif
	)
{
  struct stat stat_a;
  struct stat stat_b;
  char path[PATH_MAX];

  snprintf(path, sizeof (path), "%s/%s",
	   cache_gc_dir_g, REPA->d_name);
  Xstat(path, &stat_a);

  snprintf(path, sizeof (path), "%s/%s",
	   cache_gc_dir_g, REPB->d_name);
  Xstat(path, &stat_b);

  return (stat_b.st_mtime - stat_a.st_mtime);
#undef REPA
#undef REPB
}


void
cache_gc(char *dir,
	 char *prefix,
	 int file_limit,
	 size_t size_limit,
	 time_t time_limit)
{
  struct dirent **nl;
  int n;
  struct stat s;
  int i;
  time_t cur_time;
  size_t total_size;
  char buffer[8192];
 
  cur_time = time(NULL);
  total_size = 0;
  cache_gc_prefix_g = prefix;
  cache_gc_dir_g = dir;
  n = scandir(dir, &nl, cache_gc_select, cache_gc_compar);
  if (n < 0)
    perror("scandir()");
  else if (n > 0) {
    for (i = 0; i < n; i++) {
      snprintf(buffer, sizeof (buffer), "%s/%s", dir, nl[i]->d_name);
      Xstat(buffer, &s);
      total_size += s.st_size;
/*       printf("count:%i \t file:%s \t date:%lu \t total_size:%lu\n", i, nl[i]->d_name, s.st_mtime, total_size); */

      if ((i >= file_limit) ||
	  (total_size > size_limit) ||
	  (cur_time - s.st_mtime > time_limit)) {
	DebugLog(DF_CORE, DS_INFO, "unlink(pathname=%s)\n", buffer);
	Xunlink(buffer);
      }
      Xfree(nl[i]);
    }
    Xfree(nl);
  }
}


FILE *
fopen_cached(const char *path)
{
  struct stat stat_buf;
  int ret;
  FILE *fp;

  ret = stat(path, &stat_buf);
  if (ret) {
    if (errno == ENOENT) {
      fp = Xfopen(path, "w");

      DebugLog(DF_CORE, DS_INFO, "cache MISS for \"%s\"\n", path);

      return (fp);
    }
    else {
      DebugLog(DF_CORE, DS_ERROR,
	       "fopen_cached(path=%s): %s\n",
	       path, strerror(errno));

      return (NULL);
    }
  }

  DebugLog(DF_CORE, DS_INFO, "cache hit for \"%s\"\n", path);

  /* File exists. Touch it and return cache hit. */
  utime(path, NULL);

  return (CACHE_HIT);
}


int
cached_file(const char *path)
{
  struct stat stat_buf;
  int ret;

  ret = stat(path, &stat_buf);
  if (ret) {
    if (errno == ENOENT) {
      return (0);
    }
    else {
      DebugLog(DF_CORE, DS_ERROR,
	       "cached_file(path=%s): %s\n",
	       path, strerror(errno));
      return (-1);
    }
  }

  return (1);
}

#ifdef TEST_CACHE
int
main(int argc, char *argv[])
{
  cache_gc("/tmp/cache", "cache", 100, 10000, 3600);

  return (0);
}
#endif


/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
**
** This software is a computer program whose purpose is to detect intrusions
** in a computer network.
**
** This software is governed by the CeCILL license under French law and
** abiding by the rules of distribution of free software.  You can use,
** modify and/or redistribute the software under the terms of the CeCILL
** license as circulated by CEA, CNRS and INRIA at the following URL
** "http://www.cecill.info".
**
** As a counterpart to the access to the source code and rights to copy,
** modify and redistribute granted by the license, users are provided
** only with a limited warranty and the software's author, the holder of
** the economic rights, and the successive licensors have only limited
** liability.
**
** In this respect, the user's attention is drawn to the risks associated
** with loading, using, modifying and/or developing or reproducing the
** software by the user in light of its specific status of free software,
** that may mean that it is complicated to manipulate, and that also
** therefore means that it is reserved for developers and experienced
** professionals having in-depth computer knowledge. Users are therefore
** encouraged to load and test the software's suitability as regards
** their requirements in conditions enabling the security of their
** systems and/or data to be ensured and, more generally, to use and
** operate it in the same conditions as regards security.
**
** The fact that you are presently reading this means that you have had
** knowledge of the CeCILL license and that you accept its terms.
*/

/* End-of-file */
