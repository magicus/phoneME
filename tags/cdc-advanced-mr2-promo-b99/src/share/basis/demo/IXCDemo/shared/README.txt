# @(#)README.txt	1.6 06/10/25
# 
#
# Copyright  2008  Sun Microsystems, Inc. All rights reserved.
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


