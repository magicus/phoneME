#
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

# This script converts the line separated absolute paths passed in
# on stdin to be relative to the absolute path passed in the PWD
# variable. For example:
#     echo /a/b/c/d | awk -F/ -f abs2rel.awk PWD=/a/b/x/y
# will produce "../../c/d"

{
    if ($1 != "") {
        # relative already
        print $0
        next
    }
    S = ""
    N = split(PWD, comps, "/")
    for (i = 2; i <= N; ++i) {
        if (comps[i] != $i) {
            for (j = i; j < N; ++j) {
                S = S "../"
            }
            S = S ".."
            break
        }
    }
    if (S == "") S = "."
    for (j = i; j <= NF; ++j) {
        if (S == "") {
            S = $j
        } else {
            S = S "/" $j
        }
    }
    print S
    next
}
