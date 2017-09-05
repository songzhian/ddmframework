#include "service_conf.h"
#include "utarray.h"
#include "cJSON.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static UT_icd register_info_icd = {sizeof(register_info), NULL, NULL, NULL };

//all filters
static UT_array * service_filters;

//key : node key , value : list of register_info struct
static register_map_item * register_mapping;


int node_resource_construct_key(const char * host, int port, char * node_key, size_t len) {
    snprintf(node_key, len, "%s%s%d", host, KEY_SEPARATOR, port);
    return 0;
}

int service_resource_construct_key(const char * group, const char * service, const char * version, char * service_key, size_t len) {
    snprintf(service_key, len, "%s%s%s%s%s", group, KEY_SEPARATOR, service, KEY_SEPARATOR, version);
    return 0;
}

int provider_construct_key(const char * group, char * provider_key, size_t len) {
    snprintf(provider_key, len, "/%s/providers", group);
    return 0;
}

int service_filter_init() {
    UT_icd service_info_icd = {sizeof(service_info), NULL, NULL, NULL };
    utarray_new(service_filters, &service_info_icd);
    return 0;
}

int service_filter_destroy() {
    utarray_free(service_filters);
    return 0;
}

int service_filter_padding(service_info * srv_info, const char * group, const char * service, const char * version) {
    snprintf(srv_info->group_value, FIELD_GROUP_LEN, "%s", group);
    snprintf(srv_info->service_value, FIELD_SERVICE_LEN, "%s", service);
    snprintf(srv_info->version_value, FIELD_VERSION_LEN, "%s", version);
    return 0;
}

int service_filter_add(service_info * srv_info) {
    if (service_filter_find(srv_info)) {
        return 0;
    }
    INFO("service_filter_add : %s, %s, %s", srv_info->group_value, srv_info->service_value, srv_info->version_value);
    utarray_push_back(service_filters, srv_info);
    return 0;
}

/**
 * [service_filter_find description]
 * @param  srv_info [description]
 * @return          1 means found, 0 means not found
 */
int service_filter_find(service_info * srv_info) {
    service_info * p = NULL;
    while ( (p = (service_info*)utarray_next(service_filters, p))) {
        if ( strcmp(p->group_value, srv_info->group_value) == 0
                && strcmp(p->service_value, srv_info->service_value) == 0
                && strcmp(p->version_value, srv_info->version_value) == 0 ) {
            return 1;
        }
    }
    return 0;
}

int register_info_padding(register_info * reg_info, const char * group, const char * service, const char * version, const char * host, int port) {
    service_filter_padding(&reg_info->srv_info, group, service, version);
    snprintf(reg_info->host_value, FIELD_HOST_LEN, "%s", host);
    reg_info->port_value = port;
    return 0;
}

/**
 * [service_filter_match description]
 * @param  reg_info [description]
 * @return          1 means match, 0 means not match
 */
int service_filter_match(register_info * reg_info) {
    return service_filter_find(&reg_info->srv_info);
}

/**
 * [parse_node_register_info description]
 * @param  result  if NULL, return new array
 * @param  content [description]
 * @return         [description]
 */
int parse_node_register_info(register_info_array ** result, const char * content) {
    cJSON *root = cJSON_Parse(content);
    if (!root) {
        return -1;
    }
    if (*result == NULL) {
        *result = malloc(sizeof(register_info_array));
        UT_icd register_info_icd = {sizeof(register_info), NULL, NULL, NULL };
        utarray_new((*result)->array, &register_info_icd);
    }
    ////
    char * host_value = cJSON_GetObjectItem(root, "host")->valuestring;
    int port_value =  cJSON_GetObjectItem(root, "port")->valueint;
    char * group_value = cJSON_GetObjectItem(root, "group")->valuestring;
    cJSON * services_array = cJSON_GetObjectItem(root, "services");
    int srv_len = cJSON_GetArraySize(services_array);
    int j;
    for (j = 0; j < srv_len; ++j) {
        cJSON * service_item = cJSON_GetArrayItem(services_array, j);
        char * service_name = cJSON_GetObjectItem(service_item, "service")->valuestring;
        char * version_value = cJSON_GetObjectItem(service_item, "version")->valuestring;
        register_info reg_info;
        reg_info.status = 0;
        register_info_padding(&reg_info, group_value, service_name, version_value, host_value, port_value);
        if (service_filter_match(&reg_info)) {
            if (!find_register_info(*result, &reg_info)) {
                utarray_push_back((*result)->array, &reg_info);
            }
        }
    }
    cJSON_Delete(root);
    return 0;
}

