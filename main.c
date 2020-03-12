/****************************************************************************** 
* Copyright (C)
* File name      :main.c
* Author         :llemmx
* Date           :2011-05-03
* Description    :通讯进程入口文件
* Interface      :无
* Others         :无
*******************************************************************************
* History:        初稿
*------------------------------------------------------------------------------
* 2018-07-14 : 1.0.0 : llemmx
* Modification   :
*------------------------------------------------------------------------------
******************************************************************************/
//需要用到的系统头文件
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <signal.h>

// 自定义的头文件
#include "cmd_opt.h"
#include "glog4c.h"
#include "db_in_mem.h"
#include "objects.h"
#include "db_in_mem.h"
#include "asyncomm.h"

// 测点类型初始化
const uint16_t init_var[]={OBJSYS_CFG_FILE_PATH, DB_STRING};

// 定义模块变量
mqd_t m_app2queue, m_queue2app;
volatile sig_atomic_t m_exit_flag = 0;

// CTRL+C信号量捕获
void ctrl_c(int sig)
{
    glog4c_hit("user break process!\n");
    m_exit_flag = 1;
}

// 参考文章《SQlite数据库的C编程接口》
int main(int argc, char **argv)
{
    int ret_v = 0;//用于捕获函数返回值

    //初始化日志系统
    glog4c_init();
    //初始化内存数据库，建立系统对象
    ret_v = dbmem_create_obj(OBJSYS_ID, OBJSYS_NAME, OBJSYS_MAXID);
    if (ret_v < 0){
        // 如果出错则退出
        glog4c_err(dbmem_get_err_str(ret_v));
        exit(EXIT_FAILURE);
    }
    // 测点初始化列表
    uint16_t id_list[] = {OBJSYS_ID_LIST};
    // 批量初始化对象属性，为了让所有系统测点有一个默认值
    ret_v = dbmem_init_values(OBJSYS_ID, id_list, OBJSYS_MAXID);
    if (ret_v < 0){
        // 如果出错则退出
        glog4c_err(dbmem_get_err_str(ret_v));
        exit(EXIT_FAILURE);
    }
    
    // 解析命令行参数，解析配置文件
    ret_v = cmdopt_parser_cmd(argc, argv);
    if (CMDOPT_FAIL == ret_v) {
        glog4c_err("Parameter parsing is fail, input again!\n");
        exit(EXIT_FAILURE);
    }else if (CMDOPT_HELP == ret_v){
        exit(EXIT_SUCCESS);
    }
    
    // 读取文件参数
    dbvar *conf = dbmem_get_value(DBOBJ_SYSTEM, OBJSYS_CFG_FILE_PATH);
    if (NULL == conf) {
        glog4c_err("Config file path is error!\n");
        exit(EXIT_FAILURE);
    }

    // 解析配置文件
    ret_v = cmdopt_parser_cfg(conf->str);
    if (ret_v < 0) {
        glog4c_err("Parse config file is error!\n");
        // 后续这里遇到错误应该进入默认参数的安全模式
        exit(EXIT_FAILURE);
    }
    glog4c_info("%s", "Read config is completed!\n");

    // 根据配置文件内容创建各种通讯服务
    // 创建应用到通讯者的服务
    dbvar *a2q_name = dbmem_get_value(OBJSYS_ID, OBJSYS_CFG_A2Q);
    dbvar *q2a_name = dbmem_get_value(OBJSYS_ID, OBJSYS_CFG_Q2A);
    if (NULL == a2q_name || NULL == q2a_name) {
        glog4c_err("Can't get queue name!\n");
        exit(EXIT_FAILURE);
    }
    // 打开从应用到通讯者的队列
    m_app2queue = mq_open(a2q_name->str, O_RDONLY | O_CREAT | O_NONBLOCK, 0666, NULL);

    if (m_app2queue < 0){
        glog4c_err("open app2queue queue error:");
        glog4c_err(strerror(errno));
        exit(EXIT_FAILURE);
    }
    // 打开从通讯者到应用的队列，该队列句柄需要传递到通讯线程内部
    m_queue2app = mq_open(q2a_name->str, O_RDWR | O_CREAT | O_NONBLOCK, 0666, NULL);
    if (m_queue2app < 0){
        if (errno == EEXIST) {
            m_queue2app = mq_open(q2a_name->str, O_RDWR | O_NONBLOCK, 0666, NULL);
        }
    }
    if (m_queue2app < 0) {
        glog4c_err("open queue2app queue error:");
        glog4c_err(strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Register signals
    signal(SIGINT, ctrl_c); 

    // 读取当前队列属性
    struct mq_attr a2q_attr;

    if (mq_getattr(m_app2queue, &a2q_attr) == -1) {
        glog4c_err("get mqueue attr error:");
        glog4c_err(strerror(errno));
        mq_close(m_app2queue);
        mq_close(m_queue2app);
        exit(EXIT_FAILURE);
    }
    // 按队列缓冲尺寸申请内测
    char *buf = (char*)malloc(a2q_attr.mq_msgsize);

    // 创建异步通信线程
    ret_v = asyncomm_init(&m_queue2app);
    if (ret_v < 0) {
        // 通信线程初始化失败，终止程序
        exit(EXIT_FAILURE);
    }

    // 循环读取应用进程发来的数据
    ssize_t qsize;
    for (;m_exit_flag != 1;) {
        qsize = mq_receive(m_app2queue, buf, a2q_attr.mq_msgsize, NULL);
        if (qsize < 0){
            if (errno == EAGAIN){ // 资源未准备好，异步通信时容易发生此类事件
                usleep(10);
                continue;
            }
            glog4c_err("receive message failed:");
            glog4c_err(strerror(errno));
            continue;
        }

        // 解析对应的协议，格式简单处理. 命令2B ｜ 数量2B ｜ 类型1B ｜ 数据
        glog4c_info("get buf size = %ld\n", qsize);
    }

    mq_close(m_app2queue);
    mq_close(m_queue2app);
    closelog();
    dbmem_close();
    mq_unlink(a2q_name->str);
    mq_unlink(q2a_name->str);
    exit(EXIT_SUCCESS);
}

