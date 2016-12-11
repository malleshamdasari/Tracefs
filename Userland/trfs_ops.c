#include "trfs_ops.h"

#define MAX_BYTES 4096

extern int gflags;

/** List that contains the chain of operations
 */
trfs_omap_list *omap;

/** Add functionality of linked list
 */
static void trfs_omap_add_op(trfs_omap_list *head, trfs_omap_list *node)
{
	trfs_omap_list *temp = NULL;

	if (!omap) {
		omap = node;
		omap->next = NULL;
		return;
	}

	temp = omap;
	while (temp->next)
		temp = temp->next;
	temp->next = node;
	temp->next->next = NULL;
}

/** delete functionality of linked list
 */
static void trfs_omap_delete_op(trfs_omap_list *head, uint64_t addr, int pid)
{
	trfs_omap_list *temp, *prev;

	if (!omap)
		return;
	if (omap->addr == addr && omap->pid == pid) {
		temp = omap;
		omap = omap->next;
		free(temp);
		temp = NULL;
		return;
	}

	temp = omap;
	while (temp) {
		if (temp->addr == addr && temp->pid == pid) {
			prev->next = temp->next;
			free(temp);
			temp = NULL;
			break;
		}
		prev = temp;
		temp = temp->next;
	}
}

/** Destroy the linked list of mapped operations
 */
static void trfs_omap_destroy_list(trfs_omap_list *head)
{
	trfs_omap_list *temp, *curr;

	temp = omap;
	while (temp) {
		curr = temp;
		temp = temp->next;
		free(curr);
		curr = NULL;
	}
}

/** get the corresponding fd to a given address and pid
 */
static int trfs_omap_getfd(trfs_omap_list *head, uint64_t addr, int pid)
{
	trfs_omap_list *temp = head;

	if (!temp)
		return -1;
	while (temp) {
		if (temp->addr == addr && temp->pid == pid)
			return temp->fd;
		temp = temp->next;
	}
	return -1;
}

/* Tracing mkdir operation
 * @param[in] this structure of trace driver
 */
static void trfs_run_mkdir_op(const struct trfs_rpd *this, trfs_mkdir_op *op)
{
	int ret = 0;

	ret = mkdir(op->pathname, op->mode);
	MAP_RETVAL(ret, op->ret)
}

/* Tracing rmdir operation
 * @param[in] this structure of trace driver
 */
static void trfs_run_rmdir_op(const struct trfs_rpd *this, trfs_rmdir_op *op)
{
	int ret = 0;

	ret = rmdir(op->pathname);
	MAP_RETVAL(ret, op->ret)
}

void trfs_run_link_op(const struct trfs_rpd *this, trfs_link_op *op)
{
	int ret = 0;
	char p1[256], p2[256];

	memcpy(p1, op->pathname, op->plen);
	memcpy(p2, op->pathname+op->plen, op->hlen);
	p1[op->plen] = '\0';
	p2[op->hlen] = '\0';
	ret = link(p1, p2);
	MAP_RETVAL(ret, op->ret)

}

void trfs_run_symlink_op(const struct trfs_rpd *this, trfs_symlink_op *op)
{
	int ret = 0;
	char p1[256], p2[256];

	memcpy(p1, op->pathname, op->plen);
	memcpy(p2, op->pathname+op->plen, op->slen);
	p1[op->plen] = '\0';
	p2[op->slen] = '\0';
	ret = symlink(p2, p1);
	MAP_RETVAL(ret, op->ret)
}

/* Tracing unlink operation
 * @param[in] this structure of trace driver
 */
static void trfs_run_unlink_op(const struct trfs_rpd *this, trfs_unlink_op *op)
{
	int ret = 0;

	ret = unlink(op->pathname);
	MAP_RETVAL(ret, op->ret)
}

static void trfs_run_trunct_op(const struct trfs_rpd *this, trfs_trunct_op *op)
{

}

/* Tracing rename operation
 * @param[in] this structure of trace driver
 */
static void trfs_run_rename_op(const struct trfs_rpd *this, trfs_rename_op *op)
{
	int ret = 0;
	char p1[256], p2[256];

	memcpy(p1, op->pathname1, op->len1);
	memcpy(p2, op->pathname1+op->len1, op->len2);
	p2[op->len2] = '\0';
	ret = rename(p1, p2);
	MAP_RETVAL(ret, op->ret)
}

#define OMAP_OP(a, b, c) \
		trfs_omap_list *node; \
		node = (trfs_omap_list *)malloc(sizeof(trfs_omap_list)); \
		node->fd = a; \
		node->addr = b; \
		node->pid = c; \
		node->next = NULL; \
		trfs_omap_add_op(omap, node);

