#
# Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 only,
# as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# version 2 for more details (a copy is included at /legal/license.txt).
# 
# You should have received a copy of the GNU General Public License version
# 2 along with this work; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
# 
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
# CA 95054 or visit www.sun.com if you need additional information or have
# any questions.
#

# Use the command-line option APPMANAGER_ALL_APPS to compile copyrighted
# and non-copyrighted apps.  The default is to only compile copyrighted.
# Ex. APPMANAGER_ALL_APPS=true

# Name of resulting jar file
AwtPDA_PRESENTATION_MODE_JARFILE = AwtPDA_PresentationMode.jar

PRESENTATION_MODE_JARFILES += lib/$(AwtPDA_PRESENTATION_MODE_JARFILE)
APPMANAGER_CLASSES         += $(AwtPDA_PRESENTATION_MODE_IMPL_CLASSES)
APPS                       += $(AwtPDA_APPS)
PROVISIONED_APPS           += $(AwtPDA_APPS)
APP_IMAGEDIRS              += $(AwtPDA_APP_IMAGEDIRS)
APPMANAGER_BUILDDIRS       += $(AwtPDA_PRESENTATION_MODE_RESOURCES_DIRS)

AwtPDA_PRESENTATION_MODE_IMPL_CLASSES += \
	com.sun.appmanager.impl.presentation.AwtPDA.AwtPDAPresentationDownloader \
	com.sun.appmanager.impl.presentation.AwtPDA.AwtPDAPresentationMode \
	com.sun.appmanager.impl.presentation.AwtPDA.AwtPDAPresentationPrefs \
	com.sun.appmanager.impl.presentation.AwtPDA.AwtPDAPresentationTaskbar \
	com.sun.appmanager.impl.presentation.AwtPDA.AwtPDAPresentationSplash \
	com.sun.appmanager.impl.presentation.AwtPDA.AwtPDAPresentationLogin \
	com.sun.appmanager.impl.presentation.AwtPDA.AwtPDAPresentationMoveRemove \
	com.sun.appmanager.impl.presentation.AwtPDA.Border \
	com.sun.appmanager.impl.presentation.AwtPDA.ImageButton \
	com.sun.appmanager.impl.presentation.AwtPDA.ImagePanel \
	com.sun.appmanager.impl.presentation.AwtPDA.ImageScrollPane \
	com.sun.appmanager.impl.presentation.AwtPDA.TransparentPanel \
	com.sun.appmanager.impl.presentation.AwtPDA.SlotLayout \
	com.sun.appmanager.impl.presentation.AwtPDA.Gauge \
	com.sun.appmanager.impl.presentation.AwtPDA.InfoDialog

AwtPDA_PRESENTATION_MODE_RESOURCES_DIRS += \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/icons \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/background \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/login \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/downloader \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/progressbar \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/moveremove \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/preferences \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/gauge

AwtPDA_COPYRIGHT_APPS += \
	phone \
	ticker \
	Clock \
	WireFrame \
	DrawTest \
	TicTacToe \
	Animator \
	ArcTest \
	DitherTest \
	TextApps

AwtPDA_COPYRIGHT_APPS_IMAGEDIRS += \
	Animator \
	TicTacToe

AwtPDA_NON_COPYRIGHT_APPS += \
	allcomponents \
	CaffeineMark \
	javanoid \
	kaos \
	stoneroids \
	superfoulegg \
	zcalc \
	chess \
	hanoi


AwtPDA_NON_COPYRIGHT_APPS_IMAGEDIRS += \
	superfoulegg

# Add new apps
ifeq ($(APPMANAGER_ALL_APPS), true)
AwtPDA_APPS += \
	$(AwtPDA_COPYRIGHT_APPS) \
	$(AwtPDA_NON_COPYRIGHT_APPS)

AwtPDA_APP_IMAGEDIRS +=  \
	$(AwtPDA_COPYRIGHT_APPS_IMAGEDIRS) \
	$(AwtPDA_NON_COPYRIGHT_APPS_IMAGEDIRS)
else
AwtPDA_APPS += \
	$(AwtPDA_COPYRIGHT_APPS)

AwtPDA_APP_IMAGEDIRS +=  \
	$(AwtPDA_COPYRIGHT_APPS_IMAGEDIRS)
endif

################################ APPS ################################

##### allcomponents
allcomponents_classes = \
	AllComponents \
	ColumnLayout \
	Editor \
	InfoDialog \
	ModalYesNo \
	MultiLineLabel \
	YesNo \
	YesNoDialog
allcomponents_class_files = \
        $(patsubst %,$(allcomponents_classesdir)/%.class,$(allcomponents_classes))
