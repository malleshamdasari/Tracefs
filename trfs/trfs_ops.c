#include "trfs.h"
#include "tr_fs.h"
#include "trfs_ops.h"
#include "trfs_msgq.h"

/* To test the given bit set or not
 */
static int test_bit_set(unsigned int a, int n)
{
	return ((1 << n)&a);
}

/* Check if the operation is enabled for tracing */
#define IS_TRACE_ENABLED(a, n)	\
		if (test_bit_set(a, n) == 0) \
			return;  

/* Tracing mkdir operation
 * @param[in] this structure of trace driver
 * @param[in] dir Pointer to inode
 * @param[in] dentry Pointer to dentry
 * @param[in] mode file object mode
 * @param[in] ret return value
 * */
static void trfs_log_mkdir_op(struct trfs_log_driver *this, 
		struct inode *dir, struct dentry *dentry, int mode, int ret)
{
	int size = 0;
	int path_len = 0;
	trfs_mkdir_op *mop = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_MKDIR)

	path_len = dentry->d_name.len;
	size = sizeof(trfs_mkdir_op)+path_len;

	mop = (trfs_mkdir_op *)kmalloc(size, GFP_KERNEL);
	if (mop) {
		mop->r_id = get_next_record_id();
		mop->r_type = TRFS_OP_MKDIR;
		mop->r_size = size;
		mop->mode = mode;
		mop->len = path_len;
		mop->ret = ret;
		strcpy(mop->pathname, dentry->d_name.name);
	}
	TRFS_WRITE(mop, size);
}

/* Tracing rmdir operation
 * @param[in] this structure of trace driver
 * @param[in] dir Pointer to inode
 * @param[in] dentry Pointer to dentry
 * @param[in] ret return value
 * */
static void trfs_log_rmdir_op(struct trfs_log_driver *this,
			struct inode *dir, struct dentry *dentry, int ret)
{
	int size = 0;
	int path_len = 0;
	trfs_rmdir_op *rop = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_RMDIR)

	path_len = dentry->d_name.len;
	size = sizeof(trfs_rmdir_op)+path_len;

	rop = (trfs_rmdir_op *)kmalloc(size, GFP_KERNEL);
	if (rop) {
		rop->r_id = get_next_record_id();
		rop->r_type = TRFS_OP_RMDIR;
		rop->r_size = size;
		rop->len = path_len;
		rop->ret = ret;
		strcpy(rop->pathname, dentry->d_name.name);
	}
	TRFS_WRITE(rop, size);
}

/* Tracing unlink operation
 * @param[in] this structure of trace driver
 * @param[in] dir Pointer to inode
 * @param[in] dentry Pointer to dentry
 * @param[in] ret return value
 * */
static void trfs_log_unlink_op(struct trfs_log_driver *this, 
			struct inode *dir, struct dentry *dentry, int ret)
{
	int size = 0;
	int path_len = 0;
	trfs_unlink_op *op = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_UNLINK)

	path_len = dentry->d_name.len;
	size = sizeof(trfs_unlink_op)+path_len;

	op = (trfs_unlink_op *)kmalloc(size, GFP_KERNEL);
	if (op) {
		op->r_id = get_next_record_id();
		op->r_type = TRFS_OP_UNLINK;
		op->r_size = size;
		op->len = path_len;
		op->ret = ret;
		strcpy(op->pathname, dentry->d_name.name);
	}
	TRFS_WRITE(op, size);
}

/* Tracing link operation
 * @param[in] this structure of trace driver
 * @param[in] dir Pointer to inode
 * @param[in] old_dentry Pointer to old dentry
 * @param[in] new_dentry Pointer to new dentry
 * @param[in] ret return value
 * */
static void trfs_log_link_op(struct trfs_log_driver *this, 
		struct dentry *old_dentry, struct inode *dir,
					struct dentry *new_dentry, int ret)
{
        int size = 0;
        int path_len1 = 0;
        int path_len2 = 0;
        trfs_link_op *op = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_LINK)

        path_len1 = old_dentry->d_name.len;
        path_len2 = old_dentry->d_name.len;
        size = sizeof(trfs_link_op)+path_len1+path_len2;

        op = (trfs_link_op *)kmalloc(size, GFP_KERNEL);
        if (op) {
                op->r_id = get_next_record_id();
                op->r_type = TRFS_OP_LINK;
                op->r_size = size;
                op->plen = path_len1;
                op->hlen = path_len2;
                op->ret = ret;
                strcpy(op->pathname, old_dentry->d_name.name);
                strcpy(op->pathname+path_len1, new_dentry->d_name.name);
        }
        TRFS_WRITE(op, size);
}

