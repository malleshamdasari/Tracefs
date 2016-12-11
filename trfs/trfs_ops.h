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

#include <linux/sched.h>

#include "structs.h"

typedef enum trfs_ops_ {
	TRFS_OP_CREATE		= 1,
	TRFS_OP_LOOKUP		= 2,
	TRFS_OP_LINK		= 3,
	TRFS_OP_UNLINK		= 4,
	TRFS_OP_SYMLINK		= 5,
	TRFS_OP_MKDIR		= 6,
	TRFS_OP_RMDIR		= 7,
	TRFS_OP_MKNOD		= 8,	/* inode operations */
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
	TRFS_OP_OPEN		= 23,	/* file operations*/
	TRFS_OP_CLOSE		= 24,
	TRFS_OP_FLUSH		= 25,
	TRFS_OP_FSYNC		= 26,
	TRFS_OP_FASYNC		= 27,
	TRFS_OP_READ_ITER	= 28,
	TRFS_OP_WRITE_ITER	= 29,
	TRFS_OP_FILE_RELEASE	= 30
}trfs_ops;

typedef enum trfs_arg_id_ {
	TRFS_PID	= 0,
	TRFS_UID	= 1
}trfs_arg_id;

struct trfs_log_driver;

struct trfs_log_ops {
	void (*trace_open_op)(struct trfs_log_driver *this, 
				struct inode *dir, struct file *file, int ret);
	void (*trace_read_op)(struct trfs_log_driver *this,
		       struct file *file, size_t count, loff_t *ppos, int ret);
	void (*trace_write_op)(struct trfs_log_driver *this, struct file *file,
			 const char *buf, size_t count, loff_t *ppos, int ret);
	void (*trace_close_op)(struct trfs_log_driver *this,
				struct inode *dir, struct file *file, int ret);
	void (*trace_mkdir_op)(struct trfs_log_driver *this,
	      struct inode *dir, struct dentry *dentry, int mode, int ret);
	void (*trace_rmdir_op)(struct trfs_log_driver *this,
			struct inode *dir, struct dentry *dentry, int ret);
	void (*trace_unlink_op)(struct trfs_log_driver *this, 
			struct inode *dir, struct dentry *dentry, int ret);
	void (*trace_link_op)(struct trfs_log_driver *this, 
		struct dentry *old_dentry, struct inode *dir,
					  struct dentry *new_dentry, int ret);
	void (*trace_symlink_op)(struct trfs_log_driver *this,
		 		struct inode *dir, struct dentry *dentry,
						   const char *sname, int ret);
	void (*trace_rename_op)(struct trfs_log_driver *this, 
		struct inode *old_dir, struct dentry *old_dentry, 
		struct inode *new_dir, struct dentry *new_dentry, int ret);
	void (*trace_mknod_op)(struct trfs_log_driver *this, struct inode *dir, 
		struct dentry *dentry, mode_t mode, dev_t dev, int ret);
	void (*trace_setxattr_op)(struct trfs_log_driver *this,
		struct dentry *dentry, const char *name, const void *value,
					size_t size, int flags, int ret);
	void (*trace_getxattr_op)(struct trfs_log_driver *this, 
		struct dentry *dentry, const char *name, void *buffer,
							size_t size, int ret);
	void (*trace_listxattr_op)(struct trfs_log_driver *this, struct dentry *dentry,
				char *buffer, size_t buffer_size, int ret);
	void (*trace_removexattr_op)(struct trfs_log_driver *this,
			struct dentry *dentry, const char *name, int ret);
};

struct trfs_log_driver {
	int pid;
	unsigned int bitmap;
	struct trfs_log_ops *ops;
};

struct trfs_log_driver* trfs_log_driver_init(void);

void trfs_log_driver_exit(struct trfs_log_driver*);

#endif	/* End of _TRFS_OPS_H_ */