/* Tracing open operation
 * @param[in] this structure of trace driver
 */
static void trfs_run_open_op(const struct trfs_rpd *this, trfs_open_op *op)
{
	int ret = 0;

	ret = open(op->pathname, op->flags, op->mode);
	OMAP_OP(ret, op->addr, op->pid);
	printf("opened %s file \n",op->pathname);
}

/* Tracing read operation
 * @param[in] this structure of trace driver
 */
static void trfs_run_read_op(const struct trfs_rpd *this, trfs_read_op *op)
{
	int fd = 0;
	int bytes = 0;
	char buf[MAX_BYTES];

	fd = trfs_omap_getfd(omap, op->addr, op->pid);
	bytes = read(fd, buf, op->count);
	MAP_RETVAL(bytes, op->ret)
	printf("reading from %s file \n",op->pathname);
}

/* Tracing write operation
 * @param[in] this structure of trace driver
 */
static void trfs_run_write_op(const struct trfs_rpd *this, trfs_write_op *op)
{
	int fd = 0;
	int bytes = 0;

	fd = trfs_omap_getfd(omap, op->addr, op->pid);
	bytes = write(fd, op->pathname+op->len, op->count);
	MAP_RETVAL(bytes, op->count);
	printf("writing to %s file \n",op->pathname);
}

/* Tracing close operation
 * @param[in] this structure of trace driver
 */
static void trfs_run_close_op(const struct trfs_rpd *this, trfs_close_op *op)
{
	int fd = 0;

	fd = trfs_omap_getfd(omap, op->addr, op->pid);
	trfs_omap_delete_op(omap, op->addr, op->pid);
	close(fd);
	printf("Closing %s file \n",op->pathname);
}

/* Tracing setattr operation
 * @param[in] this structure of trace driver
 */
static void trfs_run_setxattr_op(const struct trfs_rpd *this,
						trfs_setxattr_op *op)
{
	int ret = 0;

	//ret = setxattr(op->pathname);
	MAP_RETVAL(ret, op->ret)
}

/* Tracing setattr operation
 * @param[in] this structure of trace driver
 */
static void trfs_run_getxattr_op(const struct trfs_rpd *this,
						trfs_getxattr_op *op)
{
	int ret = 0;

	//ret = getxattr(op->pathname);
	MAP_RETVAL(ret, op->ret)
}

/* Tracing setattr operation
 * @param[in] this structure of trace driver
 */
static void trfs_run_listxattr_op(const struct trfs_rpd *this,
						trfs_listxattr_op *op)
{
	int ret = 0;

	//ret = listxattr(op->pathname);
	MAP_RETVAL(ret, op->ret)
}

/* Tracing setattr operation
 * @param[in] this structure of trace driver
 */
static void trfs_run_removexattr_op(const struct trfs_rpd *this, 
						trfs_removexattr_op *op)
{
	int ret = 0;

	//ret = listxattr(op->pathname);
	MAP_RETVAL(ret, op->ret)
}

/** Structure to hold the replay operations
 */
static struct trfs_log_ops log_ops = {
	replay_open_op		:	trfs_run_open_op,
	replay_read_op		:	trfs_run_read_op,
	replay_write_op		:	trfs_run_write_op,
	replay_close_op		:	trfs_run_close_op,
	replay_mkdir_op		:	trfs_run_mkdir_op,
	replay_rmdir_op		:	trfs_run_rmdir_op,
	replay_link_op		:	trfs_run_link_op,
	replay_trunct_op	:	trfs_run_trunct_op,
	replay_symlink_op	:	trfs_run_symlink_op,
	replay_unlink_op	:	trfs_run_unlink_op,
	replay_rename_op	:	trfs_run_rename_op,
	replay_setxattr_op	:	trfs_run_setxattr_op,
	replay_getxattr_op	:	trfs_run_getxattr_op,
	replay_listxattr_op	:	trfs_run_listxattr_op,
	replay_removexattr_op	:	trfs_run_removexattr_op
};

/** Initialize the replay structure
 */
struct trfs_rpd *trfs_rpd_init(void)
{
	struct trfs_rpd *rpd = NULL;
		
	omap = NULL;

	rpd = (struct trfs_rpd *)malloc(sizeof(struct trfs_rpd));
	if (rpd)
		rpd->ops = &log_ops;
	return rpd;
}

/** Destroy the mapping list of replay ops
 */
void trfs_rpd_exit(struct trfs_rpd *rpd)
{
	if (rpd)
		free(rpd);
	trfs_omap_destroy_list(omap);
}
