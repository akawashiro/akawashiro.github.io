# CTF の Pwn に再入門する記録

## 目標

[詳解セキュリティコンテスト CTF で学ぶ脆弱性攻略の技術](https://book.mynavi.jp/ec/products/detail/id=122750)の Part 5 に載っているすべての攻撃手法を理解し、それらを使った簡単な問題を自力で解けるようになること。つまり、以下の攻撃手法をすべて自分で使えるようになること。

- スタックベースエクスプロイト
  - 局所変数の改竄
  - Return-Oriented Programming
  - Stack Pivot
- 共有ライブラリと関数呼び出し
  - GOT Overwrite
  - Return to PLT
  - Return to libc
- ヒープベースエクスプロイト
  - ヒープ・libc のアドレスの取得
  - tcache エントリの重複
  - tcache エントリの汚染
  - mp\_.tcache_bins の改竄
  - fastbin チャンクの重複
  - fastbin の汚染
  - global_max_fast の改竄
  - smallbin の汚染
  - largebin の汚染
  - size の改竄
- 仕様に起因する脆弱性
  - 制御を奪う
  - 書式文字列攻撃

## 問題

### NEMU

問題はまだ公開されていない。DBL で左に 1bit ずらせるのも気づけなかったし、書き込み可能な領域が足りないときに他のヒープとつなげてシェルコードを完成させるのも気づかなかった。

- [RICERCA CTF 2023](https://2023.ctf.ricsec.co.jp/)
- [入力からシェルコードを入れている writeup](https://zenn.dev/ri5255/articles/4d5bac95f7238d)
- [jmp short/jmp near](https://www.felixcloutier.com/x86/jmp.html)
  - 近くに jmp できるやつ

やりかけ

```
from pwn import *
import random

io = process('./chall')

def f(s1, s2):
    s = io.recvuntil("opcode: ")
    print(s.decode())
    print(s1)
    d = {"LOAD": "1", "MOV": "2", "INC": "3", "DBL": "4", "ADDI": "5", "ADD": "6"}
    io.sendline(d[s1])
    s = io.recvuntil("operand: ")
    print(s.decode())
    print(s2)
    io.sendline(s2)

p = 0xcccccccc
f("LOAD", f"#{p}")
f("MOV", "r3")

p = 0xcccccccc
f("LOAD", f"#{p}")
f("MOV", "r2")

# Rewrite trampoline
p = 0xcccccccc
f("LOAD", f"#{p}")
f("MOV", "r1")
```

## 環境構築

### ghidra のインストール

[https://github.com/NationalSecurityAgency/ghidra](https://github.com/NationalSecurityAgency/ghidra)

### gdb-peda のインストール

[https://github.com/longld/peda](https://github.com/longld/peda)

```
git clone https://github.com/longld/peda.git ~/peda
echo "source ~/peda/peda.py" >> ~/.gdbinit
```

### Pwngdb のインストール

[https://github.com/scwuaptx/Pwngdb](https://github.com/scwuaptx/Pwngdb)

```
git clone https://github.com/scwuaptx/Pwngdb.git
tail -n +3 ~/Pwngdb/.gdbinit >> ~/.gdbinit
```

### pwntools のインストール

```
pip3 install pwntools
```

## デバッグテクニック

### ASLR の切り方

```
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```

### GDB がアタッチできないときの対処

```
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
```

### core ファイルをカレントディレクトリに作る

```
ulimit -c unlimited
sudo service apport stop
```

### プロセスが core ファイルを出力したあとにバックトレースを見る

run.sh

```
#! /bin/bash
rm -f core.*
python3 r.py
CORE=$(find . | grep core)
gdb ./binary ${CORE} -x bt.gdb
```

bt.gdb

```
bt
q
```

### プログラムカウンタを`0xcc` の上に持ってきてデバッガで見る
