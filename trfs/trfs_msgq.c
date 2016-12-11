#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/sched.h>
#include "trfs_msgq.h"
#include "tr_fs.h"

/** Max number of packets to be queued.
 *  */
#define TRFS_MAX_TX_Q_MSGS         1024

/** Max size of packet data.
 *  */
#define TRFS_MAX_TX_Q_MSG_SIZE     100 

/** Maximum tx queue nodes.
 *  */
#define TRFS_TX_Q_MAX_NODES 10

/** Message Queue List.
 *  */
static trfs_mq_info_t  trfs_msg_queues_g_g[TRFS_MQ_MAX_NO_OF_Q];
static int   numAfdxQs ;

extern trfs_log_write tlw;

trfs_mq_info_t * trfs_mq_get_node_by_id(trfsQid_t mqId);

/* Puts the Message in Given Message Queue.
 */
static int  trfsMqPutMsg(trfs_mq_info_t *node, unsigned char *msg_p, 
                       				int64_t msgLen, int priority);

/** Gets the Message from given Message Queue.
 */
static int  trfsMqGetMsg(trfs_mq_info_t *node, int **msg_p, 
         					int *msgLen, bool copy_only);

trfsMqAttr_t tx_q_attr;

/** Queue id to access the Tx queue.
 *  */
trfsQid_t    tx_q_id[TRFS_TX_Q_MAX_NODES];

/**
 * this function create queue to keep packets
 * @param[out] 0(0) : if q created successfully
 *             -1(-1): otherwise
 */
int trfs_create_log_q(void)
{
	int i;

	tx_q_attr.maxMsgs = TRFS_MAX_TX_Q_MSGS;
	tx_q_attr.msgSize = TRFS_MAX_TX_Q_MSG_SIZE;
	strcpy(tx_q_attr.name,"TRFS LOG Q");
	tx_q_attr.flags = TRFS_MQ_FIFO;

	for (i=0; i<TRFS_TX_Q_MAX_NODES; i++)
	{
		if (trfsMqOpen(&tx_q_attr, &tx_q_id[i]) != 0)
		{
			printk("TRFS: Q create failed \n");
			return -1;
		}
	}
	return 0;
}

/** Opens the Message Queue with given Attributes.
 */
int trfsMqOpen(trfsMqAttr_t *mq, trfsQid_t *mqId)
{
	int jj;
	trfs_mq_info_t  *node;
	static trfsQid_t mqIdSeq = 1;
	
	/* Validation */
	if ((mq == NULL) || (mqId == NULL)) {
	   return TRFS_ERR_INVALID_PARAM;
	}
	if ((mq->flags & TRFS_MQ_FIFO) && 
	    			(mq->flags & TRFS_MQ_PRIORITY)) {
	   return TRFS_ERR_INVALID_PARAM;
	}
	if ((mq->maxMsgs <= 0) || (mq->msgSize <= 0)) {
	   return TRFS_ERR_INVALID_PARAM;
	}
	
	if (mq->msgSize > TRFS_MQ_MAX_MSG_SIZE) {
	   return TRFS_ERR_INVALID_PARAM;
	}
	
	/* If flags not given, Set default flags */
	if (mq->flags == 0) {
	   mq->flags = TRFS_MQ_DEFAULT_TYPE;
	}
	
	/* Initialize the output */
	*mqId = -1;
	
	/* Creating Message Queue */
	if (numAfdxQs >= TRFS_MQ_MAX_NO_OF_Q) 
	   	return TRFS_ERR_INVALID_PARAM ;
	
	node = &trfs_msg_queues_g_g[numAfdxQs];
	if (node == NULL) {
	   return -1;
	}
	
	memcpy(&node->attr, mq, sizeof(trfsMqAttr_t));
	node->attr.counter = 0;
	node->mqId = mqIdSeq++;
	node->next = NULL;
	
	/* initializing the Wait Queue */
	init_waitqueue_head(&node->wq);
	
	if (numAfdxQs > 0) {
	   	trfs_msg_queues_g_g[numAfdxQs-1].next = node;
	}
	numAfdxQs++;
	for(jj=0;jj<=node->attr.maxMsgs;jj++) {
	   	node->msg[jj].pri = 0;
	   	node->msg[jj].len = 0;
	}
	*mqId = node->mqId;
	return 0;   
}


/* Closes the given message Queue 
 */
