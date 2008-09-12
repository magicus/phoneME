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

JAVACALL_LIB=$(DRIVE)$(JAVACALL_OUTPUT_DIR)/lib/javacall_nams_ui$(DEBUG_SUFFIX).lib

MIDP_OBJ_DIR=$(MIDP_OUTPUT_DIR)/obj$(DEBUG_SUFFIX)/i386
MIDP_EXE_DIR=$(MIDP_OUTPUT_DIR)/bin/i386
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

A ?= @

all: appManager background splashscreen

appManager: $(MIDP_OBJ_DIR)/appManager$(OBJ) $(MIDP_OBJ_DIR)/javacallImpl$(OBJ) $(MIDP_OBJ_DIR)/appManagerUtils$(OBJ) $(MIDP_OBJ_DIR)/appManagerProgress$(OBJ) $(MIDP_OBJ_DIR)/appManagerPermissions$(OBJ) $(MIDP_OBJ_DIR)/appManagerLCDUI$(OBJ) $(MIDP_OBJ_DIR)/appManager_resource.res
	$(A)$(LD) $(LD_FLAGS) $(DRIVE)$(MIDP_OBJ_DIR)/appManager$(OBJ) $(DRIVE)$(MIDP_OBJ_DIR)/javacallImpl$(OBJ) $(DRIVE)$(MIDP_OBJ_DIR)/appManagerUtils$(OBJ) $(DRIVE)$(MIDP_OBJ_DIR)/appManagerProgress$(OBJ) $(DRIVE)$(MIDP_OBJ_DIR)/appManagerPermissions$(OBJ) $(DRIVE)$(MIDP_OBJ_DIR)/appManagerLCDUI$(OBJ) $(DRIVE)$(MIDP_OBJ_DIR)/appManager_resource.res $(LIBS) $(LD_OUTPUT)$(DRIVE)$(MIDP_EXE_DIR)/appManager$(EXE)
	$(A)cp $(VC_PDB) $(DRIVE)$(MIDP_EXE_DIR)

$(MIDP_OBJ_DIR)/%$(OBJ): $(SRC_DIR)/%.cpp $(SRC_DIR)/*.h
	$(A)$(CPP) $(CFLAGS) $(DEBUG_CFLAGS) $(EXTRA_CFLAGS) $< $(CC_OUTPUT)$@

$(MIDP_OBJ_DIR)/%.res: $(SRC_DIR)/res/%.rc $(SRC_DIR)/res/toolbar.bmp $(SRC_DIR)/res/*.ico
	$(A)$(RC) $(RCFLAGS)  /d "NDEBUG" /fo $@ $<

background: $(MIDP_EXE_DIR)/background.bmp

splashscreen: $(MIDP_EXE_DIR)/splash_screen_240x320.bmp 

$(MIDP_EXE_DIR)/%.bmp: $(SRC_DIR)/res/%.bmp
	$(A)cp $< $(DRIVE)$(MIDP_EXE_DIR)
