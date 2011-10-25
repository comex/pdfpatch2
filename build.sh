#!/bin/sh
set -e
/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/gcc-4.2 -arch armv7 -isysroot /var/sdk -Wall -dynamiclib -I.. -L.. -lsubstrate -o pdfpatch2.dylib pdfpatch2.c
cp -a pdfpatch2.{dylib,plist} root/Library/MobileSubstrate/DynamicLibraries/
DYLD_INSERT_LIBRARIES=/usr/src/fauxsu/fauxsu.dylib dpkg-deb -b root pdfpatch2_1_iphoneos-arm.deb

