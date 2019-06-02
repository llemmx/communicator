#ifndef OBJECTS_H_
#define OBJECTS_H_

// 系统对象定义,后续可以采用配置文件的形式进行初始化
// 内置对象编号
#define DBOBJ_SYSTEM 1
// 以下是对象定义区域
#define OBJSYS_ID            0x0001 // 系统对象
#define OBJSYS_NAME          "communicator"
// 对象属性
#define OBJSYS_CFG_FILE_PATH 0x0001 // 配置文件路径
#define OBJSYS_CFG_A2Q       0x0002 // posix message with App to Communicator
#define OBJSYS_CFG_Q2A       0x0003 // posix message with Communicator to App
#define OBJSYS_SERIAL_EN     0x0004 // 串口是否生效
#define OBJSYS_SERIAL1       0x0005 // 串口1路径
#define OBJSYS_MAXID         0x0005 // 最大测点数量
// 属性列表
#define OBJSYS_ID_LIST       OBJSYS_CFG_FILE_PATH, OBJSYS_CFG_A2Q, OBJSYS_CFG_Q2A, OBJSYS_SERIAL_EN, OBJSYS_SERIAL1

#endif
