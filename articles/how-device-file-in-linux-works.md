---
title: How device file in Linux works
layout: default
---

# How device file in Linux works <!-- omit in toc -->

A device file in Linux is an interface for various devices in the form of files. Although we can read and write a device file just the same as a normal file, read and write requests to device control requests by device file mechanism.

This article explains how Linux kernel and kernel modules convert read and write requests to device control requests. Because a device file depends on both a device driver and file system, I start this article with a chapter on a device driver and then file system. Finally, I show how the file system connects a device file with a device driver.

I wrote this article mainly using [Understanding the Linux Kernel, 3rd Edition](https://www.oreilly.co.jp/books/9784873113135/) and [https://github.com/torvalds/linux/tree/v6.1](https://github.com/torvalds/linux/tree/v6.1).

## Table of contents <!-- omit in toc -->

- [Device driver](#device-driver)
  - [Example of device driver](#example-of-device-driver)
  - [What we can see in `read_write.c`](#what-we-can-see-in-read_writec)
  - [insmod](#insmod)
    - [insmod in user space](#insmod-in-user-space)
    - [insmod in kernel space](#insmod-in-kernel-space)
- [File](#file)
  - [VFS(Virtual File System)](#vfsvirtual-file-system)
    - [inode](#inode)
    - [inode of normal files](#inode-of-normal-files)
    - [inode of device files](#inode-of-device-files)
- [Connect device driver and device file](#connect-device-driver-and-device-file)
  - [mknod](#mknod)
    - [mknod in user space](#mknod-in-user-space)
    - [mknod in kernel space](#mknod-in-kernel-space)
- [Reference](#reference)
- [Contacts](#contacts)

## Device driver

A device driver is a collection of kernel routines. Each routine corresponds to one Virtual File System (VFS) operation, which I will explain later.

### Example of device driver

I show a small but complete example of device driver below. This example is composed of `read_write.c` and `Makefile`. These two files are from [Johannes4Linux/Linux_Driver_Tutorial/03_read_write](https://github.com/Johannes4Linux/Linux_Driver_Tutorial/tree/main/03_read_write).

```
/ *read_write.c * /
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>

    MODULE_LICENSE("GPL");

#define DRIVER_MAJOR 333
#define DRIVER_NAME "read_write_driver"

static ssize_t driver_read(struct file *File, char *user_buffer, size_t count,
                           loff_t *offs) {
  user_buffer[0] = 'A';
  return 1;
}

static ssize_t driver_write(struct file *File, const char *user_buffer,
                            size_t count, loff_t *offs) {
  return 1;
}

static int driver_open(struct inode *device_file, struct file *instance) {
  printk("read_write_driver - open was called!\n");
  return 0;
}

static int driver_close(struct inode *device_file, struct file *instance) {
  printk("read_write_driver - close was called!\n");
  return 0;
}

static struct file_operations fops = {.open = driver_open,
                                      .release = driver_close,
                                      .read = driver_read,
                                      .write = driver_write};

static int __init ModuleInit(void) {
  printk("read_write_driver - ModuleInit was called!\n");
  register_chrdev(DRIVER_MAJOR, DRIVER_NAME, &fops);
  return 0;
}

static void __exit ModuleExit(void) {
  printk("read_write_driver - ModuleExit was called!\n");
  unregister_chrdev(DRIVER_MAJOR, DRIVER_NAME);
}

module_init(ModuleInit);
module_exit(ModuleExit);
```

```
# Makefile
obj-m += read_write.o

all:
    make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
    make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

Use the below commands to build and install this example device driver example to your Linux. Then, you can read infinite `A` from the device file.

```
$ make
$ sudo insmod read_write.ko
$ sudo mknod /dev/read_write c 333 1
$ cat /dev/read_write
AAAAAA...
```

### What we can see in `read_write.c`

We can see in ``read_write.c`:

- The major number of this device driver is `333`.
  - For your information, this major number `333` has no meaning. You can rewrite it with any arbitrary number.
- A Device driver is just a collection of functions.
- `open(2)`, `release(2)`, `read(2)` and `write(2)` are corresponds to `myDevice_open`, `driver_close`, `myDevice_read` and `driver_write`.

Because `cat /dev/read_write` calls `driver_read` of this device driver, infinite `A`s are read from it.

### insmod

`insmod(8)` is a command to load a kernel module into the Linux kernel. In this section, I explain how `sudo insmod read_write.ko` load `read_write.ko` into the kernel.

#### insmod in user space

`strace(1)` shows `insmod(8)` calls `finit_module(2)` system call.

```
# strace insmod read_write.ko
...
openat(AT_FDCWD, "/home/akira/misc/linux-device-file/driver_for_article/read_write.ko", O_RDONLY|O_CLOEXEC) = 3
read(3, "\177ELF\2\1", 6)               = 6
lseek(3, 0, SEEK_SET)                   = 0
newfstatat(3, "", {st_mode=S_IFREG|0664, st_size=6936, ...}, AT_EMPTY_PATH) = 0
mmap(NULL, 6936, PROT_READ, MAP_PRIVATE, 3, 0) = 0x7fc8aae77000
finit_module(3, "", 0)                  = 0
munmap(0x7fc8aae77000, 6936)            = 0
close(3)                                = 0
exit_group(0)                           = ?
+++ exited with 0 +++
```

#### insmod in kernel space

`finit_module(2)` is defined in Linux kernel at [kernel/module/main.c#29l6](https://github.com/akawashiro/linux/blob/4aeb800558b98b2a39ee5d007730878e28da96ca/kernel/module/main.c#L2916).

```
SYSCALL_DEFINE3(finit_module, int, fd, const char __user *, uargs, int, flags)
```

`finit_module(2)` calls [do_init_module](https://github.com/akawashiro/linux/blob/4aeb800558b98b2a39ee5d007730878e28da96ca/kernel/module/main.c#L2440) which initialise the kernel module.

```
/*
 * This is where the real work happens.
 *
 * Keep it uninlined to provide a reliable breakpoint target, e.g. for the gdb
 * helper command 'lx-symbols'.
 */
static noinline int do_init_module(struct module *mod)
```

[do_init_module](https://github.com/akawashiro/linux/blob/4aeb800558b98b2a39ee5d007730878e28da96ca/kernel/module/main.c#L2440) calls `ModuleInit` in the device driver through [ret = do_one_initcall(mod->init);](https://github.com/akawashiro/linux/blob/4aeb800558b98b2a39ee5d007730878e28da96ca/kernel/module/main.c#L2455).

```
    /* Start the module */
    if (mod->init != NULL)
        ret = do_one_initcall(mod->init);
    if (ret < 0) {
        goto fail_free_freeinit;
    }
```

[\_\_apply_relocate_add](https://github.com/akawashiro/linux/blob/7c8da299ff040d55f3e2163e6725cb1eef7155a9/arch/x86/kernel/module.c#L131-L220) set `mod->init`. This function process relocation information in the kernel module as we can infer from its name. Although I tried to understand the relationship between relocation information in the kernel module and `mod->init` by inserting many `printk`, I failed. Please tell me if you know it.

```
static int __apply_relocate_add(Elf64_Shdr *sechdrs,
           const char *strtab,
           unsigned int symindex,
           unsigned int relsec,
           struct module *me,
           void *(*write)(void *dest, const void *src, size_t len))
```

`ModuleInit`, called through `mod->init`, calls [\_\_register_chrdev](https://github.com/akawashiro/linux/blob/7c8da299ff040d55f3e2163e6725cb1eef7155a9/fs/char_dev.c#L247-L302) and [kobj_map](https://github.com/akawashiro/linux/blob/7c8da299ff040d55f3e2163e6725cb1eef7155a9/drivers/base/map.c#L32-L66). [kobj_map](https://github.com/akawashiro/linux/blob/7c8da299ff040d55f3e2163e6725cb1eef7155a9/drivers/base/map.c#L32-L66) register the kernel module device to [cdev_map](https://github.com/akawashiro/linux/blob/7c8da299ff040d55f3e2163e6725cb1eef7155a9/fs/char_dev.c#L28). This is the end of loading process of the kernel module.

```
int kobj_map(struct kobj_map *domain, dev_t dev, unsigned long range,
         struct module *module, kobj_probe_t *probe,
         int (*lock)(dev_t, void *), void *data)
{
    ...
    mutex_lock(domain->lock);
    for (i = 0, p -= n; i < n; i++, p++, index++) {
        struct probe **s = &domain->probes[index % 255];
        while (*s && (*s)->range < range)
            s = &(*s)->next;
        p->next = *s;
        *s = p;
    }
    ...
}
```

## File

This section explains "file" of "device file".

### VFS(Virtual File System)

VFS is a software layer in the Linux kernel that handles all standard UNIX filesystem system calls. It offers `open(2)`, `close(2)`, `write(2)` and etc. Owing to this software layer, users can use the same software for different file systems such as `ext4`, `NFS`, `proc`. For example, `cat(1)` can do both `cat /proc/self/maps` and `cat ./README.md`. However, `cat /proc/self/maps` shows memory map, and `cat ./README.md` shows a part of the disk.

VFS is implemented in an objected-oriented way using struct and function pointers.

#### inode

inode object in VFS is an object that represents "normal files". It is defined in [include/linux/fs.h](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/include/linux/fs.h#L588-L703).

```
struct inode {
    umode_t         i_mode;
    unsigned short      i_opflags;
    kuid_t          i_uid;
    kgid_t          i_gid;
    unsigned int        i_flags;
  ...
  union {
        struct pipe_inode_info  *i_pipe;
        struct cdev     *i_cdev;
        char            *i_link;
        unsigned        i_dir_seq;
    };
  ...
};
```

Other objects in VFS are superblock object which holds information of the filesystem itself, file objects which have information of opened file and process, d entry objects which have information on directories.

#### inode of normal files

`stat(1)` shows the inode information of the file, which corresponds to `struct inode`.

```
[@goshun](master)~/misc/linux-device-file
> stat README.md
  File: README.md
  Size: 20              Blocks: 8          IO Block: 4096   regular file
Device: fd01h/64769d    Inode: 49676330    Links: 1
Access: (0664/-rw-rw-r--)  Uid: ( 1000/   akira)   Gid: ( 1000/   akira)
Access: 2023-01-28 11:19:15.104727788 +0900
Modify: 2023-01-28 11:19:13.748734093 +0900
Change: 2023-01-28 11:19:13.748734093 +0900
 Birth: 2023-01-28 11:19:13.748734093 +0900
```

#### inode of device files

Because a device file is just a file, it has inode also. To find device files on your computer, you can use `ls -il`. Character device files start with `c`, and block device files starts with `b`.

```
[@goshun]/dev
> ls -il /dev/nvme0*
201 crw------- 1 root root 240, 0  1月 29 19:02 /dev/nvme0
319 brw-rw---- 1 root disk 259, 0  1月 29 19:02 /dev/nvme0n1
320 brw-rw---- 1 root disk 259, 1  1月 29 19:02 /dev/nvme0n1p1
321 brw-rw---- 1 root disk 259, 2  1月 29 19:02 /dev/nvme0n1p2
322 brw-rw---- 1 root disk 259, 3  1月 29 19:02 /dev/nvme0n1p3
```

```
[@goshun](master)~/misc/linux-device-file
> stat /dev/nvme0n1
  File: /dev/nvme0n1
  Size: 0               Blocks: 0          IO Block: 4096   block special file
Device: 5h/5d   Inode: 319         Links: 1     Device type: 103,0
Access: (0660/brw-rw----)  Uid: (    0/    root)   Gid: (    6/    disk)
Access: 2023-01-28 10:03:26.964000726 +0900
Modify: 2023-01-28 10:03:26.960000726 +0900
Change: 2023-01-28 10:03:26.960000726 +0900
 Birth: -
```

## Connect device driver and device file

### mknod

`mknod(1)` is a command to make a special file such as a character device file or a block device file. I made `/dev/read_write` using `sudo mknod /dev/read_write c 333 1` in [Example of device driver](#example-of-device-driver). [mknod(2)](https://man7.org/linux/man-pages/man2/mknod.2.html) is a system call used in `mknod(1)` and used to make a node on filesystems.

#### mknod in user space

Let's check how `mknod(2)` are called using `strace(1)`. I show the output of `strace mknod /dev/read_write c 333 1` below. Because `0x14d` is `333` in decimal,`mknod(2)` make an inode with `333` major number and `1` minor number.

As a side note, `mknod(2)` and `mknodat(2)` are almost the same. The only difference is `mknod(2)` takes a relative path, although `mknodat(2)` takes an absolute path.

```
# strace mknod /dev/read_write c 333 1
...
close(3)                                = 0
mknodat(AT_FDCWD, "/dev/read_write", S_IFCHR|0666, makedev(0x14d, 0x1)) = 0
close(1)                                = 0
close(2)                                = 0
exit_group(0)                           = ?
+++ exited with 0 +++
```

#### mknod in kernel space

`mknodat(2)` in kernel space starts from [do_mknodat](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3939-L3988). From now, I follow all code relating to the connection between a device file and a device driver. From now, the device is a character device, and the filesystem is ext4 for simplicity.

```
static int do_mknodat(int dfd, struct filename *name, umode_t mode,
        unsigned int dev)
```

[do_mknodat](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3939-L3988) calls `vfs_mknod` in [fs/namei.c#L3970-L3972](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3970-L3972) to make a character device or a block device.

```
        case S_IFCHR: case S_IFBLK:
            error = vfs_mknod(mnt_userns, path.dentry->d_inode,
                      dentry, mode, new_decode_dev(dev));
```

`vfs_mknod` is defined at [fs/namei.c#L3874-L3891](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3874-L3891).

```
/**
 * vfs_mknod - create device node or file
 * @mnt_userns: user namespace of the mount the inode was found from
 * @dir:    inode of @dentry
 * @dentry: pointer to dentry of the base directory
 * @mode:   mode of the new device node or file
 * @dev:    device number of device to create
 *
 * Create a device node or file.
 *
 * If the inode has been found through an idmapped mount the user namespace of
 * the vfsmount must be passed through @mnt_userns. This function will then take
 * care to map the inode according to @mnt_userns before checking permissions.
 * On non-idmapped mounts or if permission checking is to be performed on the
 * raw inode simply passs init_user_ns.
 */
int vfs_mknod(struct user_namespace *mnt_userns, struct inode *dir,
          struct dentry *dentry, umode_t mode, dev_t dev)
```

`vfs_mknod` calls `mknod` of dentry. Although implementation of `mknod` are different depending on filesystems, we follow `mknod` of `ext4` in this article. `vfs_mknod` calls `mknod` at [fs/namei.c#L3915](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3915).

```
    error = dir->i_op->mknod(mnt_userns, dir, dentry, mode, dev);
```

`mknod` of `ext4` is defined at [fs/ext4/namei.c#L4191](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/ext4/namei.c#L4191).

```
const struct inode_operations ext4_dir_inode_operations = {
    ...
    .mknod      = ext4_mknod,
    ...
};
```

`ext4_mknod` is defined at [fs/ext4/namei.c#L2830-L2862](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/ext4/namei.c#L2830-L2862). `init_special_inode` may have something to do with device initilization.

```
static int ext4_mknod(struct user_namespace *mnt_userns, struct inode *dir,
              struct dentry *dentry, umode_t mode, dev_t rdev)
{
    ...
        init_special_inode(inode, inode->i_mode, rdev);
    ...
}
```

For character devices, `def_chr_fops` is set at [fs/inode.c#L2291-L2309](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/inode.c#L2291-L2309).

```
void init_special_inode(struct inode *inode, umode_t mode, dev_t rdev)
{
    inode->i_mode = mode;
    if (S_ISCHR(mode)) {
        inode->i_fop = &def_chr_fops;
        inode->i_rdev = rdev;
    } else if (S_ISBLK(mode)) {
    ...
  }
}
EXPORT_SYMBOL(init_special_inode);
```

`def_chr_fops` is defined at [fs/char_dev.c#L447-L455](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/char_dev.c#L447-L455).

```
/*
 * Dummy default file-operations: the only thing this does
 * is contain the open that then fills in the correct operations
 * depending on the special file...
 */
const struct file_operations def_chr_fops = {
    .open = chrdev_open,
    .llseek = noop_llseek,
};
```

`chrdev_open` searches device driver in `kobj_lookup` defined at [fs/char_dev.c#L370-L424](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/char_dev.c#L370-L424).

```
/*
 * Called every time a character special file is opened
 */
static int chrdev_open(struct inode *inode, struct file *filp)
{
...
        kobj = kobj_lookup(cdev_map, inode->i_rdev, &idx);
...
}
```

Finally, we reached [kobj_map](https://github.com/akawashiro/linux/blob/7c8da299ff040d55f3e2163e6725cb1eef7155a9/drivers/base/map.c#L32-L66) which we have seen in `insmod`. [drivers/base/map.c#L95-L133](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/drivers/base/map.c#L95-L133) connects a device driver to a device file.

```
struct kobject *kobj_lookup(struct kobj_map *domain, dev_t dev, int *index)
{
    struct kobject *kobj;
    struct probe *p;
    unsigned long best = ~0UL;

retry:
    mutex_lock(domain->lock);
    for (p = domain->probes[MAJOR(dev) % 255]; p; p = p->next) {
        struct kobject *(*probe)(dev_t, int *, void *);
        struct module *owner;
        void *data;

        if (p->dev > dev || p->dev + p->range - 1 < dev)
            continue;
        if (p->range - 1 >= best)
            break;
        if (!try_module_get(p->owner))
            continue;
        owner = p->owner;
        data = p->data;
        probe = p->get;
        best = p->range - 1;
        *index = dev - p->dev;
        if (p->lock && p->lock(dev, data) < 0) {
            module_put(owner);
            continue;
        }
        mutex_unlock(domain->lock);
        kobj = probe(dev, index, data);
        /* Currently ->owner protects _only_ ->probe() itself. */
        module_put(owner);
        if (kobj)
            return kobj;
        goto retry;
    }
    mutex_unlock(domain->lock);
    return NULL;
}

```

At last, I confirm my understanding by patching the Linux kernel. I added a `printk` at [drivers/base/map.c#L114-L115](https://github.com/akawashiro/linux/blob/4aeb800558b98b2a39ee5d007730878e28da96ca/drivers/base/map.c#L114-L115), installed custom kernel and made a device driver just same as [Example of device driver](#example-of-device-driver).

```
> git diff --patch "device-file-experiment~1"
diff --git a/drivers/base/map.c b/drivers/base/map.c
index 83aeb09ca161..57037223932e 100644
--- a/drivers/base/map.c
+++ b/drivers/base/map.c
@@ -111,6 +111,8 @@ struct kobject *kobj_lookup(struct kobj_map *domain, dev_t dev, int *index)
                        break;
                if (!try_module_get(p->owner))
                        continue;
+
+               printk("%s:%d MAJOR(dev)=%u MINOR(dev)=%u\n", __FILE__, __LINE__, MAJOR(dev), MINOR(dev));
                owner = p->owner;
                data = p->data;
                probe = p->get;
```

I show the `dmesg -wH` result when `cat /dev/read_write`. You can see that `read_write_driver` is called when `cat(2)` open `/dev/read_write`.

```
# dmesg -wH
...
[ +18.898110] drivers/base/map.c:115 MAJOR(dev)=136 MINOR(dev)=2
[ +10.920752] drivers/base/map.c:115 MAJOR(dev)=136 MINOR(dev)=3
[  +9.170364] loop0: detected capacity change from 0 to 8
[  +1.212845] drivers/base/map.c:115 MAJOR(dev)=333 MINOR(dev)=1
[  +0.000010] read_write_driver - open was called!
[  +2.141643] read_write_driver - close was called!
```

## Reference

- [Understanding the Linux Kernel, 3rd Edition](https://www.oreilly.co.jp/books/9784873113135/)
- [Johannes4Linux/Linux_Driver_Tutorial](https://github.com/Johannes4Linux/Linux_Driver_Tutorial)
- [init_module(2) — Linux manual page](https://man7.org/linux/man-pages/man2/init_module.2.html)
- [linux kernel における insmod の裏側を確認](https://qiita.com/hon_no_mushi/items/9865febd245afd887d26)
- [https://github.com/torvalds/linux/tree/v6.1](https://github.com/torvalds/linux/tree/v6.1)
- [組み込み Linux デバイスドライバの作り方](https://qiita.com/iwatake2222/items/1fdd2e0faaaa868a2db2)
- [Linux のドライバの初期化が呼ばれる流れ](https://qiita.com/rarul/items/308d4eef138b511aa233)

## Contacts

If you find any bug in this article, please get in touch with me at [twitter.com/a_kawashiro](https://twitter.com/a_kawashiro). You can find other contact information at [https://akawashiro.github.io/](https://akawashiro.github.io/).
