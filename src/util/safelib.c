/**
 ** @file safelib.c
 ** wrappers for common syscall/stdlib functions with error handling.
 ** recommandations taken from GNU coding standards, R. Stevens APUE...
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Mon Jan 20 17:11:46 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "safelib.h"

#ifndef LIBUNSAFE

static unsigned long __safelib_allocated = 0;
static unsigned long __safelib_freed = 0;


/* ---------------- stdlib ------------------ */

void *
__safelib_xzmalloc(const char *file, const int line, size_t size)
{
  void *ptr;

  if (size > 0) {
    __safelib_allocated++;
    ptr = malloc(size);
    if (!ptr) {
      fprintf(stderr, "%s:%i:malloc(size=%zd): errno=%i: %s\n",
              file, line, size, errno, strerror(errno));
      exit(EXIT_FAILURE);
    }
    memset(ptr, 0, size);
  } else {
    fprintf(stderr, "warning: malloc(size=%zd)\n", size);
    return (NULL);
  }

  return (ptr);
}

void *
__safelib_xcalloc(const char *file, const int line, size_t n, size_t size)
{
  void *ptr;

  __safelib_allocated++;
  if ((ptr = calloc(n, size)) == NULL) {
    fprintf(stderr, "%s:%i:calloc(blocks=%zd,size=%zd): errno=%i: %s\n",
            file, line, n, size, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (ptr);
}


void *
__safelib_xmalloc(const char *file, const int line, size_t size)
{
  void *ptr;

  if (size > 0) {
    __safelib_allocated++;
    ptr = malloc(size);
    if (!ptr) {
      fprintf(stderr, "%s:%i:malloc(size=%zd): errno=%i: %s\n",
              file, line, size, errno, strerror(errno));
      exit(EXIT_FAILURE);
    }
  } else {
    fprintf(stderr, "warning: malloc(size=%zd)\n", size);
    return (NULL);
  }

  return (ptr);
}

void *
__safelib_xrealloc(const char *file, const int line, void *ptr, size_t size)
{
  void *new_ptr;

  new_ptr = realloc(ptr, size);
  if (!new_ptr) {
    fprintf(stderr, "%s:%i:realloc(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (new_ptr);
}

void 
__safelib_xfree(const char *file, const int line, void *ptr)
{
  __safelib_freed++;

  if (ptr == NULL) {
    fprintf(stderr, "%s:%i: Warning: free(NULL)\n", file, line);
    raise(SIGSTOP);
    return ;
  }

  free(ptr);
}

char *
__safelib_xstrdup(const char *file, const int line, char *s)
{
  char  *ns;

  ns = strdup(s);
  if (!ns) {
    fprintf(stderr, "%s:%i:strdup(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (ns);
}

void
__safelib_xrealpath(const char *file, const int line, const char *path, char *resolved_path)
{
  if (realpath(path, resolved_path) == NULL) {
    fprintf(stderr, "%s:%i:realpath(path=%s): errno=%i: %s\n",
            file, line, path, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

DIR *
__safelib_xopendir(const char *file, const int line,  const char *name)
{
  DIR *dir;

  dir = opendir(name);
  if (dir == NULL) {
    fprintf(stderr, "%s:%i:opendir(path=%s): errno=%i: %s\n",
            file, line, name, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (dir);
}

/* --- stdio --- */

void
__safelib_xfclose(const char *file, const int line, FILE *fp)
{
  if (fclose(fp) != 0) {
    fprintf(stderr, "%s:%i:fclose(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

FILE *
__safelib_xfdopen(const char *file, const int line, int fd, const char *type)
{
  FILE *fp;

  if ( (fp = fdopen(fd, type)) == NULL) {
    fprintf(stderr, "%s:%i:xfdopen(fd=%i,mode=%s): errno=%i: %s\n",
            file, line, fd, type, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return(fp);
}

char *
__safelib_xfgets(const char *file, const int line, char *ptr, int n, FILE *stream)
{
  char *rptr;

  if ( (rptr = fgets(ptr, n, stream)) == NULL && ferror(stream)) {
    fprintf(stderr, "%s:%i:xfgets(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (rptr);
}

FILE *
__safelib_xfopen(const char *file, const int line, const char *filename, const char *mode)
{
  FILE *fp;

  if ( (fp = fopen(filename, mode)) == NULL) {
    fprintf(stderr, "%s:%i:xfopen(filename=%s,mode=%s): errno=%i: %s\n",
            file, line, filename, mode, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return(fp);
}

void
__safelib_xfputs(const char *file, const int line, const char *ptr, FILE *stream)
{
  if (fputs(ptr, stream) == EOF) {
    fprintf(stderr, "%s:%i:xfputs(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

FILE *
__safelib_xpopen(const char *file, const int line, const char *command, const char *type)
{
  FILE *fp;

  if ( (fp = popen(command, type)) == NULL) {
    fprintf(stderr, "%s:%i:xpopen(command=%s,type=%s): errno=%i: %s\n",
            file, line, command, type, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return(fp);
}

int
__safelib_xpclose(const char *file, const int line, FILE *stream)
{
  int ret;

  ret = pclose(stream);
  if (ret == -1 && errno != ECHILD) {
    fprintf(stderr, "%s:%i:xpclose(): errno=%i: %s\n",
            file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
#ifdef LIBDEBUG_VERBOSE
  else if (ret == -1 && errno == ECHILD) {
    fprintf(stderr, "%s:%i:info:xpclose(): errno=%i: cannot obtain the child status\n",
            file, line, errno);
  }
#endif

  return (ret);
}

int
__safelib_xmkstemp(const char *file, const int line, char *template)
{
  int ret;

  if ((ret = mkstemp(template)) == -1) {
    fprintf(stderr, "%s:%i:xmkstemp(template=%s): errno=%i: %s\n",
            file, line, template, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (ret);
}


/* ---------------- syscall ------------------ */

int
__safelib_xopen(const char *file, const int line, const char *pathname, int flags, mode_t mode)
{
  int fd;

  if ((fd = open(pathname, flags, mode)) < 0) {
    fprintf(stderr, "%s:%i:open(path=%s,flags=%08x,mode=%04o): errno=%i: %s\n",
            file, line, pathname, flags, mode, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (fd);
}

void
__safelib_xuname(const char *file, const int line, struct utsname *buf)
{
  if (uname(buf) != 0) {
    fprintf(stderr, "%s:%i:uname(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void
__safelib_xstat(const char *file, const int line, const char *file_name, struct stat *buf)
{
  if (stat(file_name, buf) < 0) {
    fprintf(stderr, "%s:%i:stat(file=%s): errno=%i: %s\n",
            file, line, file_name, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void
__safelib_xfstat(const char *file, const int line, int filedes, struct stat *buf)
{
  if (fstat(filedes, buf) < 0) {
    fprintf(stderr, "%s:%i:fstat(fd=%i): errno=%i: %s\n",
            file, line, filedes, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}


int
__safelib_xsocket(const char *file, const int line, int family, int type, int protocol)
{
  int n;

  if ((n = socket(family, type, protocol)) < 0) {
    fprintf(stderr, "%s:%i:socket(fam=%i,type=%i,proto=%i): errno=%i: %s\n",
            file, line, family, type, protocol, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (n);
}


int
__safelib_xaccept(const char *file, const int line, int fd, struct sockaddr *sa, socklen_t *salenptr)
{
  int n;

again:
  if ((n = accept(fd, sa, salenptr)) < 0) {
    if (errno == ECONNABORTED) {
      goto again;
    } else {
      fprintf(stderr, "%s:%i:accept(): errno=%i: %s\n", file, line, errno, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  return (n);
}

void
__safelib_xbind(const char *file, const int line, int fd, const struct sockaddr *sa, socklen_t salen)
{
  if (bind(fd, sa, salen) < 0) {
    fprintf(stderr, "%s:%i:bind(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void
__safelib_xconnect(const char *file, const int line, int fd, const struct sockaddr *sa, socklen_t salen)
{
  if (connect(fd, sa, salen) < 0) {
    fprintf(stderr, "%s:%i:connect(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void
__safelib_xlisten(const char *file, const int line, int fd, int backlog)
{
  if (listen(fd, backlog) < 0) {
    fprintf(stderr, "%s:%i:listen(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}


int
__safelib_xselect(const char *file, const int line, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	struct timeval *timeout)
{
  int n;

again:
  if ( (n = select(nfds, readfds, writefds, exceptfds, timeout)) < 0) {
    if (errno == EINTR) {
#ifdef LIBDEBUG_VERBOSE
      fprintf(stderr, "%s:%i:info:select(): errno=%i: %s\n", file, line, errno, strerror(errno));
#endif
      goto again;
    } else {
      fprintf(stderr, "%s:%i:select(): errno=%i: %s\n", file, line, errno, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  return (n);
}

void
__safelib_xgetsockopt(const char *file, const int line, int fd, int level, int optname, void *optval, socklen_t *optlenptr)
{
  if (getsockopt(fd, level, optname, optval, optlenptr) < 0) {
    fprintf(stderr, "%s:%i:getsockopt(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void
__safelib_xsetsockopt(const char *file, const int line, int fd, int level, int optname, const void *optval,
	    socklen_t optlen)
{
  if (setsockopt(fd, level, optname, optval, optlen) < 0) {
    fprintf(stderr, "%s:%i:setsockopt(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void
__safelib_xsendto(const char *file, const int line, int fd, const void *ptr, size_t nbytes, int flags,
       const struct sockaddr *sa, socklen_t salen)
{
  if (sendto(fd, ptr, nbytes, flags, sa, salen) != nbytes) {
    fprintf(stderr, "%s:%i:sendto(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

ssize_t
__safelib_xrecvfrom(const char *file, const int line, int fd, void *ptr, size_t nbytes, int flags,
          struct sockaddr *sa, socklen_t *salenptr)
{
  ssize_t n;

  if ( (n = recvfrom(fd, ptr, nbytes, flags, sa, salenptr)) < 0) {
    fprintf(stderr, "%s:%i:recvfrom(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (n);
}

size_t
__safelib_xread(const char *file, const int line, int fd, void *buf, size_t count)
{
  ssize_t n;

  if ( (n = read(fd, buf, count)) == -1) {
    fprintf(stderr, "%s:%i:read(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (n);
}

size_t
__safelib_xwrite(const char *file, const int line, int fd, void *ptr, size_t nbytes)
{
  size_t n;

  if ((n = write(fd, ptr, nbytes)) == -1) {
    fprintf(stderr, "%s:%i:write(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (n);
}

void
__safelib_xlink(const char *file, const int line, const char *oldpath, const char *newpath)
{
  int ret;

  ret = link(oldpath, newpath);
  if (ret == -1) {
    fprintf(stderr, "%s:%i:link(oldpath=%s,oldpath=%s): errno=%i: %s\n",
            file, line, oldpath, newpath, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void
__safelib_xunlink(const char *file, const int line, const char *pathname)
{
  int ret;

  ret = unlink(pathname);
  if (ret == -1) {
    fprintf(stderr, "%s:%i:unlink(path=%s): errno=%i: %s\n",
            file, line, pathname, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void
__safelib_xchmod(const char *file, const int line, const char *path, mode_t mode)
{
  int ret;

  ret = chmod(path, mode);
  if (ret == -1) {
    fprintf(stderr, "%s:%i:chmod(path=%s,mode=%o): errno=%i: %s\n",
            file, line, path, mode, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}

sighandler_t
__safelib_xsignal(const char *file, const int line, int signum, sighandler_t handler)
{
  sighandler_t ret;

  ret = signal(signum, handler);
  if (ret == SIG_ERR) {
    fprintf(stderr, "%s:%i:signal(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (ret);
}

void  *
__safelib_xmmap(const char *file, const int line, 
                void *start, size_t length,
                int prot, int flags, int fd,
                off_t offset)
{
  void *ptr;

  ptr = mmap(start, length, prot, flags, fd, offset);
  if (ptr == MAP_FAILED) {
    fprintf(stderr, "%s:%i:mmap(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (ptr);
}

void
__safelib_xmunmap(const char *file, const int line, void *start, size_t length)
{
  int ret;

  ret = munmap(start, length);
  if (ret) {
    fprintf(stderr, "%s:%i:munmap(): errno=%i: %s\n", file, line, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }
}


int
__safelib_xdup(const char *file, const int line, int oldfd)
{
  int ret;

  ret = dup(oldfd);
  if (ret) {
    fprintf(stderr, "%s:%i:dup(oldfd=%i): errno=%i: %s\n",
            file, line, oldfd, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (ret);
}

int
__safelib_xdup2(const char *file, const int line, int oldfd, int newfd)
{
  int ret;

  ret = dup2(oldfd, newfd);
  if (ret) {
    fprintf(stderr, "%s:%i:dup2(oldfd=%i,newfd=%i): errno=%i: %s\n",
            file, line, oldfd, newfd, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  return (ret);
}



void
fprintf_safelib_memstat(FILE *fp)
{
  static long last_diff = 0;
  long diff;

  diff = __safelib_allocated - __safelib_freed;
  fprintf(fp, "alloc: %lu free: %lu (diff %li diff2 %li)\n",
          __safelib_allocated, __safelib_freed, diff, diff - last_diff);
  last_diff = diff;
}

#endif /* LIBUBSAFE */

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
