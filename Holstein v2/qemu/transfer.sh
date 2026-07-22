#!/bin/sh
musl-gcc exploit.c -o exploit -static
mv exploit root
cd root; find . -print0 | cpio -o --null --format=newc --owner root > ../debug.cpio
cd ../