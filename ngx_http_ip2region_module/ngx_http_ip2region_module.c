#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


#include "ngx_ip2region.h"

/**
 * 存储变量数组下标，同时是region分割下标：city_id|中国|0|广东|广州|电信
 */
#define IP2REGION_CITY_ID 0
#define IP2REGION_COUNTRY_NAME 1
#define IP2REGION_REGION_CODE 2
#define IP2REGION_PROVINCE_NAME 3
#define IP2REGION_CITY_NAME 4
#define IP2REGION_ISP_DOMAIN 5
#define IP2REGION_REGION_NAME 6





static ngx_int_t ngx_http_ip2region_add_variable(ngx_conf_t *cf);


/**
 * 函数声明 公用函数
 */
static ngx_int_t
ngx_http_variable_get_handler_index(ngx_http_request_t *r, ngx_http_variable_value_t *v, int index);
static ngx_int_t
ngx_http_variable_get_handler_str(ngx_http_request_t *r, ngx_http_variable_value_t *v, char *data);
static ngx_str_t *
ngx_http_ip2region_get_addr(ngx_http_request_t *r);

static char *cutSplit(char *data, char delim, int k);

/**
 * 添加变量
 */
static ngx_int_t ngx_http_variable_ip2region_city_id(ngx_http_request_t *r,
                                                     ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_ip2region_country_name(ngx_http_request_t *r,
                                                          ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_ip2region_region_code(ngx_http_request_t *r,
                                                         ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_ip2region_province_name(ngx_http_request_t *r,
                                                           ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_ip2region_city_name(ngx_http_request_t *r,
                                                       ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_ip2region_isp_domain(ngx_http_request_t *r,
                                                        ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_variable_ip2region_region_name(ngx_http_request_t *r,
                                                         ngx_http_variable_value_t *v, uintptr_t data);
/**
 * 初始化 ngx_http_ip2region_conf_t
 */
static void*
ngx_http_ip2region_create_main_conf(ngx_conf_t *cf);
static void
ngx_http_ip2region_exit_process(ngx_cycle_t *ngx_cycle);
/**
 * 读取nginx配置
 */
static char *
ngx_http_ip2region_set_conf_ip2region(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static char *
ngx_http_ip2region_set_conf_algo(ngx_conf_t *cf, ngx_http_ip2region_conf_t *ip2region_conf, ngx_str_t value);

static ngx_conf_enum_t ngx_ip2region_search_algo[] = {
        {ngx_string("memory"), NGX_IP2REGION_MEMORY},
        {ngx_string("binary"), NGX_IP2REGION_BINARY},
        {ngx_string("btree"),  NGX_IP2REGION_BTREE},
        {ngx_null_string, NGX_IP2REGION_MEMORY}//默认
};


/**
 * 需要添加的变量
 * 数组
 */
static ngx_http_variable_t ngx_http_ip2region_variables[] = {
        {
                ngx_string("ip2region_city_id"),
                           NULL, ngx_http_variable_ip2region_city_id,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        {
                ngx_string("ip2region_country_name"),
                           NULL, ngx_http_variable_ip2region_country_name,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        {
                ngx_string("ip2region_region_code"),
                           NULL, ngx_http_variable_ip2region_region_code,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        {
                ngx_string("ip2region_province_name"),
                           NULL, ngx_http_variable_ip2region_province_name,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        {
                ngx_string("ip2region_city_name"),
                           NULL, ngx_http_variable_ip2region_city_name,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        {
                ngx_string("ip2region_isp_domain"),
                           NULL, ngx_http_variable_ip2region_isp_domain,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        {
                ngx_string("ip2region_region_name"),
                           NULL, ngx_http_variable_ip2region_region_name,
                NGX_HTTP_VAR_CHANGEABLE, 0, 0
        },
        { ngx_null_string, NULL, NULL, 0, 0, 0 }
};



static char *
ngx_http_ip2region_set_conf_ip2region(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_ip2region_conf_t	*ip2region_conf = conf;
    ngx_str_t        *value;

    if (ip2region_conf->db_file.len != 0) {
        return "db_file already set!";
    }

    value = cf->args->elts;

    if (value[1].len == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "No ip2region database file specified.");
        return NGX_CONF_ERROR;
    }

    ip2region_conf->db_file = value[1];

    if (ngx_conf_full_name(cf->cycle, &ip2region_conf->db_file, 0) != NGX_OK) {//不是文件绝对路径则报错
        return NGX_CONF_ERROR;
    }

    ngx_log_debug1(NGX_LOG_INFO, ngx_cycle->log, 0, "%V", &ip2region_conf->db_file);
    if (value[2].len != 0) {
        char *algo = ngx_http_ip2region_set_conf_algo(cf, ip2region_conf, value[2]);
        if ( algo != NGX_CONF_OK) {
            return algo;
        }
//        ip2region_conf->algo = NGX_IP2REGION_MEMORY;//默认
    }

    if (create_ip2region(ip2region_conf, cf->log) != NGX_OK) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "Unable to open database file \"%s\".", value[1].data);
        return NGX_CONF_ERROR;
    }

    ngx_http_ip2region_add_variable(cf);

    return NGX_CONF_OK;
}

static char *
ngx_http_ip2region_set_conf_algo(ngx_conf_t *cf, ngx_http_ip2region_conf_t *ip2region_conf, ngx_str_t value) {

    ngx_conf_enum_t *e = ngx_ip2region_search_algo;
    ngx_uint_t i;

    for (i = 0; e[i].name.len != 0; i++) {
        if (e[i].name.len != value.len
            || ngx_strcasecmp(e[i].name.data, value.data) != 0) {
            continue;
        }

        ip2region_conf->algo = e[i].value;
        return NGX_CONF_OK;
    }

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                       "invalid value \"%s\"", value.data);
    return NGX_CONF_ERROR;
}





/**
 * 读取 nginx 配置
 */
static ngx_command_t ngx_http_ip2region_commands[] = {
        {
                ngx_string("ip2region"),
                NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE12,
                ngx_http_ip2region_set_conf_ip2region,
                NGX_HTTP_MAIN_CONF_OFFSET, 0, NULL
        },
        ngx_null_command
};






/**
 * 调用添加变量函数入口，创建配置入口，初始化配置函数入口
 */
static ngx_http_module_t ngx_http_ip2region_module_ctx = {
        NULL,       /* preconfiguration */
        NULL,                                  /* postconfiguration */

        ngx_http_ip2region_create_main_conf,   /* create main configuration */
        NULL,     /* init main configuration */

        NULL,                                  /* create server configuration */
        NULL,                                  /* merge server configuration */
        NULL,                                  /* create location configuration */
        NULL                                   /* merge location configuration */
};
ngx_module_t  ngx_http_ip2region_module = {
        NGX_MODULE_V1,
        &ngx_http_ip2region_module_ctx,       /* module context */
        ngx_http_ip2region_commands,          /* module directives */
        NGX_HTTP_MODULE,                      /* module type */
        NULL,                                 /* init master */
        NULL,                                 /* init module */
        NULL,      /* init process */
        NULL,                                 /* init thread */
        NULL,                                 /* exit thread */
        ngx_http_ip2region_exit_process,      /* exit process */
        NULL,                                 /* exit master */
        NGX_MODULE_V1_PADDING
};
static void*
ngx_http_ip2region_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_ip2region_conf_t  *ip2region_conf;
    ip2region_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_ip2region_conf_t));

    if(ip2region_conf == NULL) {
        return NULL;
    }
    ip2region_conf->db_file.len = 0;
    ip2region_conf->algo = NGX_IP2REGION_MEMORY;
    return ip2region_conf;
}
static void
ngx_http_ip2region_exit_process(ngx_cycle_t *ngx_cycle)
{
    destroy_ip2region(&ngx_cycle->new_log);//todo 这里如果无法打印，传NULL吧
}


/**
 * 截取字符串的第几个，从0开始
 */
static char *cutSplit(char *data, char delim, int k) {
    if (data == NULL) {
        return NULL;
    }
    int n = 0;
    char *p = data;
    while (p[0] != '\0') {
        if (p[0] == delim) {
            p[0] = '\0';
            n++;
            if (n <= k) {
                data = p + 1;
            } else {
                break;
            }
        }
        p++;
    }
    if (n < k) {
        data = p;//找不到，返回空字符串
    }
    return data;
}

/**
 * 统一返回str值
 */


static ngx_int_t
ngx_http_variable_get_handler_index(ngx_http_request_t *r, ngx_http_variable_value_t *v, int index) {

    ngx_http_variable_value_t *region_name_value =
            ngx_http_get_flushed_variable(r, ngx_http_ip2region_variables[IP2REGION_REGION_NAME].index);

    char *data = ngx_pnalloc(r->pool, region_name_value->len);
    ngx_memcpy(data, (char *) region_name_value->data, region_name_value->len + 1);

    data = cutSplit((char *) data, '|', index);

    return ngx_http_variable_get_handler_str(r, v, data);
}

static ngx_int_t
ngx_http_variable_get_handler_str(ngx_http_request_t *r, ngx_http_variable_value_t *v, char *data) {
    if (data == NULL) {
        return NGX_ERROR;
    }

    size_t len;
    len = ngx_strlen(data) + 1;
    v->data = ngx_pnalloc(r->pool, len);

    if (v->data == NULL) {
        return NGX_ERROR;
    }
    ngx_memcpy(v->data, data, len);

    v->len = len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


/**
 * 从request获取ip地址
 */
static ngx_str_t *
ngx_http_ip2region_get_addr(ngx_http_request_t *r) {

    ngx_addr_t addr;
    size_t size;
    u_char *p = ngx_pnalloc(r->pool, 16);//size = NGX_INET_ADDRSTRLEN + 1
    addr.sockaddr = r->connection->sockaddr;
    addr.socklen = r->connection->socklen;

#if defined(nginx_version) && (nginx_version) >= 1005003
    size = ngx_sock_ntop(addr.sockaddr, addr.socklen, p, NGX_INET6_ADDRSTRLEN, 0);
#else
    size = ngx_sock_ntop(addr.sockaddr, p, NGX_INET6_ADDRSTRLEN, 0);
#endif

    p[size] = '\0';

    ngx_str_t *addr_str = ngx_pnalloc(r->pool, sizeof(ngx_str_t));
    addr_str->len = strlen((char *) p);
    addr_str->data = ngx_pnalloc(r->pool, addr_str->len);
    ngx_memcpy(addr_str->data, p, addr_str->len);
//    ngx_free
    return addr_str;
}


/**
 * 添加变量 gethandler方法
 */
static ngx_int_t
ngx_http_variable_ip2region_city_id(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_get_handler_index(r, v, IP2REGION_CITY_ID);
}


static ngx_int_t
ngx_http_variable_ip2region_country_name(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_get_handler_index(r, v, IP2REGION_COUNTRY_NAME);
}

static ngx_int_t
ngx_http_variable_ip2region_region_code(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_get_handler_index(r, v, IP2REGION_REGION_CODE);
}

static ngx_int_t
ngx_http_variable_ip2region_province_name(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_get_handler_index(r, v, IP2REGION_PROVINCE_NAME);
}

static ngx_int_t
ngx_http_variable_ip2region_city_name(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_get_handler_index(r, v, IP2REGION_CITY_NAME);
}

static ngx_int_t
ngx_http_variable_ip2region_isp_domain(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_variable_get_handler_index(r, v, IP2REGION_ISP_DOMAIN);
}

static ngx_int_t
ngx_http_variable_ip2region_region_name(ngx_http_request_t *r, ngx_http_variable_value_t *v, uintptr_t data) {

    ngx_str_t *addr_text = ngx_http_ip2region_get_addr(r);//查地址

    char region[80];
    ngx_ip2region_search(addr_text, region);//查ip2region

    return ngx_http_variable_get_handler_str(r, v, region);
}



/**
 * 遍历ngx_http_ip2region_vars数组，逐个添加变量
 * 变量的值取决于get_handler 方法
 */
static ngx_int_t
ngx_http_ip2region_add_variable(ngx_conf_t *cf)
{
    //todo 这里应该控制是否启用ip2region

    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_ip2region_variables; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;

        /**
         * 直接索引起来，方便后续获取
         */
        ngx_int_t index = ngx_http_get_variable_index(cf, &v->name);
        if (index == NGX_ERROR) {
            return NGX_ERROR;
        }
        v->index = index;//todo ngx_int_t赋值给ngx_uint_t，可能报错
    }

    return NGX_OK;
}
