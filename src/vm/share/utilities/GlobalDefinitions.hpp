/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

/** \file GlobalDefinitions.hpp
 *
 * This file holds all globally used constants & types, class
 * (forward) declarations and a few frequently used utility
 * functions.
 *
 * The definitions in this file are intended to be fully
 * portable across all ports, regardless of which C++
 * compiler you are using.  Compiler-specific global
 * definitions should be placed in file
 * "/src/vm/share/utilities/GlobalDefinitions_<compilername>.hpp".
 *
 * Operating system system specific definitions
 * should be placed in file:
 * "/src/vm/os/<os_name>/OS_<os_name>.hpp".
 */

/*
 * Rename internal classes to avoid conflicts with
 * customers class names.
 */
#define AccessFlags  JVMAccessFlags
#define AllStatic  JVMAllStatic
#define AllocationDisabler  JVMAllocationDisabler
#define Arguments  JVMArguments
#define Array  JVMArray
#define ArrayClass  JVMArrayClass
#define ArrayClassDesc  JVMArrayClassDesc
#define ArrayDesc  JVMArrayDesc
#define ArrayPointer  JVMArrayPointer
#define ArrayReferenceImpl  JVMArrayReferenceImpl
#define Assembler  JVMAssembler
#define AssemblerLoopFlags  JVMAssemblerLoopFlags
#define BasicBlock  JVMBasicBlock
#define BasicOop  JVMBasicOop
#define BinaryAssembler  JVMBinaryAssembler
#define BinaryFileStream  JVMBinaryFileStream
#define BinaryFileStreamState  JVMBinaryFileStreamState
#define BinaryLabel  JVMBinaryLabel
#define BinaryObjectWriter  JVMBinaryObjectWriter
#define BinaryROMWriter  JVMBinaryROMWriter
#define Bitset  JVMBitset
#define BlockTypeFinder  JVMBlockTypeFinder
#define BlockedThread  JVMBlockedThread
#define Boundary  JVMBoundary
#define BoundaryDesc  JVMBoundaryDesc
#define Branch  JVMBranch
#define Buffer  JVMBuffer
#define BufferedFile  JVMBufferedFile
#define BufferedFileDesc  JVMBufferedFileDesc
#define ByteArrayOutputStream  JVMByteArrayOutputStream
#define ByteStream  JVMByteStream
#define BytecodeAnalyzeClosure  JVMBytecodeAnalyzeClosure
#define BytecodeClosure  JVMBytecodeClosure
#define BytecodeCompileClosure  JVMBytecodeCompileClosure
#define BytecodeHistogram  JVMBytecodeHistogram
#define BytecodeInlinerClosure  JVMBytecodeInlinerClosure
#define BytecodeOptimizer  JVMBytecodeOptimizer
#define BytecodePrintClosure  JVMBytecodePrintClosure
#define BytecodeQuickenClosure  JVMBytecodeQuickenClosure
#define Bytecodes  JVMBytecodes
#define Bytes  JVMBytes
#define CPUVariantSupport  JVMCPUVariantSupport
#define CachedFile  JVMCachedFile
#define CachedFileHandle  JVMCachedFileHandle
#define CachedFsFile  JVMCachedFsFile
#define CallInfo  JVMCallInfo
#define CallInfoRecord  JVMCallInfoRecord
#define CallInfoWriter  JVMCallInfoWriter
#define CharStream  JVMCharStream
#define CharacterStream  JVMCharacterStream
#define CheckCastStub  JVMCheckCastStub
#define ClassClassDesc  JVMClassClassDesc
#define ClassFileParser  JVMClassFileParser
#define ClassInfo  JVMClassInfo
#define ClassInfoDesc  JVMClassInfoDesc
#define ClassMatchModifier  JVMClassMatchModifier
#define ClassParserState  JVMClassParserState
#define ClassParserStateDesc  JVMClassParserStateDesc
#define ClassPathAccess  JVMClassPathAccess
#define ClassTypeImpl  JVMClassTypeImpl
#define CodeGenerator  JVMCodeGenerator
#define CodeItemSummary  JVMCodeItemSummary
#define CodeOptimizer  JVMCodeOptimizer
#define CodeSummary  JVMCodeSummary
#define CompilationContinuation  JVMCompilationContinuation
#define CompilationQueueElement  JVMCompilationQueueElement
#define CompiledMethod  JVMCompiledMethod
#define CompiledMethodCache  JVMCompiledMethodCache
#define CompiledMethodDesc  JVMCompiledMethodDesc
#define Compiler  JVMCompiler
#define CompilerLiteralAccessor  JVMCompilerLiteralAccessor
#define CompilerStatePointers  JVMCompilerStatePointers
#define CompilerStatic  JVMCompilerStatic
#define CompilerStaticPointers  JVMCompilerStaticPointers
#define CompilerStubs  JVMCompilerStubs
#define CompilerTest  JVMCompilerTest
#define ConcatenatedStream  JVMConcatenatedStream
#define ConditionDesc  JVMConditionDesc
#define ConstantPool  JVMConstantPool
#define ConstantPoolDesc  JVMConstantPoolDesc
#define ConstantPoolRewriter  JVMConstantPoolRewriter
#define ConstantTag  JVMConstantTag
#define ConstructorObj  JVMConstructorObj
#define CountObjects  JVMCountObjects
#define DeadlockFinder  JVMDeadlockFinder
#define DebugHandleMarker  JVMDebugHandleMarker
#define DebuggerEvent  JVMDebuggerEvent
#define DefaultStream  JVMDefaultStream
#define DetailedMethodEnumerator  JVMDetailedMethodEnumerator
#define DisableCompilationMatcher  JVMDisableCompilationMatcher
#define Disassembler  JVMDisassembler
#define DisassemblerEnv  JVMDisassemblerEnv
#define DisassemblerInfo  JVMDisassemblerInfo
#define DumpHistogram  JVMDumpHistogram
#define DupStream  JVMDupStream
#define Entry  JVMEntry
#define EntryActivation  JVMEntryActivation
#define EntryActivationDesc  JVMEntryActivationDesc
#define EntryFrame  JVMEntryFrame
#define EntryStub  JVMEntryStub
#define ErrorMessage  JVMErrorMessage
#define ErrorMethodCounter  JVMErrorMethodCounter
#define ErrorMethodRewriter  JVMErrorMethodRewriter
#define ErrorMethodSearcher  JVMErrorMethodSearcher
#define EventLogger  JVMEventLogger
#define ExceptionModifier  JVMExceptionModifier
#define ExcessiveGCBooster  JVMExcessiveGCBooster
#define ExecutionStack  JVMExecutionStack
#define ExecutionStackDesc  JVMExecutionStackDesc
#define ExtendedValue  JVMExtendedValue
#define FPUControl  JVMFPUControl
#define FPURegisterMap  JVMFPURegisterMap
#define FarClass  JVMFarClass
#define FarClassDesc  JVMFarClassDesc
#define FastOopInStackObj  JVMFastOopInStackObj
#define Field  JVMField
#define FieldAddress  JVMFieldAddress
#define FieldCounter  JVMFieldCounter
#define FieldEnumerator  JVMFieldEnumerator
#define FieldFinder  JVMFieldFinder
#define FieldObj  JVMFieldObj
#define FieldStore  JVMFieldStore
#define FieldType  JVMFieldType
#define FileCache  JVMFileCache
#define FileDecoder  JVMFileDecoder
#define FileDecoderDesc  JVMFileDecoderDesc
#define FilePath  JVMFilePath
#define FileStream  JVMFileStream
#define FileStreamState  JVMFileStreamState
#define FinalizerConsDesc  JVMFinalizerConsDesc
#define FixedArrayOutputStream  JVMFixedArrayOutputStream
#define FlatProfiler  JVMFlatProfiler
#define FloatSupport  JVMFloatSupport
#define ForwardBranchOptimizer  JVMForwardBranchOptimizer
#define Frame  JVMFrame
#define FrameStream  JVMFrameStream
#define FunctionDefinition  JVMFunctionDefinition
#define GCDisabler  JVMGCDisabler
#define GPTableGenerator  JVMGPTableGenerator
#define Generator  JVMGenerator
#define GenericAddress  JVMGenericAddress
#define GenericClassInfoDesc  JVMGenericClassInfoDesc
#define GetInstructionStarts  JVMGetInstructionStarts
#define GlobalObj  JVMGlobalObj
#define GlobalSaver  JVMGlobalSaver
#define Globals  JVMGlobals
#define HeapAddress  JVMHeapAddress
#define IncompatibleClassChangeStub  JVMIncompatibleClassChangeStub
#define IndexCheckStub  JVMIndexCheckStub
#define IndexableField  JVMIndexableField
#define IndexedAddress  JVMIndexedAddress
#define Inflater  JVMInflater
#define InflaterDesc  JVMInflaterDesc
#define Initer  JVMIniter
#define Instance  JVMInstance
#define InstanceClass  JVMInstanceClass
#define InstanceClassDesc  JVMInstanceClassDesc
#define InstanceDesc  JVMInstanceDesc
#define InstanceOfStub  JVMInstanceOfStub
#define InstanceSize  JVMInstanceSize
#define Instruction  JVMInstruction
#define IntelHalHardwareTimerForNucleus  JVMIntelHalHardwareTimerForNucleus
#define InternalCodeOptimizer  JVMInternalCodeOptimizer
#define InterpreterGenerator  JVMInterpreterGenerator
#define InterpreterStubs  JVMInterpreterStubs
#define IsolateNatives  JVMIsolateNatives
#define IsolateObj  JVMIsolateObj
#define JamClientSignal  JVMJamClientSignal
#define JarFile  JVMJarFile
#define JarInfoEntry  JVMJarInfoEntry
#define JavaClass  JVMJavaClass
#define JavaClassDesc  JVMJavaClassDesc
#define JavaClassObj  JVMJavaClassObj
#define JavaClassPatternMatcher  JVMJavaClassPatternMatcher
#define JavaDebugger  JVMJavaDebugger
#define JavaDesc  JVMJavaDesc
#define JavaFrame  JVMJavaFrame
#define JavaITable  JVMJavaITable
#define JavaNear  JVMJavaNear
#define JavaNearDesc  JVMJavaNearDesc
#define JavaOop  JVMJavaOop
#define JavaVTable  JVMJavaVTable
#define JvmTimer  JVMJvmTimer
#define KvmNativesMatcher  JVMKvmNativesMatcher
#define LargeObject  JVMLargeObject
#define LineNumberTable  JVMLineNumberTable
#define LineVarTable  JVMLineVarTable
#define LinkedBasicOop  JVMLinkedBasicOop
#define LiteralAccessor  JVMLiteralAccessor
#define LiteralElementStream  JVMLiteralElementStream
#define LiteralPoolElement  JVMLiteralPoolElement
#define LiteralStream  JVMLiteralStream
#define LiveRange  JVMLiveRange
#define LoaderContext  JVMLoaderContext
#define LocalVariableTable  JVMLocalVariableTable
#define Location  JVMLocation
#define LocationAddress  JVMLocationAddress
#define LocationModifier  JVMLocationModifier
#define Macros  JVMMacros
#define MemAccess  JVMMemAccess
#define MemCounter  JVMMemCounter
#define MemberObj  JVMMemberObj
#define MemoryAddress  JVMMemoryAddress
#define MemoryProfiler  JVMMemoryProfiler
#define MetaClass  JVMMetaClass
#define MetaObjType  JVMMetaObjType
#define MetaObjTypeDesc  JVMMetaObjTypeDesc
#define Method  JVMMethod
#define MethodCounter  JVMMethodCounter
#define MethodDesc  JVMMethodDesc
#define MethodEnumerator  JVMMethodEnumerator
#define MethodFinder  JVMMethodFinder
#define MethodInvocationClosure  JVMMethodInvocationClosure
#define MethodNode  JVMMethodNode
#define MethodObj  JVMMethodObj
#define MethodReplacer  JVMMethodReplacer
#define MethodStore  JVMMethodStore
#define MethodTrap  JVMMethodTrap
#define MethodTrapDesc  JVMMethodTrapDesc
#define MethodVariablePart  JVMMethodVariablePart
#define MixedOop  JVMMixedOop
#define MixedOopDesc  JVMMixedOopDesc
#define NamedField  JVMNamedField
#define NativeGenerator  JVMNativeGenerator
#define Natives  JVMNatives
#define Near  JVMNear
#define NearClass  JVMNearClass
#define NearClassDesc  JVMNearClassDesc
#define NearDesc  JVMNearDesc
#define NucleusSupplement  JVMNucleusSupplement
#define NullCheckStub  JVMNullCheckStub
#define OSRStub  JVMOSRStub
#define ObjArray  JVMObjArray
#define ObjArrayClass  JVMObjArrayClass
#define ObjArrayClassDesc  JVMObjArrayClassDesc
#define ObjArrayDesc  JVMObjArrayDesc
#define ObjNear  JVMObjNear
#define ObjNearDesc  JVMObjNearDesc
#define ObjectHeap  JVMObjectHeap
#define ObjectHeapVisitor  JVMObjectHeapVisitor
#define ObjectReferenceImpl  JVMObjectReferenceImpl
#define ObjectWriter  JVMObjectWriter
#define OffsetFinder  JVMOffsetFinder
#define OffsetVector  JVMOffsetVector
#define Oop  JVMOop
#define OopDesc  JVMOopDesc
#define OopDispatcher  JVMOopDispatcher
#define OopPrinter  JVMOopPrinter
#define OopROMVisitor  JVMOopROMVisitor
#define OopVisitor  JVMOopVisitor
#define OptimizerInstruction  JVMOptimizerInstruction
#define OriginalField  JVMOriginalField
#define Os  JVMOs
#define PacketDataBuffer  JVMPacketDataBuffer
#define PacketDataBufferDesc  JVMPacketDataBufferDesc
#define PacketInputStream  JVMPacketInputStream
#define PacketOutputStream  JVMPacketOutputStream
#define PacketStream  JVMPacketStream
#define PairHistogram  JVMPairHistogram
#define ParsedTypeSymbol  JVMParsedTypeSymbol
#define PendingLink  JVMPendingLink
#define PendingLinkDesc  JVMPendingLinkDesc
#define PostIndex  JVMPostIndex
#define PreIndex  JVMPreIndex
#define PrecompileMatcher  JVMPrecompileMatcher
#define PreserveVirtualStackFrameState  JVMPreserveVirtualStackFrameState
#define PrintClasses  JVMPrintClasses
#define PrintDispatcher  JVMPrintDispatcher
#define PrintObjects  JVMPrintObjects
#define PrinterWrapper  JVMPrinterWrapper
#define Profiler  JVMProfiler
#define ProfilerNode  JVMProfilerNode
#define QuickCatchStub  JVMQuickCatchStub
#define QuickNativesMatcher  JVMQuickNativesMatcher
#define ROM  JVMROM
#define ROMBundle  JVMROMBundle
#define ROMHashtableManager  JVMROMHashtableManager
#define ROMImage  JVMROMImage
#define ROMInliner  JVMROMInliner
#define ROMLookupTable  JVMROMLookupTable
#define ROMLookupTableDesc  JVMROMLookupTableDesc
#define ROMOptimizer  JVMROMOptimizer
#define ROMProfile  JVMROMProfile
#define ROMProfileDesc  JVMROMProfileDesc
#define ROMStructsWriter  JVMROMStructsWriter
#define ROMTableInfo  JVMROMTableInfo
#define ROMTools  JVMROMTools
#define ROMUniqueObjectTable  JVMROMUniqueObjectTable
#define ROMVector  JVMROMVector
#define ROMVectorDesc  JVMROMVectorDesc
#define ROMWriter  JVMROMWriter
#define ROMizerHashEntry  JVMROMizerHashEntry
#define ROMizerHashEntryDesc  JVMROMizerHashEntryDesc
#define RawLocation  JVMRawLocation
#define RawLocationData  JVMRawLocationData
#define RefArray  JVMRefArray
#define RefNode  JVMRefNode
#define RefNodeDesc  JVMRefNodeDesc
#define ReferenceTypeImpl  JVMReferenceTypeImpl
#define ReflectNatives  JVMReflectNatives
#define RegisterAllocator  JVMRegisterAllocator
#define RegisterReferenceChecker  JVMRegisterReferenceChecker
#define Relocation  JVMRelocation
#define RelocationReader  JVMRelocationReader
#define RelocationStream  JVMRelocationStream
#define RemoteTracer  JVMRemoteTracer
#define RobocopInstruction  JVMRobocopInstruction
#define RobocopTemplate  JVMRobocopTemplate
#define RomOopVisitor  JVMRomOopVisitor
#define RuntimeFrame  JVMRuntimeFrame
#define Scheduler  JVMScheduler
#define Segment  JVMSegment
#define SegmentedSourceROMWriter  JVMSegmentedSourceROMWriter
#define Semaphore  JVMSemaphore
#define SemaphoreLock  JVMSemaphoreLock
#define SerialTransport  JVMSerialTransport
#define SharedStubs  JVMSharedStubs
#define Signature  JVMSignature
#define SignatureStream  JVMSignatureStream
#define SocketTransport  JVMSocketTransport
#define SocketTransportDesc  JVMSocketTransportDesc
#define SourceAssembler  JVMSourceAssembler
#define SourceMacros  JVMSourceMacros
#define SourceObjectWriter  JVMSourceObjectWriter
#define SourceROMWriter  JVMSourceROMWriter
#define StackAddress  JVMStackAddress
#define StackFrameImpl  JVMStackFrameImpl
#define StackLock  JVMStackLock
#define StackObj  JVMStackObj
#define StackOverflowStub  JVMStackOverflowStub
#define StackValue  JVMStackValue
#define StackmapChecker  JVMStackmapChecker
#define StackmapGenerator  JVMStackmapGenerator
#define StackmapList  JVMStackmapList
#define StackmapListDesc  JVMStackmapListDesc
#define StaticBufferChecker  JVMStaticBufferChecker
#define StepModifier  JVMStepModifier
#define Stream  JVMStream
#define String  JVMString
#define StringCounter  JVMStringCounter
#define StringGatherer  JVMStringGatherer
#define StringProxyLockDesc  JVMStringProxyLockDesc
#define StringReferenceImpl  JVMStringReferenceImpl
#define StringTable  JVMStringTable
#define StringTableInfo  JVMStringTableInfo
#define StringTableVisitor  JVMStringTableVisitor
#define SumNode  JVMSumNode
#define Symbol  JVMSymbol
#define SymbolDesc  JVMSymbolDesc
#define SymbolField  JVMSymbolField
#define SymbolStream  JVMSymbolStream
#define SymbolTable  JVMSymbolTable
#define SymbolTableInfo  JVMSymbolTableInfo
#define Symbols  JVMSymbols
#define Synchronizer  JVMSynchronizer
#define SystemAllocation  JVMSystemAllocation
#define SystemClassStream  JVMSystemClassStream
#define SystemDictionary  JVMSystemDictionary
#define SystemOutStream  JVMSystemOutStream
#define Task  JVMTask
#define TaskAllocationContext  JVMTaskAllocationContext
#define TaskContext  JVMTaskContext
#define TaskContextSave  JVMTaskContextSave
#define TaskDesc  JVMTaskDesc
#define TaskDescPointers  JVMTaskDescPointers
#define TaskGCContext  JVMTaskGCContext
#define TaskGCContextDebug  JVMTaskGCContextDebug
#define TaskList  JVMTaskList
#define TaskMemoryInfo  JVMTaskMemoryInfo
#define TaskMirror  JVMTaskMirror
#define TaskMirrorDesc  JVMTaskMirrorDesc
#define TempRegister  JVMTempRegister
#define Template  JVMTemplate
#define TemplateTable  JVMTemplateTable
#define TemporaryModifyGlobal  JVMTemporaryModifyGlobal
#define TemporarySwitchTask  JVMTemporarySwitchTask
#define TemporarySwitchTaskTo  JVMTemporarySwitchTaskTo
#define TextKlassLookupTable  JVMTextKlassLookupTable
#define Thread  JVMThread
#define ThreadDesc  JVMThreadDesc
#define ThreadDescPointers  JVMThreadDescPointers
#define ThreadObj  JVMThreadObj
#define ThreadReferenceImpl  JVMThreadReferenceImpl
#define Throw  JVMThrow
#define ThrowExceptionStub  JVMThrowExceptionStub
#define Throwable  JVMThrowable
#define Ticks  JVMTicks
#define TimerTickStub  JVMTimerTickStub
#define TransferGraph  JVMTransferGraph
#define Transport  JVMTransport
#define TransportDesc  JVMTransportDesc
#define Traps  JVMTraps
#define TypeArray  JVMTypeArray
#define TypeArrayClass  JVMTypeArrayClass
#define TypeArrayClassDesc  JVMTypeArrayClassDesc
#define TypeArrayDesc  JVMTypeArrayDesc
#define TypeCheckStub  JVMTypeCheckStub
#define TypeSymbol  JVMTypeSymbol
#define UTF8Stream  JVMUTF8Stream
#define UnicodeStream  JVMUnicodeStream
#define Universe  JVMUniverse
#define UnlockExceptionStub  JVMUnlockExceptionStub
#define UpdateManager  JVMUpdateManager
#define UsingFastOops  JVMUsingFastOops
#define VMEvent  JVMVMEvent
#define VMEventDesc  JVMVMEventDesc
#define VMEventModifier  JVMVMEventModifier
#define VMEventModifierDesc  JVMVMEventModifierDesc
#define VMEventModifierDescPointers  JVMVMEventModifierDescPointers
#define VMEventStream  JVMVMEventStream
#define VMImpl  JVMVMImpl
#define VSFMergeTest  JVMVSFMergeTest
#define VSFMergeTester  JVMVSFMergeTester
#define VSFStream  JVMVSFStream
#define Value  JVMValue
#define Verifier  JVMVerifier
#define VerifierFrame  JVMVerifierFrame
#define VerifyMethodCodes  JVMVerifyMethodCodes
#define VerifyObjects  JVMVerifyObjects
#define VerifyOopWriteBarrier  JVMVerifyOopWriteBarrier
#define VerifyReferencesToLargeObjectArea  JVMVerifyReferencesToLargeObjectArea
#define VirtualStackFrame  JVMVirtualStackFrame
#define VirtualStackFrameDesc  JVMVirtualStackFrameDesc
#define VisitDispatcher  JVMVisitDispatcher
#define VisitorField  JVMVisitorField
#define WTKProfiler  JVMWTKProfiler
#define WTKThreadRecord  JVMWTKThreadRecord
#define WTKThreadRecordDesc  JVMWTKThreadRecordDesc
#define WeakReference  JVMWeakReference
#define ZeroDivisorCheckStub  JVMZeroDivisorCheckStub


