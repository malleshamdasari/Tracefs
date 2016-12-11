#include <linux/ioctl.h>

#ifndef __TRFS_IOCTL_BITMAP__
#define __TRFS_IOCTL_BITMAP__

#define IOC_MAGIC 'k'
#define IOCTL_TRFS_SET_BITMAP _IO(IOC_MAGIC,0) 
#define IOCTL_TRFS_GET_BITMAP _IO(IOC_MAGIC,1) 

typedef struct trctl_args_ {
        int bitmap;
}trctl_args;

int trfs_ioctl_init(void);

void trfs_ioctl_exit(void);

#endif
