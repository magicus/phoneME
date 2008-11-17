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
    java.lang.Object \
    java.io.Serializable \
    java.lang.Throwable \
    java.lang.Exception \
    java.lang.RuntimeException \
    java.lang.NullPointerException \
    java.lang.IndexOutOfBoundsException \
    java.lang.ArrayIndexOutOfBoundsException \
    java.lang.ArithmeticException \
    sun.io.UnknownCharacterException \
    sun.io.ConversionBufferFullException \
    sun.io.MalformedInputException \
    java.io.CharConversionException \
    java.io.IOException \
    java.lang.Comparable \
    java.lang.CharSequence \
    java.util.Comparator \
    java.lang.String \
    java.security.PrivilegedAction \
    java.lang.Class \
    java.lang.Class$$LoadingList \
    java.lang.AssertionStatusDirectives \
    java.net.FileNameMap \
    java.net.ContentHandler \
    java.net.URLConnection \
    java.lang.ref.Reference \
    java.lang.ref.PhantomReference \
    java.lang.ref.FinalReference \
    java.lang.ref.WeakReference \
    java.lang.ref.SoftReference \
    java.lang.Long \
    java.lang.Integer \
    java.lang.Boolean \
    java.lang.Character \
    java.lang.Float \
    java.lang.Short \
    java.lang.Byte \
    java.lang.Double \
    java.lang.Cloneable \
    java.lang.ThreadLocal \
    java.lang.Number \
    java.lang.Runnable \
    java.lang.Thread \
    java.lang.StackTraceElement \
    java.util.Enumeration \
    java.security.PrivilegedActionException \
    java.security.PrivilegedExceptionAction \
    java.lang.ClassLoader \
    java.lang.ClassLoader$$NativeLibrary \
    java.lang.Shutdown \
    sun.misc.ThreadRegistry \
    sun.misc.CVM \
    java.lang.System \
    java.security.SecureClassLoader \
    java.security.CodeSource \
    java.io.File \
    sun.misc.Launcher \
    sun.misc.Launcher$$AppClassLoader \
    sun.misc.Launcher$$ClassContainer \
    java.lang.ref.Finalizer \
    java.security.AccessController \
    java.lang.StackOverflowError \
    java.lang.OutOfMemoryError \
    java.io.InvalidClassException \
    java.lang.UnsupportedOperationException \
    java.lang.StringIndexOutOfBoundsException \
    java.lang.NoSuchFieldError \
    java.lang.NoSuchMethodError \
    java.lang.NoClassDefFoundError \
    java.lang.InterruptedException \
    java.lang.InternalError \
    java.lang.InstantiationError \
    java.lang.IllegalMonitorStateException \
    java.lang.IllegalStateException \
    java.lang.IllegalAccessException \
    java.lang.IllegalArgumentException \
    java.lang.ClassNotFoundException \
    java.lang.CloneNotSupportedException \
    java.lang.ArrayStoreException \
    java.lang.ClassCastException \
    java.lang.AbstractMethodError \
    java.lang.IncompatibleClassChangeError \
    java.lang.LinkageError \
    java.lang.VirtualMachineError \
    java.net.URLStreamHandlerFactory \
    java.security.PermissionCollection \
    java.io.ObjectStreamException \
    java.net.URLClassLoader \
    java.lang.Error \
    java.lang.InstantiationException \
    java.lang.NegativeArraySizeException \
    java.lang.UnsatisfiedLinkError \
    java.lang.VerifyError \
    java.lang.UnsupportedClassVersionError \
    java.lang.IllegalAccessError \
    java.lang.ClassFormatError \
    sun.io.Markable \
    java.io.InputStream \
    java.io.FilterInputStream \
    java.util.zip.InflaterInputStream \
    java.util.zip.ZipConstants \
    java.util.zip.ZipEntry \
    java.util.jar.JarEntry \
    java.util.zip.ZipFile \
    java.util.jar.JarFile \
    java.lang.ClassCircularityError \

ifeq ($(CVM_REFLECT), true)

CVM_BUILDTIME_CLASSES_min += \
    java.lang.reflect.Member \
    java.lang.reflect.AccessibleObject \
    java.lang.reflect.Constructor \
    java.lang.reflect.Constructor$$ArgumentException \
    java.lang.reflect.Constructor$$AccessException \
    java.lang.reflect.Method \
    java.lang.reflect.Method$$ArgumentException \
    java.lang.reflect.Method$$AccessException \
    java.lang.reflect.Field \
    java.lang.NoSuchFieldException \
    java.lang.NoSuchMethodException \