/* Tracing symlink operation
 * @param[in] this structure of trace driver
 * @param[in] dir Pointer to inode
 * @param[in] dentry Pointer to dentry
 * @param[in] sname Pointer to symlink name
 * @param[in] ret return value
 * */
static void trfs_log_symlink_op(struct trfs_log_driver *this, 
			struct inode *dir, struct dentry *dentry, 
						const char *sname, int ret)
{
        int size = 0;
        int path_len = 0;
        trfs_symlink_op *op = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_SYMLINK)

        path_len = dentry->d_name.len;
        size = sizeof(trfs_symlink_op)+path_len+strlen(sname);

        op = (trfs_symlink_op *)kmalloc(size, GFP_KERNEL);
        if (op) {
                op->r_id = get_next_record_id();
                op->r_type = TRFS_OP_SYMLINK;
                op->r_size = size;
                op->plen = path_len;
                op->slen = strlen(sname);
                op->ret = ret;
                strcpy(op->pathname, dentry->d_name.name);
                strcpy(op->pathname+path_len, sname);
        }
        TRFS_WRITE(op, size);
}

/* Tracing rename operation
 * @param[in] this structure of trace driver
 * @param[in] old_dir Pointer to old inode
 * @param[in] old_dentry Pointer to old dentry
 * @param[in] new_dir Pointer to old inode
 * @param[in] new_dentry Pointer to old dentry
 * @param[in] ret return value
 * */
static void trfs_log_rename_op(struct trfs_log_driver *this, 
		struct inode *old_dir, struct dentry *old_dentry, 
		struct inode *new_dir, struct dentry *new_dentry, int ret)
{
	int size = 0;
	int path_len1 = 0;
	int path_len2 = 0;
	trfs_rename_op *rop = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_RENAME)

	path_len1 = old_dentry->d_name.len;
	path_len2 = new_dentry->d_name.len;
	size = sizeof(trfs_rename_op)+path_len1+path_len2;

	rop = (trfs_rename_op *)kmalloc(size, GFP_KERNEL);
	if (rop) {
		rop->r_id = get_next_record_id();
		rop->r_type = TRFS_OP_RENAME;
		rop->r_size = size;
		rop->len1 = path_len1;
		rop->len2 = path_len2;
		rop->ret = ret;
		strcpy(rop->pathname1, old_dentry->d_name.name);
		strcpy(rop->pathname1+path_len1, new_dentry->d_name.name);
	}
	TRFS_WRITE(rop, size);
}

/* Tracing open operation
 * @param[in] this structure of trace driver
 * @param[in] dir Pointer to inode
 * @param[in] dentry Pointer to dentry
 * @param[in] ret return value
 * */
static void trfs_log_open_op(struct trfs_log_driver *this, 
				struct inode *dir, struct file *file, int ret)
{
	int size = 0;
	int path_len = 0;
	trfs_open_op *op = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_OPEN)

	path_len = file->f_path.dentry->d_name.len;
	size = sizeof(trfs_open_op)+path_len;

	op = (trfs_open_op *)kmalloc(size, GFP_KERNEL);
	if (op) {
		op->r_id = get_next_record_id();
		op->r_type = TRFS_OP_OPEN;
		op->r_size = size;
		op->pid = (int) task_pid_nr(current);
		op->flags = file->f_flags;
		op->mode = file->f_mode;
		op->len = path_len;
		op->ret = ret;
		memcpy((char *)&op->addr, (char *)&file, sizeof(struct file *));
		strcpy(op->pathname, file->f_path.dentry->d_name.name);
	}
	TRFS_WRITE(op, size);
}

/* Tracing read operation
 * @param[in] this structure of trace driver
 * @param[in] dir Pointer to inode
 * @param[in] dentry Pointer to dentry
 * @param[in] ret return value
 * */
static void trfs_log_read_op(struct trfs_log_driver *this, 
			struct file *file, size_t count, loff_t *ppos, int ret)
{
	int size = 0;
	int path_len = 0;
	trfs_read_op *op = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_READ)

	path_len = file->f_path.dentry->d_name.len;
	size = sizeof(trfs_read_op)+path_len;

	op = (trfs_read_op *)kmalloc(size, GFP_KERNEL);
	if (op) {
		op->r_id = get_next_record_id();
		op->r_type = TRFS_OP_READ;
		op->r_size = size;
		op->pid = (int) task_pid_nr(current);
		op->len = path_len;
		op->count = count;
		op->ppos = *ppos;
		op->ret = ret;
		memcpy((char *)&op->addr, (char *)&file, sizeof(struct file *));
		strcpy(op->pathname, file->f_path.dentry->d_name.name);
	}
	TRFS_WRITE(op, size)
}

