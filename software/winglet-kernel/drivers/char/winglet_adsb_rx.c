#include <linux/build_bug.h>
#include <linux/bitops.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/firmware/sunxi_dsp.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/poll.h>

#define ADSB_MSG_BUFFER_DEPTH 32

#define MSG_FLAG_LONG_MSG 0x8000
#define MSG_LENGTH_LONG 14
#define MSG_LENGTH_SHORT 7
struct adsb_msg {
	u16 metadata;
	u8 msg[14];
};

// device data holder, this structure may be extended to hold additional data
struct adsb_rx_inst {
	struct platform_device *pdev;
	struct platform_device *dsp_pdev;
	struct cdev cdev;
	dev_t adsb_dev;
	struct device *char_dev;
	wait_queue_head_t wq;
	DECLARE_KFIFO(adsb_buffer, struct adsb_msg, ADSB_MSG_BUFFER_DEPTH);
	int in_test_mode;
};

/******************************************
 * Driver Low Level
 *****************************************/

#define MAX_DEV 4

static void adsb_rx_mbox_cb(unsigned int mbox, unsigned int val, void *arg)
{
	struct adsb_rx_inst *inst = (struct adsb_rx_inst *)arg;

	if (mbox != 0)
		return;

	struct adsb_msg msg;

	if (inst->in_test_mode) {
		if (val == 0xCA91D04E) {
			// Capture complete, throw random data into msg to report it
			memset(&msg, 0, sizeof(msg));
		} else {
			return;
		}
	} else {
		void __iomem *src = sunxi_dsp_lookup_localaddr(inst->dsp_pdev, val);
		if (IS_ERR(src)) {
			dev_err(&inst->pdev->dev,
				"Failed to determine DEST addr: %ld\n",
				PTR_ERR(src));
		}

		memcpy_fromio(&msg, src, sizeof(msg));
		int ret = sunxi_dsp_msgbox_send(inst->dsp_pdev, 0, val);

		if (ret < 0) {
			dev_err(&inst->pdev->dev,
				"Failed to notify DSP that sample read: %d\n",
				ret);
			return;
		}
	}

	if (!kfifo_put(&inst->adsb_buffer, msg)) {
		dev_warn_ratelimited(
			&inst->pdev->dev,
			"ADSB Packet Dropped... Buffer not read fast enough by user\n");
		return;
	}
	wake_up(&inst->wq);
}

// initialize file_operations
static int adsbdev_open(struct inode *inode, struct file *file)
{
	struct adsb_rx_inst *inst =
		container_of(inode->i_cdev, struct adsb_rx_inst, cdev);
	INIT_KFIFO(inst->adsb_buffer);

	dev_info(&inst->pdev->dev, "device open\n");
	return sunxi_dsp_enable("dsp0.hex", inst->dsp_pdev, adsb_rx_mbox_cb,
				inst);
}

static int adsbdev_release(struct inode *inode, struct file *file)
{
	struct adsb_rx_inst *inst =
		container_of(inode->i_cdev, struct adsb_rx_inst, cdev);
	dev_info(&inst->pdev->dev, "device close\n");
	sunxi_dsp_disable(inst->dsp_pdev);
	return 0;
}

static long adsbdev_ioctl(struct file *file, unsigned int cmd,
			  unsigned long arg)
{
	struct adsb_rx_inst *inst =
		container_of(file->f_path.dentry->d_inode->i_cdev,
			     struct adsb_rx_inst, cdev);
	dev_info(&inst->pdev->dev, "device ioctl\n");
	return -EINVAL;
}

