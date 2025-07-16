#ifndef __IOCTL_H__
#define __IOCTL_H__

#define MAGIC_NUM 0xF0
#define IOCTL_LED_ON            _IO(MAGIC_NUM, 0)         //_IO: không truyền data
#define IOCTL_LED_OFF           _IO(MAGIC_NUM, 1)         // _IO: không truyền data
#define IOCTL_LED_TOGGLE        _IO(MAGIC_NUM, 2)         // _IOW: user gửi dữ liệu cho kernel

typedef struct  blink{
    int on_time_ms;
    int off_time_ms;
    int time;
} blink;

#define IOCTL_LED_BLINK    _IOW(MAGIC_NUM, 3, blink)

#endif