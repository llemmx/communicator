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
#include <string.h>
#include <mqueue.h>

#include "glog4c.h"
#include "asyncomm.h"

#define PT_EXIT 0
#define PT_RUN  1

pthread_t m_thread;      // 通信线程句柄
struct epoll_event m_ev; // epoll handle for module
int m_ephandel; // epoll 句柄
volatile int m_pexit_flag = PT_RUN; // 线程退出标志，这里申请需要注意是非易挥发行变量

/******************************************************************************
* Description    : 异步通信数据获取函数.
* Input          : arg - 队列句柄，用于跨进程/线程传递通信数据
* Output         : None
* Return         : 返回值参考头文件定义
*------------------------------------------------------------------------------
* 2020-01-30     : 1.0.0 : llemmx
* Modification   :
******************************************************************************/
static void *asyncomm_get_msg(void *arg)
{
    mqd_t *m_queue2app = (mqd_t *)arg; // 队列句柄

    if (NULL == arg) {
        glog4c_err("pointer value is NULL.");
        glog4c_info("Beacuse parameter is NULL. So end of thread.");
        return NULL;
    }

    for (;m_pexit_flag != PT_EXIT;) {
        usleep(1000);
    }

    glog4c_info("Communication thread exited.\n");
}

/******************************************************************************
* Description    : 异步通信初始化函数.当前只考虑支持1个线程，后期跟进实际情况可以考虑支持多
* 个接收线程的支持
* Input          : value - 队列句柄，用于跨进程/线程传递通信数据
* Output         : None
* Return         : 返回值参考头文件定义
*------------------------------------------------------------------------------
* 2020-01-30     : 1.0.0 : llemmx
* Modification   :
******************************************************************************/
int asyncomm_init(mqd_t *value)
{
    int ret;
    //pthread_attr_t attr;

    if (NULL == value) {
        return ASY_ER_PARAM;
    }

    // 创建EPOLL
    m_ephandel = epoll_create(EPOLL_CLOEXEC); // 在多进程环境下，退出时会关闭对应的文件描述符
    if (m_ephandel < 0) {
        // 如果申请失败，这里要直接终止程序
        glog4c_err(strerror(errno));
        return ASY_ER_EPOLL;
    }

    // 如果将来需要扩展线程堆栈尺寸或调整参数就需要使用这个功能
    //ret = pthread_attr_init(&attr);
    ret = pthread_create(&m_thread, NULL, asyncomm_get_msg, (void *)value);
    if (ret != 0) {
        glog4c_err("Create pthread is error.\n");
        return ASY_ER_THREAD;
    }

    close(m_ephandel);

    return ASY_OK;
}

int asyncomm_register(int nfd)
{
    int ret = ASY_OK;

    if (nfd <= 0) {
        return ASY_ER_PARAM;
    }

    struct epoll_event ev;

    ev.events  =EPOLLIN | EPOLLET;
    ev.data.fd =nfd;
    // 注册可通行的文件句柄
    int epret = epoll_ctl(m_ephandel, EPOLL_CTL_ADD, nfd, &ev);
    if (EPERM == epret) {
        ret = ASY_ER_UNEPFILE;
        glog4c_hit("The target file fd does not support epoll.");
    } else if (EEXIST == epret) {
        ret = ASY_OK;
        glog4c_hit("Repeat registration");
    } else {
        ret = ASY_ER_UNKNOW;
        glog4c_err("Unknow error!");
    }

    return ret;
}

int asyncomm_remove(int ofd)
{
    struct epoll_event ev;

    int epret = epoll_ctl(m_ephandel, EPOLL_CTL_DEL, ofd, &ev);

    if (ENOENT == epret) {
        glog4c_hit("fd is not registered with this epoll instance.");
    }
    return ASY_OK;
}

int asyncomm_exit(void)
{
    m_pexit_flag = PT_EXIT;
}

int asyncomm_open_serial()
{
    return ASY_OK;
}

int asyncomm_open_tcpc()
{
    return ASY_OK;
}

int asyncomm_open_tcps()
{
    return ASY_OK;
}