allcomponents_classesdir = $(APPMANAGER_APPDIR)/allcomponents/classes
allcomponents_srcdir = $(WS_APPMANAGER_APPS_DIR)/allcomponents
allcomponents_icondir = $(allcomponents_srcdir)/icons
AwtPDA_APP_ICONS += allcomponents.png
AwtPDA_APP_DESCRIPTORS += AllComponents.app

##### javanoid
javanoid_classes = \
	javanoid \
	jnball \
	jnblock \
	jnbuffer \
	jnbullet \
	jngame \
	jnmovingobject \
	jnobject \
	jnpaddle \
	jnpill \
	jnstatus
javanoid_class_files = \
        $(patsubst %,$(javanoid_classesdir)/%.class,$(javanoid_classes))
javanoid_classesdir = $(APPMANAGER_APPDIR)/javanoid/classes
javanoid_srcdir = $(WS_APPMANAGER_APPS_DIR)/javanoid
javanoid_icondir = $(javanoid_srcdir)/icons
AwtPDA_APP_ICONS += javanoid.png
AwtPDA_APP_DESCRIPTORS += Javanoid.app

##### kaos
kaos_classes = \
	net/ultrametrics/kaos/Point2D \
	net/ultrametrics/kaos/BoundingBox \
	net/ultrametrics/kaos/Scaling \
	net/ultrametrics/kaos/RegionAligner \
	net/ultrametrics/kaos/ZoomTypes \
	net/ultrametrics/kaos/AbstractCalculator \
	net/ultrametrics/kaos/MandelbrotCalculator \
	net/ultrametrics/kaos/WeirdMandelbrotCalculator \
	net/ultrametrics/kaos/PixelPeer \
	net/ultrametrics/kaos/PixelGenerator \
	net/ultrametrics/kaos/RandomPixelGenerator \
	net/ultrametrics/kaos/SmoothShadedPixelGenerator \
	net/ultrametrics/kaos/MandelbrotPixelGenerator \
	net/ultrametrics/kaos/PopupFrame \
	net/ultrametrics/kaos/PropertyHelper \
	net/ultrametrics/kaos/Settings \
	net/ultrametrics/kaos/FractalCanvasMouseListener \
	net/ultrametrics/kaos/FractalCanvas \
	net/ultrametrics/kaos/KaosMenu \
	net/ultrametrics/kaos/Kaos
kaos_class_files = \
        $(patsubst %,$(kaos_classesdir)/%.class,$(kaos_classes))
kaos_classesdir = $(APPMANAGER_APPDIR)/kaos/classes
kaos_srcdir = $(WS_APPMANAGER_APPS_DIR)/kaos
kaos_icondir = $(kaos_srcdir)/icons
AwtPDA_APP_ICONS += kaos.png
AwtPDA_APP_DESCRIPTORS += Kaos.app

##### stoneroids
stoneroids_classes = \
	GameActor \
	Stoneroids \
	stAlienShip \
	stAsteroid \
	stLaser \
	stPowerup \
	stShip
stoneroids_class_files = \
        $(patsubst %,$(stoneroids_classesdir)/%.class,$(stoneroids_classes))
stoneroids_classesdir = $(APPMANAGER_APPDIR)/stoneroids/classes
stoneroids_srcdir = $(WS_APPMANAGER_APPS_DIR)/stoneroids
stoneroids_icondir = $(stoneroids_srcdir)/icons
AwtPDA_APP_ICONS += stoneroids.png
AwtPDA_APP_DESCRIPTORS += Stoneroids.app

##### superfoulegg
superfoulegg_classes = \
	Egg \
	FallingEggs \
	FixedEggs \
	GameThread \
	IO \
	InputInterface \
	JarResources \
	Options \
	Player \
	PlayerInter \
	PlayersEggs \
	Stage \
	SuperFE
superfoulegg_class_files = \
        $(patsubst %,$(superfoulegg_classesdir)/%.class,$(superfoulegg_classes))
superfoulegg_classesdir = $(APPMANAGER_APPDIR)/superfoulegg/classes
superfoulegg_srcdir = $(WS_APPMANAGER_APPS_DIR)/superfoulegg
superfoulegg_icondir = $(superfoulegg_srcdir)/icons
AwtPDA_APP_ICONS += superfoulegg.png
AwtPDA_APP_DESCRIPTORS += SuperFE.app

##### ticker
ticker_classes = \
	Ticker
ticker_class_files = \
        $(patsubst %,$(ticker_classesdir)/%.class,$(ticker_classes))
