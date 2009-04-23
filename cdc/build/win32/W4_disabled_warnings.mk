#
# Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

# These are all warnings you probably want to disable if you ever
# enable /W4

# function undefined; assuming extern returning int
CCFLAGS		+= /wd4013
# 'type cast' : from data pointer 'void *' to function pointer
CCFLAGS		+= /wd4055
# unreferenced local variable
CCFLAGS		+= /wd4101
# unreferenced label
CCFLAGS		+= /wd4102
# nonstandard extension, function/data pointer conversion in expression
CCFLAGS		+= /wd4152
# local variable is initialized but not referenced
CCFLAGS		+= /wd4189
# nonstandard extension used : function given file scope
CCFLAGS		+= /wd4210
# nonstandard extension used : redefined extern to static
CCFLAGS		+= /wd4211
# nonstandard extension used : cast on l-value
CCFLAGS		+= /wd4213
# nonstandard extension used : must specify at least a storage class or a type
CCFLAGS		+= /wd4218
# cast truncates constant value
CCFLAGS		+= /wd4310
# missing type specifier - int assumed. 
CCFLAGS		+= /wd4431
# unreferenced local function has been removed
CCFLAGS		+= /wd4505
#  '&' : check operator precedence for possible error; use parentheses
CCFLAGS		+= /wd4554
# potentially uninitialized local variable
#CCFLAGS		+= /wd4701
# assignment within conditional expression
CCFLAGS		+= /wd4706
# forcing value to bool 'true' or 'false' (performance warning)
CCFLAGS		+= /wd4800


# > signed/unsigned mismatch
CCFLAGS		+= /wd4018
# type cast to void* function pointer
CCFLAGS		+= /wd4054
# differs in indirection to slightly different base types
CCFLAGS		+= /wd4057
# unreferenced formal parameter
CCFLAGS		+= /wd4100
# named type definition in parentheses
CCFLAGS		+= /wd4115
# conditional expression is a constant
CCFLAGS		+= /wd4127
#  '==' : logical operation on address of string constant
CCFLAGS		+= /wd4130
# uses old-style function declarator
CCFLAGS		+= /wd4131
# nonstandard extension used : nameless struct/union
CCFLAGS		+= /wd4201
# nonstandard extension used : translation unit is empty
CCFLAGS		+= /wd4206
# nonstandard extension used : bit field types other than int
CCFLAGS		+= /wd4214
# return valie signed/unsigned mismatch
CCFLAGS		+= /wd4245
# possible loss of data with implicit cast to smaller size
CCFLAGS		+= /wd4244
# != or == signed/unsigned mismatch
CCFLAGS		+= /wd4389
# comma operator within array index expression
CCFLAGS		+= /wd4709
