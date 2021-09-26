#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "../../../common/shmem_protocol.h"

#define CAP_SIZE 8192
#define CAP_DT_US 1000

float cap_buf[CAP_SIZE][2];
int cap_idx = 0;

volatile bool quit = false;
int sh_fd;
const struct norns_shmem_data *src;

int shm_init() {
    sh_fd = shm_open(NORNS_SHMEM_NAME, O_RDONLY, 0600);
    if (sh_fd < 0) {
        return 1;
    }
    ftruncate(sh_fd, sizeof(struct norns_shmem_data));
    src = (const struct norns_shmem_data *)
            mmap(NULL, sizeof(struct norns_shmem_data), PROT_READ, MAP_SHARED, sh_fd, 0);
    return 0;
}


void cleanup() {
    munmap((void *) src, sizeof(struct norns_shmem_data));
    close(sh_fd);
    shm_unlink(NORNS_SHMEM_NAME);
}

void *read_loop(void *p) {
    (void) p;
    while (!quit) {
        cap_buf[cap_idx][0] = src->softcut.commandsLoad;
        cap_buf[cap_idx][1] = src->softcut.processLoad;
        cap_idx++;
        if (cap_idx >= CAP_SIZE) {
            quit = true;
        }
        usleep(CAP_DT_US);
    }
    return NULL;
}

int main() {

    shm_init();

    pthread_t tid;
    pthread_attr_t tatt;
    pthread_attr_init(&tatt);
    pthread_create(&tid, &tatt, &read_loop, NULL);
    pthread_join(tid, NULL);

    printf("# cmd\tproc\t\n");
    for (int i=0; i<CAP_SIZE; ++i) {
        printf("%f\t%f\t\n", cap_buf[i][0], cap_buf[i][1]);
    }
    printf("\n\n");

//    getchar();
//    quit = true;

    cleanup();
    return 0;
}

