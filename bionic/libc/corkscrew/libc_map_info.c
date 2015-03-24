/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "libc_map_info.h"
#include "libc_corkscrew.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>


 map_info_t *g_ProcessMapsNeed[2]; //only save the first excute in maps
static uintptr_t   g_maxProcessMaps=0;
// 6f000000-6f01e000 rwxp 00000000 00:0c 16389419   /system/lib/libcomposer.so\n
// 012345678901234567890123456789012345678901234567890123456789
// 0         1         2         3         4         5
static map_info_t* parse_maps_line(const char* line)
{
    unsigned long int start;
    unsigned long int end;
    char permissions[5];
    int name_pos;
    if (sscanf(line, "%lx-%lx %4s %*x %*x:%*x %*d%n", &start, &end,
            permissions, &name_pos) != 3) {
        return NULL;
    }

    while (isspace(line[name_pos])) {
        name_pos += 1;
    }
    const char* name = line + name_pos;
    size_t name_len = strlen(name);
    if (name_len && name[name_len - 1] == '\n') {
        name_len -= 1;
    }
		if(strstr(name,"/dev/pvrsrvkm")) //ignore alloc mem section in 3d driver
			return NULL;
		if(strstr(permissions,"r--p")) //ignore r--p section
			return NULL;
    map_info_t* mi = dlcalloc(1, sizeof(map_info_t) + name_len + 1);
    if (mi) {
    		if(permissions[0] == 'r' ||permissions[1] == 'w' ||permissions[2] == 'x')
				{
        		mi->start = start;
        		mi->end = end;
				
		        mi->is_readable = strlen(permissions) == 4 && permissions[0] == 'r';
		        mi->is_writable = strlen(permissions) == 4 && permissions[1] == 'w';
		        mi->is_executable = strlen(permissions) == 4 && permissions[2] == 'x';
		        mi->data = NULL;
		        memcpy(mi->name, name, name_len);
		        mi->name[name_len] = '\0';
		        ALOGV("Parsed map: start=0x%08x, end=0x%08x, "
		                "is_readable=%d, is_writable=%d, is_executable=%d, name=%s",
		                mi->start, mi->end,
		                mi->is_readable, mi->is_writable, mi->is_executable, mi->name);
		    }
		    else
		    {	
		    	dlfree(mi);
		    	mi=NULL;
		    }	
  	}
    return mi;
}

extern uint32_t Exidx_Cache_size;
extern ExidxTable gExidxTable;


int filegets(char *buf, int size,  int fd) {
    char tmp;
	int real_size=0;
    if (size == 0 || buf == NULL)
        return 0;

    while(size--){
		read(fd, &tmp, 1);
        if(tmp == EOF)
            return 0;
        if (tmp == '\n') {
            buf[real_size] = '\0';
            break;
        }

        buf[real_size++] = tmp;
    }

    
    return real_size;
}


map_info_t* load_map_info_list(pid_t tid) {
    char path[PATH_MAX];
    char line[1024];
    map_info_t* milist = NULL;
	int fd;
	g_maxProcessMaps=0;
	Exidx_Cache_size=0;
	memset(&gExidxTable,sizeof(gExidxTable),0);
	
    snprintf(path, PATH_MAX, "/proc/%d/maps", tid);
   
	
   fd = open(path, O_RDONLY);
	if (fd < 0) {
        ALOGD("%s: open device failed \n",path);
        return false;
    }
	else
	{
		if(filegets(line, sizeof(line), fd))
		{
			ALOGV("the first maps:%s\n",line);
		 	map_info_t* mi = parse_maps_line(line);
		    if(mi) 
			{
		        mi->next = milist;
		        milist = mi;	
				g_ProcessMapsNeed[0]=milist;
				g_maxProcessMaps++;
				
		    }

		}
	}
	 close(fd);
	
    ALOGV("P-MAPS:%d",g_maxProcessMaps);	
    return milist;
}

void free_map_info_list(map_info_t* milist) {
    while (milist) {
        map_info_t* next = milist->next;
        dlfree(milist);
        milist = next;
    }
}




