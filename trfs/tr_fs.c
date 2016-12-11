/*
 * Copyright (c) 1998-2015 Erez Zadok
 * Copyright (c) 2009	   Shrikar Archak
 * Copyright (c) 2003-2015 Stony Brook University
 * Copyright (c) 2003-2015 The Research Foundation of SUNY
 * Copyright (c) 2016 	   Mallesham Dasari
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kthread.h>

#include "trfs.h"
#include "tr_fs.h"
#include "trfs_msgq.h"

/** Global stucture for async writing 
 */
extern trfs_log_write tlw;

/** Generates and returns a new id for each record
 */
unsigned int get_next_record_id( )
{
	static unsigned int id = 1;

	mutex_lock(&tlw.id_lock);
	id++;
	mutex_unlock(&tlw.id_lock);
	return id;
}

/** Function to validate the newly opened/created file.
 * @param[in] filp File pointer given for verification.
 * @param[in] flags Read and write permissions file.
 * @return: ret - 0: On success
 * 		- -ve: On failure 
 */
static int trfs_file_verify(struct file* filp, trfs_rw_perm flags)
{
        int ret = 0;

	/* Check if the file opened successfully or not. */
	if (!filp) {
		printk(KERN_INFO "Opening file: Failed. \n");
		ret = -EBADF;
		goto out;
	}

	/* Check if there is an error in file pointer. */
	if (IS_ERR(filp)) {
                printk(KERN_INFO "File error : %d \n", (int) PTR_ERR(filp));
		ret = PTR_ERR(filp);
		goto out;
	}

	/* Check if the file is regular or not. */
        if ((!S_ISREG(file_inode(filp)->i_mode))) {
                printk(KERN_INFO "Checking if the file is regular: Failed. \n");
		ret = -EIO;
                goto out; 
        }

	/* Check read permissions for input files. */
	if (flags == TRFS_READ_PERM) {
		if (!(filp->f_mode & FMODE_READ)) {
			printk(KERN_INFO "Input file for read mode: Failed. \n");
			ret = -EIO;
			goto out;
		}
	}

        /* Check write permissions for the output file. */
	if (flags == TRFS_WRITE_PERM) {
		if (!(filp->f_mode & FMODE_WRITE)) {
			printk(KERN_INFO "Output file for write mode: Failed. \n");
			ret = -EIO;
			goto out;
		}
	}
out:	
	return ret;
}

/** Initializes the trace file writing 
 * param[in] t Global stucture for async writing 
 */
int trfs_log_write_init(trfs_log_write *t)
{
	int err = 0;

	tlw.tfile_name = t->tfile_name;
	if (tlw.tfile_name == NULL) {
		printk("Output file is null \n");
		err = -ENOENT;
		goto out;
	}

	tlw.tfile = filp_open(tlw.tfile_name, O_WRONLY|O_CREAT|O_TRUNC, 0777);
        err = trfs_file_verify(tlw.tfile, TRFS_WRITE_PERM);
	tlw.tfile->f_pos = 0;
        mutex_init(&tlw.q_lock); 
        mutex_init(&tlw.id_lock); 
out:
	return err;
}

/** Function to write the file.
 * param[in] filp File pointer to output file.
 * param[in] data Buffer from the data is read and wrote to file.
 * param[in] size Number of bytes to be written.
 */
static int trfs_file_write(struct file* filp,
				 unsigned char* data, unsigned int size)
{
	int bytes = 0;
	mm_segment_t oldfs;

        /* File pointer verification. */
        if (!filp) {
                printk(KERN_INFO "File Pointer error. \n");
                bytes = -EACCES;
                goto out;
        }

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	
	/* Write into the file byte by byte. */	
	bytes = vfs_write(filp, data, size, &filp->f_pos); 

	set_fs(oldfs);

        if ((bytes == 0) || (bytes < 0)) {
                bytes = -EBADF;
                goto out;
        }
out:
	return bytes;
}

/** Function to close the file. 
 * param[in] filp File pointer to be closed.
 */
static void trfs_file_close(struct file* filp)
{
	if (filp != NULL)
		if (!IS_ERR(filp))
			filp_close(filp, NULL);
}

/** Function to flush the remaining bytes to tfile. 
 */
void trfs_log_write_flush(void)
{
	trfs_file_write(tlw.tfile, (char *)tlw.page, tlw.page_size);
}

/** Function to flush the bytes to tfile periodically. 
 */
void trfs_log_write_flush_periodic(void)
{
	while(1) {
		trfs_file_write(tlw.tfile, (char *)tlw.page, tlw.page_size);
		tlw.page_size = 0;
		msleep(10000);
	}
}

/** Async record writing thread
 * It contains the efficient queue handling
 * Blocks until there is a message in the queue
 * Takes only a pointer of the message
 */
int trfs_log_write_func(void *p)
{
	int err = 0;
    	int *msg1 = NULL, *msg2 = NULL, len=1;
	char *buf = NULL;
	
	tlw.page = (char *)kmalloc(TRFS_PAGE_SIZE, GFP_KERNEL);
	buf = tlw.page;
	tlw.page_size = 0;
	
    	while(1)
    	{
    	    	msg1 = kmalloc(sizeof(int), GFP_KERNEL);
	    	msg2 = msg1;
    	    	if(trfsMqRecv(TRFS_LOG_QID, &msg1, &len, 0, -1) < 0 )
    	    	{
    	    	   	printk("before uuMqRecv failure\n");
	    	    	err = -EINVAL;
	    	    	goto out;
    	    	}
    	 	else {
			/* End of the file system tracing */
			if(*msg1 == TRFS_LOG_COMPLETE) {
    	            		printk("Exiting: Rx thread \n");
    	            		break; 
    	        	}
    	        	else
    	        	{
				if (tlw.page_size+len>TRFS_PAGE_SIZE) {
    	        	    		trfs_file_write(tlw.tfile,
					    (char *)tlw.page, tlw.page_size);
					buf = tlw.page;
					tlw.page_size = 0;
				}
				memcpy(buf, (char *)msg1, len);
				buf += len;
			        tlw.page_size += len;
    	        	    	if (msg1) {
    	        	    	   kfree(msg1);
			    	   msg1 = NULL;
    	        	    	}
    	        	    	if (msg2) {
    	        	    	   kfree(msg2);
			    	   msg2 = NULL;
    	        	    	}
    	        	} 
    	    	}
    	}
out:
	if (tlw.page) {
		kfree(tlw.page);
		tlw.page = NULL;
	}
    	if (msg1) {
    	   	kfree(msg1);
		msg1 = NULL;
    	}
    	if (msg2) {
    	   	kfree(msg2);
		msg2 = NULL;
    	}
	return err;
}

/* Tracing over, close the file
 */
void trfs_log_write_close(void)
{
	trfs_file_close(tlw.tfile);
	if (tlw.tfile_name) {
		kfree(tlw.tfile_name);
		tlw.tfile_name = NULL;
	}
}

/* kthread is initialized in this function
 * Async callback is started
 */
int trfs_kthread_create(struct task_struct *thread,
				 char *thread_name, thread_cb_func cb_func)
{
	thread = kthread_create( cb_func, NULL, thread_name );
	if (thread) {
        	wake_up_process( thread );
    	}
    	return 0;
}

/** kthread exit handling while unmouting
 */
int trfs_kthread_exit(struct task_struct *thread)
{
	char exit_msg = 0;
	char *p;
	
	p = &exit_msg;

        TRFS_WRITE(p, sizeof(char *));
   	return kthread_stop(thread);
}
