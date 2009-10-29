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
# @(#)defs_cdc.mk	1.66 06/10/10
#

# JSR-75 optional package (if present in the build) can set this to false
# in order to use its own "file:" protocol handler
USE_CDC_FILE_PROTOCOL ?= true

GENERATED_CLASSES += \
   sun.misc.BuildFlags \
   com.sun.cdc.config.PackageManager \
   sun.misc.DefaultLocaleList \

# Currently, we generate offsets when we ROMize, so these need to
# be in the minimal set.  If we generated offsets for non-ROMized
# classes, then offsets could be wrong if the bootclasspath is
# changed.

# CVM_BUILDTIME_CLASSES_min
#
# These classes define the minimal ROMized set.
# 
# CVM_BUILDTIME_CLASSES_nullapp
#
# These classes define the minimal set of classes that will
# the JVM needs to start a null application.
#
# CVM_BUILDTIME_CLASSES
#
# The rest of the old ROMized set that included the
# transitive closure of all dependencies.  This list
# is used to supplement CLASSLIB_CLASSES, but keeps
# its original name for backwards compatibility with
# other makefiles that might add to it.

CVM_BUILDTIME_CLASSES_min += $(CVM_OFFSETS_CLASSES)

# And missing parents for those offset classes

CVM_BUILDTIME_CLASSES_min += \
    java.util.List \
    java.util.RandomAccess \
    java.util.Collection \
    java.util.AbstractCollection \

# Required to be ROMized by "simple sync" optimization support
CVM_BUILDTIME_CLASSES_min += \
    java.util.AbstractList \
    java.util.Vector \
    java.util.Vector$$1 \
    java.util.Map \
    java.util.Dictionary \
    java.util.Hashtable \
    java.util.Random \

CVM_BUILDTIME_CLASSES_min += \
    java.io.CharConversionException \
    java.io.File \
    java.io.FilterInputStream \
    java.io.IOException \
    java.io.InputStream \
    java.io.InvalidClassException \
    java.io.ObjectStreamException \
    java.io.Serializable \
    java.lang.AbstractMethodError \
    java.lang.ArithmeticException \
    java.lang.ArrayIndexOutOfBoundsException \
    java.lang.ArrayStoreException \
    java.lang.AssertionStatusDirectives \
    java.lang.Boolean \
    java.lang.Byte \
    java.lang.CharSequence \
    java.lang.Character \
    java.lang.Class \
    java.lang.Class$$LoadingList \
    java.lang.ClassCastException \
    java.lang.ClassCircularityError \
    java.lang.ClassFormatError \
    java.lang.ClassLoader \
    java.lang.ClassLoader$$NativeLibrary \
    java.lang.ClassNotFoundException \
    java.lang.CloneNotSupportedException \
    java.lang.Cloneable \
    java.lang.Comparable \
    java.lang.Double \
    java.lang.Error \
    java.lang.Exception \
    java.lang.Float \
    java.lang.IllegalAccessError \
    java.lang.IllegalAccessException \
    java.lang.IllegalArgumentException \
    java.lang.IllegalMonitorStateException \
    java.lang.IllegalStateException \
    java.lang.IncompatibleClassChangeError \
    java.lang.IndexOutOfBoundsException \
    java.lang.InstantiationError \
    java.lang.InstantiationException \
    java.lang.Integer \
    java.lang.InternalError \
    java.lang.InterruptedException \
    java.lang.LinkageError \
    java.lang.Long \
    java.lang.NegativeArraySizeException \
    java.lang.NoClassDefFoundError \
    java.lang.NoSuchFieldError \
    java.lang.NoSuchMethodError \
    java.lang.NullPointerException \
    java.lang.Number \
    java.lang.Object \
    java.lang.OutOfMemoryError \
    java.lang.Runnable \
    java.lang.RuntimeException \
    java.lang.Short \
    java.lang.Shutdown \
    java.lang.StackOverflowError \
    java.lang.StackTraceElement \
    java.lang.String \
    java.lang.StringBuffer \
    java.lang.StringIndexOutOfBoundsException \
    java.lang.System \
    java.lang.Thread \
    java.lang.ThreadGroup \
    java.lang.ThreadLocal \
    java.lang.Throwable \
    java.lang.UnsatisfiedLinkError \
    java.lang.UnsupportedClassVersionError \
    java.lang.UnsupportedOperationException \
    java.lang.VerifyError \
    java.lang.VirtualMachineError \
    java.lang.ref.FinalReference \
    java.lang.ref.Finalizer \
    java.lang.ref.PhantomReference \
    java.lang.ref.Reference \
    java.lang.ref.SoftReference \
    java.lang.ref.WeakReference \
    java.net.ContentHandler \
    java.net.FileNameMap \
    java.net.URLClassLoader \
    java.net.URLConnection \
    java.net.URLStreamHandlerFactory \
    java.security.AccessController \
    java.security.CodeSource \
    java.security.PermissionCollection \
    java.security.PrivilegedAction \
    java.security.PrivilegedActionException \
    java.security.PrivilegedExceptionAction \
    java.security.SecureClassLoader \
    java.util.Comparator \
    java.util.Enumeration \
    java.util.jar.JarEntry \
    java.util.jar.JarFile \
    java.util.zip.InflaterInputStream \
    java.util.zip.ZipConstants \
    java.util.zip.ZipEntry \
    java.util.zip.ZipFile \
    sun.io.ByteToCharConverter \
    sun.io.CharToByteConverter \
    sun.io.CharToByteISO8859_1 \
    sun.io.ConversionBufferFullException \
    sun.io.MalformedInputException \
    sun.io.Markable \
    sun.io.UnknownCharacterException \
    sun.misc.CVM \
    sun.misc.Launcher \
    sun.misc.Launcher$$AppClassLoader \
    sun.misc.Launcher$$ClassContainer \
    sun.misc.ThreadRegistry \

