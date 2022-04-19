#include <csignal>
#include <thread>
#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <stdexcept>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcgi_config.h>
#include <fcgiapp.h>
#include <fcgios.h>

#define LOG_MODULE_NAME   "MAIN"
#include "loger.h"

#define NUM_THREAD    4 
       
static std::atomic<int> stop_main;
static std::mutex stop_threads_mutex;
static std::condition_variable stop_threads;
static int fcgi_socket = -1;

static std::mutex request_mutex;


static void worker_thread(int tid)
{
    Log::msg(MSG_DEBUG, "thread: %d starts\n", tid);

    int ret = 0;
    FCGX_Request request;
    if(FCGX_InitRequest(&request, fcgi_socket, FCGI_FAIL_ACCEPT_ON_INTR) != 0) { 
        log_err("FCGX_InitRequest() failed\n"); 
        return; 
    }

    for(;;)
    {
        request_mutex.lock();
        ret = FCGX_Accept_r(&request);
        request_mutex.unlock();

        Log::msg(MSG_DEBUG | MSG_TO_FILE, "thread: %d test message\n", tid);
        if (ret < 0){
            log_err("FCGX_Accept_r() failed\n");
            break;
        }
        

        FCGX_Finish_r(&request);

        

        std::unique_lock<std::mutex> lock(stop_threads_mutex);
        if(stop_threads.wait_for(lock, std::chrono::milliseconds(5250)) == std::cv_status::no_timeout){
            Log::msg(MSG_DEBUG, "thread: %d interrupted\n", tid);
            break;
        } 
    }
}


static inline void signal_handler_init(std::initializer_list<int> signals)
{
    stop_main.store(0);

    auto signal_handler = [](int sig_num){ stop_main.store(sig_num); };

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signal_handler;        
    sigemptyset(&act.sa_mask);     

    for (const auto sig : signals){
        sigaddset(&act.sa_mask, sig);
        sigaction(sig, &act, nullptr);
    }                                        
}

static inline int fcgi_init(const std::string& socket_path, const int backlog)
{
    FCGX_Init(); 
    int socket_fd = FCGX_OpenSocket(socket_path.c_str(), backlog);
    if(socket_fd < 0) throw std::runtime_error("fcgi socket opening failed");

    return socket_fd;
}

static inline void fcgi_deinit(const int& socket_fd)
{
    FCGX_ShutdownPending();
    if(socket_fd >= 0) {
        shutdown(socket_fd, SHUT_RD);
        OS_IpcClose(socket_fd);
    }
    OS_LibShutdown();
}

int main(int argc, char* argv[]) 
{ 
    try{
        Log::init(MSG_VERBOSE, "Log.log", 1000);

        signal_handler_init({SIGINT, SIGQUIT, SIGTERM, SIGHUP});

        fcgi_socket = fcgi_init("./tests/test.sock", 20);

        std::thread t[NUM_THREAD];
        for(int i = 0; i < NUM_THREAD; ++i){
            t[i] = std::thread(worker_thread, i);
        }

        while(!stop_main.load()){

            sleep(10); // Сигналы прерывают сон
        }

        Log::msg(MSG_DEBUG | MSG_TO_FILE, "catched signal: %d\n", stop_main.load());

        fcgi_deinit(fcgi_socket);

        stop_threads.notify_all();

        for(int i = 0; i < NUM_THREAD; ++i){
            t[i].join();
        }

        Log::msg(MSG_DEBUG, "all threads stopped. exiting\n");

        return 0;      
    }
    catch(const std::runtime_error& e){
        log_err("%s\n", e.what());
        return 1;
    }
    catch(...){
        log_err("Unknown exception occured\n");
        return 2;
    }
}



#if 0
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
#endif