/* Tracing write operation
 * @param[in] this structure of trace driver
 * @param[in] dir Pointer to inode
 * @param[in] dentry Pointer to dentry
 * @param[in] ret return value
 * */
static void trfs_log_write_op(struct trfs_log_driver *this, 
       struct file *file, const char *buf, size_t count, loff_t *ppos, int ret)
{
	int size = 0;
	int path_len = 0;
	trfs_write_op *op = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_WRITE)

	path_len = file->f_path.dentry->d_name.len;
	size = sizeof(trfs_write_op)+path_len+count;

	op = (trfs_write_op *)kmalloc(size, GFP_KERNEL);
	if (op) {
		op->r_id = get_next_record_id();
		op->r_type = TRFS_OP_WRITE;
		op->r_size = size;
		op->pid = (int) task_pid_nr(current);
		op->len = path_len;
		op->count = count;
		op->ppos = *ppos;
		op->ret = ret;
		memcpy((char *)&op->addr, (char *)&file, sizeof(struct file *));
		memcpy(op->pathname, file->f_path.dentry->d_name.name, path_len);
		memcpy(op->pathname+path_len, buf, count);
	}
	TRFS_WRITE(op, size)
}

/* Tracing close operation
 * @param[in] this structure of trace driver
 * @param[in] dir Pointer to inode
 * @param[in] dentry Pointer to dentry
 * @param[in] ret return value
 * */
static void trfs_log_close_op(struct trfs_log_driver *this, 
				struct inode *dir, struct file *file, int ret)
{
	int size = 0;
	int path_len = 0;
	trfs_close_op *op = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_CLOSE)

	path_len = file->f_path.dentry->d_name.len;
	size = sizeof(trfs_close_op)+path_len;

	op = (trfs_close_op *)kmalloc(size, GFP_KERNEL);
	if (op) {
		op->r_id = get_next_record_id();
		op->r_type = TRFS_OP_CLOSE;
		op->r_size = size;
		op->pid = (int) task_pid_nr(current);
		op->len = path_len;
		op->ret = ret;
		memcpy((char *)&op->addr, (char *)&file, sizeof(struct file *));
		memcpy(op->pathname, file->f_path.dentry->d_name.name, op->len);
	}
	TRFS_WRITE(op, size);
}

/* Tracing mknod operation
 * @param[in] this structure of trace driver
 * @param[in] dir Pointer to inode
 * @param[in] dentry Pointer to dentry
 * @param[in] ret return value
 */
static void trfs_log_mknod_op(struct trfs_log_driver *this, struct inode *dir, 
		struct dentry *dentry, mode_t mode, dev_t dev, int ret)
{
	int size = 0;
	int path_len = 0;
	trfs_mknod_op *op = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_CLOSE)

	path_len = dentry->d_name.len;
	size = sizeof(trfs_mknod_op)+path_len;

	op = (trfs_mknod_op *)kmalloc(size, GFP_KERNEL);
	if (op) {
		op->r_id = get_next_record_id();
		op->r_type = TRFS_OP_CLOSE;
		op->r_size = size;
		op->pid = (int) task_pid_nr(current);
		op->len = path_len;
		op->ret = ret;
		memcpy(op->pathname, dentry->d_name.name, op->len);
	}
	TRFS_WRITE(op, size);
}

/* Tracing setxattr operation
 * @param[in] this structure of trace driver
 * @param[in] dentry Pointer to dentry
 * @param[in] ret return value
 */
static void trfs_log_setxattr_op(struct trfs_log_driver *this,
		struct dentry *dentry, const char *name, const void *value,
					size_t size1, int flags, int ret)
{
	int size = 0;
	int path_len = 0;
	trfs_setxattr_op *op = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_SETXATTR)

	path_len = dentry->d_name.len;
	size = sizeof(trfs_setxattr_op)+path_len;

	op = (trfs_setxattr_op *)kmalloc(size, GFP_KERNEL);
	if (op) {
		op->r_id = get_next_record_id();
		op->r_type = TRFS_OP_SETXATTR;
		op->r_size = size;
		op->pid = (int) task_pid_nr(current);
		op->len = path_len;
		op->ret = ret;
		memcpy(op->pathname, dentry->d_name.name, op->len);
	}
	TRFS_WRITE(op, size);
}

