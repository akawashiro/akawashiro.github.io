#include <cstdint>
#include <elf.h>
#include <fcntl.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/procfs.h>
#include <sys/user.h>
#include <unistd.h>

#include <string>

std::string ShowNT(uint32_t type) {
  if (type == NT_PRSTATUS) {
    return "NT_PRSTATUS";
  } else if (type == NT_PRFPREG) {
    return "NT_PRFPREG";
  } else if (type == NT_PRPSINFO) {
    return "NT_PRPSINFO";
  } else if (type == NT_TASKSTRUCT) {
    return "NT_TASKSTRUCT";
  } else if (type == NT_AUXV) {
    return "NT_AUXV";
  } else if (type == NT_FILE) {
    return "NT_FILE";
  } else if (type == NT_SIGINFO) {
    return "NT_SIGINFO";
  } else if (type == NT_X86_XSTATE) {
    return "NT_X86_XSTATE";
  } else {
    return "Unknown";
  }
}

int main(int argc, char const *argv[]) {
  if (argc != 2) {
    printf("Usage: %s <path to core file>\n", argv[0]);
  }

  int fd = open(argv[1], O_RDONLY);
  size_t size = lseek(fd, 0, SEEK_END);
  size_t mapped_size = (size + 0xfff) & ~0xfff;
  const char *head = reinterpret_cast<const char *>(
      mmap(NULL, mapped_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0));

  const Elf64_Ehdr *ehdr = reinterpret_cast<const Elf64_Ehdr *>(head);
  for (size_t pi = 0; pi < ehdr->e_phnum; pi++) {
    const Elf64_Phdr *phdr =
        (reinterpret_cast<const Elf64_Phdr *>(head + ehdr->e_phoff)) + pi;
    if (phdr->p_type == PT_NOTE) {
      size_t offset_in_note = 0;
      while (offset_in_note < phdr->p_filesz) {
        const char *note = head + phdr->p_offset + offset_in_note;
        int32_t name_size =
            (*reinterpret_cast<const int32_t *>(note) + 3) / 4 * 4;
        int32_t desc_size =
            (*(reinterpret_cast<const int32_t *>(note) + 1) + 3) / 4 * 4;
        uint32_t type = *(reinterpret_cast<const uint32_t *>(note) + 2);
        const char *name = note + 4 * 3;
        // printf("name_size: %x desc_size: %x name: %s type: %s\n", name_size,
        //        desc_size, name, ShowNT(type).c_str());

        if (type == NT_PRSTATUS) {
          const prstatus_t *prstatus =
              reinterpret_cast<const prstatus_t *>(note + 4 * 3 + name_size);
          printf("prstatus->pr_pid: %d\n", prstatus->pr_pid);
          const struct user_regs_struct *regs =
              reinterpret_cast<const struct user_regs_struct *>(
                  prstatus->pr_reg);
          printf("regs->rip: %llx\n", regs->rip);
        }

        offset_in_note += 4 * 3 + name_size + desc_size;
        // printf("phdr->p_filesz : %lx offset_in_note: %lx\n", phdr->p_filesz,
        //        offset_in_note);
      }
    }
  }
  return 0;
}
