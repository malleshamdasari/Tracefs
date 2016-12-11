TRFS_VERSION="0.1"

EXTRA_CFLAGS += -DTRFS_VERSION=\"$(TRFS_VERSION)\"

obj-$(CONFIG_TRFS_FS) += trfs.o

trfs-y := dentry.o file.o inode.o main.o super.o lookup.o mmap.o tr_fs.o trfs_msgq.o trfs_ioct.o trfs_ops.o trfs_crc.o