ifeq ($(CVM_REFLECT), true)
CVM_BUILDTIME_CLASSES_min += \
    java.lang.reflect.AccessibleObject \
    java.lang.reflect.Constructor \
    java.lang.reflect.Constructor$$ArgumentException \
    java.lang.reflect.Constructor$$AccessException \
    java.lang.reflect.Field \
    java.lang.reflect.Member \
    java.lang.reflect.Method \
    java.lang.reflect.Method$$ArgumentException \
    java.lang.reflect.Method$$AccessException \
    java.lang.NoSuchFieldException \
    java.lang.NoSuchMethodException
endif

# We need JNI headers generated for these
# using javah, but they don't show up in the class
# list because they don't correspond to a .java file.

CVM_EXTRA_JNI_CLASSES += \
    'java.lang.ClassLoader$$NativeLibrary' \
    'sun.misc.Launcher$$AppClassLoader' \
    java.net.InetAddressImplFactory \

CVM_BUILDTIME_CLASSES_nullapp += \
    java.io.BufferedInputStream \
    java.io.BufferedOutputStream \
    java.io.BufferedWriter \
    java.io.ExpiringCache \
    java.io.ExpiringCache$$Entry \
    java.io.FileDescriptor \
    java.io.FileInputStream \
    java.io.FileOutputStream \
    java.io.FilePermission \
    java.io.FilePermission$$1 \
    java.io.FilePermissionCollection \
    java.io.FileSystem \
    java.io.FilterOutputStream \
    java.io.ObjectStreamClass \
    java.io.ObjectStreamField \
    java.io.OutputStream \
    java.io.OutputStreamWriter \
    java.io.PrintStream \
    java.io.Writer \
    java.lang.CharacterDataLatin1 \
    java.lang.Integer$$1 \
    java.lang.Long$$1 \
    java.lang.RuntimePermission \
    java.lang.Shutdown$$Lock \
    java.lang.String$$CaseInsensitiveComparator \
    java.lang.StringCoding \
    java.lang.StringCoding$$ConverterSD \
    java.lang.StringCoding$$ConverterSE \
    java.lang.StringCoding$$StringDecoder \
    java.lang.StringCoding$$StringEncoder \
    java.lang.SystemClassLoaderAction \
    java.lang.Terminator \
    java.lang.ThreadLocal$$ThreadLocalMap \
    java.lang.ThreadLocal$$ThreadLocalMap$$Entry \
    java.lang.Void \
    java.lang.ref.Finalizer$$FinalizerThread \
    java.lang.ref.Reference$$ReferenceHandler \
    java.lang.ref.ReferenceQueue \
    java.lang.ref.ReferenceQueue$$Lock \
    java.lang.ref.ReferenceQueue$$Null \
    java.lang.reflect.Modifier \
    java.lang.reflect.ReflectPermission \
    java.net.Parts \
    java.net.URL \
    java.net.URLStreamHandler \
    java.net.UnknownContentHandler \
    java.security.AccessControlContext \
    java.security.AllPermission \
    java.security.BasicPermission \
    java.security.BasicPermissionCollection \
    java.security.Guard \
    java.security.Permission \
    java.security.Permissions \
    java.security.Principal \
    java.security.ProtectionDomain \
    java.security.UnresolvedPermission \
    java.security.cert.Certificate \
    java.util.AbstractMap \
    java.util.AbstractSet \
    java.util.ArrayList \
    java.util.BitSet \
    java.util.HashMap \
    java.util.HashMap$$Entry \
    java.util.HashSet \
    java.util.Hashtable$$EmptyEnumerator \
    java.util.Hashtable$$EmptyIterator \
    java.util.Hashtable$$Entry \
    java.util.Iterator \
    java.util.Locale \
    java.util.Map$$Entry \
    java.util.Properties \
    java.util.Set \
    java.util.Stack \
    sun.io.ByteToCharASCII \
    sun.io.CharToByteASCII \
    sun.io.Converters \
    sun.misc.Launcher$$1 \
    sun.misc.Launcher$$3 \
    sun.misc.Launcher$$4 \
    sun.misc.Launcher$$Factory \
    sun.misc.SoftCache \
    sun.misc.URLClassPath \
    sun.misc.Version \
    sun.misc.VersionHelper \
    sun.net.www.MessageHeader \
    sun.net.www.ParseUtil \
    sun.net.www.URLConnection \
    sun.net.www.protocol.file.FileURLConnection \
    sun.net.www.protocol.file.Handler \
    sun.net.www.protocol.jar.Handler \
    sun.security.action.GetPropertyAction \
    sun.security.util.Debug \

