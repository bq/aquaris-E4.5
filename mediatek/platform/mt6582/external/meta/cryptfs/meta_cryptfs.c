#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <time.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>

#include "FT_Public.h"
#include "meta_common.h"
#include "meta_cryptfs_para.h"


#undef  LOG_TAG
#define LOG_TAG  "CRYPTFS_META"


/********************************************************************************
//FUNCTION:
//		META_CRYPTFS_init
//DESCRIPTION:
//		CRYPTFS Init for META test.
//
//PARAMETERS:
//		void
//RETURN VALUE:
//		true : success
//      false: failed
//
********************************************************************************/
bool META_CRYPTFS_init()
{
	LOGD("META_CRYPTFS_init ...\n");
	return 1;
}

/********************************************************************************
//FUNCTION:
//		META_CRYPTFS_deinit
//DESCRIPTION:
//		CRYPTFS deinit for META test.
//
//PARAMETERS:
//		void
//RETURN VALUE:
//		void
//     
********************************************************************************/
void META_CRYPTFS_deinit()
{
	LOGD("META_CRYPTFS_deinit ...\n");
	return;   
}


int get_encrypt_phone_status()
{
	char crypto_state[PROPERTY_VALUE_MAX];
	property_get("ro.crypto.state", crypto_state, "");

    if ( !strcmp(crypto_state, "encrypted")){
         return 1;
    }
    else { 
         return 0;
    }
}
int decrypt_data(char *passwd)
{
    int rtn=0;
    char cmd[255] = {'\0'};
    sprintf(cmd, "cryptfs checkpw %s", passwd);

    rtn = do_cmd_for_cryptfs(cmd);
	if (rtn) {
      LOGE("Fail: cryptfs checkpw, err=%d\n", rtn);
	  return  rtn;
    }

	rtn = do_cmd_for_cryptfs("cryptfs restart");
	 if (rtn) {
	   LOGE("Fail: cryptfs restart, err=%d\n", rtn);
	   return  rtn;
	 }

    return 0;
}

int do_cmd_for_cryptfs(char* cmd) {
   
    int i;
    int ret;
    int sock;
    char final_cmd[255] = "0 "; /* 0 is a (now required) sequence number */
	ret = strlcat(final_cmd, cmd, sizeof(final_cmd));
	if (ret >= sizeof(final_cmd)) {
		LOGE("Fail: the cmd is too long (%s)", final_cmd);
		return (-1);
    }

    if ((sock = socket_local_client("vold",
                                     ANDROID_SOCKET_NAMESPACE_RESERVED,
                                     SOCK_STREAM)) < 0) {
        LOGE("Error connecting (%s)\n", strerror(errno));
        exit(4);
    }
	LOGD("do_cmd_for_cryptfs: %s\n", final_cmd);

    if (write(sock, final_cmd, strlen(final_cmd) + 1) < 0) {
        LOGE("Fail: write socket");
        return errno;
    }

    ret = do_monitor_for_cryptfs(sock, 1);
	close(sock);

    return ret;
}

int do_monitor_for_cryptfs(int sock, int stop_after_cmd) {
    char *buffer = malloc(4096);

    if (!stop_after_cmd)
        LOGD("[Connected to Vold]\n");

    while(1) {
        fd_set read_fds;
        struct timeval to;
        int rc = 0;

        to.tv_sec = 10;
        to.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);

        if ((rc = select(sock +1, &read_fds, NULL, NULL, &to)) < 0) {
            LOGE("Error in select (%s)\n", strerror(errno));
            free(buffer);
            return errno;
        } else if (!rc) {
            continue;
            LOGE("[TIMEOUT]\n");
            return ETIMEDOUT;
        } else if (FD_ISSET(sock, &read_fds)) {
            memset(buffer, 0, 4096);
            if ((rc = read(sock, buffer, 4096)) <= 0) {
                if (rc == 0)
                    LOGE("Lost connection to Vold - did it crash?\n");
                else
                    LOGE("Error reading data (%s)\n", strerror(errno));
                free(buffer);
                if (rc == 0)
                    return ECONNRESET;
                return errno;
            }
            
            int offset = 0;
            int i = 0;

            for (i = 0; i < rc; i++) {
                if (buffer[i] == '\0') {
                    int code;
					int rtn_code= -1;
                    char tmp[4];
                    char* token;

                    strncpy(tmp, buffer + offset, 3);
                    tmp[3] = '\0';
                    code = atoi(tmp);

                    LOGD("'%s'\n", buffer + offset);
                    if (stop_after_cmd) {
                        if (code >= 200 && code < 600) {
                            if (code == 200) {
                                if (strlen(buffer+offset) > 4) {
                                   token = strtok(buffer+offset+4, " ");								  
								   token = strtok(NULL, " ");
                                   rtn_code = atoi(token);
                                }
                                LOGD("cryptfs cmd, rtn_code=%d\n", rtn_code);
                                free(buffer);
                                return rtn_code;                                
                            }
                            else {
                                LOGE("invalid cryptfs cmd \n");
                                free(buffer);
                                return -1;
                            }
                        }
                    }
                    offset = i + 1;
                }
            }
        }
    }
    free(buffer);
    return 0;
}

/********************************************************************************
//FUNCTION:
//		META_CRYPTFS_OP
//DESCRIPTION:
//		META CRYPTFS test main process function.
//
//PARAMETERS:
//
//RETURN VALUE:
//		void
//      
********************************************************************************/
void META_CRYPTFS_OP(FT_CRYPTFS_REQ *req) 
{
	LOGD("req->op:%d\n", req->op);
	int ret = 0;
	FT_CRYPTFS_CNF cryptfs_cnf;
	memcpy(&cryptfs_cnf, req, sizeof(FT_H) + sizeof(CRYPTFS_OP));
	cryptfs_cnf.header.id ++; 
	switch (req->op) {
	      case CRYPTFS_OP_QUERY_STATUS:              
              {
                  bool encrypted_status = 0;
				  cryptfs_cnf.m_status = META_SUCCESS;
				  encrypted_status = get_encrypt_phone_status();
				  LOGD("encrypted_status:%d \n", encrypted_status);
		    	  cryptfs_cnf.result.query_status_cnf.status = encrypted_status;
				  WriteDataToPC(&cryptfs_cnf, sizeof(FT_CRYPTFS_CNF), NULL, 0);
              }
			break;

	      case CRYPTFS_OP_VERIFY:
              {
                  char* pw = req->cmd.verify_req.pwd;
				  int pw_len = req->cmd.verify_req.length;

				  cryptfs_cnf.m_status = META_SUCCESS;             
	              LOGD("pw = %s, pw_len = %d \n", pw, pw_len);
                  if (pw_len < 4  || pw_len > 16) {
					  cryptfs_cnf.result.verify_cnf.decrypt_result = 0;   
					  LOGE("Invalid passwd length =%d \n", pw_len);
					  WriteDataToPC(&cryptfs_cnf, sizeof(FT_CRYPTFS_CNF), NULL, 0);
                      break;
                  }
                   
				  if(!decrypt_data(pw)) {
				     cryptfs_cnf.result.verify_cnf.decrypt_result = 1;			  
	              }
	              else {
	                cryptfs_cnf.result.verify_cnf.decrypt_result = 0;			   
	              }

				  LOGD("verify result:%d \n", cryptfs_cnf.result.verify_cnf.decrypt_result);
				  WriteDataToPC(&cryptfs_cnf, sizeof(FT_CRYPTFS_CNF), NULL, 0);
              }
			break;

	      default:	
            LOGE("Error: unsupport op code = %d\n", req->op);	  	
			break;
	}		
}





