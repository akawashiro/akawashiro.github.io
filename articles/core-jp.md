
# コアダンプファイル

## コアダンプファイルとは何?

- プログラムがクラッシュしたときにでるやつ
```
./run.sh: line 13: 383833 Segmentation fault      (core dumped) ./make_core
```
- [man 5 core](https://linuxjm.osdn.jp/html/LDP_man-pages/man5/core.5.html) より抜粋
> ある種のシグナルを受けた場合のデフォルトのアクションは、 プロセスを終了し (terminate)、 コアダンプファイル (core dump file) を生成することである。コアダンプファイルは、ディスク上に生成される 終了時のプロセスのメモリーイメージを内容とするファイルである。 このイメージをデバッガ (例えば gdb(1)) に読み込んで、 プログラムが終了した時点のプログラムの状態を検査することができる。 どのシグナルを受けたときにプロセスがコアダンプを生成するかのリストは signal(7) に書かれている。
- 使い方
  - `gdb <プログラムへのパス> <コアダンプファイルへのパス>`

## gdb の限界
- `gdb` は壊れたプログラムを処理できない
  - sold の出力結果を入れると `gdb` 自身のコアができる
- `gdb` を使わずにコアダンプファイルが読みたいので読んでみました

## コアダンプファイルのフォーマット
- コアダンプファイルはただのELFファイル
- ELFヘッダの `e_type` フィールドが `ET_CORE` になっている
- プログラムヘッダテーブルは
  - `pt_type == PT_LOAD` のものはクラッシュしたときのメモリの内容を表す
  - `pt_type == PT_NOTE` のものはレジスタの値やプロセスの情報が入っている

## デモ

### コアダンプファイルを生成するためのプログラム
- 範囲外アクセスで コアダンプファイルを生成する C のプログラムをつくる。
- 解析を簡単にするため、範囲外アクセスの前に `/proc/self/maps` を出力しておく。

```c
#include <stdint.h>
#include <stdio.h>

uint64_t a = 0xaaaabbbbccccdddd;

int main() {
  // Print all lines in /proc/self/maps
  FILE *fp = fopen("/proc/self/maps", "r");
  char buf[0x1000];
  while (fgets(buf, sizeof(buf), fp)) {
    printf("%s", buf);
  }
  fclose(fp);

  for (size_t i = 0; i < 0x100000; i++) {
    *(&a + i) = 0xdeadbeefdeadbeef;
  }
}
```

- コンパイルする。

```bash
gcc -o ./make_core ./make_core.c
```

### コアダンプファイルを生成
- コアダンプファイルの生成場所を指定しておく。
- コアダンプファイルのサイズ制限を外す。
- ASLR は切っておく。

```bash
$ sudo bash -c 'echo core.%t > /proc/sys/kernel/core_pattern'
$ ulimit -c unlimited
$ echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
$ ./make_core
555555554000-555555555000 r--p 00000000 103:01 44838307                  /home/akira/ghq/github.com/akawashiro/akawashiro.github.io/articles/core/make_core
555555555000-555555556000 r-xp 00001000 103:01 44838307                  /home/akira/ghq/github.com/akawashiro/akawashiro.github.io/articles/core/make_core
555555556000-555555557000 r--p 00002000 103:01 44838307                  /home/akira/ghq/github.com/akawashiro/akawashiro.github.io/articles/core/make_core
555555557000-555555558000 r--p 00002000 103:01 44838307                  /home/akira/ghq/github.com/akawashiro/akawashiro.github.io/articles/core/make_core
555555558000-555555559000 rw-p 00003000 103:01 44838307                  /home/akira/ghq/github.com/akawashiro/akawashiro.github.io/articles/core/make_core
555555559000-55555557a000 rw-p 00000000 00:00 0                          [heap]
7ffff7c00000-7ffff7c28000 r--p 00000000 103:01 2386144                   /usr/lib/x86_64-linux-gnu/libc.so.6
7ffff7c28000-7ffff7dbd000 r-xp 00028000 103:01 2386144                   /usr/lib/x86_64-linux-gnu/libc.so.6
7ffff7dbd000-7ffff7e15000 r--p 001bd000 103:01 2386144                   /usr/lib/x86_64-linux-gnu/libc.so.6
7ffff7e15000-7ffff7e19000 r--p 00214000 103:01 2386144                   /usr/lib/x86_64-linux-gnu/libc.so.6
7ffff7e19000-7ffff7e1b000 rw-p 00218000 103:01 2386144                   /usr/lib/x86_64-linux-gnu/libc.so.6
7ffff7e1b000-7ffff7e28000 rw-p 00000000 00:00 0
7ffff7fa5000-7ffff7fa8000 rw-p 00000000 00:00 0
7ffff7fbb000-7ffff7fbd000 rw-p 00000000 00:00 0
7ffff7fbd000-7ffff7fc1000 r--p 00000000 00:00 0                          [vvar]
7ffff7fc1000-7ffff7fc3000 r-xp 00000000 00:00 0                          [vdso]
7ffff7fc3000-7ffff7fc5000 r--p 00000000 103:01 2385802                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7ffff7fc5000-7ffff7fef000 r-xp 00002000 103:01 2385802                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7ffff7fef000-7ffff7ffa000 r--p 0002c000 103:01 2385802                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7ffff7ffb000-7ffff7ffd000 r--p 00037000 103:01 2385802                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7ffff7ffd000-7ffff7fff000 rw-p 00039000 103:01 2385802                   /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
7ffffffdd000-7ffffffff000 rw-p 00000000 00:00 0                          [stack]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
```

### readelfを使ったコアダンプファイルの解析
- コアダンプファイルのセグメントを見てみる。
- バッファオーバーランでどこを書こうとして Segmentation fault がおきたのかをしらべる。
  - RW がついているのがグローバル変数のおいてあるセクションのはず。
  - `/proc/self/maps` をみると `555555559000-55555557a000` が `heap` になっている。
  - `LOAD           0x0000000000004000 0x0000555555558000 0x0000000000000000` があやしいと分かった。

```bash
$ CORE_FILENAME=$(find . | grep "^./core.*" | sort -R | head -n 1)
$ readelf -l ${CORE_FILENAME}
Elf file type is CORE (Core file)
Entry point 0x0
There are 24 program headers, starting at offset 64

Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  NOTE           0x0000000000000580 0x0000000000000000 0x0000000000000000
                 0x00000000000013f8 0x0000000000000000         0x0
  LOAD           0x0000000000002000 0x0000555555554000 0x0000000000000000
                 0x0000000000001000 0x0000000000001000  R      0x1000
  LOAD           0x0000000000003000 0x0000555555555000 0x0000000000000000
                 0x0000000000000000 0x0000000000001000  R E    0x1000
  LOAD           0x0000000000003000 0x0000555555556000 0x0000000000000000
                 0x0000000000000000 0x0000000000001000  R      0x1000
  LOAD           0x0000000000003000 0x0000555555557000 0x0000000000000000
                 0x0000000000001000 0x0000000000001000  R      0x1000
  LOAD           0x0000000000004000 0x0000555555558000 0x0000000000000000
                 0x0000000000001000 0x0000000000001000  RW     0x1000
  LOAD           0x0000000000005000 0x0000555555559000 0x0000000000000000
                 0x0000000000021000 0x0000000000021000  RW     0x1000
  LOAD           0x0000000000026000 0x00007ffff7c00000 0x0000000000000000
                 0x0000000000001000 0x0000000000028000  R      0x1000
  LOAD           0x0000000000027000 0x00007ffff7c28000 0x0000000000000000
                 0x0000000000000000 0x0000000000195000  R E    0x1000
  LOAD           0x0000000000027000 0x00007ffff7dbd000 0x0000000000000000
                 0x0000000000000000 0x0000000000058000  R      0x1000
  LOAD           0x0000000000027000 0x00007ffff7e15000 0x0000000000000000
                 0x0000000000004000 0x0000000000004000  R      0x1000
  LOAD           0x000000000002b000 0x00007ffff7e19000 0x0000000000000000
                 0x0000000000002000 0x0000000000002000  RW     0x1000
  LOAD           0x000000000002d000 0x00007ffff7e1b000 0x0000000000000000
                 0x000000000000d000 0x000000000000d000  RW     0x1000
  LOAD           0x000000000003a000 0x00007ffff7fa5000 0x0000000000000000
                 0x0000000000003000 0x0000000000003000  RW     0x1000
  LOAD           0x000000000003d000 0x00007ffff7fbb000 0x0000000000000000
                 0x0000000000002000 0x0000000000002000  RW     0x1000
  LOAD           0x000000000003f000 0x00007ffff7fbd000 0x0000000000000000
                 0x0000000000004000 0x0000000000004000  R      0x1000
  LOAD           0x0000000000043000 0x00007ffff7fc1000 0x0000000000000000
                 0x0000000000002000 0x0000000000002000  R E    0x1000
  LOAD           0x0000000000045000 0x00007ffff7fc3000 0x0000000000000000
                 0x0000000000001000 0x0000000000002000  R      0x1000
  LOAD           0x0000000000046000 0x00007ffff7fc5000 0x0000000000000000
                 0x0000000000000000 0x000000000002a000  R E    0x1000
  LOAD           0x0000000000046000 0x00007ffff7fef000 0x0000000000000000
                 0x0000000000000000 0x000000000000b000  R      0x1000
  LOAD           0x0000000000046000 0x00007ffff7ffb000 0x0000000000000000
                 0x0000000000002000 0x0000000000002000  R      0x1000
  LOAD           0x0000000000048000 0x00007ffff7ffd000 0x0000000000000000
                 0x0000000000002000 0x0000000000002000  RW     0x1000
  LOAD           0x000000000004a000 0x00007ffffffdd000 0x0000000000000000
                 0x0000000000022000 0x0000000000022000  RW     0x1000
  LOAD           0x000000000006c000 0xffffffffff600000 0x0000000000000000
                 0x0000000000001000 0x0000000000001000    E    0x1000
```

### バイナリエディタを使ったコアダンプファイルの解析
- 怪しいところをバイナリエディタで見てみる。
- バッファオーバーランで`0xdeadbeefdeadbeef` を書きつぶして、プログラムの先頭まで書いてページのアクセス違反で落ちたことがわかる。
  - 通常プログラムの自体は RX でロードされる。

```
$ xxd -s 0x3000 -l 0x100000 ${CORE_FILENAME}
00003000: 0100 0200 7200 2f70 726f 632f 7365 6c66  ....r./proc/self
00003010: 2f6d 6170 7300 2573 0000 0000 011b 033b  /maps.%s.......;
00003020: 3000 0000 0500 0000 04f0 ffff 6400 0000  0...........d...

============================= 省略 ================================

00003fe0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
00003ff0: 0000 0000 0000 0000 a059 c4f7 ff7f 0000  .........Y......
00004000: 0000 0000 0000 0000 0880 5555 5555 0000  ..........UUUU..
00004010: efbe adde efbe adde efbe adde efbe adde  ................
00004020: efbe adde efbe adde efbe adde efbe adde  ................

============================= 省略 ================================

00025fe0: efbe adde efbe adde efbe adde efbe adde  ................
00025ff0: efbe adde efbe adde efbe adde efbe adde  ................
00026000: 7f45 4c46 0201 0103 0000 0000 0000 0000  .ELF............
00026010: 0300 3e00 0100 0000 509f 0200 0000 0000  ..>.....P.......
00026020: 4000 0000 0000 0000 f0c0 2100 0000 0000  @.........!.....

============================= 省略 ================================

0006cfe0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
0006cff0: 0000 0000 0000 0000 0000 0000 0000 0000  ................
```

### コアダンプファイルの `PT_NOTE` の中身
- コアダンプファイルの `PT_NOTE` を見てみる。
- `NT_X86_XSTATE` という気になるパートがある。
  - [tanakmuraさんが書いたxsave/xrstor についての記事](https://zenn.dev/tanakmura/articles/8af62a260ff05d) に詳細がある。

```bash
$ readelf --note ./core.1687577511.931088

Displaying notes found at file offset 0x00000580 with length 0x000013f8:
  Owner                Data size 	Description
  CORE                 0x00000150	NT_PRSTATUS (prstatus structure)
  CORE                 0x00000088	NT_PRPSINFO (prpsinfo structure)
  CORE                 0x00000080	NT_SIGINFO (siginfo_t data)
  CORE                 0x00000150	NT_AUXV (auxiliary vector)
  CORE                 0x00000439	NT_FILE (mapped files)
    Page size: 4096
                 Start                 End         Page Offset
    0x0000555555554000  0x0000555555555000  0x0000000000000000
        /home/akira/ghq/github.com/akawashiro/misc/core/make_core
    0x0000555555555000  0x0000555555556000  0x0000000000000001
        /home/akira/ghq/github.com/akawashiro/misc/core/make_core
    0x0000555555556000  0x0000555555557000  0x0000000000000002
        /home/akira/ghq/github.com/akawashiro/misc/core/make_core
    0x0000555555557000  0x0000555555558000  0x0000000000000002
        /home/akira/ghq/github.com/akawashiro/misc/core/make_core
    0x0000555555558000  0x0000555555559000  0x0000000000000003
        /home/akira/ghq/github.com/akawashiro/misc/core/make_core
    0x00007ffff7c00000  0x00007ffff7c28000  0x0000000000000000
        /usr/lib/x86_64-linux-gnu/libc.so.6
    0x00007ffff7c28000  0x00007ffff7dbd000  0x0000000000000028
        /usr/lib/x86_64-linux-gnu/libc.so.6
    0x00007ffff7dbd000  0x00007ffff7e15000  0x00000000000001bd
        /usr/lib/x86_64-linux-gnu/libc.so.6
    0x00007ffff7e15000  0x00007ffff7e19000  0x0000000000000214
        /usr/lib/x86_64-linux-gnu/libc.so.6
    0x00007ffff7e19000  0x00007ffff7e1b000  0x0000000000000218
        /usr/lib/x86_64-linux-gnu/libc.so.6
    0x00007ffff7fc3000  0x00007ffff7fc5000  0x0000000000000000
        /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    0x00007ffff7fc5000  0x00007ffff7fef000  0x0000000000000002
        /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    0x00007ffff7fef000  0x00007ffff7ffa000  0x000000000000002c
        /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    0x00007ffff7ffb000  0x00007ffff7ffd000  0x0000000000000037
        /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
    0x00007ffff7ffd000  0x00007ffff7fff000  0x0000000000000039
        /usr/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2
  CORE                 0x00000200	NT_FPREGSET (floating point registers)
  LINUX                0x00000988	NT_X86_XSTATE (x86 XSAVE extended state)
   description data: 7f 03 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 1f 00 00 ff ff 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ff ff ff ff 00 00 00 00 00 ff ff ff 00 ff ff ff 66 66 66 66 66 66 66 36 30 31 30 30 30 20 2d 2d 2f 61 6b 61 77 61 73 68 69 72 6f 2f 6d 69 73 63 20 20 20 20 20 20 2f 68 6f 6d 65 2f 61 6b 69 72 33 30 30 30 20 31 30 33 3a 30 31 20 34 33 31 35 35 35 35 35 35 38 30 30 30 2d 35 35 35 35 35 35 00 00 00 00 01 1b 03 3b 30 00 00 00 05 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 07 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 02 02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 54 55 55 55 00 00 00 00
```

### コアダンプファイルをパースするプログラム
- コアダンプファイル自体をパースする C++のプログラムをつくる。
  - Process ID と RIP(プログラムカウンタ)を表示する。
  - `prstatus_t` 構造体にいろいろ入っている
    - [linux/elfcore.h](https://elixir.bootlin.com/linux/v4.7/source/include/uapi/linux/elfcore.h#L94)

```cpp
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
```

### 実行してみる
```bash
$ g++ -o ./parse_core ./parse_core.cc
$ ./parse_core ${CORE_FILENAME}
prstatus->pr_pid: 931088
regs->rip: 55555555528f
```
- PID は 931088。
- Segmentation fault が起きた RIP は 55555555528f と分かった。

### プログラムがクラッシュした命令を知りたい
- `55555555528f` に対応するのは `555555555000-555555556000` の部分。

```bash
555555554000-555555555000 r--p 00000000 103:01 44838307                  /home/akira/ghq/github.com/akawashiro/akawashiro.github.io/articles/core/make_core
555555555000-555555556000 r-xp 00001000 103:01 44838307                  /home/akira/ghq/github.com/akawashiro/akawashiro.github.io/articles/core/make_core
555555556000-555555557000 r--p 00002000 103:01 44838307                  /home/akira/ghq/github.com/akawashiro/akawashiro.github.io/articles/core/make_core
555555557000-555555558000 r--p 00002000 103:01 44838307                  /home/akira/ghq/github.com/akawashiro/akawashiro.github.io/articles/core/make_core
555555558000-555555559000 rw-p 00003000 103:01 44838307                  /home/akira/ghq/github.com/akawashiro/akawashiro.github.io/articles/core/make_core
```

- セグメントヘッダの情報を合わせて考えると、`55555555528f` は `55555555528f - 555555554000 = 128f` に対応している。

```
$ readelf -l make_core

Elf file type is DYN (Position-Independent Executable file)
Entry point 0x10e0
There are 13 program headers, starting at offset 64

Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  PHDR           0x0000000000000040 0x0000000000000040 0x0000000000000040
                 0x00000000000002d8 0x00000000000002d8  R      0x8
  INTERP         0x0000000000000318 0x0000000000000318 0x0000000000000318
                 0x000000000000001c 0x000000000000001c  R      0x1
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
  LOAD           0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000730 0x0000000000000730  R      0x1000
  LOAD           0x0000000000001000 0x0000000000001000 0x0000000000001000
                 0x00000000000002d1 0x00000000000002d1  R E    0x1000
  LOAD           0x0000000000002000 0x0000000000002000 0x0000000000002000
                 0x00000000000000fc 0x00000000000000fc  R      0x1000
  LOAD           0x0000000000002d98 0x0000000000003d98 0x0000000000003d98
                 0x0000000000000280 0x0000000000000288  RW     0x1000
  DYNAMIC        0x0000000000002da8 0x0000000000003da8 0x0000000000003da8
                 0x00000000000001f0 0x00000000000001f0  RW     0x8
  NOTE           0x0000000000000338 0x0000000000000338 0x0000000000000338
                 0x0000000000000030 0x0000000000000030  R      0x8
  NOTE           0x0000000000000368 0x0000000000000368 0x0000000000000368
                 0x0000000000000044 0x0000000000000044  R      0x4
  GNU_PROPERTY   0x0000000000000338 0x0000000000000338 0x0000000000000338
                 0x0000000000000030 0x0000000000000030  R      0x8
  GNU_EH_FRAME   0x000000000000201c 0x000000000000201c 0x000000000000201c
                 0x0000000000000034 0x0000000000000034  R      0x4
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x10
  GNU_RELRO      0x0000000000002d98 0x0000000000003d98 0x0000000000003d98
                 0x0000000000000268 0x0000000000000268  R      0x1

 Section to Segment mapping:
  Segment Sections...
   00
   01     .interp
   02     .interp .note.gnu.property .note.gnu.build-id .note.ABI-tag .gnu.hash .dynsym .dynstr .gnu.version .gnu.version_r .rela.dyn .rela.plt
   03     .init .plt .plt.got .plt.sec .text .fini
   04     .rodata .eh_frame_hdr .eh_frame
   05     .init_array .fini_array .dynamic .got .data .bss
   06     .dynamic
   07     .note.gnu.property
   08     .note.gnu.build-id .note.ABI-tag
   09     .note.gnu.property
   10     .eh_frame_hdr
   11
   12     .init_array .fini_array .dynamic .got
```

- `128f` 周辺をディスアセンブルしてみると確かに `0xdeadbeefdeadbeef` を書き込んでいそうなことがわかる。

```bash
$ objdump -d make_core | grep 128f -A 10 -B 10
    125f:       48 c7 85 e0 ef ff ff    movq   $0x0,-0x1020(%rbp)
    1266:       00 00 00 00
    126a:       eb 2e                   jmp    129a <main+0xd1>
    126c:       48 8b 85 e0 ef ff ff    mov    -0x1020(%rbp),%rax
    1273:       48 8d 14 c5 00 00 00    lea    0x0(,%rax,8),%rdx
    127a:       00
    127b:       48 8d 05 8e 2d 00 00    lea    0x2d8e(%rip),%rax        # 4010 <a>
    1282:       48 01 d0                add    %rdx,%rax
    1285:       48 b9 ef be ad de ef    movabs $0xdeadbeefdeadbeef,%rcx
    128c:       be ad de
    128f:       48 89 08                mov    %rcx,(%rax)
    1292:       48 83 85 e0 ef ff ff    addq   $0x1,-0x1020(%rbp)
    1299:       01
    129a:       48 81 bd e0 ef ff ff    cmpq   $0xfffff,-0x1020(%rbp)
    12a1:       ff ff 0f 00
    12a5:       76 c5                   jbe    126c <main+0xa3>
    12a7:       b8 00 00 00 00          mov    $0x0,%eax
    12ac:       48 8b 55 f8             mov    -0x8(%rbp),%rdx
    12b0:       64 48 2b 14 25 28 00    sub    %fs:0x28,%rdx
    12b7:       00 00
    12b9:       74 05                   je     12c0 <main+0xf7>
```

## readcore
- いまやったようなことを手軽にやりたい
- [https://github.com/akawashiro/readcore](https://github.com/akawashiro/readcore) を作っている

## おまけ - 誰が コアダンプファイルを生成しているのか?
- コアダンプファイルを生成しているのはLinuxカーネル
  - [fs/binfmt_elf.c](https://github.com/akawashiro/linux/blob/0457e5153e0e8420134f60921349099e907264ca/fs/binfmt_elf.c#L2162-L2169)
- クラッシュしたときはユーザ空間のプロセスは動けないので当然といえは当然

## 参考文献
- [man 5 core](https://linuxjm.osdn.jp/html/LDP_man-pages/man5/core.5.html)
- [man 7 signal](https://linuxjm.osdn.jp/html/LDP_man-pages/man7/signal.7.html)
  - 標準シグナル
