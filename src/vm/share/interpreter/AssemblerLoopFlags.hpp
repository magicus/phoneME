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

extern "C" {
    extern jint assembler_loop_type;
}

class AssemblerLoopFlags: public AllStatic {
public:
  static jint get_flags() {
    jint result = has_Interpreter;
    if (::GenerateDebugAssembly)   { result += has_GenerateDebugAssembler; }
    if (::TraceBytecodes)          { result += has_TraceBytecodes; }
    if (::Deterministic)           { result += has_Deterministic; }
    if (::PrintBytecodeHistogram)  { result += has_PrintBytecodeHistogram; }
    if (::PrintPairHistogram)      { result += has_PrintPairHistogram; }
#if ENABLE_FLOAT
    result += has_FloatingPoint;
#endif
    if (JavaStackDirection > 0)    { result += has_AscendingJavaStack; }
    if (TaggedJavaStack)           { result += has_TaggedJavaStack; }
    return result;
  }

  static bool GeneratedInterpreterLoop() {
     return (assembler_loop_type != 0);
  }

  static bool DebugAssembler() {
    return (assembler_loop_type & has_GenerateDebugAssembler) != 0;
  }

  static bool TraceBytecodes() { 
    return (assembler_loop_type & has_TraceBytecodes) != 0;
  }

  static bool Deterministic() { 
    return (assembler_loop_type & has_Deterministic) != 0;
  }

  static bool PrintBytecodeHistogram() { 
    return (assembler_loop_type & has_PrintBytecodeHistogram) != 0;    
  }

  static bool PrintPairHistogram() { 
    return (assembler_loop_type & has_PrintPairHistogram) != 0;    
  }

  static bool AscendingJavaStack() {
    return (assembler_loop_type & has_AscendingJavaStack) != 0;    
  }

  static bool UsingedTaggedJavaStack() {
    return (assembler_loop_type & has_TaggedJavaStack) != 0;    
  }

private:
  enum {
    has_Interpreter =               1,
    has_GenerateDebugAssembler =    2,
    has_TraceBytecodes =            4,
    has_Deterministic =             8,
    has_PrintBytecodeHistogram = 0x10, 
    has_PrintPairHistogram     = 0x20,
    has_FloatingPoint          = 0x40,
    has_AscendingJavaStack     = 0x80,
    has_TaggedJavaStack        = 0x100
  };

};