CVM_BUILDTIME_CLASSES += \
    com.sun.cdc.config.DynamicProperties \
    com.sun.cdc.config.PackageManager \
    com.sun.cdc.config.PropertyProvider \
    com.sun.cdc.config.PropertyProviderAdapter \
    java.io.BufferedReader \
    java.io.ByteArrayInputStream \
    java.io.ByteArrayOutputStream \
    java.io.DataInput \
    java.io.DataInputStream \
    java.io.DataOutput \
    java.io.DataOutputStream \
    java.io.EOFException \
    java.io.Externalizable \
    java.io.FileFilter \
    java.io.FileNotFoundException \
    java.io.FileReader \
    java.io.FileWriter \
    java.io.FilenameFilter \
    java.io.InputStreamReader \
    java.io.InterruptedIOException \
    java.io.InvalidObjectException \
    java.io.NotActiveException \
    java.io.NotSerializableException \
    java.io.ObjectInput \
    java.io.ObjectInputStream \
    java.io.ObjectInputValidation \
    java.io.ObjectOutput \
    java.io.ObjectOutputStream \
    java.io.ObjectStreamConstants \
    java.io.OptionalDataException \
    java.io.PipedInputStream \
    java.io.PipedOutputStream \
    java.io.PrintWriter \
    java.io.PushbackInputStream \
    java.io.Reader \
    java.io.SerializablePermission \
    java.io.StreamCorruptedException \
    java.io.StreamTokenizer \
    java.io.SyncFailedException \
    java.io.UTFDataFormatException \
    java.io.UnsupportedEncodingException \
    java.io.WriteAbortedException \
    java.lang.AssertionError \
    java.lang.CharacterData \
    java.lang.ExceptionInInitializerError \
    java.lang.FloatingDecimal \
    java.lang.IllegalThreadStateException \
    java.lang.InheritableThreadLocal \
    java.lang.Math \
    java.lang.NumberFormatException \
    java.lang.Package \
    java.lang.Process \
    java.lang.Runtime \
    java.lang.SecurityException \
    java.lang.SecurityManager \
    java.lang.StrictMath \
    java.lang.ThreadDeath \
    java.math.BigInteger \
    java.math.BitSieve \
    java.math.MutableBigInteger \
    java.math.SignedMutableBigInteger \
    java.net.ContentHandlerFactory \
    java.net.Inet4Address \
    java.net.Inet4AddressImpl \
    java.net.Inet6Address \
    java.net.Inet6AddressImpl \
    java.net.InetAddress \
    java.net.InetAddressImpl \
    java.net.JarURLConnection \
    java.net.MalformedURLException \
    java.net.NetPermission \
    java.net.ProtocolException \
    java.net.SocketPermission \
    java.net.UnknownHostException \
    java.net.UnknownServiceException \
    java.security.AccessControlException \
    java.security.DigestException \
    java.security.DigestOutputStream \
    java.security.DomainCombiner \
    java.security.GeneralSecurityException \
    java.security.GuardedObject \
    java.security.InvalidAlgorithmParameterException \
    java.security.InvalidKeyException \
    java.security.InvalidParameterException \
    java.security.Key \
    java.security.KeyException \
    java.security.MessageDigest \
    java.security.MessageDigestSpi \
    java.security.NoSuchAlgorithmException \
    java.security.NoSuchProviderException \
    java.security.Policy \
    java.security.Provider \
    java.security.ProviderException \
    java.security.PublicKey \
    java.security.Security \
    java.security.SecurityPermission \
    java.security.SignatureException \
    java.security.UnresolvedPermissionCollection \
    java.security.cert.CertificateEncodingException \
    java.security.cert.CertificateException \
    java.text.Annotation \
    java.text.AttributedCharacterIterator \
    java.text.AttributedString \
    java.text.CharacterIterator \
    java.text.ChoiceFormat \
    java.text.DateFormat \
    java.text.DateFormatSymbols \
    java.text.DecimalFormat \
    java.text.DecimalFormatSymbols \
    java.text.DigitList \
    java.text.FieldPosition \
    java.text.Format \
    java.text.MessageFormat \
    java.text.NumberFormat \
    java.text.ParseException \
    java.text.ParsePosition \
    java.text.SimpleDateFormat \
    java.util.AbstractSequentialList \
    java.util.Arrays \
    java.util.Calendar \
    java.util.Collections \
    java.util.ConcurrentModificationException \
    java.util.Currency \
    java.util.Date \
    java.util.EmptyStackException \
    java.util.GregorianCalendar \
    java.util.IdentityHashMap \
    java.util.LinkedHashMap \
    java.util.LinkedHashSet \
    java.util.LinkedList \
    java.util.ListIterator \
    java.util.ListResourceBundle \
    java.util.MissingResourceException \
    java.util.NoSuchElementException \
    java.util.PropertyPermission \
    java.util.PropertyResourceBundle \
    java.util.ResourceBundle \
    java.util.ResourceBundleEnumeration \
    java.util.SimpleTimeZone \
    java.util.SortedMap \
    java.util.SortedSet \
    java.util.StringTokenizer \
    java.util.TimeZone \
    java.util.TreeMap \
    java.util.TreeSet \
    java.util.WeakHashMap \
    java.util.jar.Attributes \
    java.util.jar.JarException \
    java.util.jar.JarInputStream \
    java.util.jar.JarVerifier \
    java.util.jar.Manifest \
    java.util.zip.CRC32 \
    java.util.zip.Checksum \
    java.util.zip.DataFormatException \
    java.util.zip.Inflater \
    java.util.zip.ZipException \
    java.util.zip.ZipInputStream \
    sun.io.ByteToCharISO8859_1 \
    sun.io.ByteToCharUTF16 \
    sun.io.ByteToCharUnicode \
    sun.io.ByteToCharUnicodeBig \
    sun.io.ByteToCharUnicodeBigUnmarked \
    sun.io.ByteToCharUnicodeLittle \
    sun.io.ByteToCharUnicodeLittleUnmarked \
    sun.io.CharToByteUTF16 \
    sun.io.CharToByteUTF8 \
    sun.io.CharToByteUnicode \
    sun.io.CharToByteUnicodeBig \
    sun.io.CharToByteUnicodeBigUnmarked \
    sun.io.CharToByteUnicodeLittle \
    sun.io.CharToByteUnicodeLittleUnmarked \
    sun.io.MarkableReader \
    sun.misc.ClassFileTransformer \
    sun.misc.BuildFlags \
    sun.misc.DefaultLocaleList \
    sun.misc.Resource \
    sun.misc.Service \
    sun.net.spi.nameservice.NameService \
    sun.net.www.MimeTable \
    sun.security.provider.PolicyFile \
    sun.security.provider.Sun \
    sun.text.Utility \
    sun.text.resources.BreakIteratorRules \
    sun.text.resources.LocaleData \
    sun.util.BuddhistCalendar \
    sun.util.calendar.CalendarDate \
    sun.util.calendar.CalendarSystem \
    sun.util.calendar.Gregorian \
    sun.util.calendar.ZoneInfo \
    sun.util.calendar.ZoneInfoFile \

