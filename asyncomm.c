//------------------------------------------------------------------------------
// Protability:       gunc99.
// Design Pattern:    None.
// Base Classes:      None.
// MultiThread Safe:  None.
// Exception Safe:    No Creation, No process
// Library/package:   None.
// Source files:      asyncomm.c
// Related Document:  None.
// Organize:          
// Email:             llemmx@gmail.com
//------------------------------------------------------------------------------
// Release Note:
//     负责异步通信的模块，所有的网络，串口均在这个线程进行管理。
//------------------------------------------------------------------------------
// Version    Date          Author    Note
//------------------------------------------------------------------------------
// 1.0.0      2019-01-20    llemmx    -Original
//------------------------------------------------------------------------------

#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <net/if.h>
#include <unistd.h>
#include <aio.h>
#include <errno.h>
#include <stdlib.h>

#include "glog4c.h"
#include "asyncomm.h"

#define PT_EXIT 0
#define PT_RUN  1

pthread_t m_ephandle;      // 通信线程句柄
struct epoll_event m_ev; // epoll handle for module
volatile int m_pexit_flag = PT_RUN; // 线程退出标志

static void *asyncomm_get_msg(void *arg)
{
    mqd_t *m_queue2app == arg; // 队列句柄

    for (;m_pexit_flag != PT_EXIT;) {
        usleep(1000);
    }

    glog4c_info("Communication thread exited.\n");
}

int asyncomm_init(mqd_t *value)
{
    int ret;
    //pthread_attr_t attr;

    if (NULL == value) {
        return ASY_ER_PARAM;
    }

    // 创建EPOLL
    m_ephandle = epoll_create(EPOLL_CLOEXEC); // 在多进程环境下，退出时会关闭对应的文件描述符
    if (m_ephandle < 0) {
        glog4c_err(strerror(errno));
    }

    // 如果将来需要扩展线程堆栈尺寸或调整参数就需要使用这个功能
    //ret = pthread_attr_init(&attr);
    ret = pthread_create(&m_handle, NULL, asyncomm_get_msg, (void *)value);
    if (ret != 0) {
        glog4c_err("Create pthread is error.\n");
    }

    return ASY_OK;
}

int asyncomm_exit(void)
{
    m_pexit_flag = PT_EXIT;
}
