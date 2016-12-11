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

#include "trfs.h"
#include "tr_fs.h"
#include "trfs_msgq.h"
#include "trfs_ioct.h"
#include "trfs_ops.h"

/*
 * The inode cache is used with alloc_inode for both our inode info and the
 * vfs inode.
 */
static struct kmem_cache *trfs_inode_cachep;
extern struct task_struct *trfs_kthread;
//extern struct task_struct *trfs_fthread;

extern struct trfs_log_driver *tld;

static void trfs_exit(void)
{
	trfs_log_write_flush();
	trfs_log_write_close();
	if (trfs_kthread) 
		trfs_kthread_exit(trfs_kthread);
	//if (trfs_fthread) 
		//trfs_kthread_exit(trfs_fthread);
	trfs_delete_log_q();
	trfs_ioctl_exit();
	trfs_log_driver_exit(tld);
}

/* final actions when unmounting a file system */
static void trfs_put_super(struct super_block *sb)
{
	struct trfs_sb_info *spd;
	struct super_block *s;

	trfs_exit();

	spd = TRFS_SB(sb);
	if (!spd)
		return;

	/* decrement lower super references */
	s = trfs_lower_super(sb);
	trfs_set_lower_super(sb, NULL);
	atomic_dec(&s->s_active);

	kfree(spd);
	sb->s_fs_info = NULL;
}

static int trfs_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	int err;
	struct path lower_path;

	trfs_get_lower_path(dentry, &lower_path);
	err = vfs_statfs(&lower_path, buf);
	trfs_put_lower_path(dentry, &lower_path);

	/* set return buf to our f/s to avoid confusing user-level utils */
	buf->f_type = TRFS_SUPER_MAGIC;

	return err;
}

/*
 * @flags: numeric mount options
 * @options: mount options string
 */
static int trfs_remount_fs(struct super_block *sb, int *flags, char *options)
{
	int err = 0;

	/*
	 * The VFS will take care of "ro" and "rw" flags among others.  We
	 * can safely accept a few flags (RDONLY, MANDLOCK), and honor
	 * SILENT, but anything else left over is an error.
	 */
	if ((*flags & ~(MS_RDONLY | MS_MANDLOCK | MS_SILENT)) != 0) {
		printk(KERN_ERR
		       "trfs: remount flags 0x%x unsupported\n", *flags);
		err = -EINVAL;
	}

	return err;
}

/*
 * Called by iput() when the inode reference count reached zero
 * and the inode is not hashed anywhere.  Used to clear anything
 * that needs to be, before the inode is completely destroyed and put
 * on the inode free list.
 */
static void trfs_evict_inode(struct inode *inode)
{
	struct inode *lower_inode;

	truncate_inode_pages(&inode->i_data, 0);
	clear_inode(inode);
	/*
	 * Decrement a reference to a lower_inode, which was incremented
	 * by our read_inode when it was created initially.
	 */
	lower_inode = trfs_lower_inode(inode);
	trfs_set_lower_inode(inode, NULL);
	iput(lower_inode);
}

static struct inode *trfs_alloc_inode(struct super_block *sb)
{
	struct trfs_inode_info *i;

	i = kmem_cache_alloc(trfs_inode_cachep, GFP_KERNEL);
	if (!i)
		return NULL;

	/* memset everything up to the inode to 0 */
	memset(i, 0, offsetof(struct trfs_inode_info, vfs_inode));

	i->vfs_inode.i_version = 1;
	return &i->vfs_inode;
}

static void trfs_destroy_inode(struct inode *inode)
{
	kmem_cache_free(trfs_inode_cachep, TRFS_I(inode));
}

/* trfs inode cache constructor */
static void init_once(void *obj)
{
	struct trfs_inode_info *i = obj;

	inode_init_once(&i->vfs_inode);
}

int trfs_init_inode_cache(void)
{
	int err = 0;

	trfs_inode_cachep =
		kmem_cache_create("trfs_inode_cache",
				  sizeof(struct trfs_inode_info), 0,
				  SLAB_RECLAIM_ACCOUNT, init_once);
	if (!trfs_inode_cachep)
		err = -ENOMEM;
	return err;
}

/* trfs inode cache destructor */
void trfs_destroy_inode_cache(void)
{
	if (trfs_inode_cachep)
		kmem_cache_destroy(trfs_inode_cachep);
}

/*
 * Used only in nfs, to kill any pending RPC tasks, so that subsequent
 * code can actually succeed and won't leave tasks that need handling.
 */
static void trfs_umount_begin(struct super_block *sb)
{
	struct super_block *lower_sb;

	lower_sb = trfs_lower_super(sb);
	if (lower_sb && lower_sb->s_op && lower_sb->s_op->umount_begin)
		lower_sb->s_op->umount_begin(lower_sb);
}

const struct super_operations trfs_sops = {
	.put_super	= trfs_put_super,
	.statfs		= trfs_statfs,
	.remount_fs	= trfs_remount_fs,
	.evict_inode	= trfs_evict_inode,
	.umount_begin	= trfs_umount_begin,
	.show_options	= generic_show_options,
	.alloc_inode	= trfs_alloc_inode,
	.destroy_inode	= trfs_destroy_inode,
	.drop_inode	= generic_delete_inode,
};

/* NFS support */

static struct inode *trfs_nfs_get_inode(struct super_block *sb, u64 ino,
					  u32 generation)
{
	struct super_block *lower_sb;
	struct inode *inode;
	struct inode *lower_inode;

	lower_sb = trfs_lower_super(sb);
	lower_inode = ilookup(lower_sb, ino);
	inode = trfs_iget(sb, lower_inode);
	return inode;
}

static struct dentry *trfs_fh_to_dentry(struct super_block *sb,
					  struct fid *fid, int fh_len,
					  int fh_type)
{
	return generic_fh_to_dentry(sb, fid, fh_len, fh_type,
				    trfs_nfs_get_inode);
}

static struct dentry *trfs_fh_to_parent(struct super_block *sb,
					  struct fid *fid, int fh_len,
					  int fh_type)
{
	return generic_fh_to_parent(sb, fid, fh_len, fh_type,
				    trfs_nfs_get_inode);
}

/*
 * all other funcs are default as defined in exportfs/expfs.c
 */

const struct export_operations trfs_export_ops = {
	.fh_to_dentry	   = trfs_fh_to_dentry,
	.fh_to_parent	   = trfs_fh_to_parent
};
