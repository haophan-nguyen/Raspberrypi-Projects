#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#define DEVICE_PATH "/dev/my_misc"

char read_buf[1024];
char write_buf[1024];
volatile int count = 0;

void sig_handler(int signo, siginfo_t *info, void *context) {
    printf("Received signal %d from kernel. Data attached: %d\n", signo, info->si_int);
    count++;
}

int main() {
    int fd;

    // Register signal handler for SIGUSR1
    struct sigaction sa;
    sa.sa_sigaction = sig_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    // Open device file
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }
    printf("Device opened successfully\n");

    // 1. Send a normal string
    strcpy(write_buf, "Hello Phan Hao");
    write(fd, write_buf, strlen(write_buf));
    printf("Wrote string '%s' to device\n", write_buf);

    // 2. Read it back
    read(fd, read_buf, sizeof(read_buf));
    printf("Read string '%s' from device\n", read_buf);

    // 3. Send PID to kernel
    snprintf(write_buf, sizeof(write_buf), "%d", getpid());
    write(fd, write_buf, strlen(write_buf));
    printf("Sent PID %s to kernel\n", write_buf);

    // 4. Send 'trigger' to initiate signal from kernel
    strcpy(write_buf, "trigger");
    write(fd, write_buf, strlen(write_buf));
    printf("Sent 'trigger' to kernel\n");

    // 5. Wait for signal from kernel
    printf("Waiting for signal from kernel...\n");
    while(count < 1);
    printf("Received signal ok\n");
    close(fd);
    return 0;
}
