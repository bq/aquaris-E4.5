#include "ion_priv.h"
#include "ion_debug.h"
#include <linux/slab.h>
#include <linux/stacktrace.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <linux/dma-buf.h>
#include <linux/fdtable.h>
#include <linux/fs.h>
extern unsigned int ion_debugger;
extern unsigned int ion_log_level;
extern unsigned int ion_debugger_history; 
extern unsigned int ion_log_limit;
#define ION_DEBUG_LOG(LOG_LEVEL,fmt,...) {\
	if(ion_log_level >= LOG_LEVEL)\
	{\
		printk(fmt,##__VA_ARGS__);\
	}\
}
void *get_record(unsigned int type, ion_sys_record_t *param);
int remove_record(unsigned int node_type,unsigned int *remove_reocrd);

ObjectTable gUserBtTable;
ObjectTable gKernelBtTable;
ObjectTable gUserMappingTable;
ObjectTable gKernelSymbolTable;
ObjectTable gBufferTable;
StringTable gMappingStringTable;
StringTable gKernelPathTable;

struct kmem_cache *ion_client_usage_cachep = NULL;
struct kmem_cache *ion_buffer_usage_cachep = NULL;
struct kmem_cache *ion_address_usage_cachep = NULL;
struct kmem_cache *ion_fd_usage_cachep = NULL;
struct kmem_cache *ion_list_buffer_cachep = NULL;
struct kmem_cache *ion_list_process_cachep = NULL;
unsigned int buffer_node_size = 0;
unsigned int process_node_size = 0;
unsigned int list_buffer_size = 0;
unsigned int fd_node_size = 0;
unsigned int client_node_size = 0;
unsigned int mmap_node_size = 0;
unsigned int total_size = 0;

unsigned int list_buffer_cache_created = false;
unsigned int list_process_cache_created = false;
unsigned int buffer_cache_created = false;
unsigned int fd_cache_created = false;
unsigned int address_cache_created = false;
unsigned int client_cache_created = false;
struct ion_client_usage_record *client_using_list = NULL;
struct ion_client_usage_record *client_freed_list = NULL;
struct ion_buffer_record *buffer_created_list = NULL;
struct ion_buffer_record *buffer_destroyed_list = NULL;
unsigned int destroyed_buffer_count = 0;
DEFINE_MUTEX(gUserBtTable_mutex);
DEFINE_MUTEX(gKernelBtTable_mutex);
DEFINE_MUTEX(gMappingStringTable_mutex);
DEFINE_MUTEX(gKernelPathTable_mutex);
DEFINE_MUTEX(client_usage_mutex);
DEFINE_MUTEX(buffer_lifecycle_mutex);
DEFINE_MUTEX(process_lifecycle_mutex);
struct ion_process_record *process_created_list = NULL;
struct ion_process_record *process_destroyed_list = NULL;
void disable_ion_debugger(void)
{
	ion_debugger = 0;	
}
int ion_debug_show_backtrace(struct ion_record_basic_info *tracking_info,unsigned int show_backtrace_type)
{
	unsigned int i = 0;
	unsigned int backtrace_count = 0;
	ObjectEntry *tmp = NULL;
	unsigned int stringCount = KSYM_SYMBOL_LEN+30;	
	if(tracking_info == NULL)
	{
		return 0;		
	}
	if(show_backtrace_type == ALLOCATE_BACKTRACE_INFO)
	{
		tmp = (ObjectEntry *)tracking_info->allocate_backtrace;	
		if(tmp == NULL)
			return 0;
		backtrace_count = tmp->numEntries;
	}
	else if(show_backtrace_type == RELEASE_BACKTRACE_INFO)
	{
		tmp = (ObjectEntry *)tracking_info->release_backtrace;
		if(tmp == NULL)
			return 0;
                backtrace_count = tmp->numEntries;
	}

	for(i = 0;i < backtrace_count;i++)
	{
		char tmpString[stringCount];
		ion_get_backtrace_info(tracking_info,tmpString,stringCount,i,show_backtrace_type);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "%s",tmpString);
	}
	return 1;	
}
void ion_debug_show_basic_info_record(struct ion_record_basic_info *tracking_info)
{
	if(tracking_info != NULL)
	{
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===recordID.pid : %d\n",tracking_info->recordID.pid);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===recordID.group_pid : %d\n",tracking_info->recordID.group_pid); 
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===recordID.client_address : 0x%x\n",(unsigned int)tracking_info->recordID.client_address);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===recordID.client: 0x%x\n",(unsigned int)tracking_info->recordID.client);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===recordID.buffer : 0x%x\n",(unsigned int)tracking_info->recordID.buffer);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===record type  : %d\n",tracking_info->record_type);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===from_kernel  : %d\n",tracking_info->from_kernel);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===allocate_backtrace : 0x%x\n",(unsigned int)tracking_info->allocate_backtrace);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===allocate_map : 0x%x\n",(unsigned int)tracking_info->allocate_map);
		ion_debug_show_backtrace(tracking_info,ALLOCATE_BACKTRACE_INFO);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===release_backtrace : 0x%x\n",(unsigned int)tracking_info->release_backtrace);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===release_map : 0x%x\n",(unsigned int)tracking_info->release_map);
		ion_debug_show_backtrace(tracking_info,RELEASE_BACKTRACE_INFO);
	}
}
void ion_debug_show_buffer_usage_record(struct ion_buffer_usage_record *buffer_usage_record)
{
	if(buffer_usage_record != NULL)
	{
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===========================================\n");
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===buffer usage record : %x\n",(unsigned int)buffer_usage_record);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===buffer_usage_record.next : 0x%x\n",(unsigned int)buffer_usage_record->next);
		ion_debug_show_basic_info_record(&(buffer_usage_record->tracking_info));
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===buffer_usage_record.handle : 0x%x\n",(unsigned int)buffer_usage_record->handle);	
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===========================================\n");	
	}
}

void ion_debug_show_address_usage_record(struct ion_address_usage_record *address_usage_record)
{
	if(address_usage_record != NULL)
	{
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===========================================\n");
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===address usage record : %x\n",(unsigned int)address_usage_record);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===address_usage_record.next : 0x%x\n",(unsigned int)address_usage_record->next);
        	ion_debug_show_basic_info_record(&(address_usage_record->tracking_info));
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===address_usage_record.address_type : %x\n",address_usage_record->address_type);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===address_usage_record.mapping_address : 0x%x\n",(unsigned int)address_usage_record->mapping_address);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===address_usage_record.size : %x\n",address_usage_record->size);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===address_usage_record.fd : 0x%x\n",address_usage_record->fd);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===========================================\n");
	}
}

void ion_debug_show_fd_usage_record(struct ion_fd_usage_record *fd_usage_record)
{
	if(fd_usage_record != NULL)
	{
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===========================================\n");
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===fd usage record : %x\n",(unsigned int)fd_usage_record);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===fd_usage_record.next : 0x%x\n",(unsigned int)fd_usage_record->next);
        	ion_debug_show_basic_info_record(&(fd_usage_record->tracking_info));
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===fd_usage_record.fd : 0x%d\n",fd_usage_record->fd);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===========================================\n");
	}
}

