#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdint.h>  //uint8_t,uint16_t,uint32_t,uint64_t
#include <sys/types.h> //size_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * CRC32 hash algorithm.
 * cyclic Redundancy check
 * @param  data   [description]
 * @param  length [description]
 * @return        [description]
 */
uint32_t crc32(const void * data, size_t length);

#ifdef __cplusplus
}
#endif

#endif
