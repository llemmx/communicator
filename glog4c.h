#ifndef GLOG4C_H_
#define GLOG4C_H_

#include <syslog.h>

//安gun标准extern inline特性与宏定义类似不单独汇编，在代码中展开
//采用inline特性的主要原因是允许编译器编译时会对代码调用进行检查而不是直接替换
/*extern inline void glog4c_init(void) {
    openlog("communicator", LOG_PERROR, LOG_LOCAL7);
}

extern inline void glog4c_close(void) {
    closelog();
}

extern inline void glog4c_err(const char *arg) {
    syslog(LOG_ERR, "%s:%d::%s", __FILE__, __LINE__, arg);
}*/

#define glog4c_init() {openlog("communicator", LOG_PERROR, LOG_LOCAL7);}
#define glog4c_close() {closelog();}
#define glog4c_err(arg) {syslog(LOG_ERR, "%s:%d::%s", __FILE__, __LINE__, arg);}

#if G_DEBUG
    #define glog4c_info(format, arg...) { printf("%s:%d::",__FILE__,__LINE__);printf(format, ## arg); }
    #define glog4c_hit(arg) { printf("%s:%d::%s", __FILE__, __LINE__, arg); }
#else
    #define glog4c_info(format, arg...)
    #define glog4c_hit(arg)
#endif

#endif
