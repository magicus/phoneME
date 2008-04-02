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
#
# make_fs_image.tcl -- create a C source file that contains a simulated,
#                      read-only file system image.
#
# Usage:
#
#    tclsh make_fs_image.tcl <outfile> <root1> <subdir1> <root2> <subdir2> ...
#

set outfile [lindex $argv 0]
set dirty 0

if {![file exists $outfile]} {
    set dirty 1
    set outfile_date 0
} else {
    set outfile_date [file mtime $outfile]
}

# Each element of filelist is
# {host_path_name target_path_name}

set filelist {}

puts -nonewline stdout "checking fs "
flush stdout

foreach {root subdir} [lrange $argv 1 end] {
    regsub -all \\\\ $root / root
    regsub -all \\\\ $subdir / subdir

    regsub -all /+ $root / $root
    regsub -all /+ $subdir / subdir

    set len [expr [string length $root] + 1]

    if {![file exists $root] || ![file isdir $root]} {
        puts stderr "$root is not a directory"
        exit -1;
    } else {
        foreach src [glob -nocomplain $root/$subdir/*] {
            if {[regexp {([.]java$)|([.]c$)} $src]} {
                continue;
            }
            if {[file isfile $src]} {
                set src_date [file mtime $src]
                if {$src_date > $outfile_date} {
                    set dirty 1
                }
                set host_path_name $src
                set target_path_name "[string range $src $len end]"
                if {[string comp [string index $target_path_name 0] "."]} {
                    set target_path_name "./$target_path_name"
                }
                regsub {^[.]/} $target_path_name /fs/ target_path_name
                lappend filelist [list $host_path_name $target_path_name]
                puts -nonewline stdout "."
                flush stdout
            }
        }
    }
}

#puts $filelist
#exit -1
puts ""

if {$dirty} {
    puts -nonewline stdout "updating fs "
    flush stdout

    set fd [open $outfile w]
    fconfigure $fd -translation lf -encoding ascii
    set num 0

    puts $fd "typedef struct {"
    puts $fd "    const char * name;"
    puts $fd "    const unsigned char * data;"
    puts $fd "    int length;"
    puts $fd "} file_info;"

    foreach pair $filelist {
        set host_path_name [lindex $pair 0]
        set target_path_name [lindex $pair 1]

        set numbytes 0
        set linewidth 10
        set len [file size $host_path_name]

        puts $fd "#define file_length$num [file size $host_path_name]"

        puts $fd \
            "static const char file_name$num\[\] = \"$target_path_name\";"
        puts -nonewline $fd \
            "static const unsigned char file_data$num\[$len\] = {"

        set srcfd [open $host_path_name]
        fconfigure $srcfd -translation binary -encoding binary
        while {![eof $srcfd] && $numbytes < $len} {
            set byte [read $srcfd 1]
            binary scan $byte c value
            set value [expr $value & 0xff]
            set value [format %x $value]
            if {[string length $value] == 1} {
                set value 0x0$value
            } else {
                set value 0x$value
            }
            if {($numbytes % $linewidth) == 0} {
                puts -nonewline $fd "\n\t"
            }
            puts -nonewline $fd "$value,"
            incr numbytes
        }
        puts $fd "\n};"

        close $srcfd
        incr num

        puts -nonewline stdout "."
        flush stdout
    }

    puts ""

    puts $fd "const file_info fs_image_table\[\] = \{"
    for {set i 0} {$i < $num} {incr i} {
        puts $fd "    \{"
        puts $fd "        file_name$i,"
        puts $fd "        file_data$i,"
        puts $fd "        file_length$i"
        puts $fd "    \},"
    }

    puts $fd "    \{"
    puts $fd "        (const char*) 0,"
    puts $fd "        (const unsigned char*) 0,"
    puts $fd "        0"
    puts $fd "    \},"
    puts $fd "\};"

    close $fd

    puts "updated $outfile"
    flush stdout

    if {$num == 0} {
        puts "Warning: empty FSImage.c created.";
        puts "Check your MAKE_FS_IMAGE environment variable"
    }
}

