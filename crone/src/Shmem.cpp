//
// Created by emb on 9/5/21.
//

#include <cassert>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "Shmem.hpp"

int crone::ShmemWriter::sh_fd = -1;
struct norns_shmem_data *crone::ShmemWriter::sink = nullptr;
bool crone::ShmemWriter::didInit = false;

bool  crone::ShmemWriter::init() {
    if (didInit) {
        return true;
    }
    sh_fd = shm_open(NORNS_SHMEM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (sh_fd < 0) {
        didInit = false;
        return false;
    }
    long sz = sizeof(struct norns_shmem_data);
    assert(ftruncate(sh_fd, sz) == 0);
    sink = (struct norns_shmem_data *)
            mmap(nullptr, sz, PROT_WRITE, MAP_SHARED, sh_fd, 0);
    didInit = true;
    return true;
}

void crone::ShmemWriter::cleanup() {
    munmap(sink, sizeof(struct norns_shmem_data));
    close(sh_fd);
    sh_fd = -1;
    shm_unlink(NORNS_SHMEM_NAME);
    sink = nullptr;
    didInit = false;
}