static ssize_t adsbdev_read(struct file *file, char __user *buf, size_t count,
			    loff_t *offset)
{
	struct adsb_rx_inst *inst =
		container_of(file->f_path.dentry->d_inode->i_cdev,
			     struct adsb_rx_inst, cdev);
	dev_vdbg(&inst->pdev->dev, "device read\n");
	int ret;

	if (inst->in_test_mode) {
		if (count > SAMPLE_BUF_LEN) {
			count = SAMPLE_BUF_LEN;
		}
		if (count % 2 != 0) {
			count /= 2;
			count *= 2;
		}

		ret = sunxi_dsp_msgbox_send(inst->dsp_pdev, 0, count);
		if (ret < 0) {
			return ret;
		}
	}

	struct adsb_msg msg;
	while (!kfifo_get(&inst->adsb_buffer, &msg)) {
		ret = wait_event_interruptible(
			inst->wq, !kfifo_is_empty(&inst->adsb_buffer));
		if (ret == -ERESTARTSYS) {
			return -EINTR;
		}
	}

	if (inst->in_test_mode) {
		ret = __copy_to_user(
			buf, sunxi_dsp_sample_buf_addr(inst->dsp_pdev), count);
		if (ret) {
			return ret;
		}
		return count;
	}

	size_t msg_len = (msg.metadata & MSG_FLAG_LONG_MSG ? MSG_LENGTH_LONG :
							     MSG_LENGTH_SHORT);

	if (count < msg_len) {
		return -EINVAL;
	}

	ret = __copy_to_user(buf, msg.msg, msg_len);
	if (ret) {
		return ret;
	}

	return msg_len;
}

static ssize_t adsbdev_write(struct file *file, const char __user *buf,
			     size_t count, loff_t *offset)
{
	// struct adsb_rx_inst *inst = container_of(file->f_path.dentry->d_inode->i_cdev, struct adsb_rx_inst, cdev);
	// dev_info(&inst->pdev->dev, "device write\n");
	return -EINVAL;
}

static __poll_t adsbdev_poll(struct file *file, struct poll_table_struct *wait)
{
	struct adsb_rx_inst *inst =
		container_of(file->f_path.dentry->d_inode->i_cdev,
			     struct adsb_rx_inst, cdev);
	dev_vdbg(&inst->pdev->dev, "device poll\n");

	__poll_t mask = 0;

	poll_wait(file, &inst->wq, wait);

	if (!kfifo_is_empty(&inst->adsb_buffer)) {
		mask |= (POLLIN | POLLRDNORM);
	}

	return mask;
}

static const struct file_operations adsbdev_fops = {
	.owner = THIS_MODULE,
	.open = adsbdev_open,
	.release = adsbdev_release,
	.poll = adsbdev_poll,
	.unlocked_ioctl = adsbdev_ioctl,
	.read = adsbdev_read,
	.write = adsbdev_write
};

// global storage for device Major number
static int adsb_devmajor = 0;
static DEFINE_SPINLOCK(adsbminor_lock);
static u32 adsb_minormask = 0;
static_assert(sizeof(adsb_minormask) * 8 >= MAX_DEV,
	      "Too many devices defined");

static int adsb_get_next_dev(dev_t *dev_out)
{
	unsigned long flags;
	spin_lock_irqsave(&adsbminor_lock, flags);

	if (!(adsb_minormask ^ ((1 << MAX_DEV) - 1))) {
		spin_unlock_irqrestore(&adsbminor_lock, flags);

		// No devices remain to be allocated
		return -1;
	}

	int first_zero = ffz(adsb_minormask);
	adsb_minormask |= 1 << first_zero;
	spin_unlock_irqrestore(&adsbminor_lock, flags);

	*dev_out = MKDEV(adsb_devmajor, first_zero);

	return 0;
}

static void adsb_release_dev(dev_t dev)
{
	WARN(!(adsb_minormask & (1 << MINOR(dev))),
	     "minor dev already released");
	adsb_minormask &= ~(1ul << MINOR(dev));
}

// sysfs class structure
static struct class *adsb_class = NULL;

