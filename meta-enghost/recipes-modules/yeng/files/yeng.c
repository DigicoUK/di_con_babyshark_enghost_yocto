#define DEBUG

#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/idr.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>

#define DRIVER_NAME "yeng"
#define CDEV_NAME_PREFIX "xengstream"
#define MAX_DEVICES 16

/*#define READ_IRQ_DEBUG*/
#if defined(READ_IRQ_DEBUG)
#define dev_yengirq_info(dev, fmt, ...) \
	dev_info(dev, fmt, ##__VA_ARGS__);
#else
#define dev_yengirq_info(dev, fmt, ...) \
({ \
	if (0) \
		dev_info(dev, fmt, ##__VA_ARGS__); \
})
#endif

#define READ_BUFFER_SIZE_DWORDS 2048

/* Registers */
#define YENG_IPR		0 /* interrupt pending */
#define YENG_IER		4 /* interrupt enable */
#define YENG_ICR		8 /* interrupt control */
#define YENG_RW_TEST		12 /* read write test */
#define YENG_STREAM_DATA	16 /* streamer fifo data */
#define YENG_AUDIO_DATA		20 /* audio fifo data */
#define YENG_EXT_ADDR		24 /* extended address */
#define YENG_EXT_DATA		28 /* extended data */

static DEFINE_IDA(yeng_ida);
static DEFINE_MUTEX(yeng_ida_lock);
static DEFINE_SPINLOCK(yeng_kfifo_spinlock);

static struct class *yeng_class;
static dev_t yeng_cdev_first;

struct yeng_device {
	struct cdev cdev;
	struct device *dev;
	void __iomem *iomem;
	dev_t cdev_id;

	DECLARE_KFIFO_PTR(read_fifo, u32);
	u32 *read_buffer;
};

/* hardware access */

static u32 yeng_hw_read(struct yeng_device *yeng, u32 offset)
{
	dev_dbg(yeng->dev, "hw read %d", offset);
	return ioread32(yeng->iomem + offset);
}

static void yeng_hw_read_rep(struct yeng_device *yeng, u32 offset, void *buf, unsigned int count)
{
	dev_dbg(yeng->dev, "hw read repeat %d (count %d)", offset, count);
	ioread32_rep(yeng->iomem + offset, buf, count);
}

static void yeng_hw_write(struct yeng_device *yeng, u32 offset, u32 value)
{
	dev_dbg(yeng->dev, "hw write %d w %d", offset, value);
	iowrite32(value, yeng->iomem + offset);
	wmb(); /* liam: is this actually required? */
}

static irqreturn_t yeng_read_irq_handler(int irq, void *dev)
{
	struct yeng_device *yeng;
	int read_size;
	int capped_read_size;
	int i;
	unsigned long flags;

	unsigned int rw_test;

	yeng = dev;
	dev_dbg(yeng->dev, "read irq");

	spin_lock_irqsave(&yeng_kfifo_spinlock, flags);

	rw_test = yeng_hw_read(yeng, YENG_RW_TEST);
	read_size = rw_test >> 16;

	dev_yengirq_info(yeng->dev, "The full rw_test was %08x (upper %d, lower %d)\n", rw_test, rw_test >> 16, rw_test & 0xffff);

	/*dev_info(yeng->dev, "read size %d"*/

	if (read_size > READ_BUFFER_SIZE_DWORDS) {
		dev_warn(yeng->dev, "Attempted read size %x greater than read buffer %x, data will be lost", read_size, READ_BUFFER_SIZE_DWORDS);
		rw_test = yeng_hw_read(yeng, YENG_RW_TEST);
		dev_yengirq_info(yeng->dev, "try 2: %08x\n", rw_test);
	}

	capped_read_size = min(read_size, READ_BUFFER_SIZE_DWORDS);

	yeng_hw_read_rep(yeng, YENG_STREAM_DATA, yeng->read_buffer, capped_read_size);

	for(i = 0; i < min(read_size, 32); i++) {
	    dev_yengirq_info(yeng->dev, "read buffer[%d]: %08x\n", i, yeng->read_buffer[i]);
	}

	kfifo_in(&yeng->read_fifo, yeng->read_buffer, capped_read_size);

	spin_unlock_irqrestore(&yeng_kfifo_spinlock, flags);

	return IRQ_HANDLED;
}

/* Character device ops */

static ssize_t yeng_cdev_read(struct file *file, char __user *buf, size_t count, loff_t *fpos)
{
	struct yeng_device *yeng;
	int res;
	int copied;

	yeng = file->private_data;
	dev_dbg(yeng->dev, "read count=%d", count);

	if (!kfifo_is_empty(&yeng->read_fifo)) {
		res = kfifo_to_user(&yeng->read_fifo, buf, count, &copied);
		if (res < 0) {
			dev_err(yeng->dev, "Error copying fifo to user %d", res);
			return res;
		}

		return copied;
	} else {
		return 0;
	}
}

static ssize_t yeng_cdev_write(struct file *file, const char __user *buf, size_t count, loff_t *fpos)
{
	struct yeng_device *yeng;
	size_t count_dwords;
	int i;
	u32 incoming;
	u32 __user *buf_u32;

	yeng = file->private_data;
	dev_dbg(yeng->dev, "write count=%d", count);

	if ((count < 4) || (count % sizeof(u32) != 0)) {
		dev_err(yeng->dev, "invalid write size %d", count);
		return -EINVAL;
	}

	count_dwords = count / sizeof(u32);

	buf_u32 = (u32*)buf;
	for (i = 0; i < count_dwords; i++) {
		get_user(incoming, &buf_u32[i]);
		/* TODO(liam): true or false? */
		/* Andrea : We don't need to check the buffer because the FPGA will always outperform us (famous last words) */
		yeng_hw_write(yeng, YENG_STREAM_DATA, incoming);
	}

	return count;
}

static int yeng_cdev_open(struct inode *inode, struct file *file)
{
	struct yeng_device *yeng;

	yeng = container_of(inode->i_cdev, struct yeng_device, cdev);
	file->private_data = yeng;
	dev_dbg(yeng->dev, "open");
	return 0;
}

static int yeng_cdev_release(struct inode *inode, struct file *file)
{
	struct yeng_device *yeng;

	yeng = container_of(inode->i_cdev, struct yeng_device, cdev);
	dev_dbg(yeng->dev, "close");
	return 0;
}

static struct file_operations yeng_fops = {
	.owner = THIS_MODULE,
	.read = yeng_cdev_read,
	.write = yeng_cdev_write,
	.open = yeng_cdev_open,
	.release = yeng_cdev_release,
};

/* driver and device setups */

static void yeng_clk_disable_unprepare(void *data)
{
	clk_disable_unprepare(data);
}

int yeng_probe(struct platform_device *pdev)
{
	struct resource *regs_resource;
	void __iomem *iomem;
	int irq;
	int minor;
	struct yeng_device *yeng;
	int res;
	struct device *dev;
	struct device *dev_create;
	struct clk *clk;

	dev = &pdev->dev;

	dev_dbg(dev, "probe");

	/* alloc device private struct */
	yeng = devm_kzalloc(dev, sizeof(struct yeng_device), GFP_KERNEL);
	if (!yeng)
		return -ENOMEM;
	yeng->dev = dev;

	/* alloc read buffer */
	yeng->read_buffer = devm_kcalloc(dev, READ_BUFFER_SIZE_DWORDS, sizeof(u32), GFP_KERNEL);
	if (!yeng->read_buffer)
		return -ENOMEM;

	/* alloc read fifo */
	res = kfifo_alloc(&yeng->read_fifo, 0x40000, GFP_KERNEL);
	if (res < 0)
		return res;

	/* ioremap memory mapped registers */
	iomem = devm_platform_get_and_ioremap_resource(pdev, 0, &regs_resource);
	if (IS_ERR(iomem)) {
		res = PTR_ERR(iomem);
		goto out_free_kfifo;
	}
	yeng->iomem = iomem;
	dev_dbg(dev, "registers mapped: %x-%x", regs_resource->start, regs_resource->end);


	/* get virtual irq number from device tree */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		res = irq;
		goto out_free_kfifo;
	}
	dev_dbg(dev, "virt irq claimed: %d", irq);

	/* setup irq handler */
	res = devm_request_irq(dev, irq, yeng_read_irq_handler, 0, pdev->name, yeng);
	if (res < 0)
		goto out_free_kfifo;
	dev_dbg(dev, "irq handler registered");

	/* get clock from device tree */
	clk = devm_clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		res = dev_err_probe(dev, PTR_ERR(clk), "Unable to get clock");
		goto out_free_kfifo;
	}

	/* enable clock */
	res = clk_prepare_enable(clk);
	if (res)
		goto out_free_kfifo;
	res = devm_add_action_or_reset(dev, yeng_clk_disable_unprepare, clk);
	if (res)
		goto out_free_kfifo;
	dev_dbg(dev, "enabled clock");

	/* init character device */
	cdev_init(&yeng->cdev, &yeng_fops);
	yeng->cdev.owner = THIS_MODULE;

	/* allocate character device minor number */
	mutex_lock(&yeng_ida_lock);
	minor = ida_simple_get(&yeng_ida, 0, 0, GFP_KERNEL);
	mutex_unlock(&yeng_ida_lock);
	if (minor >= MAX_DEVICES) {
		/* ran out of minors */
		res = -ENODEV;
		goto out_remove_ida;
	}

	/* add character device */
	yeng->cdev_id = MKDEV(MAJOR(yeng_cdev_first), MINOR(minor));
	res = cdev_add(&yeng->cdev, yeng->cdev_id, 1);
	if (res < 0)
		goto out_remove_ida;
	dev_dbg(dev, "added cdev, major=%d minor=%d", MAJOR(yeng_cdev_first), MINOR(minor));

	/* create device file */
	dev_create = device_create(yeng_class, NULL, yeng->cdev_id, NULL, CDEV_NAME_PREFIX "%d", minor);
	if (IS_ERR(dev_create)) {
		res = PTR_ERR(dev_create);
		goto out_del_cdev;
	}
	dev_dbg(dev, "created device " CDEV_NAME_PREFIX "%d", minor);

	/* store private device struct */
	platform_set_drvdata(pdev, yeng);


	yeng_hw_write(yeng, YENG_IER, 1);
	yeng_hw_write(yeng, YENG_ICR, 1); /* not sure on this one, keep same as prev driver */

	return 0;

/*out_destroy_device:*/
/*	device_destroy(yeng_class, yeng->cdev_id);*/
out_del_cdev:
	cdev_del(&yeng->cdev);
out_remove_ida:
	mutex_lock(&yeng_ida_lock);
	ida_simple_remove(&yeng_ida, minor);
	mutex_unlock(&yeng_ida_lock);
out_free_kfifo:
	kfifo_free(&yeng->read_fifo);
	return res;
}