ifeq ($(CVM_REFLECT), true)
CVM_BUILDTIME_CLASSES += \
    java.lang.reflect.Array \
    java.lang.reflect.InvocationHandler \
    java.lang.reflect.InvocationTargetException \
    java.lang.reflect.Proxy \
    sun.misc.ProxyGenerator \
    java.lang.reflect.UndeclaredThrowableException
endif

# %begin lvm
ifeq ($(CVM_LVM), true)
CVM_BUILDTIME_CLASSES += \
    sun.misc.LogicalVM
endif
# %end lvm

# These need to be romized to keep PMVM happy
ifeq ($(USE_JUMP), true)
CVM_BUILDTIME_CLASSES += \
    sun.io.ByteToCharUTF8 \
    sun.net.www.protocol.jar.JarFileFactory \
    sun.net.www.protocol.jar.URLJarFile
else
CLASSLIB_CLASSES += \
    sun.io.ByteToCharUTF8 \
    sun.net.www.protocol.jar.JarFileFactory \
    sun.net.www.protocol.jar.URLJarFile
endif

ifeq ($(CVM_AOT), true)
CLASSLIB_CLASSES += \
    sun.misc.Warmup
endif

CVM_POLICY_SRC  ?= $(CVM_TOP)/src/share/lib/security/java.policy

