#ifndef _SERVICE_MGT_H_
#define _SERVICE_MGT_H_

#include "zk_handle.h"
#include "cJSON.h"

#include <sys/types.h>
#include <inttypes.h>  //uint64_t

#ifdef __cplusplus
extern "C" {
#endif


/**
 * [provider_stub_init description]
 * @return [description]
 */
int provider_stub_init(const char * zk_addrs, const char * config_file_path);

/**
 * [provider_stub_destroy description]
 * @return [description]
 */
int provider_stub_destroy();

/**
 * register services
 * @param  services_define_file_path [file path of services define]
 * @param  zh                        [zk handle]
 * @return                           [return 0 means success , or failed]
 */
int register_services(zk_handle * zh);

/**
 * [get_cur_node_path description]
 * @return [description]
 */
const char * get_cur_node_path();

#ifdef __cplusplus
}
#endif

#endif