const map_info_t* find_map_info(const map_info_t* milist, uintptr_t addr) {
    const map_info_t* mi = milist;
	int high,low;
	high = g_maxProcessMaps-2;//假设数组是从小到大排列的
	low = 0;
	int midle = (g_maxProcessMaps)>>1;  
	while(high >= low)
	{
			midle = (high + low)>>1;
			if(g_ProcessMapsNeed[midle]==NULL)
      {
         	  ALOGD("maps null, midle:%d,Max:%d",midle,g_maxProcessMaps);	
         	  return NULL;
      }  
			if(addr >= g_ProcessMapsNeed[midle]->start && addr < g_ProcessMapsNeed[midle]->end)
					return g_ProcessMapsNeed[midle];
			if(g_ProcessMapsNeed[midle]->start > addr)
					high = midle - 1;         //前提是假设数组是从小到大排序，否则不确定是该加1还是减1
			else if(g_ProcessMapsNeed[midle]->end < addr )
					low = midle + 1;
	}
	while (mi && !(addr >= mi->start && addr < mi->end)) 
	{
		mi = mi->next;
	}
	ALOGD("binarysearch fail");
	return mi;

}

bool is_readable_map(const map_info_t* milist, uintptr_t addr) {
    const map_info_t* mi = find_map_info(milist, addr);
    return mi && mi->is_readable;
}

bool is_writable_map(const map_info_t* milist, uintptr_t addr) {
    const map_info_t* mi = find_map_info(milist, addr);
    return mi && mi->is_writable;
}

bool is_executable_map(const map_info_t* milist, uintptr_t addr) {
    const map_info_t* mi = find_map_info(milist, addr);
    return mi && mi->is_executable;
}

static pthread_mutex_t g_my_map_info_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static map_info_t* g_my_map_info_list = NULL;

static const int64_t MAX_CACHE_AGE = 50 * 1000 * 1000000LL;

typedef struct {
    uint32_t refs;
    int64_t timestamp;
} my_map_info_data_t;

static int64_t now() {
    struct timespec t;
    t.tv_sec = t.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000000000LL + t.tv_nsec;
}

static void dec_ref(map_info_t* milist, my_map_info_data_t* data) {
    if (!--data->refs) {
        ALOGV("Freed my_map_info_list %p.", milist);
        dlfree(data);
        free_map_info_list(milist);
		Release_Exidx_entry(&gExidxTable);
    }
}

map_info_t* acquire_my_map_info_list() {
    pthread_mutex_lock(&g_my_map_info_list_mutex);

    int64_t time = now();
    if (g_my_map_info_list) {
        my_map_info_data_t* data = (my_map_info_data_t*)g_my_map_info_list->data;
        int64_t age = time - data->timestamp;
        if (age >= MAX_CACHE_AGE) {
            ALOGV("Invalidated my_map_info_list %p, age=%lld.", g_my_map_info_list, age);
            dec_ref(g_my_map_info_list, data);
            g_my_map_info_list = NULL;
        } else {
            ALOGV("Reusing my_map_info_list %p, age=%lld.", g_my_map_info_list, age);
        }
    }

    if (!g_my_map_info_list) {
        my_map_info_data_t* data = (my_map_info_data_t*)dlmalloc(sizeof(my_map_info_data_t));
        g_my_map_info_list = load_map_info_list(getpid());
        if (g_my_map_info_list) {
            ALOGV("Loaded my_map_info_list %p.", g_my_map_info_list);
            g_my_map_info_list->data = data;
            data->refs = 1;
            data->timestamp = time;
        } else {
            dlfree(data);
        }
    }

    map_info_t* milist = g_my_map_info_list;
    if (milist) {
        my_map_info_data_t* data = (my_map_info_data_t*)g_my_map_info_list->data;
        data->refs += 1;
    }

    pthread_mutex_unlock(&g_my_map_info_list_mutex);
    return milist;
}

void release_my_map_info_list(map_info_t* milist) {
    if (milist) {
        pthread_mutex_lock(&g_my_map_info_list_mutex);

        my_map_info_data_t* data = (my_map_info_data_t*)milist->data;
        dec_ref(milist, data);

        pthread_mutex_unlock(&g_my_map_info_list_mutex);
    }
}
