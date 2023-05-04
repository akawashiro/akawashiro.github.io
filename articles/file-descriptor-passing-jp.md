# File Descriptor Passingの仕組み <!-- omit in toc -->

File Descriptor Passingとは、LinuxにおいてUNIX domain socketを使ってファイルディスクリプタをプロセス間でやり取りする手法です。ファイルディスクリプタを受け取る側にそのファイルを開く権限がない場合でも、そのファイルへの読み書きが行えるようになります。主にソフトウェアをセキュリティ的に強固にするために使われる手法であり、[gcs-fuse-csi-driver](https://github.com/GoogleCloudPlatform/gcs-fuse-csi-driver)[^1] などで使われています。

この記事では、File Descriptor Passingがどのように実現されるかを説明します。まず、前提知識であるファイルディスクリプタとUNIX domain socketを説明し、その後File Descriptor Passingの挙動を解説します。

## 目次 <!-- omit in toc -->
- [ファイルディスクリプタとは何か](#ファイルディスクリプタとは何か)
- [UNIX domain socketとは何か](#unix-domain-socketとは何か)
- [File Descriptor Passingの例](#file-descriptor-passingの例)
- [カーネル](#カーネル)
- [参考](#参考)
- [連絡先](#連絡先)

## ファイルディスクリプタとは何か
> UNIXのファイル記述子は、一種のケイパビリティである。sendmsg() システムコールを使うとプロセス間でファイル記述子をやり取りすることができる。つまり、UNIXのプロセスが持つファイル記述子テーブルは「ケイパビリティリスト (C-list)」の実例と見ることもできる。
- [ファイル記述子](https://ja.wikipedia.org/wiki/%E3%83%95%E3%82%A1%E3%82%A4%E3%83%AB%E8%A8%98%E8%BF%B0%E5%AD%90)

## UNIX domain socketとは何か

## File Descriptor Passingの例
冒頭で述べた等にFile Descriptor PassingとはUNIX domain socketを使ってファイルディスクリプタをプロセス間でやり取りする手法でした。File Descriptor Passingを使って、プロセス間でファイルディスクリプタを送受信する例を下に示します。長いので折りたたんでいます。この例は[FD passing for DRI.Next](https://keithp.com/blogs/fd-passing/)より引用、一部改変したものです。

<details>
<summary>file-descriptor-passing.c</summary>

```
/* 
 * Run this program with
 * gcc file-descriptor-passing.c -o file-descriptor-passing && ./file-descriptor-passing
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

ssize_t sock_fd_write(int sock, void *buf, ssize_t buflen, int fd) {
  ssize_t size;
  struct msghdr msg;
  struct iovec iov;
  union {
    struct cmsghdr cmsghdr;
    char control[CMSG_SPACE(sizeof(int))];
  } cmsgu;
  struct cmsghdr *cmsg;

  iov.iov_base = buf;
  iov.iov_len = buflen;

  msg.msg_name = NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  if (fd != -1) {
    msg.msg_control = cmsgu.control;
    msg.msg_controllen = sizeof(cmsgu.control);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;

    printf("sock_fd_write: passing fd %d\n", fd);
    *((int *)CMSG_DATA(cmsg)) = fd;
  } else {
    msg.msg_control = NULL;
    msg.msg_controllen = 0;
    printf("sock_fd_write: not passing fd\n");
  }

  size = sendmsg(sock, &msg, 0);

  if (size < 0)
    perror("sock_fd_write: sendmsg");
  return size;
}

ssize_t sock_fd_read(int sock, void *buf, ssize_t bufsize, int *fd) {
  ssize_t size;

  if (fd) {
    struct msghdr msg;
    struct iovec iov;
    union {
      struct cmsghdr cmsghdr;
      char control[CMSG_SPACE(sizeof(int))];
    } cmsgu;
    struct cmsghdr *cmsg;

    iov.iov_base = buf;
    iov.iov_len = bufsize;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsgu.control;
    msg.msg_controllen = sizeof(cmsgu.control);
    size = recvmsg(sock, &msg, 0);
    if (size < 0) {
      perror("recvmsg");
      exit(1);
    }
    cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
      if (cmsg->cmsg_level != SOL_SOCKET) {
        fprintf(stderr, "invalid cmsg_level %d\n", cmsg->cmsg_level);
        exit(1);
      }
      if (cmsg->cmsg_type != SCM_RIGHTS) {
        fprintf(stderr, "invalid cmsg_type %d\n", cmsg->cmsg_type);
        exit(1);
      }

      *fd = *((int *)CMSG_DATA(cmsg));
      printf("received fd %d\n", *fd);
    } else
      *fd = -1;
  } else {
    size = read(sock, buf, bufsize);
    if (size < 0) {
      perror("read");
      exit(1);
    }
  }
  return size;
}

void child(int sock) {
  int fd;
  char buf[16];
  ssize_t size;

  sleep(1);
  for (;;) {
    size = sock_fd_read(sock, buf, sizeof(buf), &fd);
    if (size <= 0)
      break;
    printf("Child: read %zd\n", size);
    if (fd != -1) {
      dprintf(fd, "Child: hello, world fd=%d pid=%d\n", fd, getpid());
      close(fd);
    }
  }
}

void parent(int sock) {
  ssize_t size;
  int i;
  int fd;

  fd = 1;
  size = sock_fd_write(sock, "1", 1, 1);
  printf("Parent: wrote %zd fd=%d pid=%d\n", size, fd, getpid());
}

int main(int argc, char **argv) {
  int sv[2];
  int pid;

  if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sv) < 0) {
    perror("socketpair");
    exit(1);
  }
  switch ((pid = fork())) {
  case 0:
    close(sv[0]);
    child(sv[1]);
    break;
  case -1:
    perror("fork");
    exit(1);
  default:
    close(sv[1]);
    parent(sv[0]);
    break;
  }
  return 0;
}
```
</details>

このプログラムは親と子の２つのプロセスを生成します。親プロセスは標準出力を指すファイルディスクリプタ(`fd 1`)をUnix domain socketで子プロセスに渡します。
```
ssize_t sock_fd_write(int sock, void *buf, ssize_t buflen, int fd) {
  /* 省略 */
  *((int *)CMSG_DATA(cmsg)) = fd;
  /* 省略 */
  size = sendmsg(sock, &msg, 0);
  /* 省略 */
}

void parent(int sock) {
  ssize_t size;
  int i;
  int fd;

  fd = 1;
  size = sock_fd_write(sock, "1", 1, 1);
  printf("Parent: wrote %zd fd=%d pid=%d\n", size, fd, getpid());
}
```

子プロセスはUnix domain socketから親プロセスの標準出力を指すファイルディスクリプタを受け取り、そのファイルディスクリプタにメッセージを書き込みます。
```
ssize_t sock_fd_read(int sock, void *buf, ssize_t bufsize, int *fd) {
  /* 省略 */
  size = recvmsg(sock, &msg, 0);
  /* 省略 */
  *fd = *((int *)CMSG_DATA(cmsg));
  /* 省略 */
  return size;
}

void child(int sock) {
  int fd;
  char buf[16];
  ssize_t size;

  sleep(1);
  for (;;) {
    size = sock_fd_read(sock, buf, sizeof(buf), &fd);
    if (size <= 0)
      break;
    printf("Child: read %zd\n", size);
    if (fd != -1) {
      dprintf(fd, "Child: hello, world fd=%d pid=%d\n", fd, getpid());
      close(fd);
    }
  }
}
```

このプログラムをコンパイル実行すると、子プロセスの書き込んだ `Child: hello, world fd=3 pid=135181` という文字列が親プロセスの標準出力に書き込まれ、画面に出力されます。
```
$ gcc file-descriptor-passing.c -o file-descriptor-passing
$ ./file-descriptor-passing 
sock_fd_write: passing fd 1
Parent: wrote 1 fd=1 pid=135180
$ received fd 3
Child: read 1
Child: hello, world fd=3 pid=135181
```

## カーネル
- https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/net/unix/af_unix.c#L1904
- https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/net/core/scm.c#L155-L161
- https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/net/core/scm.c#L65
- https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/file.c#L1154-L1193
- https://github.com/akawashiro/linux/blob/e1e12e81071431dbc6a93a6436c3997ee481a41a/fs/file.c#L604

## 参考
- [unix(7) — Linux manual page](https://man7.org/linux/man-pages/man7/unix.7.html)
  - SCM_RIGHTSの部分
- [FD passing for DRI.Next](https://keithp.com/blogs/fd-passing/)
- [https://blog.cloudflare.com/know-your-scm_rights/](https://blog.cloudflare.com/know-your-scm_rights/)
- "Socket"-level control message types https://elixir.bootlin.com/linux/v5.17.3/source/include/linux/socket.h#L163

## 連絡先
この記事に誤りがあった場合は[Twitter](https://twitter.com/a_kawashiro)等で連絡をください。修正します。その他の連絡先は [https://akawashiro.github.io/](https://akawashiro.github.io/) にあります。

[^1]: [Shingo Omuraさんのツイート](https://twitter.com/everpeace/status/1649211343428550661)