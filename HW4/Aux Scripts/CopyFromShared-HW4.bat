#!/bin/bash
echo Copying

cd /root/tests/HW4
cp /mnt/hgfs/SharedFolder/Not_In_Kernel/* ./ -r

$SHELL
