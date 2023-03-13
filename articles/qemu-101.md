# QEMUでUbuntuの仮想マシンを作ってsshログインする
```
設定項目 	データ型 	本 Web ページでの設定値
仮想マシン・イメージファイルのファイル名 	文字列 	/home/os001.qcow2
仮想マシン・イメージファイルのフォーマット 	文字列 	qcow2
仮想マシン・イメージファイルの最大サイズ 	数値 	80 (GB)
ゲスト OS の種類 	文字列 	precise (Ubuntu 22.04)
仮想マシンに割り当てるメインメモリのサイズ 	数値 	2048 (MB)
仮想マシンの起動に使う ISO イメージファイル名 	文字列 	/home/ubuntu-22.04-desktop-amd64.iso
```

```
HOST_NAME=rausu3
DISK_PATH=${HOME}/${HOST_NAME}.qcow3
DISK_SIZE_GB=80GB
MEMORY_SIZE_MB=2048
PORT_FORWARD=5555

qemu-img create -f qcow2 ${DISK_PATH} ${DISK_SIZE_GB}
qemu-system-x86_64 -hda ${DISK_PATH} -m ${MEMORY_SIZE_MB} -cdrom ~/Downloads/ubuntu-22.04.1-desktop-amd64.iso -boot d --enable-kvm -usb -serial none -parallel none
# Do Ubuntu install here
# sudo apt install openssh-server
qemu-system-x86_64 -hda ${DISK_PATH} -m ${MEMORY_SIZE_MB} -boot d --enable-kvm -usb -serial none -parallel none -smp $(nproc) -device e1000,netdev=net0 -netdev user,id=net0,hostfwd=tcp::${PORT_FORWARD}-:22
ssh localhost -p ${PORT_FORWARD} "mkdir /home/akira/.ssh/"
scp -P ${PORT_FORWARD} ~/.ssh/id_rsa.pub localhost:~/.ssh/authorized_keys
```
## 参考
- [QEMU をインストール，Ubuntu 仮想マシンの作成（Ubuntu 上](https://www.kkaneko.jp/tools/ubuntu/ubuntuqemu.html)
- [QEMU/Options](https://wiki.gentoo.org/wiki/QEMU/Options)
