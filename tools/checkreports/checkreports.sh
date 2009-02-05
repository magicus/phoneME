#!/bin/sh

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

# Launch 'env DBG_SH="set -x" sh checkreports ...' for debug
${DBG_SH}

# 1. Take head of all brief reports into brief_summary.txt;
# 2. Get results line (begins with 'Tests run');
# 3. Grab Failures and Errors count:
#    Example: 'Tests run: 15, Failures: 2, Errors: 0, Time elapsed: 5.859 sec'
#    sed output:  '2 0'
#    grep checks if it's not '0 0'.
# 4. If there are errors or failures, exit.
#
 
func_log()
{
    $VERBOSE && echo $@
}

func_showHelp()
{
    cat << \
EOM

Usage: checkreports [options] [dir]
Options:
  -h, --help         show this usage message
  -v, --verbose      verbosity
  -r, --run-tests    run tests first (launch '\$MAKE_CMD run-unittests'
                     from the current directory)

  dir    path to the root directory of the reports. Default: \$REPORTS_DIR.

Report bugs to jump-eng@sun.com
EOM
}

func_checkArgs()
{
    # You must provide a path to root directory of unit tests reports as paramter
    if [ $# -gt 3 ]
    then
        func_showHelp
        exit 1
    fi

    RUN_TESTS=false
    VERBOSE=false
    
    for A in $*
    do
        if [ $A = "-v" -o $A = "--verbose" ]
        then
            VERBOSE=true
        elif [ $A = "-r" -o $A = "--run-tests" ]
        then
            RUN_TESTS=true
        elif [ -d "$A" ]
        then
            REPORTS_DIR=$A
        elif [ $A = "-h" -o $A = "--help" ]
        then
            func_showHelp; exit 0
        else
            echo "Unknown option: $A" >&2
            func_showHelp; exit 1
        fi
    done
    
    if [ ! -d "$REPORTS_DIR" ]
    then
        echo "Invalid reports dir: '$REPORTS_DIR'" >&2
        func_showHelp
        exit 1
    fi
}

func_generateBriefSum()
{
    BRIEF_SUMMARY="$REPORTS_DIR/brief_summary.txt"

    # Cleanup old summary if exist
    [ -w "$BRIEF_SUMMARY" ] && rm "$BRIEF_SUMMARY"

    # For each report file get first 2 lines and append BRIEF_SUMMARY file.
    for REPORT in `find "$REPORTS_DIR" -name "TEST*.txt"`
    do
        head -n 2 "$REPORT" >>"$BRIEF_SUMMARY"
        echo "" >>"$BRIEF_SUMMARY"
    done
    [ -f "$BRIEF_SUMMARY" ] || func_log "Warning: there were no reports found"

    touch "$BRIEF_SUMMARY"
    $VERBOSE && cat "$BRIEF_SUMMARY"
}

# Search for failures and errors in BRIEF_SUMMARY file
func_checkResults()
{
    if ( grep "^Tests run:" "$BRIEF_SUMMARY" \
        | sed "s/.*Failures: \([0-9]\+\), Errors: \([0-9]\+\).*/\1 \2/g" \
        | grep -v "0 0" >>/dev/null )
    then
        echo "Unit tests has failures or errors. Check '$BRIEF_SUMMARY' for details." >&2
        exit 1
    fi
}

func_runTests()
{
    $RUN_TESTS || return 0
    
    if [ $VERBOSE ]
    then $MAKE_CMD run-unittests
    else $MAKE_CMD run-unittests >/dev/null
    fi
}

func_checkArgs $*
if ! func_runTests
then
    echo "Cannot run unit tests. Exit." >&2
    exit 1
fi
func_generateBriefSum
func_checkResults

