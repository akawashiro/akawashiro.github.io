# CTFのPwnに再入門する記録
## 目標
[詳解セキュリティコンテスト CTFで学ぶ脆弱性攻略の技術](https://book.mynavi.jp/ec/products/detail/id=122750)のPart 5に載っているすべての攻撃手法を理解し、それらを使った簡単な問題を自力で解けるようになること。つまり、以下の攻撃手法をすべて自分で使えるようになること。
- スタックベースエクスプロイト
    - 局所変数の改竄
    - Return-Oriented Programming
    - Stack Pivot
- 共有ライブラリと関数呼び出し
    - GOT Overwrite
    - Return to PLT
    - Return to libc
- ヒープベースエクスプロイト
    - ヒープ・libcのアドレスの取得
    - tcacheエントリの重複
    - tcacheエントリの汚染
    - mp_.tcache_binsの改竄
    - fastbinチャンクの重複
    - fastbinの汚染
    - global_max_fastの改竄
    - smallbinの汚染
    - largebinの汚染
    - sizeの改竄
- 仕様に起因する脆弱性
    - 制御を奪う
    - 書式文字列攻撃

## 問題
### NEMU
問題はまだ公開されていない。DBLで左に1bitずらせるのも気づけなかったし、書き込み可能な領域が足りないときに他のヒープとつなげてシェルコードを完成させるのも気づかなかった。
- [RICERCA CTF 2023](https://2023.ctf.ricsec.co.jp/)
- [入力からシェルコードを入れているwriteup](https://zenn.dev/ri5255/articles/4d5bac95f7238d)
- [jmp short/jmp near](https://www.felixcloutier.com/x86/jmp.html)
    - 近くにjmpできるやつ

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
### ghidraのインストール
[https://github.com/NationalSecurityAgency/ghidra](https://github.com/NationalSecurityAgency/ghidra)
### gdb-pedaのインストール
[https://github.com/longld/peda](https://github.com/longld/peda)
```
git clone https://github.com/longld/peda.git ~/peda
echo "source ~/peda/peda.py" >> ~/.gdbinit
```
### Pwngdbのインストール
[https://github.com/scwuaptx/Pwngdb](https://github.com/scwuaptx/Pwngdb)
```
git clone https://github.com/scwuaptx/Pwngdb.git
tail -n +3 ~/Pwngdb/.gdbinit >> ~/.gdbinit
```
### pwntoolsのインストール
```
pip3 install pwntools
```

## デバッグテクニック
### ASLRの切り方
```
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
```
### GDBがアタッチできないときの対処
```
echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
```
### coreファイルをカレントディレクトリに作る
```
ulimit -c unlimited
sudo service apport stop
```
### プロセスがcoreファイルを出力したあとにバックトレースを見る
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
