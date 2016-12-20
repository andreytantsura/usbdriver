#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/types.h>
#include <linux/tty.h>    
#include <linux/version.h> 
#include <linux/delay.h> 
#include <linux/reboot.h> 

struct task_struct *tAgetty;
struct task_struct *task;
bool stopThread = true;
bool isTry = true;
static int param = 1;
module_param( param, int, 0 );
int countTry = 3;

static int thread_agetty_uninterrupyible( void * data) 
{
	// основной цикл потока
	while(stopThread)
	{
		for_each_process(task)
		{

			if (strcmp(task->comm, "agetty") == 0 && task->state == TASK_INTERRUPTIBLE)
			{
				ssleep(1);
				printk(KERN_ERR "tty: %s [%d] %u \n", task->comm , task->pid, (u32)task->state);
				task->state = TASK_UNINTERRUPTIBLE;
			}
		}
	}
	if (countTry <= 0)
	{
		printk(KERN_ERR "Your computer will reboot in 3...\n");
		ssleep(1);
		printk(KERN_ERR "Your computer will reboot in 2...\n");
		ssleep(1);
		printk(KERN_ERR "Your computer will reboot in 1...\n");
		ssleep(1);
		kernel_restart(NULL);		
	}

	return -1;
}

static int pen_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(interface);
	printk(KERN_ERR "Connect your USB-device:\n");

	printk( KERN_ERR "idVendor=0x%hX, idProduct=0x%hX, bcdDevice=0x%hX\n",
		dev->descriptor.idVendor,
		dev->descriptor.idProduct, dev->descriptor.bcdDevice ); 
	
	if (isTry)
	{
		if (dev->descriptor.idVendor == 0x8564 && dev->descriptor.idProduct == 0x1000)
		{
			stopThread = false;
			isTry = false;
			ssleep(1);
			for_each_process(task)
			{
				if (strcmp(task->comm, "agetty") == 0 && task->state == TASK_UNINTERRUPTIBLE)
				{
					printk(KERN_ERR "flash: %s [%d] %u \n", task->comm , task->pid, (u32)task->state);
					task->state = TASK_INTERRUPTIBLE;
				}
			}
		} else {

			countTry--;
			printk(KERN_ERR "Tries left:  %i \n", countTry);
			if (countTry <= 0)
			{
				stopThread = false;
			}
		}		
	}

	
	return 0;
}

static void pen_disconnect(struct usb_interface *interface)
{
	printk(KERN_ERR "flash: disconnect\n");
}

static struct usb_device_id pen_table[] =
{
	{ USB_DEVICE(0x13FE, 0x3E00) },
	{ USB_DEVICE(0x8564, 0x1000) },
    	{} /* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, pen_table);

static struct usb_driver pen_driver =
{
	.name = "driver",
	.probe = pen_probe,
	.disconnect = pen_disconnect,
	.id_table = pen_table,
};

static int __init pen_init(void)
{
	printk("init: start\n");

	// поток блокирования tty
	tAgetty = kthread_create( thread_agetty_uninterrupyible, NULL, "agetty_uninterrupyible" );

	if (!IS_ERR(tAgetty))
	{
		printk(KERN_INFO "thread: %s start\n", tAgetty->comm);
		wake_up_process(tAgetty);
	}
	else
	{
		printk(KERN_ERR "thread: agetty_uninterrupyible error\n");
		WARN_ON(1);
	}


	return usb_register(&pen_driver);
}

static void __exit pen_exit(void)
{
	usb_deregister(&pen_driver);
}

module_init(pen_init);
module_exit(pen_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("none");
MODULE_DESCRIPTION("USB Pen Info Driver");
