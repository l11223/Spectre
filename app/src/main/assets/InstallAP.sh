#!/bin/sh
# By SakuraKyuo

OUTFD=/proc/self/fd/$2

function ui_print() {
  echo -e "ui_print $1\nui_print" >> $OUTFD
}

function ui_printfile() {
  while IFS='' read -r line || $BB [[ -n "$line" ]]; do
    ui_print "$line";
  done < $1;
}

function kernelFlagsErr(){
	ui_print "- Installation has Aborted!"
	ui_print "- Spectre requires CONFIG_KALLSYMS to be Enabled."
	ui_print "- But your kernel seems NOT enabled it."
	exit
}

function patchNote(){
	ui_print "- Patch Done"
	ui_print "- Patch Key is: Ap$skey"
	ui_print "- We do have saved Origin Boot image to /data"
	ui_print "- If you encounter bootloop, reboot into Recovery and flash it"
	exit
}

function failed(){
	ui_printfile /dev/tmp/install/log
	ui_print "- Patch Failed."
	ui_print "- Please feedback to the developer with the screenshots."
	exit
}

function boot_execute_ab(){
	./lib/arm64-v8a/libbootimg.so unpack boot.img
	if [[ ! $(./lib/arm64-v8a/libsystools.so -i ./kernel -f | grep CONFIG_KALLSYMS=y) ]]; then
		kernelFlagsErr
	fi
	mv kernel kernel-origin
	./lib/arm64-v8a/libsystools.so -p --image kernel-origin --skey "Ap$skey" --kpimg ./assets/kpimg --out ./kernel 2>&1 | tee /dev/tmp/install/log
	if [[ ! $(cat /dev/tmp/install/log | grep "patch done") ]]; then
		failed
	fi
	ui_printfile /dev/tmp/install/log
	./lib/arm64-v8a/libbootimg.so repack boot.img
	dd if=/dev/tmp/install/new-boot.img of=/dev/block/by-name/boot$slot
	mv boot.img /data/boot.img
	patchNote
}

function boot_execute(){
	./lib/arm64-v8a/libbootimg.so unpack boot.img
	if [[ ! $(./lib/arm64-v8a/libsystools.so -i ./kernel -f | grep CONFIG_KALLSYMS=y) ]]; then
		kernelFlagsErr
	fi
	mv kernel kernel-origin
	./lib/arm64-v8a/libsystools.so -p --image kernel-origin --skey "Ap$skey" --kpimg ./assets/kpimg --out ./kernel 2>&1 | tee /dev/tmp/install/log
	if [[ ! $(cat /dev/tmp/install/log | grep "patch done") ]]; then
		failed
	fi
	ui_printfile /dev/tmp/install/log
	./lib/arm64-v8a/libbootimg.so repack boot.img
	dd if=/dev/tmp/install/new-boot.img of=/dev/block/by-name/boot$slot
	mv boot.img /data/boot.img
	patchNote
}

function main(){

cd /dev/tmp/install

chmod a+x ./assets/kpimg
chmod a+x ./lib/arm64-v8a/libsystools.so
chmod a+x ./lib/arm64-v8a/libbootimg.so

slot=$(getprop ro.boot.slot_suffix)

skey=$(cat /proc/sys/kernel/random/uuid | cut -d \- -f1)

if [[ ! "$slot" == "" ]]; then

	ui_print ""
	ui_print "- You are using A/B device."

	# Script author
	ui_print "- Install Script by SakuraKyuo"

	# Get kernel
	ui_print ""
	dd if=/dev/block/by-name/boot$slot of=/dev/tmp/install/boot.img
	if [[ "$?" == 0 ]]; then
		ui_print "- Detected boot partition."
		boot_execute_ab
	fi

else

	ui_print "You are using A Only device."

	# Get kernel
	ui_print ""
	dd if=/dev/block/by-name/boot of=/dev/tmp/install/boot.img
	if [[ "$?" == 0 ]]; then
		ui_print "- Detected boot partition."
		boot_execute
	fi

fi

}

main
