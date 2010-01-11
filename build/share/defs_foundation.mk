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
# @(#)defs_foundation.mk	1.77 06/10/10
#

# Include target specific makefiles first
include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_foundation.mk

PROFILE_SRCDIRS += \
   $(CVM_SHAREROOT)/foundation/classes

#
# Buildtime classes for foundation's transitive closure.
#
CVM_BUILDTIME_CLASSES += \
   java.io.StringReader \
   java.net.HttpURLConnection \
   java.net.URI \
   java.net.URISyntaxException \
   java.security.AlgorithmParameters \
   java.security.AlgorithmParametersSpi \
   java.security.Certificate \
   java.security.Identity \
   java.security.IdentityScope \
   java.security.KeyFactory \
   java.security.KeyFactorySpi \
   java.security.KeyManagementException \
   java.security.KeyPair \
   java.security.KeyStore \
   java.security.KeyStoreException \
   java.security.KeyStoreSpi \
   java.security.PrivateKey \
   java.security.SecureRandom \
   java.security.SecureRandomSpi \
   java.security.Signature \
   java.security.SignatureSpi \
   java.security.Signer \
   java.security.UnrecoverableKeyException \
   java.security.cert.CertificateExpiredException \
   java.security.cert.CertificateFactory \
   java.security.cert.CertificateFactorySpi \
   java.security.cert.CertificateNotYetValidException \
   java.security.cert.CertificateParsingException \
   java.security.cert.CertPath \
   java.security.cert.CRL \
   java.security.cert.CRLException \
   java.security.cert.X509Certificate \
   java.security.cert.X509CRL \
   java.security.cert.X509CRLEntry \
   java.security.cert.X509Extension \
   java.security.interfaces.DSAKey \
   java.security.interfaces.DSAParams \
   java.security.interfaces.DSAPublicKey \
   java.security.spec.AlgorithmParameterSpec \
   java.security.spec.EncodedKeySpec \
   java.security.spec.InvalidKeySpecException \
   java.security.spec.InvalidParameterSpecException \
   java.security.spec.KeySpec \
   java.security.spec.X509EncodedKeySpec \
   java.text.StringCharacterIterator \
   javax.security.auth.x500.X500Principal \
   sun.security.action.GetBooleanAction \
   sun.security.action.GetIntegerAction \
   sun.security.action.LoadLibraryAction \
   sun.security.pkcs.ContentInfo \
   sun.security.pkcs.PKCS7 \
   sun.security.pkcs.PKCS9Attribute \
   sun.security.pkcs.PKCS9Attributes \
   sun.security.pkcs.ParsingException \
   sun.security.pkcs.SignerInfo \
   sun.security.provider.IdentityDatabase \
   sun.security.provider.NativeSeedGenerator \
   sun.security.provider.PolicyParser \
   sun.security.provider.SecureRandom \
   sun.security.provider.SeedGenerator \
   sun.security.provider.SystemIdentity \
   sun.security.provider.SystemSigner \
   sun.security.provider.X509Factory \
   sun.security.provider.certpath.X509CertPath \
   sun.security.util.BitArray \
   sun.security.util.ByteArrayLexOrder \
   sun.security.util.ByteArrayTagOrder \
   sun.security.util.Cache \
   sun.security.util.DerEncoder \
   sun.security.util.DerIndefLenConverter \
   sun.security.util.DerInputBuffer \
   sun.security.util.DerInputStream \
   sun.security.util.DerOutputStream \
   sun.security.util.DerValue \
   sun.security.util.ManifestDigester \
   sun.security.util.ManifestEntryVerifier \
   sun.security.util.ObjectIdentifier \
   sun.security.util.PropertyExpander \
   sun.security.util.Resources \
   sun.security.util.ResourcesMgr \
   sun.security.util.SecurityConstants \
   sun.security.util.SignatureFileVerifier \
   sun.security.x509.AVA \
   sun.security.x509.AlgorithmId \
   sun.security.x509.AttributeNameEnumeration \
   sun.security.x509.AuthorityKeyIdentifierExtension \
   sun.security.x509.BasicConstraintsExtension \
   sun.security.x509.CRLExtensions \
   sun.security.x509.CRLNumberExtension \
   sun.security.x509.CRLReasonCodeExtension \
   sun.security.x509.CertAttrSet \
   sun.security.x509.CertificateAlgorithmId \
   sun.security.x509.CertificateExtensions \
   sun.security.x509.CertificateIssuerName \
   sun.security.x509.CertificateIssuerUniqueIdentity \
   sun.security.x509.CertificatePolicyId \
   sun.security.x509.CertificatePolicyMap \
   sun.security.x509.CertificateSerialNumber \
   sun.security.x509.CertificateSubjectName \
   sun.security.x509.CertificateSubjectUniqueIdentity \
   sun.security.x509.CertificateValidity \
   sun.security.x509.CertificateVersion \
   sun.security.x509.CertificateX509Key \
   sun.security.x509.DNSName \
   sun.security.x509.EDIPartyName \
   sun.security.x509.ExtendedKeyUsageExtension \
   sun.security.x509.Extension \
   sun.security.x509.GeneralName \
   sun.security.x509.GeneralNameInterface \
   sun.security.x509.GeneralNames \
   sun.security.x509.GeneralSubtree \
   sun.security.x509.GeneralSubtrees \
   sun.security.x509.IPAddressName \
   sun.security.x509.IssuerAlternativeNameExtension \
   sun.security.x509.KeyIdentifier \
   sun.security.x509.KeyUsageExtension \
   sun.security.x509.NameConstraintsExtension \
   sun.security.x509.NetscapeCertTypeExtension \
   sun.security.x509.OIDMap \
   sun.security.x509.OIDName \
   sun.security.x509.OtherName \
   sun.security.x509.PKIXExtensions \
   sun.security.x509.PolicyConstraintsExtension \
   sun.security.x509.PolicyMappingsExtension \
   sun.security.x509.PrivateKeyUsageExtension \
   sun.security.x509.RDN \
   sun.security.x509.RFC822Name \
   sun.security.x509.SerialNumber \
   sun.security.x509.SubjectAlternativeNameExtension \
   sun.security.x509.SubjectKeyIdentifierExtension \
   sun.security.x509.URIName \
   sun.security.x509.UniqueIdentity \
   sun.security.x509.UnparseableExtension \
   sun.security.x509.X500Name$1 \
   sun.security.x509.X500Name \
   sun.security.x509.X509AttributeName \
   sun.security.x509.X509CRLEntryImpl \
   sun.security.x509.X509CRLImpl \
   sun.security.x509.X509CertImpl \
   sun.security.x509.X509CertInfo \
   sun.security.x509.X509Key \
   sun.text.DecompData \
   sun.text.Normalizer \
   sun.text.ComposeData \
   sun.text.CompactByteArray \
   sun.text.CompactCharArray \
   sun.misc.NetworkMetrics \
   sun.misc.NetworkMetricsInf \

