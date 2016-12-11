obj-m += treplay.o
OTHER_OBJS = trctl.o 

all: treplay trctl

treplay: treplay.c
	gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi treplay.c trfs_ops.c -o treplay

trctl: trctl.c
	gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi trctl.c -o trctl
clean:
	rm -f treplay trctl
