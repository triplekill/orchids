%{

/**
 ** @file snare.l
 ** parser for Snare text log files.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Thu Sep 11 18:41:03 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h> /* for open() modes */

#include "snare.tab.h"

#include "orchids.h"

int snarelex(void);
void snareerror(char *s);
void snareparse_set_str(char *str, size_t s);
void snareparse_reset(void);

static int my_yyinput(char *buf, int max_size);


#define JAN 0
#define FEB 1
#define MAR 2
#define AVR 3
#define MAY 4
#define JUN 5
#define JUL 6
#define AUG 7
#define SEP 8
#define OCT 9
#define NOV 10
#define DEC 11

static char *input_string_g = NULL;
static int   input_string_sz_g = 0;
static char *input_string_ptr_g = NULL;

#define YY_INPUT(b, r, ms) (r = my_yyinput(b, ms))

%}

%option nounput never-interactive

%x SYSCALL
%x DATA_STR
%x DATA_STR_SUB
%x INTSTR
%x DATE
%x ATTRIBS
%x STRADDR

%%

 /* reserved keywords and operators */
"LinuxAudit" {                 return (LINUX_AUDIT);      }
"event"      { BEGIN SYSCALL;  return (TOKEN_EVENT);      }
"user"       { BEGIN INTSTR;   return (TOKEN_USER);       }
"process"    { BEGIN INTSTR;   return (TOKEN_PROCESS);    }
"path"       { BEGIN DATA_STR; return (TOKEN_PATH);       }
"destpath"   { BEGIN DATA_STR; return (TOKEN_DESTPATH);   }
"owner"      { BEGIN INTSTR;   return (TOKEN_OWNER);      }
"attributes" { BEGIN ATTRIBS;  return (TOKEN_ATTRIBUTES); }
"arguments"  { BEGIN DATA_STR; return (TOKEN_ARGUMENTS);  }
"target"     { BEGIN INTSTR;   return (TOKEN_TARGET);     }
"socket"     { BEGIN STRADDR;  return (TOKEN_SOCKET);     }
"return"     { BEGIN INTSTR;   return (TOKEN_RETURN);     }
"sequence"   { BEGIN INTSTR;   return (TOKEN_SEQUENCE);   }

 /* syscalls */
 /* io class syscalls  */
<SYSCALL>"chmod"       { return (SYS_CHMOD);        }
<SYSCALL>"chroot"      { return (SYS_CHROOT);       }
<SYSCALL>"creat"       { return (SYS_CREAT);        }
<SYSCALL>"fchmod"      { return (SYS_FCHMOD);       }
<SYSCALL>"ftruncate64" { return (SYS_FTRUNCATE64);  }
<SYSCALL>"ftruncate"   { return (SYS_FTRUNCATE);    }
<SYSCALL>"mkdir"       { return (SYS_MKDIR);        }
<SYSCALL>"mknod"       { return (SYS_MKNOD);        }
<SYSCALL>"open"        { return (SYS_OPEN);         }
<SYSCALL>"rmdir"       { return (SYS_RMDIR);        }
<SYSCALL>"truncate64"  { return (SYS_TRUNCATE64);   }
<SYSCALL>"truncate"    { return (SYS_TRUNCATE);     }
<SYSCALL>"unlink"      { return (SYS_UNLINK);       }

 /* chown class syscalls */
<SYSCALL>"chown32"    { return (SYS_CHOWN32);      }
<SYSCALL>"chown"      { return (SYS_CHOWN);        }
<SYSCALL>"fchown"     { return (SYS_FCHOWN);       }
<SYSCALL>"lchown32"   { return (SYS_LCHOWN32);     }
<SYSCALL>"lchown"     { return (SYS_LCHOWN);       }

 /* exec class syscalls */
<SYSCALL>"execve"     { return (SYS_EXECVE);       }

 /* process control class syscalls */
<SYSCALL>"exit"       { return (SYS_EXIT);         }
<SYSCALL>"reboot"     { return (SYS_REBOOT);       }

 /* net class syscalls */
<SYSCALL>"socketcall" { return (SYS_SOCKETCALL);   }

 /* copy class syscalls */
<SYSCALL>"link"       { return (SYS_LINK);         }
<SYSCALL>"mount"      { return (SYS_MOUNT);        }
<SYSCALL>"rename"     { return (SYS_RENAME);       }
<SYSCALL>"symlink"    { return (SYS_SYMLINK);      }
<SYSCALL>"umount"     { return (SYS_UMOUNT);       }

 /* su class syscalls */
