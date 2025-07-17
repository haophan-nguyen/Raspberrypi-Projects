#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>

/* Test app for 06_kthread */
int main(){

    /* Open the device */
    int fd = open("/dev/my_btn", O_RDWR);
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
            printf("Button pressed\n");
        }
    }
    close(fd);
    return 0;
}
