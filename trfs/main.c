/*
 * Copyright (c) 1998-2015 Erez Zadok
 * Copyright (c) 2009	   Shrikar Archak
 * Copyright (c) 2003-2015 Stony Brook University
 * Copyright (c) 2003-2015 The Research Foundation of SUNY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/parser.h>
#include <linux/string.h>

#include "trfs.h"
#include "tr_fs.h"
#include "trfs_msgq.h"
#include "trfs_ioct.h"
#include "trfs_ops.h"

trfs_log_write tlw;
struct trfs_log_driver *tld = NULL;

typedef enum trfs_tokens_ {
	trfs_filename,
	trfs_opt_err
}trfs_tokens;

typedef struct trfs_options_ {
	char *filename;
	int err;
}trfs_options;

typedef struct trfs_raw_data_ {
	const char *dev_name;
	char *tfile;
}trfs_raw_data;

static const match_table_t tokens = {
	{trfs_filename, "tfile=%s"},
	{trfs_opt_err, NULL}
};

struct task_struct *trfs_kthread;
struct task_struct *trfs_fthread;

trfs_options* trfs_parse_options(char *options)
{
	char *p;
	int len;
	int token;
	trfs_options *t_op;
	substring_t args[MAX_OPT_ARGS];

	t_op = kmalloc(sizeof(trfs_options), GFP_KERNEL);
	if (!t_op) {
		printk("No Memory \n");
		return NULL;	
	}
	
	t_op->err = 0;
	t_op->filename = NULL;

	if (!options) {
                t_op->err = -EINVAL;
                goto out;
        }
        while ((p = strsep(&options, ",")) != NULL) {
                if (!*p)
                        continue;
                token = match_token(p, tokens, args);
		switch (token) {
			case trfs_filename: 
				len = strlen(args[0].from);
				t_op->filename=kmalloc(len+1,GFP_KERNEL);
				strcpy(t_op->filename,args[0].from);
				t_op->err=0;		
				break;
			case trfs_opt_err:
			default:
				t_op->err=-EINVAL;
				printk("Unrecognized option\n");
				break;
		}
	}
out:
	return t_op;	
}

/*
 * There is no need to lock the trfs_super_info's rwsem as there is no
 * way anyone can have a reference to the superblock at this point in time.
 */
