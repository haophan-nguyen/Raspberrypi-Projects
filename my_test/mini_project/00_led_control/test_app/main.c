#include <stdio.h>
#include <unistd.h>

int main() {
    FILE *fd = fopen("/dev/led_control", "w");
    if (fd == NULL) {
        perror("Failed to open /dev/led_control");
        return 1;
    }

    for (int i = 0; i < 10; i++) {
        fputc('1', fd);     // Bật đèn
        fflush(fd);         // 💡 Đảm bảo dữ liệu được ghi ngay
        sleep(1);

        fputc('0', fd);     // Tắt đèn
        fflush(fd);         // 💡 Đảm bảo dữ liệu được ghi ngay
        sleep(1);
    }

    fclose(fd);
    return 0;
}