int yeng_remove(struct platform_device *pdev)
{
	struct yeng_device *yeng;

	yeng = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "remove device (cdev major=%d minor=%d)", MAJOR(yeng->cdev_id), MINOR(yeng->cdev_id));

	kfifo_free(&yeng->read_fifo);
	device_destroy(yeng_class, yeng->cdev_id);
	cdev_del(&yeng->cdev);
	mutex_lock(&yeng_ida_lock);
	ida_simple_remove(&yeng_ida, MINOR(yeng->cdev_id));
	mutex_unlock(&yeng_ida_lock);
	return 0;
}

static const struct of_device_id yeng_of_match[] = {
	{ .compatible = "digico,yeng", },
	{}
};
MODULE_DEVICE_TABLE(of, yeng_of_match);

static struct platform_driver yeng_platform_driver = {
	.probe = yeng_probe,
	.remove = yeng_remove,
	.driver = {
		.name = "yeng",
		.of_match_table = yeng_of_match,
	},
};

static int __init yeng_init(void)
{
	int res = 0;

	yeng_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(yeng_class))
		return PTR_ERR(yeng_class);

	res = alloc_chrdev_region(&yeng_cdev_first, 0, MAX_DEVICES, "yeng");
	if (res < 0)
		goto out_destroy_class;

	res = platform_driver_register(&yeng_platform_driver);
	if (res < 0)
		goto out_unregister_chrdev;

	return 0;

out_unregister_chrdev:
	unregister_chrdev_region(yeng_cdev_first, MAX_DEVICES);
out_destroy_class:
	class_destroy(yeng_class);
	return res;
}
module_init(yeng_init);


static void __exit yeng_exit(void)
{
	platform_driver_unregister(&yeng_platform_driver);
	unregister_chrdev_region(yeng_cdev_first, MAX_DEVICES);
	class_destroy(yeng_class);
}
module_exit(yeng_exit);

MODULE_DESCRIPTION("PS-PL engine comms rewritten as platform device");
MODULE_AUTHOR("Liam Pribis");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