/**
 * [find_register_info description]
 * @param  result   [description]
 * @param  reg_info [description]
 * @return          1 means found, 0 means not found
 */
int find_register_info(register_info_array * result, register_info * reg_info) {
    register_info * p = NULL;
    while ( (p = (register_info*)utarray_next(result->array, p))) {
        if ( strcmp(p->srv_info.group_value, reg_info->srv_info.group_value) == 0
                && strcmp(p->srv_info.service_value, reg_info->srv_info.service_value) == 0
                && strcmp(p->srv_info.version_value, reg_info->srv_info.version_value) == 0
                && strcmp(p->host_value, reg_info->host_value) == 0
                && p->port_value == reg_info->port_value ) {
            return 1;
        }
    }
    return 0;
}

int copy_register_info(register_info_array * src, register_info_array * dest, int status) {
    register_info * p = NULL;
    while ( (p = (register_info*)utarray_next(src->array, p))) {
        utarray_push_back(dest->array, p);
        register_info * tmp = utarray_back(dest->array);
        tmp->status = status;
    }
    return 0;
}

int destroy_register_info_array(register_info_array * info_array) {
    if (!info_array) {
        return 0;
    }
    utarray_free(info_array->array);
    free(info_array);
    return 0;
}

/**
 * diff between left array and right array
 * @param  left_array  [description]
 * @param  right_array [description]
 * @return             [description]
 */
register_info_array * diff_register_info(register_info_array * left_array, register_info_array * right_array) {
    if (!left_array && !right_array) {
        return NULL;
    }
    register_info_array * result = malloc(sizeof(register_info_array));
    UT_icd register_info_icd = {sizeof(register_info), NULL, NULL, NULL };
    utarray_new(result->array, &register_info_icd);
    register_info_array * temp;
    int status = 0;
    if (left_array && !right_array) {
        status = -1;
        temp = left_array;
        copy_register_info(temp, result, status);
    } else if (!left_array && right_array) {
        status = 1;
        temp = right_array;
        copy_register_info(temp, result, status);
    } else {
        //find deleted ones
        register_info * p = NULL;
        while ( (p = (register_info*)utarray_next(left_array->array, p))) {
            if (!find_register_info(right_array, p)) {
                utarray_push_back(result->array, p);
                register_info * tmp = utarray_back(result->array);
                tmp->status = -1;
            }
        }
        //find added one
        p = NULL;
        while ( (p = (register_info*)utarray_next(right_array->array, p))) {
            if (!find_register_info(left_array, p)) {
                utarray_push_back(result->array, p);
                register_info * tmp = utarray_back(result->array);
                tmp->status = 1;
            }
        }
    }
    return result;
}

int register_mapping_init() {
    register_mapping = NULL;
    return 0;
}

int register_mapping_destroy() {
    register_map_item *current, *tmp;
    HASH_ITER(hh, register_mapping, current, tmp) {
        HASH_DEL(register_mapping, current);
        destroy_register_info_array(current->info_array);
        free(current);
    }
    return 0;
}

register_map_item * register_mapping_find_item(const char * provider_key) {
    register_map_item * item = NULL;
    HASH_FIND_STR(register_mapping, provider_key, item);
    return item;
}

register_info_array * register_mapping_find(const char * provider_key) {
    register_map_item * item = register_mapping_find_item(provider_key);
    if (item) {
        return item->info_array;
    }
    return NULL;
}

int register_mapping_put(const char * provider_key, register_info_array * info_array) {
    register_map_item * item = register_mapping_find_item(provider_key);
    if (item) {
        item->info_array = info_array;
        return 0;
    }
    item = malloc(sizeof(register_map_item));
    snprintf(item->provider_key, PROVIDER_KEY_LEN, "%s", provider_key);
    item->info_array = info_array;
    HASH_ADD_STR(register_mapping, provider_key, item);
    return 0;
}