ticker_classesdir = $(APPMANAGER_APPDIR)/ticker/classes
ticker_srcdir = $(WS_APPMANAGER_APPS_DIR)/ticker
ticker_icondir = $(ticker_srcdir)/icons
AwtPDA_APP_ICONS += ticker.png
AwtPDA_APP_DESCRIPTORS += MyTicker.app

##### zcalc
zcalc_classes = \
	CalcFrame \
	Calculator \
	MessageBox \
	Zcalc
zcalc_class_files = \
        $(patsubst %,$(zcalc_classesdir)/%.class,$(zcalc_classes))
zcalc_classesdir = $(APPMANAGER_APPDIR)/zcalc/classes
zcalc_srcdir = $(WS_APPMANAGER_APPS_DIR)/zcalc
zcalc_icondir = $(zcalc_srcdir)/icons
AwtPDA_APP_ICONS += zcalc.png
AwtPDA_APP_DESCRIPTORS += Zcalc.app

##### chess
chess_classes = \
	Board \
	BoardDisplay \
	CastleMove \
	ChessEvaluator \
	ChessGenerator \
	EPMove \
	Game \
	Generator \
	HashTable \
	Move \
	MsgBox \
	MultiLineLabel \
	PV \
	PromoteMove \
	RandomX \
	SBE \
	SimpleMove \
	Statistics

chess_class_files = \
        $(patsubst %,$(chess_classesdir)/%.class,$(chess_classes))
chess_classesdir = $(APPMANAGER_APPDIR)/chess/classes
chess_srcdir = $(WS_APPMANAGER_APPS_DIR)/chess
chess_icondir = $(chess_srcdir)/icons
AwtPDA_APP_ICONS += chess.png
AwtPDA_APP_DESCRIPTORS += Chess.app

##### hanoi
hanoi_classes = \
	hanoi \
	hanoicanvas \
	hncontrol \
	movingring \
	ring
hanoi_class_files = \
        $(patsubst %,$(hanoi_classesdir)/%.class,$(hanoi_classes))
hanoi_classesdir = $(APPMANAGER_APPDIR)/hanoi/classes
hanoi_srcdir = $(WS_APPMANAGER_APPS_DIR)/hanoi
hanoi_icondir = $(hanoi_srcdir)/icons
AwtPDA_APP_ICONS += hanoi.png
AwtPDA_APP_DESCRIPTORS += Hanoi.app

##### phone
phone_classes = \
	PhoneService \
        PhoneServiceImpl \
	ImagePanel \
	KeyLocationHash \
	Phone \
	PhoneImage
phone_class_files = \
        $(patsubst %,$(phone_classesdir)/%.class,$(phone_classes))
phone_classesdir = $(APPMANAGER_APPDIR)/phone/classes
phone_srcdir = $(WS_APPMANAGER_APPS_DIR)/phone
phone_icondir = $(phone_srcdir)/icons
AwtPDA_APP_ICONS += phone.png
AwtPDA_APP_DESCRIPTORS += Phone.app

##### Clock
Clock_classes = \
	PhoneService \
	Clock

Clock_class_files = \
        $(patsubst %,$(Clock_classesdir)/%.class,$(Clock_classes))
Clock_classesdir = $(APPMANAGER_APPDIR)/Clock/classes
Clock_srcdir = $(WS_APPMANAGER_APPS_DIR)/Clock
Clock_icondir = $(Clock_srcdir)/icons
AwtPDA_APP_ICONS += Clock.png
AwtPDA_APP_DESCRIPTORS += Clock.app

##### WireFrame
WireFrame_classes = \
	ThreeD \
	Matrix3D

WireFrame_class_files = \
        $(patsubst %,$(WireFrame_classesdir)/%.class,$(WireFrame_classes))
WireFrame_classesdir = $(APPMANAGER_APPDIR)/WireFrame/classes
WireFrame_srcdir = $(WS_APPMANAGER_APPS_DIR)/WireFrame
WireFrame_icondir = $(WireFrame_srcdir)/icons
AwtPDA_APP_ICONS += WireFrame.png
AwtPDA_APP_DESCRIPTORS += WireFrame.app

##### DrawTest
DrawTest_classes = \
	DrawTest

DrawTest_class_files = \
        $(patsubst %,$(DrawTest_classesdir)/%.class,$(DrawTest_classes))
DrawTest_classesdir = $(APPMANAGER_APPDIR)/DrawTest/classes
DrawTest_srcdir = $(WS_APPMANAGER_APPS_DIR)/DrawTest
DrawTest_icondir = $(DrawTest_srcdir)/icons
AwtPDA_APP_ICONS += DrawTest.png
AwtPDA_APP_DESCRIPTORS += DrawTest.app

