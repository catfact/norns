//
// Created by emb on 4/20/21.
//

#ifndef SHMEM_SHM_PROTOCOL_H
#define SHMEM_SHM_PROTOCOL_H

#include <stdint.h>

#include <float.h>

#define SHMEM_NAME "/shmemtest"

#ifdef __cplusplus
extern "C" {
#endif

struct shm_data {
    uint8_t a;
    uint8_t b;
    double x;
    double y;
};

#ifdef __cplusplus
}
#endif


#endif //SHMEM_SHM_PROTOCOL_H
