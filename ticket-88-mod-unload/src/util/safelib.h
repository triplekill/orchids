/**
 ** @file safelib.h
 ** Wrappers for common syscall/stdlib functions with error handling.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Mon Jan 20 17:22:39 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef SAFELIB_H
#define SAFELIB_H

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>

#ifdef DMALLOC
#include <dmalloc.h>
#endif

typedef void (*sighandler_t)(int);

/* stdlib */

#if defined (DMALLOC) || defined (LIBUNSAFE)

#define Xmalloc(size) malloc(size)
#define Xzmalloc(size) calloc(1, size)
#define Xcalloc(n, size) calloc(n, size)
#define Xrealloc(ptr, size) realloc(ptr, size)
#define Xfree(ptr) free(ptr)

#else /* defined (DMALLOC) || defined (LIBUNSAFE) */

#define Xmalloc(size) __safelib_xmalloc(__FILE__, __LINE__, size)
#define Xzmalloc(size) __safelib_xcalloc(__FILE__, __LINE__, 1, size)
/* #define Xzmalloc(size) __safelib_xzmalloc(__FILE__, __LINE__, size) */
#define Xcalloc(n, size) __safelib_xcalloc(__FILE__, __LINE__, n, size)
#define Xrealloc(ptr, size) __safelib_xrealloc(__FILE__, __LINE__, ptr, size)
#define Xfree(ptr) do { __safelib_xfree(__FILE__, __LINE__, ptr); (ptr) = NULL; } while (0)

#endif /* defined (DMALLOC) || defined (LIBUNSAFE) */

#ifdef LIBUNSAFE

#define Xrealpath(path, resolved_path) realpath(path, resolved_path)
#define Xstrdup(s) strdup(s)
#define Xfclose(fp) fclose(fp)
#define Xfdopen(fd, type) fdopen(fd, type)
#define Xfgets(ptr, n, fp) fgets(ptr, n, fp)
#define Xfopen(filename, mode) fopen(filename, mode)
#define Xpopen(command, type) popen(command, type)
#define Xpclose(stream) pclose(stream)
#define Xmkstemp(template) mkstemp(template)
#define Xopendir(name) opendir(name)
#define Xfputs(ptr, fp) fputs(ptr, fp)
#define Xsocket(family, type, protocol) socket(family, type, protocol)
#define Xaccept(fd, sa, salenptr) accept(fd, sa, salenptr)
#define Xbind(fd, sa, salen) bind(fd, sa, salen)
#define Xconnect(fd, sa, salen) connect(fd, sa, salen)
#define Xlisten(fd, backlog) listen(fd, backlog)
#define Xselect(nfds, readfds, writefds, exceptfds, timeout) select(nfds, readfds, writefds, exceptfds, timeout)
#define Xgetsockopt(fd, level, optname, optval, optlenptr) getsockopt(fd, level, optname, optval, optlenptr)
#define Xsetsockopt(fd, level, optname, optval, optlen) setsockopt(fd, level, optname, optval, optlen)
#define Xrecvfrom(fd, ptr, nbytes, flags, sa, salenptr) recvfrom(fd, ptr, nbytes, flags, sa, salenptr)
#define Xsendto(fd, ptr, nbytes, flags, sa, salen) sendto(fd, ptr, nbytes, flags, sa, salen)
#define Xfstat(fd, buf) fstat(fd, buf)
#define Xstat(filename, buf) stat(filename, buf)
#define Xread(fd, buf, count) read(fd, buf, count)
#define Xwrite(fd, ptr, nbytes) write(fd, ptr, nbytes)
#define Xsignal(signum, handler) signal(signum, handler)
#define Xuname(buf) uname(buf)
#define Xlink(oldpath, newpath) link(oldpath, newpath)
#define Xunlink(path) unlink(path)
#define Xchmod(path, mode) chmod(path, mode)
#define Xopen(path, flags, mode) open(path, flags, mode)
#define Xmmap(start, length, prot, flags, fd, offset) mmap(start, length, prot, flags, fd, offset)
#define Xmunmap(start, length) munmap(start, length)
#define Xdup(oldfd) dup(oldfd)
#define Xdup2(oldfd, newfd) dup(oldfd, newfd)

