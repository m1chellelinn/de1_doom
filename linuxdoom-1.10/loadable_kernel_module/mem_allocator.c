#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

static unsigned long buf_size = 7*1024*1024;

static void *cpu_addr;
static dma_addr_t phys_addr;
static struct kobject *kobj;

static dev_t devno;
static struct cdev cdev;
static struct class *fpga_class;

static ssize_t phys_show(struct kobject *k, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%llu\n", (unsigned long long)phys_addr);
}
static struct kobj_attribute phys_attr = __ATTR_RO(phys);

static int fpga_open(struct inode *inode, struct file *file) {
    return 0;
}

static int fpga_release(struct inode *inode, struct file *file) {
    return 0;
}

static int fpga_mmap(struct file *filp, struct vm_area_struct *vma) {
    return dma_mmap_coherent(NULL, vma, cpu_addr, phys_addr, buf_size);
}

static const struct file_operations fpga_fops = {
    .owner   = THIS_MODULE,
    .open    = fpga_open,
    .release = fpga_release,
    .mmap    = fpga_mmap,
};

static int __init fpga_space_init(void) {
    int ret;

    cpu_addr = dma_alloc_coherent(NULL, buf_size, &phys_addr, GFP_KERNEL);
    if (!cpu_addr) return -ENOMEM;

    // sysfs entry for physical address (optional)
    kobj = kobject_create_and_add("fpga_space", kernel_kobj);
    if (!kobj) { dma_free_coherent(NULL, buf_size, cpu_addr, phys_addr); return -ENOMEM; }
    sysfs_create_file(kobj, &phys_attr.attr);

    // char device
    ret = alloc_chrdev_region(&devno, 0, 1, "fpga_allocator_space");
    if (ret < 0)
        goto err_sysfs;

    cdev_init(&cdev, &fpga_fops);
    ret = cdev_add(&cdev, devno, 1);
    if (ret < 0)
        goto err_chrdev;

    fpga_class = class_create(THIS_MODULE, "fpga_allocator");
    if (IS_ERR(fpga_class)) {
        ret = PTR_ERR(fpga_class);
        goto err_cdev;
    }
    device_create(fpga_class, NULL, devno, NULL, "fpga_allocator_space");

    pr_info("fpga_shm: reserved %lu bytes @ phys 0x%llx, device /dev/fpga_allocator_space\n",
            buf_size, (unsigned long long)phys_addr);

    return 0;

err_cdev:
    cdev_del(&cdev);
err_chrdev:
    unregister_chrdev_region(devno, 1);
err_sysfs:
    sysfs_remove_file(kobj, &phys_attr.attr);
    kobject_put(kobj);
    dma_free_coherent(NULL, buf_size, cpu_addr, phys_addr);
    return ret;
}

static void __exit fpga_space_exit(void) {
    device_destroy(fpga_class, devno);
    class_destroy(fpga_class);
    cdev_del(&cdev);
    unregister_chrdev_region(devno, 1);

    sysfs_remove_file(kobj, &phys_attr.attr);
    kobject_put(kobj);

    dma_free_coherent(NULL, buf_size, cpu_addr, phys_addr);
}

module_init(fpga_space_init);
module_exit(fpga_space_exit);
MODULE_LICENSE("GPL");
