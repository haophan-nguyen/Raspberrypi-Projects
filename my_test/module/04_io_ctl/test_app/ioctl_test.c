#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../ioctl.h"

int main(){
    blink b;

    /* Open the device */
    int fd = open("/dev/led_blink", O_RDWR);
    if(-1 == fd){
        printf("Open device failed!\n");
        return -1;
    }
    printf("Open the device success\n");

    printf("led on\n");
    ioctl(fd, IOCTL_LED_ON);
    sleep(1);
    printf("led off\n");
    ioctl(fd, IOCTL_LED_OFF);
    sleep(1);
    printf("led toggle\n");
    ioctl(fd, IOCTL_LED_TOGGLE);
    sleep(1);
    printf("led toggle\n");
    ioctl(fd, IOCTL_LED_TOGGLE);

    b.off_time_ms = 1000;
    b.on_time_ms = 2000;
    b.time = 10;
    printf("led blink\n");
    ioctl(fd, IOCTL_LED_BLINK, &b);

    close(fd);
    return 0;
}
