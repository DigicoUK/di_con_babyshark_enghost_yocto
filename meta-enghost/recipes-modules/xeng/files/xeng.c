/*
 *  xeng.c:
 *  kernel driver
 *  created 2 devices xeng0 and xeng1
 *  xeng0 provides access to the engine control stream
 *  xeng1 provides access to the engine audio stream
 */
//#define DEBUG 
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>		/* for put_user */
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/debugfs.h>

#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/delay.h>
/* We want an interrupt */
#include <linux/gpio.h> 
#include <linux/interrupt.h>

#include "xeng.h"
#include "fpga_access.h"

#define DRIVER_AUTHOR		"Paul Corcoran <paulc@digiconsoles.com> , Andrea Campanella <andrea@digiconsoles.com>"
#define DRIVER_VERSION		"1.21"
#define DRIVER_DESCRIPTION	"Digico P161xx xilinx engine driver"
#define MEM_SIZE	65536			/* 8 * 32 bit registers = 32 bytes */
#define IMX_GPIO_NR(bank, nr) (((bank) - 1) * 32 + (nr))

/*
 *
 * GPIO6=ARMIRQ from fpga
 *
 * GPIO6 signals 0-15 use IRQ 108 (bank 1 IO 6)
 * (see page 225 of the Processor Ref Manual)
 * We use this as our EIM interrupt from Guy.
 * It's active high.
 */

#define FPGA_IRQ_GPIO   IMX_GPIO_NR(1,6) 
#define IRQ_NUM      gpio_to_irq(FPGA_IRQ_GPIO) //166 


static DEFINE_KFIFO(xeng_read_fifo , uint32_t, 262144);
//static DEFINE_KFIFO(xeng_write_fifo, uint32_t, 16384);


/* 
 * module_param(foo, int, 0000)
 * The first param is the parameters name
 * The second param is it's data type
 * The final argument is the permissions bits, 
 * for exposing parameters in sysfs (if non-zero) at a later stage.
 */

static int benchmark = 0 ;
module_param(benchmark, int, S_IRUGO);
MODULE_PARM_DESC(benchmark, "Enable the performance output in the kernel logs. 1 to enable, 0 to disable (default)");

static int read_method = 0 ;
module_param(read_method, int, S_IRUGO);
MODULE_PARM_DESC(read_method, " 0 to use blocking reads (default), 1 to use nonblocking reads");

static int mem_base = 0x08000000;
module_param(mem_base, int, S_IRUGO);
MODULE_PARM_DESC(mem_base, "The memory base address for the EIM bus.");	


static int tx_threshold = 0 ;
module_param(tx_threshold, int, S_IRUGO);
MODULE_PARM_DESC(tx_threshold, "Interrupt Tx streamer threshold (0)=1 (default) , (1)=2, (2)=4, (3)=8, (4)=16, (5)=32, (6)=64, (7)=128 DWORDs");	



/* Variables */

int g_time_interval = 1000;//in ms
struct timer_list g_timer;
/****bechmark mode variables****/
static  int RX_Fpga_Buffer_Dim = 0;
static  int Rx_IRQ_Counter = 0;
static  int Rx_Data_Counter = 0;
static  int Tx_Data_Counter = 0;
/*******************************/
static 	int test_irq_count = 0;
static 	int rx_streamer_irq_count = 0;
static 	int tx_streamer_irq_count = 0;
static 	int rx_audio_irq_count = 0;
static 	int tx_audio_irq_count = 0;
static	uint32_t FPGA_incoming_DWORD [2048];
static  int FPGA_Rx_Buffer;
static  int wq_need_awake = 0;

static 	DECLARE_WAIT_QUEUE_HEAD(write_event_queue);
static  DECLARE_WAIT_QUEUE_HEAD(wq);
static DEFINE_MUTEX(IrqLock);

