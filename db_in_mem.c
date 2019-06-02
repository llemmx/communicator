//-----------------------------------------------------------------------------
// Copyright (C)
// File name      :db_in_mem.c
// Author         :llemmx
// Date           :2018-07-15
// Description    :内存数据库，按对象形式组织内存。数据存储格式采用建值对(key-value)的方式
//                 进行存储。存储前所有数据会进行希尔排序，让所有的数据按顺序存储，方便后续高
//                 效搜索。
// Interface      :无
// Others         :无
//-----------------------------------------------------------------------------
// History:        初稿
//-----------------------------------------------------------------------------
// 2018-07-14 : 1.0.0 : llemmx
// Modification   :
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "glog4c.h"
#include "db_in_mem.h"

#define DBMEM_MAX_OBJS      8
#define DBMEM_OBJ_NAME_SIZE 20

//定义的最大对象数量，目前是32，对应4个字节
uint8_t m_objid[DBMEM_MAX_OBJS >> 3] = {0};

//定义了系统级对象
typedef struct {
    uint16_t obj_id;                    // 对象编号
    char     name[DBMEM_OBJ_NAME_SIZE]; // 对象名称
    uint16_t psize;                     // 测点数量
    dbvar    property[];                // 对象属性
}objsys;

//定义系统对象
objsys *m_objsys[DBMEM_MAX_OBJS] = {NULL};

// 对象编号查询
int dbmem_get_id(uint16_t id)
{
    return (m_objid[id >> 3] >> (id & 7)) & 1;
}
// 对象编号设置
void dbmem_set_id(uint16_t id)
{
    m_objid[id >> 3] |= 1 << (id & 7);
}
// 删除对象编号
void dbmem_clear_id(uint16_t id)
{
    m_objid[id >> 3] &= ~(1 << (id & 7));
}

// 希尔排序
void dbmem_insert_sort(uint16_t arr[], int size, int span, int grp)
{
    int tmp = 0;
    // 这里为了减少对空间的使用，直接采用索引分组的计算方式
    for (int idx = span + grp; idx < size; idx += span){
        // 判断指定元素左邻接(间隔snap个元素)数的大小，如果成立则持续交换数据并持续向前判断
        if (arr[idx] < arr[idx - span]){
            tmp = arr[idx];
            // 要从右向左开始顺序判断每个元素大小，如果小则交换，大则不改变，知道循环到最小索引
            int subid = 0;// 受到作用域的影响，变量需要定义在这里
            for (subid = idx - span; (subid >= 0 && arr[subid] > tmp); subid -= span){
                arr[subid + span] = arr[subid];
            }
            //将前面准备进行交换计算的数据交换放置到最终位置
            arr[subid + span] = tmp;
        }
    }
}

//------------------------------------------------------------------------------
// Function       :shell_sort
// Author         :llemmx    
// Date           :2017-07-31
// Description    :整形希尔排序，经过测试在PC上性能大约是16384万个随机数约7us
//                 可以参考链接http://www.cnblogs.com/skywang12345/p/3596881.html
// Input          :arr:待排序数组
//                :num:数组数量
// Output         :无
// Return         :无
//------------------------------------------------------------------------------
// Modification History: 
// 2017-07-31 (llemmx): 创建
//------------------------------------------------------------------------------
void dbmem_shell_sort(uint16_t arr[], int num)
{
    // 初始化为数组长度的一半，后续每次循环为上一次的一半，这里采用整除提高效率
    for (int span = num >> 1; span > 0; span >>= 1){
        // 按跨度长度进行分组，对分好的组进行插入排序
        for (int grp = 0; grp < span; ++grp){
            dbmem_insert_sort(arr, num, span, grp);
        }
    }
}

