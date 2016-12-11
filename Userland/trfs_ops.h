/*
 * Copyright (c) 1998-2015 Erez Zadok
 * Copyright (c) 2009	   Shrikar Archak
 * Copyright (c) 2003-2015 Stony Brook University
 * Copyright (c) 2003-2015 The Research Foundation of SUNY
 * Copyright (c) 2016	   Mallesham Dasari
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _TRFS_OPS_H_
#define _TRFS_OPS_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "structs.h"

#define MAP_RETVAL(r1, r2) \
		if (r1 == r2) {\
			printf("Success\n"); \
			if (gflags == 1) {\
				printf("Operation can be replayed \n");\
				return;\
			}\
		} \
		else {\
			printf("Failure\n");\
			if (gflags == 2) {\
				printf("Running with -s: aborting\n"); \
				exit(0); \
			} \
		}

typedef enum trfs_ops_ {
	TRFS_OP_CREATE		= 1,
	TRFS_OP_LOOKUP		= 2,
	TRFS_OP_LINK		= 3,
	TRFS_OP_UNLINK		= 4,
	TRFS_OP_SYMLINK		= 5,
	TRFS_OP_MKDIR		= 6,
	TRFS_OP_RMDIR		= 7,
	TRFS_OP_MKNOD		= 8,
	TRFS_OP_TRUNCATE	= 9,
	TRFS_OP_RENAME		= 10,
	TRFS_OP_PERMISSION	= 11,
	TRFS_OP_SETATTR		= 12,
	TRFS_OP_GETATTR		= 13,
	TRFS_OP_SETXATTR	= 14,
	TRFS_OP_GETXATTR	= 15,
	TRFS_OP_LISTXATTR	= 16,
	TRFS_OP_REMOVEXATTR	= 17,
	TRFS_OP_D_REVALIDATE	= 18,
        TRFS_OP_D_RELEASE	= 19,
	TRFS_OP_READ		= 20,
	TRFS_OP_WRITE		= 21,
	TRFS_OP_MMAP		= 22,
	TRFS_OP_OPEN		= 23,
	TRFS_OP_CLOSE		= 24,
	TRFS_OP_FLUSH		= 25,
	TRFS_OP_FSYNC		= 26,
	TRFS_OP_FASYNC		= 27,
	TRFS_OP_READ_ITER	= 28,
	TRFS_OP_WRITE_ITER	= 29,
	TRFS_OP_FILE_RELEASE	= 30
}trfs_ops;

typedef struct trfs_omap_list_ {
	int fd;
	uint64_t addr;
	int pid;
	struct trfs_omap_list_ *next;
}trfs_omap_list;

struct trfs_rpd;

struct trfs_log_ops {
	void (*replay_mkdir_op)(const struct trfs_rpd *this,
					 trfs_mkdir_op *buf);
	void (*replay_rmdir_op)(const struct trfs_rpd *this,
					trfs_rmdir_op *buf);
	void (*replay_link_op)(const struct trfs_rpd *this,
					trfs_link_op *buf);
	void (*replay_symlink_op)(const struct trfs_rpd *this,
					trfs_symlink_op *buf);
	void (*replay_unlink_op)(const struct trfs_rpd *this,
					trfs_unlink_op *buf);
	void (*replay_rename_op)(const struct trfs_rpd *this,
					trfs_rename_op *buf);
	void (*replay_trunct_op)(const struct trfs_rpd *this,
					trfs_trunct_op *buf);
	void (*replay_open_op)(const struct trfs_rpd *this,
					trfs_open_op *buf);
	void (*replay_read_op)(const struct trfs_rpd *this,
					trfs_read_op *buf);
	void (*replay_write_op)(const struct trfs_rpd *this,
					trfs_write_op *buf);
	void (*replay_close_op)(const struct trfs_rpd *this,
					trfs_close_op *buf);
	void (*replay_setxattr_op)(const struct trfs_rpd *this,
					trfs_setxattr_op *buf);
	void (*replay_getxattr_op)(const struct trfs_rpd *this,
					trfs_getxattr_op *buf);
	void (*replay_listxattr_op)(const struct trfs_rpd *this,
					trfs_listxattr_op *buf);
	void (*replay_removexattr_op)(const struct trfs_rpd *this,
					trfs_removexattr_op *buf);
};

struct trfs_rpd {
        struct trfs_log_ops *ops;
};

extern int flags;

struct trfs_rpd* trfs_rpd_init(void);

void trfs_rpd_exit(struct trfs_rpd*);

#endif	/* End of _TRFS_OPS_H_ */
