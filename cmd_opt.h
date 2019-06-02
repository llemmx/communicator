#ifndef CMD_OPT_H_
#define CMD_OPT_H_

#define CMDOPT_OK    0  // 命令行解析成果
#define CMDOPT_HELP -1  // 打印帮助
#define CMDOPT_FAIL -2  // 参数解析有错误
#define CMDOPT_FILE -3  // 文件不存在，或者文件打开错误
#define CMDOPT_XPATH -4 // 解析XPATH出错
#define CMDOPT_PARSE -5 // 配置文件解析出错

//命令行解析
int cmdopt_parser_cmd(int argc, char **argv);
int cmdopt_parser_cfg(char *file);

#endif
