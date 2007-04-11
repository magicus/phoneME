	

Copyright  2006 Sun Microsystems, Inc.  All rights reserved.  

Sun Microsystems, Inc. has intellectual property rights relating to
technology embodied in the product that is described in this
document. In particular, and without limitation, these intellectual
property rights may include one or more of the U.S. patents listed at
http://www.sun.com/patents and one or more additional patents or
pending patent applications in the U.S. and in other countries. 

U.S. Government Rights - Commercial software. Government users are
subject to the Sun Microsystems, Inc. standard license agreement and
applicable provisions of the FAR and its supplements.   

Use is subject to license terms.  

This distribution may include materials developed by third
parties. Sun, Sun Microsystems, the Sun logo and Java, phoneME, J2ME, JDK,
Javadoc, HotSpot, and Solaris are trademarks or registered trademarks
of Sun Microsystems, Inc. in the U.S. and other countries.   

UNIX is a registered trademark in the U.S. and other countries,
exclusively licensed through X/Open Company, Ltd. 

Copyright  2006 Sun Microsystems, Inc. Tous droits reserves.

Sun Microsystems, Inc. detient les droits de propriete intellectuels
relatifs a la technologie incorporee dans le produit qui est decrit
dans ce document. En particulier, et ce sans limitation, ces droits de
propriete intellectuelle peuvent inclure un ou plus des brevets
americains listes a l'adresse http://www.sun.com/patents et un ou les
brevets supplementaires ou les applications de brevet en attente aux
Etats - Unis et dans les autres pays. 

L'utilisation est soumise aux termes du contrat de licence.

Cette distribution peut comprendre des composants developpes par des
tierces parties. Sun,  Sun Microsystems,  le logo Sun et Java,  phoneME, J2ME,
JDK,  Javadoc,  HotSpot, et Solaris  sont des marques de fabrique ou
des marques deposees de Sun Microsystems, Inc. aux Etats-Unis et dans
d'autres pays. 

UNIX est une marque deposee aux Etats-Unis et dans d'autres pays et
licenciee exlusivement par X/Open Company, Ltd. 


j2se_test_keystore.bin is a java.security.keystore created with the
keytool utility provided with the J2SE JDK (>=1.3).

Its password is keystorepwd.

To sign MIDlet suites it has 2048 bit RSA public key pair under the alias
of dummyca and a private key password of keypwd.

To perform a very general test of HTTPS against a live Internet server, the
keystore has the RSA public key of a public certificate authority under the
alias of publicca.

For internal QA purposes, the keystore has the RSA public key of the Sun
Test CA under the alias of suntestca.

To list the keys, use the tool provided with the J2SE JDK:

  keytool -list -v -keystore j2se_test_keystore.bin -storepass keystorepwd