endif

# We need JNI headers generated for these
# using javah, but they don't show up in the class
# list because they don't correspond to a .java file.

CVM_EXTRA_JNI_CLASSES += \
    'java.lang.ClassLoader$$NativeLibrary' \
    'sun.misc.Launcher$$AppClassLoader' \
    java.net.InetAddressImplFactory \

CVM_BUILDTIME_CLASSES_nullapp += \
    java.util.Stack \
    java.io.ObjectStreamClass \
    java.io.ObjectStreamField \
    sun.misc.SoftCache \
    java.util.AbstractMap \
    java.lang.ref.ReferenceQueue \
    java.lang.ref.ReferenceQueue$$Null \
    java.lang.ref.ReferenceQueue$$Lock \
    java.util.HashMap \
    java.util.HashMap$$Entry \
    java.util.Map$$Entry \
    java.lang.String$$CaseInsensitiveComparator \
    java.lang.Shutdown$$Lock \
    java.security.AccessControlContext \
    java.util.Properties \
    java.util.Hashtable$$EmptyEnumerator \
    java.util.Hashtable$$EmptyIterator \
    java.util.Iterator \
    java.util.Hashtable$$Entry \
    java.lang.StringCoding \
    sun.io.Converters \
    sun.security.action.GetPropertyAction \
    java.lang.ThreadLocal$$ThreadLocalMap \
    java.lang.ThreadLocal$$ThreadLocalMap$$Entry \
    java.lang.ref.Reference$$ReferenceHandler \
    java.lang.StringCoding$$StringDecoder \
    java.lang.StringCoding$$ConverterSD \
    sun.io.ByteToCharASCII \
    java.lang.reflect.ReflectPermission \
    java.security.BasicPermission \
    java.security.Permission \
    java.security.Guard \
    sun.misc.Version \
    java.io.FileInputStream \
    java.lang.ref.Finalizer$$FinalizerThread \
    java.io.FileDescriptor \
    java.io.FileOutputStream \
    java.io.OutputStream \
    java.io.BufferedInputStream \
    java.io.PrintStream \
    java.io.FilterOutputStream \
    java.io.BufferedOutputStream \
    java.io.OutputStreamWriter \
    java.io.Writer \
    sun.io.CharToByteASCII \
    java.io.BufferedWriter \
    java.lang.Terminator \
    sun.misc.Launcher$$Factory \
    sun.misc.Launcher$$1 \
    sun.security.util.Debug \
    java.io.FileSystem \
    java.io.ExpiringCache \
    sun.misc.Launcher$$3 \
    java.net.URL \
    java.io.ExpiringCache$$Entry \
    java.lang.StringCoding$$StringEncoder \
    java.lang.StringCoding$$ConverterSE \
    sun.net.www.protocol.file.Handler \
    java.net.URLStreamHandler \
    java.util.Locale \
    java.lang.CharacterDataLatin1 \
    java.net.Parts \
    java.util.HashSet \
    java.util.AbstractSet \
    java.util.Set \
    sun.misc.URLClassPath \
    java.util.ArrayList \
    sun.net.www.protocol.jar.Handler \
    java.lang.SystemClassLoaderAction \
    java.lang.Integer$$1 \
    sun.misc.Launcher$$4 \
    java.security.ProtectionDomain \
    java.security.Permissions \
    sun.net.www.ParseUtil \
    java.util.BitSet \
    sun.net.www.protocol.file.FileURLConnection \
    sun.net.www.URLConnection \
    java.net.UnknownContentHandler \
    sun.net.www.MessageHeader \
    java.io.FilePermission \
    java.io.FilePermission$$1 \
    java.io.FilePermissionCollection \
    java.security.AllPermission \
    java.security.UnresolvedPermission \
    java.lang.RuntimePermission \
    java.security.BasicPermissionCollection \
    java.lang.Long$$1 \
    java.security.Principal \
    java.security.cert.Certificate \
    java.lang.reflect.Modifier \
    java.lang.Void \
    java.lang.StringCoding \

# If JVMPI/JVMTI is enabled, ThreadGroup was added to
# CVM_OFFSETS_CLASSES and thus CVM_BUILDTIME_CLASSES_min
ifeq ($(CVM_JVMPI)$(CVM_JVMTI), falsefalse)
CVM_BUILDTIME_CLASSES_nullapp += \
    java.lang.ThreadGroup
