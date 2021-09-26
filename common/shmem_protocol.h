//
// Created by emb on 9/4/21.
//

#ifndef NORNS_SHMEM_PROTOCOL_H
#define NORNS_SHMEM_PROTOCOL_H

#define NORNS_SHMEM_NAME "/norns"

#ifdef __cplusplus
#include <cstdint>
#include <cfloat>
extern "C" {
#else
#include <stdint.h>
#include <float.h>

#endif

struct softcut_shmem_data {
    float phase;
    float commandsLoad;
    float processLoad;
};

struct levels_shmem_data {
    float inLevel[2];
    float outLevel[2];
};

struct norns_shmem_data {
    struct softcut_shmem_data softcut;
    struct levels_shmem_data levels;
};

#ifdef __cplusplus
}
#endif



#endif //NORNS_SHMEM_PROTOCOL_H
