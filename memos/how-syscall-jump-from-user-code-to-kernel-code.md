---
title: syscall 命令がユーザ空間からカーネル空間にジャンプする仕組み
layout: default
---

# MSR_LSTAR レジスタ
[arch/x86/kernel/cpu/common.c#L2029](https://github.com/akawashiro/linux/blob/0c3836482481200ead7b416ca80c68a29cfdaabd/arch/x86/kernel/cpu/common.c#L2029) で`MSR_LSTAR` というレジスタに `entry_SYSCALL_64` 関数のアドレスを登録すると、`syscall` 命令の実行時にこのアドレスにジャンプする。`MSR_LSTAR` がどういう作用を持つかは[Intel® 64 and IA-32 Architectures Software Developer's  Manual Volume 4: Model-specific Registers](https://cdrdv2.intel.com/v1/dl/getContent/671098)に記述がある。
<img src="./IA32_LSTAR.png" width="50%">

# 実際のコードの流れ
1. [arch/x86/kernel/cpu/common.c#L2029](https://github.com/akawashiro/linux/blob/0c3836482481200ead7b416ca80c68a29cfdaabd/arch/x86/kernel/cpu/common.c#L2029)
  - `wrmsrl(MSR_LSTAR, (unsigned long)entry_SYSCALL_64);`
1. [arch/x86/entry/entry_64.S#L49-L170](https://github.com/akawashiro/linux/blob/0c3836482481200ead7b416ca80c68a29cfdaabd/arch/x86/entry/entry_64.S#L49-L170)
  - `syscall` 命令で最初に飛んでくる`entry_SYSCALL_64` 関数
1. [arch/x86/entry/common.c#L75-L130](https://github.com/akawashiro/linux/blob/0c3836482481200ead7b416ca80c68a29cfdaabd/arch/x86/entry/common.c#L75-L130)
  - `do_syscall_64`
1. [arch/x86/entry/common.c#L42](https://github.com/akawashiro/linux/blob/0c3836482481200ead7b416ca80c68a29cfdaabd/arch/x86/entry/common.c#L42)
  - `do_syscall_64`
1. [arch/x86/entry/syscall_64.c#L29-L31](https://github.com/akawashiro/linux/blob/0c3836482481200ead7b416ca80c68a29cfdaabd/arch/x86/entry/syscall_64.c#L29-L31)
  - `x64_sys_call`。巨大な `switch` を生成している。
1. [fs/read_write.c#L652-L656](https://github.com/akawashiro/linux/blob/0c3836482481200ead7b416ca80c68a29cfdaabd/fs/read_write.c#L652-L656)
  - `SYSCALL_DEFINE3(write, unsigned int, fd, const char __user *, buf, size_t, count)` が実際にシステムコールを行うところ。
