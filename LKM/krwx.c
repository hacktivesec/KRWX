#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/goldfish.h>
#include <linux/mm.h>
#include <linux/acpi.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include "krwx.h"
#include "lib/rw.c"
#include <linux/slab.h>

static const struct file_operations krwx_fops = {
  .open = krwx_open,
  .release = krwx_release,
  .unlocked_ioctl = krwx_ioctl,
  .write = krwx_write,
  .read = krwx_read,
};

static struct miscdevice krwx_miscdev = {
  .minor = MISC_DYNAMIC_MINOR,
  .name = "krwx",    
  .fops = &krwx_fops,
};

struct kmem_cache* dumb_kmem = NULL;
int krwx_init(void){
  int err = 0;
  pr_info("krwx::krwx_init \n");
  err = misc_register(&krwx_miscdev);
  if(err) {
    pr_err("Unable to register KRWX driver\n");
    return err;
  }
  // Creating dumb kmem_cache just to retrieve its list (and have all slab_caches)
  dumb_kmem = kmem_cache_create("dumb_krwx", 16, 8, SLAB_ACCOUNT, NULL);
  if( !dumb_kmem )
    pr_info("Error allocating the dumb cache\n");


  return 0;
}

void krwx_exit(void){
  pr_info("krwx::exit\b");
  misc_deregister(&krwx_miscdev);
}

int krwx_open(struct inode *inode, struct file *file){
  pr_info("krwx::open\n");
  return 0;
}

int krwx_release(struct inode *inode, struct file *file){
  pr_info("krwx::release\n");
  return 0;
}

struct kmem_cache* global_kmem[MAX_KMEM];

long int krwx_ioctl(struct file *fp, unsigned int cmd, unsigned long arg){
  long int ret = 0x0;
  switch(cmd){
    case IOCTL_RW_READ:
      ret = ioctl_rw_read((struct msg_read*) arg);
      break;
    case IOCTL_RW_WRITE:
      ret = ioctl_rw_write((struct msg_write *) arg);
      break;
    case IOCTL_KMALLOC:
      ret = ioctl_kmalloc((struct io_kmalloc *) arg);
      break;
    case IOCTL_KFREE:
      ret = ioctl_kfree((void*) arg);
      break;
    case IOCTL_MEMK_CREATE:
      ret = ioctl_kmem_create_usercopy((struct io_kmem_create*) arg);
      break;
    case IOCTL_MEMK_ALLOC:
      ret = ioctl_kmem_alloc((struct io_kmem_alloc*) arg);
      break;
    case IOCTL_MEMK_FREE:
      ret = ioctl_kmem_free((struct io_kmem_free*) arg);
      break;
    case IOCTL_MEMK_GET:
      ret = ioctl_kmem_get((struct io_kmem_get*) arg);
      break;
    case IOCTL_SLAB_PTR:
      ret = ioctl_slab_ptr((struct io_slab_ptr*) arg);
      break;
    default:
      break;
  }
  return ret;
}

ssize_t krwx_write(struct file * file, const char __user *buf, size_t count, loff_t *pos){
  pr_info("krwx::write - NOT IMPLEMENTED\n");
  return 0;

}

ssize_t krwx_read(struct file * file, char __user *buf, size_t count, loff_t *pos){
  pr_info("krwx::read - NOT IMPLEMENTED\n");
  return 0;
}

module_init(krwx_init);
module_exit(krwx_exit);

MODULE_AUTHOR("Alessandro Groppo @kiks");
MODULE_DESCRIPTION("Kernel Read Write Execute");
MODULE_LICENSE("GPL");