void ion_debug_show_client_usage_record(struct ion_client_usage_record *client_usage_record)
{
	if(client_usage_record != NULL)
	{	
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===========================================\n");
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===client usage record : %x\n",(unsigned int)client_usage_record);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===client_sage_record.next : 0x%x\n",(unsigned int)client_usage_record->next);
        	ion_debug_show_basic_info_record(&(client_usage_record->tracking_info));
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===client_usage_record.fd : 0x%d\n",client_usage_record->fd);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===========================================\n");
	}
}
void ion_debug_show_process_record(struct ion_process_record *process_record)
{
	if(process_record != NULL)
	{
		ION_DEBUG_LOG(ION_DEBUG_INFO, "============================================\n");
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===process : %x\n",(unsigned int)process_record);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===process.next : %x\n",(unsigned int)process_record->next);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===process.count : %d\n",process_record->count);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===process.pid : %d\n",process_record->pid);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===process.group_id : %d\n",process_record->group_id);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===process.address_using_list : %x\n",(unsigned int)process_record->address_using_list);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===process.address_freed_list : %x\n",(unsigned int)process_record->address_freed_list);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===process.fd_using_list : %x\n",(unsigned int)process_record->fd_using_list);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===process.fd_freed_list : %x\n",(unsigned int)process_record->fd_freed_list);
	}
}
void ion_debug_show_buffer_record(struct ion_buffer_record *buffer_record)
{
	if(buffer_record != NULL)
	{
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===========================================\n");
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===buffer_record : %x\n",(unsigned int)buffer_record);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===buffer_record.next : 0x%x\n",(unsigned int)buffer_record->next);
		ION_DEBUG_LOG(ION_DEBUG_INFO, "===buffer_record.buffer : 0x%x\n",(unsigned int)buffer_record->buffer_address);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===buffer_record.heap_type : 0x%d\n",(unsigned int)buffer_record->heap_type);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===buffer_record.size : 0x%d\n",buffer_record->size);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===buffer_record.buffer_using_list : 0x%x\n",(unsigned int)buffer_record->buffer_using_list);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===buffer_record.buffer_freed_list : 0x%x\n",(unsigned int)buffer_record->buffer_freed_list);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===buffer_record.address_using_list : 0x%x\n",(unsigned int)buffer_record->address_using_list);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===buffer_record.address_freed_list : 0x%x\n",(unsigned int)buffer_record->address_freed_list);
        	ION_DEBUG_LOG(ION_DEBUG_INFO, "===========================================\n");
	}
}
char *ion_get_backtrace_info(struct ion_record_basic_info *tracking_info,char *backtrace_string,unsigned int backtrace_string_len, unsigned int backtrace_index,unsigned int show_backtrace_type)
{
	ObjectEntry *tmpBacktrace = NULL;
	ObjectEntry *tmpMapping = NULL;
	unsigned int backtrace_info = BACKTRACE_MAX;
	unsigned int *backtrace = NULL;
	if(tracking_info == NULL)
	{
		ION_DEBUG_LOG(ION_DEBUG_ERROR, "[ion_get_backtrace_info]ERROR input tracking_info is NULL\n");
		return NULL;
	}

	if(show_backtrace_type == ALLOCATE_BACKTRACE_INFO)		
	{
		tmpBacktrace = (ObjectEntry *)tracking_info->allocate_backtrace;
		tmpMapping = (ObjectEntry *)tracking_info->allocate_map;
		//backtrace_info = tracking_info->allocate_backtrace_type;
		if( tracking_info->from_kernel)
	        {
       		         backtrace_info = KERNEL_BACKTRACE;
        	}
        	else
        	{
                	backtrace_info = USER_BACKTRACE;
        	}
	}
	else if(show_backtrace_type == RELEASE_BACKTRACE_INFO)
	{
		tmpBacktrace = (ObjectEntry *)tracking_info->release_backtrace;
		tmpMapping = (ObjectEntry *)tracking_info->release_map;
		backtrace_info = tracking_info->release_backtrace_type;
 	}

	if((tmpBacktrace == NULL) || (tmpBacktrace->numEntries <= 0))
        {
        	ION_DEBUG_LOG(ION_DEBUG_ERROR, "[ion_get_backtrace_info]ERROR tmpBacktrace is NULL or tmpBacktrace->numEntries <=0\n");
		return NULL;
        }
	else
	{
		backtrace = (unsigned int *)tmpBacktrace->object;	
	}
	if(backtrace == NULL)
	{
		ION_DEBUG_LOG(ION_DEBUG_ERROR, "[ion_get_backtrace_info]ERROR backtrace is NULL\n");
		return NULL;
	}
	if(backtrace_info == USER_BACKTRACE)
        {
		if(tmpMapping != NULL)
		{
                	struct mapping *backtrace_mapping = (struct mapping *)tmpMapping->object;
			if(backtrace_mapping!= NULL)
			{
                		snprintf(backtrace_string,backtrace_string_len,"USER[%d] addr: 0x%x mapping address: 0x%x - 0x%x lib: %s\n",backtrace_index,backtrace[backtrace_index],backtrace_mapping[backtrace_index].address,(backtrace_mapping[backtrace_index].address + backtrace_mapping[backtrace_index].size-1),backtrace_mapping[backtrace_index].name);
				return backtrace_string;
			}	
			else
                	{
                       		ION_DEBUG_LOG(ION_DEBUG_ERROR, "[ion_get_backtrace_info]ERROR backtrace_mapping is NULL\n");
                	}
		}
		else
		{
			ION_DEBUG_LOG(ION_DEBUG_ERROR, "[ion_get_backtrace_info]ERROR tmpMapping is NULL\n");
		}
		snprintf(backtrace_string,backtrace_string_len,"USER[%d] addr: 0x%x\n",backtrace_index,backtrace[backtrace_index]);
        }
        else if(backtrace_info == KERNEL_BACKTRACE)
        {
		if(tmpMapping != NULL)
                {
			unsigned int *backtrace_symbol = (unsigned int *)tmpMapping->object;
			if(backtrace_symbol != NULL)	
			{
				snprintf(backtrace_string,backtrace_string_len,"KERNEL[%d] addr: 0x%x symbol: %s\n",backtrace_index,backtrace[backtrace_index],(char *)*(backtrace_symbol+backtrace_index));
				//snprintf(backtrace_string,backtrace_string_len,"KERNELSPACE BACKTRACE[%d]2 address: 0x%x \n",backtrace_index,backtrace[backtrace_index]);
	
				return backtrace_string;
			}
			else
                        {
                                ION_DEBUG_LOG(ION_DEBUG_ERROR, "[ion_get_backtrace_info]ERROR backtrace_mapping is NULL\n");
                        }
		}
		else
                {
                        ION_DEBUG_LOG(ION_DEBUG_ERROR, "[ion_get_backtrace_info]ERROR tmpMapping is NULL\n");
                }
		snprintf(backtrace_string,backtrace_string_len,"KERNEL[%d] addr: 0x%x \n",backtrace_index,backtrace[backtrace_index]);
        }
	return NULL;
}
void *ion_get_list(unsigned int record_type ,void  *record, unsigned int list_type)
{

	if(record_type == LIST_BUFFER)
	{
		struct ion_buffer_record *buffer_record = (struct ion_buffer_record *)record;
		switch(list_type)
		{
	   		case BUFFER_ALLOCATION_LIST:
	   		{
				ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_list return]buffer using list %x\n",(unsigned int)buffer_record->buffer_using_list);
				ion_debug_show_buffer_usage_record(buffer_record->buffer_using_list);	
				return  buffer_record->buffer_using_list;
			}
			case BUFFER_FREE_LIST:
			{
				ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_list return]buffer freed list %x\n",(unsigned int)buffer_record->buffer_freed_list);
				ion_debug_show_buffer_usage_record(buffer_record->buffer_freed_list);
				return  buffer_record->buffer_freed_list;
     			}
        		case ADDRESS_ALLOCATION_LIST:
			{
				ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_list return] address using list %x\n",(unsigned int)buffer_record->address_using_list);
				ion_debug_show_address_usage_record(buffer_record->address_using_list);
				return  buffer_record->address_using_list;
       	   		}
        		case ADDRESS_FREE_LIST:
	   		{
				ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_list return] address freed list %x\n",(unsigned int)buffer_record->address_freed_list);
				ion_debug_show_address_usage_record(buffer_record->address_freed_list);
				return  buffer_record->address_freed_list;
	   		}
		}
	}
	else if(record_type == LIST_PROCESS)
	{
		struct ion_process_record *process_record = (struct ion_process_record *)record;
		switch(list_type)
		{
           		case FD_ALLOCATION_LIST:
	   		{
				ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_list return]process fd using list %x\n",(unsigned int)process_record->fd_using_list);
				ion_debug_show_fd_usage_record(process_record->fd_using_list);
                		return process_record->fd_using_list;
           		}
           		case FD_FREE_LIST:
	   		{
				ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_list return]process fd free list %x\n",(unsigned int)process_record->fd_freed_list);
				ion_debug_show_fd_usage_record(process_record->fd_freed_list);
				return process_record->fd_freed_list; 
           		}
		        case ADDRESS_ALLOCATION_LIST:
                	{
                       		ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_list return]process address using list %x\n",(unsigned int)process_record->address_using_list);
                        	ion_debug_show_address_usage_record(process_record->address_using_list);
                        	return  process_record->address_using_list;
                	}
                	case ADDRESS_FREE_LIST:
                	{
                        	ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_list return]process  address freed list %x\n",(unsigned int)process_record->address_freed_list);
                        	ion_debug_show_address_usage_record(process_record->address_freed_list);
                        	return  process_record->address_freed_list;
                	}

		}
	}
	ION_DEBUG_LOG(ION_DEBUG_ERROR, "[ion_get_list_from_buffer]can't find corresponding record in list_type %d record_type %x\n",list_type,record_type );
	return NULL;
}
struct ion_buffer_record *ion_get_inuse_buffer_record(void)
{
       struct ion_buffer_record *tmp_buffer = buffer_created_list;
       ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_inuse_buffer_record]return buffer_record  %x\n",(unsigned int)tmp_buffer);
	if(tmp_buffer != NULL)
	{
		//ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_inuse_buffer_record]return buffer %x buffer_size %d\n",tmp_buffer->buffer,tmp_buffer->buffer->size);
		ion_debug_show_buffer_record(tmp_buffer);
	}
	return buffer_created_list;	
}
struct ion_buffer_record *ion_get_freed_buffer_record(void)
{
	struct ion_buffer_record *tmp_buffer = buffer_destroyed_list;
        ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_freed_buffer_record]return buffer_record  %x\n",(unsigned int)tmp_buffer);
        if(tmp_buffer != NULL)
        {
                //ION_DEBUG_LOG(ION_DEBUG_INFO,"[ion_get_freed_buffer_record]return buffer %x buffer_size %d\n",tmp_buffer->buffer,tmp_buffer->buffer->size);
		ion_debug_show_buffer_record(tmp_buffer);
        }

	return buffer_destroyed_list;
}
struct ion_process_record *ion_get_inuse_process_usage_record2(void)
{
       struct ion_process_record *tmp_process = process_created_list;
        if(tmp_process != NULL)
        {
		ION_DEBUG_LOG(ION_DEBUG_INFO,"[ion_get_inuse_process_record]return process_record  %x pid %d\n",(unsigned int)tmp_process,tmp_process->pid);
                ION_DEBUG_LOG(ION_DEBUG_INFO,"[ion_get_inuse_process_record]return process %x \n",(unsigned int)tmp_process);
                ion_debug_show_process_record(tmp_process);
        }
	else
	{
		ION_DEBUG_LOG(ION_DEBUG_WARN,"[ion_get_inuse_process_recoed] tmp_process is null process_created_list is %x\n",(unsigned int)process_created_list);
	}
        return process_created_list;
}
struct ion_process_record *ion_get_freed_process_record(void)
{
       struct ion_process_record *tmp_process = process_destroyed_list;
	ION_DEBUG_LOG(ION_DEBUG_INFO,"[ion_get_freed_process_record]return process_record  %x pid %d\n",(unsigned int)tmp_process,tmp_process->pid);
        if(tmp_process != NULL)
        {
                //ION_DEBUG_LOG(ION_DEBUG_INFO,"[ion_get_freed_process_record]return process %x  %d\n",tmp_process);
                ion_debug_show_process_record(tmp_process);
        }
        return process_destroyed_list;
}
struct ion_client_usage_record *ion_get_inuse_client_record(void)
{
       struct ion_client_usage_record *tmp_client = client_using_list;
       ION_DEBUG_LOG(ION_DEBUG_INFO,"[ion_get_inuse_client_record]return client_record  %x\n",(unsigned int)tmp_client);
        if(tmp_client != NULL)
        {
                ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_inuse_client_record]return client %x \n",tmp_client);
                ion_debug_show_client_usage_record(tmp_client);
        }
        return client_using_list;
}
struct ion_client_usage_record *ion_get_freed_client_record(void)
{
       struct ion_client_usage_record *tmp_client = client_freed_list;
	ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_inuse_client_record]return client_record  %x\n",(unsigned int)tmp_client);
        if(tmp_client != NULL)
        {
                ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_inuse_client_record]return client %x \n",tmp_client);
                ion_debug_show_client_usage_record(tmp_client);
        }
        return client_freed_list;
}

unsigned int ion_get_data_from_record(void *record,unsigned int data_type)
{
	unsigned int *tmp_record = record;
	struct ion_record_basic_info *tracking_info = (struct ion_record_basic_info *)(tmp_record+1);
	switch(data_type)
	{
		case RECORD_ID:
		{
			return (unsigned int)&(tracking_info->recordID);
		}
		case RECORD_CLIENT:
		{
			return (unsigned int)tracking_info->recordID.client;
		}
		case RECORD_HANDLE:
		{
			if(tracking_info->record_type == NODE_BUFFER)
			{
				struct ion_buffer_usage_record *buffer_node = (struct ion_buffer_usage_record *)record;
				return (unsigned int)buffer_node->handle;
			}
		}
   		case RECORD_ALLOCATE_BACKTRACE_NUM:
		{
			ObjectEntry *tmp =(ObjectEntry *)tracking_info->allocate_backtrace;
			return tmp->numEntries;
		}
   		case RECORD_FREED_BACKTRACE_NUM:
		{
			ObjectEntry *tmp =(ObjectEntry *)tracking_info->release_backtrace;		
			return tmp->numEntries;
		}
   		case RECORD_ALLOCATE_MAPPING_NUM:
		{
			ObjectEntry *tmp =(ObjectEntry *)tracking_info->allocate_map;
			return tmp->numEntries;
		}
   		case RECORD_FREED_MAPPING_NUM:
		{
			ObjectEntry *tmp =(ObjectEntry *)tracking_info->release_map;	
			return tmp->numEntries;
		}
   		case RECORD_FD:
		{
			if(tracking_info->record_type == NODE_FD)
                        {
				struct ion_fd_usage_record *fd_node = (struct ion_fd_usage_record *)record;
				return (unsigned int)fd_node->fd;
			}
			else if(tracking_info->record_type == NODE_MMAP)
			{
				struct ion_address_usage_record *address_node = (struct ion_address_usage_record *)record;
                                return (unsigned int)address_node->fd;
			}
		}
   		case RECORD_ADDRESS:
		{
			if( tracking_info->record_type == NODE_MMAP)
                        {
				struct ion_address_usage_record *address_node = (struct ion_address_usage_record *)record;
				return (unsigned int)address_node->mapping_address;
			}
		}
   		case RECORD_SIZE:
		{
			if( tracking_info->record_type == NODE_MMAP)
                        {
				struct ion_address_usage_record *address_node = (struct ion_address_usage_record *)record;
				return (unsigned int)address_node->size;
			}
		}
		case RECORD_NEXT:
		{
			return *(tmp_record);
		}
		default:
		{
			ION_DEBUG_LOG(ION_DEBUG_ERROR, "[ion_get_data_from_record]can't find data type (%d)error \n",data_type);
			return 0;
		}
	}
 ION_DEBUG_LOG(ION_DEBUG_INFO, "[ion_get_data_from_record]get data type %d but wrong record type %d\n",data_type,tracking_info->record_type);
 return 0;
}
struct ion_buffer_record * search_record_in_list(struct ion_buffer *buffer,struct ion_buffer_record *list)
{
	struct ion_buffer_record *tmp_buffer_record = list;
	
	if(buffer_created_list != NULL)
	{
		while(tmp_buffer_record !=NULL)
		{
			ION_DEBUG_LOG(ION_DEBUG_INFO, "               find buffer reocrd :0x%x buffer : 0x%x \n",(unsigned int)tmp_buffer_record,(unsigned int)tmp_buffer_record->buffer_address);
			if(tmp_buffer_record->buffer_address == buffer)
			{
				ION_DEBUG_LOG(ION_DEBUG_INFO, "               found record tmp_buffer_record: 0x%x \n",(unsigned int)tmp_buffer_record);
				return tmp_buffer_record;
			}
			tmp_buffer_record = tmp_buffer_record->next;
		}
		ION_DEBUG_LOG(ION_DEBUG_INFO, "		[search_record_in_list]can't get corresponding buffer %x in buffer list %x\n",(unsigned int)buffer,(unsigned int)list);
	}
	else
	{
		ION_DEBUG_LOG(ION_DEBUG_INFO, "		[search_record_in_list]buffer_created_list is null \n");
	}
	return NULL;
}
struct ion_process_record * search_process_in_list(pid_t pid,struct ion_process_record *list)
{
        struct ion_process_record *tmp_process_record = list;
        if(process_created_list != NULL)
        {
                while(tmp_process_record !=NULL)
                {
                        ION_DEBUG_LOG(ION_DEBUG_INFO, "               tmp_process_record: 0x%x pid %d count %d\n",(unsigned int)tmp_process_record,tmp_process_record->pid,tmp_process_record->count);
                        if(tmp_process_record->pid == pid)
                        {
                                ION_DEBUG_LOG(ION_DEBUG_INFO, "               found record tmp_process_record: 0x%x \n",(unsigned int)tmp_process_record);
                                return tmp_process_record;
                        }
                        tmp_process_record = tmp_process_record->next;
                }
                ION_DEBUG_LOG(ION_DEBUG_INFO, "        [search_process_in_list]can't get corresponding process %x in process list %x\n",(unsigned int)pid,(unsigned int)list);
        }
        else
        {
                ION_DEBUG_LOG(ION_DEBUG_INFO, "        [search_process_in_list]process_created_list is null \n");
        }
        return NULL;
}