<SYSCALL>"setuid32"    { return (SYS_SETUID32);     }
<SYSCALL>"setuid"      { return (SYS_SETUID);       }
<SYSCALL>"setreuid32"  { return (SYS_SETREUID32);   }
<SYSCALL>"setreuid"    { return (SYS_SETREUID);     }
<SYSCALL>"setresuid32" { return (SYS_SETRESUID32);  }
<SYSCALL>"setresuid"   { return (SYS_SETRESUID);    }
<SYSCALL>"setgid32"    { return (SYS_SETGID32);     }
<SYSCALL>"setgid"      { return (SYS_SETGID);       }
<SYSCALL>"setregid32"  { return (SYS_SETREGID32);   }
<SYSCALL>"setregid"    { return (SYS_SETREGID);     }
<SYSCALL>"setresgid32" { return (SYS_SETRESGID32);  }
<SYSCALL>"setresgid"   { return (SYS_SETRESGID);    }

 /* kernel admin class syscalls */
<SYSCALL>"create_module" { return (SYS_CREATEMODULE); }
<SYSCALL>"delete_module" { return (SYS_DELETEMODULE); }

 /* open() mode */
<SYSCALL>"O_WRONLY"    { return (OPT_WRONLY);    }
<SYSCALL>"O_RDONLY"    { return (OPT_RDONLY);    }
<SYSCALL>"O_RDWR"      { return (OPT_RDWR);      }

 /* open() opts */
<SYSCALL>"O_CREAT"     { return (OPT_CREAT);     }
<SYSCALL>"O_EXCL"      { return (OPT_EXCL);      }
<SYSCALL>"O_NOCTTY"    { return (OPT_NOCTTY);    }
<SYSCALL>"O_TRUNC"     { return (OPT_TRUNC);     }
<SYSCALL>"O_APPEND"    { return (OPT_APPEND);    }
<SYSCALL>"O_NONBLOCK"  { return (OPT_NONBLOCK);  }
<SYSCALL>"O_SYNC"      { return (OPT_SYNC);      }
<SYSCALL>"O_NOFOLLOW"  { return (OPT_NOFOLLOW);  }
<SYSCALL>"O_DIRECTORY" { return (OPT_DIRECTORY); }
<SYSCALL>"O_LARGEFILE" { return (OPT_LARGEFILE); }

 /* mknod() node type */
<SYSCALL>"S_IFREG" { return (OPT_IFREG); }
<SYSCALL>"S_IFCHR" { return (OPT_IFCHR); }
<SYSCALL>"S_IFBLK" { return (OPT_IFBLK); }
<SYSCALL>"S_IFIFO" { return (OPT_IFIFO); }

 /* socketcall() type */
<SYSCALL>"ACCEPT"  { return (SOCK_ACCEPT); }
<SYSCALL>"CONNECT" { return (SOCK_CONNECT); }
<SYSCALL>"UNKNOWN" { return (SOCK_UNKNOWN); }

<SYSCALL>")" { BEGIN DATE; return (')'); }
<SYSCALL>. { return (snaretext[0]); }

<DATE>(Mon|Tue|Wed|Thu|Fri|Dat|Sun)\ (Jan|Feb|Mar|Apr|May|Jun|Jui|Aug|Sep|Oct|Nov|Dec)\ (\ [0-9]|[0-9]{2})\ [0-9]{2}:[0-9]{2}:[0-9]{2}\ [0-9]{4} {
  struct tm t;
  int day;

  switch (snaretext[4])
    {
    case 'A': /* Aug, Avr */
      switch (snaretext[5])
        {
        case 'u':
          t.tm_mon = AUG;
          break;
        case 'v':
          t.tm_mon = AVR;
          break;
        }
      break;

    case 'D': /* Dec */
      t.tm_mon = DEC;
      break;

    case 'F': /* Feb */
      t.tm_mon = FEB;
      break;

    case 'J': /* Jan, Jul, Jun */
      if (snaretext[5] == 'a')
        t.tm_mon = JAN;
      else if (snaretext[6] == 'l')
        t.tm_mon = JUL;
      else if (snaretext[6] == 'n')
        t.tm_mon = JUN;
      break;

    case 'M': /* Mar, May */
      switch (snaretext[6])
        {
        case 'r':
          t.tm_mon = MAR;
          break;
        case 'y':
          t.tm_mon = MAY;
          break;
        }
      break;

    case 'N': /* Nov */
          t.tm_mon = NOV;
      break;

    case 'O': /* Oct */
          t.tm_mon = OCT;
      break;

    case 'S': /* Sep */
          t.tm_mon = SEP;
      break;

    default:
      break;
    }

  if (snaretext[8] != ' ')
    day = (snaretext[8] - '0') * 10;
  else
    day = 0;
  day += snaretext[9] - '0';
  t.tm_mday = day;

  t.tm_hour = (snaretext[11] - '0') * 10 + (snaretext[12] - '0');
  t.tm_min = (snaretext[14] - '0') * 10 + (snaretext[15] - '0');
  t.tm_sec = (snaretext[17] - '0') * 10 + (snaretext[18] - '0');
  t.tm_year = ((snaretext[20] - '0') * 1000 + (snaretext[21] - '0') * 100 + 
    (snaretext[22] - '0') * 10 + (snaretext[23] - '0')) - 1900;

  snarelval.time = mktime(&t);

  return (TOKEN_DATE);
}

