#
#
#
# Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version
# 2 only, as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License version 2 for more details (a copy is
# included at /legal/license.txt).
# 
# You should have received a copy of the GNU General Public License
# version 2 along with this work; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301 USA
# 
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
# Clara, CA 95054 or visit www.sun.com if you need additional
# information or have any questions.
#
######################################################################
#

$ROMSTRUCTS_H  = $ARGV[0];

system("cp $ROMSTRUCTS_H $ROMSTRUCTS_H.orig");
open(IN,  "$ROMSTRUCTS_H.orig");
open(OUT, ">$ROMSTRUCTS_H");

print OUT "/*\n";
print OUT " * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.\n";
print OUT " * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER\n";
print OUT " * \n";
print OUT " * This program is free software; you can redistribute it and/or\n";
print OUT " * modify it under the terms of the GNU General Public License version\n";
print OUT " * 2 only, as published by the Free Software Foundation.\n";
print OUT " * \n";
print OUT " * This program is distributed in the hope that it will be useful, but\n";
print OUT " * WITHOUT ANY WARRANTY; without even the implied warranty of\n";
print OUT " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU\n";
print OUT " * General Public License version 2 for more details (a copy is\n";
print OUT " * included at /legal/license.txt).\n";
print OUT " * \n";
print OUT " * You should have received a copy of the GNU General Public License\n";
print OUT " * version 2 along with this work; if not, write to the Free Software\n";
print OUT " * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA\n";
print OUT " * 02110-1301 USA\n";
print OUT " * \n";
print OUT " * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa\n";
print OUT " * Clara, CA 95054 or visit www.sun.com if you need additional\n";
print OUT " * information or have any questions.\n";
print OUT " */\n\n";
print OUT "/* This file is auto-generated. Do not edit*/\n\n";
print OUT "#ifndef _ROM_STRUCTS_H_\n";
print OUT "#define _ROM_STRUCTS_H_\n\n";
print OUT "#define int32_t int\n\n";
print OUT "#define CVM_OBJ_HEADER void* dummy1; void* dummy2\n\n";
print OUT "typedef struct {\n";
print OUT "    CVM_OBJ_HEADER;\n";
print OUT "    int length;\n";
print OUT "    jbyte elements[1];\n";
print OUT "} jbyte_array;\n";
print OUT "\n";
print OUT "typedef struct {\n";
print OUT "    CVM_OBJ_HEADER;\n";
print OUT "    int length;\n";
print OUT "    jchar elements[1];\n";
print OUT "} jchar_array;\n";
print OUT "\n";
print OUT "typedef struct {\n";
print OUT "    CVM_OBJ_HEADER;\n";
print OUT "    int length;\n";
print OUT "    jint elements[1];\n";
print OUT "} jint_array;\n\n";
print OUT "typedef struct {\n";
print OUT "    CVM_OBJ_HEADER;\n";
print OUT "    int length;\n";
print OUT "    jint elements[1];\n";
print OUT "} jobject_array;\n\n";

$isInStruct = "false";

while ($_ = <IN>) {
    if (/typedef struct Class/) {
        s/typedef struct Class/struct Java_/g;
	$isInStruct = "true";
    }

    # Write out the structs.
    if ($isInStruct eq "true") {
        if (/struct Java_/) {
            print OUT $_;
            print OUT "    CVM_OBJ_HEADER;\n";
        } else {
            if (/} Class/) {
                # end of the current struct.
                $isInStruct = "false";
                print OUT "};\n\n"
            } else {
                if (/#define|#undef/) {
                    # Skip the #define and #undef.
                } else {
                    s/struct HArrayOfByte/jbyte_array/g;
		    s/struct HArrayOfChar/jchar_array/g;
		    s/struct Hjavax/struct Java_javax/g;
                    print OUT $_;
                }
            }
        }
    }
}

print OUT "#endif /* _ROM_STRUCTS_H_ */\n";

close(IN);
close(OUT);

unlink("$ROMSTRUCTS_H.orig");

#print("$ROMSTRUCTS_H done.\n");

