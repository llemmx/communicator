/****************************************************************************** 
* Copyright (C)
* File name      :cmd_opt.c
* Author         :llemmx
* Date           :2011-05-03
* Description    :命令行参数解析。
* Interface      :无
* Others         :无
*******************************************************************************
* History:        初稿
*------------------------------------------------------------------------------
* 2018-07-14 : 1.0.0 : llemmx
* Modification   :
*------------------------------------------------------------------------------
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <getopt.h> //获取命令行参数
#include <unistd.h>
#include <errno.h>

// 使用libxml2库
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>

#include "cmd_opt.h"
#include "glog4c.h"
#include "db_in_mem.h"
#include "objects.h"

const char *m_help_str = " \
Usage: communicator -c [DIR]\n \
Usage: communicator --help\n\n \
    communicator are used to communicate with external devices. \n \
";

int cmdopt_parser_cmd(int argc, char **argv)
{
    //识别命令行输入参数，允许输入配置文件路径
    int option_index = 0, opt = 0;
    static struct option long_options[] = {
        {"config", required_argument, 0, 'c'},
        {"help",   no_argument,       0, 0},
        {0,0,0,0}
    };
    int ret = 0;
    while ((opt = getopt_long(argc, argv, "c:p:", long_options, &option_index)) != -1) {
        switch (opt) {
        case 0://长参数处理
            if (strcmp("help", long_options[option_index].name) == 0) {
                printf("%s", m_help_str);
            } else {
                glog4c_err("unknow param\n");
            }
            return CMDOPT_HELP;
        break;
        case 'c': // 设置配置文件路径
            ret = access(optarg, F_OK);
            if (ret != 0) {
                glog4c_err(strerror(errno));
                return CMDOPT_FAIL;
            }
            ret = dbmem_set_value(OBJSYS_ID, OBJSYS_CFG_FILE_PATH, DB_STRING, optarg, strlen(optarg));
            if (ret < 0) {
                // 如果错误就返回字符串
                glog4c_err(dbmem_get_err_str(ret));
                return CMDOPT_FAIL;
            }
            //glog4c_info("get config file path2=%s\n", optarg);
        break;
        default:
            //无法识别的参数，需要退出进程
            glog4c_err("Unknow parameters, input again!\n\n");
            printf("%s", m_help_str);
            return CMDOPT_FAIL;
        }
    }
    return CMDOPT_OK;
}

//加载和读取配置文件
#define XML_NODE     1 // 代表是xml的节点
#define XML_PROPERTY 0 // 代表是xml的属性
typedef struct {
    char *xmlpath; // xml属性路径
    char *name;    // 属性名称
    uint32_t type; // 对应属性的数据类型，引用自db_in_mem.h
    uint16_t node; // 
    uint16_t id;   // 对应系统属性的键
}xmlcontent;

xmlcontent xml_content[] = {
    {"/Communicator/System/AppToQueue", "",     DB_STRING, XML_NODE,     OBJSYS_CFG_A2Q},
    {"/Communicator/System/QeueuToApp", "",     DB_STRING, XML_NODE,     OBJSYS_CFG_Q2A},
    {"/Communicator/Serial[@Enable]", "Enable", DB_BOOL,   XML_PROPERTY, OBJSYS_SERIAL_EN},
    {"/Communicator/Serial/COM1",     "",       DB_STRING, XML_NODE,     OBJSYS_SERIAL1},
};

int cmdopt_parser_cfg(char *file)
{
    if (file == NULL) {
        return CMDOPT_FILE;
    }
    xmlDocPtr xml_doc;  // open xml file
    int ret = CMDOPT_OK;

    // 打开配置文件，字符格式必需是UTF-8
    xml_doc = xmlReadFile(file, "UTF-8", XML_PARSE_RECOVER);
    if (NULL == xml_doc) {
        glog4c_err("The config file open error for system.\n");
        glog4c_info("The File is %s\n", file);
        return CMDOPT_FILE;
    }

    // 使用xpath的方式进行解析
    xmlXPathContextPtr xml_contex;
    // 创建XPATH
    xml_contex = xmlXPathNewContext(xml_doc);
    if (NULL == xml_contex) {
        glog4c_err("init xpath is error\n");
        // 如果发生错误记得释放前面打开的文件
        xmlFreeDoc(xml_doc);
        return CMDOPT_FILE;
    }

    xmlXPathObjectPtr xml_retsult;// 存储xpath临时搜索结果，xml节点或属性数据
    xmlChar *str_value;
    int xmlsize = sizeof(xml_content) / sizeof(xmlcontent);
    int xml_int = 0;
    for (int tmp = 0; tmp < xmlsize; ++tmp) {
        xml_retsult = xmlXPathEvalExpression((const xmlChar*)xml_content[tmp].xmlpath, xml_contex);
        // 如果没有检索到就跳过当前的值
        if (NULL == xml_retsult) {
            glog4c_info("Can't read value.\n");
            xmlXPathFreeObject(xml_retsult);
            continue;
        }
        if (xmlXPathNodeSetIsEmpty(xml_retsult->nodesetval)) {
            glog4c_info("Can't read needs values from config file.\n")
            xmlXPathFreeObject(xml_retsult);
            ret = CMDOPT_PARSE;
            break;
        }

        if (XML_NODE == xml_content[tmp].node) {
            // 如果是节点则直接读取值
            str_value = xmlNodeGetContent(xml_retsult->nodesetval->nodeTab[0]);
            if (NULL == str_value) {
                xmlXPathFreeObject(xml_retsult);
                continue;
            }
            // 将数据转换为内部数据并存储到数据库中
            switch (xml_content[tmp].type) {
            case DB_STRING:
                dbmem_set_value(OBJSYS_ID, xml_content[tmp].id, xml_content[tmp].type, str_value, xmlStrlen(str_value));
            break;
            case DB_BOOL:
                xml_int = xmlStrEqual(str_value, (const xmlChar*)"Enable");
                dbmem_set_value(OBJSYS_ID, xml_content[tmp].id, xml_content[tmp].type, &xml_int, sizeof(xml_int));
            break;
            }
            xmlFree(str_value);
        } else {
            // 如果是属性则加载属性
            str_value = xmlGetProp(xml_retsult->nodesetval->nodeTab[0], (xmlChar*)xml_content[tmp].name);
            if (NULL == str_value) {
                xmlXPathFreeObject(xml_retsult);
                continue;
            }
            // 将数据转换为内部数据并存储到数据库中
            switch (xml_content[tmp].type) {
            case DB_BOOL:
                xml_int = xmlStrEqual(str_value, (const xmlChar*)"Enable");
                dbmem_set_value(OBJSYS_ID, xml_content[tmp].id, xml_content[tmp].type, &xml_int, sizeof(xml_int));
            break;
            }
            xmlFree(str_value);
        }
        xmlXPathFreeObject(xml_retsult);
    }
    glog4c_hit("printf obj info\n");
    dbmem_print_property(OBJSYS_ID);

    xmlFreeDoc(xml_doc); // 关闭文件
    xmlCleanupParser();  // 释放可能由parser分配的全局变量
    return ret;
    
}