<DATE>\t { BEGIN INITIAL; return ('\t'); }
<DATE>. { return (snaretext[0]); }

<STRADDR>([0-9]{1,3}\.){3}[0-9]{1,3} { snarelval.ipv4addr.s_addr = inet_addr(snaretext); return (INETADDR); }
<STRADDR>[0-9]+ { snarelval.integer = atoi(snaretext); return (INTEGER); }
<STRADDR>\t { BEGIN INITIAL; return (snaretext[0]); }
<STRADDR>. { return (snaretext[0]); }

<DATA_STR>, { BEGIN DATA_STR_SUB; return (snaretext[0]); }
<DATA_STR_SUB>[^\t]+ { BEGIN INITIAL; /* DPRINTF( ("data_string %s\n", snaretext) ); */ snarelval.str = strdup(snaretext); return (DATA_STRING); }

<INTSTR>-?[0-9]+ { /* DPRINTF( ("int %s\n", snaretext) ); */ snarelval.integer = atoi(snaretext); return (INTEGER); }
<INTSTR>[a-zA-Z0-9\._\/\-]+ { /* DPRINTF( ("string \"%s\"\n", snaretext) ); */ snarelval.str = strdup(snaretext); return (STRING); }
<INTSTR>\t { BEGIN INITIAL; return (snaretext[0]); }
<INTSTR>\n /* skip new lines */;
<INTSTR>. { return (snaretext[0]); }

<ATTRIBS>-?[0-9]+ { snarelval.integer = atoi(snaretext); return (INTEGER); }
<ATTRIBS>[rwxsS\-]+ {
  snarelval.mode = 0;
  if (snaretext[0] == 'r') snarelval.mode |= S_IRUSR;
  if (snaretext[1] == 'w') snarelval.mode |= S_IWUSR;
  if (snaretext[2] == 'x') snarelval.mode |= S_IXUSR;
  if (snaretext[3] == 'r') snarelval.mode |= S_IRGRP;
  if (snaretext[4] == 'w') snarelval.mode |= S_IWGRP;
  if (snaretext[5] == 'x') snarelval.mode |= S_IXGRP;
  if (snaretext[6] == 'r') snarelval.mode |= S_IROTH;
  if (snaretext[7] == 'w') snarelval.mode |= S_IWOTH;
  if (snaretext[8] == 'x') snarelval.mode |= S_IXOTH;

  return (UNIXPERM);
}
<ATTRIBS>\t { BEGIN INITIAL; return (snaretext[0]); }
<ATTRIBS>\n /* skip new lines */;
<ATTRIBS>. { return (snaretext[0]); }

-?[0-9]+ { snarelval.integer = atoi(snaretext); return (INTEGER); }
[a-zA-Z0-9\._\/\-]+ { /* DPRINTF( ("string %s\n", snaretext) ); */ return (STRING); }

\n /* skip new lines */;

. { return (snaretext[0]); }

%%

/*
** yywrap()
*/

static int
my_yyinput(char *buf, int max_size)
{
  int n;

  if (max_size > (input_string_g + input_string_sz_g - input_string_ptr_g))
    n = (input_string_g + input_string_sz_g - input_string_ptr_g);
  else
    n = max_size;

  if (n > 0) {
    memcpy(buf, input_string_ptr_g, n);
    input_string_ptr_g += n;
  }

  return (n);
}

void
snareparse_reset(void)
{
  snare_flush_buffer(YY_CURRENT_BUFFER);
  BEGIN(INITIAL);
}

void
snareparse_set_str(char *str, size_t s)
{
  input_string_g = str;
  input_string_sz_g = s;
  input_string_ptr_g = str;
}

int
snarewrap(void)
{
  return (1);
}



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