#
# The following are the public classes in the Foundation specification.
#
PROFILE_PUBLIC_CLASSES += \
   java.io.CharArrayReader \
   java.io.CharArrayWriter \
   java.io.FilterReader \
   java.io.FilterWriter \
   java.io.LineNumberReader \
   java.io.PipedReader \
   java.io.PipedWriter \
   java.io.PushbackReader \
   java.io.RandomAccessFile \
   java.io.SequenceInputStream \
   java.io.StringWriter \
   java.lang.Compiler \
   java.lang.UnknownError \
   java.math.BigDecimal \
   java.net.Authenticator \
   java.net.ConnectException \
   java.net.MulticastSocket \
   java.net.NoRouteToHostException \
   java.net.PasswordAuthentication \
   java.net.ServerSocket \
   java.net.Socket \
   java.net.SocketImpl \
   java.net.SocketImplFactory \
   java.net.URLDecoder \
   java.net.URLEncoder \
   java.security.AlgorithmParameterGenerator \
   java.security.AlgorithmParameterGeneratorSpi \
   java.security.DigestInputStream \
   java.security.KeyPairGenerator \
   java.security.KeyPairGeneratorSpi \
   java.security.SignedObject \
   java.security.acl.Acl \
   java.security.acl.AclEntry \
   java.security.acl.AclNotFoundException \
   java.security.acl.Group \
   java.security.acl.LastOwnerException \
   java.security.acl.NotOwnerException \
   java.security.acl.Owner \
   java.security.acl.Permission \
   java.security.interfaces.DSAKeyPairGenerator \
   java.security.interfaces.DSAPrivateKey \
   java.security.interfaces.RSAKey \
   java.security.interfaces.RSAPrivateCrtKey \
   java.security.interfaces.RSAPrivateKey \
   java.security.interfaces.RSAPublicKey \
   java.security.spec.DSAParameterSpec \
   java.security.spec.DSAPrivateKeySpec \
   java.security.spec.DSAPublicKeySpec \
   java.security.spec.PKCS8EncodedKeySpec \
   java.security.spec.PSSParameterSpec \
   java.security.spec.RSAKeyGenParameterSpec \
   java.security.spec.RSAPrivateCrtKeySpec \
   java.security.spec.RSAPrivateKeySpec \
   java.security.spec.RSAPublicKeySpec \
   java.text.BreakIterator \
   java.text.CollationElementIterator \
   java.text.CollationKey \
   java.text.Collator \
   java.text.RuleBasedCollator \
   java.util.EventListener \
   java.util.EventListenerProxy \
   java.util.EventObject \
   java.util.Observable \
   java.util.Observer \
   java.util.Timer \
   java.util.TimerTask \
   java.util.TooManyListenersException \
