USE_DEBUG=true

ifeq ($(USE_DEBUG), true)
DEBUG_SUFFIX=_g
else
DEBUG_SUFFIX=
endif

SRC_DIR=javacall
DRIVE=d:

VC_PDB = vc70.pdb

###############################################################################

AK ?= false

ifeq ($(AK), true)

COMPONENTS_DIR=/home/ak154633/bengal/src
JAVACALL_OUTPUT_DIR=$(COMPONENTS_DIR)/phoneme-output/javacall
PCSL_LIB_DIR=$(COMPONENTS_DIR)/svn_phoneme/pcsl/output/javacall_i386/lib
CLDC_LIB=$(DRIVE)$(COMPONENTS_DIR)/svn_phoneme/cldc/bin/javacall_i386_vc/dist/lib/cldc_vm$(DEBUG_SUFFIX).lib
MIDP_OUTPUT_DIR=$(COMPONENTS_DIR)/svn_phoneme_branches/midp-cr-6712543-namsui/build/javacall/output

else

COMPONENTS_DIR=/home/az154780/irbis
JAVACALL_OUTPUT_DIR=$(COMPONENTS_DIR)/javacall/output_nams
PCSL_LIB_DIR=$(COMPONENTS_DIR)/pcsl/trunk/output/javacall_i386/lib
CLDC_LIB=$(DRIVE)$(COMPONENTS_DIR)/cldc/trunk/bin/javacall_i386_vc/dist/lib/cldc_vm$(DEBUG_SUFFIX).lib
MIDP_OUTPUT_DIR=$(COMPONENTS_DIR)/midp/branches/midp-cr-6712543/build/javacall/output

endif

###############################################################################

PCSL_LIB=$(DRIVE)$(PCSL_LIB_DIR)/libpcsl_file.lib \
         $(DRIVE)$(PCSL_LIB_DIR)/libpcsl_memory.lib \
         $(DRIVE)$(PCSL_LIB_DIR)/libpcsl_network.lib \
         $(DRIVE)$(PCSL_LIB_DIR)/libpcsl_print.lib \
         $(DRIVE)$(PCSL_LIB_DIR)/libpcsl_string.lib 

JAVACALL_LIB=$(DRIVE)$(JAVACALL_OUTPUT_DIR)/lib/javacall$(DEBUG_SUFFIX).lib

MIDP_OBJ_DIR=$(MIDP_OUTPUT_DIR)/obj$(DEBUG_SUFFIX)/i386
MIDP_LIB=$(DRIVE)$(MIDP_OBJ_DIR)/libobj.lib

RC  = rc
CC  = cl
CPP = cl
LD  = link
LD_OUTPUT = -out:
CC_OUTPUT = -Fo

OBJ = .obj
EXE = .exe

CFLAGS = /c /D_UNICODE /DUNICODE

ifeq ($(USE_DEBUG), true)

DEBUG_CFLAGS 	+= -Zi -Od -MDd
LD_FLAGS 	+= -debug 
# Force use of debug libraries
LD_FLAGS	+= -nodefaultlib:msvcrt.lib msvcrtd.lib

else # USE_DEBUG

# flags inherited from the VM : -Ox -Os -Gy -GF
EXTRA_CFLAGS    += -nologo -DWIN32 -D_WINDOWS 
EXTRA_CFLAGS 	+= -MD
LD_FLAGS 	+= -merge:CODESEGMENT=.text -merge:DATASEGMENT=.data \
		   -opt:REF -opt:ICF,8
# Force use of non-debug libraries
LD_FLAGS 	+= -nodefaultlib:msvcrtd.lib msvcrt.lib

endif # USE_DEBUG

EXTRA_CFLAGS += -I$(DRIVE)$(JAVACALL_OUTPUT_DIR)/inc

LIBS	    += user32.lib gdi32.lib kernel32.lib winmm.lib wsock32.lib vfw32.lib shell32.lib comdlg32.lib comctl32.lib
LIBS        += $(JAVACALL_LIB) $(PCSL_LIB) $(CLDC_LIB) $(MIDP_LIB)

###############################################################################

A = @

all: appManager

appManager: $(MIDP_OBJ_DIR)/appManager$(OBJ) $(MIDP_OBJ_DIR)/javacallImpl$(OBJ) $(MIDP_OBJ_DIR)/namsui_resource.res
	$(A)$(LD) $(LD_FLAGS) $(DRIVE)$(MIDP_OBJ_DIR)/appManager$(OBJ) $(DRIVE)$(MIDP_OBJ_DIR)/javacallImpl$(OBJ) $(DRIVE)$(MIDP_OBJ_DIR)/namsui_resource.res $(LIBS) $(LD_OUTPUT)$(DRIVE)$(MIDP_OUTPUT_DIR)/bin/i386/appManager$(EXE)
	$(A)cp $(VC_PDB) $(DRIVE)$(MIDP_OUTPUT_DIR)/bin/i386
#	cp $(MIDP_OUTPUT_DIR)/bin/i386/jwc_properties.ini .

$(MIDP_OBJ_DIR)/appManager$(OBJ): $(SRC_DIR)/appManager.cpp
	$(A)$(CPP) $(CFLAGS) $(DEBUG_CFLAGS) $(EXTRA_CFLAGS) $(SRC_DIR)/appManager.cpp $(CC_OUTPUT)$@

$(MIDP_OBJ_DIR)/javacallImpl$(OBJ): $(SRC_DIR)/javacallImpl.cpp
	$(A)$(CPP) $(CFLAGS) $(DEBUG_CFLAGS) $(EXTRA_CFLAGS) $(SRC_DIR)/javacallImpl.cpp $(CC_OUTPUT)$@

$(MIDP_OBJ_DIR)/%.res: $(SRC_DIR)/res/%.rc
	$(A)$(RC) $(RCFLAGS)  /d "NDEBUG" /fo $@ $<
