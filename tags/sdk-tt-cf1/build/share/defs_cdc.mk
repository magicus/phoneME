#
# Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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

CVM_BUILDTIME_CLASSES += \
   java.io.BufferedInputStream \
   java.io.BufferedOutputStream \
   java.io.BufferedReader \
   java.io.BufferedWriter \
   java.io.ByteArrayInputStream \
   java.io.ByteArrayOutputStream \
   java.io.CharConversionException \
   java.io.DataInput \
   java.io.DataInputStream \
   java.io.DataOutput \
   java.io.DataOutputStream \
   java.io.EOFException \
   java.io.Externalizable \
   java.io.File \
   sun.net.www.ParseUtil \
   java.io.FileDescriptor \
   java.io.FileFilter \
   java.io.FileInputStream \
   java.io.FileNotFoundException \
   java.io.FileOutputStream \
   java.io.FilePermission \
   java.io.FileReader \
   java.io.FileSystem \
   java.io.FileWriter \
   java.io.FilenameFilter \
   java.io.FilterInputStream \
   java.io.FilterOutputStream \
   java.io.IOException \
   java.io.InputStream \
   java.io.InputStreamReader \
   java.io.InterruptedIOException \
   java.io.InvalidClassException \
   java.io.InvalidObjectException \
   java.io.NotActiveException \
   java.io.NotSerializableException \
   java.io.ObjectInput \
   java.io.ObjectInputStream \
   java.io.ObjectInputValidation \
   java.io.ObjectOutput \
   java.io.ObjectOutputStream \
   java.io.ObjectStreamClass \
   java.io.ObjectStreamConstants \
   java.io.ObjectStreamException \
   java.io.ObjectStreamField \
   java.io.OptionalDataException \
   java.io.OutputStream \
   java.io.OutputStreamWriter \
   java.io.PipedInputStream \
   java.io.PipedOutputStream \
   java.io.PrintStream \
   java.io.PrintWriter \
   java.io.PushbackInputStream \
   java.io.Reader \
   java.io.Serializable \
   java.io.SerializablePermission \
   java.io.StreamCorruptedException \
   java.io.StreamTokenizer \
   java.io.SyncFailedException \
   java.io.UTFDataFormatException \
   java.io.UnsupportedEncodingException \
   java.io.WriteAbortedException \
   java.io.Writer \
   java.lang.AbstractMethodError \
   java.lang.ArithmeticException \
   java.lang.ArrayIndexOutOfBoundsException \
   java.lang.ArrayStoreException \
   java.lang.AssertionError \
   java.lang.AssertionStatusDirectives \
   java.lang.Boolean \
   java.lang.Byte \
   java.lang.Character \
   java.lang.CharacterData \
   java.lang.CharacterDataLatin1 \
   java.lang.CharSequence \
   java.lang.Class \
   java.lang.ClassCastException \
   java.lang.ClassCircularityError \
   java.lang.ClassFormatError \
   java.lang.ClassLoader \
   java.lang.ClassNotFoundException \
   java.lang.CloneNotSupportedException \
   java.lang.Cloneable \
   java.lang.Comparable \
   java.lang.Double \
   java.lang.Error \
   java.lang.Exception \
   java.lang.ExceptionInInitializerError \
   java.lang.Float \
   java.lang.FloatingDecimal \
   java.lang.IllegalAccessError \
   java.lang.IllegalAccessException \
   java.lang.IllegalArgumentException \
   java.lang.IllegalMonitorStateException \
   java.lang.IllegalStateException \
   java.lang.IllegalThreadStateException \
   java.lang.IncompatibleClassChangeError \
   java.lang.IndexOutOfBoundsException \
   java.lang.InheritableThreadLocal \
   java.lang.InstantiationError \
   java.lang.InstantiationException \
   java.lang.Integer \
   java.lang.InternalError \
   java.lang.InterruptedException \
   java.lang.LinkageError \
   java.lang.Long \
   java.lang.Math \
   java.lang.NegativeArraySizeException \
   java.lang.NoClassDefFoundError \
   java.lang.NoSuchFieldError \
   java.lang.NoSuchMethodError \
   java.lang.NullPointerException \
   java.lang.Number \
   java.lang.NumberFormatException \
   java.lang.Object \
   java.lang.OutOfMemoryError \
   java.lang.Package \
   java.lang.Process \
   java.lang.Runnable \
   java.lang.Runtime \
   java.lang.RuntimeException \
   java.lang.RuntimePermission \
   java.lang.SecurityException \
   java.lang.SecurityManager \
   java.lang.Short \
   java.lang.Shutdown \
   java.lang.Terminator \
   java.lang.StackOverflowError \
   java.lang.StackTraceElement \
   java.lang.StrictMath \
   java.lang.String \
   java.lang.StringBuffer \
   java.lang.StringCoding \
   java.lang.StringIndexOutOfBoundsException \
   java.lang.System \
   java.lang.Thread \
   java.lang.ThreadDeath \
   java.lang.ThreadGroup \
   java.lang.ThreadLocal \
   java.lang.Throwable \
   java.lang.UnsatisfiedLinkError \
   java.lang.UnsupportedClassVersionError \
   java.lang.UnsupportedOperationException \
   java.lang.VerifyError \
   java.lang.VirtualMachineError \
   java.lang.Void \
   java.lang.ref.FinalReference \
   java.lang.ref.Finalizer \
   java.lang.ref.PhantomReference \
   java.lang.ref.Reference \
   java.lang.ref.ReferenceQueue \
   java.lang.ref.SoftReference \
   java.lang.ref.WeakReference \
   java.math.BigInteger \
   java.math.BitSieve \
   java.math.MutableBigInteger \
   java.math.SignedMutableBigInteger \
   java.net.ContentHandler \
   java.net.ContentHandlerFactory \
   java.net.FileNameMap \
   java.net.InetAddress \
   java.net.Inet4Address \
   java.net.Inet6Address \
   java.net.Inet4AddressImpl \
   java.net.Inet6AddressImpl \
   java.net.JarURLConnection \
   java.net.MalformedURLException \
   java.net.NetPermission \
   java.net.ProtocolException \
   java.net.SocketPermission \
   java.net.URL \
   java.net.URLClassLoader \
   java.net.URLConnection \
   sun.net.www.MimeTable \
   java.net.URLStreamHandler \
   java.net.URLStreamHandlerFactory \
   java.net.UnknownHostException \
   java.net.UnknownServiceException \
   java.security.AccessControlContext \
   java.security.AccessControlException \
   java.security.AccessController \
   java.security.AllPermission \
   java.security.BasicPermission \
   java.security.CodeSource \
   java.security.DigestException \
   java.security.DigestOutputStream \
   java.security.DomainCombiner \
   java.security.GeneralSecurityException \
   java.security.Guard \
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
   java.security.Permission \
   java.security.PermissionCollection \
   java.security.Permissions \
   java.security.Policy \
   java.security.Principal \
   java.security.PrivilegedAction \
   java.security.PrivilegedActionException \
   java.security.PrivilegedExceptionAction \
   java.security.ProtectionDomain \
   java.security.Provider \
   java.security.ProviderException \
   java.security.PublicKey \
   java.security.SecureClassLoader \
   java.security.Security \
   java.security.SecurityPermission \
   java.security.SignatureException \
   java.security.UnresolvedPermission \
   java.security.UnresolvedPermissionCollection \
   java.security.cert.Certificate \
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
   java.util.AbstractCollection \
   java.util.AbstractList \
   java.util.AbstractMap \
   java.util.AbstractSequentialList \
   java.util.AbstractSet \
   java.util.ArrayList \
   java.util.Arrays \
   java.util.BitSet \
   java.util.Calendar \
   java.util.Collection \
   java.util.Collections \
   java.util.Comparator \
   java.util.ConcurrentModificationException \
   java.util.Currency \
   java.util.Date \
   java.util.Dictionary \
   java.util.EmptyStackException \
   java.util.Enumeration \
   java.util.GregorianCalendar \
   java.util.HashMap \
   java.util.HashSet \
   java.util.Hashtable \
   java.util.IdentityHashMap \
   java.util.Iterator \
   java.util.LinkedHashMap \
   java.util.LinkedHashSet \
   java.util.LinkedList \
   java.util.List \
   java.util.ListIterator \
   java.util.ListResourceBundle \
   java.util.Locale \
   java.util.Map \
   java.util.MissingResourceException \
   java.util.NoSuchElementException \
   java.util.Properties \
   java.util.PropertyPermission \
   java.util.PropertyResourceBundle \
   java.util.Random \
   java.util.RandomAccess \
   java.util.ResourceBundle \
   java.util.ResourceBundleEnumeration \
   java.util.Set \
   java.util.SimpleTimeZone \
   java.util.SortedMap \
   java.util.SortedSet \
   java.util.Stack \
   java.util.StringTokenizer \
   java.util.TimeZone \
   java.util.TreeMap \
   java.util.TreeSet \
   java.util.Vector \
   java.util.WeakHashMap \
   java.util.jar.Attributes \
   java.util.jar.JarEntry \
   java.util.jar.JarException \
   java.util.jar.JarFile \
   java.util.jar.JarInputStream \
   java.util.jar.JarVerifier \
   java.util.jar.Manifest \
   java.util.zip.CRC32 \
   java.util.zip.Checksum \
   java.util.zip.DataFormatException \
   java.util.zip.Inflater \
   java.util.zip.InflaterInputStream \
   java.util.zip.ZipConstants \
   java.util.zip.ZipEntry \
   java.util.zip.ZipException \
   java.util.zip.ZipFile \
   java.util.zip.ZipInputStream \
   sun.misc.Resource \
   sun.misc.URLClassPath \
   sun.misc.ClassFileTransformer \
   sun.misc.CVM \
   sun.misc.Launcher \
   sun.misc.ThreadRegistry \
   sun.misc.BuildFlags \
   sun.security.action.GetPropertyAction \
   sun.security.provider.PolicyFile \
   sun.misc.Service \
   sun.misc.Version \
   sun.misc.DefaultLocaleList \
   \
   sun.io.CharToByteISO8859_1 \
   sun.io.ByteToCharISO8859_1 \
   sun.io.CharToByteUTF8 \
   sun.io.ByteToCharASCII \
   sun.io.CharToByteASCII \
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
   sun.io.Markable \
   sun.io.MarkableReader \
   \
   com.sun.cdc.config.PropertyProvider \
   com.sun.cdc.config.PropertyProviderAdapter \
   com.sun.cdc.config.DynamicProperties \
   com.sun.cdc.config.PackageManager

