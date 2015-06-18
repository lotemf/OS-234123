#!/bin/bash
echo Copying

cd /root/tests/HW4
cp /mnt/hgfs/vm_shared_folder/HW4/Not_In_Kernel/* ./ -r

$SHELL