\
   java.util.jar.JarOutputStream \
   java.util.zip.Adler32 \
   java.util.zip.CheckedInputStream \
   java.util.zip.CheckedOutputStream \
   java.util.zip.Deflater \
   java.util.zip.DeflaterOutputStream \
   java.util.zip.GZIPInputStream \
   java.util.zip.GZIPOutputStream \
   java.util.zip.ZipOutputStream \
   javax.microedition.io.HttpConnection \
   javax.microedition.io.HttpsConnection \
   javax.microedition.io.SecureConnection \
   javax.microedition.io.SecurityInfo \
   javax.microedition.io.ServerSocketConnection \
   javax.microedition.io.UDPDatagramConnection \
   javax.microedition.pki.Certificate \
   javax.microedition.pki.CertificateException

#
# These are the classes in the Foundation specification that
# get pulled in by the transitive closure of the supersetted Foundation
# class implementations.  They are exposed public APIs and should show
# up on the Foundation-only list in the spec.  They are mainly pulled in
# by the sun.security.provider.Policy class.
#
PROFILE_PUBLIC_CLASSES +=

#
# And these are the implementation classes in the Foundation specification.
#
PROFILE_IMPL_CLASSES += \
   java.net.PlainSocketImpl \
   java.net.SocketInputStream \
   java.net.SocketOutputStream \
   java.text.CollationRules \
   java.text.DictionaryBasedBreakIterator \
   java.text.RuleBasedBreakIterator \
   java.text.EntryPair \
   java.text.MergeCollation \
   java.text.PatternEntry \
   java.text.RBCollationTables \
   java.text.RBTableBuilder \
   \
   sun.misc.BASE64Encoder \
   sun.misc.Compare \
   sun.misc.GC \
   sun.misc.REException \
   sun.misc.Regexp \
   sun.misc.RegexpPool \
   sun.misc.RegexpTarget \
   sun.misc.Sort \
   sun.net.NetworkClient \
   sun.net.ProgressEntry \
   sun.net.ProgressData \
   sun.net.www.MeteredStream \
   sun.net.www.HeaderParser \
   sun.net.www.http.HttpClient \
   sun.net.www.protocol.http.AuthenticationInfo \
   sun.net.www.protocol.http.BasicAuthentication \
   sun.net.www.protocol.http.DigestAuthentication \
   sun.net.www.protocol.http.Handler \
   sun.net.www.protocol.http.HttpAuthenticator \
   sun.net.www.protocol.http.HttpURLConnection \
   \
   sun.security.util.BigInt \
   sun.security.x509.AlgIdDSA \
   sun.security.x509.GeneralNamesException \
   sun.security.pkcs.PKCS8Key \
   sun.security.provider.SHA \
   sun.security.provider.SHA2 \
   sun.security.provider.SHA3 \
   sun.security.provider.SHA5 \
   sun.security.provider.DSAPrivateKey \
   sun.security.provider.DSAPublicKey \
   sun.security.provider.DSA \
   sun.security.provider.DSAKeyPairGenerator \
   sun.security.provider.DSAKeyFactory \
   sun.security.provider.DSAParameters \
   sun.security.provider.DSAParameterGenerator \
   sun.security.provider.JavaKeyStore \
   sun.security.provider.KeyProtector \
   sun.security.provider.MD5 \
   sun.security.provider.certpath.X509CertPath \
   \
   com.sun.cdc.io.j2me.socket.Protocol \
   \
   com.sun.cdc.io.j2me.http.HttpStreamConnection \
   com.sun.cdc.io.j2me.http.Protocol \
   com.sun.cdc.io.j2me.http.StreamConnectionElement \
   com.sun.cdc.io.j2me.http.StreamConnectionPool \
   \
   com.sun.cdc.io.j2me.serversocket.Protocol \
   com.sun.cdc.io.j2me.UniversalFilterInputStream \
   com.sun.cdc.io.j2me.UniversalInputStream \
   com.sun.cdc.io.j2me.UniversalFilterOutputStream \
   \
   sun.text.CompactIntArray \
   sun.text.ComposedCharIter \
   sun.text.IntHashtable \
   sun.text.NormalizerUtilities

