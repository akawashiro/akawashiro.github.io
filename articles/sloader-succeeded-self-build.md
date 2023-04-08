# Linux のローダを置き換え可能な sloader を開発しています <!-- omit in toc -->
## 概要 <!-- omit in toc -->
Linuxのスタンダードなローダ、`ld-linux-x86-64.so.2` の挙動を完全に理解するために、`ld-linux-x86-64.so.2`を完全に置き換え可能な [https://github.com/akawashiro/sloader](https://github.com/akawashiro/sloader) を2年ほど前から開発しており、ある程度動くようになってきました。

## 目次 <!-- omit in toc -->
- [ローダとは何か](#ローダとは何か)
- [`ld-linux-x86-64.so.2` の問題点](#ld-linux-x86-64so2-の問題点)
- [sloader](#sloader)
- [sloader の現状](#sloader-の現状)
- [sloaderの実装](#sloaderの実装)
  - [libc.so内のシンボルの解決](#libcso内のシンボルの解決)
  - [ロードされたプログラムのためのTLS領域の確保](#ロードされたプログラムのためのtls領域の確保)
- [sloaderの課題](#sloaderの課題)
- [お願い](#お願い)

## ローダとは何か
Linuxで実行可能なバイナリファイルを[execve(2)](https://man7.org/linux/man-pages/man2/execve.2.html)を使って実行するとき、その実行パスは大きく2つに分類されます。
- Linux カーネルが直接、バイナリファイルをメモリ空間にロードする
- バイナリが指定したローダ[^1]がバイナリファイルをメモリ空間にロードする

`readelf -l` でバイナリファイルの `PT_INTERP` セグメントを見ると、そのバイナリファイルがどちらの方法で起動されるかを確認できます。ほとんどの場合、`Requesting program interpreter: /lib64/ld-linux-x86-64.so.2` と書いてあり、これは、バイナリファイルが指定したローダがそのファイルをメモリ空間にロードすることを意味します。

```
> readelf -l $(which nvim) | grep INTERP -A 2
  INTERP         0x0000000000000318 0x0000000000000318 0x0000000000000318
                 0x000000000000001c 0x000000000000001c  R      0x1
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
```

`ld-linux-x86-64.so.2` はバイナリファイルのロード時に3つの処理を行います。
- そのバイナリが必要とする共有ライブラリの検索
- 共有ライブラリとバイナリのメモリ空間へのロード
- 共有ライブラリとバイナリ中のシンボル解決

`ld-linux-x86-64.so.2` の挙動を正確に理解することは重要です。例えば、LD_PRELOADやLD_AUDITなどの環境変数による便利なハックはその挙動を変更することで実現されています。`ld-linux-x86-64.so.2` の挙動を理解していれば、これらの環境変数で何が可能であり何が不可能かを推測できるようになります。また、共有ライブラリを無理やりリンクする[https://github.com/akawashiro/sold](https://github.com/akawashiro/sold) のようなソフトウェアの制作にはその理解が必要不可欠です。

## `ld-linux-x86-64.so.2` の問題点
`ld-linux-x86-64.so.2` は GNU libc の一部として Linux にインストールされており、そのソースコードはすべて[公開](https://www.gnu.org/software/libc/sources.html)されています。しかし、公開されているソースコードから`ld-linux-x86-64.so.2` の挙動を理解するにあたっては以下の2つの問題があります。

1つ目の問題は、GNU libcのソースコードは読みにくい点です。GNU libcは歴史の長いソフトウェアでC言語で書かれています。また、x86-84、Alpha、ARM、PowerPCなどの複数のアーキテクチャへの移植性を求められます。このため、ソースコードの至る部分でマクロが多用されており、プログラムの流れを追うのが難しくなっています。

2つ目の問題は、`libc.so` の初期化とプログラムのロードが同時に行われていて、ロード部分だけを分離して理解することができない点です。`libc.so` とはいわゆる標準Cライブラリで、バイナリファイルが `ld-linux-x86-64.so.2` によってロードされるときはほぼ確実に同時にロードされます。`libc.so` と `ld-linux-x86-64.so.2` は同じパッケージに存在し、2つのコンポーネントの役割分担が明示的にドキュメントされていません。このため、`libc.so`だけ、もしくはロードだけを分離して理解し、別のソフトウェアに切り出すことは非常に難しくなっています。

## sloader
上で述べた問題を解決し、`ld-linux-x86-64.so.2`の挙動を理解するために `ld-linux-x86-64.so.2` を置き換え可能な新しいローダを開発することにしました。つまりLinux上で起動するすべてのプログラム(`systemd`、`cat`、`find`、`firefox`等)を自分でロードしようとしています。

このローダの名前は `sloader` で、リポジトリは [https://github.com/akawashiro/sloader/](https://github.com/akawashiro/sloader/) にあります。`sloader` の開発にあたっては次の2つを原則としています。

1つ目の原則は、C言語ではなくモダンなC++を使う、です。C++20までのモダンなC++の機能を使い可読性を上げようとしています。Rustを採用することも考えたのですが、GNU libcのソースコードを参照しながら開発することを考えると、C言語との互換性がある言語のほうが良いと判断しました。

2つ目の原則は、`libc.so` の初期化はやらない、です。ロード部分だけを理解するのが目的なので複雑怪奇な `libc.so` の初期化はやりません。というよりできませんでした。`libc.so` の初期化を開始しつつ `libc.so` に依存するプログラムをロードする方法は後ほど説明します。

## sloader の現状
`sloader` は `ld-linux-x86-64.so.2` を完全に置き換えるまでにはまだ至っていません。しかし、ある程度までは完成していて、`sloader`のビルドに必要なソフトウェアをすべて起動することが可能です。具体的には `cmake`、`g++`、`ld`、`ninja`が起動でき、`sloader`自身のバイナリを生成できます。

実際に`sloader`のバイナリを`sloader`を使って生成するスクリプトは [https://github.com/akawashiro/sloader/blob/master/make-sloader-itself.sh](https://github.com/akawashiro/sloader/blob/master/make-sloader-itself.sh)です。主要な部分を抜粋しておきます。
```
$ sloader cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S . -B build \
    -D CMAKE_C_COMPILER_LAUNCHER=sloader  -D CMAKE_CXX_COMPILER_LAUNCHER=sloader \
    -D CMAKE_C_LINKER_LAUNCHER=sloader -D CMAKE_CXX_LINKER_LAUNCHER=sloader
$ sloader make VERBOSE=1
```

## sloaderの実装
### libc.so内のシンボルの解決
先程述べたように、`sloader` は `ld-linux-x86-64.so.2` を置き換えることを目標としています。当然、`sloader` でロードしたいプログラムは `libc.so` に依存しています。一方、`sloader` で `libc.so` をロードしません。

`sloader` では `sloader` 自身にリンクされた `libc.so` をロードしたプログラムから参照するように再配置情報を解決することでこの問題を解決しています。具体的には[dyn_loader.cc#L621](https://github.com/akawashiro/sloader/blob/502bae54b403423f79e04caa4901c4a76cb6aaca/dyn_loader.cc#L621)で `R_X86_64_GLOB_DAT` または `R_X86_64_JUMP_SLOT` のシンボル名がlibcのものの場合、再配置情報で指示されたアドレスに`libc.so`のアドレスではなく [std::map<std::string, Elf64_Addr> sloader_libc_map](https://github.com/akawashiro/sloader/blob/502bae54b403423f79e04caa4901c4a76cb6aaca/libc_mapping.cc#L248)内の関数ポインタの値を書き込んでいます。

### ロードされたプログラムのためのTLS領域の確保
[上](#libc.so内のシンボルの解決)で述べたように、`sloader`でロードされたプログラムは`sloader`自身にリンクされたlibcを使うのですが、この手法には別の問題があります。それはロードされたプログラムがThread Local Storage (TLS) 変数にアクセスすると、`sloader` 自身のTLS領域にアクセスしてしまうという問題です。

この問題は`sloader`内にダミーのTLS領域を確保して回避しています。[tls_secure.cc#L4](https://github.com/akawashiro/sloader/blob/502bae54b403423f79e04caa4901c4a76cb6aaca/tls_secure.cc#L4)で4096バイトのダミーのTLS領域をTLS領域の先頭に定義して、ロードされたプログラムはここを参照するようにしています。
```
constexpr int TLS_SPACE_FOR_LOADEE = 4096;
thread_local unsigned char sloader_dummy_to_secure_tls_space[TLS_SPACE_FOR_LOADEE] = {0, 0, 0, 0};
```

これで一件落着、のように思えますが、まだ問題が残っています。それは通常の方法でダミーのTLS領域をTLS領域の先頭に定義できない点です。現状では[CMakeLists.txt#L32](https://github.com/akawashiro/sloader/blob/502bae54b403423f79e04caa4901c4a76cb6aaca/CMakeLists.txt#L32)で`tls_secure.o` を最初にリンクすることで無理やり定義していますが、この方法はリンカの実装に依存しています。

更に悪いことにこの最初にリンクする手法は `libc.a` をスタティックリンクすると機能しません。このため、現在 `sloader` を起動するためには `ld-linux-x86-64.so.2` が必要という大変恥ずかしい状況になっています。

## sloaderの課題
まず [上](#ロードされたプログラムのためのTLS領域の確保)で述べたように `sloader` を`ld-linux-x86-64.so.2`なしで起動することができません。この問題は、TLSのダミー領域を確保するハックを別のものに置き換える、もしくはリンカスクリプト等でTLS領域の先頭に強制的にダミー領域を定義することで解決できるはずです。

次に `sloader` で起動できないソフトウェアがまだ多く残っています。neovimやfirefox等は起動するとSegmentation Faultを引き起こします。この原因は未だにわかっていません。

最後に `sloader` の再配置処理は遅いです。firefoxなどの大きめのプログラムの起動には1秒以上かかります。ただし、これは単純にパフォーマンスの問題であり、プロファイルを取って改善すれば解決するはずです。

## お願い
[https://github.com/akawashiro/sloader](https://github.com/akawashiro/sloader) をスターしてください。励みになります。

[^1]: ローダはダイナミックリンカと呼ばれることもありますが、この記事では「ローダ」で統一します。