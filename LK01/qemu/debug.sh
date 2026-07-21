#!/bin/sh
qemu-system-x86_64 \
    -m 64M \
    -nographic \
    -kernel bzImage \
    -append "console=ttyS0 loglevel=3 oops=panic panic=-1 nopti nokaslr" \
    -no-reboot \
    -cpu kvm64,+smep \
    -smp 1 \
    -monitor /dev/null \
    -initrd debug.cpio \
    -net nic,model=virtio \
    -net user \
    -gdb tcp::12345
