// sysfspoll: does a sysfs attribute wake poll() when its value changes (sysfs_notify)?
// usage: sysfspoll <path> <seconds>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc < 3) return 1;
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) { perror("open"); return 1; }
    char buf[32];
    pread(fd, buf, sizeof(buf) - 1, 0);  // arm: sysfs requires a read before poll
    time_t end = time(NULL) + atoi(argv[2]);
    int wakes = 0;
    while (time(NULL) < end) {
        struct pollfd p = {fd, POLLPRI | POLLERR, 0};
        int r = poll(&p, 1, 1000);
        if (r > 0) {
            ssize_t n = pread(fd, buf, sizeof(buf) - 1, 0);
            buf[n > 0 ? n : 0] = 0;
            printf("wake %d revents=0x%x value=%s", ++wakes, p.revents, buf);
            fflush(stdout);
        }
    }
    printf("done, wakes=%d\n", wakes);
    return 0;
}
