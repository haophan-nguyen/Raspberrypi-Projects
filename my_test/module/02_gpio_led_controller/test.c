#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h> 

int main(){
    char buff[5];

    /* Open the device */
    int dev = open("/dev/gpio_Led", O_RDWR);
    if(dev == -1){
        printf("Open device failed!\n");
        return -1;
    }
    printf("Open the device success\n");

    while(1){
        /* Write data to device */
        write(dev, "1", 1); 
        read(dev, buff, sizeof(buff));
        printf("Read data '%s' from device\n", buff); 
        sleep(2);
        write(dev, "0", 1); 
        read(dev, buff, sizeof(buff));
        printf("Read data '%s' from device\n", buff); 
        sleep(2);
    }


    /* Read data from device */


    /* Close the device */
    close(dev);

    return 0;
}
