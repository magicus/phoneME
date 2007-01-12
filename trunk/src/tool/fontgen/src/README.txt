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

	MIDP Font Generation Tool

Files:
rdfont.c -- converts font bitmap from the C sources to a human-editable MIDP-FONTDEF format file
wrfont.c -- converts a MIDP-FONTDEF file to C source defining the corresponding font bitmap
gxj_font_bitmap.c -- taken from the MIDP sources
gxj_intern_font_bitmap.h -- taken from the MIDP sources

Necesary pre-requisites:
1. MIDP sources
2. gcc (I used gcc version 4.0.4 20060507 (prerelease) (Debian 4.0.3-3))
3. a text editor using a monospace font
4. probably, a calculator program that can convert decimal to hexadecimal and
   back (both Windovvs and Linux have such calculators, but you will probably need
   to switch it to a mode where dec/hex buttons are shown).


Compilation:
gcc rdfont.c -o rdfont
gcc wrfont.c -o wrfont

Running:

./rdfont >myfont.midpfontdef
./wrfont <myfont.midpfontdef >my_font_bitmap_generated.c

MIDPFONTDEF Format
==================

General Notes
-------------

Trailing whitespace ignored: All characters with the 
codes ASCII 0 (nul) .. ASCII 32 (space) at the end of the line
are ignored.

No tabulation: please avoid the use of the TAB symbol (ASCII 9)

Comments:
#text
Lines that contain only whitespace (including empty ones) are also comments.
Comment lines are just ignored.

Font parameters definition:

@ nn nn nn nn nn

where nn stand for decimal numbers specifying correspondingly: width height ascent descent leading

The space after @ is not obligatory. The number of spaces before nn's is not important, but must be at least 1.

Character range definition:

% xx xx xx

where xx stand for hexadecimal encoding of, correspondingly:
the high byte, the first character's low byte, the last character's low byte.
The first and the last characters in the range MUST have the same high byte.
For example,
% 00 80 ff
specifies the range \u0080 - \u00ff.

Character definition:

: xx
<pixel data lines, number of them = height>

where xx stands for a HEXADECIMAL number representing the character's code.

The number of spaces between : and xx may be zero or more.

Comment lines MAY be used before, after, or among pixel data lines.

Pixel data lines MUST be of width+1 characters. The last character must be '.' (dot).
Other characters MUST be either spaces ' ' or asterisks '*'.
Spaces denote pixels of background color, asterisks denote pixels that form the symbol.


Notes about the Code
====================

Soon after creation of this tool, the binary font representation has changed in MIDP.
The changes are:
1) three additional bytes in the header specifying the character range;
2) multiple font bitmaps covering different character ranges are used.

To support both the new and the old format, the V1_1 macro was defined 
in wrfont.c.
When V1_1 is defined as 1, wrfont outputs the new format (with 8-byte header),
and when V1_1 is defined as 0, wrfont outputs the old format (with 5-byte 
header).
But as to rdfont.c, it works only with the old format (with 5-byte header).

The macro TEST controls insertion of line breaks into the generated source.
With line breaks, the generated code is human-viewable; without line breaks, 
it may be compared to the original code that didn't have any line breaks.


