#
# @(#)defs_personal_motif.mk	1.6 06/10/10
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

#
# Setup default AWT_LIB_LIBS. Don't override anything that the device 
# defs_personal.mk setup.
#
AWT_LIB_LIBS ?= -lXm -lXmu -lXext -lX11 -lm

#
# These are exported as system properties by defs_basis.mk
#
TOOLKIT_CLASS = sun.awt.motif.MToolkit

#
# Get the CPP includes and needed for the motif peers
#
PROFILE_INCLUDE_DIRS += $(CVM_SHAREROOT)/basis/native/image

CLASSLIB_CLASSES += \
	sun.awt.motif.InputThread \
        sun.awt.motif.MButtonPeer \
        sun.awt.motif.MComponentPeer \
	sun.awt.motif.MCanvasPeer \
        sun.awt.motif.MCheckboxMenuItemPeer \
        sun.awt.motif.MCheckboxPeer \
        sun.awt.motif.MChoicePeer \
        sun.awt.motif.MComponentPeer \
	sun.awt.motif.MDialogPeer \
	sun.awt.motif.MDrawingSurfaceInfo \
        sun.awt.motif.MEmbeddedFrame \
        sun.awt.motif.MEmbeddedFramePeer \
        sun.awt.motif.MFontPeer \
        sun.awt.motif.MFramePeer \
        sun.awt.motif.MFileDialogPeer \
        sun.awt.motif.MLabelPeer \
        sun.awt.motif.MListPeer \
        sun.awt.motif.MMenuBarPeer \
        sun.awt.motif.MMenuItemPeer \
        sun.awt.motif.MMenuPeer \
        sun.awt.motif.MPanelPeer \
        sun.awt.motif.MPopupMenuPeer \
        sun.awt.motif.MScrollPanePeer \
        sun.awt.motif.MScrollbarPeer \
        sun.awt.motif.MTextPeer \
        sun.awt.motif.MTextAreaPeer \
        sun.awt.motif.MTextFieldPeer \
        sun.awt.motif.MToolkit \
        sun.awt.motif.MTinyChoicePeer \
        sun.awt.motif.MWindowPeer \
        sun.awt.motif.PSGraphics \
        sun.awt.motif.PSPaperSize \
        sun.awt.motif.PSPrintControl \
        sun.awt.motif.PSPrintJob \
        sun.awt.motif.PSPrintStream \
        sun.awt.motif.PrintControl \
        sun.awt.motif.PrintStatusDialog \
        sun.awt.motif.UPrintDialog \
        sun.awt.motif.X11FontMetrics \
        sun.awt.motif.X11Graphics \
        sun.awt.motif.X11Image \
        sun.awt.motif.X11InputMethod \
        sun.awt.motif.X11OffScreenImage \
        sun.awt.motif.X11Selection \
        sun.awt.motif.X11SelectionHolder \
        sun.awt.motif.X11Clipboard \
        sun.awt.motif.CharToByteX11CNS11643P1 \
        sun.awt.motif.CharToByteX11CNS11643P2 \
        sun.awt.motif.CharToByteX11CNS11643P3 \
        sun.awt.motif.CharToByteX11Dingbats \
        sun.awt.motif.CharToByteX11GB2312 \
        sun.awt.motif.CharToByteX11JIS0201 \
        sun.awt.motif.CharToByteX11JIS0208 \
        sun.awt.motif.CharToByteX11KSC5601 \
        sun.awt.image.ByteArrayImageSource \
        sun.awt.image.FileImageSource \
        sun.awt.image.GifImageDecoder \
        sun.awt.image.Image \
        sun.awt.image.ImageConsumerQueue \
        sun.awt.image.ImageDecoder \
        sun.awt.image.ImageFetchable \
        sun.awt.image.ImageFetcher \
        sun.awt.image.ImageFormatException \
        sun.awt.image.ImageProductionMonitor \
        sun.awt.image.ImageRepresentation \
        sun.awt.image.ImageWatched \
        sun.awt.image.InputStreamImageSource \
        sun.awt.image.JPEGImageDecoder \
        sun.awt.image.OffScreenImageSource \
        sun.awt.image.PixelStore \
        sun.awt.image.PixelStore32 \
        sun.awt.image.PixelStore8 \
        sun.awt.image.PNGImageDecoder \
        sun.awt.image.URLImageSource \

AWT_LIB_OBJS += \
	awt.o \
	awt_Button.o \
	awt_Canvas.o \
	awt_Checkbox.o \
	awt_Choice.o \
	awt_Component.o \
	awt_Dialog.o \
	awt_DrawingSurface.o \
	awt_FileDialog.o \
	awt_Font.o \
	awt_Frame.o \
	awt_Graphics.o \
	awt_InputMethod.o \
	awt_Label.o \
	awt_List.o \
	awt_Menu.o \
	awt_MenuBar.o \
	awt_MenuItem.o \
	awt_MToolkit.o \
	awt_Panel.o \
	awt_PopupMenu.o \
	awt_ScrollPane.o \
	awt_Scrollbar.o \
	awt_Selection.o \
	awt_Text.o \
	awt_TextArea.o \
	awt_TextField.o \
	awt_Window.o \
	awt_util.o \
	canvas.o \
	color.o \
	image.o \
	multi_font.o \
	img_util.o \
	img_globals.o \
	img_colors.o \
	img_cvdirdefault.o \
	img_cvfsdefault.o \
	img_cvorddefault.o \
	img_cvdir16DcmOpqScl.o \
	img_cvdir16DcmOpqUns.o \
	img_cvdir16DcmTrnUns.o \
	img_cvdir16IcmOpqScl.o \
	img_cvdir16IcmOpqUns.o \
	img_cvdir16IcmTrnUns.o \
	img_cvdir32DcmOpqScl.o \
	img_cvdir32DcmOpqUns.o \
	img_cvdir32DcmTrnUns.o \
	img_cvdir32IcmOpqScl.o \
	img_cvdir32IcmOpqUns.o \
	img_cvdir32IcmTrnUns.o \
	img_cvdirdefault.o \
	img_cvfscDcmOpqUns.o \
	img_cvfscIcmOpqUns.o \
	img_cvfsdefault.o \
	img_cvorcDcmOpqUns.o \
	img_cvorcIcmOpqUns.o \
	img_cvorddefault.o \

