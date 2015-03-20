/*
 * Rawbulk Driver from VIA Telecom
 * Copyright (C) 2011 VIA Telecom, Inc.
 * Author: Karfield Chen (kfchen@via-telecom.com)
 * Copyright (C) 2012 VIA Telecom, Inc.
 * Author: Juelun Guo (jlguo@via-telecom.com)
 * Changes:
 *
 * Sep 2012: Juelun Guo <jlguo@via-telecom.com>
 *           Version 1.0.4
 *           changed to support for sdio bypass.
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *
 * Rawbulk is transfer performer between CBP host driver and Gadget driver
 *
 *
 * upstream:    CBP Driver ---> Gadget IN
 * downstream:  Gadget OUT ---> CBP Driver
 *
 *
 */
 
/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#define DRIVER_AUTHOR   "Juelun Guo <jlguo@via-telecom.com>"
#define DRIVER_DESC     "Rawbulk Driver - perform bypass for QingCheng"
#define DRIVER_VERSION  "1.0.4"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <mach/viatel_rawbulk.h>
#include <linux/moduleparam.h>

#define VERB0SE_DEBUG
#ifdef VERBOSE_DEBUG
#define ldbg(fmt, args...) \
    printk(KERN_DEBUG "%s: " fmt "\n", __func__, ##args)
#define tdbg(t, fmt, args...) \
    printk(KERN_DEBUG "Rawbulk %s: " fmt "\n", t->name, ##args)
#else
#define ldbg(args...)
#define tdbg(args...)
#endif

#define lerr(fmt, args...) \
    printk(KERN_ERR "%s: " fmt "\n", __func__, ##args)
#define terr(t, fmt, args...) \
    printk(KERN_ERR "Rawbulk [%s]:" fmt "\n", t->name,  ##args)
#define STOP_UPSTREAM   0x1
#define STOP_DOWNSTREAM 0x2

extern int modem_buffer_push(int port_num, const unsigned char *buf, int count);
char * transfer_name[] = {"modem", "ets", "at", "pcv", "gps"};

struct rawbulk_transfer {
    enum transfer_id id;
    spinlock_t lock;
    int control;
    
    struct usb_function *function;
    struct usb_interface *interface;
    rawbulk_autoreconn_callback_t autoreconn;
    struct {
        int ntrans;
        struct list_head transactions;
        struct usb_ep *ep;
    } upstream, downstream, repush2modem, cache_buf_lists;
    
    int sdio_block;
    int down_flow;
    spinlock_t usb_down_lock;
    spinlock_t modem_block_lock;
    struct delayed_work delayed;
    struct workqueue_struct *flow_wq;
    
    struct work_struct      read_work;
    struct work_struct      write_work;    
    struct workqueue_struct *rx_wq;
    struct workqueue_struct *tx_wq;
    struct mutex modem_up_mutex;
    struct mutex usb_up_mutex;
    struct timer_list timer;
    spinlock_t flow_lock;
};

static inline int get_epnum(struct usb_host_endpoint *ep) {
    return (int)(ep->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
}

static inline int get_maxpacksize(struct usb_host_endpoint *ep) {
    return (int)(le16_to_cpu(ep->desc.wMaxPacketSize));
}

struct cache_buf {
        int length;
        struct list_head clist;
        struct rawbulk_transfer *transfer;
        int state;
        //unsigned char buffer[0];
        char *buffer;
};

#define MAX_RESPONSE    32
struct rawbulk_transfer_model {
    struct usb_device *udev;
    struct usb_composite_dev *cdev;
    char ctrl_response[MAX_RESPONSE];
    struct rawbulk_transfer transfer[_MAX_TID];
};
static struct rawbulk_transfer_model *rawbulk;

static struct rawbulk_transfer *id_to_transfer(int transfer_id) {
    if (transfer_id < 0 || transfer_id >= _MAX_TID)
        return NULL;
    return &rawbulk->transfer[transfer_id];
}

//extern int rawbulk_usb_state_check(void);

/*
 * upstream
 */

#define UPSTREAM_STAT_FREE          0
#define UPSTREAM_STAT_UPLOADING     2

struct upstream_transaction {
    int state;
    int stalled;
    char name[32];
    struct list_head tlist;
    struct delayed_work delayed;
    struct rawbulk_transfer *transfer;
    struct usb_request *req;
    int buffer_length;
    //unsigned char buffer[0];
    char *buffer;
};

static unsigned int dump_mask = 0;
module_param(dump_mask, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(dump_mask, "Set data dump mask for each transfers");

static inline void dump_data(struct rawbulk_transfer *trans,
        const char *str, const unsigned char *data, int size) {
	int i;
    int no_chars = 0;

    if (!(dump_mask & (1 << trans->id)))
        return;

    printk(KERN_DEBUG "DUMP tid = %d, %s: len = %d, chars = \"",
            trans->id, str, size);
    for (i = 0; i < size; ++i) {
        char c = data[i];
        if (c > 0x20 && c < 0x7e) {
            printk("%c", c);
        } else {
            printk(".");
            no_chars ++;
        }
    }
    printk("\", data = ");
    for (i = 0; i < size; ++i) {
        printk("%.2x ", data[i]);
        if (i > 7)
            break;
    }
    if (size < 8) {
        printk("\n");
        return;
    } else if (i < size - 8) {
        printk("... ");
        i = size - 8;
    }
    for (; i < size; ++i)
        printk("%.2x ", data[i]);
    printk("\n");
}

static struct upstream_transaction *
alloc_upstream_transaction(struct rawbulk_transfer *transfer, int bufsz)
{
    struct upstream_transaction *t;
    
    //t = kmalloc(sizeof *t + bufsz * sizeof(unsigned char), GFP_KERNEL);
    t = kmalloc(sizeof(struct upstream_transaction), GFP_KERNEL);
    if (!t)
        return NULL;
        
    t->buffer = (char *)__get_free_page(GFP_KERNEL);
    //t->buffer = kmalloc(bufsz, GFP_KERNEL);
    if (!t->buffer) {
        kfree(t);
        return NULL;
    }   
    t->buffer_length = bufsz;

    t->req = usb_ep_alloc_request(transfer->upstream.ep, GFP_KERNEL);
    if (!t->req)
        goto failto_alloc_usb_request;
    t->req->context = t;
    t->name[0] = 0;
    sprintf(t->name, "U%d ( G:%s)", transfer->upstream.ntrans,
            transfer->upstream.ep->name);         

    INIT_LIST_HEAD(&t->tlist);
    list_add_tail(&t->tlist, &transfer->upstream.transactions);
    transfer->upstream.ntrans ++;
    t->transfer = transfer;
    t->state = UPSTREAM_STAT_FREE;
    return t;

failto_alloc_usb_request:
    //kfree(t->buffer);
    free_page((unsigned long) t->buffer);
    kfree(t);
    return NULL;
}

static void free_upstream_transaction(struct rawbulk_transfer *transfer) {
    struct list_head *p, *n;
    
    mutex_lock(&transfer->usb_up_mutex);
    list_for_each_safe(p, n, &transfer->upstream.transactions) {
        struct upstream_transaction *t = list_entry(p, struct
                upstream_transaction, tlist);

        list_del(p);
        //kfree(t->buffer);
        free_page((unsigned long) t->buffer);
        usb_ep_free_request(transfer->upstream.ep, t->req);
        kfree(t);

        transfer->upstream.ntrans --;
    }
    mutex_unlock(&transfer->usb_up_mutex);
}

static void free_upstream_sdio_buf(struct rawbulk_transfer *transfer) {    
    struct list_head *p, *n;
    
    mutex_lock(&transfer->modem_up_mutex);
    list_for_each_safe(p, n, &transfer->cache_buf_lists.transactions) {
        struct cache_buf *c = list_entry(p, struct
                cache_buf, clist);
        list_del(p);
        //kfree(c->buffer);
        free_page((unsigned long) c->buffer);
        kfree(c);
        transfer->cache_buf_lists.ntrans--;
    }
    mutex_unlock(&transfer->modem_up_mutex);
}

static void upstream_complete(struct usb_ep *ep, struct usb_request
        *req);

static void start_upstream(struct work_struct * work) {
    int ret = -1;
    struct upstream_transaction *t;
    struct rawbulk_transfer *transfer = container_of(work, struct rawbulk_transfer, write_work);
    struct cache_buf *c;
    int length;
    char * buffer;
    int part1_sz,part2_sz;
    int stop, retry = 0;
    int flag, temp;
    int i;
    struct usb_request *req;

    mutex_lock(&transfer->modem_up_mutex);

    list_for_each_entry(c, &transfer->cache_buf_lists.transactions, clist) {
        if (c && (c->state == UPSTREAM_STAT_UPLOADING) && !(transfer->control & STOP_UPSTREAM)) {               
            ret = 0;
            break;
        }
    }
    mutex_unlock(&transfer->modem_up_mutex);
            
    if (ret < 0) {
            return;
    }
      
    if (!c){
        return;
    }
    
    length = c->length;
    buffer = c->buffer;
   
reget: 
      mutex_lock(&transfer->usb_up_mutex);      
      list_for_each_entry(t, &transfer->upstream.transactions, tlist) {
            if (t && (t->state == UPSTREAM_STAT_FREE) && !(transfer->control & STOP_UPSTREAM)) {                
                ret = 0;
                retry = 0;
                break;
            }           
      }
      mutex_unlock(&transfer->usb_up_mutex);
      if (ret < 0) {            
            if(transfer->control & STOP_UPSTREAM) {             
                return;
            }            
            retry = 1;
      }
      
    
      if (retry) {           
             ldbg("%s: up request is buzy, try to get usb request\n", __func__);          
             goto reget;
      }
      if (!t->req) {
            return;
      }     
      req = t->req;

      memcpy(t->buffer, buffer, length);
      dump_data(transfer, "pushing up", t->buffer, length);
      req->length = length;
      req->buf = t->buffer;
      req->complete = upstream_complete;
      req->zero = ((length % transfer->upstream.ep->maxpacket) == 0);
      t->state = UPSTREAM_STAT_UPLOADING;      
      //if(rawbulk_usb_state_check()) {
           ret = usb_ep_queue(transfer->upstream.ep, req, GFP_ATOMIC);
      //} else
      //     return;
      if(ret < 0) {
           terr(t, "fail to queue request, %d", ret);
           t->state = UPSTREAM_STAT_FREE;
           return;
      }  	    
    c->state = UPSTREAM_STAT_FREE;
/*    length = c->length;
    buffer = c->buffer;
    part1_sz = length - (length & 511);
    part2_sz = length & 511;
    
    for(i = 0; i < 2; i++) {
        //printk("%s i = %d**3**\n", __func__, i);  
        if (i == 0) {
            flag = 1;
            temp = part1_sz;
            if(part1_sz <= 0)
             continue;
        } else {
            flag = 0;
            temp = part2_sz;
            if(part2_sz <= 0)
             continue;
        } 
        ret = -1;
reget: 
      mutex_lock(&transfer->usb_up_mutex);      
      list_for_each_entry(t, &transfer->upstream.transactions, tlist) {
            if (t && (t->state == UPSTREAM_STAT_FREE) && !(transfer->control & STOP_UPSTREAM)) {                
                ret = 0;
                retry = 0;
                break;
            }           
      }
      mutex_unlock(&transfer->usb_up_mutex);
      if (ret < 0) {            
            if(transfer->control & STOP_UPSTREAM) {             
                return;
            }            
            retry = 1;
      }
      
    
      if (retry) {
             printk("%s goto reget usb request\n", __func__);           
             goto reget;
      }
      if (!t->req) {
            return;
      }     
      req = t->req;

      memcpy(t->buffer, buffer, temp);
      dump_data(transfer, "pushing up", t->buffer, temp);
      buffer += temp;
      length -= temp;
      req->length = temp;
      req->buf = t->buffer;
      req->complete = upstream_complete;
      //req->zero = ((length % transfer->upstream.ep->maxpacket) == 0);
      req->short_not_ok = flag;
      t->state = UPSTREAM_STAT_UPLOADING;      
      if(rawbulk_usb_state_check()) {
           ret = usb_ep_queue(transfer->upstream.ep, req, GFP_ATOMIC);
           //printk("%s %d: after requeue\n", __func__, __LINE__);
      } else
           return;
      if(ret < 0) {
           terr(t, "fail to queue request, %d", ret);
           t->state = UPSTREAM_STAT_FREE;
           return;
      }  	    
      c->state = UPSTREAM_STAT_FREE; 
   }*/
}

static void upstream_complete(struct usb_ep *ep,
        struct usb_request *req) {
    struct upstream_transaction *t = req->context;
    struct rawbulk_transfer *transfer = t->transfer;
    
    t->state = UPSTREAM_STAT_FREE; 
           
    if (req->status < 0) {            
        /*if (req->status == -ESHUTDOWN)
            return;
        else
            terr(t, "req status %d", req->status);*/
        printk(" %s: req status %d\n", __func__, req->status);
        return;
    }

    if (!req->actual)
        terr(t, "req actual 0");
    queue_work(transfer->tx_wq, &transfer->write_work);
}

static void stop_upstream(struct upstream_transaction *t) {
    struct rawbulk_transfer *transfer = t->transfer;
    
    if (t->state == UPSTREAM_STAT_UPLOADING) {
        usb_ep_dequeue(transfer->upstream.ep, t->req);
    }
    t->state = UPSTREAM_STAT_FREE;
}

int rawbulk_push_upstream_buffer(int transfer_id, const void *buffer,
        unsigned int length) {
    int ret = -ENOENT;
    struct rawbulk_transfer *transfer;
    int count = length;
    struct cache_buf *c;
    
    if(transfer_id > 2)
         transfer_id--;
    else if(transfer_id == 2) {
         lerr("channal 2 is flashless, no nessesory to bypass \n");
         return 0;
    }
    
    ldbg("%s:transfer_id = %d, length = %d\n", __func__, transfer_id, length);

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return -ENODEV;
    
    mutex_lock(&transfer->modem_up_mutex);
        list_for_each_entry(c, &transfer->cache_buf_lists.transactions, clist) {
            if (c && (c->state == UPSTREAM_STAT_FREE) && !(transfer->control & STOP_UPSTREAM)) {
                list_move_tail(&c->clist, &transfer->cache_buf_lists.transactions);
                ret = 0;
                break;
            }
        }
    mutex_unlock(&transfer->modem_up_mutex);
    if (ret < 0) {
            lerr("sdio cahce is fulll\n");
            return -ENOMEM;
    }    

    memcpy(c->buffer, buffer, count);  
    c->state= UPSTREAM_STAT_UPLOADING;
    c->length = count;
    dump_data(transfer, "pushing up", c->buffer, count);
    queue_work(transfer->tx_wq, &transfer->write_work);
    return count;
}
EXPORT_SYMBOL_GPL(rawbulk_push_upstream_buffer);

/*
 * downstream
 */

#define DOWNSTREAM_STAT_FREE        0
#define DOWNSTREAM_STAT_DOWNLOADING 2

struct downstream_transaction {
    int state;
    int stalled;
    char name[32];
    struct list_head tlist;
    struct rawbulk_transfer *transfer;
    struct usb_request *req;
    int buffer_length;
    //unsigned char buffer[0];
    char *buffer;
};

static void downstream_delayed_work(struct work_struct *work);

static void downstream_complete(struct usb_ep *ep,
        struct usb_request *req);

static struct downstream_transaction *alloc_downstream_transaction(
        struct rawbulk_transfer *transfer, int bufsz) {
    struct downstream_transaction *t;
    
    //t = kzalloc(sizeof *t + bufsz * sizeof(unsigned char), GFP_ATOMIC);
    t = kmalloc(sizeof(struct downstream_transaction), GFP_ATOMIC);
    if (!t)
        return NULL;
        
    t->buffer = (char *)__get_free_page(GFP_ATOMIC);
    //t->buffer = kmalloc(bufsz, GFP_ATOMIC);
    if (!t->buffer) {
        kfree(t);
        return NULL;
    }
    t->buffer_length = bufsz;
    t->req = usb_ep_alloc_request(transfer->downstream.ep, GFP_ATOMIC);
    if (!t->req)
        goto failto_alloc_usb_request;

    t->name[0] = 0;

    INIT_LIST_HEAD(&t->tlist);
    list_add_tail(&t->tlist, &transfer->downstream.transactions);
   
    transfer->downstream.ntrans ++;
    t->transfer = transfer;
    t->state = DOWNSTREAM_STAT_FREE;
    t->stalled = 0;
    t->req->context = t;
    
    return t;

failto_alloc_usb_request:
    //kfree(t->buffer);
    free_page((unsigned long) t->buffer);
    kfree(t);
    return NULL;
}

static void free_downstream_transaction(struct rawbulk_transfer *transfer) {
    struct list_head *p, *n;
    unsigned long flags;
    
    spin_lock_irqsave(&transfer->usb_down_lock, flags);
    list_for_each_safe(p, n, &transfer->downstream.transactions) {
        struct downstream_transaction *t = list_entry(p, struct
                downstream_transaction, tlist);

        list_del(p);
        //kfree(t->buffer);
        if(t->buffer)/*NULL pointer when ETS switch*/
            free_page((unsigned long) t->buffer);
        usb_ep_free_request(transfer->downstream.ep, t->req);
        kfree(t);

        transfer->downstream.ntrans --;
    }
    spin_unlock_irqrestore(&transfer->usb_down_lock, flags);
}

static void stop_downstream(struct downstream_transaction *t) {
    struct rawbulk_transfer *transfer = t->transfer;
    
    if (t->state == DOWNSTREAM_STAT_DOWNLOADING) {
        usb_ep_dequeue(transfer->downstream.ep, t->req);
        t->state = DOWNSTREAM_STAT_FREE;
    }
}

static int queue_downstream(struct downstream_transaction *t) {
    int rc=0;
    struct rawbulk_transfer *transfer = t->transfer;
    struct usb_request *req = t->req;
    
    req->buf = t->buffer;
    req->length = t->buffer_length;
    req->complete = downstream_complete;
    //if (rawbulk_usb_state_check())
        rc = usb_ep_queue(transfer->downstream.ep, req, GFP_ATOMIC);
    //else
    //    return;
    if (rc < 0) {
        return rc;
    }

    t->state = DOWNSTREAM_STAT_DOWNLOADING;
    return 0;
}

static int start_downstream(struct downstream_transaction *t) {
    int rc=0;
    struct rawbulk_transfer *transfer = t->transfer;
    struct usb_request *req = t->req;
    int time_delayed = msecs_to_jiffies(1);
       
    if (transfer->control & STOP_DOWNSTREAM) {
        //t->state = DOWNSTREAM_STAT_FREE;
        return -EPIPE;
    }
    rc = modem_buffer_push(transfer->id, t->req->buf,  t->req->actual);
    if (rc < 0) {		   
          if(rc == -ENOMEM) {
		spin_lock(&transfer->modem_block_lock);	       
		transfer->sdio_block = 1;
		spin_unlock(&transfer->modem_block_lock);	         
		spin_lock(&transfer->usb_down_lock);		         
        list_move_tail(&t->tlist, &transfer->repush2modem.transactions);
        spin_unlock(&transfer->usb_down_lock);
        transfer->repush2modem.ntrans++;
        transfer->downstream.ntrans--;
		queue_delayed_work(transfer->flow_wq, &transfer->delayed, time_delayed);
		return -EPIPE;
	} else
		return -EPIPE;
    }

    req->buf = t->buffer;
    req->length = t->buffer_length;
    req->complete = downstream_complete;
    //if (rawbulk_usb_state_check())
        rc = usb_ep_queue(transfer->downstream.ep, req, GFP_ATOMIC);
    //else
    //    return;
    if (rc < 0) {
        terr(t, "fail to queue request, %d", rc);
        return rc;
    }

    t->state = DOWNSTREAM_STAT_DOWNLOADING;
    return 0;
}

static void downstream_complete(struct usb_ep *ep,
        struct usb_request *req) {
   
    //struct downstream_transaction *t = container_of(req->buf,
    //        struct downstream_transaction, buffer);
    
    //struct downstream_transaction *t = container_of(req->buf,
    //    struct downstream_transaction, buffer);
    struct downstream_transaction *t = req->context;
    struct rawbulk_transfer *transfer = t->transfer;
    
    t->state = DOWNSTREAM_STAT_FREE;
    
    if (req->status < 0) {
        /*if (req->status == -ESHUTDOWN)
            return;
        else
            terr(t, "req status %d", req->status);*/
        printk(" %s: req status %d\n", __func__, req->status);
        return;             
    }

    dump_data(transfer, "downstream", t->buffer, req->actual); 
    
    spin_lock(&transfer->modem_block_lock);   
    if(!!transfer->sdio_block) {
                spin_unlock(&transfer->modem_block_lock);
                
                spin_lock(&transfer->usb_down_lock);
                list_move_tail(&t->tlist, &transfer->repush2modem.transactions);
                spin_unlock(&transfer->usb_down_lock);
                transfer->repush2modem.ntrans++;
                transfer->downstream.ntrans--;
                return;
    } else{
        spin_unlock(&transfer->modem_block_lock); 
        start_downstream(t);
    }       
}

static void downstream_delayed_work(struct work_struct *work) {
    int rc = 0;
    unsigned long flags;

    struct downstream_transaction *downstream , *downstream_copy;
    struct usb_request *req;
    int time_delayed = msecs_to_jiffies(1);
    
    struct  rawbulk_transfer *transfer = container_of (work, struct
            rawbulk_transfer, delayed.work);
    
    spin_lock_irqsave(&transfer->usb_down_lock, flags);
    list_for_each_entry_safe(downstream, downstream_copy, &transfer->repush2modem.transactions, tlist) {
        spin_unlock_irqrestore(&transfer->usb_down_lock, flags);

        rc = modem_buffer_push(transfer->id, downstream->req->buf,  downstream->req->actual);
        if (rc < 0) {
             if ( rc != -ENOMEM){
                 terr(downstream, "port is not presence\n");
             }             
             if (!(transfer->control & STOP_DOWNSTREAM)) { 
                queue_delayed_work(transfer->flow_wq, &transfer->delayed, time_delayed);
             }
             return;
        }
        spin_lock_irqsave(&transfer->usb_down_lock, flags);
	    list_move_tail(&downstream->tlist, &transfer->downstream.transactions);
        spin_unlock_irqrestore(&transfer->usb_down_lock, flags);
        downstream->stalled = 0;
        downstream->state = DOWNSTREAM_STAT_FREE;
        
        req = downstream->req;
        req->buf = downstream->buffer;
        req->length = downstream->buffer_length;
        req->complete = downstream_complete;
        //if (rawbulk_usb_state_check())
            rc = usb_ep_queue(transfer->downstream.ep, req, GFP_ATOMIC);
        //else
        //    return;
        if (rc < 0) {
            terr(downstream, "fail to queue request, %d", rc);
            downstream->stalled = 1;
            return;
        }
        downstream->state = DOWNSTREAM_STAT_DOWNLOADING;
        transfer->repush2modem.ntrans--;
        transfer->downstream.ntrans++;
        spin_lock_irqsave(&transfer->usb_down_lock, flags);
    }
    spin_unlock_irqrestore(&transfer->usb_down_lock, flags);
      
    spin_lock_irqsave(&transfer->modem_block_lock, flags);       	
    transfer->sdio_block = 0;
    spin_unlock_irqrestore(&transfer->modem_block_lock, flags);
}

int rawbulk_start_transactions(int transfer_id, int nups, int ndowns, int upsz,
        int downsz) {
    int n;
    int rc, ret;
    unsigned long flags;
    struct rawbulk_transfer *transfer;
    struct upstream_transaction *upstream; //upstream_copy;
    struct downstream_transaction *downstream, *downstream_copy;
    struct cache_buf  *c;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return -ENODEV;

    if (!rawbulk->cdev)
        return -ENODEV;
        
    if (!transfer->function)
        return -ENODEV;

    ldbg("start transactions on id %d, nups %d ndowns %d upsz %d downsz %d\n",
            transfer_id, nups, ndowns, upsz, downsz);
    
    /* stop host transfer 1stly */
    ret = sdio_rawbulk_intercept(transfer->id, 1);    
    if(ret < 0) {
        lerr("bypass sdio failed, channel id = %d\n", transfer->id);
        return ret;
    }    
    transfer->sdio_block = 0;
    
	spin_lock(&transfer->flow_lock);
	transfer->down_flow = 0;
	spin_unlock(&transfer->flow_lock);
	
    mutex_lock(&transfer->usb_up_mutex);
    for (n = 0; n < nups; n ++) {
        upstream = alloc_upstream_transaction(transfer, upsz);
        if (!upstream) {
            rc = -ENOMEM;
            mutex_unlock(&transfer->usb_up_mutex);
            ldbg("fail to allocate upstream transaction n %d", n);
            goto failto_alloc_upstream;
        }
    }
    mutex_unlock(&transfer->usb_up_mutex);
    
    
    mutex_lock(&transfer->modem_up_mutex);
    for (n = 0; n < 6*nups; n++) {        
        //c = kzalloc(sizeof *c + upsz * sizeof(unsigned char), GFP_KERNEL);
        c = kmalloc(sizeof(struct cache_buf), GFP_KERNEL);
        if (!c) {
            rc = -ENOMEM;
            mutex_unlock(&transfer->modem_up_mutex);
            ldbg("fail to allocate upstream sdio buf n %d", n);
            goto failto_alloc_up_sdiobuf;
        }
        
        c->buffer = (char *)__get_free_page(GFP_KERNEL);
        //c->buffer = kmalloc(upsz, GFP_KERNEL);
        if (!c) {
            rc = -ENOMEM;
            kfree(c);
            mutex_unlock(&transfer->modem_up_mutex);
            ldbg("fail to allocate upstream sdio buf n %d", n);
            goto failto_alloc_up_sdiobuf;
        }
        c->state = UPSTREAM_STAT_FREE;
        INIT_LIST_HEAD(&c->clist);
        list_add_tail(&c->clist, &transfer->cache_buf_lists.transactions);
        transfer->cache_buf_lists.ntrans++;
    }
    mutex_unlock(&transfer->modem_up_mutex);
    
    spin_lock_irqsave(&transfer->usb_down_lock, flags);
    for (n = 0; n < ndowns; n ++) {
        downstream = alloc_downstream_transaction(transfer, downsz);
        if (!downstream) {
            rc = -ENOMEM;
            spin_unlock_irqrestore(&transfer->usb_down_lock, flags);
            ldbg("fail to allocate downstream transaction n %d", n);
            goto failto_alloc_downstream;
        }
    }

    transfer->control &= ~STOP_UPSTREAM;
    transfer->control &= ~STOP_DOWNSTREAM;
         
    list_for_each_entry_safe(downstream, downstream_copy, &transfer->downstream.transactions, tlist) {
        if (downstream->state == DOWNSTREAM_STAT_FREE && !downstream->stalled) {
            rc = queue_downstream(downstream);
            if (rc < 0) {
                spin_unlock_irqrestore(&transfer->usb_down_lock, flags);
                ldbg("fail to start downstream %s rc %d\n", downstream->name, rc);
                goto failto_start_downstream;
            }
        }
    }
    spin_unlock_irqrestore(&transfer->usb_down_lock, flags);
    return 0;

failto_start_downstream:
    spin_lock_irqsave(&transfer->usb_down_lock, flags); 
    list_for_each_entry(downstream, &transfer->downstream.transactions, tlist)
        stop_downstream(downstream);
    spin_unlock_irqrestore(&transfer->usb_down_lock, flags);                
failto_alloc_up_sdiobuf:
    free_upstream_sdio_buf(transfer);         
failto_alloc_downstream:  
    free_downstream_transaction(transfer);   
failto_alloc_upstream:
    free_upstream_transaction(transfer);    
    /* recover host transfer */
    sdio_rawbulk_intercept(transfer->id, 0);
    return rc;
}

EXPORT_SYMBOL_GPL(rawbulk_start_transactions);

void rawbulk_stop_transactions(int transfer_id) {
    unsigned long flags;
    struct rawbulk_transfer *transfer;
    struct upstream_transaction *upstream;
    struct downstream_transaction *downstream, *downstream_copy;
    struct list_head *p, *n;
    
    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return;
    if(transfer->control)
         return;
        
    spin_lock(&transfer->lock);
    transfer->control |= (STOP_UPSTREAM | STOP_DOWNSTREAM);
    spin_unlock(&transfer->lock);
    
    sdio_rawbulk_intercept(transfer->id, 0);
    
    cancel_delayed_work(&transfer->delayed);
    flush_workqueue(transfer->flow_wq);
    flush_workqueue(transfer->tx_wq);
        
    list_for_each_entry(upstream, &transfer->upstream.transactions, tlist) {
        stop_upstream(upstream);
    }   
    free_upstream_transaction(transfer);

    free_upstream_sdio_buf(transfer);
         
    list_for_each_entry_safe(downstream, downstream_copy, &transfer->downstream.transactions, tlist) {
        stop_downstream(downstream);
    } 

    spin_lock_irqsave(&transfer->usb_down_lock, flags);
    list_for_each_safe(p, n, &transfer->repush2modem.transactions) {
        struct downstream_transaction *delayed_t = list_entry(p, struct
                downstream_transaction, tlist);                               
        list_move_tail(&delayed_t->tlist, &transfer->downstream.transactions);
    }
    spin_unlock_irqrestore(&transfer->usb_down_lock, flags);
    
    spin_lock_irqsave(&transfer->modem_block_lock, flags);       	
    transfer->sdio_block = 0;
    spin_unlock_irqrestore(&transfer->modem_block_lock, flags);
    
    free_downstream_transaction(transfer);   
   
}

EXPORT_SYMBOL_GPL(rawbulk_stop_transactions);

static char *state2string(int state, int upstream) {
    if (upstream) {
        switch (state) {
            case UPSTREAM_STAT_FREE:
                return "FREE";
            case UPSTREAM_STAT_UPLOADING:
                return "UPLOADING";
            default:
                return "UNKNOW";
        }
    } else {
        switch (state) {
            case DOWNSTREAM_STAT_FREE:
                return "FREE";
            case DOWNSTREAM_STAT_DOWNLOADING:
                return "DOWNLOADING";
            default:
                return "UNKNOW";
        }
    }
}

int rawbulk_transfer_statistics(int transfer_id, char *buf) {
    char *pbuf = buf;
    struct rawbulk_transfer *transfer;
    struct upstream_transaction *upstream;
    struct downstream_transaction *downstream;
    struct cache_buf *c;
    unsigned long flags;
    
    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return sprintf(pbuf, "-ENODEV, id %d\n", transfer_id);

    pbuf += sprintf(pbuf, "rawbulk statistics:\n");
    if (rawbulk->cdev && rawbulk->cdev->config)
        pbuf += sprintf(pbuf, " gadget device: %s\n",
                rawbulk->cdev->config->label);
    else
        pbuf += sprintf(pbuf, " gadget device: -ENODEV\n");
    pbuf += sprintf(pbuf, " upstreams (total %d transactions)\n",
            transfer->upstream.ntrans);
    mutex_lock(&transfer->usb_up_mutex);        
    list_for_each_entry(upstream, &transfer->upstream.transactions, tlist) {
        pbuf += sprintf(pbuf, "  %s state: %s", upstream->name,
                state2string(upstream->state, 1));
        pbuf += sprintf(pbuf, ", maxbuf: %d bytes", upstream->buffer_length);
        if (upstream->stalled)
            pbuf += sprintf(pbuf, " (stalled!)");
        pbuf += sprintf(pbuf, "\n");
    }
    mutex_unlock(&transfer->usb_up_mutex);
    
    pbuf += sprintf(pbuf, " cache_buf_lists (total %d transactions)\n",
            transfer->cache_buf_lists.ntrans);
    mutex_lock(&transfer->modem_up_mutex);        
    list_for_each_entry(c, &transfer->cache_buf_lists.transactions, clist) {        
        pbuf += sprintf(pbuf, "  %s state:", state2string(c->state, 1));
        pbuf += sprintf(pbuf, ", maxbuf: %d bytes", c->length);
        pbuf += sprintf(pbuf, "\n");
    }
    mutex_unlock(&transfer->modem_up_mutex);
    
    pbuf += sprintf(pbuf, " downstreams (total %d transactions)\n",
            transfer->downstream.ntrans);
    spin_lock_irqsave(&transfer->usb_down_lock, flags);        
    list_for_each_entry(downstream, &transfer->downstream.transactions, tlist) {
        spin_unlock_irqrestore(&transfer->usb_down_lock, flags);
        pbuf += sprintf(pbuf, "  %s state: %s", downstream->name,
                state2string(downstream->state, 0));
        pbuf += sprintf(pbuf, ", maxbuf: %d bytes", downstream->buffer_length);
        if (downstream->stalled)
            pbuf += sprintf(pbuf, " (stalled!)");
        pbuf += sprintf(pbuf, "\n");
        spin_lock_irqsave(&transfer->usb_down_lock, flags);
    }
    spin_unlock_irqrestore(&transfer->usb_down_lock, flags);
    
    pbuf += sprintf(pbuf, " repush2modem (total %d transactions)\n",
            transfer->downstream.ntrans);
    spin_lock_irqsave(&transfer->usb_down_lock, flags);        
    list_for_each_entry(downstream, &transfer->repush2modem.transactions, tlist) {
        spin_unlock_irqrestore(&transfer->usb_down_lock, flags);
        pbuf += sprintf(pbuf, "  %s state: %s", downstream->name,
                state2string(downstream->state, 0));
        pbuf += sprintf(pbuf, ", maxbuf: %d bytes", downstream->buffer_length);
        if (downstream->stalled)
            pbuf += sprintf(pbuf, " (stalled!)");
        pbuf += sprintf(pbuf, "\n");
        spin_lock_irqsave(&transfer->usb_down_lock, flags);
    }
    spin_unlock_irqrestore(&transfer->usb_down_lock, flags);
    
    return (int)(pbuf - buf);
}

EXPORT_SYMBOL_GPL(rawbulk_transfer_statistics);

int rawbulk_bind_function(int transfer_id, struct usb_function *function, struct
        usb_ep *bulk_out, struct usb_ep *bulk_in, 
        rawbulk_autoreconn_callback_t autoreconn_callback) {
            
    struct rawbulk_transfer *transfer;
    
    if (!function || !bulk_out || !bulk_in)
        return -EINVAL;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return -ENODEV;

    transfer->downstream.ep = bulk_out;
    transfer->upstream.ep = bulk_in;
    transfer->function = function;
    rawbulk->cdev = function->config->cdev;

    transfer->autoreconn = autoreconn_callback;
    return 0;
}

EXPORT_SYMBOL_GPL(rawbulk_bind_function);

void rawbulk_unbind_function(int transfer_id) {
    int n;
    int no_functions = 1;
    struct rawbulk_transfer *transfer;

    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return;

    rawbulk_stop_transactions(transfer_id);
    transfer->downstream.ep = NULL;
    transfer->upstream.ep = NULL;
    transfer->function = NULL;

    for (n = 0; n < _MAX_TID; n ++) {
        if (!!rawbulk->transfer[n].function)
            no_functions = 0;
    }

    if (no_functions)
        rawbulk->cdev = NULL;
}

EXPORT_SYMBOL_GPL(rawbulk_unbind_function);

int rawbulk_bind_sdio_channel(int transfer_id) {  
    struct rawbulk_transfer *transfer;
    struct rawbulk_function *fn;
    
    ldbg("%d\n", transfer_id);
    
    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return -ENODEV;
    fn = rawbulk_lookup_function(transfer_id);
    if(fn)
        fn->cbp_reset = 0;
    if (transfer->autoreconn)
        transfer->autoreconn(transfer->id);
    return 0;
}

EXPORT_SYMBOL_GPL(rawbulk_bind_sdio_channel);

void rawbulk_unbind_sdio_channel(int transfer_id) {
    struct rawbulk_transfer *transfer;
    struct rawbulk_function *fn;
    
    ldbg("%d\n", transfer_id);
    transfer = id_to_transfer(transfer_id);
    if (!transfer)
        return;
    rawbulk_stop_transactions(transfer_id);
    fn = rawbulk_lookup_function(transfer_id);
    if(fn) {
        fn->cbp_reset = 1;
        rawbulk_disable_function(fn);
    }
}

EXPORT_SYMBOL_GPL(rawbulk_unbind_sdio_channel);

static __init int rawbulk_init(void) {
    int n;
    char name[20];
    
    rawbulk = kzalloc(sizeof *rawbulk, GFP_KERNEL);
    if (!rawbulk)
        return -ENOMEM;
   
    for (n = 0; n < _MAX_TID; n ++) {
        struct rawbulk_transfer *t = &rawbulk->transfer[n];

        t->id = n;
        INIT_LIST_HEAD(&t->upstream.transactions);
        INIT_LIST_HEAD(&t->downstream.transactions);
        INIT_LIST_HEAD(&t->repush2modem.transactions);      
        INIT_LIST_HEAD(&t->cache_buf_lists.transactions);      
        INIT_DELAYED_WORK(&t->delayed, downstream_delayed_work);
        memset(name, 0, 20);
        sprintf(name, "%s_flow_ctrl", transfer_name[n]);
        t->flow_wq = create_singlethread_workqueue(name);
        if (!t->flow_wq)
	        return -ENOMEM;
	    	      
	    INIT_WORK(&t->write_work, start_upstream);
	    memset(name, 0, 20);
	    sprintf(name, "%s_tx_wq", transfer_name[n]);
        t->tx_wq = create_singlethread_workqueue(name);
        if (!t->tx_wq)
	        return -ENOMEM;
	                   
        mutex_init(&t->modem_up_mutex);
        mutex_init(&t->usb_up_mutex);
        spin_lock_init(&t->lock);
        spin_lock_init(&t->usb_down_lock);
        spin_lock_init(&t->modem_block_lock);
        spin_lock_init(&t->flow_lock);
        
        t->control = STOP_UPSTREAM | STOP_DOWNSTREAM;
    }

    return 0;
}

module_init (rawbulk_init);

static __exit void rawbulk_exit(void) {
    int n;
    struct rawbulk_transfer *t;
    for (n = 0; n < _MAX_TID; n ++) {
        t = &rawbulk->transfer[n];
        rawbulk_stop_transactions(n);
        destroy_workqueue(t->flow_wq);
        destroy_workqueue(t->tx_wq);
    }
    kfree(rawbulk);
}

module_exit (rawbulk_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

