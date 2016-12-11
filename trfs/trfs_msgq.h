#ifndef __TRFS_MSGQ_H__
#define __TRFS_MSGQ_H__

#include <linux/types.h>
#include <linux/wait.h>

#define TRFS_MQ_ARRAY
#define TRFS_MQ_CIRCULAR_Q 

#define TRFS_MQ_MAX_NO_OF_Q     12
#define TRFS_MQ_MAX_NO_OF_MSGS  1024 //64

#define trfsQid_t int64_t

#define TRFS_FLUSH_TIME		50000
#define TRFS_FLUSH_TIMEOUT	-99

#define TRFS_MQ_MAX_MSG_SIZE  1024

#define TRFS_EXIT_WAITING_QUEUE -2

/* Maximum size for the Message Queue name */
#define TRFS_MAX_MQ_NAME_SIZE    30 /*16*/ /* Using Afdx port name as Queue name */    

/* Message Queue default discipline type */
#define TRFS_MQ_DEFAULT_TYPE     TRFS_MQ_FIFO

#define TRFS_GEN_ERR_BASE       0x00F00000
/** Invalid parameter passed to the function **/
#define TRFS_ERR_INVALID_PARAM  -(TRFS_GEN_ERR_BASE + 1)
/* Error code to indicate Queue Full */
#define TRFS_ERR_MQ_FULL         -(TRFS_GEN_ERR_BASE + 2)
/* Error code to indicate Queue Empty */
#define TRFS_ERR_MQ_EMPTY        -(TRFS_GEN_ERR_BASE + 3)

#define TRFS_LOG_QID		2
#define TRFS_LOG_COMPLETE	127

typedef struct trfsMqMsg {
	unsigned int  pri;   /* Priority variable */
	int64_t   len;
	unsigned char    *data_p;
} trfsMqMsg_t;

/* Flags are */
typedef enum  trfsMqFlags {
	TRFS_MQ_FIFO        = 0x00000001,
	TRFS_MQ_PRIORITY    = 0x00000002
} trfsMqFlags_t;

typedef struct trfsMqAttr {
	int  flags;          /* message queue flags                  */
	int  maxMsgs;        /* maximum number of messages           */
	int  msgSize;        /* maximum message size                 */
	int  counter;        /* number of messages currently queued  */
	char  exit_flag;        /* number of messages currently queued  */
	char   name[TRFS_MAX_MQ_NAME_SIZE]; /* message queue name */
} trfsMqAttr_t;

typedef struct trfs_mq_info
{
   trfsQid_t     mqId;  /* message Queue id */
   trfsMqAttr_t  attr;  /* message Queue attributes */
   int     index;
   wait_queue_head_t  wq; /* waitQ for blocking implementation */
#ifdef TRFS_MQ_ARRAY
   int     readIndex;
   int     writeIndex;
   trfsMqMsg_t   msg[TRFS_MQ_MAX_NO_OF_MSGS]; /* message */
#endif /* TRFS_MQ_ARRAY */
   struct trfs_mq_info *next;
} trfs_mq_info_t;


/* Opens the Message Queue with given Attributes. */
extern int trfsMqOpen(trfsMqAttr_t *mq, trfsQid_t *mqId);  


/* Closes the given message Queue. */
extern int trfsMqClose(trfsQid_t mqId);     

/*Puts the message in given Message Queue. */
extern int trfsMqSend(trfsQid_t   mqId,    
                    unsigned char  *msg_p,
                    size_t        msgLen,
                    unsigned int      msgPriority,
                    const int timeOut);


/* Gets the message form given Message Queue. */
extern int trfsMqRecv(trfsQid_t   mqId,     
                    int        **msg_ptr,
                    int        *msg_len,
                    unsigned int      msg_priority,
                    const int timeOut);

int trfs_create_log_q(void);

int trfs_delete_log_q(void);

char trfs_get_exit_flag(trfsQid_t mqId);
void trfs_set_exit_flag(trfsQid_t mqId);
void trfs_clear_exit_flag(trfsQid_t mqId);

#endif /*EndOf __TRFS_TDMA_MSGQ_H__ **/
