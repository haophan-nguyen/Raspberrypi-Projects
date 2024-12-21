obj-m += 01_read_write.o
C_TEST = test.c
CC = gcc
KDIR := /lib/modules/$(shell uname -r)/build

C_OBJ = $(C_TEST:.c=.o) 

all:
	make -C $(KDIR) M=$(PWD) modules
	$(CC) -c $(C_TEST) -o $(C_OBJ)
	$(CC) -o $(C_TEST:.c=) $(C_OBJ) 

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm $(C_TEST:.c=)

