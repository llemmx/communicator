#ifndef ASYNCOMM_H_
#define ASYNCOMM_H_

#include <mqueue.h>

#define ASY_OK 0 // 操作成果
#define ASY_ER_PARAM -1 // 参数传递错误

// 初始化异步通信线程
int asyncomm_init(mqd_t *value);

#endif