static int adsb_rx_probe(struct platform_device *pdev)
{
	int err = 0;
	struct adsb_rx_inst *pc;

	pc = devm_kzalloc(&pdev->dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;

	platform_set_drvdata(pdev, pc);
	pc->pdev = pdev;
	init_waitqueue_head(&pc->wq);
	INIT_KFIFO(pc->adsb_buffer);
	pc->in_test_mode = 0;

	pc->dsp_pdev = sunxi_dsp_by_phandle(&pdev->dev, 0);
	if (IS_ERR(pc->dsp_pdev)) {
		err = PTR_ERR(pc->dsp_pdev);
		dev_dbg(&pdev->dev, "Failed to load dsp device: %d\n", err);
		return err;
	}

	err = adsb_get_next_dev(&pc->adsb_dev);
	if (err) {
		dev_dbg(&pdev->dev, "no minor devices left");
		return -ENOSPC;
	}

	cdev_init(&pc->cdev, &adsbdev_fops);
	pc->cdev.owner = THIS_MODULE;

	err = cdev_add(&pc->cdev, pc->adsb_dev, 1);
	if (err) {
		dev_dbg(&pdev->dev, "could not add cdev: %d\n", err);
		goto release_minor;
	}

	// create device node /dev/adsbx where "x" is "i", equal to the Minor number
	pc->char_dev = device_create(adsb_class, &pdev->dev, pc->adsb_dev, pdev,
				     "adsb%d", MINOR(pc->adsb_dev));
	if (IS_ERR(pc->char_dev)) {
		err = PTR_ERR(pc->char_dev);
		dev_dbg(&pdev->dev, "could not add device: %d\n", err);
		goto delete_cdev;
	}

	dev_info(&pdev->dev, "bound adsb driver to %s\n", pc->dsp_pdev->name);

	return 0;

delete_cdev:
	cdev_del(&pc->cdev);

release_minor:
	adsb_release_dev(pc->adsb_dev);

	return err;
}

static ssize_t test_mode_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct adsb_rx_inst *inst = platform_get_drvdata(pdev);
	int target, ret;

	ret = kstrtoint(buf, 10, &target);
	if (ret)
		return ret;

	if (target != 0 && target != 1)
		return -EINVAL;

	inst->in_test_mode = target;
	return ret ? ret : count;
}

/******************************************
 * Device Attributes
 *****************************************/

static ssize_t test_mode_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct adsb_rx_inst *inst = platform_get_drvdata(pdev);

	return sprintf(buf, "%d\n", inst->in_test_mode);
}
static DEVICE_ATTR_RW(test_mode);

static struct attribute *dev_attrs[] = {
	&dev_attr_test_mode.attr,
	NULL,
};

static struct attribute_group dev_attr_group = {
	.attrs = dev_attrs,
};

static const struct attribute_group *dev_attr_groups[] = {
	&dev_attr_group,
	NULL,
};

static const struct of_device_id winglet_adsb_rx_dt_match[] = {
	{
		.compatible = "dcav,winglet-adsb-rx",
		.data = 0,
	},
	{},
};
MODULE_DEVICE_TABLE(of, winglet_adsb_rx_dt_match);

static struct platform_driver winglet_adsb_rx_driver = {
	.probe = adsb_rx_probe,
	.driver = {
		.name		= "winglet-adsb-rx",
		.owner  = THIS_MODULE,
		.of_match_table	= winglet_adsb_rx_dt_match,
		.dev_groups = dev_attr_groups,
	},
};

static int __init winglet_adsb_rx_driver_init(void)
{
	dev_t dev;
	int err;

	// allocate chardev region and assign Major number
	err = alloc_chrdev_region(&dev, 0, MAX_DEV,
				  winglet_adsb_rx_driver.driver.name);
	if (err) {
		printk(KERN_ERR "[adsb-rx] Failed to alloc chrdev: %d\n", err);
		return err;
	}

	adsb_devmajor = MAJOR(dev);
	adsb_minormask = 0;

	// create sysfs class
	adsb_class = class_create("adsb-rx");
	if (IS_ERR(adsb_class)) {
		err = PTR_ERR(adsb_class);
		printk(KERN_ERR "[adsb-rx] Failed to create class: %d\n", err);
		goto cleanup_chrdev;
	}

	err = platform_driver_register(&winglet_adsb_rx_driver);
	if (err) {
		printk(KERN_ERR
		       "[adsb-rx] Failed to register platform driver: %d\n",
		       err);
		goto cleanup_class;
	}

	return 0;

cleanup_class:
	class_destroy(adsb_class);
cleanup_chrdev:
	unregister_chrdev_region(MKDEV(adsb_devmajor, 0), MAX_DEV);

	return err;
}
module_init(winglet_adsb_rx_driver_init);

static void __exit winglet_adsb_rx_driver_exit(void)
{
	platform_driver_unregister(&winglet_adsb_rx_driver);
	class_unregister(adsb_class);
	class_destroy(adsb_class);
	unregister_chrdev_region(MKDEV(adsb_devmajor, 0), MAX_DEV);
}
module_exit(winglet_adsb_rx_driver_exit);

MODULE_AUTHOR("Robert Pafford <19439938+rjp5th@users.noreply.github.com>");
MODULE_DESCRIPTION("Winglet ADSB Receiver Driver");
