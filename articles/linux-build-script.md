# Linux kernelをソースコードからビルド、インストールするシェルスクリプトを書いた

スクリプトは [https://github.com/akawashiro/public-tools/blob/master/linux-install.sh](https://github.com/akawashiro/public-tools/blob/master/linux-install.sh) にある。

## 使い方

```
./linux-install.sh
```
でカーネルをビルドする。`${HOME}/linux`にソースコードが、`${HOME}/linux-build`にビルド生成物ができる。 `compile_command.json` が生成されるので適切なエディタのプラグインに読み込ませればタグジャンプが効くようになる。

```
INSTALL_PACKAGES=yes DELETE_SAME_NAME_KERNELS=yes INSTALL_BUILT_KERNEL=yes REBOOT_AFTER_INSTALL=yes ./linux-install.sh
```
でカーネルをビルドしてインストールまでする。`LINUX_REPOSITORY`でリポジトリを`BRANCH_NAME` で好きなブランチに変更できる。