#ifndef _MT_SCHED_IOCTL_H
#define _MT_SCHED_IOCTL_H

struct ioctl_arg {
        pid_t pid;
        unsigned int len;
        unsigned long *mask;
        unsigned long *mt_mask;
};

#define IOC_MAGIC '\x66'

#define IOCTL_SETAFFINITY	_IOW(IOC_MAGIC, 0, struct ioctl_arg)
#define IOCTL_EXITAFFINITY  	_IOW(IOC_MAGIC, 1, pid_t)
#define IOCTL_GETAFFINITY      	_IOR(IOC_MAGIC, 2, struct ioctl_arg)

#endif