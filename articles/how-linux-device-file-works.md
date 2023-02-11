# Linuxにおけるデバイスファイルの仕組み <!-- omit in toc -->
Linuxにおけるデバイスファイルとはデバイスをファイルという概念を通して扱えるようにしたものである。デバイスファイルは通常のファイルと同様に読み書きを行うことができる。しかし、実際には、その読み書きは例えばDMA等のデバイスとの情報のやり取りに変換される。デバイスファイルの例として、`/dev/nvme0` や `/dev/kvm` 等がある。

本稿では、デバイスファイルへの読み書きがどのようにデバイスへの制御に変換されるのかを述べる。ほとんどの内容は[詳解 Linuxカーネル 第3版](https://www.oreilly.co.jp/books/9784873113135/)の12章、13章に依る。参照しているLinux Kernelのソースコードのgitハッシュは`commit 830b3c68c1fb1e9176028d02ef86f3cf76aa2476 (v6.1)` である。

# 目次 <!-- omit in toc -->
- [デバイスドライバ](#デバイスドライバ)
	- [デバイスドライバを作ってみる](#デバイスドライバを作ってみる)
	- [`MyDeviceDriver.c` からわかること](#mydevicedriverc-からわかること)
- [ファイル](#ファイル)
	- [VFS(Virtual Filesytem Switch)](#vfsvirtual-filesytem-switch)
	- [inode](#inode)
	- [普通のファイルのinode](#普通のファイルのinode)
		- [デバイスファイルのinode](#デバイスファイルのinode)
- [デバイスドライバとファイルの接続](#デバイスドライバとファイルの接続)
	- [mknod](#mknod)
		- [mknodのユーザ空間での処理](#mknodのユーザ空間での処理)
		- [mknodのカーネル空間での処理](#mknodのカーネル空間での処理)
- [参考](#参考)

# デバイスドライバ
デバイスドライバとはカーネルルーチンの集合である。実際 [myDeviceDriver.c](./myDeviceDriver.c) を見ると単なる関数の集合であることがわかる。

```
/* myDeviceDriver.c
   https://qiita.com/iwatake2222/items/1fdd2e0faaaa868a2db2 よりコピー
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/uaccess.h>

#define DRIVER_NAME "MyDevice_NAME"
#define DRIVER_MAJOR 63

/* open時に呼ばれる関数 */
static int myDevice_open(struct inode *inode, struct file *file)
{
    printk("myDevice_open\n");
    return 0;
}

/* close時に呼ばれる関数 */
static int myDevice_close(struct inode *inode, struct file *file)
{
    printk("myDevice_close\n");
    return 0;
}

/* read時に呼ばれる関数 */
static ssize_t myDevice_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    printk("myDevice_read\n");
    buf[0] = 'A';
    return 1;
}

/* write時に呼ばれる関数 */
static ssize_t myDevice_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    printk("myDevice_write\n");
    return 1;
}

/* 各種システムコールに対応するハンドラテーブル */
struct file_operations s_myDevice_fops = {
    .open    = myDevice_open,
    .release = myDevice_close,
    .read    = myDevice_read,
    .write   = myDevice_write,
};

/* ロード(insmod)時に呼ばれる関数 */
static int myDevice_init(void)
{
    printk("myDevice_init\n");
    /* ★ カーネルに、本ドライバを登録する */
    register_chrdev(DRIVER_MAJOR, DRIVER_NAME, &s_myDevice_fops);
    return 0;
}

/* アンロード(rmmod)時に呼ばれる関数 */
static void myDevice_exit(void)
{
    printk("myDevice_exit\n");
    unregister_chrdev(DRIVER_MAJOR, DRIVER_NAME);
}

module_init(myDevice_init);
module_exit(myDevice_exit);

MODULE_LICENSE("GPL");
```

## デバイスドライバを作ってみる
[組み込みLinuxデバイスドライバの作り方 (2)](https://qiita.com/iwatake2222/items/580ec7db2e88beeac3de)より。
```
> make # カーネルモジュールのビルド
> sudo insmod MyDeviceModule.ko 
> cat /proc/devices | grep 63
 63 MyDevice_NAME
> sudo mknod /dev/myDevice c 63 1 # デバイスファイルの作成
> file /dev/myDevice 
/dev/myDevice: character special (63/1)
> sudo chmod 666 /dev/myDevice # 一般ユーザも書き込み可にしておく
> echo "a" > /dev/myDevice # write(3)してみる
> sudo dmesg # カーネルのログを確認
[ 8969.738033] myDevice_init
[ 9160.327418] myDevice_open
[ 9160.327426] myDevice_write
[ 9160.327427] myDevice_write
[ 9160.327432] myDevice_close
```

## `MyDeviceDriver.c` からわかること
デバイスドライバは単なる関数の集合である。`open(2)`は`myDevice_open`を、`read(2)`は`myDevice_read`を呼び出す。また、`static` キーワードがついているのでリンク時にシンボルが外に公開されず、すべての関数は `s_myDevice_fops` 経由で呼び出される。

デバイスドライバはメジャー番号とマイナー番号を持ち、`MyDeviceDriver.c`の場合は`#define DRIVER_MAJOR 63`である。この番号はデバイスファイル作成時に `sudo mknod /dev/myDevice c 63 1` のように使われる。

# ファイル

## VFS(Virtual Filesytem Switch)
VFSとは標準的なUNIXファイルシステムのすべてのシステムコールを取り扱う、カーネルが提供するソフトウェアレイヤである。提供されているシステムコールとして`open(2)`、`close(2)`、`write(2)` 等がある。このレイヤがあるので、ユーザは`ext4`、`NFS`、`proc` などの全く異なるシステムを同じプログラムでで取り扱うことができる。例えば`cat(1)` は `cat /proc/self/maps` も `cat ./README.md`も可能だが、前者はメモリ割付状態を読み出しており、後者のファイル読み出しとはやっていることが本質的に異なる。

```
[@goshun](v6.1)~/linux
> cat /proc/self/maps | head
55b048a03000-55b048a05000 r--p 00000000 fd:01 10879784                   /usr/bin/cat
55b048a05000-55b048a09000 r-xp 00002000 fd:01 10879784                   /usr/bin/cat
55b048a09000-55b048a0b000 r--p 00006000 fd:01 10879784                   /usr/bin/cat
55b048a0b000-55b048a0c000 r--p 00007000 fd:01 10879784                   /usr/bin/cat
55b048a0c000-55b048a0d000 rw-p 00008000 fd:01 10879784                   /usr/bin/cat
55b04a820000-55b04a841000 rw-p 00000000 00:00 0                          [heap]
7fe5c1a11000-7fe5c208a000 r--p 00000000 fd:01 10881441                   /usr/lib/locale/locale-archive
7fe5c208a000-7fe5c208d000 rw-p 00000000 00:00 0 
7fe5c208d000-7fe5c20b5000 r--p 00000000 fd:01 10884115                   /usr/lib/x86_64-linux-gnu/libc.so.6
7fe5c20b5000-7fe5c224a000 r-xp 00028000 fd:01 10884115                   /usr/lib/x86_64-linux-gnu/libc.so.6
```

LinuxにおいてVFSはC言語を使った疑似オブジェクト志向で実装されている。つまり、関数ポインタを持つ構造体がオブジェクトとして使われている。

## inode
inodeオブジェクトとはとはVFSにおいて「普通のファイル」に対応するオブジェクトである。定義は [fs.h](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/include/linux/fs.h#L588-L703) にある。他のオブジェクトとして、ファイルシステムそのものの情報を保持するスーパーブロックオブジェクト、オープンされているファイルとプロセスのやり取りの情報を保持するファイルオブジェクト、ディレクトリに関する情報を保持するdエントリオブジェクトがある。

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
`stat 1`を使うとファイルのiノード情報を表示することができる。`struct inode` と対応していることがわかる。
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

```
[@goshun](master)~/misc/linux-device-file
> stat --file-system README.md
  File: "README.md"
    ID: 968d19c9b6fe93c Namelen: 255     Type: ext2/ext3
Block size: 4096       Fundamental block size: 4096
Blocks: Total: 239511336  Free: 63899717   Available: 51714783
Inodes: Total: 60907520   Free: 52705995
```

### デバイスファイルのinode
先頭に`c` がついているとキャラクタデバイス、`b` がついているとブロックデバイス。
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
### mknodのユーザ空間での処理
先ほど `sudo mknod /dev/myDevice c 63 1` を使ってデバイスファイル `/dev/myDevice` を作成した。このとき使ったのは [mknod(1)](https://man7.org/linux/man-pages/man1/mknod.1.html)、つまりユーザ向けのコマンドだった。[mknod(2)](https://man7.org/linux/man-pages/man2/mknod.2.html)はこれに対応するシステムコールであり、ファイルシステム上にノード(おそらくinodeのこと)を作るために使われる。
> The system call mknod() creates a filesystem node (file, device special file, or named pipe) named pathname, with attributes specified by mode and dev.

`strace(1)`を使って`mknod(2)`がどのように呼び出されているかを調べる。フルの出力結果は [ここ](https://gist.github.com/akawashiro/4a58ac86873843fa4c1a58d3cf7d13ec) にある。`0x3f`は10進で`63`なので `/dev/myDevice` にメジャー番号と`63`、マイナー番号を`1`を指定してinodeを作っていることがわかる。ちなみに、`mknod`と`mnknodat`はパス名が相対パスになるかどうかという違いしかない。
```
> sudo strace mknod /dev/myDevice c 63 1
...
mknodat(AT_FDCWD, "/dev/myDevice", S_IFCHR|0666, makedev(0x3f, 0x1)) = 0
...
```
### mknodのカーネル空間での処理
`mknodat`の本体は [do_mknodat](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3939-L3988) にある。ここからデバイスファイルとデバイスドライバがどのように接続されるかを追う。ここではデバイスはキャラクタデバイス、ファイルシステムはext4であるとする。

キャラクタデバイス、ブロックデバイスの場合は `vfs_mknod` が呼ばれる。
https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3970-L3972
```
		case S_IFCHR: case S_IFBLK:
			error = vfs_mknod(mnt_userns, path.dentry->d_inode,
					  dentry, mode, new_decode_dev(dev));
```

`vfs_mknod`の定義はここにある。
https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3874-L3891
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

`vfs_mknod`はdエントリの `mknod` を呼ぶ。ファイルシステムごとに `mknod`の実装が異なるが今回は`ext4`のものを追ってみる。これは僕のマシンが`ext4`を使っていたためである。ちなみに、`df -T` でどのファイルシステムを使っているか調べられる。
https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/namei.c#L3915
```
	error = dir->i_op->mknod(mnt_userns, dir, dentry, mode, dev);
```

`ext4` の `mknod` はここで定義されている。
https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/ext4/namei.c#L4191
```
const struct inode_operations ext4_dir_inode_operations = {
	...
	.mknod		= ext4_mknod,
	...
};
```

`ext4_mknod` の本体はここにある。`init_special_inode` というのがデバイスに関係していそうに見える。
https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/ext4/namei.c#L2830-L2862
```
static int ext4_mknod(struct user_namespace *mnt_userns, struct inode *dir,
		      struct dentry *dentry, umode_t mode, dev_t rdev)
{
	...
		init_special_inode(inode, inode->i_mode, rdev);
	...
}
```

キャラクタデバイスの場合は `def_chr_fops` が設定されている。
https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/inode.c#L2291-L2309
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

`def_chr_fops`はこんな定義になっていた。 https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/char_dev.c#L447-L455
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

`chrdev_open`が怪しいので定義を見ると `kobj_lookup` でドライバを探していそう。
https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/char_dev.c#L370-L424
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

`MAJOR`とか出てくるのでおそらくここで間違いない。ここでファイルにデバイスドライバを紐付けている。
https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/drivers/base/map.c#L95-L133
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

実際にカーネルにパッチを当てて確認する。
[drivers/base/map.c#L114-L115](https://github.com/akawashiro/linux/blob/4aeb800558b98b2a39ee5d007730878e28da96ca/drivers/base/map.c#L114-L115)
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
![Open myDevice](./open-myDevice.png)
![dmesg when open myDevice](./dmesg-when-open-myDevice.png)

# 参考
- [詳解 Linuxカーネル 第3版](https://www.oreilly.co.jp/books/9784873113135/)
- [https://github.com/torvalds/linux/tree/v6.1](https://github.com/torvalds/linux/tree/v6.1)
- [組み込みLinuxデバイスドライバの作り方](https://qiita.com/iwatake2222/items/1fdd2e0faaaa868a2db2)