endif

CVM_BUILDTIME_CLASSES += \
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
    java.net.InetAddress \
    java.net.Inet4Address \
    java.net.Inet6Address \
    java.net.Inet4AddressImpl \
    java.net.Inet6AddressImpl \
    java.net.InetAddressImpl \
    sun.net.spi.nameservice.NameService \
    java.net.JarURLConnection \
    java.net.MalformedURLException \
    java.net.NetPermission \
    java.net.ProtocolException \
    java.net.SocketPermission \
    sun.net.www.MimeTable \
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
    sun.security.provider.Sun \
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
    sun.text.resources.LocaleData \
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
    sun.misc.Resource \
    sun.misc.ClassFileTransformer \
    sun.security.provider.PolicyFile \
    sun.misc.Service \
    \
    sun.io.ByteToCharISO8859_1 \
    sun.io.CharToByteUTF8 \
    sun.io.CharToByteUTF16 \
    sun.io.ByteToCharUTF16 \
    sun.io.ByteToCharUnicode \
    sun.io.ByteToCharUnicodeBig \
    sun.io.ByteToCharUnicodeBigUnmarked \
    sun.io.ByteToCharUnicodeLittle \
    sun.io.ByteToCharUnicodeLittleUnmarked \
    sun.io.CharToByteUnicode \
    sun.io.CharToByteUnicodeBig \
    sun.io.CharToByteUnicodeBigUnmarked \
    sun.io.CharToByteUnicodeLittle \
    sun.io.CharToByteUnicodeLittleUnmarked \
    \
    sun.text.Utility \
    sun.text.resources.BreakIteratorRules \
    \
    sun.util.calendar.CalendarDate \
    sun.util.calendar.CalendarSystem \
    sun.util.calendar.Gregorian \
    sun.util.calendar.ZoneInfo \
    sun.util.calendar.ZoneInfoFile \
    sun.util.BuddhistCalendar \
    sun.io.MarkableReader \
    \
    com.sun.cdc.config.PropertyProvider \
    com.sun.cdc.config.PropertyProviderAdapter \
    com.sun.cdc.config.DynamicProperties \
    com.sun.cdc.config.PackageManager \

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
   sun.net.www.protocol.jar.JarURLConnection \
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
   java.net.PlainDatagramSocketImpl \
   java.net.PortUnreachableException \
   java.net.SocketException \
   java.net.SocketOptions \
   java.net.SocketTimeoutException \
   java.net.NetworkInterface \
   \
   java.util.CurrencyData \
   \
   sun.text.resources.DateFormatZoneData \
   sun.text.resources.DateFormatZoneData_en \
   sun.text.resources.LocaleElements \
   sun.text.resources.LocaleElements_en \
   sun.text.resources.LocaleElements_en_US \
   \
   sun.misc.Compare \
   sun.misc.GC \
   sun.misc.Sort \
   sun.security.provider.SHA \
   \
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
   \
   com.sun.cdc.io.BufferedConnectionAdapter \
   com.sun.cdc.io.ConnectionBaseAdapter \
   com.sun.cdc.io.ConnectionBase \
   com.sun.cdc.io.DateParser \
   com.sun.cdc.io.GeneralBase \
   com.sun.cdc.io.j2me.datagram.DatagramObject \
   com.sun.cdc.io.j2me.datagram.Protocol \
   javax.microedition.io.UDPDatagramConnection \
   com.sun.cdc.io.j2me.UniversalOutputStream \
   com.sun.cdc.io.ConnectionBaseInterface \
   com.sun.cdc.i18n.Helper \
   com.sun.cdc.i18n.StreamReader \
   com.sun.cdc.i18n.StreamWriter

ifeq ($(USE_CDC_FILE_PROTOCOL), true)
CLASSLIB_CLASSES += \
   com.sun.cdc.io.j2me.file.Protocol \
   com.sun.cdc.io.j2me.file.ProtocolBase \
   com.sun.cdc.io.j2me.file.ProtocolNative
endif

ifneq ($(USE_JUMP), true)
CLASSLIB_CLASSES += \
   sun.io.ByteToCharUTF8 \
   sun.net.www.protocol.jar.JarURLConnection \
   sun.net.www.protocol.jar.JarFileFactory
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
	sun.misc.MIDletClassLoader \
	sun.misc.MIDPLauncher \
	sun.misc.CDCAppClassLoader

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