#
# JIT control
# (native support does nothing if JIT unsupported)
#
CVM_BUILDTIME_CLASSES += \
    sun.misc.JIT

#
# Classes to be loaded at runtime.
#
CLASSLIB_CLASSES += \
    java.net.BindException \
    java.net.DatagramPacket \
    java.net.DatagramSocket \
    java.net.DatagramSocketImpl \
    java.net.DatagramSocketImplFactory \
    java.net.NetworkInterface \
    java.net.PlainDatagramSocketImpl \
    java.net.PortUnreachableException \
    java.net.SocketException \
    java.net.SocketOptions \
    java.net.SocketTimeoutException \
    java.util.CurrencyData \
    sun.misc.Compare \
    sun.misc.GC \
    sun.misc.Sort \
    sun.security.provider.SHA \
    sun.text.resources.DateFormatZoneData \
    sun.text.resources.DateFormatZoneData_en \
    sun.text.resources.LocaleElements \
    sun.text.resources.LocaleElements_en \
    sun.text.resources.LocaleElements_en_US \

CLASSLIB_CLASSES += \
    com.sun.cdc.i18n.Helper \
    com.sun.cdc.i18n.StreamReader \
    com.sun.cdc.i18n.StreamWriter \
    com.sun.cdc.io.BufferedConnectionAdapter \
    com.sun.cdc.io.ConnectionBase \
    com.sun.cdc.io.ConnectionBaseAdapter \
    com.sun.cdc.io.ConnectionBaseInterface \
    com.sun.cdc.io.DateParser \
    com.sun.cdc.io.GeneralBase \
    com.sun.cdc.io.j2me.UniversalOutputStream \
    com.sun.cdc.io.j2me.datagram.DatagramObject \
    com.sun.cdc.io.j2me.datagram.Protocol \
    javax.microedition.io.CommConnection \
    javax.microedition.io.Connection \
    javax.microedition.io.ConnectionNotFoundException \
    javax.microedition.io.Connector \
    javax.microedition.io.ContentConnection \
    javax.microedition.io.Datagram \
    javax.microedition.io.DatagramConnection \
    javax.microedition.io.InputConnection \
    javax.microedition.io.OutputConnection \
    javax.microedition.io.StreamConnection \
    javax.microedition.io.StreamConnectionNotifier \
    javax.microedition.io.UDPDatagramConnection \

