#!/bin/bash
echo Copying
cd /usr/src/linux-2.4.18-14custom/kernel
cp /mnt/hgfs/SharedFolder/kernel/* ./ -r

cd /usr/src/linux-2.4.18-14custom/include
cp /mnt/hgfs/SharedFolder/include/* ./ -r

cd /usr/src/linux-2.4.18-14custom/arch
cp /mnt/hgfs/SharedFolder/arch/* ./ -r

cd /root/tests/HW2
cp /mnt/hgfs/SharedFolder/Not_In_Kernel/* ./ -r

$SHELL
