/*
 *  fpga access.c:
 *  kernel driver
 *  provides controlled access to the fpga memory mapped regs
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>		/* for put_user */
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/ioport.h>
//#include <linux/iomap.h>
#include "fpga_access.h"

#define DRIVER_AUTHOR		"Paul Corcoran <paulc@digiconsoles.com>"
#define DRIVER_DESCRIPTION	"Digico P16xx fpga access routines"

/* used to test debug read/write  access */
#ifdef DEBUG
static uint32_t initial_reg_value[NUM_REGS] = {
	0x20000001, /* INTERRUPT_PENDING_REG */
	0x30000002, /* INTERRUPT_ENABLE_REG */
	0x40000003, /* INTERRUPT_CNTL_REG */
	0x50000004, /* READ_WRITE_TEST_REG */
	0x60000005, /* STREAMER_FIFO_DATA */
	0x70000006, /* AUDIO_FIFO_DATA */
	0x80000007, /* EXTENDED_ADDR_REG */
	0x90000008  /* EXTENDED_DATA_REG */
};
#endif

/*
 * Notes on Locks mmiowb & interrupts
 *
 * See Documentation/DocBook/deviceiobook.tmpl for more information.
 */

/*
 * Initialise by fpga_access_open
 */
//void __iomem *fpga_base_p=NULL;
uint32_t *fpga_base_p = NULL;

/*
 * Read Rx Audio fifo
 * read count values into buf from the streamer fifo
 */
void fpga_read_rxafifo(uint32_t *buf, unsigned long count)
{
	ioread32_rep(fpga_base_p + FPGA_REG_AUDIO_FIFO_DATA, buf, count);
}

/*
 * Write Tx Audio fifo
 * write count values from the buf into the streamer fifo
 */
void fpga_write_txafifo(uint32_t *buf, int count)
{
	iowrite32_rep(fpga_base_p + FPGA_REG_AUDIO_FIFO_DATA, buf, count);
	wmb();
}

/*
 * Read Rx Streamer fifo
 * read count values into buf from the streamer fifo
 */
void fpga_read_rxsfifo(uint32_t *buf, unsigned long count)
{
	ioread32_rep(fpga_base_p + FPGA_REG_STREAMER_FIFO_DATA, buf, count);
}

/*
 * Write Tx Streamer fifo
 * write count values from the buf into the streamer fifo
 */
void fpga_write_txsfifo(uint32_t *buf, int count)
{
	iowrite32_rep(fpga_base_p + FPGA_REG_STREAMER_FIFO_DATA, buf, count);
	wmb();
}

/*
 * Read ICR
 */
uint32_t fpga_read_icr(void)
{
	return ioread32(fpga_base_p + FPGA_REG_INTERRUPT_CNTL_REG);
}

/*
 * Write ICR
 */
void fpga_write_icr(uint32_t val)
{
	iowrite32(val, fpga_base_p + FPGA_REG_INTERRUPT_CNTL_REG);
	wmb();
}

/*
 * Read the Interrupt pending register
 */
uint32_t fpga_read_ipr(void)
{
	return ioread32(fpga_base_p + FPGA_REG_INTERRUPT_PENDING_REG);

}

/*
 * Read the Interrupt enable register
 */
uint32_t fpga_read_ier(void)
{
	return ioread32(fpga_base_p + FPGA_REG_INTERRUPT_ENABLE_REG);

}

/*
 * Write Interrupt Eable Register
 */
void fpga_write_ier(uint32_t val)
{
	iowrite32(val, fpga_base_p + FPGA_REG_INTERRUPT_ENABLE_REG);
	wmb();
}

/*
 * Read Test Reg for RX buf data available
 */
uint32_t fpga_read_rx_data_available(void)
{
	return ioread32(fpga_base_p + FPGA_REG_READ_WRITE_TEST_REG) >> 16;
}

/*
 * Read Test Reg for TX buf space available
 */
uint32_t fpga_read_tx_space_available(void)
{
	return ioread32(fpga_base_p + FPGA_REG_READ_WRITE_TEST_REG) & 0xffff;
}

/*
 * Read Test Reg
 */
