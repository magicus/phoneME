#! /bin/sh

#
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version
# 2 only, as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the GNU
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

: 'This file is in the public domain, so clarified as of'
: '2006-07-17 by Arthur David Olson.'

: '@(#)yearistype.sh	8.2'

case $#-$1 in
	2-|2-0*|2-*[!0-9]*)
		echo "$0: wild year - $1" >&2
		exit 1 ;;
esac

case $#-$2 in
	2-even)
		case $1 in
			*[24680])			exit 0 ;;
			*)				exit 1 ;;
		esac ;;
	2-nonpres|2-nonuspres)
		case $1 in
			*[02468][048]|*[13579][26])	exit 1 ;;
			*)				exit 0 ;;
		esac ;;
	2-odd)
		case $1 in
			*[13579])			exit 0 ;;
			*)				exit 1 ;;
		esac ;;
	2-uspres)
		case $1 in
			*[02468][048]|*[13579][26])	exit 0 ;;
			*)				exit 1 ;;
		esac ;;
	2-*)
		echo "$0: wild type - $2" >&2 ;;
esac

echo "$0: usage is $0 year even|odd|uspres|nonpres|nonuspres" >&2
exit 1
