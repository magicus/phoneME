#
# @(#)README.txt	1.6 06/10/25
#
# Copyright  2008  Sun Microsystems, Inc. All rights reserved.

# 
# 
# This directory contains tools used to make the byte array
# in IxcClassLoader.  That byte array contains the class definition
# for com.sun.xlet.Utils.  If a change is needed to
# com.sun.xlet.Utils, rename the two files in this directory
# to ".java".  You need to compile Utils in a temporary
# directory called com/sun/xlet, of course.
# 
# Once you've compiled Utils, run Main over the .class
# file to generate a byte array declaration.  Then, paste this
# into IxcClassLoader.  So, for example, to build this, do
# something like the following:

mkdir -p com/sun/xlet
cat Utils.gen > com/sun/xlet/Utils.java
javac com/sun/xlet/Utils.java
if [[ $? != 0 ]] ; then
    exit;
fi
mkdir -p tmp
mv com/sun/xlet/Utils.class tmp
rm -rf com
cat Main.gen > tmp/Main.java
cd tmp
javac Main.java
java Main < Utils.class > generated.txt
echo "Paste tmp/generated.txt into IxcClassLoader.java, then rm -rf tmp."