ifeq ($(USE_CDC_FILE_PROTOCOL), true)
CLASSLIB_CLASSES += \
   com.sun.cdc.io.j2me.file.Protocol \
   com.sun.cdc.io.j2me.file.ProtocolBase \
   com.sun.cdc.io.j2me.file.ProtocolNative
endif

#
# Classes needed for dual stack support
#
ifeq ($(CVM_DUAL_STACK), true)
    CLASSLIB_CLASSES += \
	sun.misc.MemberFilter \
	sun.misc.MemberFilterConfig \
	sun.misc.MIDPImplementationClassLoader \
	sun.misc.MIDPConfig \
	sun.misc.MIDPLauncher \
	sun.misc.MIDPBridgeInterface \
	sun.misc.CDCAppClassLoader

    CVM_OFFSETS_CLASSES += \
	sun.misc.MIDletClassLoader \

#
# Classes needed for MIDP support
#
ifeq ($(USE_MIDP), true)
    CLASSLIB_CLASSES += \
	sun.misc.MIDPInternalConnectorImpl
endif
endif

#
# Library Unit Tests
#
CVM_TESTCLASSES_SRCDIRS += \
	$(CVM_TOP)/test/share/cdc/java/util/Currency \
	$(CVM_TOP)/test/share/cdc/java/lang/ClassLoader 

CVM_TEST_CLASSES  += \
	CurrencyTest \
	package1.Class1 \
	package2.Class2 \
	package1.package3.Class3

# Don't build Assert if CVM_PRELOAD_TEST=true. It results in the JNI Assert.h
# header file being created, which causes a conflict with the system assert.h
# on platforms with a file system that is not case sensitive, like Mac OS X.
ifneq ($(CVM_PRELOAD_TEST),true)
CVM_TEST_CLASSES  += \
	Assert
endif

#
# Demo stuff
#
CVM_DEMOCLASSES_SRCDIRS += $(CVM_SHAREROOT)/cdc/demo

CVM_DEMO_CLASSES += \
    cdc.HelloWorld \

# for CurrencyData
CVM_BUILDDIRS += $(CVM_DERIVEDROOT)/classes/java/util

JAVADOC_CDC_CLASSPATH   = $(LIB_CLASSESDIR):$(CVM_BUILDTIME_CLASSESDIR)
JAVADOC_CDC_BTCLASSPATH = $(JAVADOC_CDC_CLASSPATH)
JAVADOC_CDC_SRCPATH     = $(CVM_SHAREDCLASSES_SRCDIR):$(CVM_CLDCCLASSES_SRCDIR)

CDC_REPORTS_DIR  =$(REPORTS_DIR)/cdc

include $(CDC_DIR)/build/share/defs_zoneinfo.mk

include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_cdc.mk
