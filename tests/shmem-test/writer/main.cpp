#include <iostream>
#include <thread>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include "../shm_protocol.h"

volatile bool quit = false;
int sh_fd;
struct shm_data *dst;

bool shm_init() {
    sh_fd = shm_open(SHMEM_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (sh_fd < 0) {
	return false;
    }
    ftruncate(sh_fd, sizeof(struct shm_data));
    dst = (struct shm_data *)
	mmap(nullptr, sizeof(struct shm_data), PROT_WRITE, MAP_SHARED, sh_fd, 0);
    return true;
}

void cleanup() {
    munmap(dst, sizeof(struct shm_data));
    close(sh_fd);
    shm_unlink(SHMEM_NAME);
}

void write_loop() {
    uint8_t a = 0;
    uint8_t b = 0;
    double x = 0.0;
    double y = 0.0;
    while (!quit) {
        x += 1.0;
        y += 2.0;
        a += 1;
        b += 2;
        std::cout << (int)a << ", \t" << (int)b << ", \t" << x << ", \t" << y << std::endl;
	dst->a = a;
	dst->b = b;
	dst->x = x;
	dst->y = y;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

int main() {
    shm_init();

    auto t = std::thread(write_loop);
    t.detach();

    std::cin.get();
    quit = true;

    cleanup();
    return 0;
}
