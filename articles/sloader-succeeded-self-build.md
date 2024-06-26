---
title: Linuxのローダを自作する
layout: default
---

## 概要 <!-- omit in toc -->

Linux のスタンダードなローダ、`ld-linux-x86-64.so.2` の挙動を完全に理解するために、`ld-linux-x86-64.so.2`を完全に置き換え可能な [https://github.com/akawashiro/sloader](https://github.com/akawashiro/sloader) を 2 年ほど前から開発しており、ある程度動くようになってきました。

## 目次 <!-- omit in toc -->

- [ローダとは何か](#ローダとは何か)
- [`ld-linux-x86-64.so.2` の問題点](#ld-linux-x86-64so2-の問題点)
- [sloader](#sloader)
- [sloader の現状](#sloader-の現状)
- [sloader の実装](#sloaderの実装)
  - [libc.so 内のシンボルの解決](#libcso内のシンボルの解決)
  - [ロードされたプログラムのための TLS 領域の確保](#ロードされたプログラムのためのtls領域の確保)
- [sloader の課題](#sloaderの課題)
- [お願い](#お願い)

## ローダとは何か

Linux で実行可能なバイナリファイルを[execve(2)](https://man7.org/linux/man-pages/man2/execve.2.html)を使って実行するとき、その実行パスは大きく 2 つに分類されます。

- Linux カーネルが直接、バイナリファイルをメモリ空間にロードする。
- バイナリが指定したローダ[^1]がバイナリファイルをメモリ空間にロードする。

なお、詳細は Linux カーネルの[fs/binfmt_elf.c#L1249-L1280](https://github.com/akawashiro/linux/blob/830b3c68c1fb1e9176028d02ef86f3cf76aa2476/fs/binfmt_elf.c#L1249-L1280)付近を参照してください。

`readelf -l` でバイナリファイルの `PT_INTERP` セグメントを見ると、そのバイナリファイルがどちらの方法で起動されるかを確認できます。ほとんどの場合、`Requesting program interpreter: /lib64/ld-linux-x86-64.so.2` と書いてあり、これは、バイナリファイルが指定したローダがそのファイルをメモリ空間にロードすることを意味します。

```
> readelf -l $(which nvim) | grep INTERP -A 2
  INTERP         0x0000000000000318 0x0000000000000318 0x0000000000000318
                 0x000000000000001c 0x000000000000001c  R      0x1
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
```

`ld-linux-x86-64.so.2` はバイナリファイルのロード時に 3 つの処理を行います。

- そのバイナリが必要とする共有ライブラリの検索
- 共有ライブラリとバイナリのメモリ空間へのロード
- 共有ライブラリとバイナリ中のシンボル解決

`ld-linux-x86-64.so.2` の挙動を正確に理解することは重要です。例えば、LD_PRELOAD や LD_AUDIT などの環境変数による便利なハックはその挙動を変更することで実現されています。`ld-linux-x86-64.so.2` の挙動を理解していれば、これらの環境変数で何が可能であり何が不可能かを推測できるようになります。また、共有ライブラリを無理やりリンクする[https://github.com/akawashiro/sold](https://github.com/akawashiro/sold) のようなソフトウェアの制作にはその理解が必要不可欠です。

## `ld-linux-x86-64.so.2` の問題点

`ld-linux-x86-64.so.2` は GNU libc の一部として Linux にインストールされており、そのソースコードはすべて[公開](https://www.gnu.org/software/libc/sources.html)されています。しかし、公開されているソースコードから`ld-linux-x86-64.so.2` の挙動を理解するにあたっては以下の 2 つの問題があります。

1 つ目の問題は、GNU libc のソースコードは読みにくい点です。GNU libc は歴史の長いソフトウェアで C 言語で書かれています。また、x86-64、Alpha、ARM、PowerPC などの複数のアーキテクチャへの移植性を求められます。このため、ソースコードの至る部分でマクロが多用されており、プログラムの流れを追うのが難しくなっています。

2 つ目の問題は、`libc.so` の初期化とプログラムのロードが同時に行われていて、ロード部分だけを分離して理解することができない点です。`libc.so` とはいわゆる標準 C ライブラリで、バイナリファイルが `ld-linux-x86-64.so.2` によってロードされるときはほぼ確実に同時にロードされます。`libc.so` と `ld-linux-x86-64.so.2` は同じパッケージに存在し、2 つのコンポーネントの役割分担が明示的にドキュメントされていません。このため、`libc.so`だけ、もしくはロードだけを分離して理解し、別のソフトウェアに切り出すことは非常に難しくなっています。

## sloader

上で述べた問題を解決し、`ld-linux-x86-64.so.2`の挙動を理解するために `ld-linux-x86-64.so.2` を置き換え可能な新しいローダを開発することにしました。つまり Linux 上で起動するすべてのプログラム(`systemd`、`cat`、`find`、`firefox`等)を自分でロードしようとしています。

このローダの名前は `sloader` で、リポジトリは [https://github.com/akawashiro/sloader/](https://github.com/akawashiro/sloader/) にあります。`sloader` の開発にあたっては次の 2 つを原則としています。

1 つ目の原則は、C 言語ではなくモダンな C++を使う、です。C++20 までのモダンな C++の機能を使い可読性を上げようとしています。Rust を採用することも考えたのですが、GNU libc のソースコードを参照しながら開発することを考えると、C 言語との互換性がある言語のほうが良いと判断しました。

2 つ目の原則は、`libc.so` の初期化はやらない、です。ロード部分だけを理解するのが目的なので複雑怪奇な `libc.so` の初期化はやりません。というよりできませんでした。`libc.so` の初期化を開始しつつ `libc.so` に依存するプログラムをロードする方法は後ほど説明します。

## sloader の現状

`sloader` は `ld-linux-x86-64.so.2` を完全に置き換えるまでにはまだ至っていません。しかし、ある程度までは完成していて、`sloader`のビルドに必要なソフトウェアをすべて起動することが可能です。具体的には `cmake`、`g++`、`ld`、`ninja`が起動でき、`sloader`自身のバイナリを生成できます。

実際に`sloader`のバイナリを`sloader`を使って生成するスクリプトは [https://github.com/akawashiro/sloader/blob/master/make-sloader-itself.sh](https://github.com/akawashiro/sloader/blob/master/make-sloader-itself.sh)です。主要な部分を抜粋しておきます。

```
$ sloader cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S . -B build \
    -D CMAKE_C_COMPILER_LAUNCHER=sloader  -D CMAKE_CXX_COMPILER_LAUNCHER=sloader \
    -D CMAKE_C_LINKER_LAUNCHER=sloader -D CMAKE_CXX_LINKER_LAUNCHER=sloader
$ sloader make VERBOSE=1
```

また、いくつかの GUI アプリケーションを起動することにも成功しています。この画像は `sloader` を使って`xeyes`、`xconsole`、`xcalc` を起動した例です。`xeyes` は本来六角形ではなく円形の目が表示されるのですが、`sloader` の何らかのバグで六角形になっています。
![sloaderを使ってGUIアプリケーションを起動する](./xapps-launched-by-sloader.png)

## sloader の実装

### libc.so 内のシンボルの解決

先程述べたように、`sloader` は `ld-linux-x86-64.so.2` を置き換えることを目標としています。当然、`sloader` でロードしたいプログラムは `libc.so` に依存しています。一方、`sloader` で `libc.so` をロードしません。

`sloader` では `sloader` 自身にリンクされた `libc.so` をロードしたプログラムから参照するように再配置情報を解決することでこの問題を解決しています。具体的には[dyn_loader.cc#L621](https://github.com/akawashiro/sloader/blob/502bae54b403423f79e04caa4901c4a76cb6aaca/dyn_loader.cc#L621)で `R_X86_64_GLOB_DAT` または `R_X86_64_JUMP_SLOT` のシンボル名が libc のものの場合、再配置情報で指示されたアドレスに`libc.so`のアドレスではなく [std::map<std::string, Elf64_Addr> sloader_libc_map](https://github.com/akawashiro/sloader/blob/502bae54b403423f79e04caa4901c4a76cb6aaca/libc_mapping.cc#L248)内の関数ポインタの値を書き込んでいます。

### ロードされたプログラムのための TLS 領域の確保

[上](#libc.so内のシンボルの解決)で述べたように、`sloader`でロードされたプログラムは`sloader`自身にリンクされた libc を使うのですが、この手法には別の問題があります。それはロードされたプログラムが Thread Local Storage (TLS) 変数にアクセスすると、`sloader` 自身の TLS 領域にアクセスしてしまうという問題です。

この問題は`sloader`内にダミーの TLS 領域を確保して回避しています。[tls_secure.cc#L4](https://github.com/akawashiro/sloader/blob/502bae54b403423f79e04caa4901c4a76cb6aaca/tls_secure.cc#L4)で 4096 バイトのダミーの TLS 領域を TLS 領域の先頭に定義して、ロードされたプログラムはここを参照するようにしています。

```
constexpr int TLS_SPACE_FOR_LOADEE = 4096;
thread_local unsigned char sloader_dummy_to_secure_tls_space[TLS_SPACE_FOR_LOADEE] = {0, 0, 0, 0};
```

これで一件落着、のように思えますが、まだ問題が残っています。それは通常の方法でダミーの TLS 領域を TLS 領域の先頭に定義できない点です。現状では[CMakeLists.txt#L32](https://github.com/akawashiro/sloader/blob/502bae54b403423f79e04caa4901c4a76cb6aaca/CMakeLists.txt#L32)で`tls_secure.o` を最初にリンクすることで無理やり定義していますが、この方法はリンカの実装に依存しています。

更に悪いことにこの最初にリンクする手法は `libc.a` をスタティックリンクすると機能しません。このため、現在 `sloader` を起動するためには `ld-linux-x86-64.so.2` が必要という大変恥ずかしい状況になっています。

## sloader の課題

まず [上](#ロードされたプログラムのためのTLS領域の確保)で述べたように `sloader` を`ld-linux-x86-64.so.2`なしで起動することができません。この問題は、TLS のダミー領域を確保するハックを別のものに置き換える、もしくはリンカスクリプト等で TLS 領域の先頭に強制的にダミー領域を定義することで解決できるはずです。

次に `sloader` で起動できないソフトウェアがまだ多く残っています。neovim や firefox 等は起動すると Segmentation Fault を引き起こします。この原因は未だにわかっていません。

最後に `sloader` の再配置処理は遅いです。firefox などの大きめのプログラムの起動には 1 秒以上かかります。ただし、これは単純にパフォーマンスの問題であり、プロファイルを取って改善すれば解決するはずです。

## お願い

[https://github.com/akawashiro/sloader](https://github.com/akawashiro/sloader) をスターしてください。励みになります。

[^1]: ローダはダイナミックリンカと呼ばれることもありますが、この記事では「ローダ」で統一します。