//----------------------------------------------
// Forward declaration of classes -- these can be used by headers that
// only use pointer types to these classes.

class AccessFlags;
class Array;
class ArrayClassDesc;
class BinaryAssembler;
class BufferedFile;
class BytecodeClosure;
class BytecodeCompileClosure;
class CallInfo;                    // used by ENABLE_EMBEDDED_CALLINFO only
class CharacterStream;
class ClassClassDesc;
class ClassInfo;
class CompilationQueueElement;
class CompiledMethod;
class CompiledMethodDesc;
class Compiler;
class CodeGenerator;
class ConstantPool;
class ConstantPoolDesc;
class ConstantPoolRewriter;
class Entry;
class EntryActivation;
class FarClass;
class FarClassDesc;
class Field;
class FilePath;
class FinalizerConsDesc;
class FPURegisterMap;
class Frame;
class InstanceClass;
class InstanceClassDesc;
class InstanceDesc;
class IsolateObj;                 // Used by ENABLE_ISOLATES only
class JarFileParser;
class JavaClass;
class JavaClassObj;
class JavaNear;
class MetaClass;
class Method;
class MethodDesc;
class Near;
class NearClass;
class ObjArray;
class ObjArrayClass;
class ObjArrayClassDesc;
class ObjArrayDesc;
class ObjectHeap;
class ObjectHeapVisitor;
class ObjNear;
class Oop;
class OopVisitor;
class OopPrinter;
class Os;
class RefArray;
class ROM;
class ROMBundle;
class ROMImage;
class ROMOptimizer;
class ROMProfile;
class ROMWriter;
class Signature;
class StackLock;
class StackmapList;
class String;
class StringTable;
class Symbol;
class SymbolDesc;
class SymbolStream;
class SymbolTable;
class Task;                 // Used by ENABLE_ISOLATES only
class TaskAllocationContext;
class TaskContext;
class TaskMirror;           // Used by ENABLE_ISOLATES only
class TaskMirrorDesc;       // Used by ENABLE_ISOLATES only
class TemplateTable;
class Thread;
class ThreadDesc;
class ThreadObj;
class TypeArray;
class TypeArrayClass;
class TypeArrayClassDesc;
class TypeArrayDesc;
class VirtualStackFrame;
class VirtualStackFrameDesc;

//----------------------------------------------------------------------------
// Constants

const int LogBytesPerByte    = 0;
const int LogBytesPerShort   = 1;
const int LogBytesPerInt     = 2;
const int LogBytesPerWord    = 2;
const int LogBytesPerLong    = 3;

const int BytesPerByte   = 1 << LogBytesPerByte;          //1 (BytesPerByte)
const int BytesPerShort  = 1 << LogBytesPerShort;         //2 (BytesPerShort)
const int BytesPerInt    = 1 << LogBytesPerInt;           //4 (BytesPerInt)
const int BytesPerWord   = 1 << LogBytesPerWord;          //4 (BytesPerWord)
const int BytesPerLong   = 1 << LogBytesPerLong;          //8 (BytesPerLong)

const int LogBitsPerByte = 3;                             //3 (LogBitsPerByte)
const int LogBitsPerShort=LogBitsPerByte+LogBytesPerShort;//4 (LogBitsPerShort)
const int LogBitsPerInt  =LogBitsPerByte+LogBytesPerInt;  //5 (LogBitsPerInt)
const int LogBitsPerWord =LogBitsPerByte+LogBytesPerWord; //5 (LogBitsPerWord)
const int LogBitsPerLong =LogBitsPerByte+LogBytesPerLong; //6 (LogBitsPerLong)

const int BitsPerByte    = 1 << LogBitsPerByte;           // 8 (BitsPerByte)
const int BitsPerShort   = 1 << LogBitsPerShort;          //16 (BitsPerShort)
const int BitsPerInt     = 1 << LogBitsPerInt;            //32 (BitsPerInt)
const int BitsPerWord    = 1 << LogBitsPerWord;           //32 (BitsPerWord)
const int BitsPerLong    = 1 << LogBitsPerLong;           //64 (BitsPerLong)

