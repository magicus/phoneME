# @(#)README.txt	1.6 06/10/25
# 
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

IXC DEMO 
========

* Description 
-------------

This directory contains a demonstration of two Xlets that communicate with each
other.  It is loosely based on a flight simulator, where a server maintains the
position of an airplane, and a client receives position updates.

The client is in ixcXlets/clientXlet, and the server is in ixcXlets/serverXlet.
The directory "shared" contains data type definitions of the objects that are 
shared between the two.  All of the classes in shared must be a part of each Xlet.  
Since Xlets have seperate classloaders, this means that two copies of each 
shared class are loaded.



* Execution 
-----------

Execute the application from the topmost directory through the Main class -
e.g.:  
/net/system/j2me-pbp/bin/cvm -Djava.class.path=<DEMOCLASSES> IXCDemo.IXCMain


