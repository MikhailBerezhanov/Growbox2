#include <pthread.h> 
#include <sys/types.h> 
#include <stdlib.h>
#include <stdio.h> 
#include <signal.h>
#include <unistd.h>
#include <fcgi_config.h>
#include <fcgiapp.h>
#include <fcgios.h>

#include "app.h"
#include "httpserv.h"
#include "utils.h"

#define DEBUG_MSG   1
#define DEBUG_MODULE_NAME   "[ MAIN ]"
#include "dbgmsg.h"

#define THREAD_COUNT    4 
       
static void signals_waiter_init(void);
static void *request_handler(void *arg);

static int fcgx_socket; 
static bool terminated = false;
static pthread_t id[THREAD_COUNT];
static pthread_t id_tw;
static volatile sig_atomic_t sig_num = 0;

int main(int argc, char* argv[]) 
{ 
    int res = 0;
    char tmp[1024];
    memset(tmp, 0, sizeof(tmp));

    if(argc > 1)
    {
        if(!strcmp(argv[1], "--version")){
          printf("Validators' control server. Version: %s (build: '%s %s')\n", APP_VERSION, __DATE__, __TIME__);
          exit(0);
        }
        else{
          printf("Unsupported key. List of available keys:\n\t--version\tshow application version\n");
          exit(0);
        }
    }

    if((res = app_init()) > 0 ) exit(res);

    signals_waiter_init();

    FCGX_Init(); 

    // Open new socket and set access mode
    fcgx_socket = FCGX_OpenSocket(app_data.cgi_sock_path, 20); 
    if(fcgx_socket < 0) { 
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "FCGX_OpenSocket() Error\n");
	    exit(10); 
    } 
    sprintf(tmp, "chmod 666 %s", app_data.cgi_sock_path);
    if(system(tmp)){
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to set access for unix socket \n");
        exit(11);
    }

    // Create serving threads 
    for(int i = 0; i < THREAD_COUNT; i++) { 
        pthread_create(&id[i], NULL, request_handler, NULL); 
    } 

    // Wait threads
    for(int i = 0; i < THREAD_COUNT; i++) { 
        pthread_join(id[i], NULL); 
    } 

    dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "~ Cleaning-up ~\n");
    // FCGX deinit
    OS_IpcClose(fcgx_socket);
    OS_LibShutdown();
    // Termination waiter deinit
    pthread_cancel(id_tw);
    pthread_join(id_tw, NULL);
    
    dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "~ Gbweb stopped ~\n");
    exit(0); 
}


static void sig_hdl(int sig)
{
    sig_num = sig;
}

static void *termination_waiter(void* arg)
{
    // Enable cancelling thread from main
    if(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)){
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "pthread_setcancelstate() failed\n");
    }
    if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL)){
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "pthread_setcanceltype() failed\n");
    }

    for (;;) 
    {
        usleep(2000);

        if(sig_num){
            dbgmsg_v(MSG_VERBOSE, "!! Catched signal (%d) !!\n", sig_num);

            switch(sig_num){
                case SIGQUIT:
                case SIGTERM:
                case SIGINT:
                        dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "~ Terminated (%d) ~\n", sig_num);
                        dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "~ Cancelling working threads ~\n");
                        app_deinit();
                        for(int i = 0; i < THREAD_COUNT; i++) { 
                            if (pthread_cancel(id[i])) errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unable to cansel thread[%d]\n", i); 
                        }

                        return 0;

                case SIGHUP: 
                        dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "~ Reading config file ~\n");
                        app_read_config();
                        sig_num = 0;
                        break;
            }
        }
    }

    return 0;
}



static void signals_waiter_init(void)
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = sig_hdl;

    sigemptyset(&act.sa_mask);                                                             
    sigaddset(&act.sa_mask, SIGINT); 
    sigaddset(&act.sa_mask, SIGQUIT);
    sigaddset(&act.sa_mask, SIGTERM);
    sigaddset(&act.sa_mask, SIGHUP);
    sigaction(SIGINT, &act, 0);
    sigaction(SIGQUIT, &act, 0);
    sigaction(SIGTERM, &act, 0);
    sigaction(SIGHUP, &act, 0);

    if(pthread_create(&id_tw, NULL, termination_waiter, NULL)) 
        {errmsg_v(MSG_DEBUG | MSG_TO_FILE, "pthread_create(termination_waiter) failed\n");}   
}

static void *request_handler(void *arg) 
{ 
    int rc; 
    FCGX_Request request;  

    // Enable cancelling working threads from main
    if(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)){
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "pthread_setcancelstate() failed\n");
    }
    if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL)){
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "pthread_setcanceltype() failed\n");
    }

    if(FCGX_InitRequest(&request, fcgx_socket, 0) != 0) { 
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Can not init request\n"); 
        return NULL; 
    } 
 
    for(;;) 
    { 
        static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER; 

        // Accept new request
        pthread_mutex_lock(&accept_mutex); 
        rc = FCGX_Accept_r(&request); 
        pthread_mutex_unlock(&accept_mutex); 

        if(rc < 0) { 
            errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Error accepting new request (%d)\n", rc); 
            break; 
        }  

        httpserv_serve(&request);

        // Close current connection
        FCGX_Finish_r(&request);  

        pthread_mutex_lock(&app_data_mutex);
        if(app_data.nm_restart_needed){
            if(restart_network_manager()) app_data.nm_restart_needed = false;
        }

        if(app_data.reboot_needed){system_reboot();}

        if(app_data.poweroff_needed){system_poweroff();}

        pthread_mutex_unlock(&app_data_mutex);
    } 
 
    FCGX_Free(&request, fcgx_socket);
    return NULL; 
} 