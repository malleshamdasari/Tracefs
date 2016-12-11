#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h> 
#include <linux/semaphore.h>
#include <linux/cdev.h> 
#include <linux/version.h>
#include "trfs.h"
#include "trfs_ioct.h"
#include "trfs_ops.h"

/* Contains the major number of the character device. */
static int Major;
extern struct trfs_log_driver *tld;

int trfs_ioctl_open(struct inode *inode, struct file *filp)
{
	return 0;
}

int trfs_ioctl_release(struct inode *inode, struct file *filp)
{
	return 0;
}

/** Gives ioctl support for bitmap set/get operations in the kernel
 * param[in] filp File pointer to the opened character device file
 * param[in] cmd Ioctl command to chooose which operation
 * param[in] arg Argument which stores the userspace buffer
 */
long trfs_ioctl_bitmap_op(struct file *filp,
			  unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int err = 0;
	trctl_args *args = (trctl_args *)kmalloc(sizeof(trctl_args),GFP_KERNEL);
	printk("ioctl called \n");
	switch(cmd) {
		case IOCTL_TRFS_SET_BITMAP: 
			err=copy_from_user(args,(void *)arg,sizeof(trctl_args));
			if (err<0) {
				printk("Failed to set bimap for tracing\n");
			}
			tld->bitmap = args->bitmap;
			printk("Bitmap is set for tracing file operations\n");
			break;	
		case IOCTL_TRFS_GET_BITMAP:
			if (tld)
				ret = tld->bitmap;
			break;
	} 
	kfree(args);
 	return ret;
}

struct file_operations bmap_ops = {
	open:   trfs_ioctl_open,
	unlocked_ioctl: trfs_ioctl_bitmap_op,
	release: trfs_ioctl_release
};

struct cdev *kernel_cdev; 

/* Ioctl initilization. 
 */
int trfs_ioctl_init(void)
{
	int err = 0;
	dev_t trfs_device_no, dev;
	
	kernel_cdev = cdev_alloc(); 
	kernel_cdev->ops = &bmap_ops;
	kernel_cdev->owner = THIS_MODULE;

	err = alloc_chrdev_region( &trfs_device_no , 0, 1,"trfs_log_dev");
	if (err < 0) {
		printk(KERN_INFO "Failed to get Major number \n");
	 	goto out;
	}
	
	Major = MAJOR(trfs_device_no);
	dev = MKDEV(Major,0);
	err = cdev_add(kernel_cdev, dev, 1);

	if (err < 0 ) {
		printk(KERN_INFO "cdev allocation: Failed \n");
		goto out;
	}
	printk(KERN_INFO "The major number is %d\n", Major);
out:
	return err;
}

/* Ioctl de-initilization. 
 */
void trfs_ioctl_exit(void)
{
 	cdev_del(kernel_cdev);
	unregister_chrdev_region(Major, 1);
}
