#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>


/* Meta info */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phan Hao");
MODULE_DESCRIPTION("A character driver function to read write");

/*Buffer for data*/
static char buffer[255];
static size_t buff_p = 0;

/*Variable for driver and driver class*/
static dev_t device_nr;		// device number (major and minor)
static struct class *my_class;
static struct cdev my_device;

#define DRIVER_NAME "characterDriver"
#define DRIVER_CLASS "myClass"
#define DEBUG 1

/**
 * @brief This function is called when the device is opened
 */
static int driver_open(struct inode *device_file, struct file *instance){
	printk("The character driver is opened\n");
	return 0;
}

/**
 * @brief This function is called when the device is closed
 */
static int driver_close(struct inode *device_file, struct file *instance){
	printk("The character driver is closed\n");
	return 0;
}

/**
 * @brief This function is called when user want to read data
 * Read data to buffer
 */
static ssize_t driver_read(struct file *File, char *usr_buffer, size_t count, loff_t *offset){
	int amount, cp, del;

	/*Get amount of data to copy*/
	amount = min((size_t)count, buff_p);
	if (DEBUG){
		printk ("Amount of data to read: %d\n", amount);
	}

	/*Copy data to user*/
	cp = copy_to_user(usr_buffer, buffer, amount);
	if (DEBUG){
		printk ("usr_buffer: %s\n", usr_buffer);
	}

	/*Caculate data*/
	del = amount - cp;
	if (DEBUG){
		printk ("delta of read: %d\n", del);
	}

	return del;
}

/**
 * @brief This function is called when user want to write data
 * Write data to buffer
 */
static ssize_t driver_write(struct file *File, const char *usr_buffer, size_t count, loff_t *offet){
	int amount, cp, del;
	/*Get amount data to copy*/
	amount = min((int)count, (int)sizeof(buffer));

	/*Copy data from user*/
	cp = copy_from_user(buffer, usr_buffer, amount);
	buff_p = amount;

	/*Caculate data*/
	del = amount - cp;

	return del;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_close,
	.read = driver_read,
	.write = driver_write
};

/**
 * @brief This function is called when the driver is loaded into kernel
 */
static int __init ModuleInit(void){
	printk(KERN_INFO "Hello, this is character driver\n");

	/*Allocate a device number*/
	if(alloc_chrdev_region(&device_nr, 0, 1, DRIVER_NAME) < 0){
		printk("Could not be allocated the device number\n");
		return -1;
	}
	printk("Device %s was registered with Major: %d, Minor: %d\n", DRIVER_NAME, device_nr >> 20, device_nr & 0xfffff);

	/*Create device class*/
    if ((my_class = class_create(DRIVER_CLASS)) == NULL){
		printk("Device class can not be create!\n");
		goto classError;
	}

	/*create device file*/
	if(device_create(my_class, NULL, device_nr, NULL, DRIVER_NAME) == NULL){
		printk("Device file can not be create!\n");
		goto fileError;
	}

	/*Initialize device file*/
	cdev_init(&my_device, &fops);

	/*Register device to kernel*/
	if(cdev_add(&my_device, device_nr, 1) == -1){
		printk("Register device to kernel fail!\n");
		goto addError;
	}

	return 0;

classError:
	unregister_chrdev_region(device_nr, 1);
fileError:
	class_destroy(my_class);
addError:
	device_destroy(my_class, device_nr);
	return -1;
}

/**
 * @brief This function is called when the driver is removed from kernel
 */
static void __exit ModuleExit(void){
	printk(KERN_INFO "Good bye kernel!\n");
	cdev_del(&my_device);
	device_destroy(my_class, device_nr);
	class_destroy(my_class);
	unregister_chrdev_region(device_nr, 1);
}

module_init(ModuleInit);
module_exit(ModuleExit);
