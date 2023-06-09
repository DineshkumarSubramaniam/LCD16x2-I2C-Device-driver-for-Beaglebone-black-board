/*
   Device driver to print "HELLO WORLD" in lcd display
   */
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>
#include<linux/uaccess.h> 
#include<linux/cdev.h>
#include<linux/i2c.h>
#include<linux/device.h>
#include<linux/delay.h>
#include<linux/gpio.h> 
#include<linux/fs.h>
#include<linux/slab.h> 

/* Macros declaration */
#define SLAVE_ADDR	        0x27
#define I2C_BUS         	2
#define DEVICE_NAME	       "I2C_DEVICE"

/* I2C device declaration */
#define I2C_BACK_LIGHT        0X08            
#define RESET                 0x01
#define ENABLE_PIN            0X04

/* Allocating a major number */
static int major = 239;
static dev_t devno;


/* structure declaration */
/* Structure for I2C adapter  */
static struct i2c_adapter *lcd_i2c_adapter  =   NULL; 
/* structure for i2c client(LCD)*/
static struct i2c_client *lcd_client = NULL; 

/* structure for creating a device */
static struct class *cl;
static struct cdev my_lcd_cdev;
static struct file_operations fops;

/* Function declaration's */
static int lcd_init(void);
static int lcd_send_data(uint8_t data);
static int lcd_send_command(uint8_t command);


/* lcd_init module */
int lcd_init(void)
{
	printk(KERN_INFO "Entered into lcd_init function in line number %d\n",__LINE__);
	/* Waiting for >= 15msec after LCD turn ON */
	mdelay(25); 
	
	/* Initialize the lcd */
	lcd_send_command(0x03);
	/* Waiting for >= 5msec after initialize */
	mdelay(5); 
	 
        /* Return home */
	lcd_send_command(0x02);
	mdelay(1); 
        
        /* Turn off blink */
	lcd_send_command(0x08);
	mdelay(5); 
	
	/* Setting the display mode */
	lcd_send_command(0x00);
	mdelay(1); 
	
	/* Turn on display and cursor */
	lcd_send_command(0x0f);
	/* Waiting for >= 5msec after initialize */
	mdelay(5);
	
	/*  Display clear */
	lcd_send_command(0x01);
	/* Waiting for >= 5msec after initialize */
	mdelay(5);

	return 0;
}


int lcd_send_data(uint8_t data)
{

	uint8_t upper, lower, total[4]; 
	uint8_t val;

	upper = (data & 0xf0);
	/* mask off the lower nibble and the upper nibble */
	lower = ((data << 4) & 0xf0);

        /* Sets the msb 4 bits of the character */
        /* set the most significant(MSB) 4 bits of the character */
	total[0] = (upper | I2C_BACK_LIGHT | ENABLE_PIN | RESET);
	total[1] = (upper | I2C_BACK_LIGHT | RESET);
        /* set the least significant(LSB) 4 bits of the character */
	total[2] = (lower | I2C_BACK_LIGHT | ENABLE_PIN | RESET);
	total[3] = (lower | I2C_BACK_LIGHT | RESET);


	/* sending the data to the i2c client device */
	/* structure pointer , data to be transmitted , count */
	val = i2c_master_send(lcd_client, total, 4);
	if (val < 0)    
	{
		printk(KERN_INFO"i2c_master_send() Failed in lcd_send_data function in line number %d\n", __LINE__);
	}

	return 0;
}


int lcd_send_command(uint8_t command)
{

	uint8_t upper, lower, total[4]; 
        uint8_t val;

        /* for making as command doing AND with 0xf0 */
	upper = (command & 0xf0);
        /* This operation will takes 8 bit and extract the upper 4 bits*/
	lower = ((command << 4) & 0xf0);

        /* set the most significant(MSB) 4 bits of the character */
	total[0] = (upper | I2C_BACK_LIGHT | ENABLE_PIN );
	//total[1] = (upper | I2C_BACK_LIGHT | RESET);
	total[1] = (upper | I2C_BACK_LIGHT);
	
        /* set the least significant(LSB) 4 bits of the character */
	total[2] = (lower | I2C_BACK_LIGHT | ENABLE_PIN );
	//total[3] = (lower | I2C_BACK_LIGHT | RESET);
        total[3] = (lower | I2C_BACK_LIGHT);

	/* sending the data to the i2c client device */
	/* structure pointer , data to be transmitted , count */
	val = i2c_master_send(lcd_client, total, 4);
	if (val < 0)    
	{
		printk(KERN_INFO"i2c_master_send() Failed in lcd_send_command function in line number %d\n",__LINE__);
	}

	return 0;
}


