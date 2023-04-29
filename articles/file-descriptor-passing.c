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
      dprintf(fd, "Child: hello, world fd=%d\n", fd);
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
  printf("Parent: wrote %zd fd=%d\n", size, fd);
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
