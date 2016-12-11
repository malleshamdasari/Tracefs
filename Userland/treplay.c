/*******************************************************************************
 * **                                                                            **
 * ** File: treplay.c                                                            **
 * **                                                                            **
 * ** Copyright Â© 2016, OS Lab, CSE, Stony Brook University.                    **
 * ** All rights reserved.                                                       **
 * ** http://www.cs.stonybrook.edu                                               **
 * **                                                                            **
 * ** All information contained herein is property of Stony Brook University     **
 * ** unless otherwise explicitly mentioned.                                     **
 * **                                                                            **
 * ** The intellectual and technical concepts in this file are proprietary       **
 * ** to SBU and may be covered by granted or in process national                **
 * ** and international patents and are protect by trade secrets and             **
 * ** copyright law.                                                             **
 * **                                                                            **
 * ** Redistribution and use in source and binary forms of the content in        **
 * ** this file, with or without modification are not permitted unless           **
 * ** permission is explicitly granted by Stony Brook University.                **
 * **                                                                            **
 * **                                                                            **
 * ** Project: File MergeSort System Call                                        **
 * **                                                                            **
 * ** Author(s): Mallesham Dasari                                                **
 * **                                                                            **
 * *******************************************************************************/

#include "trfs_ops.h"

#define PAGE_SIZE 4096

int gflags = 0;

/** Reads the tfile into a page and reads each record from the page buffer
 * param[in] buf buffer
 * param[in] len Length of the buffer
 */
static unsigned int trfs_read_tfile_buf(struct trfs_rpd *this,
						 	char *buf, int len)
{
	char *rec = NULL;
	int ret = 0;
	unsigned short rsize = 0;
	unsigned char rtype = 0;

	while (len>0) {
		memcpy(&rsize, buf+sizeof(int), sizeof(unsigned short));
		if (rsize>len) {
			ret = rsize;
			break;	
		}
		rec = (char *)malloc(rsize);
		memcpy(rec, buf, rsize);
		rtype = rec[sizeof(int)+sizeof(unsigned short)];
		switch (rtype) {
			case TRFS_OP_MKDIR:
				this->ops->replay_mkdir_op(this,\
							 (trfs_mkdir_op *)buf);
				break;
			case TRFS_OP_RMDIR:
				this->ops->replay_rmdir_op(this,\
							(trfs_rmdir_op *)buf);
				break;
			case TRFS_OP_LINK:
				this->ops->replay_link_op(this,\
							(trfs_link_op *)buf);
				break;
			case TRFS_OP_SYMLINK:
				this->ops->replay_symlink_op(this,\
							(trfs_symlink_op *)buf);
				break;
			case TRFS_OP_UNLINK:
				this->ops->replay_unlink_op(this,\
							(trfs_unlink_op *)buf);
				break;
			case TRFS_OP_TRUNCATE:
				this->ops->replay_trunct_op(this,\
							(trfs_trunct_op *)buf);
				break;
			case TRFS_OP_RENAME:
				this->ops->replay_rename_op(this,\
							(trfs_rename_op *)buf);
				break;
			case TRFS_OP_OPEN:
				this->ops->replay_open_op(this,\
							(trfs_open_op *)buf);
				break;
			case TRFS_OP_READ:
				this->ops->replay_read_op(this,\
							(trfs_read_op *)buf);
				break;
			case TRFS_OP_WRITE:
				this->ops->replay_write_op(this,\
							(trfs_write_op *)buf);
				break;
			case TRFS_OP_CLOSE:
				this->ops->replay_close_op(this,\
							(trfs_close_op *)buf);
				break;
			default:
				break;
		}
		len -= rsize;
		buf += rsize;	
		free(rec);
	}
	return ret;
}

/** Main function to test the functionality of trfs in userland. 
 * param[in] argc Number of command line parameters.
 * param[in] argv List of arguments
 */
int main(int argc, char *argv[])
{
	int choice;
	char tfile[256];
	struct stat buf;
	char *page = NULL;
	int left_bytes = 0;
	int ret = 0, fd = 0;
	int fsize = 0, bytes = 0;
	struct trfs_rpd *rpd = NULL;

	/* Validate the number of command line parameters. */
	if ((argc<2) || (argc>4))  {
		printf("Invalid arguments: Please try ./treplay [ns] tfile\n");
		goto out;

	}

	/* Extract the flags. */
	opterr = 0;
 	while ((choice = getopt (argc, argv, "ns")) != -1) {
		switch(choice) {
			case 'n':
				gflags = 1;
				break;
			case 's':
				gflags = 2;
				break;
			case '?' :
				printf("Unknown option/No trace file.\n");
				ret = -ENOENT;
				goto out;
	
			default :
				printf("Bad Command Line Arguments. \
						 Use '-h' flag for Help.\n");
				goto out;
		}
	}

	if (optind >= argc) {
		printf("No trace file specified \n");
		ret = -ENOENT;
		goto out;
	}

	/* Extract the input filename. */
     	strcpy(tfile, argv[optind]);

	fd = open(tfile, O_RDONLY);
	if (fd<0) {
		printf("Opening the tfile: Failed \n");
		ret = -ENOENT;
		goto out;
	}
	
	rpd = trfs_rpd_init();

	fstat(fd, &buf);
	fsize = buf.st_size;

	page = (char *)malloc(PAGE_SIZE);

	while (fsize>0) {
		bytes = read(fd, page, PAGE_SIZE);
		if (bytes<=0) {
			printf("Parsing completed: Partially\n");
			break;
		}
		fsize -= bytes;
		left_bytes = trfs_read_tfile_buf(rpd, page, bytes);
		fsize += left_bytes;
		lseek(fd, (off_t)-left_bytes, SEEK_CUR);
	}
out:
	trfs_rpd_exit(rpd);
	if (page)
		free(page);
	close(fd);
        return ret;
}

/* EOF */