struct ion_process_record * search_process_by_file(struct file *file,struct ion_process_record *list)
{
        struct ion_process_record *tmp_process_record = list;
	struct ion_fd_usage_record *tmp_fd_record = NULL;
        //*previous_node == NULL;
        if(process_created_list != NULL)
        {
                while(tmp_process_record !=NULL)
                {
                        ION_DEBUG_LOG(ION_DEBUG_INFO, "               tmp_process_record: 0x%x pid %d count %d input  0x%x\n",(unsigned int)tmp_process_record,tmp_process_record->pid,tmp_process_record->count,(unsigned int)file);
			tmp_fd_record = tmp_process_record->fd_using_list;
			while(tmp_fd_record != NULL)
			{
                	        if(tmp_fd_record->file == file)
                       		{
                                	ION_DEBUG_LOG(ION_DEBUG_INFO, "               found record tmp_process_record: 0x%x file: 0x%x\n",(unsigned int)tmp_process_record,(unsigned int)tmp_fd_record->file);
                                	return tmp_process_record;
                        	}
				tmp_fd_record = tmp_fd_record->next;
			}
                        tmp_process_record = tmp_process_record->next;
                }
                ION_DEBUG_LOG(ION_DEBUG_INFO, "        [search_process_in_list]can't get corresponding in process list %x\n",(unsigned int)list);
        }
        else
        {
                ION_DEBUG_LOG(ION_DEBUG_INFO, "        [search_process_in_list]process_created_list is null \n");
        }
        return NULL;
}
#if 0
struct ion_process_record * remove_fd_usage_by_client(struct client *client,struct ion_process_record *list)
{
        struct ion_process_record *tmp_process_record = list;
        struct ion_fd_usage_record *tmp_fd_record = NULL;
        //*previous_node == NULL;
        if(process_created_list != NULL)
        {
                while(tmp_process_record !=NULL)
                {
                        ION_DEBUG_LOG(ION_DEBUG_INFO, "               tmp_process_record: 0x%x pid %d count %d input  0x%x\n",(unsigned int)tmp_process_record,tmp_process_record->pid,tmp_process_record->count,(unsigned int)file);
                        tmp_fd_record = tmp_process_record->fd_using_list;
                        while(tmp_fd_record != NULL)
                        {
                                if(tmp_fd_record->tracking_info->recordID.client_address == client)
                                {
					 move_node_to_freelist(tmp_fd_record->tracking_info->recordID.pid,(unsigned int)client,tmp_fd_record->fd,(unsigned int **)&(tmp_process_record->fd_using_list),(unsigned int **)&(tmp_process_record->fd_freed_list),SEARCH_PID,NODE_FD);

                                        ION_DEBUG_LOG(ION_DEBUG_INFO, "               remove record tmp_process_record: 0x%x fd: 0x%d\n",(unsigned int)tmp_process_record,(unsigned int)tmp_fd_record->fd);
                                }
                                tmp_fd_record = tmp_fd_record->next;
                        }
                        tmp_process_record = tmp_process_record->next;
                }
        }
        else
        {
                ION_DEBUG_LOG(ION_DEBUG_INFO, "        [search_process_in_list]process_created_list is null \n");
        }
        return NULL;
}
#endif

void get_kernel_symbol(unsigned long *backtrace,unsigned int numEntries, unsigned int *kernel_symbol)
{
	unsigned int i = 0;
	char symbol[KSYM_SYMBOL_LEN];

	for(i = 0;i < numEntries;i++)	
	{
		sprint_symbol(symbol,*(backtrace+i));
		//ION_DEBUG_LOG(ION_DEBUG_INFO,"        [get_kernel_symbol]size = %d , %s\n",strlen(symbol),symbol);
		*(kernel_symbol+i) = (unsigned int)get_kernelString_from_hashTable(symbol,strlen(symbol));
		//ION_DEBUG_LOG(ION_DEBUG_INFO,"	[get_kernel_symbol]store string at : 0x[%x]\n",(kernel_symbol+i));
	}
}
unsigned int get_kernel_backtrace(unsigned long *backtrace)
{
	unsigned long stack_entries[BACKTRACE_LEVEL];
	unsigned int i = 0;
	char tmp[KSYM_SYMBOL_LEN];
	struct stack_trace trace = {
		.nr_entries = 0,
		.entries = &stack_entries[0],
		.max_entries = BACKTRACE_LEVEL,
		.skip = 3 
	};
	save_stack_trace(&trace);
	ION_DEBUG_LOG(ION_DEBUG_INFO, "	     [get_kernel_backtrace] backtrace num: [%d]\n",trace.nr_entries);
	if(trace.nr_entries > 0)
	{
		for(i= 0 ; i < trace.nr_entries; i++)
		{
			ION_DEBUG_LOG(ION_DEBUG_INFO, "bactrace[%d] : %x  ",i, trace.entries[i]);
			sprint_symbol(tmp,trace.entries[i]);
			ION_DEBUG_LOG("%s\n",tmp);
		}
		memcpy(backtrace,(unsigned long *)trace.entries,sizeof(unsigned int)*trace.nr_entries);
	}
	return trace.nr_entries; 
}
unsigned int get_kernel_backtrace_show(unsigned long *backtrace)
{
        unsigned long stack_entries[BACKTRACE_LEVEL];
        unsigned int i = 0;
        char tmp[KSYM_SYMBOL_LEN];
        struct stack_trace trace = {
                .nr_entries = 0,
                .entries = &stack_entries[0],
                .max_entries = BACKTRACE_LEVEL,
                .skip = 3
        };
        save_stack_trace(&trace);
        ION_DEBUG_LOG(ION_DEBUG_ERROR, "      [get_kernel_backtrace] backtrace num: [%d]\n",trace.nr_entries);
        if(trace.nr_entries > 0)
        {
                for(i= 0 ; i < trace.nr_entries; i++)
                {
                        ION_DEBUG_LOG(ION_DEBUG_ERROR, "bactrace[%d] : %x  ",i, trace.entries[i]);
                        sprint_symbol(tmp,trace.entries[i]);
                        ION_DEBUG_LOG(ION_DEBUG_ERROR,"%s\n",tmp);
                }
                memcpy(backtrace,(unsigned long *)trace.entries,sizeof(unsigned int)*trace.nr_entries);
        }
        return trace.nr_entries;
}

void insert_node_to_list(void **list,unsigned int *node)
{
	//ION_DEBUG_LOG(ION_DEBUG_INFO,"list is %x node is %x\n",list,node);
	//ION_DEBUG_LOG(ION_DEBUG_INFO,"*list is %x *node is %x\n",*list,*node);
	if(*list != NULL )
	{
		*node = (unsigned int)*list;
	}
	else
	{
		*node = 0;
	}
	*list = node;
}