const int WordAlignmentMask  = (1 << LogBytesPerWord) - 1;// 0x00000003
const int LongAlignmentMask  = (1 << LogBytesPerLong) - 1;// 0x00000007

const int WordsPerLong       = 2;    // Number of stack entries for longs

const int oopSize            = sizeof(char*);
const int wordSize           = sizeof(char*);
const int longSize           = sizeof(jlong);
const int jintSize         = sizeof(jint);

const int BitsPerJavaInteger = 32;

const jint min_jint          = 0x80000000;    // smallest jint
const jint max_jint          = 0x7FFFFFFF;    // largest jint

//----------------------------------------------------------------------------
// Java type definitions

#ifndef NULL
#define NULL 0
#endif

// All kinds of 'plain' byte addresses
typedef unsigned char* address;
typedef const unsigned char* const_address;
typedef uintptr_t      address_word; // unsigned integer which will hold a
                                     // pointer except for some
                                     // implementations of a C++
                                     // linkage pointer to
                                     // function. Should never need
                                     // one of those to be placed in
                                     // this type anyway.

//  Utility functions to "portably" (?) bit twiddle pointers
//  Where portable means keep ANSI C++ compilers quiet

inline address set_address_bits(address x, int m) {
  return address(intptr_t(x) | m);
}
inline address clear_address_bits(address x, int m) {
  return address(intptr_t(x) & ~m);
}

//  Utility functions to "portably" make cast to/from function pointers.

inline address_word mask_address_bits(address x, int m) {
  return address_word(x) & m;
}
inline address_word castable_address(address x) {
  return address_word(x);
}
inline address_word castable_address(void* x) {
  return address_word(x);
}

//
// ANSI C++ does not allow casting from one pointer type to a function
// pointer directly without at best a warning. This macro accomplishes
// it silently In every case that is present at this point the value
// be cast is a pointer to a C linkage function. In somecase the type
// used for the cast reflects that linkage and a picky compiler would
// not complain. In other cases because there is no convenient place
// to place a typedef with extern C linkage (i.e a platform dependent
// header file) it doesn't. At this point no compiler seems picky
// enough to catch these instances (which are few). It is possible
// that using templates could fix these for all cases. This use of
// templates is likely so far from the middle of the road that it is
// likely to be problematic in many C++ compilers.
//
#define CAST_TO_FN_PTR(func_type, value) \
    ((func_type)(castable_address(value)))
#define CAST_FROM_FN_PTR(new_type, func_ptr) \
    ((new_type)((address_word) (func_ptr)))

#define DISTANCE(low, high)        (((jint) (high)) - ((jint) (low)))
#define DERIVED(type, ptr, offset) ((type) (((jint) (ptr))+((jint) (offset))))

#ifdef FIELD_OFFSET
#undef FIELD_OFFSET
#endif

// Field offset macro.
// NOTE: we use such temporary solution (that costs nothing in runtime anyway)
//       because GCC 3.x tries to protect us from cases where we compute offset
//       of class fields (dangerous is case of virtual inheritance), and it
//       detects such cases by checking if we access fields of NULL pointer
#define JVM_FIELD_OFFSET(type, field) (((size_t)(&((type*)1)->field)) - 1)
#define FIELD_OFFSET JVM_FIELD_OFFSET


// This macro is used to put long long constants into sources in portable way
// put into shared file as every compiler beside MS works in same way,
// for absolute cleaness - move to GlobalDefinitions_<compiler>.hpp
#ifdef _MSC_VER
#define JVM_ULL(x) x##L
#define JVM_LL(x)  x##L
#define JVM_LLD    "%I64d"
#elif UNDER_ADS
#define JVM_ULL(x) x##ULL
#define JVM_LL(x)  x##LL
#define JVM_LLD    "%lld"
#else
#define JVM_ULL(x) x##LL
#define JVM_LL(x)  x##LL
#define JVM_LLD    "%lld"
#endif

// Note that a bool is an int, not a char. The reason is for code like this
//
//     bool isxxx() {return pointer != NULL;}
//
// A clever compiler would simply return the pointer. If bool is
// defined to be a char, the compiler must do a compare and return (char)0
// or (char)1. Also, for code like this:
//
//    bool isxxx() {return pointer;}
//
// Some compiler may actually generate code that sign extends the lowest
// 2 bytes of pointer, which would be the unexpected value of 0x00 if
// you have pointer == 0x12345600.
//
// If you want to store a boolean field in a structure and save space, use
// jboolean instead (jboolean is a char). Whenever casting anything to
// a jboolean (e.g., when you call OopDesc::bool_field_put(), always use the
// CAST_TO_JBOOLEAN.

#define bool    jint

#define true    v_true
#define false   v_false

const bool true        = 1;
const bool false       = 0;

const jboolean g_true  = 1;
const jboolean g_false = 0;

#define CAST_TO_JBOOLEAN(i) ((i) ? ((jboolean) 1) : ((jboolean) 0))

//----------------------------------------------------------------------------
// JVM spec restrictions

// JVM spec, 2nd ed. section 4.8.1 (p.134)
const int max_method_code_size = 64 * 1024 - 1;

// Machine dependent stuff
#include "incls/_GlobalDefinitions_pd.hpp.incl"

inline size_t align_allocation_size(size_t size) {
  return (size + sizeof(jobject) - 1) & ~(sizeof(jobject) - 1);
}

// signed variants of alignment helpers
inline intptr_t align_size_up(intptr_t size, intptr_t alignment) {
  return (size + (alignment - 1)) & ~(alignment - 1);
}

inline intptr_t align_size_down(intptr_t size, intptr_t alignment) {
  return (size) & ~(alignment - 1);
}

//----------------------------------------------------------------------------
// String macros

// Makes a string of the argument (which is not macro-expanded)
#define STR(a)  #a

// Makes a string of the macro expansion of a
#define XSTR(a) STR(a)

//----------------------------------------------------------------------------
// Miscellaneous

#define JAVA_MIN_SUPPORTED_VERSION           45
#define JAVA_MAX_SUPPORTED_VERSION           48

#ifdef ARM
#define RESCHEDULE_COUNT 0x8000
#else
#define RESCHEDULE_COUNT 200000
#endif

#if !CROSS_GENERATOR
/*
 * On a target build, make this a compile-time constant so that the C++
 * compiler can eliminate dead code
 */
#  if ENABLE_INCREASING_JAVA_STACK
#  define JavaStackDirection  1
#  else
#  define JavaStackDirection -1
#  endif

  class EnforceCompilerJavaStackDirection {
  public:
    EnforceCompilerJavaStackDirection() {}
  };

  class EnforceRuntimeJavaStackDirection  {
  public:
    EnforceRuntimeJavaStackDirection() {}
  };


#else
/*
 * In a cross-generator build, especially in an AOT-enabled romizer,
 * JavaStackDirection needs to be a variable because the compiler may use
 * a different JavaStackDirection than everyone else.
 */
  extern "C" int JavaStackDirection;
  extern "C" int CompilerJavaStackDirection;

  // The "Runtime" -- interpreter, gc, etc, strictly use JavaStackDirection
  // according to ENABLE_INCREASING_JAVA_STACK
  class EnforceRuntimeJavaStackDirection {
    int saved_value;
  public:
    EnforceRuntimeJavaStackDirection() {
      saved_value = JavaStackDirection;
      JavaStackDirection = (ENABLE_INCREASING_JAVA_STACK) ? 1 : -1;
    }

    ~EnforceRuntimeJavaStackDirection() {
      JavaStackDirection = saved_value;
    }
  };

  // The Compiler may use an value for CompilerJavaStackDirection
  // that's opposite to JavaStackDirection. Currently the only case is
  // for Jazelle-enabled AOT compiler.
  class EnforceCompilerJavaStackDirection {
    int saved_value;
  public:
    EnforceCompilerJavaStackDirection() {
      saved_value = JavaStackDirection;
      JavaStackDirection = CompilerJavaStackDirection;
    }

    ~EnforceCompilerJavaStackDirection() {
      JavaStackDirection = saved_value;
    }
  };

#endif

// This class provides uniform access to arrays
// both in Java Heap and Native Heap
class ArrayPointer {
protected:
  Array*   _heap_array;
  int      _offset;

public:
  ArrayPointer(Array* heap_array, int offset = 0) :
    _heap_array(heap_array), _offset(offset) {}

  ArrayPointer(address native_addr) :
    _heap_array(NULL), _offset((int) native_addr) {}

  bool in_java_heap() {
    return _heap_array != NULL;
  }

  address base_address();
};

#if ENABLE_JAVA_STACK_TAGS
#define TaggedJavaStack         1
#define WordsPerStackElement    2
#define LogWordsPerStackElement 1
#else
#define TaggedJavaStack         0
#define WordsPerStackElement    1
#define LogWordsPerStackElement 0
#endif

#define BytesPerStackElement (BytesPerWord * WordsPerStackElement)
#define LogBytesPerStackElement (LogBytesPerWord + LogWordsPerStackElement)

// Use of this type is like a comment that we know we are dealing with
// a UTF-8 string pointed to by a 'char *'
typedef char *utf8;

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define STATIC_STRLEN(s) (sizeof(s) - sizeof((s)[0]))

inline jint lsw (jlong value) { return jint(value); }
inline jint msw (jlong value) { return jint(value >> 32); }

union jlong_accessor {
  jint  words[2];
  jlong long_value;
};

union jdouble_accessor {
  jint     words[2];
  jdouble  double_value;
};

#if MSW_FIRST_FOR_DOUBLE
#define WORD_FOR_MSW_IN_DOUBLE 0
#define WORD_FOR_LSW_IN_DOUBLE 1
#else
#define WORD_FOR_MSW_IN_DOUBLE 1
#define WORD_FOR_LSW_IN_DOUBLE 0
#endif

#if MSW_FIRST_FOR_LONG
#define WORD_FOR_MSW_IN_LONG 0
#define WORD_FOR_LSW_IN_LONG 1
#else
#define WORD_FOR_MSW_IN_LONG 1
#define WORD_FOR_LSW_IN_LONG 0
#endif

inline jlong jlong_from_msw_lsw(jint msw, jint lsw) {
  jlong_accessor result;
  result.words[WORD_FOR_MSW_IN_LONG] = msw;
  result.words[WORD_FOR_LSW_IN_LONG] = lsw;
  return result.long_value;
}

// Use this when we know the first (low address) word and the second (high
// address) word.  This eliminates issues of endianness
inline jlong jlong_from_low_high(jint low, jint high) {
  jlong_accessor result;

  result.words[0] = low;
  result.words[1] = high;

  return result.long_value;
}

inline jdouble jdouble_from_msw_lsw(jint msw, jint lsw) {
  jdouble_accessor result;

  result.words[WORD_FOR_MSW_IN_DOUBLE] = msw;
  result.words[WORD_FOR_LSW_IN_DOUBLE] = lsw;
  return result.double_value;
}

inline jdouble jdouble_from_low_high(jint low, jint high) {
  jdouble_accessor result;

  result.words[0] = low;
  result.words[1] = high;
  return result.double_value;
}

enum BasicType {
  T_BOOLEAN   =  4,  // 0x04  0b0100
  T_CHAR      =  5,  // 0x05  0b0101
  T_FLOAT     =  6,  // 0x06  0b0110
  T_DOUBLE    =  7,  // 0x07  0b0111
  T_BYTE      =  8,  // 0x08  0b1000
  T_SHORT     =  9,  // 0x09  0b1001
  T_INT       = 10,  // 0x0a  0b1010
  T_LONG      = 11,  // 0x0b  0b1011
  T_OBJECT    = 12,  // 0x0c  0b1100
  T_ARRAY     = 13,  // 0x0d  0b1101
  T_VOID      = 14,  // 0x0e  0b1110
  T_SYMBOLIC  = 15,                   // used *only* during ROMization
  T_ILLEGAL   = 32,
  _force_32bit_BasicType = 0x70000000
};

enum FailureMode {
  ExceptionOnFailure = 0,
  ErrorOnFailure     = 1
};

extern const jubyte stack_type_for_table[];
extern const jubyte word_size_for_table[];
extern const jubyte simple_type_symbol_table[];

// Tells whether type is T_LONG or T_DOUBLE
inline bool is_two_word(BasicType type) {
  // See binary values in declaration of BasicType enum.
  return (((juint)type) & 0x03) == 0x03;
}

// Get the stack type for a given basic type
inline BasicType stack_type_for(BasicType type) {
  // IMPL_NOTE: add GUARANTEE
  return (BasicType)(stack_type_for_table[(int)type]);
}

inline int word_size_for(BasicType type) {
  // IMPL_NOTE: add GUARANTEE
  return (int)(word_size_for_table[(int)type]);
}

#ifndef PRODUCT
const char * name_for(BasicType type);
#endif

// Tells if the given character is one of case 'B''C''D''F''I''J''S''Z'
inline bool is_simple_type_symbol(int c) {
  c -= 'B';
  if (((juint)c) <= 'Z' - 'B') {
    return (bool)simple_type_symbol_table[c];
  } else {
    return false;
  }
}

