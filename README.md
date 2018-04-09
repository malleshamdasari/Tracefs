# Tracefs
A file system to trace file system operations

1. Introduction:
----------------
The homework is based on VFS file system of Linux, and particularly based on
extensible file systems. In this, the wrapfs file system is used as a starting
point and modified it to add file operations tracing support, save the trace 
file to disk, to be able to replay it in user space.

2. Description:
---------------
The homework includes writing a stackable, tracing file system called "trfs".
It captures records of file system activity for a number of file system
operations.  For instance, for the ->read method, it records the file name, 
buffer pointer, number of bytes to read, and the return result (or errno).  
Each record is appended to a file or any other output driver on disk (e.g., a 
file on the below file system). Records written use in binary format so they 
are compact. Records has single byte encoding with the record type(mkdir,
rmdir, mknod, unlink, symlink). For example, for ->read, the record written 
can be something like this:

	<4B: record ID>
	<2B: size of the record>
	<1B: Type of record>
	<4B: bytes to read>
	<4B: buffer>
	<2B: length of pathname>
	<variable length: pathname with null-termitation>
	<4B: return value>

3. High Level Design Details:
-----------------------------
    The high level design decisions are inherited from the research work by
    Aranya et.al [1]. The design phases of the problem are divided into
    below modules:

    3.1 Kernel source design
        3.1.1 Parsing the mount options
                The trace file is passed to the trfs while mounting the file
                system. The options parsing is taken from the eCryptfs
                implementation in the linux kernel. Once parsed, the file is
                opened for writing stream of traces and closed on unmount.
        3.1.2 Input filter
                The input filter is built on top of ioctl support inside the
                kernel. From trctl, the user sends the bitmap of operations
                to be traced. This bitmap is passed as an input to input fil
                ter which intern intializes the file system operations that
                are to be traced. This could be performed dynamically as and
                when the user passes the bitmap through trctl.
        3.1.3 Assembly driver
                The assembly driver converts the vfs objects parameters into
                stream format. It generates the records and send them to the
                output queue.
        3.1.4 Output filter
                The output filter is useful for enhancements on the traced
                data, for example, compression or encryption techniques. In
                this assignment, only crc message encoding is supported. 
        3.1.5 Output driver
                Once the output filter is done with enhancing the data, it 
                passes the data to output driver(kthread/work queue) that
                the stream of traces into output device such as file or net
                link sockets. In this assignment, file driver is considered
                for the output stream writing. 
    3.2 Userland source design
        3.2.1 Ioctl bitmap passinig (trctl)
                Ioctl system call from the userland passes a bitmap of trace
                operations to the kernel. This could be used any number of 
                times during the trfs mount. It is used to set and get the 
                status of operations to be traced.
        3.2.2 Replaying (Mapping operations)
                Replay involves parsing of the trace file and map the each 
                file system operation to replay them synchronously. For some
                chain of operations such as open, read, write and close,

Low Level Design Details:
-------------------------
The low level design decisions are taken by keeping the efficiency of code in
consideration. For this, several of the above modules has below low level 
implementations:

	* Async writing
		- Queues(Blocking for efficiency) are used instead of lists
		- The queue has the enhancements such as, it does not copy 
		  the records to another buffer but takes only the pointer
		  and freed inside the kthread. This reduces huge amount of
		  memory usage while copying records from assembly driver to 
		  output driver.
	* Threading 
		- kthread is used (workqueue could also be used)
		- kthread is efficient as it is not a periodic work to do
	* Ioctl support
		- a bimap of file operations to trace
	* Locking
		- Mutex is used 
		- Spinlock is not good as the records writing could take much 
		  long time 
	* Exploiting Container Format
		- For storing the pathnames for some operations(such as open, 
		  mkdir, rmdir, rename, symlink, link and unlink), the below
		  container format is used.

		   struct trfs_open_op {
			int r_id;
			int r_size;
			   ....		/* Few more attributes */
			int path_len;
		 	char pathname[1];
		   } 

	* For efficient code structure, following techniques are build 
		- Generic macros
			- Reduces lot of redundant code
		- Function pointers
			- Gives the flexibility of adding new operations and
			  removing unneccessary operations

Functionality:
--------------
Tracing of the file system operations and replaying them again is not an easy 
task. Hence, only a set of operations are traced as shown below. For file
system operations that are not traced, there could be serveral reasons as
mentioned below.
	* The following operations are supported for tracing and replaying.
		- trfs_open
		- trfs_read
		- trfs_write
		- trfs_close
		- trfs_mkdir
		- trfs_rmdir
		- trfs_link
		- trfs_unlink
		- trfs_symlink
		- trfs_rename
		- trfs_truncate
		- trfs_setxattre
		- trfs_getxattre
		- trfs_listxattre
		- trfs_removexattre
	* The following operations are complex to replay because of their 
	  dependency on many other operations
		- trfs_mmap
		- trfs_revalidate
		- trfs_fsync
		- trfs_flush and many other ops


Compilation and Installation:
-----------------------------
The source code of trfs is added to linux kernel by the following set of
commands
	- Copy the trfs kernel sources to fs/trfs/
	- Write a Makefile to include the trfs source files inside trfs
	- Add the trfs file system to Makefile in fs/ directory with below
	  command
		config TRFS
		tristate "my fs"
	- Add the below line to Kconfig
		source "fs/my_fs/Kconfig"
	- Add a magic number in uapi/magic.h
	- Select the trfs module using below command
		$make menuconfig

The code is compiled and installed as follows:
	- kernel source compilation
		$make fs/trfs/
		$make modules
		$insmod fs/trfs/trfs.ko
		$mount -t trfs -o tfile=/tmp/tfile /some/low/path /mnt/trfs/
	- User source compilation
		$cd hw2/
		$make
		$./treplay [ns] tfile
		$mknod /mnt/trfs/some/file c MajorNumber 0
		$./trctl [cmd] tfile
Uninstall the kernel source using below commands
		$umount /mnt/trfs/
		$rmmod trfs
	
Testing:
--------
A sample test program is provided in hw2/Tests/ directory, to test the usage
of this module. Follow the below instructions to test the functionality.
	- Compile and install the file system
	- Execute the test program as below
		$cd hw2/Tests/
		$gcc sample.c
		$./a.out /mnt/trfs/some/file /mnt/trfs/some/other/file
	- Now replay using treplay
		$./treplay [ns] /tmp/tfile

Extra Credit:
-------------
A crc is implemente for the gauranteeing the reliability of the records. Please
see the trfc_crc.c for the crc source.

References:
-----------
[1]     Akshat Aranya, Charles P. Wright, and Erez Zadok. 2004. Tracefs: A File 
	System to Trace Them All. In Proceedings of the 3rd USENIX Conference on
	File and Storage Technologies (FAST '04). USENIX Association, Berkeley, 
	CA, USA, 129-145.