#else /* LIBUNSAFE */

#define Xrealpath(path, resolved_path) __safelib_xrealpath(__FILE__, __LINE__, path, resolved_path)
#define Xstrdup(s) __safelib_xstrdup(__FILE__, __LINE__, s)
#define Xfclose(fp) __safelib_xfclose(__FILE__, __LINE__, fp)
#define Xfdopen(fd, type) __safelib_xfdopen(__FILE__, __LINE__, fd, type)
#define Xfgets(ptr, n, fp) __safelib_xfgets(__FILE__, __LINE__, ptr, n, fp)
#define Xfopen(filename, mode) __safelib_xfopen(__FILE__, __LINE__, filename, mode)
#define Xpopen(command, type) __safelib_xpopen(__FILE__, __LINE__, command, type)
#define Xpclose(stream) __safelib_xpclose(__FILE__, __LINE__, stream)
#define Xmkstemp(template) __safelib_xmkstemp(__FILE__, __LINE__, template)
#define Xopendir(name) __safelib_xopendir(__FILE__, __LINE__, name)
#define Xfputs(ptr, fp) __safelib_xfputs(__FILE__, __LINE__, ptr, fp)
#define Xsocket(family, type, protocol) __safelib_xsocket(__FILE__, __LINE__, family, type, protocol)
#define Xaccept(fd, sa, salenptr) __safelib_xaccept(__FILE__, __LINE__, fd, sa, salenptr)
#define Xbind(fd, sa, salen) __safelib_xbind(__FILE__, __LINE__, fd, sa, salen)
#define Xconnect(fd, sa, salen) __safelib_xconnect(__FILE__, __LINE__, fd, sa, salen)
#define Xlisten(fd, backlog) __safelib_xlisten(__FILE__, __LINE__, fd, backlog)
#define Xselect(nfds, readfds, writefds, exceptfds, timeout) __safelib_xselect(__FILE__, __LINE__, nfds, readfds, writefds, exceptfds, timeout)
#define Xgetsockopt(fd, level, optname, optval, optlenptr) __safelib_xgetsockopt(__FILE__, __LINE__, fd, level, optname, optval, optlenptr)
#define Xsetsockopt(fd, level, optname, optval, optlen) __safelib_xsetsockopt(__FILE__, __LINE__, fd, level, optname, optval, optlen)
#define Xrecvfrom(fd, ptr, nbytes, flags, sa, salenptr) __safelib_xrecvfrom(__FILE__, __LINE__, fd, ptr, nbytes, flags, sa, salenptr)
#define Xsendto(fd, ptr, nbytes, flags, sa, salen) __safelib_xsendto(__FILE__, __LINE__, fd, ptr, nbytes, flags, sa, salen)
#define Xfstat(fd, buf) __safelib_xfstat(__FILE__, __LINE__, fd, buf)
#define Xstat(filename, buf) __safelib_xstat(__FILE__, __LINE__, filename, buf)
#define Xread(fd, buf, count) __safelib_xread(__FILE__, __LINE__, fd, buf, count)
#define Xwrite(fd, ptr, nbytes) __safelib_xwrite(__FILE__, __LINE__, fd, ptr, nbytes)
#define Xsignal(signum, handler) __safelib_xsignal(__FILE__, __LINE__, signum, handler)
#define Xuname(buf) __safelib_xuname(__FILE__, __LINE__, buf)
#define Xlink(oldpath, newpath) __safelib_xlink(__FILE__, __LINE__, oldpath, newpath)
#define Xunlink(path) __safelib_xunlink(__FILE__, __LINE__, path)
#define Xchmod(path, mode) __safelib_xchmod(__FILE__, __LINE__, path, mode)
#define Xopen(path, flags, mode) __safelib_xopen(__FILE__, __LINE__, path, flags, mode)
#define Xmmap(start, length, prot, flags, fd, offset) __safelib_xmmap(__FILE__, __LINE__, start, length, prot, flags, fd, offset)
#define Xmunmap(start, length) __safelib_xmunmap(__FILE__, __LINET__, start, length)
#define Xdup(oldfd) __safelib_xdup(__FILE__, __LINE__, oldfd)
#define Xdup2(oldfd, newfd) __safelib_xdup2(__FILE__, __LINE__, oldfd, newfd)


