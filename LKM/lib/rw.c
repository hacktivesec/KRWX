#ifndef _HOOKED_FUNCS_H
#include <linux/slab.h>
#include <asm/uaccess.h>
#include "../hooking.c"
#endif

extern struct kmem_cache* global_kmem[];
extern struct kmem_cache* dumb_kmem;

int ioctl_rw_read(struct msg_read* user_read_msg){
  struct msg_read read_msg;
  unsigned long result;
  if( copy_from_user(&read_msg, (void*) user_read_msg, sizeof(struct msg_read)) )
    return -EFAULT;
  memcpy(&result, read_msg.kaddress, sizeof(unsigned long));
  if( put_user(result, &user_read_msg->content) )
    return -EFAULT;

  return 0;
}

unsigned long ioctl_rw_write(struct msg_write* user_write_msg){
  struct msg_write write_msg;
  if( copy_from_user(&write_msg, (void*) user_write_msg, sizeof(struct msg_write)) )
    return -EFAULT;
  memcpy(write_msg.kaddress, &write_msg.value, write_msg.size);
  return 0;
}

int ioctl_kmalloc(struct io_kmalloc* __user arg){
  struct io_kmalloc sk;
  void* k_res;
  if( copy_from_user(&sk, (void*) arg, sizeof(struct io_kmalloc)) )
    return -EFAULT;
  k_res = kmalloc(sk.size, sk.flags);
  if( copy_to_user(sk.result, &k_res, sizeof(void*)) ) 
    return -EFAULT;
  return 0;
}

int ioctl_kfree(void* arg){
  kfree(arg);
  return 0;
}

int ioctl_kmem_create_usercopy(struct io_kmem_create* __user user_kmem){
  struct io_kmem_create kmem;
  void* kmem_addr;
  if( copy_from_user(&kmem, (void*) user_kmem, sizeof(struct io_kmem_create)) )
    return -EFAULT;
  pr_info("Creating cache with name: %s", kmem.name);
  kmem_addr = kmem_cache_create_usercopy(kmem.name, kmem.obj_size, kmem.align, kmem.flags, kmem.useroffset, kmem.usersize, NULL);
  if(!kmem_addr)
    return -EFAULT;

  if( put_user(kmem_addr, &user_kmem->result) )
    return -EFAULT;

  return 0;
}

int ioctl_kmem_alloc(struct io_kmem_alloc* __user user_kmem){
  void* obj;
  struct io_kmem_alloc kmem;

  if( copy_from_user(&kmem, (void*) user_kmem, sizeof(struct io_kmem_alloc)) )
    return -EFAULT;

  obj = kmem_cache_alloc(kmem.kmem_addr, kmem.flags);
  if( put_user(obj, &user_kmem->result) )
    return -EFAULT;

  return 0;
}

int ioctl_kmem_free(struct io_kmem_free* __user user_kmem){
  struct io_kmem_free kmem;
  if( copy_from_user(&kmem, (void*) user_kmem, sizeof(struct io_kmem_free)) )
    return -EFAULT;

  kmem_cache_free(kmem.kmem_addr, kmem.pointer);
  return 0;
}


int ioctl_kmem_get(struct io_kmem_get* __user user_kmem){
  struct io_kmem_get kmem;
  struct kmem_cache* s;
  struct list_head* head;

  if( copy_from_user(&kmem, (void*) user_kmem, sizeof(struct io_kmem_get)) )
    return -EFAULT;

  if( !dumb_kmem )
    return -EFAULT;

  // Cycle through all caches
  list_for_each(head, &dumb_kmem->list){
    s = list_entry(head, struct kmem_cache, list);
    if(strncmp(kmem.name, s->name, NAME_SZ) == 0){
      // Cache found !
      if( put_user(s, &user_kmem->result) )
        return -EFAULT;
    }
  }

  return 0;
}

/* 
 * Temporary workaround since virt_to_folio has been introducted in 5.17
 * https://github.com/kiks7/KRWX/issues/1
 */
#ifndef folio_page_idx
int virt_to_folio_supported = 0;
static inline struct folio *virt_to_folio(const void *x){
  return 0;
}
#endif
#ifdef folio_page_idx
int virt_to_folio_supported = 1;
#endif

int ioctl_slab_ptr(struct io_slab_ptr* __user user_slab_ptr){
  struct slab* s;
  struct kmem_cache* kmem;
  struct io_slab_ptr slab_ptr;
  size_t name_len;
  if(!virt_to_folio_supported)
    return -ENOTTY;

  if( copy_from_user(&slab_ptr, (void*) user_slab_ptr, sizeof(struct io_slab_ptr)) )
    return -EFAULT;

  if(!virt_addr_valid(slab_ptr.ptr)){
    // Invalid vaddr
    return 0;
  }
  s = (struct slab*) virt_to_folio(slab_ptr.ptr);
  if(!s)
    return 0;
  if(!virt_addr_valid(s->slab_cache)){
    return 0;
  }
  kmem = s->slab_cache;
  name_len = strlen(kmem->name);
  if(name_len > NAME_SZ)
    return -EFAULT;
  if( copy_to_user(slab_ptr.name, kmem->name, name_len) )
    return -EFAULT;
  return 0;
}
