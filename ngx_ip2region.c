#include <ngx_config.h>
#include <ngx_core.h>

#include "ip2region.h"
#include "ngx_ip2region.h"

static ngx_inline u_char *
ngx_strrchr(u_char *first, u_char *p, u_char c);

static ngx_inline uint32_t
ngx_inet_aton(u_char *data, ngx_uint_t len);

static ip2region_entry g_ip2region_entry;

search_func_ptr ip2region_search_func;

/**
 * 查询函数
 * @param addr_text
 * @param region
 * @param city
 * @return
 */
ngx_int_t
ngx_ip2region_search(ngx_str_t *addr_text, char *region)
{
    if(g_ip2region_entry.dbHandler == NULL) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "search error ip %V, ip2region doesn't create", addr_text);
        return NGX_ERROR;
    }

    uint32_t addr_binary = ngx_inet_aton(addr_text->data, addr_text->len);

    if(addr_binary == (uint32_t)NGX_ERROR) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "search error ip %V: bad ip address", addr_text);
        return NGX_ERROR;
    }

    datablock_entry datablock;
    ngx_memzero(&datablock, sizeof(datablock));
    uint_t rc = ip2region_search_func(&g_ip2region_entry, (uint_t)addr_binary, &datablock);

    if(!rc) {
        ngx_log_error(NGX_LOG_ERR, ngx_cycle->log, 0, "%V search failed", addr_text);
        return NGX_ERROR;
    }

    sprintf(region, "%d", datablock.city_id);
    strcat(region, "|");
    strcat(region, datablock.region);

    return NGX_OK;
}


/**
 * 初始ip库
 */
ngx_int_t create_ip2region(ngx_http_ip2region_conf_t *conf, ngx_log_t *log) {

    uint_t result = ip2region_create(&g_ip2region_entry, (char *) conf->db_file.data);
    if (!result) {
        if (log) {
            ngx_log_error(NGX_LOG_ERR, log, 0, "ip2region create failed: %V", &conf->db_file);
        }
        return NGX_ERROR;
    }
    switch (conf->algo) {
        default:
        case NGX_IP2REGION_MEMORY:
            ip2region_search_func = ip2region_memory_search;
            if (log) {
                ngx_log_error(NGX_LOG_INFO, log, 0, "ip2region create succcess: algo=memory");
            }
            break;

        case NGX_IP2REGION_BINARY:
            ip2region_search_func = ip2region_binary_search;
            if (log) {
                ngx_log_error(NGX_LOG_INFO, log, 0, "ip2region create succcess: algo=binary");
            }
            break;

        case NGX_IP2REGION_BTREE:
            ip2region_search_func = ip2region_btree_search;
            if (log) {
                ngx_log_error(NGX_LOG_INFO, log, 0, "ip2region create succcess: algo=btree");
            }
            break;
    }
//    if (log) {
//        ngx_log_error(NGX_LOG_INFO, log, 0, "ip2region create succcess: algo=%V", algo);
//    }
    return NGX_OK;
}

/**
 * 销毁ip库
 */
ngx_int_t destroy_ip2region(ngx_log_t *log)
{
    ip2region_destroy(&g_ip2region_entry);
    if (log) {
        ngx_log_error(NGX_LOG_INFO, log, 0, "ip2region destroy");
    }
    return NGX_OK;
}

static ngx_inline u_char *
ngx_strrchr(u_char *first, u_char *p, u_char c)
{
    while(p >= first) {
        if(*p == c) {
            return p;
        }

        p--;
    }

    return NULL;
}

static ngx_inline uint32_t
ngx_inet_aton(u_char *data, ngx_uint_t len)
{
    long long  result = 0;
    long long  temp = 0;
    u_char      *last = data + len;
    int        point = 0;

    while(data <= last) {
        if(*data == '.' || data == last) {
            if(temp > 255) {
                return NGX_ERROR;
            }

            result = (result << 8) + temp;
            temp = 0;
            ++point;

            if(point == 4) {
                break;
            }

        } else if(*data <= '9' && *data >= '0') {
            temp = temp * 10 + (*data - '0');

        } else {
            return NGX_ERROR;
        }

        ++data;
    }

    if(point != 4) {
        return NGX_ERROR;
    }

    return result;
}