#
# Foundation classes are the public and the implementation classes
# combined
#
CLASSLIB_CLASSES += \
	$(PROFILE_PUBLIC_CLASSES) \
	$(PROFILE_IMPL_CLASSES)

#
# Foundation profile natives and their source directories
#

CVM_SRCDIRS    += \
	$(CVM_SHAREROOT)/foundation/native/java/io \
	$(CVM_SHAREROOT)/foundation/native/java/lang \
	$(CVM_SHAREROOT)/foundation/native/java/util/zip \

CVM_SHAREOBJS_SPACE += \
   Compiler.o \
   Adler32.o \
   RandomAccessFile.o \
   Deflater.o

#
# Foundation profile test classes and their source directories
#

CVM_TEST_CLASSES += \
   foundation.TimerThreadTest

ifeq ($(CVM_SERIALIZATION), true)
CVM_TEST_CLASSES += \
   foundation.SerializationTestClient \
   foundation.SerializationTestServer \
   foundation.SerializationTestObject
endif

#
# Foundation Library Unit Tests
#
CVM_TESTCLASSES_SRCDIRS += \
	$(CVM_TOP)/test/share/foundation/java/net/URI \
	$(CVM_TOP)/test/share/foundation/java/net/Inet6Address

CVM_TEST_CLASSES  += \
	URITest \
	URItoURLTest \
	IPv6Test 

#
# The name of the HTML file that is generated with the names of all public
# foundation classes.
#
JAVADOC_FOUNDATION_CLASSESLIST = $(INSTALLDIR)/javadoc/foundation-classes.index.html
JAVADOC_FOUNDATION_CLASSPATH   = $(LIB_CLASSESDIR):$(CVM_BUILDTIME_CLASSESDIR)
JAVADOC_FOUNDATION_BTCLASSPATH = $(JAVADOC_FOUNDATION_CLASSPATH)
JAVADOC_FOUNDATION_SRCPATH     = $(PROFILE_SRCDIRS):$(CVM_SHAREDCLASSES_SRCDIR):$(CVM_CLDCCLASSES_SRCDIR)

#
# Include any commercial definitions.
#
-include defs_foundation_commercial.mk

# Do this last so this makefile can override source files on the vpath.
include $(CDC_DIR)/build/share/defs_cdc.mk
