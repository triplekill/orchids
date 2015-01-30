/**
 ** @file mod_openbsm.h
 ** Header file for mod_openbsm.c
 **
 ** @author Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup modules
 **
 ** @date  Started on: Ven  3 oct 2014 12:53:46 UTC
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */


#define F_OPENBSM_HDR_START 0
# define F_OPENBSM_HDR_KIND 0
# define F_OPENBSM_HDR_VERSION 1
# define F_OPENBSM_HDR_TYPE 2
# define F_OPENBSM_HDR_MODIFIER 3
# define F_OPENBSM_HDR_TIME 4
# define F_OPENBSM_HDR_IP 5
# define F_OPENBSM_HDR_FILE 6
#define F_OPENBSM_HDR_SIZE 7
#define F_OPENBSM_HDR_END (F_OPENBSM_HDR_START+F_OPENBSM_HDR_SIZE)

#define F_OPENBSM_DATA_START F_OPENBSM_HDR_END
#define F_OPENBSM_DATA_DATA 0
#define F_OPENBSM_DATA_SIZE 1
#define F_OPENBSM_DATA_END (F_OPENBSM_DATA_START+F_OPENBSM_DATA_SIZE)

#define F_OPENBSM_ATTR_START F_OPENBSM_DATA_END
#define F_OPENBSM_ATTR_FILE_ACCESS_MODE 0
#define F_OPENBSM_ATTR_OWNER_UID 1
#define F_OPENBSM_ATTR_OWNER_GID 2
#define F_OPENBSM_ATTR_FSID 3
#define F_OPENBSM_ATTR_NID 4
#define F_OPENBSM_ATTR_DEV 5
#define F_OPENBSM_ATTR_SIZE 6
#define F_OPENBSM_ATTR_END (F_OPENBSM_ATTR_START+F_OPENBSM_ATTR_SIZE)

#define F_OPENBSM_EXIT_START F_OPENBSM_ATTR_END
#define F_OPENBSM_EXIT_STATUS 0
#define F_OPENBSM_EXIT_RETVAL 1
#define F_OPENBSM_EXIT_SIZE 2
#define F_OPENBSM_EXIT_END (F_OPENBSM_EXIT_START+F_OPENBSM_EXIT_SIZE)

#define F_OPENBSM_INADDR_START F_OPENBSM_EXIT_END
#define F_OPENBSM_INADDR_ADDR 0
#define F_OPENBSM_INADDR_SIZE 1
#define F_OPENBSM_INADDR_END (F_OPENBSM_INADDR_START+F_OPENBSM_INADDR_SIZE)

#define F_OPENBSM_INADDR6_START F_OPENBSM_INADDR_END
#define F_OPENBSM_INADDR6_ADDR 0
#define F_OPENBSM_INADDR6_SIZE 1
#define F_OPENBSM_INADDR6_END (F_OPENBSM_INADDR6_START+F_OPENBSM_INADDR6_SIZE)

#define F_OPENBSM_IP_START F_OPENBSM_INADDR6_END
#define F_OPENBSM_IP_VERSION 0
#define F_OPENBSM_IP_TOS 1
#define F_OPENBSM_IP_LEN 2
#define F_OPENBSM_IP_ID 3
#define F_OPENBSM_IP_OFFSET 4
#define F_OPENBSM_IP_TTL 5
#define F_OPENBSM_IP_PROTOCOL 6
#define F_OPENBSM_IP_CHECKSUM 7
#define F_OPENBSM_IP_SOURCE 8
#define F_OPENBSM_IP_DEST 9
#define F_OPENBSM_IP_SIZE 10
#define F_OPENBSM_IP_END (F_OPENBSM_IP_START+F_OPENBSM_IP_SIZE)

#define F_OPENBSM_IPC_START F_OPENBSM_IP_END
#define F_OPENBSM_IPC_TYPE 0
#define F_OPENBSM_IPC_ID 1
#define F_OPENBSM_IPC_SIZE 2
#define F_OPENBSM_IPC_END (F_OPENBSM_IPC_START+F_OPENBSM_IPC_SIZE)

#define F_OPENBSM_IPCPERM_START F_OPENBSM_IPC_END
#define F_OPENBSM_IPCPERM_UID 0
#define F_OPENBSM_IPCPERM_GID 1
#define F_OPENBSM_IPCPERM_PUID 2
#define F_OPENBSM_IPCPERM_PGID 3
#define F_OPENBSM_IPCPERM_MODE 4
#define F_OPENBSM_IPCPERM_SEQ 5
#define F_OPENBSM_IPCPERM_KEY 6
#define F_OPENBSM_IPCPERM_SIZE 7
#define F_OPENBSM_IPCPERM_END (F_OPENBSM_IPCPERM_START+F_OPENBSM_IPCPERM_SIZE)

#define F_OPENBSM_IPORT_START F_OPENBSM_IPCPERM_END
#define F_OPENBSM_IPORT_ADDR 0
#define F_OPENBSM_IPORT_SIZE 1
#define F_OPENBSM_IPORT_END (F_OPENBSM_IPORT_START+F_OPENBSM_IPORT_SIZE)

#define F_OPENBSM_OPAQUE_START F_OPENBSM_IPORT_END
#define F_OPENBSM_OPAQUE_DATA 0
#define F_OPENBSM_OPAQUE_SIZE 1
#define F_OPENBSM_OPAQUE_END (F_OPENBSM_OPAQUE_START+F_OPENBSM_OPAQUE_SIZE)

