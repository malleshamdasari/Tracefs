#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "trctl.h" 

int main( int argc, char *argv[]) 
{
	int retval = 0;
	char *cmd = NULL;
	char mpoint[256];
	int fd, choice, ret = 0;
	trctl_args *args = NULL;

	/* Validate the number of command line parameters. */
	if (argc<2)  {
		printf("Invalid arguments: Please try ./trctl [cmd] \
				 TrfsMountPoint\n");
		goto out;

	}

	/* Extract the flags. */
	opterr = 0;
 	while ((choice = getopt (argc, argv, "c:")) != -1) {
		switch(choice) {
			case 'c':
				cmd = optarg;
				break;
			case '?' :
				perror("Unknown option character.\n");
				goto out;
	
			default :
				printf("Bad Command Line Arguments. \
						 Use '-h' flag for Help.\n");
				goto out;
		}
	}

	/* Extract the input filename. */
     	strcpy(mpoint, argv[optind]);

	fd = open(mpoint, O_RDWR, &cmd);
        if (fd == -1) {
                printf("Error in opening file \n");
                exit(-1);
        }
	if (cmd) {
		args = (trctl_args *)malloc(sizeof(trctl_args));
		if (strcmp(cmd, "all")==0)
			args->bitmap = ~(args->bitmap&0);
		else if (strcmp(cmd, "none")==0)
			args->bitmap = 0;
		else
			args->bitmap = atoi(cmd);

 		retval = ioctl(fd, IOCTL_TRFS_SET_BITMAP, args); 
	}
	else {
 		retval = ioctl(fd, IOCTL_TRFS_GET_BITMAP, 0); 
		printf("Current bitmap is: %d \n",retval);
	}
        close(fd);
out:
	if (args)
		free(args);
	return ret;
}
