#!/bin/bash
. ~/.bashrc

#define inhouse path and gpl path
inhousePath=$1
gplPath=$2

#check inhouse path
if [ -z $1 ]; then
    echo "[ERROR] The INHOUSE path does not exist"
    exit
fi

#check gpl path
if [ -z $2 ]; then
    echo "[WARNN] The GPL path does not exist, will add new GPL path..."
    gplPath=$inhousePath/../../gpl/alps
    mkdir -p $gplPath
fi

#check if inhouse kernel path exist
if [ ! -d $inhousePath/kernel ]; then
    echo "[ERROR] Can not pull out gpl part, the INHOUSE kernel folder does not exist"
    exit
fi

#check if gpl kernel path empty
if [ -d $gplPath/kernel ]; then
    echo "[WARNN] The GPL kernel folder is not empty, remove kernel..."
    rm -rf $gplPath/kernel
fi

#do the move action
echo "Pulling GPL from INHOUSE path..."
mkdir -p $gplPath/bootable/bootloader/
mv $inhousePath/kernel/ $gplPath/kernel/

#check result
cd $inhousePath
if [ -d kernel/ ]; then
    echo "[ERROR] The inhouse folder has kernel folder"
    exit
fi
cd -

cd $gplPath
if [ ! -d kernel/ ]; then
    echo "[ERROR] The gpl folder does not have kernel folder"
    exit
fi
cd -

echo "Successful~!"
echo "INHOUSE path: $inhousePath"
echo "GPL     path: $gplPath"
