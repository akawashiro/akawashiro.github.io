# Linuxにおけるデバイスファイルの仕組み <!-- omit in toc -->
Linuxにおけるデバイスファイルはデバイスをファイルという概念を通して扱えるようにしたものです。デバイスファイルは通常のファイルと同様に読み書きを行うことができます。しかし実際には、その読み書きはデバイスドライバを通じてデバイスの制御に変換されます。

この記事では、デバイスファイルへの読み書きがどのようにデバイスの制御に変換されるのかを説明します。デバイスファイルはデバイスドライバとファイルの2つのコンポーネントに依存したものであるので、最初にデバイスドライバ、次にファイルについて説明し、最後にデバイスファイルがどのようにデバイスドライバと結び付けられるかを解説します。

この記事の内容は主に[詳解 Linuxカーネル 第3版](https://www.oreilly.co.jp/books/9784873113135/)及び[https://github.com/torvalds/linux/tree/v6.1](https://github.com/torvalds/linux/tree/v6.1)によります。

# 目次 <!-- omit in toc -->
- [デバイスドライバ](#デバイスドライバ)
	- [デバイスドライバの実例](#デバイスドライバの実例)
	- [`read_write.c` からわかること](#read_writec-からわかること)
	- [insmod](#insmod)
		- [insmodのユーザ空間での処理](#insmodのユーザ空間での処理)
		- [insmodのカーネル空間での処理](#insmodのカーネル空間での処理)
- [ファイル](#ファイル)
	- [VFS(Virtual File System)](#vfsvirtual-file-system)
	- [inode](#inode)
	- [普通のファイルのinode](#普通のファイルのinode)
		- [デバイスファイルのinode](#デバイスファイルのinode)
- [デバイスドライバとファイルの接続](#デバイスドライバとファイルの接続)
	- [mknod](#mknod)
		- [mknodのユーザ空間での処理](#mknodのユーザ空間での処理)
		- [mknodのカーネル空間での処理](#mknodのカーネル空間での処理)
- [参考](#参考)

# デバイスドライバ
デバイスドライバとはカーネルルーチンの集合です。デバイスドライバは後で説明するVirtual File System(VFS)の各オペレーションをデバイス固有の関数に結びつけます。

## デバイスドライバの実例
デバイスドライバを作って実際に動かしてみます。以下のような `read_write.c` と `Makefile` を用意します。この２つは[Johannes4Linux/Linux_Driver_Tutorial/03_read_write](https://github.com/Johannes4Linux/Linux_Driver_Tutorial/tree/main/03_read_write)を一部改変したものです。

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

これをビルドしてインストールし、デバイスファイルを作成すると`A` が無限に読み出されるデバイスファイルができます。
```
$ make
$ sudo insmod read_write.ko
$ sudo mknod /dev/read_write c 333 1
$ cat /dev/read_write
AAAAAA...
```

## `read_write.c` からわかること
`read_write.c`からは次のことがわかります。
- このデバイスドライバのmajor番号は`333`である。
- デバイスドライバは単なる関数の集合である。
- `open(2)`と`myDevice_open`、`release(2)`と`driver_close`、`read(2)`と`myDevice_read`、`write(2)`と`driver_write`が対応している。

以上から`cat /dev/read_write` は このデバイスドライバの `driver_read` を呼び出すので `A` が無限に読み出されます。なお、major番号の`333`に意味はありません。

## insmod
`insmod(8)` はLinuxカーネルにカーネルモジュールを挿入するコマンドです。この章では `sudo insmod read_write.ko` がどのように`read_write.ko` をカーネルに登録するかを確認します。
### insmodのユーザ空間での処理
`strace(1)`を使って`insmod(8)` が呼び出すシステムコールを確認すると`finit_module(2)`が呼ばれています。
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
### insmodのカーネル空間での処理
`finit_module(2)` はLinuxカーネル内の [kernel/module/main.c#29l6](https://github.com/akawashiro/linux/blob/4aeb800558b98b2a39ee5d007730878e28da96ca/kernel/module/main.c#L2916) で定義されています。
```
SYSCALL_DEFINE3(finit_module, int, fd, const char __user *, uargs, int, flags)
```

ここから追っていくと [do_init_module関数](https://github.com/akawashiro/linux/blob/4aeb800558b98b2a39ee5d007730878e28da96ca/kernel/module/main.c#L2440) で初期化処理を行っていることがわかります。
```
/*
 * This is where the real work happens.
 *
 * Keep it uninlined to provide a reliable breakpoint target, e.g. for the gdb
 * helper command 'lx-symbols'.
 */
static noinline int do_init_module(struct module *mod)
```

更に追っていくと `insmod(8)` を行った際には [ret = do_one_initcall(mod->init);](https://github.com/akawashiro/linux/blob/4aeb800558b98b2a39ee5d007730878e28da96ca/kernel/module/main.c#L2455) 経由でデバイスドライバ内の`ModuleInit` が呼び出されることがわかります。
```
    /* Start the module */
	if (mod->init != NULL)
		ret = do_one_initcall(mod->init);
	if (ret < 0) {
		goto fail_free_freeinit;
	}
```

`printk` を駆使して調べると、この `mod->init` は [__apply_relocate_add](https://github.com/akawashiro/linux/blob/7c8da299ff040d55f3e2163e6725cb1eef7155a9/arch/x86/kernel/module.c#L131-L220)で設定されていました。この関数は名前から推測できるようにカーネルモジュール内の再配置を行う関数です。再配置情報と`mod->init`の関係については調べきれなかったため今後の課題とします。
```
static int __apply_relocate_add(Elf64_Shdr *sechdrs,
		   const char *strtab,
		   unsigned int symindex,
		   unsigned int relsec,
		   struct module *me,
		   void *(*write)(void *dest, const void *src, size_t len))
```

`mod->init`経由で呼び出された`ModuleInit`は `register_chrdev` を呼び出し、最終的にカーネル内の [__register_chrdev](https://github.com/akawashiro/linux/blob/7c8da299ff040d55f3e2163e6725cb1eef7155a9/fs/char_dev.c#L247-L302) を経由して [kobj_map](https://github.com/akawashiro/linux/blob/7c8da299ff040d55f3e2163e6725cb1eef7155a9/drivers/base/map.c#L32-L66)に到達します。[kobj_map](https://github.com/akawashiro/linux/blob/7c8da299ff040d55f3e2163e6725cb1eef7155a9/drivers/base/map.c#L32-L66) は [cdev_map](https://github.com/akawashiro/linux/blob/7c8da299ff040d55f3e2163e6725cb1eef7155a9/fs/char_dev.c#L28) にデバイスドライバを登録します。
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


# ファイル

## VFS(Virtual File System)
VFSとは標準的なUNIXファイルシステムのすべてのシステムコールを取り扱う、カーネルが提供するソフトウェアレイヤです。提供されているシステムコールとして`open(2)`、`close(2)`、`write(2)` 等があります。このレイヤがあるので、ユーザは`ext4`、`NFS`、`proc` などの全く異なるシステムを同じプログラムでで取り扱うことができます。

例えば`cat(1)` は `cat /proc/self/maps` も `cat ./README.md`も可能ですが、前者はメモリ割付状態を、後者ははディスク上のファイルの中身を読み出しており、全く異なるシステムを同じインターフェイスで扱っています。

LinuxにおいてVFSは構造体と関数ポインタを使ったオブジェクト指向で実装されていて、関数ポインタを持つ構造体がオブジェクトとして使われています。

## inode
inodeオブジェクトはVFSにおいて「普通のファイル」に対応するオブジェクトです。定義は [fs.h](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/include/linux/fs.h#L588-L703) にあります。inodeオブジェクト以外の他のオブジェクトとして、ファイルシステムそのものの情報を保持するスーパーブロックオブジェクト、オープンされているファイルとプロセスのやり取りの情報を保持するファイルオブジェクト、ディレクトリに関する情報を保持するdエントリオブジェクトがあります。

```
struct inode {
	umode_t			i_mode;
	unsigned short		i_opflags;
	kuid_t			i_uid;
	kgid_t			i_gid;
	unsigned int		i_flags;
  ...
  union {
		struct pipe_inode_info	*i_pipe;
		struct cdev		*i_cdev;
		char			*i_link;
		unsigned		i_dir_seq;
	};
  ...
};
```

## 普通のファイルのinode
`stat(1)`を使うとファイルのiノード情報を表示することができ、`struct inode` と対応した内容が表示されます。
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

### デバイスファイルのinode
デバイスファイルのiノード情報も表示してみます。`ls -il`で表示したときに先頭に`c` がついているとキャラクタデバイス、`b` がついているとブロックデバイスです。
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

# デバイスドライバとファイルの接続
## mknod
`mknod(1)` はブロックデバイスファイルもしくはキャラクタデバイスファイルを作るためのコマンドです。[デバイスドライバの実例](#デバイスドライバの実例) では `sudo mknod /dev/read_write c 333 1` を使ってデバイスファイル `/dev/read_write` を作成しました。[mknod(2)](https://man7.org/linux/man-pages/man2/mknod.2.html)はこれに対応するシステムコールであり、ファイルシステム上にノード(おそらくinodeのこと)を作るために使われます。

### mknodのユーザ空間での処理
`strace(1)`を使って`mknod(2)`がどのように呼び出されているかを調べます。`0x14d`は10進で`333`なので `/dev/read_write` にメジャー番号と`333`、マイナー番号を`1`を指定してinodeを作っていることがわかる。ちなみに、`mknod`と`mnknodat`はパス名が相対パスになるかどうかという違いです。
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
### mknodのカーネル空間での処理
`mknodat`の本体は [do_mknodat](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3939-L3988) にあります。ここからデバイスファイルとデバイスドライバがどのように接続されるかを追っていきます。ここではデバイスはキャラクタデバイス、ファイルシステムはext4であるとします。
```
static int do_mknodat(int dfd, struct filename *name, umode_t mode,
		unsigned int dev)
```


キャラクタデバイス、ブロックデバイスを扱う場合、[do_mknodat](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3939-L3988) は[fs/namei.c#L3970-L3972](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3970-L3972)で `vfs_mknod` を呼び出します。
```
		case S_IFCHR: case S_IFBLK:
			error = vfs_mknod(mnt_userns, path.dentry->d_inode,
					  dentry, mode, new_decode_dev(dev));
```

`vfs_mknod`の定義は[fs/namei.c#L3874-L3891](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3874-L3891)にあります。
```
/**
 * vfs_mknod - create device node or file
 * @mnt_userns:	user namespace of the mount the inode was found from
 * @dir:	inode of @dentry
 * @dentry:	pointer to dentry of the base directory
 * @mode:	mode of the new device node or file
 * @dev:	device number of device to create
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

`vfs_mknod`はdエントリの `mknod` を呼びます。ファイルシステムごとに `mknod`の実装が異なるが今回は`ext4`のものを追ってみます。`vfs_mknod` は
[fs/namei.c#L3915](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3915) で `mknod` を呼んでいます。
```
	error = dir->i_op->mknod(mnt_userns, dir, dentry, mode, dev);
```

`ext4` の `mknod` は[fs/ext4/namei.c#L4191](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/ext4/namei.c#L4191)で定義されています。
```
const struct inode_operations ext4_dir_inode_operations = {
	...
	.mknod		= ext4_mknod,
	...
};
```

`ext4_mknod` の本体はここにあり、[fs/ext4/namei.c#L2830-L2862](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/ext4/namei.c#L2830-L2862)の `init_special_inode` がデバイスに関係していそうに見えます。
```
static int ext4_mknod(struct user_namespace *mnt_userns, struct inode *dir,
		      struct dentry *dentry, umode_t mode, dev_t rdev)
{
	...
		init_special_inode(inode, inode->i_mode, rdev);
	...
}
```

キャラクタデバイスの場合は [fs/inode.c#L2291-L2309](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/inode.c#L2291-L2309) で`def_chr_fops` が設定されています。
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

`def_chr_fops`は[fs/char_dev.c#L447-L455](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/char_dev.c#L447-L455)で定義されています。
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

`chrdev_open`が怪しいので定義を見ると [fs/char_dev.c#L370-L424](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/char_dev.c#L370-L424) の`kobj_lookup` でドライバを探していそうです。
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

`insmod`のときに見た [kobj_map](https://github.com/akawashiro/linux/blob/7c8da299ff040d55f3e2163e6725cb1eef7155a9/drivers/base/map.c#L32-L66)と同じファイルにたどりついたのでここで間違いなさそうです。[drivers/base/map.c#L95-L133](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/drivers/base/map.c#L95-L133)でファイルにデバイスドライバを紐付けています。
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

最後に実際にカーネルにパッチを当てて確認してみましょう。[drivers/base/map.c#L114-L115](https://github.com/akawashiro/linux/blob/4aeb800558b98b2a39ee5d007730878e28da96ca/drivers/base/map.c#L114-L115) にログ出力を足してカーネルをインストールして、デバイスファイルを [デバイスドライバの実例](#デバイスドライバの実例) と同様に作成します。
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

`cat /dev/read_write` したときの `dmesg -wH` の様子が以下です。`cat(2)`が`/dev/read_write` を開いたときに対応するデバイスドライバが検索されて `read_write_driver` が呼ばれていることがわかります。
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

# 参考
- [詳解 Linuxカーネル 第3版](https://www.oreilly.co.jp/books/9784873113135/)
- [init_module(2) — Linux manual page](https://man7.org/linux/man-pages/man2/init_module.2.html)
- [linux kernelにおけるinsmodの裏側を確認](https://qiita.com/hon_no_mushi/items/9865febd245afd887d26)
- [https://github.com/torvalds/linux/tree/v6.1](https://github.com/torvalds/linux/tree/v6.1)
- [組み込みLinuxデバイスドライバの作り方](https://qiita.com/iwatake2222/items/1fdd2e0faaaa868a2db2)
- [Linuxのドライバの初期化が呼ばれる流れ](https://qiita.com/rarul/items/308d4eef138b511aa233)