void *find_node_in_list(pid_t pid, unsigned int client_address,unsigned int data,unsigned int **previous_node, unsigned int *list,unsigned int search_type,unsigned int node_type)
{
	unsigned int *prev_node = NULL;
	unsigned int *current_node = list;
	ion_record_basic_info_t *record_ID_tmp;
	struct ion_process_record *process_tmp = NULL;
	struct ion_buffer_record *buffer_tmp = NULL;

	while(current_node != NULL)
	{
		if((search_type == SEARCH_PROCESS_PID) && (node_type == LIST_PROCESS)&& (client_address == 0))
		{
			process_tmp = (struct ion_process_record *)(current_node);	
			ION_DEBUG_LOG(ION_DEBUG_INFO, "            [find_node_in_list]curent_node is %x process_tmp->pid is %d process_tmp->count is %d\n",(unsigned int)current_node,process_tmp->pid,process_tmp->count);	
			if(process_tmp->pid == pid)
			{
				if(process_tmp->count ==1)
				{
					*previous_node = prev_node;
					return current_node;
				}
				else
				{
					process_tmp->count--;
					return NULL;
				}
			}
		}
		else if((search_type == SEARCH_BUFFER) && (node_type == LIST_BUFFER)&& (client_address == 0))
		{
			buffer_tmp = (struct ion_buffer_record *)(current_node);
                        ION_DEBUG_LOG(ION_DEBUG_INFO, "            [find_node_in_list]curent_node is %x buffer_tmp->buffer_address is 0x%x data 0x%x\n",(unsigned int)current_node,(unsigned int)buffer_tmp->buffer_address,(unsigned int)data);
			if(buffer_tmp->buffer_address == (void *)data)
                        {
				*previous_node = prev_node;
                                return current_node;
                        }
 		}
		else if(search_type < SEARCH_PROCESS_PID)
		{
			struct ion_buffer_usage_record *tmp = (struct ion_buffer_usage_record *)current_node; //FIXME this code is not for process list 
			record_ID_tmp =(ion_record_basic_info_t *)(current_node+1);
			ION_DEBUG_LOG(ION_DEBUG_INFO, "            [find_node_in_list]current_node is %x record_ID_tmp %x record_ID_tmp->pid %d record_ID_tmp->client_address %x\n",(unsigned int)current_node,(unsigned int)record_ID_tmp,record_ID_tmp->recordID.pid,record_ID_tmp->recordID.client_address);
			if((search_type == SEARCH_PID_CLIENT)
			&&(record_ID_tmp->recordID.pid == pid)
			&&(record_ID_tmp->recordID.client_address  == client_address)
			&&(tmp->function_type != ION_FUNCTION_SHARE))//FIXME client may use the same buffer twice?
			{
				*previous_node =  prev_node;
				return current_node;
			}
			else if((search_type == SEARCH_PID)&&(record_ID_tmp->recordID.pid == pid)) 
			{
				if(node_type == NODE_FD)
				{
					struct ion_fd_usage_record *tmp = (struct ion_fd_usage_record *)current_node;
					if(tmp->fd == data)
					{
						*previous_node =  prev_node;
                       		 		return current_node;
					}
				}
				else if(node_type == NODE_MMAP)
				{
					struct ion_address_usage_record *tmp = (struct ion_address_usage_record *)current_node;
					if(tmp->mapping_address == data)
					{
						 *previous_node =  prev_node;
		               		        return current_node;
					}	
				}
                	}
		}
		else if(search_type == SEARCH_FD_GPID)
		{
                	struct ion_buffer_usage_record *tmp = (struct ion_buffer_usage_record *)current_node;
			record_ID_tmp =(ion_record_basic_info_t *)(current_node+1);
                        ION_DEBUG_LOG(ION_DEBUG_INFO, "            [find_node_in_list]current_node is %x record_ID_tmp->group_pid %d data %d \n",(unsigned int)current_node,record_ID_tmp->recordID.group_pid,data);

                        if(((record_ID_tmp->recordID.pid == pid)||(tmp->file == data)) && (tmp->function_type == ION_FUNCTION_SHARE))
			{           
				*previous_node =  prev_node;
				return current_node;
			}
		}
		else if(search_type == SEARCH_FILE) //FIXME
		{
			 struct ion_fd_usage_record *tmp = (struct ion_fd_usage_record *)current_node;

                        if(tmp->file == data)
                        {
                                *previous_node =  prev_node;
                                return current_node;
                        }
		}
		else
		{
			ION_DEBUG_LOG(ION_DEBUG_ERROR, "            [find_node_in_list]Error!!!\n");
		}
		prev_node = current_node;
		current_node = (unsigned int *)*(current_node);
	}
	ION_DEBUG_LOG(ION_DEBUG_INFO, "              [find_node_in_list]can't find node in list. search_type %d node_type %d pid %d client_address %x\n",search_type,node_type,pid,(unsigned int)client_address);
	return NULL;
}
void *move_node_to_freelist(pid_t pid,unsigned int client_address,unsigned int data,unsigned int **from, unsigned int **to,unsigned int search_type,unsigned int node_type)
{
	unsigned int *previous_node = NULL;
	unsigned int *found_node;
	ION_DEBUG_LOG(ION_DEBUG_INFO, "          [move_node_to_freelist]pid %d client_address %x from %x to %x search_type %d node_type %d\n",(int)pid,(unsigned int)client_address,(unsigned int )*from,(unsigned int )*to,search_type,node_type);
	found_node = find_node_in_list(pid,client_address,data,(unsigned int **)&previous_node,(unsigned int *)*from,search_type,node_type);
	ION_DEBUG_LOG(ION_DEBUG_INFO, "          [move_node_to_freelist] found_node %x previous_node is %x *to %x\n",(unsigned int)found_node,(unsigned int)previous_node,(unsigned int)*to);
	if(found_node != NULL)
	{
		if(previous_node == NULL)
		{
			*from = (unsigned int *)*found_node;
		}
		else
		{
			*previous_node = *(found_node);
		}
		if(ion_debugger_history)
		{
			if(*to != NULL)
			{
				*found_node = (unsigned int)*to;
			}
			else
			{
				*found_node = 0;
			}
			*to = found_node;
			ION_DEBUG_LOG(ION_DEBUG_INFO, "          [move_node_to_freelist]from list is %x to list is %x found_node is %x\n",(unsigned int)*from,(unsigned int)*to,(unsigned int)found_node);
		}
		else
		{
			ION_DEBUG_LOG(ION_DEBUG_INFO, "          [move_node_to_freelist]release node %x\n",(unsigned int)found_node);
			if(node_type == NODE_BUFFER)
			{
				buffer_node_size += (unsigned int)sizeof(ion_buffer_usage_record_t);
                                total_size += (unsigned int)sizeof(ion_buffer_usage_record_t);

                                //ION_DEBUG_LOG(ION_DEBUG_ERROR, "list_buffer[%d] buffer_node[%d] client_node[%d] fd_node[%d] mmap_node[%d] process_node[%d] total_size[%d] \n",
                //list_buffer_size,buffer_node_size,client_node_size,fd_node_size,mmap_node_size,process_node_size,total_size);

				kmem_cache_free(ion_buffer_usage_cachep,found_node);
			}
			else
			{	
				remove_record(node_type,found_node);
			}
		}
		return found_node;	
	}
	else
	{
		ION_DEBUG_LOG(ION_DEBUG_ERROR, "          [move_node_to_freelist]can't found node in list %x: node info pid %d client address %x\n",(unsigned int)from,pid,(unsigned int)client_address);
		return NULL;
	}
}
int remove_record(unsigned int node_type,unsigned int *record)
{
	struct kmem_cache *remove_record_cachep = NULL;
	struct ion_record_basic_info *tracking_info = NULL;
#if 0
	ObjectEntry *entry = NULL;
	ObjectEntry *mapping_entry = NULL;

	tracking_info = (struct ion_record_basic_info *)(remove_reocrd+1);
	entry =(ObjectEntry *)tracking_info->allocate_backtrace;
	if(entry != NULL)
	{
        	if(tracking_info->allocate_backtrace_type == USER_BACKTRACE)
		{
			atomic_dec(&(gUserBtTable.count));	
		}
		else if(tracking_info->allocate_backtrace_type == KERNEL_BACKTRACE)
		{
			atomic_dec(&(gKernelBtTable.count));	
		}
		if((tracking_info->allocate_backtrace_type == USER_BACKTRACE || tracking_info->allocate_backtrace_type == KERNEL_BACKTRACE) && atomic_dec_and_test(&(entry->reference)))
                {
                       kfree(entry);
                }
	}
#endif
	switch(node_type)
	{
		case LIST_BUFFER:
		{
			list_buffer_size -= (unsigned int)sizeof(struct ion_buffer_record);
			total_size -= (unsigned int)sizeof(struct ion_buffer_record);
			//ION_DEBUG_LOG(ION_DEBUG_ERROR, "list_buffer[%d] buffer_node[%d] client_node[%d] fd_node[%d] mmap_node[%d] process_node[%d] total_size[%d]\n",
			//list_buffer_size,buffer_node_size,client_node_size,fd_node_size,mmap_node_size,process_node_size,total_size);

			remove_record_cachep = ion_list_buffer_cachep;
			break;
		}
		case LIST_PROCESS:
		{
			process_node_size -= (unsigned int)sizeof(struct ion_process_record);
			total_size -= (unsigned int)sizeof(struct ion_process_record);
			//ION_DEBUG_LOG(ION_DEBUG_ERROR, "222list_buffer[%d] buffer_node[%d] client_node[%d] fd_node[%d] mmap_node[%d] process_node[%d] total_size[%d]\n",
                                                              //  list_buffer_size,buffer_node_size,client_node_size,fd_node_size,mmap_node_size,process_node_size,total_size);
			remove_record_cachep = ion_list_process_cachep;
			break;
		}
		case NODE_CLIENT:
		{
			client_node_size -= (unsigned int)sizeof(ion_client_usage_record_t);
			total_size -= (unsigned int)sizeof(ion_client_usage_record_t);
			//ION_DEBUG_LOG(ION_DEBUG_ERROR, "222list_buffer[%d] buffer_node[%d] client_node[%d] fd_node[%d] mmap_node[%d] process_node[%d] total_size[%d]\n",
            //    list_buffer_size,buffer_node_size,client_node_size,fd_node_size,mmap_node_size,process_node_size,total_size);

			remove_record_cachep = ion_client_usage_cachep; 
			break;
		}
		case NODE_FD:
		{
			fd_node_size -= (unsigned int)sizeof(ion_fd_usage_record_t);
			total_size -= (unsigned int)sizeof(ion_fd_usage_record_t);
			//ION_DEBUG_LOG(ION_DEBUG_ERROR, "222list_buffer[%d] buffer_node[%d] client_node[%d] fd_node[%d] mmap_node[%d] process_node[%d] total_size[%d]\n",
           //     list_buffer_size,buffer_node_size,client_node_size,fd_node_size,mmap_node_size,process_node_size,total_size);	
			remove_record_cachep = ion_fd_usage_cachep;
			break;
		}
		case NODE_MMAP:
		{
			mmap_node_size -= (unsigned int)sizeof(ion_address_usage_record_t);
			total_size -= (unsigned int)sizeof(ion_address_usage_record_t);
			//ION_DEBUG_LOG(ION_DEBUG_ERROR, "222list_buffer[%d] buffer_node[%d] client_node[%d] fd_node[%d] mmap_node[%d] process_node[%d] total_size[%d]\n",
            //    list_buffer_size,buffer_node_size,client_node_size,fd_node_size,mmap_node_size,process_node_size,total_size);

			remove_record_cachep = ion_address_usage_cachep;
			break;
		}
	}
	if(remove_record_cachep != NULL)
	{
		kmem_cache_free(remove_record_cachep,record);
	}
}
void * allocate_record(unsigned int type,ion_sys_record_t *record_param)
{
	static unsigned int buffer_node_size = 0;
	static unsigned int process_node_size = 0;	
	static unsigned int list_buffer_size = 0;
	static unsigned int fd_node_size = 0;
	static unsigned int client_node_size = 0;
	static unsigned int mmap_node_size = 0;
	static unsigned int total_size = 0; 
	switch(type)
	{
		case LIST_BUFFER:
                {
                        if(!list_buffer_cache_created)
                        {
                                ion_list_buffer_cachep = kmem_cache_create("buffer_record",sizeof(struct ion_buffer_record),0,SLAB_NO_DEBUG,NULL);
                                list_buffer_cache_created = true;
                        }
                        if(ion_list_buffer_cachep != NULL)
                        {
				struct ion_buffer_record *new_buffer_record = NULL;

				if(total_size > ion_log_limit)
                                {
					disable_ion_debugger();
					return NULL;
				}

                                new_buffer_record =  (struct ion_buffer_record *)kmem_cache_alloc(ion_list_buffer_cachep,GFP_KERNEL);
				if(new_buffer_record != NULL)
                                {
                                        //assign data into buffer record
                                        new_buffer_record->buffer_address = record_param->buffer;
                                        new_buffer_record->buffer = get_record(HASH_NODE_BUFFER,record_param);
                                        new_buffer_record->heap_type = record_param->buffer->heap->type;
                                        if(new_buffer_record->heap_type != ION_HEAP_TYPE_CARVEOUT)
                                        {
                                                new_buffer_record->priv_virt =  record_param->buffer->priv_virt;
                                        }
                                        else
                                        {
                                                new_buffer_record->priv_phys =  record_param->buffer->priv_phys;
                                        }
                                        new_buffer_record->size = record_param->buffer->size;
                                        mutex_init(&new_buffer_record->ion_buffer_usage_mutex);
                                        mutex_init(&new_buffer_record->ion_address_usage_mutex);
                                        new_buffer_record->buffer_using_list = NULL;
                                        new_buffer_record->buffer_freed_list = NULL;
                                        new_buffer_record->address_using_list = NULL;
                                        new_buffer_record->address_freed_list = NULL;
                                        mutex_lock(&buffer_lifecycle_mutex);
                                        if(buffer_created_list == NULL)
                                        {
                                                new_buffer_record->next = NULL;
                                        }
                                        else
                                        {
                                                new_buffer_record->next = buffer_created_list;
                                        }
                                        buffer_created_list = new_buffer_record;
                                        mutex_unlock(&buffer_lifecycle_mutex);

					list_buffer_size += (unsigned int)ion_list_buffer_cachep->size;
                                        total_size += (unsigned int)ion_list_buffer_cachep->size;
					if(total_size > ion_log_limit)
					{
						disable_ion_debugger();
						return NULL;
					}
					ION_DEBUG_LOG(ION_DEBUG_INFO, "list_buffer[%d] buffer_node[%d] client_node[%d] fd_node[%d] mmap_node[%d] process_node[%d] total_size[%d]\n",
                						list_buffer_size,buffer_node_size,client_node_size,fd_node_size,mmap_node_size,process_node_size,total_size);

					return (void *)new_buffer_record;
                                }
				else
				{
					ION_DEBUG_LOG(ION_DEBUG_ERROR, "can't get enough memory for LIST_BUFFER structure");
					return (void *)NULL;
				}

                        }
                        break;
                }

                case LIST_PROCESS:
                {
                        if(!list_process_cache_created)
                        {
                                ion_list_process_cachep = kmem_cache_create("process_record",sizeof(struct ion_process_record),0,SLAB_NO_DEBUG,NULL);
                                list_process_cache_created = true;
                        }
                        if(ion_list_process_cachep != NULL)
                        {
				struct ion_process_record *process_record = NULL;
				if(total_size > ion_log_limit)
                                {
					disable_ion_debugger();
                                        return NULL;
                                }

                                process_record = (struct ion_process_record *)kmem_cache_alloc(ion_list_process_cachep,GFP_KERNEL);
				if(process_record != NULL)
                                {
                                	//assign data into process_created_list
                                	process_record->pid = record_param->pid;
                                	process_record->group_id = record_param->group_id;
                                	mutex_init(&process_record->ion_fd_usage_mutex);
                                	mutex_init(&process_record->ion_address_usage_mutex);
                                	process_record->fd_using_list = NULL;
                                	process_record->fd_freed_list = NULL;
                                	process_record->address_using_list = NULL;
                                	process_record->address_freed_list = NULL;
                                	process_record->count = 1;
                                	if(process_created_list == NULL)
                                	{
                                		process_record->next = NULL;
                                	}
                                	else
                                	{
                                		process_record->next = process_created_list;
                                	}
                                	process_created_list = process_record;

					process_node_size += (unsigned int)ion_list_process_cachep->size;
                                        total_size += (unsigned int)ion_list_process_cachep->size;

					ION_DEBUG_LOG(ION_DEBUG_INFO, "list_buffer[%d] buffer_node[%d] client_node[%d] fd_node[%d] mmap_node[%d] process_node[%d] total_size[%d]\n",
                						list_buffer_size,buffer_node_size,client_node_size,fd_node_size,mmap_node_size,process_node_size,total_size);
					return (void *)process_record;
                               }
			       else
                               {
                                        ION_DEBUG_LOG(ION_DEBUG_WARN, "can't get enough memory for LIST_PROCESS structure");
                                        return (void *)NULL;
                               }
                        }
                        break;
                }

	        case NODE_BUFFER:
		{
			if(!buffer_cache_created)
			{
				ion_buffer_usage_cachep = kmem_cache_create("buffer_usage_record",sizeof(ion_buffer_usage_record_t),0,SLAB_NO_DEBUG,NULL);
				buffer_cache_created = true;
			}
			if(ion_buffer_usage_cachep != NULL)
			{
                                if(total_size > ion_log_limit)
                                {
					disable_ion_debugger();
                                        return NULL;
                                }
				buffer_node_size += (unsigned int)ion_buffer_usage_cachep->size;
                                total_size += (unsigned int)ion_buffer_usage_cachep->size;

				ION_DEBUG_LOG(ION_DEBUG_INFO, "list_buffer[%d] buffer_node[%d] client_node[%d] fd_node[%d] mmap_node[%d] process_node[%d] total_size[%d]\n",
                				list_buffer_size,buffer_node_size,client_node_size,fd_node_size,mmap_node_size,process_node_size,total_size);

				return (void *)kmem_cache_alloc(ion_buffer_usage_cachep,GFP_KERNEL);	
			}
			break;
		}
		case NODE_FD:
		{
			if(!fd_cache_created)
			{
				ion_fd_usage_cachep = kmem_cache_create("fd_record",sizeof(ion_fd_usage_record_t),0,SLAB_NO_DEBUG,NULL);	
				fd_cache_created = true;
			}
			if(ion_fd_usage_cachep != NULL)
			{
                                if(total_size > ion_log_limit)
                                {
					disable_ion_debugger();
                                        return NULL;
                                }

				fd_node_size += (unsigned int)ion_fd_usage_cachep->size;
                                total_size += (unsigned int)ion_fd_usage_cachep->size;
				ION_DEBUG_LOG(ION_DEBUG_INFO, "list_buffer[%d] buffer_node[%d] client_node[%d] fd_node[%d] mmap_node[%d] process_node[%d] total_size[%d]\n",
                				list_buffer_size,buffer_node_size,client_node_size,fd_node_size,mmap_node_size,process_node_size,total_size);

				return (void *)kmem_cache_alloc(ion_fd_usage_cachep,GFP_KERNEL);
			}
			break;
		}
		case NODE_CLIENT:
		{
			if(!client_cache_created)
			{
				ion_client_usage_cachep = kmem_cache_create("client_record",sizeof(ion_client_usage_record_t),0,SLAB_NO_DEBUG,NULL);
				client_cache_created = true;
			}
			if(ion_client_usage_cachep != NULL)
			{
                                if(total_size > ion_log_limit)
                                {
					disable_ion_debugger();
                                        return NULL;
                                }

				client_node_size += (unsigned int)ion_client_usage_cachep->size;
                                total_size += (unsigned int)ion_client_usage_cachep->size;
				ION_DEBUG_LOG(ION_DEBUG_INFO, "list_buffer[%d] buffer_node[%d] client_node[%d] fd_node[%d] mmap_node[%d] process_node[%d] total_size[%d]\n",
		           			list_buffer_size,buffer_node_size,client_node_size,fd_node_size,mmap_node_size,process_node_size,total_size);

				return (void *)kmem_cache_alloc(ion_client_usage_cachep,GFP_KERNEL);	
			}
			break;
		}
		case NODE_MMAP:
		{
			if(!address_cache_created)
			{
				ion_address_usage_cachep = kmem_cache_create("address_record",sizeof(ion_address_usage_record_t),0,SLAB_NO_DEBUG,NULL);
				address_cache_created = true;
			}
			if(ion_address_usage_cachep != NULL)
			{
                                if(total_size > ion_log_limit)
                                {
					disable_ion_debugger();
                                        return NULL;
                                }

				mmap_node_size += (unsigned int)ion_address_usage_cachep->size;
                                total_size += (unsigned int)ion_address_usage_cachep->size;
				ION_DEBUG_LOG(ION_DEBUG_INFO, "list_buffer[%d] buffer_node[%d] client_node[%d] fd_node[%d] mmap_node[%d] process_node[%d] total_size[%d]\n",
       						list_buffer_size,buffer_node_size,client_node_size,fd_node_size,mmap_node_size,process_node_size,total_size);

				return (void *)kmem_cache_alloc(ion_address_usage_cachep,GFP_KERNEL);
			}
			break;
		}
		case NODE_MAX:
		default:
		{
			ION_DEBUG_LOG(ION_DEBUG_ERROR, "[ERROR!!]allocate_record wrong type %d",type);
			break;
		} 
	}
	return NULL;
}
/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 0x9e370001UL
static inline unsigned int hash_32(unsigned int val, unsigned int bits)
{
        /* On some cpus multiply is faster, on others gcc will do shifts */
        unsigned int hash = val * GOLDEN_RATIO_PRIME_32;

        /* High bits are more random, so use them. */
        return hash >> (32 - bits);
}