enum BasicTypeWordSize {
  T_BOOLEAN_word_size = 1,
  T_CHAR_word_size    = 1,
  T_FLOAT_word_size   = 1,
  T_DOUBLE_word_size  = 2,
  T_BYTE_word_size    = 1,
  T_SHORT_word_size   = 1,
  T_INT_word_size     = 1,
  T_LONG_word_size    = 2,
  T_OBJECT_word_size  = 1,
  T_ARRAY_word_size   = 1,
  T_SYMBOLIC_word_size= 1,
  T_VOID_word_size    = 0
};

enum BasicTypeByteSize {
  T_BOOLEAN_byte_size = 1,
  T_CHAR_byte_size    = 2,
  T_FLOAT_byte_size   = 4,
  T_DOUBLE_byte_size  = 8,
  T_BYTE_byte_size    = 1,
  T_SHORT_byte_size   = 2,
  T_INT_byte_size     = 4,
  T_LONG_byte_size    = 8,
  T_OBJECT_byte_size  = 4,
  T_ARRAY_byte_size   = 4,
  T_SYMBOLIC_byte_size= 4,
  T_VOID_byte_size    = 0
};

// Tags for stack marking
// Note: It is crucial that each tag gets its own bit because occasionally
//       we want to test for more than one tag simultaneously; e.g.,
//       this is done in the
//       SharedStubs::generate_shared_invoke_method() when setting up
//       parameters for Java activation (actually there it is
//       sufficient to be able to check for {int, float} and {long,
//       double} simultaneously, which is possible with an encoding
//       that uses fewer bits, if necessary).
enum Tag {
  uninitialized_tag =      0,
  obj_tag           = 1 << 0,
  int_tag           = 1 << 1,
  long_tag          = 1 << 2,
  long2_tag         = 1 << 3,
  float_tag         = 1 << 4,
  double_tag        = 1 << 5,
  double2_tag       = 1 << 6,
  ret_tag           = 1 << 7,
  _force_32bit_Tag  = 0x70000000
};

Tag basic_type2tag(BasicType t);
int byte_size_for(BasicType type);

//----------------------------------------------------------------------------

// Basic support for errors (general debug facilities not defined at this
// point fo the include phase)

#ifdef AZZERT
void power_of_2_fatal();
void check_basic_types();
#endif

//----------------------------------------------------------------------------
// Utility functions for bitfield manipulations

const intptr_t AllBits = ~0;   // all bits set in a word
const intptr_t NoBits  =  0;   // no bits set in a word
const intptr_t OneBit  =  1;   // only right_most bit set in a word

// get a word with the n.th or the right-most n bits set
// (note: #define used only so that they can be used in enum constant
// definitions)
#define nth_bit(n)        (n >= BitsPerWord ? 0 : OneBit << (n))
#define right_n_bits(n)   (nth_bit(n) - 1)

// bit-operations using a mask m
inline void     set_bits    (intptr_t& x, intptr_t m) { x |= m; }
inline void     clear_bits  (intptr_t& x, intptr_t m) { x &= ~m; }
inline intptr_t mask_bits   (intptr_t  x, intptr_t m) { return x & m; }

// bit-operations using the n.th bit
inline void set_nth_bit(intptr_t& x, int n) {
  set_bits(x, nth_bit(n));
}
inline void clear_nth_bit(intptr_t& x, int n) {
  clear_bits(x, nth_bit(n));
}
inline bool is_set_nth_bit(intptr_t  x, int n) {
  return mask_bits (x, nth_bit(n)) != NoBits;
}

// returns the bitfield of x starting at start_bit_no with length
// field_length (no sign-extension!)
inline intptr_t bitfield(intptr_t x, int start_bit_no, int field_length) {
  return mask_bits(x >> start_bit_no, right_n_bits(field_length));
}

// returns number of bytes sufficient to hold bitmap of the given length
inline size_t bitmap_size(int length) {
  return ((length - 1) >> LogBitsPerByte) + 1;
}

//----------------------------------------------------------------------------
// Utility functions for integers

// min/max
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

// true if x is a power of 2, false otherwise
inline bool is_power_of_2(intptr_t x) {
  return (x > 0) && mask_bits(x, x - 1) == NoBits;
}

// largest i such that 2^i <= x
//  A negative value of 'x' will return '31'
inline int jvm_log2(intptr_t x) {
  int i = -1;
  uintptr_t p =  1;
  while (p != 0 && p <= (uintptr_t)x) {
    // p = 2^(i+1) && p <= x (i.e., 2^(i+1) <= x)
    i++; p *= 2;
  }
  // p = 2^(i+1) && x < p (i.e., 2^i <= x < 2^(i+1))
  // (if p = 0 then overflow occurred and i = 31)
  return i;
}

// the argument must be exactly a power of 2
inline int exact_log2(intptr_t x) {
#ifdef AZZERT
  if (!is_power_of_2(x)) {
    power_of_2_fatal();
  }
#endif
  return jvm_log2(x);
}

// returns integer round-up to the nearest multiple of s (s must be a power
// of two)
inline intptr_t round_to(intptr_t x, int s) {
#ifdef AZZERT
  if (!is_power_of_2(s)) {
    power_of_2_fatal();
  }
#endif
  const int m = s - 1;
  return mask_bits(x + m, ~m);
}

inline bool is_odd (int x) { return (bool)(x & 1); }
inline bool is_even(int x) { return !is_odd(x); }

// And to get shorts from ints and vice versa

inline jshort extract_low_jshort_from_jint(jint x) {
  return (jshort)(x & 0xffff);
}

inline jshort extract_high_jshort_from_jint(jint x) {
  return (jshort)((x >> 16) & 0xffff);
}

inline jint construct_jint_from_jshorts(jshort high, jshort low) {
  return ((jint)high<<16) | low;
}

inline jushort extract_low_jushort_from_jint(jint x) {
  return (jushort)(x & 0xffff);
}

inline jushort extract_high_jushort_from_jint(jint x) {
  return (jushort)((x >> 16) & 0xffff);
}

inline jint construct_jint_from_jushorts(jushort high, jushort low) {
  return ((jint)high<<16) | low;
}


#define _IGNORE_ME_(x)
#define CURRENT_HAS_PENDING_EXCEPTION (jvm_fast_globals.current_pending_exception)

#ifndef PRODUCT

// Dummy class just used in debug builds to check that functions declared
// to throw exceptions are called as such.

class Traps {
};

#define JVM_IGNORE_TRAPS (void)_traps_ptr

// The JVM_TRAPS macros facilitate the declaration of functions that
// throw exceptions.
// Convention: Use the JVM_TRAPS macro as the last argument of such a function;
// e.g.:
//     int this_function_may_trap(int x, float y JVM_TRAPS)

#define SETUP_ERROR_CHECKER_ARG Traps* _traps_ptr = NULL

extern jint global_check_count;

#define JVM_SINGLE_ARG_TRAPS     Traps* _traps_ptr
#define JVM_TRAPS                , JVM_SINGLE_ARG_TRAPS
#define INC_COUNTER              global_check_count++;

// JVM_ZCHECK: should be used by functions that returns 0 iff an exception
// happens. C++ compilers should generate better code for JVM_ZCHECK than
// for the other JVM_CHECK macros.

