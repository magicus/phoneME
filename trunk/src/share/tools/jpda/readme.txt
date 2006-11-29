#
# @(#)readme.txt	1.9 06/10/25 
#
# Copyright 2006 Sun Microsystems, Inc. All rights reserved.
# 
# Sun Microsystems, Inc. has intellectual property rights relating to technology
# embodied in the product that is described in this document. In particular, and
# without limitation, these intellectual property rights may include one or more
# of the U.S. patents listed at http://www.sun.com/patents and one or more
# additional patents or pending patent applications in the U.S. and in other
# countries.
# U.S. Government Rights - Commercial software. Government users are subject to
# the Sun Microsystems, Inc. standard license agreement and applicable provisions
# of the FAR and its supplements.
# 
# Use is subject to license terms.
# 
# This distribution may include materials developed by third parties.Sun, Sun
# Microsystems, phoneME and Java are trademarks or registered trademarks of Sun
# Microsystems, Inc. in the U.S. and other countries.
# 
# 
# Copyright 2006 Sun Microsystems, Inc. Tous droits réservés.
# 
# Sun Microsystems, Inc. détient les droits de propriété intellectuels relatifs à
# la technologie incorporée dans le produit qui est décrit dans ce document. En
# particulier, et ce sans limitation, ces droits de propriété intellectuelle
# peuvent inclure un ou plus des brevets américains listés à l'adresse
# http://www.sun.com/patents et un ou les brevets supplémentaires ou les
# applications de brevet en attente aux Etats - Unis et dans les autres pays.
# 
# L'utilisation est soumise aux termes du contrat de licence.
# 
# Cette distribution peut comprendre des composants développés par des tierces
# parties.
# 
# Sun, Sun Microsystems, phoneME et Java sont des marques de fabrique ou des
# marques déposées de Sun Microsystems, Inc. aux Etats-Unis et dans d'autres pays.
#

This version of the JPDA has been modified from the standard
distribution in the following ways:

1. A modification to transport_initialize() in transport.c which
   changes the mechanism by which the transport library is found.
   Instead of relying on the environment variable LD_LIBRARY_PATH to
   find, for example, libdt_socket.so, the new routine uses the JNI to
   get the system properties sun.boot.class.path and java.library.path
   and searches them for the shared object.

2. A new build-time flag, JPDA_NO_DLALLOC, has been added to eliminate
   usage of the internal memory allocator and use malloc() instead
   (preventing having to port dlAlloc.c to, or even make it compilable
   on, platforms which don't have the necessary underlying routines to
   implement it.) Use this by typing "gnumake JPDA_NO_DLALLOC=true" in
   the build directory, or by specifying it in the makefile.

3. stdio.h was missing from a few files which needed it on platforms
   which have fewer system header file interdependencies than Solaris.

The applicable diffs have been sent to the JPDA team for consideration.

Files changed:

./build/solaris/makefiles/Defs.gmk
./build/solaris/GNUmakefile
./build/solaris/back/GNUmakefile
./build/solaris/jdwpgen/GNUmakefile
./build/solaris/transport/socket/GNUmakefile
./build/share/minclude/jdwp.cmk
./build/vxworks/makefiles/Defs.gmk
./build/vxworks/GNUmakefile
./build/vxworks/back/CClassHeaders/JDWPCommands.h
./build/vxworks/back/GNUmakefile
./build/vxworks/jdwpgen/GNUmakefile
./build/vxworks/transport/socket/GNUmakefile
./src/share/back/commonRef.c
./src/share/back/debugInit.c
./src/share/back/debugLoop.c
./src/share/back/transport.c
./src/share/back/stepControl.c
./src/share/back/transport.h
./src/share/back/util.c
./src/share/back/util.h
./src/vxworks/back/exec_md.c
./src/vxworks/back/linker_md.c
./src/vxworks/transport/socket/socket_md.c
./readme.txt