static unsigned int RSHash(char* str, unsigned int len) 
{ 
    unsigned int b = 378551; 
    unsigned int a = 63689; 
    unsigned int hash = 0; 
    unsigned int i = 0; 

    for(i = 0; i < len; str++, i++) 
    { 
        hash = hash * a + (*str); 
        a    = a * b; 
    } 

    return hash;
}
static uint32_t get_hash(void* object, size_t numEntries)
{
    unsigned int *backtrace = NULL;
    unsigned int hash = 0;
    size_t i;
    backtrace = (unsigned int *)object;
    if (backtrace == NULL) return 0;
    for (i = 0 ; i < numEntries ; i++) {
        hash = (hash * 33) + (*(backtrace+i) >> 2);
    }
    return hash;
}
static uint32_t get_mapping_hash(struct mapping* object, size_t numEntries)
{
    unsigned int *mapping_address = NULL;
    unsigned int hash = 0;
    size_t i;
    if (object == NULL) return 0;
    for (i = 0 ; i < numEntries ; i++) {
	mapping_address = (unsigned int *)(object+i);
        hash = (hash * 33) + (*(mapping_address) >> 2);
    }
    return hash;
}

static ObjectEntry* find_entry(ObjectTable* table, unsigned int slot,void* object,unsigned int  numEntries)
{
    ObjectEntry* entry = table->slots[slot];
    while (entry != NULL) {
        if ( entry->numEntries == numEntries &&
                !memcmp(object, entry->object, numEntries * sizeof(unsigned int))) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static ObjectEntry* find_mapping_entry(ObjectTable* table, unsigned int slot,void* object,unsigned int  numEntries)
{
    ObjectEntry* entry = table->slots[slot];
    while (entry != NULL) {
        if ( entry->numEntries == numEntries &&
                !memcmp(object, entry->object, numEntries * sizeof(struct mapping))) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static ObjectEntry* find_buffer_entry(ObjectTable* table, unsigned int slot,struct ion_buffer *object)
{
    ObjectEntry* entry = table->slots[slot];
    while (entry != NULL) {
	struct ion_buffer *tmp_buffer =(struct ion_buffer *)entry->object;
        if ( (tmp_buffer->size == object->size)&&(tmp_buffer->heap == object->heap) &&(entry->object == (void *)object))
        {
		if(object->heap->type != ION_HEAP_TYPE_CARVEOUT)
                {
                   if(tmp_buffer->priv_virt ==  object->priv_virt)
		   {
			return entry;	
		   }
                }
                else
                {
                   if(tmp_buffer->priv_phys ==  object->priv_phys)
		   {
			return entry;
		   }
                }
        }
        entry = entry->next;
    }
    return NULL;
}

static StringEntry* find_string_entry(StringTable* table, unsigned int slot,char *string_name,unsigned int string_len)
{
    StringEntry* entry = table->slots[slot];
    while (entry != NULL) {
        if ( entry->string_len == string_len &&
                !memcmp(string_name, entry->name, string_len)) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}
char *get_userString_from_hashTable(char *string_name,unsigned int len)
{
	return get_string(string_name,len,&gMappingStringTable,&gMappingStringTable_mutex);
}
char *get_kernelString_from_hashTable(char *string_name,unsigned int len)
{
	return get_string(string_name,len+1,&gKernelPathTable,&gKernelPathTable_mutex); //add 1 for '\0'
}
//get string form hash table or create new hash node in hash table
char *get_string(char *string_name,unsigned int len,StringTable *table,struct mutex *string_mutex)
{
    unsigned int hash;
    unsigned int slot;
    static unsigned int string_size = 0;
    static unsigned int string_struct_size = 0;
    StringEntry *entry = NULL;
    hash = RSHash(string_name,len);
    slot = hash % OBJECT_TABLE_SIZE;
    mutex_lock(string_mutex);
    entry = find_string_entry(table,slot,(void *)string_name,len);
    mutex_unlock(string_mutex);
    if(entry != NULL)
    {
	//ION_DEBUG_LOG(ION_DEBUG_INFO,"	[get_string] find string in string hash table : addres 0x[%x]%s\n",entry->name,entry->name,entry->string_len);
	return entry->name;
    }
    else
    {
	entry = kmalloc(sizeof(StringEntry),GFP_KERNEL);
	string_struct_size += (unsigned int)sizeof(StringEntry);
	string_size += len;	
	entry->name = kmalloc(len,GFP_KERNEL); 
	memcpy(entry->name,string_name,len);
    	entry->slot = slot;
	entry->string_len = len;
	entry->reference = 1;
        entry->prev = NULL;
	mutex_lock(string_mutex);
        entry->next =  table->slots[slot];
        table->slots[slot] = entry;
        if(entry->next != NULL)
       	{ 
        	entry->next->prev = entry;
        }
        table->count++;
	mutex_unlock(string_mutex);
	ION_DEBUG_LOG(ION_DEBUG_WARN, "string_struct_size [%d] string_size[%d]\n",string_struct_size,string_size);
	//ION_DEBUG_LOG(ION_DEBUG_INFO,"        [get_string]create new node in string hash table: address 0x[%x]%s size %d\n",entry->name,entry->name,len);
	return entry->name;
    }
}
//get record from hash table or create new node from slab allocator
void *get_record(unsigned int type, ion_sys_record_t *param)
{
	ion_sys_record_t *tmp = param;
	ObjectEntry *entry = NULL;
	unsigned int hash;
	unsigned int slot;
	if(tmp != NULL)
	{	
		switch(type)
		{
			case HASH_NODE_CLIENT:
			{
				break;
			}
			case HASH_NODE_BUFFER:
			{
				hash = (unsigned int)hash_32(((unsigned int)param->buffer+param->buffer->size+param->buffer->heap->type), 16);
				slot = hash % OBJECT_TABLE_SIZE;
				entry = find_buffer_entry(&gBufferTable,slot ,param->buffer);
				if(entry != NULL)
				{
					//ION_DEBUG_LOG(ION_DEBUG_INFO,"        [get_record][BUFFER]find the same entry\n ");
					entry->reference++;
				}
				else
				{
					//ION_DEBUG_LOG(ION_DEBUG_INFO,"        [get_record][BUFFER] can't find entry in hash table and create new entry\n");
					entry = kmalloc(sizeof(ObjectEntry)+sizeof(struct ion_buffer),GFP_KERNEL);
                                        entry->reference = 1;
                                        entry->prev = NULL;
                                        entry->slot = slot;
                                        entry->next = gBufferTable.slots[slot];
                                        entry->numEntries = (unsigned int)param->buffer; 
                                        memcpy(entry->object,param->buffer, sizeof(struct ion_buffer));
                                        gBufferTable.slots[slot] = entry;
                                        if(entry->next != NULL)
                                        {
                                                entry->next->prev = entry;
                                        }
                                        gBufferTable.count++;
				}
				
				return entry->object;
			}
			case HASH_NODE_USER_BACKTRACE:
			{
				hash = get_hash(param->backtrace,param->backtrace_num);
				slot = hash % OBJECT_TABLE_SIZE;
				mutex_lock(&gUserBtTable_mutex);
				entry = find_entry(&gUserBtTable,slot,(void *)param->backtrace,param->backtrace_num);
				mutex_unlock(&gUserBtTable_mutex);
				if(entry != NULL)
				{
					//ION_DEBUG_LOG(ION_DEBUG_INFO,"        [get_record][USER_BACKTRACE]find the same entry entry 0x%x entry num %d\n",entry->object,entry->numEntries);

					entry->reference++;
				}
				else
				{
					entry = kmalloc(sizeof(ObjectEntry)+(param->backtrace_num * sizeof(unsigned int)),GFP_KERNEL);
					entry->reference = 1;
					entry->prev = NULL;
					entry->slot = slot;
					entry->numEntries = param->backtrace_num;
					memcpy(entry->object,&(param->backtrace[0]),entry->numEntries * sizeof(unsigned int));
					mutex_lock(&gUserBtTable_mutex);
					entry->next = gUserBtTable.slots[slot];
					gUserBtTable.slots[slot] = entry;
					if(entry->next != NULL)
					{
						entry->next->prev = entry;
					}
					gUserBtTable.count++;
					mutex_unlock(&gUserBtTable_mutex);
					// ION_DEBUG_LOG(ION_DEBUG_INFO,"        [get_record][USER_BACKTRACE]create new entry>object  0x%x entry num %d souce %x source2 %x\n",entry->object,entry->numEntries,&param->backtrace[0],&(param->backtrace[0]));

				}
				return entry;
			}
			case HASH_NODE_KERNEL_BACKTRACE:
			{
                                hash = get_hash(param->backtrace,param->backtrace_num);
                                slot = hash % OBJECT_TABLE_SIZE;
				mutex_lock(&gKernelBtTable_mutex);
                                entry = find_entry(&gKernelBtTable,slot,(void *)param->backtrace,param->backtrace_num);
				mutex_unlock(&gKernelBtTable_mutex);
                                if(entry != NULL)
                                {
                                        //ION_DEBUG_LOG(ION_DEBUG_INFO,"        [get_record][KERNEL_BACKTRACE]find the same entry entry 0x%x entry num %d\n",entry->object,entry->numEntries);
                                        entry->reference++;
                                }
                                else
                                {
                                        entry = kmalloc(sizeof(ObjectEntry)+(param->backtrace_num * sizeof(unsigned int)),GFP_KERNEL);
                                        entry->reference = 1;
                                        entry->prev = NULL;
                                        entry->slot = slot;
                                        entry->numEntries = param->backtrace_num;
                                        memcpy(entry->object,param->backtrace,entry->numEntries * sizeof(unsigned int));
					mutex_lock(&gKernelBtTable_mutex);
					entry->next = gKernelBtTable.slots[slot];
                                        gKernelBtTable.slots[slot] = entry;
                                        if(entry->next != NULL)
                                        {
                                                entry->next->prev = entry;
                                        }
                                        gKernelBtTable.count++;
					mutex_unlock(&gKernelBtTable_mutex);
					//ION_DEBUG_LOG(ION_DEBUG_INFO,"        [get_record][KERNEL_BACKTRACE]create new entry>object  0x%x entry num %d\n",entry->object,entry->numEntries);
                                }
                                return entry;
			}
			case HASH_NODE_USER_MAPPING:
			{
                                hash = get_mapping_hash(&(param->mapping_record[0]),param->backtrace_num);
                                slot = hash % OBJECT_TABLE_SIZE;
                                entry = find_mapping_entry(&gUserMappingTable,slot,(void *)&(param->mapping_record[0]),param->backtrace_num);
                                if(entry != NULL)
                                {
                                        //ION_DEBUG_LOG(ION_DEBUG_INFO,"        [get_record][USER_MAPPING_INFO]find the same entry entry 0x%x entry num %d\n",entry->object,entry->numEntries);
                                        entry->reference++;
                                }
                                else
                                {
                                        entry = kmalloc(sizeof(ObjectEntry)+(param->backtrace_num * sizeof(struct mapping)),GFP_KERNEL);
                                        entry->reference = 1;
                                        entry->prev = NULL;
                                        entry->slot = slot;
                                        entry->next = gUserMappingTable.slots[slot];
                                        entry->numEntries = param->backtrace_num;
                                        memcpy(entry->object,&(param->mapping_record[0]),entry->numEntries * sizeof(struct mapping));
                                        gUserMappingTable.slots[slot] = entry;
                                        if(entry->next != NULL)
                                        {
                                                entry->next->prev = entry;
                                        }
                                        gUserMappingTable.count++;
					//ION_DEBUG_LOG(ION_DEBUG_INFO,"        [get_record][USER_MAPPING]create new entry>object  0x%x entry num %d source %x source2 %x\n",entry->object,entry->numEntries,&param->backtrace[0],&(param->backtrace[0]));
                                }
                                return entry;

			}
			case HASH_NODE_KERNEL_SYMBOL:
			{
				hash = get_hash(param->kernel_symbol,param->backtrace_num);
                                slot = hash % OBJECT_TABLE_SIZE;
                                entry = find_entry(&gKernelSymbolTable,slot,(void *)param->kernel_symbol,param->backtrace_num);
                                if(entry != NULL)
                                {
                                        //printk("        [get_record][KERNEL_SYMBOL]find the same entry entry 0x%x entry num %d\n",entry->object,entry->numEntries);
                                        entry->reference++;
                                }
                                else
                                {
					//unsigned int *temp = NULL;
                                        entry = kmalloc(sizeof(ObjectEntry)+(param->backtrace_num * sizeof(unsigned int)),GFP_KERNEL);
                                        entry->reference = 1;
                                        entry->prev = NULL;
                                        entry->slot = slot;
                                        entry->next = gKernelSymbolTable.slots[slot];
                                        entry->numEntries = param->backtrace_num;
                                        memcpy(entry->object,param->kernel_symbol,entry->numEntries * sizeof(unsigned int));
                                        gKernelSymbolTable.slots[slot] = entry;
					//tmp = (unsigned int *)entry->object;

                                        if(entry->next != NULL)
                                        {
                                                entry->next->prev = entry;
                                        }
                                        gKernelSymbolTable.count++;
                                }
                                
                                return entry;

				break;
			}

			case HASH_NODE_MAX:
			default:
			{
				ION_DEBUG_LOG(ION_DEBUG_ERROR, "        [get_record][ERROR] get_record error type %d",type);
				break;
			}
		}
	}
	return NULL;
}
void *create_new_record_into_process(ion_sys_record_t *record_param, struct ion_process_record *process_record,unsigned int node_type,unsigned int from_kernel)
{
	unsigned int *new_node = NULL;
        struct ion_client *tmp = NULL;
	new_node = (void *)allocate_record(node_type,record_param);
	ION_DEBUG_LOG(ION_DEBUG_INFO,"           [%d][%d][create_new_record_into_process(%d)] process_record 0x%x node_type %d new node %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)process_record,node_type,(unsigned int)new_node);
	
	if(new_node != NULL)
	{
		 struct ion_record_basic_info *tracking_info = NULL;
	         tmp = record_param->client;
            	 tracking_info = (struct ion_record_basic_info *)(new_node+1);

            	//assign data into new record
            	tracking_info->recordID.pid = record_param->pid;
            	tracking_info->recordID.group_pid = (pid_t)tmp->pid;
            	tracking_info->recordID.client_address = (unsigned int)record_param->client;
            	tracking_info->recordID.client = record_param->client; //FIXME it should be stored in hash table
            	tracking_info->recordID.process_record = process_record;
            	tracking_info->from_kernel = from_kernel;
            	tracking_info->allocate_backtrace = NULL;
            	tracking_info->allocate_map = NULL;
            	tracking_info->release_backtrace = NULL;
            	tracking_info->release_map = NULL;
            	tracking_info->release_backtrace_type = BACKTRACE_MAX;
		tracking_info->record_type = node_type;

            	if(!from_kernel)
            	{
                	//tracking_info->allocate_backtrace_type = USER_BACKTRACE;
                	tracking_info->allocate_backtrace = (unsigned int *)get_record(HASH_NODE_USER_BACKTRACE,record_param);
               		tracking_info->allocate_map = (unsigned int *)get_record(HASH_NODE_USER_MAPPING,record_param);
            	}
            	else
            	{
                	//tracking_info->allocate_backtrace_type = KERNEL_BACKTRACE;
                	tracking_info->allocate_backtrace = (unsigned int *)get_record(HASH_NODE_KERNEL_BACKTRACE,record_param);
               		tracking_info->allocate_map = (unsigned int *)get_record(HASH_NODE_KERNEL_SYMBOL,record_param);
            	}

		switch(node_type)
		{
			case NODE_MMAP:
			{
				struct ion_address_usage_record *tmp = (struct ion_address_usage_record *)new_node;
	                        if(from_kernel)
	     	                {
	                                tmp->address_type = record_param->address_type;
       		                }
                        	else
                        	{
                               		tmp->address_type = ADDRESS_USER_VIRTUAL;
                        	}
                        	tmp->fd = record_param->fd;
                        	tmp->mapping_address = record_param->address;
                        	tmp->size = record_param->length;
                       		tmp->buffer = record_param->buffer; 
				//add new node into allocate list
                        	mutex_lock(&process_record->ion_address_usage_mutex);
                        	insert_node_to_list((void **)&process_record->address_using_list,(unsigned int *)new_node);
                        	mutex_unlock(&process_record->ion_address_usage_mutex);

				break;
			}
			case NODE_FD:
			{
				struct ion_fd_usage_record *tmp = (struct ion_fd_usage_record *)new_node;
	                        tmp->fd = record_param->fd;
                       		tmp->buffer = record_param->buffer; 
				tmp->handle = record_param->handle;
				tmp->file = record_param->file;
				//add new node into allocate list
                        	mutex_lock(&process_record->ion_fd_usage_mutex);
                        	insert_node_to_list((void **)&process_record->fd_using_list,(unsigned int *)new_node);
                        	mutex_unlock(&process_record->ion_fd_usage_mutex);
				break;
			}
		}
		return new_node;
	}
	return NULL;
}
void *create_new_record_into_list(ion_sys_record_t *record_param,struct ion_buffer_record *buffer,unsigned int node_type,unsigned int from_kernel)
{
	unsigned int *new_node = NULL;
	struct ion_client *tmp = NULL;
	//assign data into buffer usage record
	new_node = (void *)allocate_record(node_type,record_param);
	if(new_node != NULL)
        {
	    struct ion_record_basic_info *tracking_info = NULL; 
	    tmp = record_param->client;	
	    tracking_info = (struct ion_record_basic_info *)(new_node+1);
            //assign data into new record
            tracking_info->recordID.pid = record_param->pid;
            tracking_info->recordID.group_pid = (pid_t)tmp->pid;
            tracking_info->recordID.client_address = (unsigned int)record_param->client; 
	    tracking_info->recordID.client = record_param->client; //FIXME it should be stored in hash table

	    //get_task_comm(tracking_info->recordID.task_comm, record_param->client->task);
	    tracking_info->recordID.buffer = buffer;
	    tracking_info->from_kernel = from_kernel;
            tracking_info->allocate_backtrace = NULL;
	    tracking_info->allocate_map = NULL;
	    tracking_info->release_backtrace = NULL;
	    tracking_info->release_map = NULL;
	    tracking_info->release_backtrace_type = BACKTRACE_MAX;
	    tracking_info->record_type = node_type;
	
	    if(!from_kernel)
	    {
		//tracking_info->allocate_backtrace_type = USER_BACKTRACE;
		tracking_info->allocate_backtrace = (unsigned int *)get_record(HASH_NODE_USER_BACKTRACE,record_param);
            	tracking_info->allocate_map = (unsigned int *)get_record(HASH_NODE_USER_MAPPING,record_param);
            }
	    else
	    {
		//tracking_info->allocate_backtrace_type = KERNEL_BACKTRACE;
		tracking_info->allocate_backtrace = (unsigned int *)get_record(HASH_NODE_KERNEL_BACKTRACE,record_param);
		tracking_info->allocate_map = (unsigned int *)get_record(HASH_NODE_KERNEL_SYMBOL,record_param);
	    }
	    switch(node_type)
	    {
		case NODE_BUFFER:
		{
			struct ion_buffer_usage_record *tmp = (struct ion_buffer_usage_record *)new_node;
            		tmp->handle = record_param->handle; //FIXME it should be stored in hash table
			tmp->fd = record_param->fd; // this is for ion share record
            		//ION_DEBUG_LOG(ION_DEBUG_INFO,"[create_new_record_into_list]new_node %x buffer_using_list%x\n",new_node,buffer->buffer_using_list);
            		
            		//add new node into allocate list
            		mutex_lock(&buffer->ion_buffer_usage_mutex);
            		insert_node_to_list((void **)&buffer->buffer_using_list,(unsigned int *)new_node);
            		mutex_unlock(&buffer->ion_buffer_usage_mutex);
			break;
		}

		case NODE_MMAP:
		{
			struct ion_address_usage_record *tmp = (struct ion_address_usage_record *)new_node;
			if(from_kernel)
			{
				tmp->address_type = record_param->address_type;
			}
			else
			{
				tmp->address_type = ADDRESS_USER_VIRTUAL;
			}
                        tmp->fd = record_param->fd;
			tmp->mapping_address = record_param->address;
			tmp->size = record_param->length;
                        //ION_DEBUG_LOG(ION_DEBUG_INFO,"[create_new_record_into_list]new_node %x address__using_list%x\n",new_node,buffer->address_using_list);
                        //add new node into allocate list
                        mutex_lock(&buffer->ion_address_usage_mutex);
                        insert_node_to_list((void **)&buffer->address_using_list,(unsigned int *)new_node); //FIXME
                        mutex_unlock(&buffer->ion_address_usage_mutex);
			break;	
		}
		case NODE_CLIENT:
		{
			struct ion_client_usage_record *tmp = (struct ion_client_usage_record *)new_node;
			//ION_DEBUG_LOG(ION_DEBUG_INFO,"[ION_FUNCTION_OPEN]new_node %x client_using_list%x\n",new_node,client_using_list);
			if(!from_kernel) {
  				tmp->fd = record_param->fd; 
			}
	                //add new node into allocate list
                        mutex_lock(&client_usage_mutex);
                        insert_node_to_list((void **)&client_using_list,(unsigned int *)new_node);
                        mutex_unlock(&client_usage_mutex);
			break;
		}
	    }
        }
        else
        {
            ION_DEBUG_LOG(ION_DEBUG_ERROR,"[create_new_record_into_list] can't get new node type \n");
        }
	return (void *)new_node;
}
unsigned int record_release_backtrace(ion_sys_record_t *record_param,struct ion_record_basic_info *tracking_info,unsigned int from_kernel)
{
	if((record_param != NULL) && (tracking_info != NULL))
	{	
		if(!from_kernel)
		{
			tracking_info->release_backtrace_type = USER_BACKTRACE;
			tracking_info->release_backtrace = (unsigned int *)get_record(HASH_NODE_USER_BACKTRACE,record_param);
			tracking_info->release_map = (unsigned int *)get_record(HASH_NODE_USER_MAPPING,record_param);
		}
		else
		{
			tracking_info->release_backtrace_type = KERNEL_BACKTRACE;
			tracking_info->release_backtrace = (unsigned int *)get_record(HASH_NODE_KERNEL_BACKTRACE,record_param);
			tracking_info->release_map = (unsigned int *)get_record(HASH_NODE_KERNEL_SYMBOL,record_param);
		}
		return 1;
	}
	return 0;
}
int record_ion_info(int from_kernel,ion_sys_record_t *record_param)
{
	//ION_DEBUG_LOG(ION_DEBUG_INFO, "  [%d][%d][FUNCTION(%d)_%d][record_ion_info]  \n",record_param->pid,record_param->group_id,from_kernel,record_param->action);
        {
                //store userspace infomation    
                switch(record_param->action)
                {

                        case ION_FUNCTION_OPEN:
			case ION_FUNCTION_CREATE_CLIENT:
                        {

				struct ion_client_usage_record *new_node = NULL;
				struct ion_process_record *process_record = NULL;
				struct ion_fd_usage_record *new_node2 = NULL;
				ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_OPEN(%d)] fd %d \n",record_param->pid,record_param->group_id,from_kernel,record_param->fd);

				//create client node
				new_node = (struct ion_client_usage_record *)create_new_record_into_list(record_param,NULL,NODE_CLIENT,from_kernel);
				
				//find process record in list
				if(from_kernel == 0)
				{
					//find process record in list
					mutex_lock(&process_lifecycle_mutex);
                               		process_record = search_process_in_list(record_param->pid, process_created_list);

					//If it can't find corresponding process record, we will create new record and insert new record into process_created_list
                               		if(process_record == NULL)
                               		{
						//create new process node
                               			process_record = (struct ion_process_record *)allocate_record(LIST_PROCESS,record_param);
                               			if(process_record != NULL)
                               			{
							ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_OPEN(%d)]create new process record 0x%x in process [%d] process created list %x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)process_record,(int)record_param->pid,(unsigned int)process_created_list);

                                               	}
						else
						{
							ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_OPEN(%d)] ERROR !!!process %d can't get enough for LIST_PROCESS structure\n",record_param->pid,record_param->group_id,from_kernel,(int)record_param->pid);
                                		}
					}
					else
					{
						process_record->count++;
						ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_OPEN(%d)]found process record 0x%x in process [%d] process->count %d \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)process_record,record_param->pid,process_record->count);

					}
					mutex_unlock(&process_lifecycle_mutex);
					 //assign data into fd record
	                                new_node2 = (struct ion_fd_usage_record *)create_new_record_into_process(record_param,process_record,NODE_FD,from_kernel);
       	                                ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_OPEN(%d)]done new fd node is %x insert node into fd using list %x  \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)new_node2,(unsigned int)process_record->fd_using_list);

				}
		               	ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_OPEN(%d)] DONE  new_node %x client_using_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)new_node,(unsigned int)client_using_list);
                                break;
                        }
                        case ION_FUNCTION_CLOSE:
			case ION_FUNCTION_DESTROY_CLIENT:
                        {
				ion_client_usage_record_t *found_node = NULL;
				ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_CLOSE(%d)]  client %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client);

				//move client node from allocate list into free list
				mutex_lock(&client_usage_mutex);
				found_node = move_node_to_freelist(record_param->pid,(unsigned int)record_param->client,0,(unsigned int **)&client_using_list,(unsigned int **)&client_freed_list,SEARCH_PID_CLIENT,NODE_CLIENT);
				mutex_unlock(&client_usage_mutex);
				if(ion_debugger_history)
				{
				if(found_node != NULL)
				{
					record_release_backtrace(record_param,&(found_node->tracking_info),from_kernel);	
				}
				else
				{
						ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_CLOSE(%d)]ERROR can't find client: 0x%x in client_using_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client,(unsigned int)client_using_list);
					break;
				}
				}
				if(from_kernel == 0)
				{
					//move process node to destroyed list if it referece count is 1	
					ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_CLOSE(%d)]  DONE prepare to move process %d into destroyed list %x\n",record_param->pid,record_param->group_id,from_kernel,record_param->pid,(unsigned int)process_destroyed_list);
					mutex_lock(&process_lifecycle_mutex);
					move_node_to_freelist(record_param->pid,0,0,(unsigned int **)&process_created_list, (unsigned int **)&process_destroyed_list,SEARCH_PROCESS_PID,LIST_PROCESS);
					mutex_unlock(&process_lifecycle_mutex);
					ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_CLOSE(%d)]  DONE process_destroyed_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)process_destroyed_list);	
                                }
				ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_CLOSE(%d)]  DONE client_freed_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)client_freed_list);

				break;
                        }
                        case ION_FUNCTION_ALLOC:
                        case ION_FUNCTION_ALLOC_MM:
                        case ION_FUNCTION_ALLOC_CONT:
                        {
				struct ion_buffer_record *new_buffer_record = NULL;
				struct ion_buffer_usage_record *new_node = NULL;
				ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_ALLOC(%d)_%d]buffer: 0x%x handle: 0x%x\n",record_param->pid,record_param->group_id,from_kernel,record_param->action,(unsigned int)record_param->buffer,(unsigned int)record_param->handle);
				if(record_param->buffer == NULL)
				{
					ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_ALLOC(%d)_%d]ERROR!!! buffer: 0x%x is NULL\n",record_param->pid,record_param->group_id,from_kernel,record_param->action,(unsigned int)record_param->buffer);
					break;
				}

				//create buffer node
				new_buffer_record = (struct ion_buffer_record *)allocate_record(LIST_BUFFER,record_param);
				if(new_buffer_record != NULL)
				{
					//assign data into buffer usage record
					new_node = (struct ion_buffer_usage_record *)create_new_record_into_list(record_param,new_buffer_record,NODE_BUFFER,from_kernel);
					if(new_node != NULL)
					{
						new_node->function_type = ION_FUNCTION_ALLOC;
					}
				}
				else
				{
					ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_ALLOC(%d)_%d]ERROR!!! buffer: 0x%x can't get  enough memory for create LIST_BUFFER structure\n",record_param->pid,record_param->group_id,from_kernel,record_param->action,(unsigned int)record_param->buffer);
					break;
				}
				ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_ALLOC(%d)_%d] DONE create buffer_usage_record : 0x%x buffer_created_list: 0x%x new buffer record: 0x%x buffer_using_list: 0x%x\n",record_param->pid,record_param->group_id,from_kernel,record_param->action,(unsigned int)new_node,(unsigned int)buffer_created_list,(unsigned int)new_buffer_record,(unsigned int)new_buffer_record->buffer_using_list);
                                break;
                        }
			case ION_FUNCTION_IMPORT:
			{
				//find buffer record in buffer created list
                                struct ion_buffer_record *buffer_record = NULL;
                                //struct ion_buffer_usage_record *found_node = NULL;
				struct ion_buffer_usage_record *new_node = NULL;
				ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_IMPORT(%d)]input buffer: 0x%x fd %d\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)record_param->fd);
				if(record_param->buffer == NULL)
                                {
                                        ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_IMPORT(%d)]ERROR!!! buffer: 0x%x is NULL\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer);
                                }
                                mutex_lock(&buffer_lifecycle_mutex);
                                buffer_record  = search_record_in_list(record_param->buffer,buffer_created_list);
                                mutex_unlock(&buffer_lifecycle_mutex);
                                if(buffer_record == NULL)
                                {
                                        ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_IMPORT(%d)]can't found corresponding buffer 0x%x in buffer created list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
                                        break;
                                }
				//assign data into buffer usage record
                                new_node = (struct ion_buffer_usage_record *)create_new_record_into_list(record_param,buffer_record,NODE_BUFFER,from_kernel);
				if(new_node != NULL)
                                {
                                        new_node->function_type = ION_FUNCTION_IMPORT;
                                }
 
			 	ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_IMPORT(%d)]DONE new_node %x buffer_using_list%x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)new_node,(unsigned int)buffer_record->buffer_using_list);	
				break;
			}

                        case ION_FUNCTION_FREE:
                        {
				//find buffer record in buffer created list 
				struct ion_buffer_record *buffer_record = NULL;
				struct ion_buffer_usage_record *found_node = NULL;
				ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_FREE(%d)]input buffer: 0x%x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer);
				if(record_param->buffer == NULL)
                                {
                                        ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_FREE(%d)]ERROR!!! buffer: 0x%x is NULL\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer);
                                }

				mutex_lock(&buffer_lifecycle_mutex);
				buffer_record  = search_record_in_list(record_param->buffer,buffer_created_list);
				mutex_unlock(&buffer_lifecycle_mutex);
				if(buffer_record == NULL)
				{
					ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_FREE(%d)]can't found corresponding buffer 0x%x in buffer created list %x",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
					break;
				}
				
                                //move buffer node from allocate list into free list
                                mutex_lock(&buffer_record->ion_buffer_usage_mutex);
                                found_node = move_node_to_freelist(record_param->pid,(unsigned int)record_param->client,0,(unsigned int **)&(buffer_record->buffer_using_list),(unsigned int **)&(buffer_record->buffer_freed_list),SEARCH_PID_CLIENT,NODE_BUFFER);
                                mutex_unlock(&buffer_record->ion_buffer_usage_mutex);
				if(ion_debugger_history)
				{
                                if(found_node != NULL)
                                {
					record_release_backtrace(record_param,&(found_node->tracking_info),from_kernel);	
                                }
				else
				{
						ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_FREE(%d)]can't found corresponding buffer usage record client 0x%x in buffer using list 0x%x",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client,(unsigned int)buffer_record->buffer_using_list);
                                        break;
					}
				}
				if(buffer_record->buffer_using_list == NULL)
                                {
					mutex_lock(&buffer_lifecycle_mutex);
					move_node_to_freelist(record_param->pid,0,(unsigned int)record_param->buffer,(unsigned int **)&buffer_created_list, (unsigned int **)&buffer_destroyed_list,SEARCH_BUFFER,LIST_BUFFER);
					mutex_unlock(&buffer_lifecycle_mutex);
                                        ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_FREE(%d)]move buffer record %xfrom buffer_created_list %x to buffer_destroyed_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)buffer_record,(unsigned int)buffer_created_list,(unsigned int)buffer_destroyed_list);

				}
				
				//count total buffer free list if buffer free list is full. remove olderest buffer record 
				destroyed_buffer_count++; //FIXME waiting for real case 
                               	ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_FREE(%d)] DONE buffer_using_list %x buffer_freed_list %x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)buffer_record->buffer_using_list,(unsigned int)buffer_record->buffer_freed_list);
 
				break;
                        }
                        case ION_FUNCTION_MMAP:
                        {
				//find buffer record in buffer created list
                                struct ion_buffer_record *buffer_record = NULL;
				struct ion_process_record *process_record = NULL;
				struct ion_address_usage_record *new_node = NULL;
				struct ion_address_usage_record *new_node2 = NULL;
				ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_MMAP(%d)]input buffer: 0x%x mapping_address %x length %d\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,record_param->address,record_param->length);
				if((record_param->buffer == NULL) && (record_param->fd != 0) && !from_kernel)
				{
					struct dma_buf *dmabuf;
					dmabuf = dma_buf_get(record_param->fd);
 					if(dmabuf->priv != NULL);
					{
						record_param->buffer = dmabuf->priv;	
						dma_buf_put(dmabuf);
					}
					ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_MMAP(%d)]get buffer from fd %d input buffer: 0x%x \n",record_param->pid,record_param->group_id,from_kernel,record_param->fd,(unsigned int)record_param->buffer);
				}
				if(record_param->buffer == NULL)
                                {
                                        ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_MMAP(%d)]ERROR!!! buffer: 0x%x is NULL\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer);
                                        break;
                                }
				if(from_kernel)
				{	
					//find corresponding buffer in created buffer list
                                	mutex_lock(&buffer_lifecycle_mutex);
                                	buffer_record  = search_record_in_list(record_param->buffer,buffer_created_list);
                           		mutex_unlock(&buffer_lifecycle_mutex);
                   			if(buffer_record == NULL)
                                	{
                                       		ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_MMAP(%d)]ERROR !!! can't found corresponding buffer 0x%x in buffer created list %x",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
                                        	break;
                                	}
					//assign data into mmap record
                                	new_node = (struct ion_address_usage_record *)create_new_record_into_list(record_param,buffer_record,NODE_MMAP,from_kernel);
					ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_MMAP(%d)] DONE new node:  0x%x in buffer record address mapping 0x%x size %d address using list: 0x%x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)new_node,new_node->mapping_address,new_node->size,(unsigned int)buffer_record->address_using_list);
				}
				else
				{	
					//find corresponding process in created process list
					mutex_lock(&process_lifecycle_mutex);
					process_record = search_process_in_list(record_param->pid, process_created_list);	
					mutex_unlock(&process_lifecycle_mutex);	
 					if(process_record == NULL)
               	                	{
           	                        	ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_MMAP(%d)]ERROR !!! can't found corresponding process %d in process created list %x",record_param->pid,record_param->group_id,from_kernel,record_param->pid,(unsigned int)process_created_list);
               	                        	break;
                                	}
					new_node2 = (struct ion_address_usage_record *)create_new_record_into_process(record_param,process_record,NODE_MMAP,from_kernel);
                                	ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_MMAP(%d)] DONE new node 0x%x in process record address mapping 0x%x size %d address using list 0x%x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)new_node2,(unsigned int)new_node2->mapping_address,new_node2->size,(unsigned int)process_record->address_using_list);
				}	 
                                break;
                        }
                        case ION_FUNCTION_MUNMAP:
                        {
				//find buffer record in buffer created list
                                struct ion_buffer_record *buffer_record = NULL;
                                struct ion_address_usage_record *found_node = NULL;
				struct ion_address_usage_record *found_node2 = NULL;
				struct ion_process_record *process_record = NULL;
				//struct ion_address_uage_record *found_ndoe2 = NULL;
				ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_MUNMAP(%d)]input buffer: 0x%x address 0x%x size %d\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,record_param->address,record_param->length);
				if(from_kernel)
				{	
                    		        mutex_lock(&buffer_lifecycle_mutex);
                       	        	buffer_record  = search_record_in_list(record_param->buffer,buffer_created_list);
                    	        	mutex_unlock(&buffer_lifecycle_mutex);
                    	        	if(buffer_record == NULL)
                                	{
                                        	ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_MUNMAP(%d)]ERROR !!! can't found corresponding buffer 0x%x in buffer created list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
                                        	break;
                                	}
				
					//move buffer node from allocate list into free list
                                	mutex_lock(&buffer_record->ion_address_usage_mutex);
                                	found_node = move_node_to_freelist(record_param->pid,(unsigned int )record_param->client,record_param->address,(unsigned int **)&(buffer_record->address_using_list),(unsigned int **)&(buffer_record->address_freed_list),SEARCH_PID,NODE_MMAP);
                                	mutex_unlock(&buffer_record->ion_address_usage_mutex);
					if(ion_debugger_history)
					{
                                	if(found_node != NULL)
                                	{
                               			record_release_backtrace(record_param,&(found_node->tracking_info),from_kernel); 
					}
					else
					{
                                        		ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_MUNMAP(%d)]can't found corresponding buffer usage record client 0x%x in address using list 0x%x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client,(unsigned int)buffer_record->address_using_list);
                                        	break;
                                		}
                                	}
				}
				else
				{
					mutex_lock(&process_lifecycle_mutex);
                                	process_record  = search_process_in_list(record_param->pid,process_created_list);
                                	mutex_unlock(&process_lifecycle_mutex);
                                	if(process_record == NULL)
                                	{
                                        	ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_MUNMAP(%d)]ERROR !!! can't found corresponding process pid %d in process created list %x\n",record_param->pid,record_param->group_id,from_kernel,record_param->pid,(unsigned int)process_created_list);
                                        	break;
                                	}

					//move buffer node from allocate list into free list
                                	mutex_lock(&process_record->ion_address_usage_mutex);
                                	found_node2 = move_node_to_freelist(record_param->pid,(unsigned int)record_param->client,record_param->address,(unsigned int **)&(process_record->address_using_list),(unsigned int **)&(process_record->address_freed_list),SEARCH_PID,NODE_MMAP);
                                	mutex_unlock(&process_record->ion_address_usage_mutex);
					if(ion_debugger_history)
					{
                                	if(found_node2 != NULL)
                                	{
                               			record_release_backtrace(record_param,&(found_node2->tracking_info),from_kernel); 
					}
                                	else
                                	{
                                        		ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_MUNMAP(%d)]can't found corresponding address usage record process 0x%d in address using list 0x%x\n",record_param->pid,record_param->group_id,from_kernel,record_param->pid,(unsigned int)process_record->address_using_list);
                                        	break;
                                	}
					}
                               			ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_MUNMAP(%d)] DONE client %x process->address_using_list %x process->address_free_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int )record_param->client,(unsigned int)process_record->address_using_list,(unsigned int)process_record->address_freed_list);
				} 
				break;
                        }
                        case ION_FUNCTION_SHARE:
                        {
				//create fd record 
				//add fd record into allocate list
                                struct ion_process_record *process_record = NULL;
                                struct ion_fd_usage_record *new_node = NULL;
				//find buffer record in buffer created list
                                struct ion_buffer_record *buffer_record = NULL;
                                //struct ion_buffer_usage_record *found_node = NULL;
                                struct ion_buffer_usage_record *new_buffer_node = NULL;
                                //if((from_kernel != 1) && (record_param->fd != 0))
                                {
                                        struct files_struct *files = current->files;
                                        struct fdtable *fdt;
                                        struct file * filp;
                                        spin_lock(&files->file_lock);
                                        fdt = files_fdtable(files);
                                        if (record_param->fd >= fdt->max_fds)
                                                goto out_unlock;
                                        filp = fdt->fd[record_param->fd];
out_unlock:
                                        record_param->file = filp;
                                        spin_unlock(&files->file_lock);
                                }

				ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_SHARE(%d)]input buffer: 0x%x fd %d file 0x%x current %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,record_param->fd,(unsigned int)record_param->file,(unsigned int)current);
                                mutex_lock(&process_lifecycle_mutex);
                                process_record = search_process_in_list(record_param->pid,process_created_list);
                                mutex_unlock(&process_lifecycle_mutex);
                                if(process_record == NULL)
                                {
                                        ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_SHARE(%d)] can't found corresponding process %d in process created list %x",record_param->pid,record_param->group_id,from_kernel,record_param->pid,(unsigned int)process_created_list);
					if(from_kernel == 1)
                                	{
                                        	//find process record in list
                                        	mutex_lock(&process_lifecycle_mutex);
                                        	process_record = search_process_in_list(record_param->pid, process_created_list);

                                        	//If it can't find corresponding process record, we will create new record and insert new record into process_created_list
                                        	if(process_record == NULL)
                                        	{
                                                	//create buffer node
                                                	process_record = (struct ion_process_record *)allocate_record(LIST_PROCESS,record_param);
                                                	if(process_record != NULL)
                                                	{
								ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_SHARE(%d)]create new process record 0x%x in process [%d] count %d process created list %x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)process_record,(int)record_param->group_id,process_record->count,(unsigned int)process_created_list);

	                                               	}
							else
							{
                                                		ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_SHARE(%d)] ERROR !!!process [%d] can't get enough memory to create LIST_PROCESS strucutre \n",record_param->pid,record_param->group_id,from_kernel,(int)record_param->group_id);
							}
                                		}
						mutex_unlock(&process_lifecycle_mutex);
					}
					else
                                        {
                                                ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_SHARE(%d)]Error !!!!Userspace should find correspond process to  buffer 0x%x in buffer created list %x",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
                                        	break;
                                	}

				}

                                //assign data into fd record
                                new_node = (struct ion_fd_usage_record *)create_new_record_into_process(record_param,process_record,NODE_FD,from_kernel);
                                ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_SHARE(%d)] new process node is %x insert node into fd using list %x  \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)new_node,(unsigned int)process_record->fd_using_list);
				if(record_param->buffer == NULL)
                                {
                                        ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_SHARE(%d)]ERROR!!! buffer: 0x%x is NULL\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer);
                                        break;                                
                                }
                                mutex_lock(&buffer_lifecycle_mutex);
                                buffer_record  = search_record_in_list(record_param->buffer,buffer_created_list);
                                mutex_unlock(&buffer_lifecycle_mutex);
                                if(buffer_record == NULL)
                                {
                                        ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_SHARE(%d)]can't found corresponding buffer 0x%x in buffer created list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
                                        break;
                                }
                                //assign data into buffer usage record
                                new_buffer_node = (struct ion_buffer_usage_record *)create_new_record_into_list(record_param,buffer_record,NODE_BUFFER,from_kernel);
                                if(new_buffer_node != NULL)
                                {
                                        new_buffer_node->function_type = ION_FUNCTION_SHARE;
					new_buffer_node->file = record_param->file;
                                }

                                ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_SHARE(%d)]DONE new_buffer_node %x buffer_using_list%x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)new_buffer_node,(unsigned int)buffer_record->buffer_using_list);
				break;
                        }
                        case ION_FUNCTION_SHARE_CLOSE:
                        {
                                struct ion_fd_usage_record *found_node = NULL;
				struct ion_process_record *process_record = NULL;

				ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]input buffer: 0x%x fd %d file 0x%x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,record_param->fd,(unsigned int)record_param->file);
                                mutex_lock(&process_lifecycle_mutex);
				if(record_param->fd != 0)
				{
                                	process_record  = search_process_in_list(record_param->pid,process_created_list);
				}
				if(process_record == NULL)
                                {
                                        process_record  = search_process_by_file(record_param->file,process_created_list);
                                }
                                mutex_unlock(&process_lifecycle_mutex);

                                if(process_record == NULL)
                                {
                                        ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]ERROR !!!can't found corresponding process %d in process created list %x\n",record_param->file,record_param->group_id,from_kernel,(unsigned int)record_param->pid,(unsigned int)process_created_list);
                                        break;
                                }

				//move fd node from allocate list into free list
                                mutex_lock(&process_record->ion_fd_usage_mutex);
				if(record_param->fd != 0)
				{
                                	found_node = move_node_to_freelist(record_param->pid,(unsigned int)record_param->client,record_param->fd,(unsigned int **)&(process_record->fd_using_list),(unsigned int **)&(process_record->fd_freed_list),SEARCH_PID,NODE_FD);
				}
				else
				{
					found_node = move_node_to_freelist(record_param->pid,(unsigned int)record_param->client,record_param->file,(unsigned int **)&(process_record->fd_using_list),(unsigned int **)&(process_record->fd_freed_list),SEARCH_FILE,NODE_FD);
				}
                                mutex_unlock(&process_record->ion_fd_usage_mutex);
				if(ion_debugger_history)
				{
                                	if((found_node != NULL) && ion_debugger_history)
                                	{
                               			record_release_backtrace(record_param,&(found_node->tracking_info),from_kernel); 
					}
					else
                                	{
                                       		ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]can't found corresponding process  record client 0x%x in fd using list 0x%x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client,(unsigned int)process_record->fd_using_list);
                                        	break;

                                	}
				}

				//move fd record to free list
                                ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)] client %x fd_using_list %x fd_freed_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client,(unsigned int)process_record->fd_using_list,(unsigned int)process_record->fd_freed_list);
