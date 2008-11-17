#!/bin/bash
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

OPTQ=-q
TMP=/tmp/checkout$$
trap 'rm -f $TMP' 0 1 2 3 15

function usage {
cat <<- \EOF
	usage: checkout.sh [-nv] [specfile]
	-n     print svn commands but don't execute them
	-v     run the svn commands without the -q option

	This script creates and maintains a family of svn working copies.
	The specfile describes the mapping between each working copy and
	its related portion of a svn repository.

	The specfile must contain one or more lines of the form:

	    URL  dirname

	If dirname does not exist, 'svn co' is done from that URL.
	If dirname exists and is mapped to that URL, 'svn update' is done.
	If dirname exists and is mapped to a different URL, 'svn switch'
	is done on that URL.
	EOF
}

while getopts 'nv' ARG ; do
    case $ARG in
        n)
            OPTN=1
            ;;
        v)
            OPTQ=
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done

shift $(( $OPTIND-1 ))
if [ $# -gt 0 -a -r "$1" ] ; then
    exec < $1
fi

while read repourl base ; do
    # echo '<' $repourl $base
    if [ ! -d $base ] ; then
        cmd=(svn co $OPTQ $repourl $base)
    elif svn info $base > $TMP ; then
        wcurl=$(awk '$1 == "URL:" { print $2 }' $TMP)
        if [ "$repourl" = "$wcurl" ] ; then
            cmd=(svn update $OPTQ $repourl $base)
        else
            cmd=(svn switch $OPTQ $repourl $base)
        fi
    else
        continue
    fi

    echo ${cmd[*]}
    if [ ! "$OPTN" ]; then
        "${cmd[@]}"
    fi
done
