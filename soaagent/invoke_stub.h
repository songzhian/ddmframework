#ifndef __INVOKE_STUB_H__
#define __INVOKE_STUB_H__

#include <sys/types.h> //size_t
#include <inttypes.h>  //uint64_t

#ifdef __cplusplus
extern "C" {
#endif

// #define MAX_SERVICES_DECLARE_CONTENT_SIZE  1024*10

/**
 * [watch_declared_services description]
 * @param  config_file_path [description]
 * @return                  [description]
 */
int watch_declared_services(const char * config_file_path, int force_refresh);

/**
 * [show_resource_status description]
 * @return [description]
 */
int show_resource_status();

/**
 * [refresh_declared_services description]
 * @return [description]
 */
int refresh_declared_services();

/**
 * [invoke_stub_init description]
 * @return [description]
 */
int invoke_stub_init(const char * zk_addrs, const char * config_file_path);

/**
 * [invoke_stub_destroy description]
 * @return [description]
 */
int invoke_stub_destroy();

/**
 * [invoke_service description]
 * @param  group       [description]
 * @param  service     [description]
 * @param  version     [description]
 * @param  req_body    [description]
 * @param  res_buf     [description]
 * @param  res_buf_len [description]
 * @return             0 means success, -1 means failed
 */
int invoke_service(const char * group, const char * service, const char * version, const void * req_body, size_t req_len, void ** res_buf, size_t * res_buf_len);



#ifdef __cplusplus
}
#endif

#endif