#endif /* LIBUNSAFE */

void *__safelib_xmalloc(const char *file, const int line, size_t size);
void *__safelib_xzmalloc(const char *file, const int line, size_t size);
void *__safelib_xcalloc(const char *file, const int line, size_t n, size_t size);
void *__safelib_xrealloc(const char *file, const int line, void *ptr, size_t size);
void __safelib_xrealpath(const char *file, const int line, const char *path, char *resolved_path);
void __safelib_xfree(const char *file, const int line, void *ptr);
char *__safelib_xstrdup(const char *file, const int line, char *s);

/* stdio */
void __safelib_xfclose(const char *file, const int line, FILE *fp);
FILE *__safelib_xfdopen(const char *file, const int line, int fd, const char *type);
char *__safelib_xfgets(const char *file, const int line, char *ptr, int n, FILE *stream);
FILE *__safelib_xfopen(const char *file, const int line, const char *filename, const char *mode);
FILE *__safelib_xpopen(const char *file, const int line, const char *command, const char *type);
int __safelib_xpclose(const char *file, const int line, FILE *stream);
int __safelib_xmkstemp(const char *file, const int line, char *template);
void __safelib_xfputs(const char *file, const int line, const char *ptr, FILE *stream);
DIR *__safelib_xopendir(const char *file, const int line,  const char *name);

/* syscalls */
int __safelib_xopen(const char *file, const int line, const char *pathname, int flags, mode_t mode);
int __safelib_xsocket(const char *file, const int line, int family, int type, int protocol);
int __safelib_xaccept(const char *file, const int line, int fd, struct sockaddr *sa, socklen_t *salenptr);
void __safelib_xbind(const char *file, const int line, int fd, const struct sockaddr *sa, socklen_t salen);
void __safelib_xconnect(const char *file, const int line, int fd, const struct sockaddr *sa, socklen_t salen);
void __safelib_xlisten(const char *file, const int line, int fd, int backlog);
int __safelib_xselect(const char *file, const int line, int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
void __safelib_xgetsockopt(const char *file, const int line, int fd, int level, int optname, void *optval, socklen_t *optlenptr);
void __safelib_xsetsockopt(const char *file, const int line, int fd, int level, int optname, const void *optval, socklen_t optlen);
ssize_t __safelib_xrecvfrom(const char *file, const int line, int fd, void *ptr, size_t nbytes, int flags, struct sockaddr *sa, socklen_t *salenptr);
void __safelib_xsendto(const char *file, const int line, int fd, const void *ptr, size_t nbytes, int flags, const struct sockaddr *sa, socklen_t salen);
void __safelib_xstat(const char *file, const int line, const char *file_name, struct stat *buf);
void __safelib_xfstat(const char *file, const int line, int filedes, struct stat *buf);
size_t __safelib_xread(const char *file, const int line, int fd, void *buf, size_t count);
size_t __safelib_xwrite(const char *file, const int line, int fd, void *ptr, size_t nbytes);
sighandler_t __safelib_xsignal(const char *file, const int line, int signum, sighandler_t handler);
void __safelib_xuname(const char *file, const int line, struct utsname *buf);
void __safelib_xlink(const char *file, const int line, const char *oldpath, const char *newpath);
void __safelib_xunlink(const char *file, const int line, const char *pathname);
void __safelib_xchmod(const char *file, const int line, const char *path, mode_t mode);
void  *__safelib_xmmap(const char *file, const int line, 
                       void *start, size_t length,
                       int prot, int flags, int fd,
                       off_t offset);
void __safelib_xmunmap(const char *file, const int line, void *start, size_t length);
int __safelib_xdup(const char *file, const int line, int oldfd);
int __safelib_xdup2(const char *file, const int line, int oldfd, int newfd);

void fprintf_safelib_memstat(FILE *fp);


#endif /* SAFELIB_H */



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