##### TicTacToe
TicTacToe_classes = \
	TicTacToe

TicTacToe_class_files = \
        $(patsubst %,$(TicTacToe_classesdir)/%.class,$(TicTacToe_classes))
TicTacToe_classesdir = $(APPMANAGER_APPDIR)/TicTacToe/classes
TicTacToe_srcdir = $(WS_APPMANAGER_APPS_DIR)/TicTacToe
TicTacToe_icondir = $(TicTacToe_srcdir)/icons
AwtPDA_APP_ICONS += TicTacToe.png
AwtPDA_APP_DESCRIPTORS += TicTacToe.app

##### Animator
Animator_classes = \
	Animator
Animator_class_files = \
        $(patsubst %,$(Animator_classesdir)/%.class,$(Animator_classes))
Animator_classesdir = $(APPMANAGER_APPDIR)/Animator/classes
Animator_srcdir = $(WS_APPMANAGER_APPS_DIR)/Animator
Animator_icondir = $(Animator_srcdir)/icons
AwtPDA_APP_ICONS += Animator.png
AwtPDA_APP_DESCRIPTORS += Animator.app

##### ArcTest
ArcTest_classes = \
	ArcTest
ArcTest_class_files = \
        $(patsubst %,$(ArcTest_classesdir)/%.class,$(ArcTest_classes))
ArcTest_classesdir = $(APPMANAGER_APPDIR)/ArcTest/classes
ArcTest_srcdir = $(WS_APPMANAGER_APPS_DIR)/ArcTest
ArcTest_icondir = $(ArcTest_srcdir)/icons
AwtPDA_APP_ICONS += ArcTest.png
AwtPDA_APP_DESCRIPTORS += ArcTest.app

##### DitherTest
DitherTest_classes = \
	DitherTest
DitherTest_class_files = \
        $(patsubst %,$(DitherTest_classesdir)/%.class,$(DitherTest_classes))
DitherTest_classesdir = $(APPMANAGER_APPDIR)/DitherTest/classes
DitherTest_srcdir = $(WS_APPMANAGER_APPS_DIR)/DitherTest
DitherTest_icondir = $(DitherTest_srcdir)/icons
AwtPDA_APP_ICONS += DitherTest.png
AwtPDA_APP_DESCRIPTORS += DitherTest.app

##### TextApps
TextApps_classes = \
	Blink \
	NervousText
TextApps_class_files = \
        $(patsubst %,$(TextApps_classesdir)/%.class,$(TextApps_classes))
TextApps_classesdir = $(APPMANAGER_APPDIR)/TextApps/classes
TextApps_srcdir = $(WS_APPMANAGER_APPS_DIR)/TextApps
TextApps_icondir = $(TextApps_srcdir)/icons
AwtPDA_APP_ICONS += TextApps.png
AwtPDA_APP_DESCRIPTORS += Blink.app NervousText.app

##### CaffeineMark
CaffeineMark_classesdir = $(APPMANAGER_APPDIR)/CaffeineMark/classes
CaffeineMark_srcdir = $(WS_APPMANAGER_APPS_DIR)/CaffeineMark
CaffeineMark_icondir = $(CaffeineMark_srcdir)/icons
CaffeineMark_classes = \
	BenchmarkMonitor \
	BenchmarkUnit \
	CaffeineMarkApp \
	CaffeineMarkApplet \
	CaffeineMarkBenchmark \
	CaffeineMarkEmbeddedApp \
	CaffeineMarkEmbeddedBenchmark \
	CaffeineMarkFrame \
	CaffeineMarkIndiv \
	DialogAtom \
	FloatAtom \
	GraphicsAtom \
	ImageAtom \
	LogicAtom \
	LoopAtom \
	MethodAtom \
	SieveAtom \
	StopWatch \
	StringAtom \
	TestDialog \
	TestWindow

CaffeineMark_class_files = \
        $(patsubst %,$(CaffeineMark_classesdir)/%.class,$(CaffeineMark_classes))
AwtPDA_APP_ICONS += CaffeineMark.png
AwtPDA_APP_DESCRIPTORS += CaffeineMark.app

################### VALUES FOR MENUS & MISC ###############################
AwtPDA_APP_ICONS += \
	Games.png \
	Generic.png \
	Graphics.png \
	Graphics2.png \
	Text.png \
	Utils.png \
	Utils2.png

AwtPDA_APP_DESCRIPTORS += \
	Text.menu \
	games.menu \
	graphics.menu \
	ticker.menu \
	utils.menu
