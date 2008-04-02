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

all: ProfView.jar

ProfView.jar: ProfView.java TimeLine.java
	javac ProfView.java TimeLine.java
	rm -f ProfView.jar
	jar cfm ProfView.jar ProfView.mf *.class icons/*.gif

run: ProfView.jar
	java -jar ProfView.jar graph.txt

run1: ProfView.jar
	java -jar ProfView.jar ../../kvm/kvm/vmwin/build/jprof.txt

run2: ProfView.jar
	java -jar ProfView.jar e:/apps/PalmEmu/loader/1.0-b21-kbrowser.txt

run3: ProfView.jar
	java -jar ProfView.jar e:/ws/mep/MIDP1.0.3/build/win32/jprof.txt

run4: ProfView.jar
	java -jar ProfView.jar n:/ws/MIDP1.0.3/build/solaris/jprof.txt

run5: ProfView.jar
	java -jar ProfView.jar n:/ws/kvm/build/solaris/jprof.txt


clean:
	rm -rf *.class *.jar *~

