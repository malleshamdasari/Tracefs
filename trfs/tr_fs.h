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

#ifndef _TR_FS_H_
#define _TR_FS_H_

#include <linux/kthread.h>
#include <linux/delay.h>

#define TRFS_PAGE_SIZE 4096

#define TRFS_WRITE(buf, size) \
	if (trfsMqSend(TRFS_LOG_QID, (char *)buf, size, 0, -1) < 0) { \
        	printk("Pushing the record into queue: Failed \n"); \
        	if (buf) \
                	kfree(buf); \
        }

typedef enum trfs_rw_perm_ {
        TRFS_READ_PERM        = 0,
        TRFS_WRITE_PERM       = 1
}trfs_rw_perm;

typedef struct trfs_log_write_ {
	char *tfile_name;
	struct file *tfile;
	char *page;
	int page_size;
	int fthread_exit;
	struct mutex q_lock;
	struct mutex id_lock;
	struct mutex page_lock;
}trfs_log_write;

int trfs_log_write_init(trfs_log_write *t);

typedef int (*thread_cb_func) (void *);

void trfs_log_write_close(void);

unsigned int get_next_record_id(void);

int trfs_kthread_create(struct task_struct *thread,
				 char *thread_name, thread_cb_func cb_func);

int trfs_kthread_exit(struct task_struct *thread);

int trfs_log_write_func(void *p);

/** Function to flush the bytes to tfile periodically. 
 */
void trfs_log_write_flush_periodic(void);

void trfs_log_write_flush(void);

void trfs_logthread_exit(void);

void trfs_fthread_exit(void);

#endif	/* End of _TR_FS_H_ */