#define JVM_SINGLE_ARG_ZCHECK(cond) \
          _traps_ptr); \
        INC_COUNTER; \
        if (!(cond)) { \
          GUARANTEE(CURRENT_HAS_PENDING_EXCEPTION, \
                    "JVM_ZCHECK sanity check"); \
          return; \
        } \
        GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION,  \
                    "JVM_ZCHECK sanity check 2"); \
        _IGNORE_ME_(0

#define JVM_SINGLE_ARG_ZCHECK_0(cond) \
          _traps_ptr); \
        INC_COUNTER; \
        if (!(cond)) { \
          GUARANTEE(CURRENT_HAS_PENDING_EXCEPTION, \
                    "JVM_ZCHECK sanity check"); \
          return 0; \
        } \
        GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION,  \
                    "JVM_ZCHECK sanity check 2"); \
        _IGNORE_ME_(0

#define JVM_ZCHECK(cond)                  , JVM_SINGLE_ARG_ZCHECK(cond)
#define JVM_ZCHECK_0(cond)                , JVM_SINGLE_ARG_ZCHECK_0(cond)
#define JVM_OZCHECK(obj)                  , JVM_SINGLE_ARG_ZCHECK_0(((obj).not_null()))
#define JVM_SINGLE_ARG_OZCHECK(obj)         JVM_SINGLE_ARG_ZCHECK_0(((obj).not_null()))

// Single-argument signature versions of the JVM_CHECK and THROW macros below:
#define JVM_SINGLE_ARG_CHECK \
          _traps_ptr); \
        INC_COUNTER; \
        if (CURRENT_HAS_PENDING_EXCEPTION) { \
          return; \
        } \
        _IGNORE_ME_(0

#define JVM_SINGLE_ARG_CHECK_(result) \
          _traps_ptr); \
        INC_COUNTER; \
        if (CURRENT_HAS_PENDING_EXCEPTION) { \
          return result; \
        } \
        _IGNORE_ME_(0

#define JVM_SINGLE_ARG_THROW \
          _traps_ptr); \
        INC_COUNTER; \
        return; \
        _IGNORE_ME_(0

#define JVM_SINGLE_ARG_THROW_(result) \
          _traps_ptr); \
        INC_COUNTER; \
        return result; \
        _IGNORE_ME_(0

// The JVM_CHECK... macros should be used to pass along a THREAD reference and to
// check for pending exceptions.
//
// Macro naming conventions: Macros that end with _ require a result value to
// be returned. They are for functions with non-void result type. The result
// value is usually ignored because of the exception and is only needed for
// syntactic correctness. The _0 ending is a shortcut for  _(0) since this is
// a frequent case (see further below). Example:
//    int result = this_function_may_trap(x_arg, y_arg JVM_CHECK_0);
//
// CAUTION: make sure that the function call using a JVM_CHECK macro is not the
// only statement of a conditional branch w/o enclosing {} braces, since the
// JVM_CHECK macros expand into several statements!

#define JVM_CHECK                      , JVM_SINGLE_ARG_CHECK
#define JVM_CHECK_(result)             , JVM_SINGLE_ARG_CHECK_(result)

// Unconditional variant to be passed as last argument to Throw::throw_...()
// calls

#define JVM_THROW                     , JVM_SINGLE_ARG_THROW
#define JVM_THROW_(result)            , JVM_SINGLE_ARG_THROW_(result)

// This is a maintenance helper macro. To save on static footprint, it
// is possible to optimize calls to functions with a JVM_TRAPS arg,
// that occur as the logically last statement in a function. In those
// cases, the caller supplies THREAD as the arg, instead of using the
// JVM_CHECK macro, so as to not generate the superfluous code
// sequence
//
//   'if (CURRENT_HAS_PENDING_EXCEPTION) return;'.
//
// To mark the code as something that is likely to be affected by
// future changes, JVM_NO_CHECK_AT_BOTTOM should be used instead of
// THREAD.

#define JVM_NO_CHECK_AT_BOTTOM              , _traps_ptr); _IGNORE_ME_(0
#define JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM   _traps_ptr);   _IGNORE_ME_(0

#define JVM_NO_CHECK_AT_BOTTOM_0            , _traps_ptr); _IGNORE_ME_(0

#define JVM_NO_CHECK                        JVM_NO_CHECK_AT_BOTTOM
#define JVM_SINGLE_ARG_NO_CHECK             JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM

#else

/*
 * PRODUCT mode
 */

#define SETUP_ERROR_CHECKER_ARG

#define JVM_IGNORE_TRAPS
#define JVM_SINGLE_ARG_TRAPS
#define JVM_TRAPS

#define JVM_SINGLE_ARG_ZCHECK(cond) \
          ); \
        if (!(cond)) { \
          return; \
        } \
        _IGNORE_ME_(0

#define JVM_SINGLE_ARG_ZCHECK_0(cond) \
          ); \
        if (!(cond)) { \
          return 0; \
        } \
        _IGNORE_ME_(0

#define JVM_ZCHECK(cond)                  JVM_SINGLE_ARG_ZCHECK(cond)
#define JVM_ZCHECK_0(cond)                JVM_SINGLE_ARG_ZCHECK_0(cond)
#define JVM_OZCHECK(obj)                  JVM_SINGLE_ARG_ZCHECK_0(((obj).not_null()))
#define JVM_SINGLE_ARG_OZCHECK(obj)       JVM_SINGLE_ARG_ZCHECK_0(((obj).not_null()))

// In product builds we minimize the amount of code these macros expand to,
// forgoing syntax checks that are present otherwise (see non-product above)

#define JVM_SINGLE_ARG_CHECK \
          ); \
        if (CURRENT_HAS_PENDING_EXCEPTION) { \
          return; \
        } \
        _IGNORE_ME_(0

#define JVM_SINGLE_ARG_CHECK_(result) \
          ); \
        if (CURRENT_HAS_PENDING_EXCEPTION) { \
          return result;  \
        } \
        _IGNORE_ME_(0

#define JVM_SINGLE_ARG_THROW \
          ); \
        return; \
        _IGNORE_ME_(0

#define JVM_SINGLE_ARG_THROW_(result) \
          ); \
        return result; \
        _IGNORE_ME_(0

#define JVM_CHECK                      JVM_SINGLE_ARG_CHECK
#define JVM_CHECK_(result)             JVM_SINGLE_ARG_CHECK_(result)
#define JVM_THROW                      JVM_SINGLE_ARG_THROW
#define JVM_THROW_(result)             JVM_SINGLE_ARG_THROW_(result)

#define JVM_NO_CHECK_AT_BOTTOM
#define JVM_NO_CHECK_AT_BOTTOM_0
#define JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM

#endif

// Shorthand versions for cases where '0' satisfies the required result type:
#define JVM_SINGLE_ARG_CHECK_0         JVM_SINGLE_ARG_CHECK_(0)
#define JVM_CHECK_0                    JVM_CHECK_(0)
#define JVM_SINGLE_ARG_THROW_0         JVM_SINGLE_ARG_THROW_(0)
#define JVM_THROW_0                    JVM_THROW_(0)

#define JVM_NO_CHECK                   JVM_NO_CHECK_AT_BOTTOM
#define JVM_SINGLE_ARG_NO_CHECK        JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM

// Occasionally, we want to delay a check.  We put JVM_NO_CHECK into the
// statement, and then use a JVM_DELAYED_CHECK further down
//   [We invert the test in order to avoid the "dangling else" problem.]
#define JVM_DELAYED_CHECK \
     if (!CURRENT_HAS_PENDING_EXCEPTION) ; else return
#define JVM_DELAYED_CHECK_(arg) \
     if (!CURRENT_HAS_PENDING_EXCEPTION) ; else return (arg)
#define JVM_DELAYED_CHECK_0 \
     if (!CURRENT_HAS_PENDING_EXCEPTION) ; else return (0)

#define JVM_MUST_SUCCEED \
        JVM_NO_CHECK); \
        GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "must succeed"); \
        _IGNORE_ME_(0

#define JVM_SINGLE_ARG_MUST_SUCCEED \
        JVM_SINGLE_ARG_NO_CHECK); \
        GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "must succeed"); \
        _IGNORE_ME_(0

typedef class OopDesc* ReturnOop;

#define ERROR_MESSAGES_DO(template)\
 template(empty_message, "") \
 template(ve_stack_overflow,       "Stack Overflow") \
 template(ve_stack_underflow,      "Stack Underflow") \
 template(ve_stack_expect_cat1,    "Unexpected Long or Double on stack") \
 template(ve_stack_bad_type,       "Bad type on stack") \
 template(ve_locals_overflow,      "Too many locals") \
 template(ve_locals_bad_type,      "Bad type in local") \
 template(ve_locals_underflow,     "Locals underflow") \
 template(ve_target_bad_type,      "Inconsistent or missing stackmap at" \
                                   " target")\
 template(ve_back_branch_uninit,   "Backwards branch with uninitialized" \
                                   " object")\
 template(ve_seq_bad_type,         "Inconsistent stackmap at next" \
                                   " instruction") \
 template(ve_expect_class,         "Expect constant pool entry of type" \
                                   " class") \
 template(ve_expect_throwable,     "Expect subclass of java.lang.Throwable") \
 template(ve_bad_lookupswitch,     "Items in lookupswitch not sorted") \
 template(ve_bad_ldc,              "Bad constant pool for ldc") \
 template(ve_baload_bad_type,      "baload requires byte[] or boolean[]") \
 template(ve_aaload_bad_type,      "aaload requires subtype of Object[]") \
 template(ve_bastore_bad_type,     "bastore requires byte[] or boolean[]") \
 template(ve_aastore_bad_type,     "Bad array or element type for aastore") \
 template(ve_field_bad_type,       "VE_FIELD_BAD_TYPE") \
 template(ve_expect_methodref,     "Bad constant pool type for invoker") \
 template(ve_args_not_enough,      "Insufficient args on stack for method" \
                                   " call")\
 template(ve_args_bad_type,        "Bad arguments on stack for method call") \
 template(ve_expect_invokespecial, "Bad invocation of initialization method") \
 template(ve_expect_new,           "Bad stackmap reference to uninitialized" \
                                   " object") \
 template(ve_expect_uninit,        "Initializer called on already" \
                                   " initialized" \
                                   " object") \
 template(ve_bad_instr,            "Illegal byte code (possibly an optimized" \
                                   " or fast bytecode)") \
 template(ve_expect_array,         "Arraylength on non-array") \
 template(ve_multianewarray,       "Bad dimension of constant pool for" \
                                   " multianewarray") \
 template(ve_expect_no_retval,     "Value returned from void method") \
 template(ve_retval_bad_type,      "Wrong value returned from method") \
 template(ve_expect_retval,        "Value not returned from method") \
 template(ve_return_uninit_this,   "Initializer not initializing this") \
 template(ve_bad_stackmap,         "Illegal offset for stackmap") \
 template(ve_fall_through,         "Code can fall off the bottom") \
 template(ve_expect_zero,          "Last byte of invokeinterface must" \
                                   " be zero") \
 template(ve_nargs_mismatch,       "Bad nargs field for invokeinterface") \
 template(ve_invokespecial,        "Bad call to invokespecial") \
 template(ve_bad_init_call,        "Bad call to <init> method") \
 template(ve_expect_fieldref,      "Constant pool entry must be a field" \
                                   " reference") \
 template(ve_final_method_override,"Override of final method") \
 template(ve_middle_of_byte_code,  "Code ends in middle of byte code") \
 template(ve_bad_new_offset,       "ITEM_NewObject stackmap type has illegal" \
                                   " offset") \
 template(ve_cp_index_out_of_bounds, \
                                   "Constant pool index is out of bounds") \
 template(truncated_class_file,    "Truncated class file") \
 template(invalid_utf8_string,     "Invalid UTF8 string in .class file") \
 template(invalid_constant_tag,    "invalid constant tag") \
 template(constantpool_overflow,   "Constant pool overflow in .class file") \
 template(unknown_constant_tag,    "Unknown constant tag in .class file") \
 template(bad_type_reference,      "Bad type reference in .class file") \
 template(invalid_method_name,     "Invalid method name in .class file") \
 template(invalid_method_signature,"Invalid method signature in .class file") \
 template(invalid_field_name,      "Invalid field name in .class file") \
 template(invalid_field_signature, "Invalid field signature in .class file") \
 template(invalid_signature,       "Invalid signature in .class file") \
 template(invalid_class_name,      "Invalid class name in .class file") \
 template(invalid_class_version,   "Invalid version in .class file") \
 template(class_implements_array_class, \
                                   "Class must not implement array class") \
 template(invalid_attribute,       "Invalid attribute in .class file") \
 template(invalid_synthetic_attribute, \
                                   "Invalid synthetic attribute in .class" \
                                   " file")\
 template(multiple_inner_class_attribute, \
                                   "Multiple InnerClasses attributes")\
 template(duplicate_field,         "Duplicate field in .class file") \
 template(invalid_field_access_flags, \
                                   "Invalid field access flags in .class" \
                                   " file") \
 template(invalid_field_type,      "Invalid field type in .class file") \
 template(bad_stackmap_size,       "Bad stack map - unexpected size") \
 template(bad_stackmap_type,       "Bad stack map - unexpected stack type") \
 template(bad_constantpool_index,  "Bad constant pool index in class file") \
 template(corrupted_exception_handler, \
                                   "Corrupted exception handler in .class" \
                                   " file")\
 template(invalid_method_access_flags, \
                                   "Invalid method access flags in .class" \
                                   " file") \
 template(superfluous_code_attribute, \
                                   "Superfluos code attribute in .class" \
                                   " file") \
 template(excessive_code_length,   "Excessive code length in .class file") \
 template(duplicate_stackmap_attribute, \
                                   "Duplicate StackMap attribute") \
 template(bad_stackmap_attribute_size, \
                                   "Bad Stackmap attribute size") \
 template(duplicate_exception_table, \
                                   "Duplicate exception table in .class" \
                                   " file") \
 template(corrupted_attribute,     "Corrupted attribute in .class file") \
 template(missing_code_attribute,  "Missing code attribute in .class file") \
 template(too_many_method_parameters, \
                                   "More than 255 method parameters in" \
                                   " .class file") \
 template(invalid_frame_size,      "Invalid frame size in .class file") \
 template(invalid_class_file,      "Invalid .class file") \
 template(class_implements_self,   "Class implements itself") \
 template(class_implements_noninterface, \
                                   "Class implements non-interface") \
 template(circular_interfaces,     "Circular interfaces") \
 template(recursive_class_structure, \
                                   "Recursive class structure") \
 template(incompatible_magic_value,"Incompatible magic value in .class file") \
 template(bad_class_flags,         "Bad class flags in .class file") \
 template(wrong_class_name,        "Wrong class name") \
 template(invalid_superclass,      "Invalid superclass") \
 template(interfaces_must_extend_JLO, \
                                   "Interfaces must have java.lang.Object as" \
                                   " superclass") \
 template(cannot_inherit_from_final_class, \
                                   "Cannot inherit from final class") \
 template(must_not_extend_interface, \
                                   "Must not extend an interface") \
 template(inconsistent_classfile_size, \
                                   "Inconsistent .class file size") \
 template(circular_superclasses,   "Circular super classes") \
 template(invalid_constant,        "java.lang.ClassFormatError: Invalid" \
                                   " constant in .class file") \
 template(constantpool_index_out_of_bounds, \
                                   "Constant pool index out of bounds") \
 template(no_class_def_found_error,"NoClassDefFoundError") \
 template(illegal_access_error,    "IllegalAccessError") \
 template(instantiation_error,     "InstantiationError") \
 template(no_such_field,           "java.lang.NoSuchFieldError") \
 template(field_changed,           "Field changed") \
 template(method_changed,          "Method changed") \
 template(class_changed,           "Class changed") \
 template(overriding_final_method, "Overriding a final method") \
 template(ldiv_err,                "ldiv error") \
 template(lrem_err,                "lrem error") \
 template(idiv_err,                "idiv error") \
 template(illegal_code,            "Illegal code") \
 template(illegal_wide_code,       "Illegal wide code") \
 template(abstract_method_error,   "AbstractMethodError") \
 template(arraycopy_null_pointer,  "Null pointer in array copy") \
 template(arraycopy_incompatible_types, \
                                   "Array types not equal in array copy") \
 template(already_started,         "Already started") \
 template(subtype_check_failed,    "Subtype check failed") \
 template(bad_ref_index_range,     "Bad ref index (out of range)") \
 template(bad_ref_index_allocation,"Bad ref index (unallocated)") \
 template(thread_unlocked_wait,    "Thread does not own lock when calling" \
                                   " wait")\
 template(thread_unlocked_notify,  "Thread does not own lock when calling" \
                                   " notify") \
 template(thread_unlocked_notifyall, \
                                   "Thread does not own lock when calling" \
                                   " notifyAll") \
 template(main_method_not_found,   "main() method not found") \
 template(exit_not_allowed,        "System.exit() not allowed") \
 template(restricted_package,      "May not load classes in restricted" \
                                   " package") \
 template(error_tag_count,         "Error tag count") \
 template(static_offset_exceeded_max_value_of_ushort, \
                                   "Static offset exceeded max value of" \
                                   " ushort") \
 template(must_be_a_power_of_2,    "Must be a power of 2") \
 template(generic_fatal_error,     "Fatal error") \
 template(cannot_represent_imm_32_as_rotated_imm_8, \
                                   "Cannot represent imm_32 as rotated imm_8")\
 template(must_have_temporary_register, \
                                   "Must have temporary register") \
 template(flat_profiler_buffer_overflow, \
                                   "Flat profiler buffer overflow") \
 template(create_event_failed,     "CreateEvent() failed") \
 template(duplicate_handle_failed, "DuplicateHandle() failed") \
 template(system_resource_unavailable, \
                                   "System resource unavailable") \
 template(root_class_loading_failure, \
                                   "Couldn't load root class, please check" \
                                   " classpath") \
 template(bootstrap_heap_too_small,"Heap too small to bootstrap VM") \
 template(java_lang_object_not_found, \
                                   "Couldn't load java.lang.Object, please" \
                                   " check classpath") \
 template(main_class_not_found, \
                                   "Couldn't load specified class during" \
                                   " bootstrapping, please check classpath") \
 template(run_method_not_found,    "run() method not found") \
 template(native_method_error,     "Error in native method") \
 template(jarfile_error,           "Couldn't load JarFile") \
 template(no_such_method,          "java.lang.NoSuchMethodError") \
 template(illegal_access_exception,"java.lang.IllegalAccessException") \
 template(instantiation_exception, "java.lang.InstantiationException") \
 template(java_lang_object_cannot_implement_interfaces, \
                                   "java.lang.Object cannot implement" \
                                   " interfaces") \
 template(internal_class_size_mismatch, \
                                   "Internal class loaded from classfile" \
                                   " has incorrect size") \
 template(internal_class_must_have_fixed_size, \
                                   "Internal class must have fixed size") \
 template(internal_field_must_be_valid, \
                                   "Internal field must be valid") \
 template(internal_field_must_be_non_static, \
                                   "Internal field must be non static") \
 template(internal_field_must_be_static, \
                                   "Internal field must be static") \
 template(internal_field_offset_mismatch, \
                                   "Internal field offset mismatch") \
 template(allocation_cannot_fail_during_bootstrap, \
                                   "Allocation cannot fail during bootstrap") \
 template(class_not_resolved_during_compilation, \
                                   "class cannot be resolved during " \
                                   "compilation (without violating JLS)") \
 template(allocation_failed_while_GC_disabled, \
                                   "allocation failed while GC disabled") \
 template(cannot_nest_isolate_context_switch, \
                                    "Cannot nest isolate context switch")\
 template(too_many_running_isolates, \
                                   "Too many running isolates") \
 template(romized_string_64k_limit_overflow, \
                                   "romized string 64k limit overflow") \
 template(isolate_not_started,     "Isolate not started") \
 template(verification_error,      "Verification error") \
 template(binary_file_error,       "BinaryFileStream output error") \
 template(romizer_not_supported,   "Romizer not supported in this VM build") \
 template(unsatisfied_link_error,  "Failed to load shared library. " \
                                   "Make sure dynamic loading is enabled, " \
                                   "and library exists") \
 template(too_many_romizations,    "One one instance of the Romizer can " \
                                   "be executed at any time") \
 template(romization_requires_fresh_vm, \
                                   "Romization must be done in a fresh VM " \
                                   "(or fresh Task in MVM mode") \
 template(isolate_already_started, "Isolate already started") \
 /* MethodTrap-specific messages */ \
 template(no_more_free_slots,      "No more free slots") \
 template(invalid_method,          "Invalid method name") \
 template(trap_already_set,        "Trap already set") \
 template(invalid_trap_handle,     "Invalid trap handle") \
 template(parameter_types_mismatch,"Parameter types mismatch") \
 template(jvm_class_must_be_hidden,"com.sun.cldchi.jvm must be marked " \
                                   "as a HiddenPackage in your ROM " \
                                   "configuration file")


#if !USE_VERBOSE_ERROR_MSG

#define ERROR_MESSAGE_DECLARE(identifier, string)    identifier,
enum ErrorMsgTag {
  ERROR_MESSAGES_DO(ERROR_MESSAGE_DECLARE)
  number_of_error_messages
};

#else

typedef char const * ErrorMsgTag;
#define ERROR_MESSAGE_DECLARE(identifier, string) \
   const ErrorMsgTag identifier = string;
ERROR_MESSAGES_DO(ERROR_MESSAGE_DECLARE)

#endif

// Redefine INTERP_LOG_SIZE in your Makefile to change the size of
// the interpretation log. (INTERP_LOG_SIZE-1) must be a power of two.
#ifndef INTERP_LOG_SIZE
#define INTERP_LOG_SIZE (8+1)  // Extra element to make the log NULL-terminated
#endif

enum {
  INTERP_LOG_MASK = (INTERP_LOG_SIZE-1) * sizeof(OopDesc*) - 1
};

#define ForInterpretationLog( var ) \
  OopDesc** var = _interpretation_log; for( --var; *++var; )

enum {
  method_execution_sensor_size = 512    // 2048 max, 12-bit signed negative
                                        // offset on ARM
};

enum {
  PROTECTED_PAGE_SIZE         = 4096,    // limited by 12-bit offset on ARM
  INTERPRETER_TIMER_TICK_SLOT = PROTECTED_PAGE_SIZE - 4,
  COMPILER_TIMER_TICK_SLOT    = PROTECTED_PAGE_SIZE - 8
};

extern "C" unsigned char _protected_page[];

enum {
  SYSTEM_TASK = 0,
#if ENABLE_ISOLATES
  LOG_MAX_TASKS = 4,
#else
  LOG_MAX_TASKS = 1,    // system task, and one 'main' task
#endif
  MAX_TASKS = 1 << LOG_MAX_TASKS
};

/*
 * Macro for declaring quick global variables. The number of entries must be
 * a multiple of 4. See Assembler_arm.hpp for details (search for "16").
 */
#define FORALL_JVM_FAST_GLOBALS(template, x)        \
  template(x, OopDesc*,  current_thread)            \
  template(x, OopDesc*,  current_pending_exception) \
  template(x, Oop*,      last_handle)               \
  template(x, OopDesc**, collection_area_start)     \
                                                    \
  template(x, OopDesc**, heap_start)                \
  template(x, OopDesc**, heap_top)                  \
  template(x, OopDesc**, inline_allocation_top)     \
  template(x, OopDesc**, inline_allocation_end)     \
                                                    \
  template(x, OopDesc**, heap_limit)                \
  template(x, OopDesc**, heap_end)                  \
  template(x, address,   bitvector_base)            \
  template(x, int,       bit_selector)              \
                                                    \
  template(x, OopDesc**, young_generation_start)    \
  template(x, OopDesc**, collection_area_end)       \
  template(x, size_t,    young_generation_target_size) \
  template(x, OopDesc**, end_fixed_objects)         \
                                                    \
  template(x, OopDesc**, marking_stack_start)       \
  template(x, OopDesc**, marking_stack_top)         \
  template(x, OopDesc**, marking_stack_end)         \
  template(x, bool,      marking_stack_overflow)    \
                                                    \
  template(x, OopDesc***,slices_start)              \
  template(x, size_t,    slice_shift)               \
  template(x, size_t,    slice_offset_bits)         \
  template(x, size_t,    slice_size)                \
                                                    \
  template(x, size_t,    near_mask)                 \
  template(x, size_t,    slice_offset_mask)         \
  template(x, size_t,    nof_slices)                \
  template(x, OopDesc**, compaction_start)          \
                                                    \
  template(x, OopDesc**, compaction_top)            \
  template(x, int,       heap_min)                  \
  template(x, int,       heap_capacity)             \
  template(x, size_t,    heap_size)                 \
                                                    \
  template(x, OopDesc**, compiler_area_start)       \
  template(x, OopDesc**, compiler_area_top)         \
  template(x, OopDesc**, large_object_area_bottom)  \
  template(x, OopDesc**, compiler_area_temp_top)    \
  template(x, OopDesc**, compiler_area_temp_bottom) \
                                                           \
  /* frequently used values by Compiler*/                  \
  template(x, Method*,            compiler_method)         \
  template(x, CodeGenerator*,     compiler_code_generator) \
  template(x, int,                compiler_bci)            \
                                                           \
  template(x, BytecodeCompileClosure*,  compiler_closure)  \
  template(x, int,                num_stack_lock_words)    \
  template(x, address,   class_list_base)           \
  template(x, address,   mirror_list_base)


#define DECLARE_FAST_GLOBAL_FIELD(DUMMY, type, name) type name;

struct JVMFastGlobals {
  FORALL_JVM_FAST_GLOBALS(DECLARE_FAST_GLOBAL_FIELD, DUMMY)
};

#define _current_thread               jvm_fast_globals.current_thread
#define _current_pending_exception    jvm_fast_globals.current_pending_exception
#define _last_handle                  jvm_fast_globals.last_handle
#define _bitvector_base               jvm_fast_globals.bitvector_base
#define _bit_selector                 jvm_fast_globals.bit_selector

#define _heap_start                   jvm_fast_globals.heap_start
#define _heap_top                     jvm_fast_globals.heap_top
#define _inline_allocation_top        jvm_fast_globals.inline_allocation_top
#define _inline_allocation_end        jvm_fast_globals.inline_allocation_end

#define _heap_limit                   jvm_fast_globals.heap_limit
#define _heap_end                     jvm_fast_globals.heap_end
#define _collection_area_start        jvm_fast_globals.collection_area_start
#define _class_list_base              jvm_fast_globals.class_list_base
#define _mirror_list_base             jvm_fast_globals.mirror_list_base

#define _young_generation_start       jvm_fast_globals.young_generation_start
#define _collection_area_end          jvm_fast_globals.collection_area_end
#define _young_generation_target_size jvm_fast_globals.young_generation_target_size
#define _end_fixed_objects            jvm_fast_globals.end_fixed_objects

#define _marking_stack_start          jvm_fast_globals.marking_stack_start
#define _marking_stack_top            jvm_fast_globals.marking_stack_top
#define _marking_stack_end            jvm_fast_globals.marking_stack_end
#define _marking_stack_overflow       jvm_fast_globals.marking_stack_overflow

#define _slices_start                 jvm_fast_globals.slices_start
#define _slice_shift                  jvm_fast_globals.slice_shift
#define _slice_offset_bits            jvm_fast_globals.slice_offset_bits
#define _slice_size                   jvm_fast_globals.slice_size

#define _near_mask                    jvm_fast_globals.near_mask
#define _slice_offset_mask            jvm_fast_globals.slice_offset_mask
#define _nof_slices                   jvm_fast_globals.nof_slices
#define _compaction_start             jvm_fast_globals.compaction_start

#define _compaction_top               jvm_fast_globals.compaction_top
#define _heap_min                     jvm_fast_globals.heap_min
#define _heap_capacity                jvm_fast_globals.heap_capacity
#define _heap_size                    jvm_fast_globals.heap_size

#define _compiler_area_start          jvm_fast_globals.compiler_area_start
#define _compiler_area_top            jvm_fast_globals.compiler_area_top
#define _large_object_area_bottom     jvm_fast_globals.large_object_area_bottom
#define _compiler_area_temp_top       jvm_fast_globals.compiler_area_temp_top
#define _compiler_area_temp_bottom    jvm_fast_globals.compiler_area_temp_bottom

#define _compiler_method              jvm_fast_globals.compiler_method
#define _compiler_code_generator      jvm_fast_globals.compiler_code_generator
#define _compiler_bci                 jvm_fast_globals.compiler_bci

#define _compiler_closure             jvm_fast_globals.compiler_closure
#define _num_stack_lock_words         jvm_fast_globals.num_stack_lock_words

/* Variables used by the interpreter and compiler */
extern "C" {
  extern JVMFastGlobals jvm_fast_globals;

  extern OopDesc**_persistent_handles_addr; // used by ARM ports only.
  extern OopDesc* _interpretation_log[];
  extern int      _interpretation_log_idx;

  extern unsigned char _method_execution_sensor[];

  extern OopDesc* persistent_handles[];
  extern int      _jvm_in_raw_pointers_block;
  extern int      _jvm_in_quick_native_method;
  extern char*    _jvm_quick_native_exception;

#if ENABLE_PROFILER
  extern int      _jvm_profiler_in_native_method;
#endif

  extern OopDesc* _interned_string_near_addr;

#if ROMIZED_PRODUCT
  extern const int system_symbols[];
#else
  extern       int system_symbols[];
#endif

  // Scheduler.cpp
  extern OopDesc* _next_runnable_thread;

  // Thread.cpp
#if ENABLE_ISOLATES
  extern OopDesc*  _current_task;
  extern OopDesc* _task_class_init_marker;
#endif
  extern address  _current_stack_limit;
  extern address  _compiler_stack_limit;
  extern int      _rt_timer_ticks;
  extern address  _primordial_sp;
#if ENABLE_CLDC_11
  extern bool     _is_pending_interrupt;
#endif

  extern int*     _rom_constant_pool_fast;  // ARM only
  extern _KNI_HandleInfo*
                  last_kni_handle_info;
  extern struct _JVM_PerformanceCounters
                  jvm_perf_count;

  extern Oop*     last_raw_handle;
  extern OopDesc**_old_generation_end;

#ifdef AZZERT
  extern jint AllocationDisabler__disabling_count;
#endif

  // Generated oopmaps. See Generator::generate_oopmaps.
  extern const jubyte oopmap_Empty[];

  extern const unsigned char omit_frame_table[];

  // Asm loop
  void shared_call_vm();
  void shared_call_vm_oop();
  void shared_call_vm_exception();
  void switch_thread(Thread* thread);
  void start_lightweight_thread_asm();
  void force_terminated(Thread* thread);
  address setup_stack_asm(address sp);

#if ENABLE_COMPILER
  void compiler_rethrow_exception();
  void compiler_new_object();
  void compiler_new_obj_array();
  void compiler_new_type_array();
  void compiler_idiv_irem();
#if USE_COMPILER_GLUE_CODE
  void compiler_glue_code_start();
  void compiler_glue_code_end();
#endif
#if ENABLE_COMPRESSED_VSF
  void compiler_callvm_stubs_start();
  void compiler_callvm_stubs_end();
#endif
#endif

#if ENABLE_ARM_VFP
  void vfp_redo();
  void vfp_fcmp_redo();
  void vfp_double_redo();
  void vfp_dcmp_redo();
#endif

  void shared_monitor_enter();
  void shared_monitor_exit();
  void shared_lock_synchronized_method();
  void shared_unlock_synchronized_method();
  void shared_code_for_handling_of_exception_forwarding();

  // a method with a fixed entry is impossible to compile
  void fixed_interpreter_method_entry();
  void fixed_interpreter_fast_method_entry_0();
  void fixed_interpreter_fast_method_entry_1();
  void fixed_interpreter_fast_method_entry_2();
  void fixed_interpreter_fast_method_entry_3();
  void fixed_interpreter_fast_method_entry_4();

  void interpreter_method_entry();
  void interpreter_fast_method_entry_0();
  void interpreter_fast_method_entry_1();
  void interpreter_fast_method_entry_2();
  void interpreter_fast_method_entry_3();
  void interpreter_fast_method_entry_4();

  void quick_void_native_method_entry();
  void quick_int_native_method_entry();
  void quick_obj_native_method_entry();

  void interpreter_grow_stack();

  void shared_invoke_compiler();

  void shared_fast_getbyte_accessor();
  void shared_fast_getshort_accessor();
  void shared_fast_getchar_accessor();
  void shared_fast_getint_accessor();
  void shared_fast_getlong_accessor();

  void shared_fast_getfloat_accessor();
  void shared_fast_getdouble_accessor();
  void shared_fast_getfloat_static_accessor();
  void shared_fast_getdouble_static_accessor();

  void shared_fast_getbyte_static_accessor();
  void shared_fast_getshort_static_accessor();
  void shared_fast_getchar_static_accessor();
  void shared_fast_getint_static_accessor();
  void shared_fast_getlong_static_accessor();

  void invoke_pending_entries(Thread* thread);
  void primordial_to_current_thread();
  void current_thread_to_primordial();
  void call_on_primordial_stack(void (*)(void));
#if ENABLE_CPU_VARIANT
  void initialize_cpu_variant();
  void enable_cpu_variant();
  void disable_cpu_variant();
#endif

#if ENABLE_REFLECTION || ENABLE_JAVA_DEBUGGER
  void entry_return_void();
  void entry_return_word();
  void entry_return_long();
  void entry_return_float();
  void entry_return_double();
  void entry_return_object();
#endif

#if ENABLE_JAVA_DEBUGGER
  void shared_call_vm_oop_return();
#endif

  // InterpreterRuntime.cpp

  OopDesc * multianewarray(JVM_SINGLE_ARG_TRAPS);
  OopDesc * newobject(JVM_SINGLE_ARG_TRAPS);
  OopDesc* _newobject(Thread* thread, OopDesc* raw_klass JVM_TRAPS);

  jlong jvm_lmul(jlong a, jlong b);
  jlong jvm_ldiv(jlong a, jlong b);
  jlong jvm_lrem(jlong a, jlong b);
  jlong jvm_lshl(jlong a, jint b);
  jlong jvm_lshr(jlong a, jint b);
  jlong jvm_lushr(jlong a, jint b);

#if ENABLE_THUMB_COMPILER
  jlong jvm_ladd(jlong op1, jlong op2);
  jlong jvm_lsub(jlong op1, jlong op2);
  jlong jvm_land(jlong op1, jlong op2);
  jlong jvm_lor(jlong op1, jlong op2);
  jlong jvm_lxor(jlong op1, jlong op2);
  jint  jvm_lcmp(jlong op1, jlong op2);
  jlong jvm_lmin(jlong op1, jlong op2);
  jlong jvm_lmax(jlong op1, jlong op2);
#endif

#if ENABLE_COMPILER
  void uncommon_trap();
  void deoptimize();
#endif

  void internal_stack_tag_exception();
  void verify_stack();
  void lock_stack_lock(Thread *thread, StackLock* stack_lock JVM_TRAPS);
  void unlock_stack_lock(Thread *thread, StackLock* stack_lock JVM_TRAPS);
  void stack_overflow(Thread *thread, address stack_pointer);
  void timer_tick();
  void signal_waiters(Thread *thread, StackLock*);

  void array_store_type_check(JVM_SINGLE_ARG_TRAPS);
  void checkcast(JVM_SINGLE_ARG_TRAPS);
  jint instanceof(JVM_SINGLE_ARG_TRAPS);

  void division_by_zero_exception(JVM_SINGLE_ARG_TRAPS);
  void illegal_monitor_state_exception(JVM_SINGLE_ARG_TRAPS);
  void array_index_out_of_bounds_exception(JVM_SINGLE_ARG_TRAPS);
  void null_pointer_exception(JVM_SINGLE_ARG_TRAPS);
  void incompatible_class_change_error(JVM_SINGLE_ARG_TRAPS);
  void arithmetic_exception(JVM_SINGLE_ARG_TRAPS);

  ReturnOop get_illegal_monitor_state_exception(JVM_SINGLE_ARG_TRAPS);
  ReturnOop get_array_index_out_of_bounds_exception(JVM_SINGLE_ARG_TRAPS);
  ReturnOop get_null_pointer_exception(JVM_SINGLE_ARG_TRAPS);
  ReturnOop get_division_by_zero_exception(JVM_SINGLE_ARG_TRAPS);
  ReturnOop get_incompatible_class_change_error(JVM_SINGLE_ARG_TRAPS);

  // Natives.cpp
#if ENABLE_DYNAMIC_NATIVE_METHODS

#if ENABLE_FLOAT
#define FOR_FLOAT_TYPES_ARG(template, arg) \
   template(jfloat,  T_FLOAT,  arg)            \
   template(jdouble, T_DOUBLE, arg)    
#else
#define FOR_FLOAT_TYPES_ARG(template, arg)
#endif

#define FOR_NON_VOID_TYPES_ARG(template, arg) \
   template(jboolean, T_BOOLEAN, arg)         \
   template(jbyte,    T_BYTE,    arg)         \
   template(jchar,    T_CHAR,    arg)         \
   template(jshort,   T_SHORT,   arg)         \
   template(jint,     T_INT,     arg)         \
   template(jlong,    T_LONG,    arg)         \
   template(jobject,  T_OBJECT,  arg)         \
   template(jarray,   T_ARRAY,   arg)         \
   FOR_FLOAT_TYPES_ARG(template, arg)

#define FOR_NON_VOID_TYPES(template) FOR_NON_VOID_TYPES_ARG(template, 0)

#define FOR_ALL_TYPES_ARG(template, arg)          \
   template(void,     T_VOID,    arg)             \
   FOR_NON_VOID_TYPES_ARG(template, arg)

#define FOR_ALL_TYPES(template) FOR_ALL_TYPES_ARG(template, 0)

#define DECLARE_JAVA_UNIMPLEMENTED(jtype, ttype, arg) \
  jtype Java_ ## jtype ## _unimplemented(JVM_SINGLE_ARG_TRAPS);

  FOR_NON_VOID_TYPES(DECLARE_JAVA_UNIMPLEMENTED)

#endif

  void Java_void_unimplemented(JVM_SINGLE_ARG_TRAPS);

  void Java_abstract_method_execution(JVM_SINGLE_ARG_TRAPS);
  void Java_illegal_method_execution(JVM_SINGLE_ARG_TRAPS);
  void Java_incompatible_method_execution(JVM_SINGLE_ARG_TRAPS);

  // Debugger code
  void handle_single_step(Thread *thread);
  void handle_compiled_code_caught_exception(Thread *thread, jint catch_offset);
  void handle_exception_info(Thread *thread);
#if ENABLE_JAVA_DEBUGGER
#define SAVE_CURRENT_EXCEPTION     \
        Oop __dbg_current_exc__ = Thread::current_pending_exception(); \
        Thread::clear_current_pending_exception();

#define RESTORE_CURRENT_EXCEPTION  \
        Thread::set_current_pending_exception(&__dbg_current_exc__);

  extern int _debugger_active;
#else
#define _debugger_active 0
#endif
  void shared_invoke_debug();
  address get_rom_debug_method(Thread *, OopDesc *);
  // Misc

  void initialize_class(Thread* thread, OopDesc* raw_klass JVM_TRAPS);
  ReturnOop task_barrier(Thread *thread, OopDesc *java_class JVM_TRAPS);
  ReturnOop compiled_code_task_barrier(Thread *thread, OopDesc *java_class
                                       JVM_TRAPS);
  void real_time_tick(int delay_time);
  void trace_bytecode();

  void jprof_record_thread_switch();
  void jprof_record_method_transition();

  bool get_UseVerifier();

  // The following are used by ENABLE_KVM_COMPAT
  void garbageCollect(int moreMemory);
  void kvmcompat_initialize();
  void kvmcompat_oops_do(void do_oop(OopDesc**));
} // extern "C"

//----------------------------------------------------------------------
// UNICODE-capable filename support. This typedef is also included in
// jvm.h for external interfaces, hence the #ifndef JVM_PATHCHAR_DEFINED
//
// If your system requires unicode for JvmPathChar, you need to include
// #define USE_UNICODE_FOR_FILENAMES 1 in os/<port>/Globals_<port>.hpp

#ifndef JVM_PATHCHAR_DEFINED
#define JVM_PATHCHAR_DEFINED 1

// PCSL requires Unicode filenames
#if ENABLE_PCSL

#if defined(USE_UNICODE_FOR_FILENAMES)
#if !USE_UNICODE_FOR_FILENAMES
#error "USE_UNICODE_FOR_FILENAMES must be set if ENABLE_PCSL is set"
#endif
#else
#define USE_UNICODE_FOR_FILENAMES 1
#endif

#else

#ifndef USE_UNICODE_FOR_FILENAMES
#define USE_UNICODE_FOR_FILENAMES 0
#endif

#endif /* ENABLE_PCSL */

#if USE_UNICODE_FOR_FILENAMES
typedef jchar JvmPathChar;
#else
typedef char JvmPathChar;
#endif

#endif // JVM_PATHCHAR_DEFINED

typedef JvmPathChar PathChar; // REMOVE!

enum {
NAME_BUFFER_SIZE = 270
};

//----------------------------------------------------------------------
// Pure-virtual function declaration

// If you declare a pure virtual function, some compilers (e.g., gcc 2.9)
// will link in a large static library (for exception handling, etc). So
// instead of declaring your pure virtual function as:
//
//    void foo() = 0;
//    int bar()  = 0;
//
// use these macros:
//
//    void foo() JVM_PURE_VIRTUAL;
//    int bar()  JVM_PURE_VIRTUAL_0;

#define JVM_PURE_VIRTUAL \
  { \
    SHOULD_NOT_REACH_HERE(); \
  }
#define JVM_PURE_VIRTUAL_WITH_TRAPS \
  { \
    JVM_IGNORE_TRAPS; \
    SHOULD_NOT_REACH_HERE(); \
  }
#define JVM_PURE_VIRTUAL_1_PARAM(param) \
  {                                     \
    (void)param;                        \
    SHOULD_NOT_REACH_HERE();            \
  }
#define JVM_PURE_VIRTUAL_2_PARAM(param1, param2) \
  {                                     \
    (void)param1;                       \
    (void)param2;                       \
    SHOULD_NOT_REACH_HERE();            \
  }
#define JVM_PURE_VIRTUAL_3_PARAM(param1, param2, param3) \
  {                                     \
    (void)param1;                       \
    (void)param2;                       \
    (void)param3;                       \
    SHOULD_NOT_REACH_HERE();            \
  }
#define JVM_PURE_VIRTUAL_0   {SHOULD_NOT_REACH_HERE(); return 0;}
#define JVM_PURE_VIRTUAL_1_PARAM_0(param) \
  {                                       \
    (void)param;                          \
    SHOULD_NOT_REACH_HERE();              \
    return 0;                             \
  }
#define JVM_PURE_VIRTUAL_(x) {SHOULD_NOT_REACH_HERE(); return (x);}

// Native function argument access macros

#define PARAMETER_ADDRESS(index) \
  (_kni_parameter_base + index * JavaStackDirection * BytesPerStackElement)

#define GET_PARAMETER_AS_OOP(index) \
  (*( (ReturnOop*) (PARAMETER_ADDRESS(index)) ))

#if ENABLE_FLOAT

static inline jint float_bits(jfloat f) {
  return *(jint*)&f;
}

static inline jlong double_bits(jdouble d) {
  jdouble_accessor acc;
  acc.double_value = d;
  if (MSW_FIRST_FOR_DOUBLE == MSW_FIRST_FOR_LONG) {
    return jlong_from_low_high(acc.words[0], acc.words[1]);
  } else {
    return jlong_from_low_high(acc.words[1], acc.words[0]);
  }
}

static inline jfloat float_from_bits(jint bits) {
  return *(jfloat*)&bits;
}

static inline jdouble double_from_bits(jlong bits) {
  jlong_accessor acc;
  acc.long_value = bits;
  if (MSW_FIRST_FOR_DOUBLE == MSW_FIRST_FOR_LONG) {
    return jdouble_from_low_high(acc.words[0], acc.words[1]);
  } else {
    return jdouble_from_low_high(acc.words[1], acc.words[0]);
  }
}

#else
// those are here only to support FP ops used by VM itself
static inline jfloat  jvm_i2f(jint x)                { return (float)x; }
       inline jint    jvm_f2i(jfloat x)              { return (jint)x; }
static inline jfloat  jvm_fmul(jfloat x, jfloat y)   { return x * y; }
static inline jfloat  jvm_fdiv(jfloat x, jfloat y)   { return x / y; }
static inline jdouble jvm_dmul(jdouble x, jdouble y) { return x * y; }
static inline jdouble jvm_ddiv(jdouble x, jdouble y) { return x / y; }
static inline jdouble jvm_f2d(jfloat x)              { return (jdouble)x; }
static inline jfloat  jvm_d2f(jdouble x)             { return (jfloat)x; }
static inline jint    jvm_d2i(jdouble x)             { return (jint)x; }
static inline jfloat  jvm_l2f(jlong x)               { return (jfloat)x; }
static inline jlong   jvm_f2l(jfloat x)              { return (jlong)x; }
static inline jdouble jvm_i2d(jint x)                { return (jdouble)x; }
static inline jdouble jvm_l2d(jlong x)               { return (jdouble)x; }
static inline jint    jvm_fcmpg(jfloat x, jfloat y)  { return ((x > y)  ? 1 :
                                                               (x == y) ? 0 :
                                                               (x < y)  ? -1:
                                                                          1);
                                                     }
#endif

#if ENABLE_THUMB_LIBC_GLUE

// Do this only if you're running on a linux distribution with
// a GLIBC that's not built with interworking support.

#if ENABLE_INTERPRETER_GENERATOR && !defined(GENERATE_LIBC_GLUE)
#define GENERATE_LIBC_GLUE 1
#endif
#if defined(ARM) && !CROSS_GENERATOR
#define USE_LIBC_GLUE 1
#endif

#endif

#ifdef USE_LIBC_GLUE

// Do this only if you're running on a linux distribution with
// a GLIBC that's not built with interworking support.
//
// As side effect all our external dependencies are listed here

// those will be defined in interpreter loop as interworking aware stubs
// to actual GLIBC calls
extern "C" {
extern void* jvm_memset(void *s, int c, int n);
extern void* jvm_memmove(void *dest, const void *src, int n);
extern char* jvm_strcpy(char *dest, const char *src);
extern char* jvm_strncpy(char *dest, const char *src, int n);
extern int   jvm_strcmp(const char *s1, const char *s2);
extern int   jvm_strncmp(const char *s1, const char *s2,int n);
extern int   jvm_strlen(const char *s);
extern char* jvm_strchr(const char *s, int c);
extern char* jvm_strrchr(const char *s, int c);
extern char* jvm_strcat(char *dest, const char *src);

extern int   jvm_sscanf(const char *str, const char *format, ...);
extern int   jvm_printf(const char *format, ...);
extern int   jvm_sprintf(char *str, const char *format, ...);
extern int   jvm_vsprintf(char *str, const char *format, va_list ap);
extern int   jvm_vsnprintf(char *str, int size, const char *format, va_list ap);
extern int   jvm_gettimeofday(void *tv, void *tz);
extern void  jvm_exit(int status);

extern void* jvm_fopen(const char *path, const char *mode);
extern int   jvm_fprintf(void *stream, const char *format, ...);
extern int   jvm_fread(void *ptr, int size, int nmemb, void *stream);
extern int   jvm_fwrite(const void *ptr, int size, int  nmemb,  void* stream);
extern int   jvm_fseek(void *stream, long offset, int whence);
extern long  jvm_ftell(void *stream);
extern char* jvm_fgets(char *s, int size, void *stream);
extern int   jvm_fflush(void *stream);
extern int   jvm_fclose(void *stream);
extern int   jvm_feof(void *stream);
extern int   jvm_ferror(void *stream);

extern int   jvm_socket(int domain, int type, int protocol);
extern int   jvm_shutdown(int s, int how);
extern void* jvm_gethostbyname(const char *name);
extern int   jvm_connect(int  sockfd, const void* serv_addr, int addrlen);
extern unsigned short  jvm_htons(unsigned short hostshort);
extern int   jvm_recv(int s, void *buf, int len, int flags);
extern int   jvm_send(int s, const void *msg, int len, int flags);
extern int   jvm_select(int n, void *readfds, void *writefds, void *exceptfds,
                        void *timeout);

extern int   jvm_stat(const char *file_name, void *buf);
extern int   jvm_remove(const char *pathname);
extern int   jvm_open(const char *pathname, int flags);
extern int   jvm_close(int fd);
extern int   jvm_lseek(int fd, unsigned int /* ??? */, int);
extern int   jvm_fcntl(int fd, int cmd, ...);
extern int   jvm_rename(const char *oldpath, const char *newpath);

extern void* jvm_malloc(int size);
extern void  jvm_free(void *ptr);
extern char* jvm_getenv(const char *name);

extern int   jvm_setitimer(int which, void* value, void* ovalue);
extern void* jvm_signal(int signum, void (*)(int));
extern int   jvm_sigaction(int signum, const void* act, void* oldact);
extern int   jvm_sigaltstack(const void*, void*);
extern void  jvm_usleep(unsigned long usec);
extern int   jvm_nanosleep(const void *req, void *rem);
extern void  jvm_abort(void);
extern void* jvm_mmap(void *start, int length, int prot , int flags, int fd, int offset);
extern int   jvm_munmap(void *start, int length);
extern long  jvm_sysconf(int name);
extern int   jvm_atoi(const char *nptr);

#define jvm_qsort jvm_slow_sort

// IMPL_NOTE: on Linux, qsort will call the compar function (usually
// compiled as THUMB) while the CPU is in ARM mode.
//
extern void  jvm_slow_sort(void *base, int nmemb, int size,
                             int(*compar)(const void *, const void *));

}

#else

#define jvm_memset      memset
#define jvm_memmove     memmove
#define jvm_strcmp      strcmp
#define jvm_strncmp     strncmp
#define jvm_strcpy      strcpy
#define jvm_strncpy     strncpy
#define jvm_strlen      strlen
#define jvm_strchr      strchr
#define jvm_strrchr     strrchr
#define jvm_strcat      strcat

#define jvm_printf      printf
#define jvm_sprintf     sprintf
#define jvm_vsprintf    vsprintf
#define jvm_vsnprintf   vsnprintf
#define jvm_sscanf      sscanf

#define jvm_fopen       fopen
#define jvm_fread       fread
#define jvm_fwrite      fwrite
#define jvm_fprintf     fprintf
#define jvm_ftell       ftell
#define jvm_fseek       fseek
#define jvm_fflush      fflush
#define jvm_fclose      fclose
#define jvm_fgets       fgets
#define jvm_feof        feof
#define jvm_ferror      ferror

#define jvm_rename      rename
#define jvm_stat        stat
#define jvm_remove      remove
#define jvm_signal      signal
#define jvm_sigaction   sigaction
#define jvm_setitimer   setitimer
#define jvm_sigaltstack sigaltstack
#define jvm_abort       abort

#define jvm_getenv      getenv
#define jvm_exit        exit
#define jvm_gettimeofday gettimeofday
#define jvm_malloc      malloc
#define jvm_free        free
#define jvm_qsort       qsort
#define jvm_signal      signal
#define jvm_usleep      usleep
#define jvm_nanosleep   nanosleep

#define jvm_socket      socket
#define jvm_gethostbyname gethostbyname
#define jvm_connect     connect
#define jvm_htons       htons
#define jvm_fcntl       fcntl
#define jvm_open        open
#define jvm_close       close
#define jvm_lseek       lseek
#define jvm_shutdown    shutdown
#define jvm_recv        recv
#define jvm_send        send
#define jvm_select      select
#define jvm_mmap        mmap
#define jvm_munmap      munmap
#define jvm_sysconf     sysconf
#define jvm_atoi        atoi

#endif

// fast memcpy and memcmp are implemented on ARM only (but not Thumb-2)
#if !defined(ARM) || ENABLE_ARM_V6T2
#undef ENABLE_FAST_MEM_ROUTINES
#define ENABLE_FAST_MEM_ROUTINES 0
#endif

#if ENABLE_FAST_MEM_ROUTINES || defined(USE_LIBC_GLUE)
// Use the routines inside Interpreter_arm.s
extern "C" void* jvm_memcpy(void *dest, const void *src, int n);
extern "C" int   jvm_memcmp(const void *s1, const void *s2, int n);
#else
// Use the routines in the C library.
#define jvm_memcpy      memcpy
#define jvm_memcmp      memcmp
#endif

#if defined(AZZERT) && !ENABLE_ZERO_YOUNG_GENERATION
#define DIRTY_HEAP(start, length)  jvm_memset(start, 0xAB, length)
#else
#define DIRTY_HEAP(start, length)
#endif

#if ARM_EXECUTABLE && ENABLE_ZERO_YOUNG_GENERATION && !ENABLE_ARM_V7
extern "C" void fast_memclear(void* start, int length);
#else
inline void fast_memclear(void* start, int length) {
  jvm_memset(start, 0, length);
}
#endif

extern "C" size_t fn_strlen(const JvmPathChar* str);
extern "C" void   fn_strcat(JvmPathChar* s1, const JvmPathChar *s2);

//-------------------------------------------------------------------
// Debugging support for compiled class initialization barriers
#if ENABLE_ISOLATES
enum CIBType {
  cib_on_getstatic = 1,
  cib_on_putstatic,
  cib_on_invokestatic,
  cib_on_new,
  num_cib_types
};
#endif

//---------------------------------------------------------------------
// OopMap generation and validation
typedef void (*oopmaps_doer)(BasicType type, void *param,
                             const char *name, size_t offset, int flags);

typedef void (*oop_doer)(OopDesc**);

#define OOPMAP_VARIABLE_OBJ 0x10000000
#define OOPMAP_UNSIGNED     0x20000000

#define OOPMAP_ENTRY_4(doer, param, type, name) \
  doer(type, param, #name, name ## _offset(), 0)

#define OOPMAP_ENTRY_5(doer, param, type, name, flags) \
  doer(type, param, #name, name ## _offset(), flags)

#define OOPMAP_CLASSES_DO_CORE(template) \
  template(ConstantPool) \
  template(Method) \
  template(InstanceClass) \
  template(ArrayClass) \
  template(ObjNear) \
  template(FarClass) \
  template(EntryActivation) \
  template(ClassInfo) \
  template(StackmapList) \
  template(CompiledMethod)

// There used to be some compiler classes.  There aren't any more, but we
// leave this around just in case we get more in the future.
#if USE_COMPILER_STRUCTURES
#define OOPMAP_CLASSES_DO_COMPILER(template)
#else
#define OOPMAP_CLASSES_DO_COMPILER(template)
#endif

#if ENABLE_ISOLATES
#define OOPMAP_CLASSES_DO_ISOLATES(template) \
  template(Boundary) \
  template(TaskMirror)
#else
#define OOPMAP_CLASSES_DO_ISOLATES(template)
#endif

#define OOPMAP_CLASSES_DO(template) \
  OOPMAP_CLASSES_DO_CORE(template) \
  OOPMAP_CLASSES_DO_COMPILER(template) \
  OOPMAP_CLASSES_DO_ISOLATES(template)

#define OOPMAP_DECLARE(x) \
  extern const jubyte oopmap_ ## x [];

extern "C" {
  OOPMAP_CLASSES_DO(OOPMAP_DECLARE)
}

/*
 * ARM stack frame definitions: these are also used by Frame_arm.hpp
 * and Frame_c.hpp to support AOT compilation.
 */

inline int arm_JavaFrame__arg_offset_from_sp(int index) {
  if (JavaStackDirection < 0) {
    // The stack pointer points at the beginning of the last argument
    return BytesPerStackElement * index;
  } else {
    // The stack pointer points at the end of the last argument
    return (-BytesPerStackElement * (index + 1)) + BytesPerWord;
  }
}

inline int arm_JavaFrame__return_address_offset()       { return -20; }
inline int arm_JavaFrame__bcp_store_offset()            { return -16; }
inline int arm_JavaFrame__locals_pointer_offset()       { return -12; }
inline int arm_JavaFrame__cpool_offset()                { return - 8; }
inline int arm_JavaFrame__method_offset()               { return - 4; }
inline int arm_JavaFrame__caller_fp_offset()            { return   0; }
inline int arm_JavaFrame__stack_bottom_pointer_offset() { return   4; }

// These two slots are not used in this architecture.
inline int arm_JavaFrame__stored_int_value1_offset()    {return  -99999;}
inline int arm_JavaFrame__stored_int_value2_offset()    {return  -99999;}

inline int arm_JavaFrame__frame_desc_size() {
    return 7*BytesPerWord;
}

inline int arm_JavaFrame__end_of_locals_offset() {
  return JavaStackDirection < 0 ? 8 : -24;
}

inline int arm_JavaFrame__empty_stack_offset() {
    return JavaStackDirection < 0 ? -20 : 4;
}

inline int arm_EntryFrame__pending_activation_offset()   { return  -28; }
inline int arm_EntryFrame__pending_exception_offset()    { return  -24; }
inline int arm_EntryFrame__fake_return_address_offset()  { return  -20; }
inline int arm_EntryFrame__stored_int_value2_offset()    { return  -16; }
inline int arm_EntryFrame__stored_int_value1_offset()    { return  -12; }
inline int arm_EntryFrame__stored_last_sp_offset()       { return  - 8; }
inline int arm_EntryFrame__stored_last_fp_offset()       { return  - 4; }
inline int arm_EntryFrame__real_return_address_offset()  { return    0; }
inline int arm_EntryFrame__stored_obj_value_offset()     { return    4; }

inline int arm_EntryFrame__frame_desc_size()     { return 9 * BytesPerWord;}

// When an EntryFrame is empty, the sp points at the word above
inline int arm_EntryFrame__empty_stack_offset()          {
  return JavaStackDirection < 0 ? -28 : +4;
}

#if ENABLE_EMBEDDED_CALLINFO
// number of bytes between the return address and the start of the callinfo
inline int arm_JavaFrame__callinfo_offset_from_return_address() { return -4;}
#endif

// number of bytes between a stack tag value and its tag
inline int arm_StackValue__stack_tag_offset() { return 4; }

/*
 * Internationalization: Redefine these macros if you want to print
 * user-visible messages in a different language
 */

#ifndef MSG_UNCAUGHT_EXCEPTIONS
#define MSG_UNCAUGHT_EXCEPTIONS "Uncaught exception: "
#endif

#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
typedef class SegmentedSourceROMWriter CurrentSourceROMWriter;
#else
typedef class SourceROMWriter CurrentSourceROMWriter;
#endif

#if ENABLE_PERFORMANCE_COUNTERS
#define PERFORMANCE_COUNTER_INCREMENT(name, value) \
        jvm_perf_count.name += value
#define PERFORMANCE_COUNTER_SET_MAX(name, value) \
        if (jvm_perf_count.name < value) { \
          jvm_perf_count.name = value; \
        }
#else
#define PERFORMANCE_COUNTER_INCREMENT(name, value)
#define PERFORMANCE_COUNTER_SET_MAX(name, value)
#endif
