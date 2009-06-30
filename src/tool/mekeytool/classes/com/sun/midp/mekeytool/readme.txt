	

Copyright  2007  Sun Microsystems, Inc. All rights reserved.


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
