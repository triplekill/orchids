/**
 ** @file orchids_defaults.h
 ** Orchids default parameters definition.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup core
 **
 ** @date  Started on: Tue Jan 14 14:00:41 2003
 **/


/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef ORCHIDS_DEFAULTS_H
#define ORCHIDS_DEFAULTS_H

#ifndef SYSCONFDIR
#define SYSCONFDIR "/etc"
#endif

#ifndef PKGDATADIR
#define PKGDATADIR "/usr/share/orchids"
#endif

#ifndef LOCALSTATEDIR
#define LOCALSTATEDIR "/var"
#endif

#ifndef LIBDIR
#define LIBDIR "/usr/lib"
#endif

#define DEFAULT_CONFIG_FILE SYSCONFDIR "/orchids/orchids.conf"

#define DEFAULT_MODULES_DIR LIBDIR "/orchids/modules"

#define DEFAULT_IN_PERIOD 5
#define DEFAULT_RULE_DIR "./rules"
#define DEFAULT_FIELD_ACTIVATION 1

#define DEFAULT_TIMEOUT 600

/* #define PATH_TO_DOT "/usr/local/bin/dot" */
/* #define PATH_TO_EPSTOPDF "/usr/bin/epstopdf" */
/* #define PATH_TO_CONVERT "/usr/X11R6/bin/convert" */

#define COMMAND_PREFIX "nice -15 "

#ifndef PATH_TO_DOT
# define PATH_TO_DOT "dot"
#endif /* PATH_TO_DOT */

#ifndef PATH_TO_EPSTOPDF
# define PATH_TO_EPSTOPDF "epstopdf"
#endif /* PATH_TO_EPSTOPDF */

#ifndef PATH_TO_CONVERT
# define PATH_TO_CONVERT "convert"
#endif /* PATH_TO_CONVERT */

#ifndef PATH_TO_CPP
# define PATH_TO_CPP "cpp"
#endif /* PATH_TO_CPP */

#define DEFAULT_PREPROC_CMD "cat"

#ifndef PATH_TO_SENDMAIL
# define PATH_TO_SENDMAIL "/usr/sbin/sendmail"
#endif /* PATH_TO_SENDMAIL */

#define DEFAULT_ORCHIDS_LOCKFILE    LOCALSTATEDIR "/run/orchids/orchids.lock"
#define DEFAULT_OUTPUTHTML_LOCKFILE LOCALSTATEDIR "/run/orchids/orchids-outputhtml.lock"

#define ORCHIDS_TMPDIR "/tmp"
#define ORCHIDS_SENDMAIL_TEMPFILE \
  ORCHIDS_TMPDIR "/orchids-sendmail-temp-XXXXXX"

#endif /* ORCHIDS_DEFAULTS_H */



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
