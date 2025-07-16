#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>

int main(){

    /* Open the device */
    int fd = open("/dev/led_poll", O_RDWR);
    if(-1 == fd){
        printf("Open device failed!\n");
        return -1;
    }
    printf("Open the device success\n");

    struct pollfd pfd = {
        .fd = fd,
        .events = POLLIN
    };

    while(1){
        int ret = poll(&pfd, 1, -1);
        if(ret > 0 && (pfd.revents & POLLIN)){
            char buf[16];
            int n = read(fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                printf("Button pressed! Data from driver: %s", buf);
            }
        }
    }
    close(fd);
    return 0;
}