#define F_OPENBSM_PATH_START F_OPENBSM_OPAQUE_END
#define F_OPENBSM_PATH_DATA 0
#define F_OPENBSM_PATH_SIZE 1
#define F_OPENBSM_PATH_END (F_OPENBSM_PATH_START+F_OPENBSM_PATH_SIZE)

#define F_OPENBSM_PROCESS_START F_OPENBSM_PATH_END
#define F_OPENBSM_PROCESS_AUID 0
#define F_OPENBSM_PROCESS_EUID 1
#define F_OPENBSM_PROCESS_EGID 2
#define F_OPENBSM_PROCESS_RUID 3
#define F_OPENBSM_PROCESS_RGID 4
#define F_OPENBSM_PROCESS_PID 5
#define F_OPENBSM_PROCESS_SID 6
#define F_OPENBSM_PROCESS_TID_PORT 7
#define F_OPENBSM_PROCESS_TID_ADDR 8
#define F_OPENBSM_PROCESS_SIZE 9
#define F_OPENBSM_PROCESS_END (F_OPENBSM_PROCESS_START+F_OPENBSM_PROCESS_SIZE)

#define F_OPENBSM_SUBJECT_START F_OPENBSM_PROCESS_END
#define F_OPENBSM_SUBJECT_AUID 0
#define F_OPENBSM_SUBJECT_EUID 1
#define F_OPENBSM_SUBJECT_EGID 2
#define F_OPENBSM_SUBJECT_RUID 3
#define F_OPENBSM_SUBJECT_RGID 4
#define F_OPENBSM_SUBJECT_PID 5
#define F_OPENBSM_SUBJECT_SID 6
#define F_OPENBSM_SUBJECT_TID_PORT 7
#define F_OPENBSM_SUBJECT_TID_ADDR 8
#define F_OPENBSM_SUBJECT_SIZE 9
#define F_OPENBSM_SUBJECT_END (F_OPENBSM_SUBJECT_START+F_OPENBSM_SUBJECT_SIZE)

#define F_OPENBSM_RETURN_START F_OPENBSM_SUBJECT_END
#define F_OPENBSM_RETURN_ERRNO 0
#define F_OPENBSM_RETURN_VALUE 1
#define F_OPENBSM_RETURN_SIZE 2
#define F_OPENBSM_RETURN_END (F_OPENBSM_RETURN_START+F_OPENBSM_RETURN_SIZE)

#define F_OPENBSM_SEQ_START F_OPENBSM_RETURN_END
#define F_OPENBSM_SEQ_NUM 0
#define F_OPENBSM_SEQ_SIZE 1
#define F_OPENBSM_SEQ_END (F_OPENBSM_SEQ_START+F_OPENBSM_SEQ_SIZE)

#define F_OPENBSM_SOCK_START F_OPENBSM_SEQ_END
#define F_OPENBSM_SOCK_FAMILY 0
#define F_OPENBSM_SOCK_PORT 1
#define F_OPENBSM_SOCK_ADDR 2
#define F_OPENBSM_SOCK_PATH 3
#define F_OPENBSM_SOCK_SIZE 4
#define F_OPENBSM_SOCK_END (F_OPENBSM_SOCK_START+F_OPENBSM_SOCK_SIZE)

#define F_OPENBSM_SOCKET_START F_OPENBSM_SOCK_END
#define F_OPENBSM_SOCKET_DOMAIN 0
#define F_OPENBSM_SOCKET_TYPE 1
#define F_OPENBSM_SOCKET_LPORT 2
#define F_OPENBSM_SOCKET_LADDR 3
#define F_OPENBSM_SOCKET_RPORT 4
#define F_OPENBSM_SOCKET_RADDR 5
#define F_OPENBSM_SOCKET_SIZE 6
#define F_OPENBSM_SOCKET_END (F_OPENBSM_SOCKET_START+F_OPENBSM_SOCKET_SIZE)

#define F_OPENBSM_TEXT_START F_OPENBSM_SOCKET_END
#define F_OPENBSM_TEXT_TEXT 0
#define F_OPENBSM_TEXT_SIZE 1
#define F_OPENBSM_TEXT_END (F_OPENBSM_TEXT_START+F_OPENBSM_TEXT_SIZE)

#define F_OPENBSM_ZONENAME_START F_OPENBSM_TEXT_END
#define F_OPENBSM_ZONENAME_TEXT 0
#define F_OPENBSM_ZONENAME_SIZE 1
#define F_OPENBSM_ZONENAME_END (F_OPENBSM_ZONENAME_START+F_OPENBSM_ZONENAME_SIZE)

#define F_OPENBSM_REGULAR_END F_OPENBSM_ZONENAME_END
// arguments have entries from F_OPENBSM_ARG_START (arg1)
// through F_OPENBSM_ARG_START+F_OPENBSM_MAX_ARGS-1 (arg<F_OPENBSM_MAX_ARGS>)
// no F_OPENBSM field should be added after these
// F_OPENBSM_MAX_ARGS should not be changed
// If really necessary, then the last loop in gen_mod_openbsm.c should
// go from 0 to F_OPENBSM_MAX_ARGS, always.

/*
** Copyright (c) 2014 by Jean GOUBAULT-LARRECQ, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Jean GOUBAULT-LARRECQ <goubault@lsv.ens-cachan.fr>
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
