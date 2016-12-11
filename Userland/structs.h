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

#ifndef _TRFS_STRUCTS_H_
#define _TRFS_STRUCTS_H_

#include <stdint.h>

typedef struct trfs_open_op_ {
        unsigned int r_id;
        unsigned short r_size;
        char r_type;
        int flags;
        mode_t mode;
        unsigned int pid;
        uint64_t addr;
        int ret;
        unsigned short len;
        char pathname[1];
}trfs_open_op;

typedef struct trfs_read_op_ {
        unsigned int r_id;
        unsigned short r_size;
        char r_type;
        unsigned int pid;
        uint64_t addr;
	size_t count;
	int ppos;
        int ret;
        unsigned short len;
        char pathname[1];
}trfs_read_op;

typedef struct trfs_write_op_ {
        unsigned int r_id;
        unsigned short r_size;
        char r_type;
        unsigned int pid;
        uint64_t addr;
	size_t count;
	int ppos;
        int ret;
        unsigned short len;
        char pathname[1];
}trfs_write_op;

typedef struct trfs_close_op_ {
        unsigned int r_id;
        unsigned short r_size;
        char r_type;
        unsigned int pid;
        uint64_t addr;
        int ret;
        unsigned short len;
        char pathname[1];
}trfs_close_op;

typedef struct trfs_mkdir_op_ {
        unsigned int r_id;
        unsigned short r_size;
        char r_type;
        unsigned int pid;
        int mode;
        int ret;
        unsigned short len;
        char pathname[1];
}trfs_mkdir_op;

typedef struct trfs_rmdir_op_ {
        unsigned int r_id;
        unsigned short r_size;
        char r_type;
        unsigned int pid;
        int ret;
        unsigned short len;
        char pathname[1];
}trfs_rmdir_op;

typedef struct trfs_unlink_op_ {
        unsigned int r_id;
        unsigned short r_size;
        char r_type;
        unsigned int pid;
        int ret;
        unsigned short len;
        char pathname[1];
}trfs_unlink_op;

typedef struct trfs_rename_op_ {
        unsigned int r_id;
        unsigned short r_size;
        char r_type;
        unsigned int pid;
        int ret;
        unsigned short len1;
        unsigned short len2;
        char pathname1[1];
        char pathname2[1];
}trfs_rename_op;

typedef struct trfs_link_op_ {
        unsigned int r_id;
        unsigned short r_size;
        char r_type;
        unsigned int pid;
        int ret;
        unsigned short plen;
        unsigned short hlen;
        char pathname[1];
}trfs_link_op;

typedef struct trfs_symlink_op_ {
        unsigned int r_id;
        unsigned short r_size;
        char r_type;
        unsigned int pid;
        int ret;
        unsigned short plen;
        unsigned short slen;
        char pathname[1];
}trfs_symlink_op;

typedef struct trfs_trunct_op_ {
        unsigned int r_id;
        unsigned short r_size;
        char r_type;
        unsigned int pid;
        int ret;
        unsigned short len;
        char pathname[1];
}trfs_trunct_op;

typedef struct trfs_mknod_op_ {
        unsigned int r_id;
        unsigned short r_size;
        char r_type;
        unsigned int pid;
        int ret;
        unsigned short len;
        char pathname[1];
}trfs_mknod_op;

typedef struct trfs_setxattr_op_ {
        unsigned int r_id;
        unsigned short r_size;
        uint8_t r_type;
        unsigned int pid;
        uint64_t addr;
        int ret;
        unsigned short len;
        char pathname[1];
}trfs_setxattr_op;

typedef struct trfs_getxattr_op_ {
        unsigned int r_id;
        unsigned short r_size;
        uint8_t r_type;
        unsigned int pid;
        uint64_t addr;
        int ret;
        unsigned short len;
        char pathname[1];
}trfs_getxattr_op;

typedef struct trfs_listxattr_op_ {
        unsigned int r_id;
        unsigned short r_size;
        uint8_t r_type;
        unsigned int pid;
        uint64_t addr;
        int ret;
        unsigned short len;
        char pathname[1];
}trfs_listxattr_op;

typedef struct trfs_removexattr_op_ {
        unsigned int r_id;
        unsigned short r_size;
        uint8_t r_type;
        unsigned int pid;
        uint64_t addr;
	int ret;
        unsigned short len;
        char pathname[1];
}trfs_removexattr_op;

#endif 
