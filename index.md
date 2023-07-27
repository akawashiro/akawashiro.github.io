---
title: Akira Kawata
---

# Akira Kawata <!-- omit in toc -->

## Table of Contents <!-- omit in toc -->

- [Interests](#interests)
- [Links](#links)
- [Articles](#articles)
- [Publication](#publication)
  - [APLAS 2019](#aplas-2019)
  - [Egison Workshop 2018](#egison-workshop-2018)
  - [JSSST2018](#jssst2018)
  - [ICFP 2020 Haskell Implementation Workshop](#icfp-2020-haskell-implementation-workshop)
  - [Private talk](#private-talk)
  - [Kernel/VM 探検隊 online part5 ](#kernelvm探検隊online-part5-)
- [Activities](#activities)
- [Work Experience](#work-experience)
- [GPG Public Key](#gpg-public-key)
- [Software and Hardware](#software-and-hardware)
  - [ros3fs](#ros3fs)
  - [sloader](#sloader)
  - [jendeley](#jendeley)
  - [z3shape](#z3shape)
  - [sold](#sold)
  - [kearch](#kearch)
  - [typed-egison](#typed-egison)
  - [wavplayer](#wavplayer)
  - [cropass](#cropass)
  - [hinvader](#hinvader)
  - [ml2wasm](#ml2wasm)
  - [N-body-simulation](#n-body-simulation)

## Interests

- Executable and Linkable Format of Linux
- Linux kernel inside
- Program analysis

## Links

- [GitHub](https://github.com/akawashiro)
- [Blog](http://a-kawashiro.hatenablog.com/)
- [Twitter](https://twitter.com/a_kawashiro)
- [Mastodon](https://mstdn.jp/@a_kawashiro)
- [Misskey](https://misskey.io/@a_kawashiro)
- [LinkedIn](https://www.linkedin.com/in/akirakawata/)
- [keybase](https://keybase.io/a_kawashiro)
- [Zenn](https://zenn.dev/a_kawashiro)

## Articles

- [Linux のローダを自作する](https://zenn.dev/a_kawashiro/articles/506a224d206418)
- [Linux におけるデバイスファイルの仕組み](https://zenn.dev/a_kawashiro/articles/387fa97163dd66)
- [jendeley - JSON ベースの文書管理ソフトウェア](https://zenn.dev/a_kawashiro/articles/a2170f967f9508)
- [Linux kernel をソースコードからビルド、インストールするシェルスクリプトを書いた](./articles/linux-build-script.md)
- [LD_AUDIT and Global Offset Table (in Japanese)](https://a-kawashiro.hatenablog.com/entry/2022/01/08/220526)
- [What is GNU_IFUNC? (in Japanese)](https://a-kawashiro.hatenablog.com/entry/2021/11/07/100540)
- [What and how does libc save information into jmp_buf used in setjmp and longjmp? (in Japanese)](https://a-kawashiro.hatenablog.com/entry/2020/12/31/184339)
- [JAX から MN-Core を利用する](https://tech.preferred.jp/ja/blog/jax-on-mncore/)
  - As the main mentor, I made the prototype of XLA to ONNX converter and support Maekawa-san during intern.

## Publication

### [Kernel/VM 探検隊@東京 No16](https://kernelvm.connpass.com/event/287261/)

- Talk on sloader
- [自作ローダのための libc 初期化ハック](https://akawashiro.github.io/sloader-libc-hacks.pdf)

### [Kernel/VM 探検隊 online part5 ](https://kernelvm.connpass.com/event/256248/)

- Short talk on AT_RANDOM
- [Slide: 補助ベクトルとプロセスのロード](https://akawashiro.github.io/auxval_kernelvm_20220828.pdf)

### [ICFP 2020 Haskell Implementation Workshop](https://icfp20.sigplan.org/details/hiw-2020-papers/10/Sweet-Egison-a-Haskell-Library-for-Non-Deterministic-Pattern-Matching)

- Sweet Egison: a Haskell Library for Non-Deterministic Pattern Matching

### [APLAS 2019](https://conf.researchr.org/home/aplas-2019)

- A Dependently Typed Multi-Stage Calculus -- This is a formal multistage calculus with dependent types. We design its type system and prove type soundness.
- [Paper on axiv](https://arxiv.org/abs/1908.02035)

### [Egison Workshop 2018](https://connpass.com/event/102061/)

- Introduction to the type system of Egison.
- [Handout of the type system](https://akawashiro.github.io/EgisonTypingrules.pdf)
- [Slide](https://akawashiro.github.io/EgisonTypeSystem.pdf)

### [JSSST2018](https://jssst2018.wordpress.com/)

- Type theory for multistage programming including dependent types
- [Paper](http://jssst.or.jp/files/user/taikai/2018/PPL/ppl1-3.pdf)

### Private talk

- [Slide: /lib64/ld-linux-x86-64.so.2](https://docs.google.com/presentation/d/1WPxr6d_me_QU3mRWxBzs7y2iPhwV2YeAoB4EGcG9H90/edit?usp=sharing)
- Random talk on glibc dynamic linker of Linux

## Activities

- [The 11th Japan Olympiad in Informatics, Silver medal](https://www.ioi-jp.org/joi/2011/2012-medalists.html)
- [IPA The MITOH Project 2018](https://www.ipa.go.jp/jinzai/mitou/2018/gaiyou_s-2)
  - Distibuted Search Engine Using Focus Crawling and Federated Search

## Work Experience

- Software engineer, Japan, Preferred Networks, April 2020
- Internship, Japan, Rakuten Institute of Technology, March 2018 - April 2018

## GPG Public Key

- Fingure print: 3FB4269CA58D57F0326C1F7488737135568C1AC5
- [Public key file](9804D984406FEE5605D5CB82A8DEC03E3DF3BDAD.html)

## Software and Hardware

### ros3fs

- ros3fs, Read Only S3 File System, is a Linux FUSE adapter for AWS S3 and S3 compatible object storage such as Apache Ozone. ros3fs focuses on speed, only speed.
- [Repository](https://github.com/akawashiro/ros3fs)

### sloader

- `sloader` is an ELF loader that aims to replace `ld-linux.so.2` of `glibc`.
- [Introduction article (in Japanese)](https://zenn.dev/a_kawashiro/articles/506a224d206418)
- [Introduction article (in English)](https://akawashiro.github.io/articles/sloader-succeeded-self-build-en)
- [Repository](https://github.com/akawashiro/sloader)

### jendeley

- jendeley is a JSON based document organizing software.
- [Document](https://akawashiro.github.io/jendeley/)
- [Repository](https://github.com/akawashiro/jendeley)
- [Introduction article (in Japanese)](https://zenn.dev/a_kawashiro/articles/a2170f967f9508)
- [Introduction slides (in Japanese)](./jendeley-KMC-reikai-slide.pdf)

### z3shape

- z3shape infer shapes in ONNX file using Z3 SMT solver.
- [Repository](https://github.com/akawashiro/z3shape)

### sold

- sold is a linker software which links depending shared objects to a binary or a shared object.
- [Repository](https://github.com/akawashiro/sold)
- [Presentation in kernelvm (in Japanese)](./sold_kernelvm_20211120.pdf)
- [Demo](https://www.youtube.com/watch?v=f6EMyVrq3jo)

### kearch

- kearch is a distributed search engine made from scratch. There are two type of search engine -- specialized and meta. A meta search engine takes query from uses and distribute it to appropriate
  specialized search engine.
- [Repository](https://github.com/kearch/kearch)
- [Final presenation of MITOU project (in Japanese)](kearchFinalPresentation.pdf)
- [Demo movie in the final presenation (in Japanese)](https://youtu.be/tErMAEk8wLQ)

### typed-egison

- typed-egison is a type checker for Egison programming language. I designed all typing rules for Egison and make type inferencer for Egison with Haskell. You can use it from interpreter.
- [Repository](https://github.com/egison/typed-egison)

### wavplayer

- wavplayer is a mini music player using ATMEGA168 and SD card. I wrote FAT16 file system for ATMEGA168. Musics are stored in wav files in the root directory.
- [Repository](https://github.com/akawashiro/wavplayer)

### cropass

- cropass is a password manager written in Go. I wrote this to use myself.
  You can generate, add and lookup password records.
- [Repository](https://github.com/akawashiro/cropass)

### hinvader

- hinvader is a simple invader like game wirtten in Haskell. I used GLUT to reder the screen.
- [Repository](https://github.com/akawashiro/hinvader)

### ml2wasm

- ml2wasm is a compiler from ML like language (MinCaml) to WebAssembly.
- [Repository](https://github.com/akawashiro/ml2wasm)

### N-body-simulation

- N-body-simulation is a toy n-body simulator.
- [Repository](https://github.com/akawashiro/N-body-simulation)