/* /sys/kernel/debug directory */
static struct dentry *debug_dir, *status_dir, *regs_dir;
static dev_t Major;				/* Major number assigned to our device driver */
static struct class *xeng_class;	/* Tie with the device model */
static irq_handler_t  Reader_Interrupt_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);
static int device_open_nonblocking(struct inode *, struct file *);
static int device_release_nonblocking(struct inode *, struct file *);
static ssize_t device_read_nonblocking(struct file *, char *, size_t, loff_t *);
static ssize_t device_read_blocking(struct file *, char *, size_t, loff_t *);
static ssize_t device_write_nonblocking(struct file *, const char *, size_t, loff_t *);
static int debug_open(struct inode *inode, struct file *file);
static ssize_t debug_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t debug_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

/* These are the device drivers file operations (for testing) */
static struct file_operations fops_nonblocking = {
	.owner = THIS_MODULE,		// prevent module being unloaded while in use
	.read = device_read_nonblocking,
	.write = device_write_nonblocking,
	.open = device_open_nonblocking,
	.release = device_release_nonblocking
};

static struct file_operations fops_blocking = {
	.owner = THIS_MODULE,		// prevent module being unloaded while in use
	.read = device_read_blocking,
	.write = device_write_nonblocking,
	.open = device_open_nonblocking,
	.release = device_release_nonblocking
};


/* These are the debugfs file operations */
static struct file_operations debug_fops = {
	.owner = THIS_MODULE,		// prevent module being unloaded while in use
	.open = debug_open,
	.read = debug_read,
	.write = debug_write
		//  .release = debug_release
};

typedef struct
{
	struct cdev mCdev;			/* The character device structure */
	char name[10];				/* device name "xeng0" or "xeng1"  */
	FPGA_REG fpgaReg;
} XENG_DEV;

XENG_DEV *xeng_dev_test;
XENG_DEV *xeng_dev_stream;

