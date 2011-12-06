/**
 ** @file rawsnare.h
 ** rawsnare structures.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Fri Feb 14 16:36:13 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef RAWSNARE_H
#define RAWSNARE_H

#define AUDIT_CLASS_NONE  0
#define AUDIT_CLASS_IO    1 /* Input/output (file opens) */
#define AUDIT_CLASS_PC    2 /* Process Control */
#define AUDIT_CLASS_EXEC  3 /* Execution */
#define AUDIT_CLASS_NET   4 /* Network related */
#define AUDIT_CLASS_ADMIN 5 /* Administrative events (XXX: NOT USED) */
#define AUDIT_CLASS_CH    6 /* CHMOD event.*/
#define AUDIT_CLASS_CP    7 /* Where more than one pathname is required */
#define AUDIT_CLASS_SU    8 /* SetUID */
#define AUDIT_CLASS_AD    9 /* Admin such as create/delete module */
#define AUDIT_CLASS_PT    10 /* Special ptrace */
#define AUDIT_CLASS_KILL  11 /* Special kill */
#define NUMCLASS          12

#define MAX_PATH 512

typedef struct header_token_s header_token_t;
struct header_token_s
{
  unsigned short event_class;
  unsigned short event_id;
  unsigned short event_size;
  struct timeval time;

  int user_id;
  int euser_id;
  int group_id;
  int egroup_id;
};

typedef struct return_token_s return_token_t;
struct return_token_s
{
  int returncode;
};

typedef struct path_token_s path_token_t;
struct path_token_s
{
  char path[MAX_PATH];
};

typedef struct attributes_token_s attributes_token_t;
struct attributes_token_s
{
  int mode;
  unsigned long createmode;
};

typedef struct owner_token_s owner_token_t;
struct owner_token_s
{
  int owner;
  int group;
};

typedef struct execargs_token_s execargs_token_t;
struct execargs_token_s
{
  char args[MAX_PATH];
};

#define MAXCOMMAND 16
typedef struct process_token_s process_token_t;
struct process_token_s
{
  pid_t pid;
  pid_t ppid;
  char name[MAXCOMMAND];
};

typedef struct target_token_s target_token_t;
struct target_token_s
{
  int id;
  int rid;
  int sid;
};

typedef struct connection_token_s connection_token_t;
struct connection_token_s
{
  char src_ip[40];
  int  src_port;
  char dst_ip[40];
  int  dst_port;
};

typedef struct null_class_s null_class_t;
struct null_class_s
{
  header_token_t t_header;
};

typedef struct io_class_s io_class_t;
struct io_class_s
{
  header_token_t     t_header;
  return_token_t     t_return;
  process_token_t    t_process;
  path_token_t       t_path;
  path_token_t       t_pwd;
  attributes_token_t t_attributes;
};

typedef struct ch_class_s ch_class_t;
struct ch_class_s
{
  header_token_t  t_header;
  return_token_t  t_return;
  process_token_t t_process;
  path_token_t    t_path;
  path_token_t    t_pwd;
  owner_token_t   t_owner;
};

typedef struct ex_class_s ex_class_t;
struct ex_class_s
{
  header_token_t   t_header;
  return_token_t   t_return;
  process_token_t  t_process;
  path_token_t     t_path;
  path_token_t     t_pwd;
  execargs_token_t t_execargs;
};

typedef struct pc_class_s pc_class_t;
struct pc_class_s
{
  header_token_t  t_header;
  return_token_t  t_return;
  process_token_t t_process;
};

typedef struct cp_class_s cp_class_t;
struct cp_class_s
{
  header_token_t  t_header;
  return_token_t  t_return;
  process_token_t t_process;
  path_token_t    t_sourcepath;
  path_token_t    t_pwd;
  path_token_t    t_destpath;
};

typedef struct su_class_s su_class_t;
struct su_class_s
{
  header_token_t  t_header;
  return_token_t  t_return;
  process_token_t t_process;
  target_token_t  t_target;
};

typedef struct nt_class_s nt_class_t;
struct nt_class_s
{
  header_token_t     t_header;
  return_token_t     t_return;
  process_token_t    t_process;
  int                syscall;
  connection_token_t t_connection;
};

typedef struct ad_class_s ad_class_t;
struct ad_class_s
{
  header_token_t  t_header;
  return_token_t  t_return;
  process_token_t t_process;
  path_token_t    t_name;
};

typedef struct pt_class_s pt_class_t;
struct pt_class_s
{
  header_token_t  t_header;
  return_token_t  t_return;
  process_token_t t_process;
  int             request; /* ptrace request (see <sys/ptrace.h>) */
  long            addr;
  long            data;
  pid_t           pid;
} pt_class;

typedef struct kill_class_s kill_class_t;
struct kill_class_s
{
  header_token_t  t_header;
  return_token_t  t_return;
  process_token_t t_process;
  pid_t           pid;
  int             sig;
} kill_class;

/* --- linux syscall names --- */

#endif /* RAWSNARE_H */


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