uint32_t fpga_read_test_reg(void)
{
	return ioread32(fpga_base_p + FPGA_REG_READ_WRITE_TEST_REG);
}

/*
 * Write Test Reg
 */
void fpga_write_test_reg(uint32_t val)
{
	iowrite32(val, fpga_base_p + FPGA_REG_READ_WRITE_TEST_REG);
	wmb();
}

/*
 * Write Extended Data Reg
 */
void fpga_write_extended_data_reg(uint32_t val)
{
	iowrite32(val, fpga_base_p + FPGA_REG_EXTENDED_DATA_REG);
	wmb();
}

/*
 * Read Extended Data Reg
 */
uint32_t fpga_read_extended_data_reg(void)
{
	return ioread32(fpga_base_p + FPGA_REG_EXTENDED_DATA_REG);	
}

/*
 * Write Extended Addr Reg
 */
void fpga_write_extended_addr_reg(uint32_t val)
{
	iowrite32(val, fpga_base_p + FPGA_REG_EXTENDED_ADDR_REG);
	wmb();
}

/*
 * Read Extended Addr Reg
 */
uint32_t fpga_read_extended_addr_reg(void)
{
	return ioread32(fpga_base_p + FPGA_REG_EXTENDED_ADDR_REG);

}

/*
 * Read Extended Reg
 */
uint32_t fpga_read_extended_reg(uint32_t addr)
{
	iowrite32(addr, fpga_base_p + FPGA_REG_EXTENDED_ADDR_REG);
	wmb();
	return ioread32(fpga_base_p + FPGA_REG_EXTENDED_DATA_REG);
}

/*
 * Write Extended Reg
 */
void fpga_write_extended_reg(uint32_t addr, uint32_t val)
{
	iowrite32(val, fpga_base_p + FPGA_REG_EXTENDED_ADDR_REG);
	wmb();
	iowrite32(val, fpga_base_p + FPGA_REG_EXTENDED_DATA_REG);
	wmb();
}

/*
 * 	base_addr = start address in physical memory
 * 	len = size of memory area in bytes
 * 	name = "name of the memory region"
 *
 *
 * 	reserve memory for fpga
 * 	map memory for fpga
 */
int fpga_access_open(unsigned long base_addr, unsigned long len, char *name)
{
	int status;
#ifdef DEBUG
	int i;
	uint32_t *p1;
#endif

	status = 0;
	fpga_base_p = NULL;

#ifdef DEBUG
	/* Not running on the real HW so just allocate some */
	/* kernel memory and pretend */
	fpga_base_p = kmalloc((NUM_REGS * REGS_BYTE_SIZE), GFP_KERNEL);
	if(fpga_base_p == NULL)
	{
		printk(KERN_ALERT "xeng : ioremap_nocache failed!\n");
		status = -ENOMEM;
		goto error;
	}

	p1 = (uint32_t *) fpga_base_p;
	for(i = 0; i < NUM_REGS; i++)
	{
		*(p1 + i) = initial_reg_value[i];
	}

#else
	/* This is the real thing!
	 * HW registers are memory mapped
	 * and can be found at base_addr + reg_offset
	 */
	if(request_mem_region(base_addr, len, name) == NULL)
	{
		printk(KERN_ALERT "xeng : request_mem_region failed!\n");
		status = -EBUSY;
		goto error;
	}

	fpga_base_p = ioremap(base_addr, len);

	if(fpga_base_p == NULL)
	{
		printk(KERN_ALERT "xeng : ioremap_nocache failed!\n");
		status = -ENOMEM;
		goto error;
	}
#endif

  error:
	return status;
}

/*
 * 	Tidy up
 */
int fpga_access_close(unsigned long base_addr, unsigned long len)
{
	int status;
	status = 0;

#ifdef DEBUG
	if(fpga_base_p != NULL)
	{
		kfree(fpga_base_p);
		fpga_base_p = NULL;
	}

#else
	if(fpga_base_p != NULL)
	{
		release_mem_region(base_addr, len);
		iounmap(fpga_base_p);
		fpga_base_p = NULL;
	}
#endif
	return status;
}

/*
EXPORT_SYMBOL(fpga_access_open);
EXPORT_SYMBOL(fpga_access_close);
*/

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