/* Tracing getxattr operation
 * @param[in] this structure of trace driver
 * @param[in] dentry Pointer to dentry
 * @param[in] ret return value
 */
static void trfs_log_getxattr_op(struct trfs_log_driver *this, 
		struct dentry *dentry, const char *name, void *buffer,
							size_t size1, int ret)
{
	int size = 0;
	int path_len = 0;
	trfs_getxattr_op *op = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_GETXATTR)

	path_len = dentry->d_name.len;
	size = sizeof(trfs_getxattr_op)+path_len;

	op = (trfs_getxattr_op *)kmalloc(size, GFP_KERNEL);
	if (op) {
		op->r_id = get_next_record_id();
		op->r_type = TRFS_OP_GETXATTR;
		op->r_size = size;
		op->pid = (int) task_pid_nr(current);
		op->len = path_len;
		op->ret = ret;
		memcpy(op->pathname, dentry->d_name.name, op->len);
	}
	TRFS_WRITE(op, size);
}

/* Tracing listxattr operation
 * @param[in] this structure of trace driver
 * @param[in] dentry Pointer to dentry
 * @param[in] ret return value
 */
static void trfs_log_listxattr_op(struct trfs_log_driver *this, 
			struct dentry *dentry,
 				char *buffer, size_t buffer_size, int ret)
{
	int size = 0;
	int path_len = 0;
	trfs_listxattr_op *op = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_LISTXATTR)

	path_len = dentry->d_name.len;
	size = sizeof(trfs_listxattr_op)+path_len;

	op = (trfs_listxattr_op *)kmalloc(size, GFP_KERNEL);
	if (op) {
		op->r_id = get_next_record_id();
		op->r_type = TRFS_OP_LISTXATTR;
		op->r_size = size;
		op->pid = (int) task_pid_nr(current);
		op->len = path_len;
		op->ret = ret;
		memcpy(op->pathname, dentry->d_name.name, op->len);
	}
	TRFS_WRITE(op, size);
}

/* Tracing removexattr operation
 * @param[in] this structure of trace driver
 * @param[in] dentry Pointer to dentry
 * @param[in] ret return value
 */
static void trfs_log_removexattr_op(struct trfs_log_driver *this,
			struct dentry *dentry, const char *name, int ret)
{
	int size = 0;
	int path_len = 0;
	trfs_removexattr_op *op = NULL;

	IS_TRACE_ENABLED(this->bitmap, TRFS_OP_REMOVEXATTR)

	path_len = dentry->d_name.len;
	size = sizeof(trfs_removexattr_op)+path_len;

	op = (trfs_removexattr_op *)kmalloc(size, GFP_KERNEL);
	if (op) {
		op->r_id = get_next_record_id();
		op->r_type = TRFS_OP_REMOVEXATTR;
		op->r_size = size;
		op->pid = (int) task_pid_nr(current);
		op->len = path_len;
		op->ret = ret;
		memcpy(op->pathname, dentry->d_name.name, op->len);
	}
	TRFS_WRITE(op, size);
}

/** Structure for file trace operations
 */
static struct trfs_log_ops log_ops = {
	trace_open_op		:	trfs_log_open_op,
	trace_read_op		:	trfs_log_read_op,
	trace_write_op		:	trfs_log_write_op,
	trace_close_op		:	trfs_log_close_op,
	trace_mkdir_op		:	trfs_log_mkdir_op,
	trace_rmdir_op		:	trfs_log_rmdir_op,
	trace_unlink_op		:	trfs_log_unlink_op,
	trace_link_op		:	trfs_log_link_op,
	trace_symlink_op	:	trfs_log_symlink_op,
	trace_mknod_op		:	trfs_log_mknod_op,
	trace_rename_op		:	trfs_log_rename_op,
	trace_setxattr_op	:	trfs_log_setxattr_op,
	trace_getxattr_op	:	trfs_log_getxattr_op,
	trace_listxattr_op	:	trfs_log_listxattr_op,
	trace_removexattr_op	:	trfs_log_removexattr_op
};

/** Initializes output operations structure
 */
struct trfs_log_driver *trfs_log_driver_init(void) 
{
	struct trfs_log_driver *tld = NULL;

	tld = (struct trfs_log_driver *)kmalloc(
				sizeof(struct trfs_log_driver), GFP_KERNEL);
	if (tld) {
		tld->ops = &log_ops;
		tld->bitmap = ~(tld->bitmap&0);
	}
	return tld;
}

/** Uninitializes the output ops structure
 */
void trfs_log_driver_exit(struct trfs_log_driver *tld) 
{
	if (tld)
		kfree(tld);	
}