/*IRQ Handler*/
static irq_handler_t Reader_Interrupt_handler(unsigned int irq, void *dev_id, struct pt_regs *regs)
{
	int i;

	FPGA_Rx_Buffer = fpga_read_rx_data_available();

	if (benchmark == 1) {
		RX_Fpga_Buffer_Dim = FPGA_Rx_Buffer;
		Rx_IRQ_Counter++;
	}

	ioread32_rep(fpga_base_p + FPGA_REG_STREAMER_FIFO_DATA,&FPGA_incoming_DWORD,FPGA_Rx_Buffer);


	for (i = 0; i < FPGA_Rx_Buffer; ++i)
	{

		if(benchmark == 1){	Rx_Data_Counter ++; }
		mutex_lock(&IrqLock);
		kfifo_put(&xeng_read_fifo, &FPGA_incoming_DWORD[i]);	
		mutex_unlock(&IrqLock);
	}

	return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

void _TimerHandler(unsigned long data)
{
	/*Restarting the timer...*/
	mod_timer( &g_timer, jiffies + msecs_to_jiffies(g_time_interval));
	printk(KERN_ALERT "xeng : [Read : %d B/s, %d WORDs,%d IRQ,%d Buffer][Write :  %d B/s, %d WORDs ]\n",(Rx_Data_Counter*4),Rx_Data_Counter,Rx_IRQ_Counter,RX_Fpga_Buffer_Dim,(Tx_Data_Counter*4),Tx_Data_Counter);	
	Tx_Data_Counter = 0;
	Rx_Data_Counter = 0;
	Rx_IRQ_Counter = 0;
	RX_Fpga_Buffer_Dim = 0 ;
}

/* create the following files & dirs:
 *
 * xeng---status---test_irq_count
 *      |        |
 *      |         -rx_streamer_irq_count
 *      |        |
 *      |         -rx_streamer_irq_count
 *      |        |
 *      |         -rx_streamer_irq_count
 *      |        |
 *      |         -rx_streamer_irq_count
 *      |
 *       --regs----ipr
 *               |
 *                -ier
 *               |
 *                -icr
 *               |
 *                -rwt
 *               |
 *                -afd
 *               |
 *                -sfd
 *               |
 *                -exa
 *               |
 *                -exd
 */
int create_debugfs_entries(void)
{
	int status, done, i;

	status = 0;
	done = 0;
	for(i = 0; ((status == 0) & (done == 0)); i++)
	{
		switch (i)
		{
			case 0:
				debug_dir = debugfs_create_dir(DEVICE_NAME, NULL);
				if(debug_dir == NULL)
				{
					printk(KERN_ALERT "xeng : debugfs_create_dir:%s failed\n", DEVICE_NAME);
					status = -EPERM;
					done = 1;
				}
				break;
			case 1:
				status_dir = debugfs_create_dir("status", debug_dir);
				if(status_dir == NULL)
				{
					printk(KERN_ALERT "xeng : debugfs_create_dir:%s failed\n", "status");
					status = -EPERM;
					done = 1;
				}
				break;
			case 2:
				regs_dir = debugfs_create_dir("regs", debug_dir);
				if(status_dir == NULL)
				{
					printk(KERN_ALERT "xeng : debugfs_create_dir:%s failed\n", "regs");
					status = -EPERM;
					done = 1;
				}
				break;
			default:
				done = 1;
				break;
		}
	}

	if(status == 0)
	{
		/* first simple u32 values */
		debugfs_create_u32("test_irq_count", 0666, status_dir, (u32 *) & test_irq_count);
		debugfs_create_u32("rx_streamer_irq_count", 0666, status_dir, (u32 *) & rx_streamer_irq_count);
		debugfs_create_u32("tx_streamer_irq_count", 0666, status_dir, (u32 *) & tx_streamer_irq_count);
		debugfs_create_u32("rx_audio_irq_count", 0666, status_dir, (u32 *) & rx_audio_irq_count);
		debugfs_create_u32("tx_audio_irq_count", 0666, status_dir, (u32 *) & tx_audio_irq_count);

		/* More complicated (see debugfs_read + debugfs_write */
		/* private data = register offset.Note forcing an int into a (void *) */
		/* passed thru by open */
		debugfs_create_file("ipr", 0444, regs_dir, (void *) FPGA_REG_INTERRUPT_PENDING_REG, &debug_fops);	/* read only */
		debugfs_create_file("ier", 0666, regs_dir, (void *) FPGA_REG_INTERRUPT_ENABLE_REG , &debug_fops);
		debugfs_create_file("icr", 0666, regs_dir, (void *) FPGA_REG_INTERRUPT_CNTL_REG   , &debug_fops);
		debugfs_create_file("rwt", 0666, regs_dir, (void *) FPGA_REG_READ_WRITE_TEST_REG  , &debug_fops);
		debugfs_create_file("sfd", 0666, regs_dir, (void *) FPGA_REG_STREAMER_FIFO_DATA   , &debug_fops);
		debugfs_create_file("afd", 0666, regs_dir, (void *) FPGA_REG_AUDIO_FIFO_DATA      , &debug_fops);
		debugfs_create_file("exa", 0666, regs_dir, (void *) FPGA_REG_EXTENDED_ADDR_REG    , &debug_fops);
		debugfs_create_file("exd", 0666, regs_dir, (void *) FPGA_REG_EXTENDED_DATA_REG	  , &debug_fops);

	}
	return (status);
}

int makeDev(struct cdev *pCdev, const char *devName, unsigned major, unsigned minor, const struct file_operations *fops)
{
	int status;
	printk(KERN_ALERT "xeng : %s():%d devName: %s\n", __func__, __LINE__, devName);
	cdev_init(pCdev, fops);
	pCdev->owner = THIS_MODULE;

	/* connect major/minor numbers to the cdev */
	/* NOTE.. device now alive!! */
	if((status = cdev_add(pCdev, (major + minor), 1)))
	{
		printk(KERN_ALERT "xeng : cdev_add failed at line %d!\n", __LINE__);
		return status;
	}

	/* send uevents to udev, so it'll create /dev nodes */
	device_create(xeng_class, NULL, MKDEV(MAJOR(major), minor), NULL, devName);

	return SUCCESS;
}

/*This function is called when the module is loaded*/
int init_module(void)
{

	int status;

	/*Andrea : I'm going to leave the SysFs entries on in case we want to monitor the IRQ fromt the file system*/
	gpio_request(FPGA_IRQ_GPIO, "sysfs");       // Set up the gpioButton
	gpio_direction_input(FPGA_IRQ_GPIO);        // Set the button GPIO to be an input
	gpio_export(FPGA_IRQ_GPIO, true);          // Causes gpio to appear in /sys/class/gpio

	status = fpga_access_open(mem_base, MEM_SIZE, "xeng");

	if(status)
	{
		printk(KERN_ALERT "xeng : fpga_access_open failed! status=%d\n", status);
		return status;
	}

	/* request a device major number and a range of minor numbers */
	// TODO: FIXME: are those parameters correct?
	if(alloc_chrdev_region(&Major, 0, NUM_DEVICES + 1, DEVICE_NAME) < 0) // <------ +++ +
	{
		printk(KERN_ALERT "xeng : alloc_chrdev_region failed!\n");
		return -1;
	}

	status = create_debugfs_entries();

	if(status)
	{
		printk(KERN_ALERT "xeng : create_debugfs_entries failed!\n");
		return status;
	}

	/* Create a device class */
	if((xeng_class = class_create(THIS_MODULE, DEVICE_NAME)) == NULL)
	{
		// TODO: FIXME: are those parameters correct?
		unregister_chrdev_region(Major, NUM_DEVICES + 1); // <------ +++ +
		printk(KERN_ALERT "xeng : class_create failed!\n");
		return (-1);
	}

	if((xeng_dev_test = (XENG_DEV *) kmalloc(sizeof(XENG_DEV), GFP_KERNEL)))
	{
		// atomic_set(&xeng_dev_test->device_open, 0); // set device_open = 0
		xeng_dev_test->fpgaReg = FPGA_REG_READ_WRITE_TEST_REG;
		if((status = makeDev(&xeng_dev_test->mCdev, "xengtest", Major, 2, &fops_nonblocking)) != SUCCESS)
			return status;
	}
	else
	{
		printk(KERN_ALERT "xeng : kmalloc failed at line %d!\n", __LINE__);
		return -ENOMEM;
	}

	if((xeng_dev_stream = (XENG_DEV *) kmalloc(sizeof(XENG_DEV), GFP_KERNEL)))
	{
		xeng_dev_stream->fpgaReg = FPGA_REG_STREAMER_FIFO_DATA;
		if (read_method == 0)
		{
			printk(KERN_ALERT "xeng : Loading in blocking mode\n");
			if((status = makeDev(&xeng_dev_stream->mCdev, "xengstream", Major, 3, &fops_blocking)) != SUCCESS)
				return status;	
		}
		else if(read_method == 1){
			printk(KERN_ALERT "xeng : Loading in nonblocking mode\n");
			if((status = makeDev(&xeng_dev_stream->mCdev, "xengstream", Major, 3, &fops_nonblocking)) != SUCCESS)
				return status;


		}

	}
	else
	{
		printk(KERN_ALERT "xeng : kmalloc failed at line %d!\n", __LINE__);
		return -ENOMEM;
	}


	printk(KERN_ALERT "xeng : major number=%d mem_base=%08x\n", MAJOR(Major), mem_base);

	status = request_irq(IRQ_NUM,(irq_handler_t) Reader_Interrupt_handler,IRQF_TRIGGER_HIGH, DEVICE_NAME, &xeng_dev_stream);


	if (status < 0) {
		printk("xeng : request_irq %d failed with code %d , EIO %d , EINVAL %d\n",IRQ_NUM, status, EIO, EINVAL);
		return status;
	}
	else{
		printk(KERN_INFO "xeng : request of the irq %d succeded.\n",IRQ_NUM);
	}

	printk(KERN_INFO "xeng : Init successfully finished.\n");

	/*The Interrupts must be enabled after the handler*/

	fpga_write_ier(0x01);	// Set Global Interupt on

	//(0)=1, (1)=2, (2)=4, (3)=8, (4)=16, (5)=32, (6)=64, (7)=128
	//fpga_write_icr((tx_threshold << 5) + 0x001);	// Enable RX Buffer Interrupt
	fpga_write_icr((tx_threshold * 256) + 1);	// Enable RX Buffer Interrupt

	return SUCCESS;
}

/*This function is called when the module is unloaded*/
void cleanup_module(void)
{
	if(benchmark == 1){
		//delete the benchmark timer.
		del_timer(&g_timer); 	
	}
	/*Free the IRQ*/
	free_irq(IRQ_NUM,&xeng_dev_stream);
	/* destroy devices, cdev, free up memory etc */
	device_destroy(xeng_class, MKDEV(MAJOR(Major), 2));
	cdev_del(&xeng_dev_test->mCdev);
	kfree(xeng_dev_test);

	device_destroy(xeng_class, MKDEV(MAJOR(Major), 3));
	cdev_del(&xeng_dev_stream->mCdev);
	kfree(xeng_dev_stream);

	/* Remove debug dir */
	debugfs_remove_recursive(debug_dir);

	/* Now destroy the class */
	class_destroy(xeng_class);

	/* release memory assocaiated with fpga access */
	fpga_access_close(mem_base, MEM_SIZE);

	printk(KERN_ALERT "xeng : Unloaded\n");
}

/*
 * Methods
 */

/*
 * Called when a process tries to open the device file, like
 * "cat /dev/xeng0"
 * RESTRICTION only 1 open allowed per device at the mo
 */
static int device_open_nonblocking(struct inode *pInode, struct file *pFile)
{
	XENG_DEV *dev;		/* device information */

	/*  Find the device */
	dev = container_of(pInode->i_cdev, XENG_DEV, mCdev);

	/* and use pFile->private_data to point to the device data */
	pFile->private_data = dev;

	return SUCCESS;
}

/*
 * Called when a process closes the device file.
 */
static int device_release_nonblocking(struct inode *pInode, struct file *pFile)
{
	return 0;
}

/*
 * Called when a process reads the device file
 *
 * return value
 * positive = number read bytes
 * zero = end of read
 * negative = error
 *
 */



static ssize_t device_read_nonblocking(struct file *pFile, char *buf, size_t count, loff_t *f_pos)
{
	uint32_t *Buffer_reference_pointer = (uint32_t *) buf;
	uint32_t status = 0;
	uint32_t actual = 0;
	size_t File_lenght, i;
	XENG_DEV *dev = pFile->private_data;	/* get dev struct from private_data */

	File_lenght = count >> 2;

	if(!File_lenght)
		return -EINVAL; // at least 4 bytes space required in the buffer

	switch(dev->fpgaReg)
	{
		case FPGA_REG_READ_WRITE_TEST_REG:
			for(i = 0; i < File_lenght; i++)
				Buffer_reference_pointer[i] = fpga_read_test_reg();
			break;
		case FPGA_REG_STREAMER_FIFO_DATA:

			if(kfifo_len(&xeng_read_fifo) > 0 )
			{
				wq_need_awake = 1;
				wake_up_interruptible(&wq);
				mutex_lock(&IrqLock);
				status = kfifo_to_user(&xeng_read_fifo, buf, count, &actual);
				mutex_unlock(&IrqLock);
				return status ? status : actual;
			}

			else{						
				wait_event_interruptible(wq,wq_need_awake);
				wq_need_awake = 0;
				//printk(KERN_DEBUG "process %i (%s) going to sleep\n",current->pid, current->comm);

				return 0; /* EOF */
				//return -EAGAIN; /* signal: tell the fs layer to handle it */
			}



			break;
		default:
			break;
	}

	return File_lenght << 2;
}


/*
 * Called when a process reads the device file
 *
 * return value
 * positive = number read bytes
 * zero = end of read
 * negative = error
 *
 */
static ssize_t device_read_blocking(struct file *pFile, char *buf, size_t count, loff_t *f_pos)
{
	uint32_t *Buffer_reference_pointer = (uint32_t *) buf;
	uint32_t status = 0;
	uint32_t actual = 0;
	size_t File_lenght, i;
	XENG_DEV *dev = pFile->private_data;	/* get dev struct from private_data */

	File_lenght = count >> 2;

	if(!File_lenght)
		return -EINVAL; // at least 4 bytes space required in the buffer

	switch(dev->fpgaReg)
	{
		case FPGA_REG_READ_WRITE_TEST_REG:
			for(i = 0; i < File_lenght; i++)
				Buffer_reference_pointer[i] = fpga_read_test_reg();
			break;
		case FPGA_REG_STREAMER_FIFO_DATA:

			/*Read from the interrupt filled buffer if there is data*/
			if(kfifo_len(&xeng_read_fifo) > 0 )
			{
				status = kfifo_to_user(&xeng_read_fifo, buf, count, &actual);				

				return status ? status : actual;
			}

			else{
				File_lenght = -1;
				return 0; /* signal: tell the fs layer to handle it */
			}

			break;
		default:
			break;
	}

	return File_lenght << 2;
}


/*
 * Called when a process writes to dev file: echo "hi" > /dev/xengstream
 *
 * return value
 * positive = number read bytes
 * zero = end of read
 * negative = error
 *
 */

static ssize_t device_write_nonblocking(struct file *pFile, const char *buf, size_t count, loff_t *f_pos)
{
	uint32_t *Buffer_reference_pointer = (uint32_t *) buf;
	uint32_t Incoming_Data;

	size_t Data_lenght, i;
	XENG_DEV *dev = pFile->private_data;	/* get dev struct from private_data */

	if(count < 4 || count & 3)
		return -EINVAL; // at least 4 bytes required in the buffer and buffer size must be dividable by 4

	Data_lenght = count >> 2;

	switch(dev->fpgaReg)
	{
		case FPGA_REG_READ_WRITE_TEST_REG:
			for(i = 0; i < Data_lenght; i++)
				fpga_write_test_reg(Buffer_reference_pointer[i]);
			break;
		case FPGA_REG_STREAMER_FIFO_DATA:
			//Andrea : We don't need to check the buffer because the FPGA will always outperform us (Last famous words)
			//Data_lenght = min(Data_lenght, fpga_read_tx_space_available());
			//printk(KERN_ALERT "xeng : Data received, is %d long\n",count);	
			//copy_from_user(Incoming_Data_Buffer,Buffer_reference_pointer,Data_lenght);
			//fpga_write_txsfifo(Incoming_Data_Buffer,Data_lenght);

			for(i = 0; i < Data_lenght; i++)
			{
				//while(fpga_read_tx_space_available() < 1) {}
				//wmb(); 
				if(benchmark == 1){	Tx_Data_Counter ++; }
				get_user(Incoming_Data, Buffer_reference_pointer + i);
				//	if(benchmark == 1){	

				//		printk(KERN_ALERT "xeng : Writing DWORD : %x\n", Incoming_Data);

				//	}
				iowrite32(Incoming_Data, fpga_base_p + FPGA_REG_STREAMER_FIFO_DATA);
				//wmb(); 
			}


			/*
			   uint32_t payload[5] = { 0xFFa50004,0x002D0007,0x00000000,0x00000000,0xFFD2000B};
			   int f =0 ;
			   for (f = 0; f < 1048576; ++f)
			   {
			   for(i = 0; i < 5; i++)
			   {
			//while(fpga_read_tx_space_available() < 1) {}
			//wmb(); 
			//get_user(Incoming_Data, Buffer_reference_pointer + i);
			iowrite32(payload[i], fpga_base_p + FPGA_REG_STREAMER_FIFO_DATA);
			//wmb(); 
			}
			}
			*/

			break;
		default:
			break;
	}

	return Data_lenght << 2;
}

/*
 * Open is important as it copies the
 * private data from the inode to the file!
 *
 */
static int debug_open(struct inode *pInode, struct file *pFile)
{
	if(pInode->i_private)
		pFile->private_data = pInode->i_private;
	return SUCCESS;
}

/*
 * Called when a process reads from /sys/kernel/debug/xeng/fpga_regs
 *
 * return value
 * positive = number read bytes
 * zero = end of read
 * negative = error
 *
 */
static ssize_t debug_read(struct file *pFile, char *buf, size_t count, loff_t *f_pos)
{
	ssize_t len, bytes_read;
	FPGA_REG reg_addr = 0;
	char local_buf[32];
	uint32_t val;

	reg_addr = (FPGA_REG) pFile->private_data;

	switch(reg_addr)
	{
		case FPGA_REG_INTERRUPT_PENDING_REG:
			val = fpga_read_ipr();
			break;
		case FPGA_REG_INTERRUPT_ENABLE_REG:
			val = fpga_read_ier();
			break;
		case FPGA_REG_INTERRUPT_CNTL_REG:
			val = fpga_read_icr();
			break;
		case FPGA_REG_READ_WRITE_TEST_REG:
			val = fpga_read_test_reg();
			break;
		case FPGA_REG_STREAMER_FIFO_DATA:
			fpga_read_rxsfifo(&val, 1);
			break;
		case FPGA_REG_AUDIO_FIFO_DATA:
			fpga_read_rxafifo(&val, 1);
			break;
		case FPGA_REG_EXTENDED_ADDR_REG:
			val = fpga_read_extended_addr_reg();
			break;
		case FPGA_REG_EXTENDED_DATA_REG:
			val = fpga_read_extended_data_reg();
			break;
		default:
			len = scnprintf(local_buf, sizeof(local_buf), "read error");
			return -EINVAL;
			break;
	}

	/* Convert val to a string for user space */
	len = scnprintf(local_buf, sizeof(local_buf), "%08x\n", val);

	/*
	 *The simple_read_from_buffer() function reads up to @count bytes from the
	 *buffer @local_buf at offset @f_pos into the user space address starting at @buf.
	 *
	 *On success, the number of bytes read is returned and the offset @ppos is
	 *advanced by this number, or negative value is returned on error.
	 */
	bytes_read = simple_read_from_buffer(buf, count, f_pos, local_buf, len);

	return bytes_read;
}

/*
 * Called when a process writes to /sys/kernel/debug/xeng/fpga_regs
 *
 * return value
 * positive = number read bytes
 * zero = end of read
 * negative = error
 *
 */
static ssize_t debug_write(struct file *pFile, const char *buf, size_t count, loff_t *f_pos)
{
	ssize_t bytes_written = 0;
	FPGA_REG reg_addr, res = 0;
	char local_buf[32];
	uint32_t val;

	reg_addr = (FPGA_REG) pFile->private_data;

	/* The simple_write_to_buffer() function reads up to @count bytes from the user
	 * space address starting at @buf into the buffer @local_buf at offset @f_pos.
	 *
	 * On success, the number of bytes written is returned and the offset @ppos is
	 * advanced by this number, or negative value is returned on error.
	 */
	bytes_written = simple_write_to_buffer(local_buf, sizeof(local_buf) - 1, f_pos, buf, count);
	local_buf[bytes_written] = 0;

	/* convert string to u32 */
	res = kstrtou32(local_buf, 0, &val);
	if(res)
	{
		printk(KERN_ALERT "xeng : debug_write kstrol returned %d\n", res);
		return -EINVAL;
	}

	/* bounds check reg addr */
	switch(reg_addr)
	{
		case FPGA_REG_INTERRUPT_PENDING_REG:
			break;
		case FPGA_REG_INTERRUPT_ENABLE_REG:
			fpga_write_ier(val);
			break;
		case FPGA_REG_INTERRUPT_CNTL_REG:
			fpga_write_icr(val);
			break;
		case FPGA_REG_READ_WRITE_TEST_REG:
			fpga_write_test_reg(val);
			break;
		case FPGA_REG_STREAMER_FIFO_DATA:
			fpga_write_txsfifo(&val, 1);
			break;
		case FPGA_REG_AUDIO_FIFO_DATA:
			fpga_write_txafifo(&val, 1);
			break;
		case FPGA_REG_EXTENDED_ADDR_REG:
			fpga_write_extended_addr_reg(val);
			break;
		case FPGA_REG_EXTENDED_DATA_REG:
			fpga_write_extended_data_reg(val);
			break;
		default:
			return -EINVAL;
			break;
	}
	return bytes_written;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
