---
title: ros3fs - オブジェクトストレージ用の高速な FUSE
layout: default
---

# ros3fs - オブジェクトストレージ用の高速な FUSE

[ros3fs](https://github.com/akawashiro/ros3fs)は S3 互換のオブジェクトストレージのための FUSE です。[ros3fs](https://github.com/akawashiro/ros3fs)は読み込み専用かつバケットのデータの更新に追随しないという強い制約を設ける代わりに、高速にデータの閲覧を可能にしています。極端な例では既存実装に比べて 100~1000 倍の高速化を実現しました。

## オブジェクトストレージ

[AWS S3](https://aws.amazon.com/s3/)や[Cloudflare R2](https://developers.cloudflare.com/r2/)、[MinIO](https://min.io/)、[Apache Ozone](https://ozone.apache.org/)などはオブジェクトストレージと呼ばれるソフトウェアです。オブジェクトストレージはスケーラビリティにすぐれており、[AWS S3](https://aws.amazon.com/s3/)や[Cloudflare R2](https://developers.cloudflare.com/r2/)などのクラウドサービスとして提供されている場合は手軽に安価に使い始めることができます。

オブジェクトストレージはファイルをオブジェクトという単位で扱います。オブジェクトはフラットな空間に格納され、ディレクトリという概念はありません。アクセスには通常 REST API を使います。[AWS S3](https://aws.amazon.com/s3/)が策定した API がよく知られており、[Cloudflare R2](https://developers.cloudflare.com/r2/)、[MinIO](https://min.io/)、[Apache Ozone](https://ozone.apache.org/)はどれも[AWS S3](https://aws.amazon.com/s3/)互換の API を実装しています。このため、これらのソフトウェアはすべて[AWS S3 cli](https://docs.aws.amazon.com/cli/latest/reference/s3/)経由で利用することができます。

[AWS S3](https://aws.amazon.com/s3/)を含むオブジェクトストレージは手軽なのですが、一つ欠点があります。それはデータを通常のファイルのように閲覧できない点です。オブジェクトストレージ上のデータを直接`cat`や`grep`することはできませんし、[GNOME Files](https://gitlab.gnome.org/GNOME/nautilus)のような GUI のファイルマネージャで見ることもできません。

## FUSE

このような問題を解決する一つの方法が[FUSE](https://www.kernel.org/doc/html/next/filesystems/fuse.html)です。[FUSE](https://www.kernel.org/doc/html/next/filesystems/fuse.html)とは Filesystem in Userspace の略であり、[Linux](https://github.com/libfuse/libfuse)、[Mac OS](https://osxfuse.github.io/)、Windows[^1]で利用できる技術です。

[FUSE](https://www.kernel.org/doc/html/next/filesystems/fuse.html)を使うとファイルシステムをユーザ空間内に実装できます。このファイルシステムの実装を自由に書くことができるため、例えば`open`システムコールを AWS S3 の[GetObject API](https://docs.aws.amazon.com/AmazonS3/latest/API/API_GetObject.html)に結び付けることで、ファイルを開く操作を自動的に S3 のオブジェクトを取得する操作に変換することができます。

また、オブジェクトストレージにはディレクトリ構造がありませんが、FUSE を使って模擬的にそれを実現することも可能です。オブジェクト名を"/"などのパス区切り文字で区切ったものをディレクトリとしてユーザに見せるように FUSE を実装すれば、オブジェクトストレージがディレクトリを持ったファイルシステムのように見せることができます。

## 既存の s3 向け FUSE の問題点

S3 互換の API を持つオブジェクトストレージ向けの FUSE は AWS 本家による[mountpoint-s3](https://github.com/awslabs/mountpoint-s3)や[s3fs-fuse](https://github.com/s3fs-fuse/s3fs-fuse)などの既存実装があります。どちらを使っても問題なくオブジェクトストレージをファイルシステムとして利用することができます。

しかし、この二つの実装にはどちらも**遅い**という問題点があります。特にディレクトリ構造を走査するような操作は遅く、オブジェクト数の多いバケットを FUSE でマウントして`ls`コマンドを打つと体感できる程度の遅延があります。これはおそらく、dエントリのようなディレクトリ構造を保持する専用のデータ構造がないオブジェクトストレージを、無理やりファイルシステムとして見せているために生じる遅延であり、根本的に解決することが困難な問題です。

## ros3fs

この問題を回避して`ls`の遅延を最小化するために作ったのが[ros3fs](https://github.com/akawashiro/ros3fs)です。[ros3fs](https://github.com/akawashiro/ros3fs)は S3 互換のオブジェクトストレージのための FUSE であり、ディレクトリ構造の走査を含む読み出しの遅延を最小化することを目的に設計されています。

Linux で[ros3fs](https://github.com/akawashiro/ros3fs)で使うにはビルドする必要があります。Ubuntu 22.04 で ros3fs をビルドする手順は以下の通りです。

```bash
$ sudo apt-get install -y cmake g++ git libfuse3-dev ninja-build zlib1g-dev libcurl4-openssl-dev libssl-dev ccache pkg-config
$ git clone https://github.com/akawashiro/ros3fs.git
$ cd ros3fs
$ mkdir build
$ ./build-aws-sdk-cpp.sh ./build
$ cmake -S . -B build
$ cmake --build build -- -j
$ cmake --build build -- install
```

その後、このコマンドで S3 互換のオブジェクトストレージをマウントできます。マウントした後は`<MOUNTPOINT>`ディレクトリにオブジェクトストレージのデータがマウントされています。`ls`や`cat`などのコマンドを使って中身を確認することも可能です。

```bash
$ ./build/ros3fs <MOUNTPOINT> -f -d --endpoint=<ENDPOINT URL> --bucket_name=<BUCKET NAME ENDS WITH '/'> --cache_dir=<CACHE DIRECTORY>
$ ls <MOUNTPOINT>
dir_a/  testfile_a  testfile_b  testfile_c
$ cat testfile_a
hoge
```

先ほど述べたように、[ros3fs](https://github.com/akawashiro/ros3fs)は読み出しの遅延を最小化することを目的に設計されています。その代償としていくつかの制限があります。まず書き込みはサポートしません。これは書き込みをサポートすると[ros3fs](https://github.com/akawashiro/ros3fs)のキャッシュとオブジェクトストレージの間の整合性をとるのが困難になるためです。

また、[ros3fs](https://github.com/akawashiro/ros3fs)はバケットに存在するデータと異なる古いデータを読みだすことがあります。遅延を最小化するために[ros3fs](https://github.com/akawashiro/ros3fs)はデータを極端にキャッシュします。[ros3fs](https://github.com/akawashiro/ros3fs)は起動時にすべてのオブジェクト名を取得しディレクトリ構造を構築します。また、一度アクセスしたデータはローカルに保存し、二回目以降のアクセスではそのデータを読みます。このため、[ros3fs](https://github.com/akawashiro/ros3fs)でバケットをマウントした後にそのバケットのオブジェクトが変更された場合、その変更を読みだせないケースがあります。

このような思い切った設計の背景には、筆者のオブジェクトストレージの使い方があります。筆者はオブジェクトストレージをバックアップデータの保存先として使っており、そのバケットの更新頻度は非常に低いです。一方、バックアップしたデータを参照する頻度は更新頻度に比べて高いです。このため、バケットをマウントした後オブジェクトを変更するケースをサポートしない判断をしました。この判断により、読み取りについてはほかの S3 向けの FUSE と比べて大幅に高速になっています。

## 性能比較

ローカルに[Apache Ozone](https://ozone.apache.org/)のサーバを構築し、1000 個のテキストファイルをバケットに格納してから、FUSE でマウントし`grep`で検索した時の性能を比較します。この性能比較は[benchmark.sh](https://github.com/akawashiro/ros3fs/blob/master/benchmark.sh)で行いました。なお、この性能比較では、キャッシュのウォームアップがあるため[ros3fs](https://github.com/akawashiro/ros3fs)にかなり有利なものです。

[ros3fs](https://github.com/akawashiro/ros3fs)はコミットハッシュ`afa6156e753539b7a530be9c7c25cdb987b5ffad`、[s3fs-fuse](https://github.com/s3fs-fuse/s3fs-fuse)は`V1.90`、[mountpoint-s3](https://github.com/awslabs/mountpoint-s3)は`v1.0.2`を使い、OS は`Ubuntu 22.04.2 LTS`、CPU は`AMD Ryzen 9 5950X`です。

測定は[ros3fs](https://github.com/akawashiro/ros3fs)に含まれるスクリプトを使って行いました。まず、オブジェクトストレージソフトウェアの一つである[Apache Ozone](https://ozone.apache.org/)を起動します。

```bash
./launch-ozone.sh
```

次に別のターミナルで`./benchmark.sh`を起動すると測定を行います。

<details><summary>`./benchmark.sh` の実行結果の例</summary><div>

```bash
./benchmark.sh
# 省略
========== Compare grep performance without cache warmup ==========
time grep -r /home/akira/ghq/github.com/akawashiro/ros3fs/build_benchmark/ros3fs_mountpoint -e 123

real    0m3.046s
user    0m0.000s
sys     0m0.021s
time grep -r /home/akira/ghq/github.com/akawashiro/ros3fs/build_benchmark/s3fs-fuse_mountpoint -e 123

real    0m2.042s
user    0m0.005s
sys     0m0.016s
time grep -r /home/akira/ghq/github.com/akawashiro/ros3fs/build_benchmark/mountpoint-s3_mountpoint -e 123

real    0m8.660s
user    0m0.004s
sys     0m0.024s
============================================================
========== Compare grep performance with cache warmup ==========
Benchmark 1: grep -r /home/akira/ghq/github.com/akawashiro/ros3fs/build_benchmark/ros3fs_mountpoint -e '123'
  Time (mean ± σ):      15.2 ms ±   1.1 ms    [User: 1.9 ms, System: 4.1 ms]
  Range (min … max):    12.7 ms …  17.0 ms    10 runs

Benchmark 1: grep -r /home/akira/ghq/github.com/akawashiro/ros3fs/build_benchmark/s3fs-fuse_mountpoint -e '123'
  Time (mean ± σ):      2.056 s ±  0.028 s    [User: 0.004 s, System: 0.019 s]
  Range (min … max):    2.009 s …  2.112 s    10 runs

Benchmark 1: grep -r /home/akira/ghq/github.com/akawashiro/ros3fs/build_benchmark/mountpoint-s3_mountpoint -e '123'
  Time (mean ± σ):     10.866 s ±  2.068 s    [User: 0.007 s, System: 0.023 s]
  Range (min … max):    8.913 s … 14.761 s    10 runs

=========================================================
========== Compare find performance without cache warmup ==========                                                                                                time find /home/akira/ghq/github.com/akawashiro/ros3fs/build_benchmark/ros3fs_mountpoint

real    0m0.022s
user    0m0.005s
sys     0m0.000s
time find /home/akira/ghq/github.com/akawashiro/ros3fs/build_benchmark/s3fs-fuse_mountpoint

real    0m0.090s
user    0m0.001s
sys     0m0.000s
time find /home/akira/ghq/github.com/akawashiro/ros3fs/build_benchmark/mountpoint-s3_mountpoint

real    0m0.045s
user    0m0.000s
sys     0m0.003s
============================================================
========== Compare find performance with cache warmup ==========
Benchmark 1: find /home/akira/ghq/github.com/akawashiro/ros3fs/build_benchmark/ros3fs_mountpoint
  Time (mean ± σ):       4.0 ms ±   0.2 ms    [User: 1.1 ms, System: 0.4 ms]
  Range (min … max):     3.7 ms …   4.5 ms    10 runs

  Warning: Command took less than 5 ms to complete. Results might be inaccurate.

Benchmark 1: find /home/akira/ghq/github.com/akawashiro/ros3fs/build_benchmark/s3fs-fuse_mountpoint
  Time (mean ± σ):      22.5 ms ±   1.9 ms    [User: 0.8 ms, System: 0.3 ms]
  Range (min … max):    19.2 ms …  26.0 ms    10 runs

Benchmark 1: find /home/akira/ghq/github.com/akawashiro/ros3fs/build_benchmark/mountpoint-s3_mountpoint
  Time (mean ± σ):      43.8 ms ±   2.5 ms    [User: 1.1 ms, System: 0.4 ms]
  Range (min … max):    41.2 ms …  49.3 ms    10 runs

=========================================================
```
</details>

### `grep`を使ったバケット内のファイル内容の検索
|                                | ros3fs  | s3fs-fuse | mountpoint-s3 |
| ------------------------------ | ------- | --------- | ------------- |
| キャッシュのウォームアップなし | 3046ms  | 2042ms    | 8660ms        |
| キャッシュのウォームアップあり | 15.2 ms | 2056 ms   | 10866 ms      |

### `find`を使ったバケット内のファイルの列挙
|                                | ros3fs | s3fs-fuse | mountpoint-s3 |
| ------------------------------ | ------ | --------- | ------------- |
| キャッシュのウォームアップなし | 22ms   | 90ms      | 45ms          |
| キャッシュのウォームアップあり | 4.0 ms | 22.5 ms   | 43.8 ms       |

## まとめ

S3 互換のオブジェクトストレージのための FUSE、[ros3fs](https://github.com/akawashiro/ros3fs)を実装しました。[ros3fs](https://github.com/akawashiro/ros3fs)は読み込み専用かつバケットのデータの更新を反映しないという強い制約のもとではありますが、既存実装に比べて非常に高速なデータの閲覧が可能にしました。

[^1]: Windows で FUSE を利用するのはあまり一般的ではないようです。[WinFsp](https://github.com/winfsp/winfsp)がありますが使ったことはありません。
