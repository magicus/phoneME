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

This directory contains a project for the ACL data encoder.

This utility is intended to help to tranclate ACL data from text format to 
ANSI 1.0 DER encoded format.

Format of the input text file is described in the HowToUseACL.txt file.
Input file "acl_0" should be placed in the current directory.

Output files, containig DER encoded information will be generated in the 
<output_dir>/jsr177/acl_data/files directory.
This directory will contain files of the file system that could be downloaded into 
a card file system.
