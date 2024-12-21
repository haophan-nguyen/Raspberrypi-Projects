#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h> 

int main(){
    char *data = "Hello Phan Hao";
    char buff[255];

    /* Open the device */
    int dev = open("/dev/characterDriver", O_RDWR);
    if(dev == -1){
        printf("Open device failed!\n");
        return -1;
    }
    printf("Open the device success\n");

    /* Write data to device */
    write(dev, data, strlen(data)); 
    printf("Write data '%s' to device\n", data);

    /* Read data from device */
    read(dev, buff, sizeof(buff));
    printf("Read data '%s' from device\n", buff); 

    /* Close the device */
    close(dev);

    return 0;
}