int trfsMqClose(trfsQid_t mqId)
{
	#if 0
	trfs_mq_info_t  *tmp, *prev_p, *rmnode_p = NULL;
	
	if (trfs_mq_info_gp == NULL)
	{
	   return -1;
	}
	
	if (trfs_mq_info_gp->mqId == mqId)
	{
	   rmnode_p = trfs_mq_info_gp;
	   trfs_mq_info_gp = trfs_mq_info_gp->next;
	}
	else
	{
	   prev_p = trfs_mq_info_gp;
	   for (tmp = trfs_mq_info_gp->next; tmp != NULL; tmp = tmp->next)
	   {
	      if (tmp->mqId == mqId)
	      {
	         /* Queue Found */
	         rmnode_p = tmp;
	         prev_p->next = tmp->next;
	      }
	      prev_p = tmp;
	   }
	}
	if (rmnode_p == NULL)
	{
	   return -1;
	}
	trfsMqFree(rmnode_p);
	#endif
	return 0;
}

/* Puts the message in given Message Queue 
 */
int trfsMqSend(trfsQid_t mqId, unsigned char *msg_p, size_t msgLen,
      				unsigned int msgPriority, const int timeOut)
{
	trfs_mq_info_t  *tmp;
	
	/* Validation */
	if ((msg_p == NULL) || (msgLen <= 0)) {
	   	return -1;
	}
	
	/* Get the Node */
	tmp = trfs_mq_get_node_by_id(mqId);
	if (tmp == NULL) { 
	   	/* Message Queue Node not found */
	   	return -1;
	}
	
	/* Check for Message size */
	if (msgLen<=0) {
	    	return TRFS_ERR_INVALID_PARAM;
	}
	
	mutex_lock(&tlw.q_lock);
	
	/* Put the MSG in Message Queue */
	if (trfsMqPutMsg(tmp, msg_p, msgLen, msgPriority) == -1) {
		mutex_unlock(&tlw.q_lock);
	     	return -1; 
	}
	mutex_unlock(&tlw.q_lock);
	
	return msgLen;
}


/* Gets the message form given Message Queue 
 */
int trfsMqRecv (trfsQid_t mqId, int **msg_p, int *msgLen,
      				unsigned int  msg_priority, const int timeOut)
{
	trfs_mq_info_t  *tmp;
	int flush_flag = 0;
	
	/* Validation */
	if ((msg_p == NULL) || (msgLen == NULL)) {
		return -1;
	}
	
	/* Get the Node */
	tmp = trfs_mq_get_node_by_id(mqId);
	if (tmp == NULL) {
	   	/* Message Queue Node not found */
	   	printk("Message Queue Node not found \n");
	   	return -1;
	}
	if (timeOut > 0) {
	   	int64_t wqret;

		flush_flag = 1;
	   	
	   	wqret = wait_event_interruptible_timeout(tmp->wq, 
			(tmp->attr.counter != 0), msecs_to_jiffies(timeOut));
	}
	else if (timeOut == -1) {
	   	wait_event_interruptible(tmp->wq,
			(((trfs_get_exit_flag(mqId))==1)||
						(tmp->attr.counter != 0)));
	}
	if (trfs_get_exit_flag(mqId) == 1) {
	  	printk("Exiting waiting queue \n");
		return TRFS_EXIT_WAITING_QUEUE;       
	}
	
	if (tmp->attr.counter == 0) {
	   	/* Time out. queue empty */
		return TRFS_FLUSH_TIMEOUT;
	}
	
	mutex_lock(&tlw.q_lock);

	/* Get the MSG from Message Queue */
	if (trfsMqGetMsg(tmp, msg_p, msgLen, 0) == -1) {
	     	printk("Not able to get the message from the queue \n");
		mutex_unlock(&tlw.q_lock);
	   	return -1;
	}
	mutex_unlock(&tlw.q_lock);
	
	return *msgLen;
}

/* Gets the current Attributes of given Message Queue 
 */
int trfsMqGetAttr(trfsQid_t  mqId, trfsMqAttr_t  *mqAttr)
{
	trfs_mq_info_t *tmp;
	
	if (mqAttr == NULL) {
	   	return TRFS_ERR_INVALID_PARAM;
	}
	
	/* Get the Node */
	tmp = trfs_mq_get_node_by_id(mqId);
	if (tmp == NULL) {
		/* Message Queue Node not found */
	   	return -1;
	}
	
	/* Filling the attributes of message Queue */
	memcpy(mqAttr, &tmp->attr, sizeof(trfsMqAttr_t));
	
	return -1;
} /* trfsMqGetAttr */




/* Sets the given Attributes to given Massage Queue 
 */
int trfsMqSetAttr(trfsQid_t  mqId, const trfsMqAttr_t  *mqAttr)
{
	return -1;
}


/* Gives the Message Queue Node for given ID 
 */