static int i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret, minor;
	int storage = 0, i;
	struct device *de_create;
	
	printk(KERN_INFO"THE STRING STARTED TO WRITE IN LCD IN LINE NUMBER %d\n",__LINE__);
	
	char string[] = "HELLO WORLD! !";
	
	printk(KERN_INFO"THE STRING FINISHED TO WRITE IN LCD IN LINE NUMBER %d\n",__LINE__);
	
	printk(KERN_INFO"Inside the i2c_probe function , probed successflly in line number %d\n", __LINE__);
	devno = MKDEV(major, 0);
	
	/* allocates a character device region and stores the starting device number and the number of devices in dev */
	ret = alloc_chrdev_region(&devno, 0, 1,"I2C_DEVICE_DRIVER");
	if(ret < 0)
	{
	        printk(KERN_INFO"alloc_chrdev_region is failed in line number %d\n", __LINE__);
		return -1;
	}

        /* Assigning the major and minor numbers to the variable's */
	major = MAJOR(devno);
	minor = MINOR(devno);
	
	printk(KERN_INFO "alloc_chrdev_region() allocated dev number of Major number is:%d and  Minor number is:%d in line number %d\n", major, minor,__LINE__);


	/* Creating an Device class */
	/* Group devices with similar characteristics */
	/* owner, name of the class (describe the type of device) */
	cl = class_create(THIS_MODULE, "I2C_DEVICE");
	if(IS_ERR(cl))
	{
		printk(KERN_INFO"class_create() failed in line number %d\n",__LINE__);
		unregister_chrdev_region(devno, 1);
		return -1;
	}
	
	printk(KERN_INFO"Class created successfully in line number %d\n",__LINE__);

        /* Individual devices within a device class */
	/* Class of device , parent for device file,  major and minor number define panna use aakum, Device data ,Device name */
	de_create = device_create(cl, NULL, devno, NULL,"I2C_DEVICE");
	if(IS_ERR(de_create))
	{
		printk(KERN_INFO"device_create() failed in line number %d\n",__LINE__);
		class_destroy(cl);
		unregister_chrdev_region(devno, 1);
		return -1;
	}
	printk(KERN_INFO"Device created successfully in line number %d\n", __LINE__);

	/* Initializes the character device cdev with the file operations structure */
	cdev_init(&my_lcd_cdev, &fops);

	/* Notify the kernel about the new device */
	ret = cdev_add(&my_lcd_cdev, devno, 1);
	if(ret < 0) 
	{
		printk(KERN_INFO "cdev_add() failed in line number %d\n",__LINE__);
		device_destroy(cl, devno);
		class_destroy(cl);
		unregister_chrdev_region(devno, 1);
	}
	printk(KERN_INFO "cdev_add() added cdev into kernel in line number %d\n",__LINE__);
        
	/* Calling lcd_init function */
	lcd_init();
	
	printk(KERN_INFO"lcd_init() invoked in line number %d",__LINE__);

	storage = sizeof(string);
	
	for(i=0; i < (storage-1); i++)
	{
		lcd_send_data(string[i]);
	}

	printk(KERN_INFO"lcd_send_data() invoked successfully in the line number %d\n",__LINE__);
	return 0;
}


static int i2c_remove(struct i2c_client *client)
{
	printk(KERN_INFO"Entered into i2c_remove() in line number %d\n",__LINE__);

	/* Deletes the character device structure associated with the driver */
	cdev_del(&my_lcd_cdev);
	printk(KERN_INFO"cdev_del() destroy dev file successfull in line number %d\n",__LINE__);

        /* Destroys the device file associated with the driver */
	device_destroy(cl, devno);
	printk(KERN_INFO"device_destroy() destroy dev file successfull in line number %d\n",__LINE__);

	/* Destroys the device class created for the driver */
	class_destroy(cl);
	printk(KERN_INFO"class_destroy() destroy dev class successfull in line number %d\n",__LINE__);

	/* Un-register the character device driver(Major and minor number) */
	unregister_chrdev_region(devno, 1);
	printk(KERN_INFO"unregister_chardev_region() released dev num successfull in line %d\n",__LINE__);


	return 0;
}


/* This structure will give the information(ID) of slave device */
static const struct i2c_device_id i2c_id[] = {
	{ DEVICE_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, i2c_id);


/* declares and initializes an i2c_driver structure */
static struct i2c_driver i2c_driver = {
	.driver = {
		.name	= DEVICE_NAME,
		.owner	= THIS_MODULE,
	},
	.probe = i2c_probe,
	.remove = i2c_remove,
	.id_table = i2c_id,
};


/*
   STRUCTURE FOR I2C BOARD INFORMATION 
   */
static struct i2c_board_info i2c_info = {
	I2C_BOARD_INFO(DEVICE_NAME, SLAVE_ADDR)
};

static int __init i2c_init(void)
{
	lcd_i2c_adapter     = i2c_get_adapter(I2C_BUS);
	
	printk(KERN_INFO"\n\n\n\nTHE I2C DEVICE DRIVER IS STARTS INSERTING FROM HERE FROM LINE NUMBER %d......\n",__LINE__);
	
	printk(KERN_INFO"i2c_get_adapter() invoked in i2c_init function in line number %d\n",__LINE__);

	if( lcd_i2c_adapter  != NULL )
	{
		lcd_client = i2c_new_device(lcd_i2c_adapter, &i2c_info);
		printk(KERN_INFO"i2c_new_device() invoked in i2c_init function in line number %d\n",__LINE__);

		if( lcd_client != NULL )
		{
			i2c_add_driver(&i2c_driver);
			printk(KERN_INFO"i2c_add_driver() invoked in i2c_init in line number %d\n",__LINE__);
			return 0;
		}

		i2c_put_adapter(lcd_i2c_adapter);
		printk(KERN_INFO"i2c_put_adapter() invoked in i2c_init function in line number %d\n",__LINE__);
	}

	pr_info("LCD Driver Added Successfully\n");
	return 0; 

}

static void __exit i2c_exit(void)
{
	printk(KERN_INFO"Entering into exit module in line number %d\n",__LINE__);

        /* unregister an I2C device that was previously registered with the system using the i2c_new_device() function */
	i2c_unregister_device(lcd_client);

        /* used to unregister an I2C driver from the system */
	i2c_del_driver(&i2c_driver);
	printk(KERN_INFO"i2c_del_driver() invoked succesfull in line number %d\n",__LINE__);
	printk(KERN_INFO"EXITING FROM DRIVER IN LINE NUMBER %d.....\n",__LINE__);	

}

module_init(i2c_init);
module_exit(i2c_exit);

/* Module licenses */
MODULE_AUTHOR("Dinesh Kumar Subramaniam");
MODULE_DESCRIPTION("Device driver for lcd display associated with PCF8574T I2C module for printing hello world !");
MODULE_LICENSE("GPL");
