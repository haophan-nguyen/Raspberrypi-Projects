#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>

/* Meta info */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phan Hao");
MODULE_DESCRIPTION("A character driver function to read write");


/*Variable for driver and driver class*/
static dev_t device_nr;		// device number (major and minor)
static struct class *my_class;
static struct cdev my_device;

#define DRIVER_NAME "gpio_Led"
#define DRIVER_CLASS "myClass"
#define DEBUG 0
#define GPIO4 516

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
	char tmp[3];

	/* Get amount of data to copy */
	amount = min(count, sizeof(tmp));

	/* Read value of led */
	if(DEBUG)
		printk("This is read func Value of led: %d\n", gpio_get_value(GPIO4));

	tmp[0] = gpio_get_value(GPIO4) + '0';

	/* Copy data to user */
	cp = copy_to_user(usr_buffer, &tmp, amount);

	/* Calculate data */
	del = amount - cp;

	return del;
}

/**
 * @brief This function is called when user want to write data
 * Write data to buffer
 */
static ssize_t driver_write(struct file *File, const char *usr_buffer, size_t count, loff_t *offet){
	int amount, cp, del;
	char value;
	/*Get amount data to copy*/
	amount = min((int)count, (int)sizeof(value));

	/*Copy data from user*/
	cp = copy_from_user(&value, usr_buffer, amount);
	
	/* Setting the LED */
	switch(value) {
		case '0':
			gpio_set_value(GPIO4, 0);
			break;
		case '1':
			gpio_set_value(GPIO4, 1);
			break;
		default:
			printk("Invalid Input!\n");
			break;
	}
	if(DEBUG)
		printk("This is write func Value of led: %d\n", gpio_get_value(GPIO4));

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

    /* GPIO 4 init */
	if(gpio_request(GPIO4, "rpi-gpio-4")) {
		printk("Gpio led - Can not allocate GPIO 4\n");
		goto addError;
	}
	printk("Gpio led - Allocate GPIO 4 success\n");

	/*Set DPIO 4 direction*/
	if(gpio_direction_output(GPIO4, 0)){
		printk("Gpio led - Can not set gpio4 to output\n");
		goto gpio4Error;
	}
	printk("Gpio led - Set gpio4 to output success\n");
	if(DEBUG)
		printk("Gpio led - Value of led: %d\n", gpio_get_value(GPIO4));

	return 0;

classError:
	unregister_chrdev_region(device_nr, 1);
fileError:
	class_destroy(my_class);
addError:
	device_destroy(my_class, device_nr);
gpio4Error:
	gpio_free(516);
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
	gpio_free(4);
}

module_init(ModuleInit);
module_exit(ModuleExit);