static int trfs_read_super(struct super_block *sb, void *raw_data, int silent)
{
	int err = 0;
	struct super_block *lower_sb;
	struct path lower_path;
	const char *dev_name = NULL;
	struct inode *inode;
	trfs_options *t_op = NULL;
	trfs_log_write tlw1;
	trfs_raw_data *rdata = (trfs_raw_data *)raw_data;
	dev_name = rdata->dev_name;

	if (!dev_name) {
		printk(KERN_ERR
		       "trfs: read_super: missing dev_name argument\n");
		err = -EINVAL;
		goto out;
	}

	t_op = trfs_parse_options(rdata->tfile);
	if (!t_op) {
		err = -ENOMEM;
		goto out;
	}
	if (t_op->err!=0 && rdata->tfile!=NULL) {
                printk("Error in Parsing\n");
                err=t_op->err;
                goto out;
        }
	if (trfs_create_log_q() < 0) {
		printk("Message queue creation failed \n");
		err = -EINVAL;
		goto out;
	}
	tlw1.tfile_name = t_op->filename;
	if (trfs_log_write_init(&tlw1) < 0 ) {
		printk("Output write init failed \n");
		err = -EINVAL;
		goto out;
	}

	//trfs_kthread_create(trfs_fthread, "TRFS_FLUSH_THREAD",
	//					&trfs_log_flush_func);
	tld = trfs_log_driver_init();
	trfs_kthread_create(trfs_kthread, "TRFS_LOG_THREAD",
						&trfs_log_write_func);
	if (tld == NULL) {
		printk("No Memory for log driver \n");
		err = -ENOMEM;
		goto out;
	}
	trfs_ioctl_init();
	
	/* parse lower path */
	err = kern_path(dev_name, LOOKUP_FOLLOW | LOOKUP_DIRECTORY,
			&lower_path);
	if (err) {
		printk(KERN_ERR	"trfs: error accessing "
		       "lower directory '%s'\n", dev_name);
		goto out;
	}

	/* allocate superblock private data */
	sb->s_fs_info = kzalloc(sizeof(struct trfs_sb_info), GFP_KERNEL);
	if (!TRFS_SB(sb)) {
		printk(KERN_CRIT "trfs: read_super: out of memory\n");
		err = -ENOMEM;
		goto out_free;
	}

	/* set the lower superblock field of upper superblock */
	lower_sb = lower_path.dentry->d_sb;
	atomic_inc(&lower_sb->s_active);
	trfs_set_lower_super(sb, lower_sb);

	/* inherit maxbytes from lower file system */
	sb->s_maxbytes = lower_sb->s_maxbytes;

	/*
	 * Our c/m/atime granularity is 1 ns because we may stack on file
	 * systems whose granularity is as good.
	 */
	sb->s_time_gran = 1;

	sb->s_op = &trfs_sops;

	sb->s_export_op = &trfs_export_ops; /* adding NFS support */

	/* get a new inode and allocate our root dentry */
	inode = trfs_iget(sb, d_inode(lower_path.dentry));
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto out_sput;
	}
	sb->s_root = d_make_root(inode);
	if (!sb->s_root) {
		err = -ENOMEM;
		goto out_iput;
	}
	d_set_d_op(sb->s_root, &trfs_dops);

	/* link the upper and lower dentries */
	sb->s_root->d_fsdata = NULL;
	err = new_dentry_private_data(sb->s_root);
	if (err)
		goto out_freeroot;

	/* if get here: cannot have error */

	/* set the lower dentries for s_root */
	trfs_set_lower_path(sb->s_root, &lower_path);

	/*
	 * No need to call interpose because we already have a positive
	 * dentry, which was instantiated by d_make_root.  Just need to
	 * d_rehash it.
	 */
	d_rehash(sb->s_root);
	if (!silent)
		printk(KERN_INFO
		       "trfs: mounted on top of %s type %s\n",
		       dev_name, lower_sb->s_type->name);
	goto out; /* all is well */

	/* no longer needed: free_dentry_private_data(sb->s_root); */
out_freeroot:
	dput(sb->s_root);
out_iput:
	iput(inode);
out_sput:
	/* drop refs we took earlier */
	atomic_dec(&lower_sb->s_active);
	kfree(TRFS_SB(sb));
	sb->s_fs_info = NULL;
out_free:
	if (t_op) {
		kfree(t_op);
		t_op = NULL;
	}
	path_put(&lower_path);

out:
	return err;
}

struct dentry *trfs_mount(struct file_system_type *fs_type, int flags,
			    const char *dev_name, void *raw_data)
{
	trfs_raw_data data;
	data.dev_name = dev_name;
	data.tfile= (char*) raw_data;

	return mount_nodev(fs_type, flags, (void *)&data,
			   trfs_read_super);
}

static struct file_system_type trfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= TRFS_NAME,
	.mount		= trfs_mount,
	.kill_sb	= generic_shutdown_super,
	.fs_flags	= 0,
};
MODULE_ALIAS_FS(TRFS_NAME);

static int __init init_trfs_fs(void)
{
	int err;

	pr_info("Registering trfs " TRFS_VERSION "\n");

	err = trfs_init_inode_cache();
	if (err)
		goto out;
	err = trfs_init_dentry_cache();
	if (err)
		goto out;
	err = register_filesystem(&trfs_fs_type);
out:
	if (err) {
		trfs_destroy_inode_cache();
		trfs_destroy_dentry_cache();
	}
	return err;
}

static void __exit exit_trfs_fs(void)
{
	trfs_destroy_inode_cache();
	trfs_destroy_dentry_cache();
	unregister_filesystem(&trfs_fs_type);
	pr_info("Completed trfs module unload\n");
}

MODULE_AUTHOR("Erez Zadok, Filesystems and Storage Lab, Stony Brook University"
	      " (http://www.fsl.cs.sunysb.edu/)");
MODULE_DESCRIPTION("Trfs " TRFS_VERSION
		   " (http://trfs.filesystems.org/)");
MODULE_LICENSE("GPL");

module_init(init_trfs_fs);
module_exit(exit_trfs_fs);
