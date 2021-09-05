//
// Created by emb on 9/4/21.
//

#ifndef NORNS_SHMEM_HPP
#define NORNS_SHMEM_HPP

#include "shmem_protocol.h"
namespace crone {
class ShmemWriter {
    static int sh_fd;
    static struct norns_shmem_data *sink;
    static bool didInit;
  public:
    static bool init();
    static void cleanup();
    static struct norns_shmem_data *getSink() { return sink; }
};
} // namespace crone

#endif // NORNS_SHMEM_HPP
