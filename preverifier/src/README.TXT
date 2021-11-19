Copyright  2007  Sun Microsystems, Inc. All rights reserved.

PREVERIFIER TOOL

This directory contains the source code of the 
preverifier tool that is used for preprocessing
Java class files for the KVM.  The preprocessed
class files are regular Java class files, but 
they contain some additional/modified information
that will help speed up runtime verification.

The preverifier implementation is derived from
the J2SE class file verifier that was written
originally for the "Classic" Java Virtual Machine.
Therefore, the implementation style is quite 
different from the rest of the KVM codebase.