#if 1
{				
				//find buffer record in buffer created list
                                struct ion_buffer_record *buffer_record = NULL;
                                struct ion_buffer_usage_record *found_node = NULL;

                                ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]input buffer: 0x%x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer);
                                if(record_param->buffer == NULL)
                                {
                                        ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]ERROR!!! buffer: 0x%x is NULL\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer);
                                        break;
                                }
                                mutex_lock(&buffer_lifecycle_mutex);
                                buffer_record  = search_record_in_list(record_param->buffer,buffer_created_list);
                                mutex_unlock(&buffer_lifecycle_mutex);
                                if(buffer_record == NULL)
                                {
                                        ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]can't found corresponding buffer 0x%x in buffer created list %x",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
                                        break;
                                }

                                //move buffer node from allocate list into free list
                                mutex_lock(&buffer_record->ion_buffer_usage_mutex);
                                found_node = move_node_to_freelist(record_param->pid,(unsigned int)0,record_param->file,(unsigned int **)&(buffer_record->buffer_using_list),(unsigned int **)&(buffer_record->buffer_freed_list),SEARCH_FD_GPID,NODE_BUFFER);
                                mutex_unlock(&buffer_record->ion_buffer_usage_mutex);
				if(ion_debugger_history)
				{
                                	if(found_node != NULL)
                                	{
                               			record_release_backtrace(record_param,&(found_node->tracking_info),from_kernel); 
					}
					else
                                	{
                                        	ION_DEBUG_LOG(ION_DEBUG_ERROR, "    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]can't found corresponding buffer usage record client 0x%x in buffer using list 0x%x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client,(unsigned int)buffer_record->buffer_using_list);
                                        	break;
                                	}
				}
                                if(buffer_record->buffer_using_list == NULL)
                                {
                                        mutex_lock(&buffer_lifecycle_mutex);
                                        move_node_to_freelist(record_param->pid,0,(unsigned int)record_param->buffer,(unsigned int **)&buffer_created_list, (unsigned int **)&buffer_destroyed_list,SEARCH_BUFFER,LIST_BUFFER);
                                        mutex_unlock(&buffer_lifecycle_mutex);
                                        ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]move buffer record %xfrom buffer_created_list %x to buffer_destroyed_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)buffer_record,(unsigned int)buffer_created_list,(unsigned int)buffer_destroyed_list);

                                }

                                //count total buffer free list if buffer free list is full. remove olderest buffer record
                                destroyed_buffer_count++; //FIXME waiting for real case
                                ION_DEBUG_LOG(ION_DEBUG_TRACE, "    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)] DONE buffer_using_list %x buffer_freed_list %x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)buffer_record->buffer_using_list,(unsigned int)buffer_record->buffer_freed_list);
}
#endif
                                break;
                        }
                        default:
                                ION_DEBUG_LOG(ION_DEBUG_ERROR, "[ERROR]record_ion_info error action\n");
                }

        }
	//ION_DEBUG_LOG(ION_DEBUG_INFO, "  [%d][%d][FUNCTION(%d)_%d][record_ion_info] DONE\n",record_param->pid,record_param->group_id,from_kernel,record_param->action);
	return 1;
}

