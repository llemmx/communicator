#ifndef ASYNCOMM_H_
#define ASYNCOMM_H_

#include <mqueue.h>

#define ASY_OK 0 // 操作成果
#define ASY_ER_PARAM -1 // 参数传递错误，重新申请或传递
#define ASY_ER_EPOLL -2 // 申请EPOLL资源错误，重新申请或传递
#define ASY_ER_THREAD -3 // 申请线程时发生错误，重新申请或传递
#define ASY_ER_UNEPFILE -4 // 不支持EPoll的文件描述符，建议放弃注册
#define ASY_ER_UNKNOW -5 //未知错误，这个一般比较危险，建议abort

// 初始化异步通信线程
int asyncomm_init(mqd_t *value);

#endif