ifeq ($(CVM_REFLECT), true)
CVM_BUILDTIME_CLASSES += \
   java.lang.reflect.AccessibleObject \
   java.lang.reflect.Array \
   java.lang.reflect.Constructor \
   java.lang.reflect.Field \
   java.lang.reflect.InvocationHandler \
   java.lang.reflect.InvocationTargetException \
   java.lang.reflect.Member \
   java.lang.reflect.Method \
   java.lang.reflect.Modifier \
   java.lang.reflect.Proxy \
   sun.misc.ProxyGenerator \
   java.lang.reflect.ReflectPermission \
   java.lang.reflect.UndeclaredThrowableException \
   java.lang.NoSuchFieldException \
   java.lang.NoSuchMethodException
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
   java.io.ExpiringCache \
   sun.net.www.protocol.jar.Handler \
   sun.net.www.protocol.jar.JarURLConnection \
   sun.net.www.protocol.jar.JarFileFactory \
   sun.net.www.protocol.jar.URLJarFile
endif

ifeq ($(CVM_AOT), true)
CLASSLIB_CLASSES += \
   sun.mtask.Warmup
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
   sun.net.www.MessageHeader \
   sun.net.www.URLConnection \
   sun.net.www.protocol.file.FileURLConnection \
   sun.net.www.protocol.file.Handler \
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
   com.sun.cdc.io.j2me.file.Protocol \
   com.sun.cdc.io.j2me.file.ProtocolBase \
   com.sun.cdc.io.j2me.file.ProtocolNative \
   com.sun.cdc.io.j2me.UniversalOutputStream \
   com.sun.cdc.io.ConnectionBaseInterface \
   com.sun.cdc.i18n.Helper \
   com.sun.cdc.i18n.StreamReader \
   com.sun.cdc.i18n.StreamWriter

ifneq ($(USE_JUMP), true)
CLASSLIB_CLASSES += \
   sun.io.ByteToCharUTF8 \
   sun.net.www.protocol.jar.Handler \
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
	sun.misc.MIDPInternalConnectorImpl \
	sun.misc.MIDPLauncher \
	sun.misc.CDCAppClassLoader
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

include ../share/defs_zoneinfo.mk

include ../$(TARGET_OS)/defs_cdc.mk