//------------------------------------------------------------------------------
// Function       :dbmem_binary_search
// Author         :llemmx    
// Date           :2017-07-31
// Description    :折半查找，由于数组较小所以折半查找效率很高
// Input          :obj_id:对象编号
//                :id:属性测点编号
// Output         :无
// Return         :无
//------------------------------------------------------------------------------
// Modification History: 
// 2018-07-31 (llemmx): 创建
//------------------------------------------------------------------------------
dbvar *dbmem_binary_search(uint16_t obj_id, uint16_t id)
{
    // 内部函数，已经确保参数正确
    objsys *obj_tmp = m_objsys[obj_id];
    uint16_t head = 0, end = obj_tmp->psize, idx=0;
    
    while (head < end) {
        idx = (head + end) >> 1;
        if (id > obj_tmp->property[idx].id) {
            head = idx;
        } else if (id < obj_tmp->property[idx].id) {
            end  = idx;
        } else {
            return &obj_tmp->property[idx];
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------
// Function       :dbmem_create_obj
// Author         :llemmx    
// Date           :2017-07-31
// Description    :创建设备对象
// Input          :obj_id:对象编号
//                :name:对象名称
//                :size:对象属性的个数，注意不是字节数
// Output         :无
// Return         :成功返回OBJSYS_RET_OK,失败按头文件中的定义返回
//------------------------------------------------------------------------------
// Modification History: 
// 2017-07-31 (llemmx): 创建
//------------------------------------------------------------------------------
int dbmem_create_obj(uint16_t obj_id, const char *name, int size)
{
    if (NULL == name || obj_id > DBMEM_MAX_OBJS) {
        glog4c_err("Error param name\n");
        return OBJSYS_RET_PARAM;
    }
    //创建对象
    if (dbmem_get_id(obj_id) == 0) {
        // 在位表中注册对象
        dbmem_set_id(obj_id);
        // 根据属性数量分配空间，一般来说分配后不会随便改变数量
        objsys *obtmp;
        obtmp = (objsys*)malloc(sizeof(objsys) + (sizeof(dbvar) * size));
        if (NULL == obtmp){
            glog4c_err(strerror(errno));
            dbmem_clear_id(obj_id);
            return OBJSYS_RET_FMEM;
        }
        obtmp->obj_id = obj_id;
        obtmp->psize  = size;
        strncpy(obtmp->name, name, DBMEM_OBJ_NAME_SIZE);
        m_objsys[obj_id] = obtmp;
    }else{
        return OBJSYS_RET_IDUSED;
    }
    return OBJSYS_RET_OK;
}

// 初始化对象属性值，在创建对象后就要立刻初始化
int dbmem_init_values(uint16_t obj_id, uint16_t *var, uint16_t size)
{
    if (NULL == var || size == 0 || obj_id > DBMEM_MAX_OBJS) {
        glog4c_err("Error param var\n");
        return OBJSYS_RET_PARAM;
    }
    if (0 == dbmem_get_id(obj_id)){
        return OBJSYS_RET_UNKNOWOBJ;
    }
    //索引排序
    //测点顺序初始化，一旦初始化完毕后不可再变动
    uint16_t  len = sizeof(uint16_t) * size;
    uint16_t *tmp = (uint16_t*)malloc(len);
    memcpy(tmp, var, len);
    // 对测点进行排序，为后面的快速查询做准备
    dbmem_shell_sort(tmp, size);
    // 初始化每个属性的编号
    objsys *obj_tmp = m_objsys[obj_id];
    // 检查对象属性与设置的属性数量是否一致，如果不一致则仅按最大值初始化，记录警告
    int max_num = size > obj_tmp->psize ? obj_tmp->psize : size;
    for (int id = 0; id < max_num; ++id) {
        obj_tmp->property[id].id = tmp[id];
        obj_tmp->property[id].u64 = 0; // 初始化为0，避免随机数
    }

    free(tmp);
    return OBJSYS_RET_OK;
}

//保存单条对象数据
int dbmem_set_value(uint16_t obj_id, uint16_t var_id, int type, void *value, uint32_t size)
{
    int result = OBJSYS_RET_OK;
    
    // 参数检查
    if (NULL == value) {
        glog4c_info("Paramater is error!\n");
        result = OBJSYS_RET_PARAM;
        goto EXIT_SV;
    }

    // 目前只存储一个配置文件路径
    if (dbmem_get_id(obj_id) == 1) {
        // 取出对应的对象
        dbvar *var_tmp = dbmem_binary_search(obj_id, var_id);
        if (var_tmp == NULL){
            glog4c_info("We can't find id form obj_id=%d\n", obj_id);
            result = OBJSYS_RET_UNKNOWOBJ;
            goto EXIT_SV;
        }
        char *str    = NULL;
        //uint8_t *buf = NULL;
        //int size     = 0;

        //如果设置的数据类型不一致，并且是字符串等需要重新释放
        if (DB_STRING == var_tmp->type) {
            free(var_tmp->str);
        } else if (DB_BLOB == var_tmp->type) {
            free(var_tmp->blob);
        }
        if (type != var_tmp->type) {
            var_tmp->type = type;
        }

        // 将数据转换为对应变量
        switch (type){
        case DB_INT8:
            var_tmp->i8  = (*(int8_t*)value) & 0xFF;
            var_tmp->len = sizeof(int8_t);
        break;
        case DB_UINT8:
            var_tmp->u8  = (*(uint8_t*)value) & 0xFF;
            var_tmp->len = sizeof(uint8_t);
        break;
        case DB_INT16:
            var_tmp->i16  = (*(int16_t*)value) & 0xFFFF;
            var_tmp->len  = sizeof(int16_t);
        break;
        case DB_UINT16:
            var_tmp->u16  = (*(uint16_t*)value) & 0xFFFF;
            var_tmp->len  = sizeof(uint16_t);
        break;
        case DB_INT32:
            var_tmp->i32  = (*(int32_t*)value) & 0xFFFFFFFF;
            var_tmp->len  = sizeof(int32_t);
        break;
        case DB_UINT32:
            var_tmp->u32 = (*(uint32_t*)value) & 0xFFFFFFFF;
            var_tmp->len = sizeof(uint32_t);
        break;
        case DB_INT64:
            var_tmp->i64  = (*(int64_t*)value);
            var_tmp->len  = sizeof(int64_t);
        break;
        case DB_UINT64:
            var_tmp->u64 = (*(uint64_t*)value);
            var_tmp->len = sizeof(uint64_t);
        break;
        case DB_FLOAT:
            var_tmp->f  = (*(float*)value);
            var_tmp->len = sizeof(float);
        break;
        case DB_DOUBLE:
            var_tmp->d = (*(double*)value);
            var_tmp->len = sizeof(double);
        break;
        case DB_STRING: // 保存字符串格式，单位B
            // 取出字符串
            str = (char*)value;
            /*if (NULL == str) {
                result = OBJSYS_RET_PARAM;
                goto EXIT_SV;
            }*/
            // 为测点分配空间
            var_tmp->str = (char*)malloc(size + 1);
            if (NULL == var_tmp->str){
                glog4c_err(strerror(errno));
                result = OBJSYS_RET_FMEM;
                goto EXIT_SV;
            }
            // 拷贝字符串, 这里必需用安全字符串拷贝，否则发生过缓冲溢出的问题
            strncpy(var_tmp->str, str, size);
            var_tmp->str[size] = '\0';
            var_tmp->len = size;
        break;
        case DB_BLOB: // 保存二进制数据，单位B;这里取值时需要注意还有大小参数
            {
                // 取出数组
                /*uint8_t *buf = (uint8_t*)value;
                if (NULL == buf) {
                    result = OBJSYS_RET_PARAM;
                    goto EXIT_SV;
                }*/
                // 为测点分配空间
                var_tmp->blob = (uint8_t*)malloc(size);
                if (NULL == var_tmp->blob){
                    glog4c_err(strerror(errno));
                    result = OBJSYS_RET_FMEM;
                    goto EXIT_SV;
                }
                memcpy(var_tmp->blob, (uint8_t*)value, size);
                var_tmp->len = size;
            }
        break;
        case DB_BOOL:
            var_tmp->bl   = (*(int32_t*)value) & 0xFFFFFFFF;
            var_tmp->len  = sizeof(int32_t);
        break;
        default:
            var_tmp->len = 0;
        }
    }
EXIT_SV:
    return result;
}

//读取单条对象数据指针
dbvar *dbmem_get_value(uint16_t obj_id, uint16_t var_id)
{
    dbvar *var_tmp = NULL;
    // 如果这个对象存在
    if (dbmem_get_id(obj_id) == 1) {
        // 取出对应的对象
        var_tmp = dbmem_binary_search(obj_id, var_id);
        if (var_tmp == NULL){
            glog4c_info("We can't find id form obj_id=%d\n", obj_id);
        }
    }
    return var_tmp;
}

//消除内存结构
int dbmem_close(void)
{
    // 枚举所有对象的所有变量
    for (int idx=0; idx < DBMEM_MAX_OBJS; ++idx){
        if (m_objsys[idx] != NULL){
            for (int imp = 0; imp < m_objsys[idx]->psize; ++imp) {
                int con = 0;
                con  = (DB_STRING == m_objsys[idx]->property[imp].type);
                con |= (DB_BLOB == m_objsys[idx]->property[imp].type);
                con &= (NULL != m_objsys[idx]->property[imp].str);
                if (con) {
                    free(m_objsys[idx]->property[imp].str);
                }
            }
            free(m_objsys[idx]);
        }
    }
    glog4c_hit("close memory db\n")
    return OBJSYS_RET_OK;
}

static char *errstr[] = {
    "Nothing",                 // Nothing-0
    "Current transmit parameters useage is error.", // OBJSYS_RET_PARAM-1
    "The object id was used.", // OBJSYS_RET_IDUSED-2
    "Out of memory.",          // OBJSYS_RET_FMEM-3
    "Unknow object id.",       // OBJSYS_RET_UNKNOWOBJ-4
    "Unknow property id."      // OBJSYS_RET_UNKNOWID-5
    "Error data type."         // OBJSYS_RET_TYPE-6
};

char *dbmem_get_err_str(int err)
{
    if (err > 0 || err < OBJSYS_RET_MAX) {
        return errstr[0];
    }
    int idx = -1 * err;

    return errstr[idx];
}

void dbmem_print_property(uint16_t obj_id)
{
    objsys *cur;
    // 如果这个对象存在
    if (dbmem_get_id(obj_id) == 1) {
        cur = m_objsys[obj_id];

        glog4c_info("obj id = %d\n", cur->obj_id);
        glog4c_info("obj name = %s\n", cur->name);
        for (int idx = 0; idx < cur->psize; ++idx) {
            switch (cur->property[idx].type) {
            case DB_NULL:
                glog4c_info("id=%d::value = NULL\n", cur->property[idx].id);
            break;
            case DB_INT8:
                glog4c_info("id=%d::value = %d\n", cur->property[idx].id, cur->property[idx].i8);
            break;
            case DB_INT16:
                glog4c_info("id=%d::value = %d\n", cur->property[idx].id, cur->property[idx].i16);
            break;
            case DB_INT32:
                glog4c_info("id=%d::value = %d\n", cur->property[idx].id, cur->property[idx].i32);
            break;
            case DB_INT64:
                glog4c_info("id=%d::value = %li\n", cur->property[idx].id, cur->property[idx].i64);
            break;
            case DB_UINT8:
                glog4c_info("id=%d::value = %d\n", cur->property[idx].id, cur->property[idx].u8);
            break;
            case DB_UINT16:
                glog4c_info("id=%d::value = %d\n", cur->property[idx].id, cur->property[idx].u16);
            break;
            case DB_UINT32:
                glog4c_info("id=%d::value = %d\n", cur->property[idx].id, cur->property[idx].u32);
            break;
            case DB_UINT64:
                glog4c_info("id=%d::value = %ld\n", cur->property[idx].id, cur->property[idx].u64);
            break;
            case DB_FLOAT:
                glog4c_info("id=%d::value = %f\n", cur->property[idx].id, cur->property[idx].f);
            break;
            case DB_DOUBLE:
                glog4c_info("id=%d::value = %f\n", cur->property[idx].id, cur->property[idx].d);
            break;
            case DB_STRING:
                glog4c_info("id=%d::value = %s\n", cur->property[idx].id, cur->property[idx].str);
            break;
            case DB_BLOB:
                glog4c_info("id=%d::value size = %d\n", cur->property[idx].id, cur->property[idx].len);
            break;
            case DB_BOOL:
                glog4c_info("id=%d::value = %d\n", cur->property[idx].id, cur->property[idx].bl);
            break;
            }
        }
    }
}
