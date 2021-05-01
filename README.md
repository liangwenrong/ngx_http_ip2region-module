# ngx_http_ip2region-module
魔改版

```
  ip2region "/opt/nginx3/ip2region.db";
 
  ip2region "/opt/nginx3/ip2region.db" "btree"; #memory（默认）、binary
```

```
  log_format  main escape=none  '$ip2region_region_name - $ip2region_city_id - $ip2region_country_name - $ip2region_region_code - $ip2region_province_name - $ip2region_city_name - $ip2region_isp_domain - $remote_addr - $remote_user [$time_local] "$request" '
                      '$status $body_bytes_sent "$http_referer" '
                      '"$http_user_agent" "$http_x_forwarded_for"';
```