trfs_mq_info_t * trfs_mq_get_node_by_id(trfsQid_t mqId)
{
	#ifdef TRFS_MQ_ARRAY
	int  i;
	
	for (i=0; i< TRFS_MQ_MAX_NO_OF_Q; i++) {
	   	if (trfs_msg_queues_g_g[i].mqId == mqId) {
	      		return &trfs_msg_queues_g_g[i];
	   	}
	} 
	#else
	trfs_mq_info_t *tmp;
	for (tmp = &trfs_msg_queues_g_g[0]; tmp != NULL; tmp = tmp->next) {
		if (tmp->mqId == mqId) {
	      		return tmp;
	   	}
	}
	
	#endif
	return NULL;
}

/* Puts the Message in Given Message Queue 
 */
static int  trfsMqPutMsg(trfs_mq_info_t *node, unsigned char *msg_p, 
                        		 int64_t msgLen, int priority)
{
	int wIndex;

	/* Check Array is full or not */
	#ifdef TRFS_MQ_CIRCULAR_Q
	if (node->attr.counter == node->attr.maxMsgs) {
	   return TRFS_ERR_MQ_FULL; 
	}
	#else
	if (node->attr.counter == node->attr.maxMsgs) {
	   return TRFS_ERR_MQ_FULL; 
	}
	#endif
	
	#if defined TRFS_MQ_BUFFER
	/* Put the Message in Buffer */
	implement
	#elif defined  TRFS_MQ_ARRAY
	
	wIndex = node->writeIndex;
	node->writeIndex = (node->writeIndex + 1)%(node->attr.maxMsgs);

	/* Put the Message in Array, increment the Read index */
	node->msg[wIndex].len =  msgLen;
	node->msg[wIndex].pri =  priority;
	node->msg[wIndex].data_p = msg_p;
	
	node->attr.counter = node->attr.counter+1;
	
	#else
	implement
	/* Put the Message in linked list */
	#endif
	if (node->attr.counter >=1) {
	   /* Wake up any waiting process on this Q */
	   wake_up_interruptible(&node->wq);
	}
	return 0;
}

/** Gets the Message from given Message Queue.
 */
static int  trfsMqGetMsg(trfs_mq_info_t *node, int **msg_p, 
                       				int *msgLen, bool copy_only)
{
	/* check Array is empty or not */
	if (node->attr.counter == 0) {
	   return TRFS_ERR_MQ_EMPTY; 
	}
	#ifdef TRFS_MQ_BUFFER
	   /* Get the Message from Buffer */
	#elif defined TRFS_MQ_ARRAY
	 /* Get the Message from Array, decrement the current index. */
	 {
	     *msgLen = node->msg[node->readIndex].len;
	 }
	 
	 *msg_p = (int *)node->msg[node->readIndex].data_p;
	#else
	  /* Get the Message from Linked List */
	#endif
	   /* copy the message length in to 'msgLen' */
	if (copy_only == 0) {
	   	/* delete the message from Queue */
	   	node->attr.counter = node->attr.counter-1;
	   	node->readIndex = (node->readIndex + 1) % (node->attr.maxMsgs);
	}
	return 0;
} /* trfsMqGetMsg */


/* Gives message availability of given Message Queue */
int trfsMqIsMsgAvailable(trfsQid_t  mqId)
{
	trfs_mq_info_t *tmp;
	
	/* Get the Node */
	tmp = trfs_mq_get_node_by_id(mqId);
	if (tmp == NULL)
	{
	   /* Message Queue Node not found */
	   return 0; /* TODO: return boolean values */
	}
	
	if (tmp->attr.counter > 0)
	{
	   return 1;
	}
	
	return 0;
} /* trfsMqIsMsgAvailable */

/**
 * this function delete the created queue
 * @param[out] 0(0) : if q deleted successfully
 *            -1(-1): otherwise
 */ 
int trfs_delete_log_q(void)
{
	int i;

	for (i=0; i<TRFS_TX_Q_MAX_NODES; i++) {
             if (tx_q_id[i]) {
	         trfsMqClose(tx_q_id[i]);
	     }
	}
	return 0;
}

char trfs_get_exit_flag(trfsQid_t mqId)
{
	trfs_mq_info_t  *tmp;
	tmp = trfs_mq_get_node_by_id(mqId);
	return tmp->attr.exit_flag; 
} 

void trfs_set_exit_flag(trfsQid_t mqId)
{
	trfs_mq_info_t  *tmp;
	tmp = trfs_mq_get_node_by_id(mqId);
	tmp->attr.exit_flag =1; 
	wake_up_interruptible(&tmp->wq);
} 

void trfs_clear_exit_flag(trfsQid_t mqId)
{
	trfs_mq_info_t  *tmp;
	tmp = trfs_mq_get_node_by_id(mqId);
	tmp->attr.exit_flag =0; 
} 

/* EOF */

