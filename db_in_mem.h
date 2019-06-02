#ifndef DBINMEM_H_
#define DBINMEM_H_

#include <stdint.h>

//需要注意的是，所有处于db memory管理的数据都需要通过db memory库进行释放

//数据类型
#define DB_NULL   0
#define DB_INT8   1
#define DB_UINT8  2
#define DB_INT16  3
#define DB_UINT16 4
#define DB_INT32  5
#define DB_UINT32 6
#define DB_INT64  7
#define DB_UINT64 8
#define DB_FLOAT  9
#define DB_DOUBLE 10
#define DB_STRING 11
#define DB_BLOB   12
#define DB_BOOL   13

typedef struct {
    struct {
        uint32_t id  :12; //数据编号
        uint32_t type:4;  //数据类型
        //数据长度，如果是固定长度则为变量的字长，如果是字符串则表示数据总长为xxB
        uint32_t len :16;
    };
    union {
        int8_t   i8;
        uint8_t  u8;
        int16_t  i16;
        uint16_t u16;
        int32_t  i32;
        uint32_t u32;
        int64_t  i64;
        uint64_t u64;
        float    f;
        double   d;
        char     *str;
        uint8_t  *blob;
        int32_t  bl;
    };
}dbvar;

//函数反回结果定义
#define OBJSYS_RET_OK         0
#define OBJSYS_RET_PARAM     -1 // 参数错误
#define OBJSYS_RET_IDUSED    -2 // id已经被使用
#define OBJSYS_RET_FMEM      -3 // 内存不足
#define OBJSYS_RET_UNKNOWOBJ -4 // 未知对象
#define OBJSYS_RET_UNKNOWID  -5 // 未知测点
#define OBJSYS_RET_TYPE      -6 // 数据类型错误
#define OBJSYS_RET_MAX       -6 // 最末端ID号，用于计算

// 使用顺序是创建对象->初始化属性->设置默认值,最后是用完毕后要消除内存结构
// 创建对象,内部对象创建参考objects.h,外部对象参考相应的配置文件
int dbmem_create_obj(uint16_t obj_id, const char *name, int size);
// 初始化对象属性
int dbmem_init_values(uint16_t obj_id, uint16_t *var, uint16_t size);
// 保存单条对象数据
int dbmem_set_value(uint16_t obj_id, uint16_t var_id, int type, void *value, uint32_t size);
// 读取单条对象数据
dbvar *dbmem_get_value(uint16_t obj_id, uint16_t var_id);
// 打印对象属性
void dbmem_print_property(uint16_t obj_id);
// 格式化错误消息
char *dbmem_get_err_str(int err);
// 消除内存结构
int dbmem_close(void);

#endif
