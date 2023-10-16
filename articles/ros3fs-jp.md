# ros3fs
## オブジェクトストレージ
[AWS S3](https://aws.amazon.com/s3/)や[Cloudflare R2](https://developers.cloudflare.com/r2/)、[MinIO](https://min.io/)、[Apache Ozone](https://ozone.apache.org/)などはオブジェクトストレージと呼ばれるソフトウェアです。オブジェクトストレージはスケーラビリティにすぐれており、[AWS S3](https://aws.amazon.com/s3/)や[Cloudflare R2](https://developers.cloudflare.com/r2/)などのクラウドサービスとして提供されている場合は手軽に安価に使い始めることができます。

オブジェクトストレージはファイルをオブジェクトという単位で扱います。オブジェクトはフラットな空間に格納され、ディレクトリという概念はありません。アクセスには通常REST APIを使います。[AWS S3](https://aws.amazon.com/s3/)が策定したAPIがよく知られており、[Cloudflare R2](https://developers.cloudflare.com/r2/)、[MinIO](https://min.io/)、[Apache Ozone](https://ozone.apache.org/)はどれも[AWS S3](https://aws.amazon.com/s3/)互換のAPIを実装しています。このため、これらのソフトウェアはすべて[AWS S3 cli](https://docs.aws.amazon.com/cli/latest/reference/s3/)経由で利用することができます。

[AWS S3](https://aws.amazon.com/s3/)を含むオブジェクトストレージは手軽なのですが、一つ欠点があります。それはデータを通常のファイルのように閲覧できない点です。オブジェクトストレージ上のデータを直接`cat`や`grep`することはできませんし、[GNOME Files](https://gitlab.gnome.org/GNOME/nautilus)のようなGUIのファイルマネージャで見ることもできません。

## FUSE
このような問題を解決する一つの方法が[FUSE](https://www.kernel.org/doc/html/next/filesystems/fuse.html)です。[FUSE](https://www.kernel.org/doc/html/next/filesystems/fuse.html)とはFilesystem in Userspaceの略であり、[Linux](https://github.com/libfuse/libfuse)、[Mac OS](https://osxfuse.github.io/)、Windows[^1]で利用できる技術です。

[FUSE](https://www.kernel.org/doc/html/next/filesystems/fuse.html)を使うとファイルシステムをユーザ空間内に実装できます。このファイルシステムの実装を自由に書くことができるため、例えば`open`システムコールをAWS S3の[GetObject API](https://docs.aws.amazon.com/AmazonS3/latest/API/API_GetObject.html)に結び付けることで、ファイルを開く操作を自動的にS3のオブジェクトを取得する操作に変換することができます。

また、オブジェクトストレージにはディレクトリ構造がありませんが、FUSEを使って模擬的にそれを実現することも可能です。オブジェクト名を"/"などのパス区切り文字で区切ったものをディレクトリとしてユーザに見せるようにFUSEを実装すれば、オブジェクトストレージがディレクトリを持ったファイルシステムのように見せることができます。

## 既存のs3向けFUSEの問題点
S3互換のAPIを持つオブジェクトストレージ向けのFUSEはAWS本家による[mountpoint-s3](https://github.com/awslabs/mountpoint-s3)や[s3fs-fuse](https://github.com/s3fs-fuse/s3fs-fuse)などの既存実装があります。どちらを使っても問題なくオブジェクトストレージをファイルシステムとして利用することができます。

しかし、この二つの実装にはどちらも**遅い**という問題点があります。特にディレクトリ構造を走査するような操作は遅く、オブジェクト数の多いバケットをFUSEでマウントして`ls`コマンドを打つと体感できる程度の遅延があります。これはおそらく、dエントリのようなディレクトリ構造を保持する専用のデータ構造がないオブジェクトストレージを無理やりファイルシステムとして見せているために生じる遅延であり、根本的に解決することが困難な問題です。

## ros3fs
この問題を回避して`ls`の遅延を最小化するために作ったのが[ros3fs](https://github.com/akawashiro/ros3fs)です。[ros3fs](https://github.com/akawashiro/ros3fs)はS3互換のオブジェクトストレージのためのFUSEであり、ディレクトリ構造の走査を含む読み出しの遅延を最小化することを目的に設計されています。 

## 性能比較

```bash
Benchmark 1: grep -Rnw /home/akira/ghq/github.com/akawashiro/ros3fs/build_compare_with_mountpoint-s3/ros3fs_mountpoint -e '123'
  Time (mean ± σ):      15.0 ms ±   2.5 ms    [User: 1.7 ms, System: 4.4 ms]
  Range (min … max):    11.7 ms …  31.6 ms    198 runs

+ hyperfine --ignore-failure --warmup 3 'grep -Rnw /home/akira/ghq/github.com/akawashiro/ros3fs/build_compare_with_mountpoint-s3/s3fs-fuse_mountpoint -e '\''123'\'''
Benchmark 1: grep -Rnw /home/akira/ghq/github.com/akawashiro/ros3fs/build_compare_with_mountpoint-s3/s3fs-fuse_mountpoint -e '123'
  Time (mean ± σ):      2.062 s ±  0.043 s    [User: 0.002 s, System: 0.023 s]
  Range (min … max):    2.002 s …  2.153 s    10 runs

+ hyperfine --ignore-failure --warmup 3 'grep -Rnw /home/akira/ghq/github.com/akawashiro/ros3fs/build_compare_with_mountpoint-s3/mountpoint-s3_mountpoint -e '\''123'\'''
Benchmark 1: grep -Rnw /home/akira/ghq/github.com/akawashiro/ros3fs/build_compare_with_mountpoint-s3/mountpoint-s3_mountpoint -e '123'
  Time (mean ± σ):     10.904 s ±  2.504 s    [User: 0.003 s, System: 0.028 s]
  Range (min … max):    8.313 s … 15.699 s    10 runs
```

[^1]: WindowsでFUSEを利用するのはあまり一般的ではないようです。[WinFsp](https://github.com/winfsp/winfsp)がありますが使ったことはありません。