#ifndef __NGX_IP2REGION_H__
#define __NGX_IP2REGION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "ip2region.h"

/**
* 存储IP库查询方式
*/
#define NGX_IP2REGION_MEMORY 0
#define NGX_IP2REGION_BINARY 1
#define NGX_IP2REGION_BTREE 2

/**
 * 存储 nginx 配置参数 结构体 变量
 */
typedef struct ngx_http_ip2region_conf_s {
    ngx_flag_t  enable;//on=1,off=0
    ngx_str_t   db_file;
    ngx_uint_t  algo;
} ngx_http_ip2region_conf_t;

typedef uint_t (*search_func_ptr)(ip2region_t, uint_t, datablock_t);

extern ngx_int_t create_ip2region(ngx_http_ip2region_conf_t *conf, ngx_log_t *log);
extern ngx_int_t destroy_ip2region(ngx_log_t *log);
extern ngx_int_t ngx_ip2region_search(ngx_str_t *addr_text, char *region);

#ifdef __cplusplus
}
#endif

#endif
