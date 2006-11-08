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


ifeq ($(APPMANAGER_ALL_APPS), true)
all:: AwtPDA_compile_apps AwtPDA_resources AwtPDA_install_copyrighted AwtPDA_install_noncopyrighted
else
all:: AwtPDA_compile_apps AwtPDA_resources AwtPDA_install_copyrighted
endif

AwtPDA_compile_apps: $(AwtPDA_APPS)

AwtPDA_install_copyrighted: 
	$(AT)echo "Installing AwtPDA Presentation Mode..."
	$(AT)cd $(APPMANAGER_CLASSESDIR); $(CVM_JAR) cf ../lib/$(AwtPDA_PRESENTATION_MODE_JARFILE) com/sun/appmanager/impl/presentation/AwtPDA/*
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/Animator.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/ArcTest.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/Clock.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/DitherTest.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/DrawTest.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/Games.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/Generic.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/Graphics.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/Graphics2.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/Text.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/TicTacToe.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/Tickers.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/Utils.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/Utils2.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/WireFrame.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/phone.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/ticker.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/Animator.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/ArcTest.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/Clock.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/DitherTest.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/DrawTest.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/MyTicker.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/Phone.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/TicTacToe.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/WireFrame.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/Text.menu $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/games.menu $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/graphics.menu $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/ticker.menu $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/utils.menu $(APPMANAGER_APPMENU_DIR)/

AwtPDA_install_noncopyrighted:
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/CaffeineMark.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/allcomponents.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/chess.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/hanoi.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/javanoid.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/kaos.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/stoneroids.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/superfoulegg.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPICONS_DIR)/zcalc.png  $(APPMANAGER_APPICONS_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/Hanoi.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/Javanoid.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/Kaos.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/Stoneroids.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/SuperFE.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/Zcalc.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/Chess.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/AllComponents.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/CaffeineMark.app $(APPMANAGER_APPMENU_DIR)/

AwtPDA_resources:
	$(AT)cp -f $(WS_APPMANAGER_SRCDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/*.properties $(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/
	$(AT)cp -f $(WS_APPMANAGER_SRCDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/background/*.png $(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/background/
	$(AT)cp -f $(WS_APPMANAGER_SRCDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/icons/*.png $(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/icons/
	$(AT)cp -f $(WS_APPMANAGER_SRCDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/login/*.png $(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/login/
	$(AT)cp -f $(WS_APPMANAGER_SRCDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/downloader/*.png $(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/downloader/
	$(AT)cp -f $(WS_APPMANAGER_SRCDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/progressbar/*.png $(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/progressbar/
	$(AT)cp -f $(WS_APPMANAGER_SRCDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/moveremove/*.png $(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/moveremove/
	$(AT)cp -f $(WS_APPMANAGER_SRCDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/preferences/*.png $(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/preferences/
	$(AT)cp -f $(WS_APPMANAGER_SRCDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/gauge/*.png $(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/AwtPDA/resources/gauge/

################################ APPS ################################

##### allcomponents
allcomponents:  .delete.allcomponentsclasses.list .report.allcomponentsclasses $(allcomponents_class_files) .compile.allcomponentsclasses .copy.allcomponentsresources .jar.allcomponentsclasses

$(allcomponents_classesdir)/%.class: $(allcomponents_srcdir)/%.java
	$(AT)echo $? >> .allcomponentsclasses.list

.report.allcomponentsclasses:
	@echo "Checking for appmanager app: allcomponents classes to compile..."

.compile.allcomponentsclasses:
	$(AT)if [ -s .allcomponentsclasses.list ] ; then                   \
                echo "Compiling allcomponents classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(allcomponents_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(allcomponents_srcdir)       \
                        @.allcomponentsclasses.list ;                      \
        fi

.delete.allcomponentsclasses.list:
	$(AT)$(RM) .allcomponentsclasses.list

.copy.allcomponentsresources:
	$(AT)echo "Copying allcomponents resources..."
	$(AT)cp -f $(allcomponents_icondir)/*.png $(allcomponents_classesdir)/

.jar.allcomponentsclasses:
	$(AT)echo "Jarring allcomponents..."
	$(AT)cd $(allcomponents_classesdir); $(CVM_JAR) cf ../allcomponents.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(allcomponents_classesdir)

##### javanoid
javanoid:  .delete.javanoidclasses.list .report.javanoidclasses $(javanoid_class_files) .compile.javanoidclasses .copy.javanoidresources .jar.javanoidclasses

$(javanoid_classesdir)/%.class: $(javanoid_srcdir)/%.java
	$(AT)echo $? >> .javanoidclasses.list

.report.javanoidclasses:
	@echo "Checking for appmanager app: javanoid classes to compile..."

.compile.javanoidclasses:
	$(AT)if [ -s .javanoidclasses.list ] ; then                   \
                echo "Compiling javanoid classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(javanoid_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(javanoid_srcdir)       \
                        @.javanoidclasses.list ;                      \
        fi

.delete.javanoidclasses.list:
	$(AT)$(RM) .javanoidclasses.list

.copy.javanoidresources:
	$(AT)echo "Copying javanoid resources...";
	$(AT)cp -f $(javanoid_srcdir)/*.gif $(javanoid_classesdir)
	$(AT)cp -f $(javanoid_srcdir)/*.dat $(javanoid_classesdir)
	$(AT)cp -f $(javanoid_srcdir)/*.au $(javanoid_classesdir)
	$(AT)cp -f $(javanoid_icondir)/*.png $(javanoid_classesdir)/

.jar.javanoidclasses:
	$(AT)echo "Jarring javanoid..."
	$(AT)cd $(javanoid_classesdir); $(CVM_JAR) cf ../javanoid.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(javanoid_classesdir)

##### kaos
kaos:  .delete.kaosclasses.list .report.kaosclasses $(kaos_class_files) .compile.kaosclasses .copy.kaosresources .jar.kaosclasses

$(kaos_classesdir)/%.class: $(kaos_srcdir)/%.java
	$(AT)echo $? >> .kaosclasses.list

.report.kaosclasses:
	@echo "Checking for appmanager app: kaos classes to compile..."

.compile.kaosclasses:
	$(AT)if [ -s .kaosclasses.list ] ; then                   \
                echo "Compiling kaos classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(kaos_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(kaos_srcdir)       \
                        @.kaosclasses.list ;                      \
        fi

.delete.kaosclasses.list:
	$(AT)$(RM) .kaosclasses.list

.copy.kaosresources:
	$(AT)echo "Copying kaos resources..."
	$(AT)cp -f $(kaos_srcdir)/net/ultrametrics/kaos/Kaos.properties $(kaos_classesdir)
	$(AT)cp -f $(kaos_icondir)/*.png $(kaos_classesdir)/

.jar.kaosclasses:
	$(AT)echo "Jarring kaos..."
	$(AT)cd $(kaos_classesdir); $(CVM_JAR) cf ../kaos.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(kaos_classesdir)

##### stoneroids
stoneroids:  .delete.stoneroidsclasses.list .report.stoneroidsclasses $(stoneroids_class_files) .copy.stoneroidsresources .compile.stoneroidsclasses .jar.stoneroidsclasses

$(stoneroids_classesdir)/%.class: $(stoneroids_srcdir)/%.java
	$(AT)echo $? >> .stoneroidsclasses.list

.report.stoneroidsclasses:
	@echo "Checking for appmanager app: stoneroids classes to compile..."

.compile.stoneroidsclasses:
	$(AT)if [ -s .stoneroidsclasses.list ] ; then                   \
                echo "Compiling stoneroids classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(stoneroids_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(stoneroids_srcdir)       \
                        @.stoneroidsclasses.list ;                      \
        fi

.delete.stoneroidsclasses.list:
	$(AT)$(RM) .stoneroidsclasses.list 

.copy.stoneroidsresources:
	$(AT)echo "Copying stoneroids resources..."
	$(AT)cp -f $(stoneroids_icondir)/*.png $(stoneroids_classesdir)/

.jar.stoneroidsclasses:
	$(AT)echo "Jarring stoneroids..."
	$(AT)cd $(stoneroids_classesdir); $(CVM_JAR) cf ../stoneroids.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(stoneroids_classesdir)

##### superfoulegg
superfoulegg:  .delete.superfouleggclasses.list .report.superfouleggclasses $(superfoulegg_class_files) .compile.superfouleggclasses .copy.superfouleggresources .jar.superfouleggclasses

$(superfoulegg_classesdir)/%.class: $(superfoulegg_srcdir)/%.java
	$(AT)echo $? >> .superfouleggclasses.list

.report.superfouleggclasses:
	@echo "Checking for appmanager app: superfoulegg classes to compile..."

.compile.superfouleggclasses:
	$(AT)if [ -s .superfouleggclasses.list ] ; then                   \
                echo "Compiling superfoulegg classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(superfoulegg_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(superfoulegg_srcdir)       \
                        @.superfouleggclasses.list ;                      \
        fi

.delete.superfouleggclasses.list:
	$(AT)$(RM) .superfouleggclasses.list

.copy.superfouleggresources:
	$(AT)echo "Copying superfoulegg resources..."
	$(AT)cp -f $(superfoulegg_srcdir)/images/*gif $(superfoulegg_classesdir)/images
	$(AT)cp -f $(superfoulegg_icondir)/*.png $(superfoulegg_classesdir)/

.jar.superfouleggclasses:
	$(AT)echo "Jarring superfoulegg..."
	$(AT)cd $(superfoulegg_classesdir); $(CVM_JAR) cf ../superfoulegg.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(superfoulegg_classesdir)

##### ticker
ticker:  .delete.tickerclasses.list .report.tickerclasses $(ticker_class_files) .compile.tickerclasses .copy.tickerresources .jar.tickerclasses

$(ticker_classesdir)/%.class: $(ticker_srcdir)/%.java
	$(AT)echo $? >> .tickerclasses.list

.report.tickerclasses:
	@echo "Checking for appmanager app: ticker classes to compile..."

.compile.tickerclasses:
	$(AT)if [ -s .tickerclasses.list ] ; then                   \
                echo "Compiling ticker classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(ticker_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(ticker_srcdir)       \
                        @.tickerclasses.list ;                      \
        fi

.delete.tickerclasses.list:
	$(AT)$(RM) .tickerclasses.list

.copy.tickerresources:
	$(AT)echo "Copying ticker resources..."
	$(AT)cp -f $(ticker_srcdir)/*.png $(ticker_classesdir)/
	$(AT)cp -f $(ticker_icondir)/*.png $(ticker_classesdir)/

.jar.tickerclasses:
	$(AT)echo "Jarring ticker..."
	$(AT)cd $(ticker_classesdir); $(CVM_JAR) cf ../ticker.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(ticker_classesdir)

##### zcalc
zcalc:  .delete.zcalcclasses.list .report.zcalcclasses $(zcalc_class_files) .compile.zcalcclasses .copy.zcalcresources .jar.zcalcclasses

$(zcalc_classesdir)/%.class: $(zcalc_srcdir)/%.java
	$(AT)echo $? >> .zcalcclasses.list

.report.zcalcclasses:
	@echo "Checking for appmanager app: zcalc classes to compile..."

.compile.zcalcclasses:
	$(AT)if [ -s .zcalcclasses.list ] ; then                   \
                echo "Compiling zcalc classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(zcalc_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(zcalc_srcdir)       \
                        @.zcalcclasses.list ;                      \
        fi

.delete.zcalcclasses.list:
	$(AT)$(RM) .zcalcclasses.list

.copy.zcalcresources:
	$(AT)echo "Copying zcalc resources..."
	$(AT)cp -f $(zcalc_icondir)/*.png $(zcalc_classesdir)/

.jar.zcalcclasses:
	$(AT)echo "Jarring zcalc..."
	$(AT)cd $(zcalc_classesdir); $(CVM_JAR) cf ../zcalc.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(zcalc_classesdir)

##### chess
chess:  .delete.chessclasses.list .report.chessclasses $(chess_class_files) .compile.chessclasses .copy.chessresources .jar.chessclasses

$(chess_classesdir)/%.class: $(chess_srcdir)/%.java
	$(AT)echo $? >> .chessclasses.list

.report.chessclasses:
	@echo "Checking for appmanager app: chess classes to compile..."

.compile.chessclasses:
	$(AT)if [ -s .chessclasses.list ] ; then                   \
                echo "Compiling chess classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(chess_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(chess_srcdir)       \
                        @.chessclasses.list ;                      \
        fi

.delete.chessclasses.list:
	$(AT)$(RM) .chessclasses.list

.copy.chessresources:
	$(AT)echo "Copying chess resources..."
	$(AT)cp -f $(chess_srcdir)/*gif $(chess_classesdir)/
	$(AT)cp -f $(chess_icondir)/*.png $(chess_classesdir)/

.jar.chessclasses:
	$(AT)echo "Jarring chess..."
	$(AT)cd $(chess_classesdir); $(CVM_JAR) cf ../chess.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(chess_classesdir)

##### hanoi
hanoi:  .delete.hanoiclasses.list .report.hanoiclasses $(hanoi_class_files) .compile.hanoiclasses .copy.hanoiresources .jar.hanoiclasses

$(hanoi_classesdir)/%.class: $(hanoi_srcdir)/%.java
	$(AT)echo $? >> .hanoiclasses.list

.report.hanoiclasses:
	@echo "Checking for appmanager app: hanoi classes to compile..."

.compile.hanoiclasses:
	$(AT)if [ -s .hanoiclasses.list ] ; then                   \
                echo "Compiling hanoi classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(hanoi_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(hanoi_srcdir)       \
                        @.hanoiclasses.list ;                      \
        fi

.delete.hanoiclasses.list:
	$(AT)$(RM) .hanoiclasses.list

.copy.hanoiresources:
	$(AT)echo "Copying hanoi resources..."
	$(AT)cp -f $(hanoi_icondir)/*.png $(hanoi_classesdir)/

.jar.hanoiclasses:
	$(AT)echo "Jarring hanoi..."
	$(AT)cd $(hanoi_classesdir); $(CVM_JAR) cf ../hanoi.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(hanoi_classesdir)

##### phone
phone:  .delete.phoneclasses.list .report.phoneclasses $(phone_class_files) .compile.phoneclasses .copy.phoneresources .jar.phoneclasses

$(phone_classesdir)/%.class: $(phone_srcdir)/%.java
	$(AT)echo $? >> .phoneclasses.list

.report.phoneclasses:
	@echo "Checking for appmanager app: phone classes to compile..."

.compile.phoneclasses:
	$(AT)if [ -s .phoneclasses.list ] ; then                   \
                echo "Compiling phone classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(phone_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(phone_srcdir)       \
                        @.phoneclasses.list ;                      \
        fi

.delete.phoneclasses.list:
	$(AT)$(RM) .phoneclasses.list

.copy.phoneresources:
	$(AT)echo "Copying phone resources..."
	$(AT)cp -f $(phone_srcdir)/*png $(phone_classesdir)/
	$(AT)cp -f $(phone_icondir)/*.png $(phone_classesdir)/

.jar.phoneclasses:
	$(AT)echo "Jarring phone..."
	$(AT)cd $(phone_classesdir); $(CVM_JAR) cf ../phone.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(phone_classesdir)

##### Clock
Clock:  .delete.Clockclasses.list .report.Clockclasses $(Clock_class_files) .compile.Clockclasses .copy.Clockresources .jar.Clockclassses

$(Clock_classesdir)/%.class: $(Clock_srcdir)/%.java
	$(AT)echo $? >> .Clockclasses.list

.report.Clockclasses:
	@echo "Checking for appmanager app: Clock classes to compile..."

.compile.Clockclasses:
	$(AT)if [ -s .Clockclasses.list ] ; then                   \
                echo "Compiling Clock classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(Clock_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(Clock_srcdir)       \
                        @.Clockclasses.list ;                      \
        fi

.delete.Clockclasses.list:
	$(AT)$(RM) .Clockclasses.list

.copy.Clockresources:
	$(AT)echo "Copying Clock resources..."
	$(AT)cp -f $(Clock_icondir)/*.png $(Clock_classesdir)/

.jar.Clockclassses:
	$(AT)echo "Jarring Clock..."
	$(AT)cd $(Clock_classesdir); $(CVM_JAR) cf ../Clock.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(Clock_classesdir)

##### DrawTest
DrawTest:  .delete.DrawTestclasses.list .report.DrawTestclasses $(DrawTest_class_files) .compile.DrawTestclasses .copy.DrawTestresources .jar.DrawTestclasses

$(DrawTest_classesdir)/%.class: $(DrawTest_srcdir)/%.java
	$(AT)echo $? >> .DrawTestclasses.list

.report.DrawTestclasses:
	@echo "Checking for appmanager app: DrawTest classes to compile..."

.compile.DrawTestclasses:
	$(AT)if [ -s .DrawTestclasses.list ] ; then                   \
                echo "Compiling DrawTest classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(DrawTest_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(DrawTest_srcdir)       \
                        @.DrawTestclasses.list ;                      \
        fi

.delete.DrawTestclasses.list:
	$(AT)$(RM) .DrawTestclasses.list

.copy.DrawTestresources:
	$(AT)echo "Copying DrawTest resources..."
	$(AT)cp -f $(DrawTest_icondir)/*.png $(DrawTest_classesdir)/

.jar.DrawTestclasses:
	$(AT)echo "Jarring DrawTest..."
	$(AT)cd $(DrawTest_classesdir); $(CVM_JAR) cf ../DrawTest.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(DrawTest_classesdir)

##### WireFrame
WireFrame:  .delete.WireFrameclasses.list .report.WireFrameclasses $(WireFrame_class_files) .compile.WireFrameclasses .copy.WireFrameresources .jar.WireFrameclassses

$(WireFrame_classesdir)/%.class: $(WireFrame_srcdir)/%.java
	$(AT)echo $? >> .WireFrameclasses.list

.report.WireFrameclasses:
	@echo "Checking for appmanager app: WireFrame classes to compile..."

.compile.WireFrameclasses:
	$(AT)if [ -s .WireFrameclasses.list ] ; then                   \
                echo "Compiling WireFrame classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(WireFrame_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(WireFrame_srcdir)       \
                        @.WireFrameclasses.list ;                      \
        fi

.delete.WireFrameclasses.list:
	$(AT)$(RM) .WireFrameclasses.list

.copy.WireFrameresources:
	$(AT)echo "Copying WireFrame resources..."
	$(AT)cp -f $(WireFrame_srcdir)/*.obj $(WireFrame_classesdir)/
	$(AT)cp -f $(WireFrame_icondir)/*.png $(WireFrame_classesdir)/

.jar.WireFrameclassses:
	$(AT)echo "Jarring WireFrame..."
	$(AT)cd $(WireFrame_classesdir); $(CVM_JAR) cf ../WireFrame.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(WireFrame_classesdir)

##### TicTacToe
TicTacToe:  .delete.TicTacToeclasses.list .report.TicTacToeclasses $(TicTacToe_class_files) .compile.TicTacToeclasses .copy.TicTacToeresources .jar.TicTacToeclasses

$(TicTacToe_classesdir)/%.class: $(TicTacToe_srcdir)/%.java
	$(AT)echo $? >> .TicTacToeclasses.list

.report.TicTacToeclasses:
	@echo "Checking for appmanager app: TicTacToe classes to compile..."

.compile.TicTacToeclasses:
	$(AT)if [ -s .TicTacToeclasses.list ] ; then                   \
                echo "Compiling TicTacToe classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(TicTacToe_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(TicTacToe_srcdir)       \
                        @.TicTacToeclasses.list ;                      \
        fi

.delete.TicTacToeclasses.list:
	$(AT)$(RM) .TicTacToeclasses.list

.copy.TicTacToeresources:
	$(AT)echo "Copying TicTacToe resources..."
	$(AT)cp -f $(TicTacToe_srcdir)/images/*gif $(TicTacToe_classesdir)/images
	$(AT)cp -f $(TicTacToe_icondir)/*.png $(TicTacToe_classesdir)

.jar.TicTacToeclasses:
	$(AT)echo "Jarring TicTacToe..."
	$(AT)cd $(TicTacToe_classesdir); $(CVM_JAR) cf ../TicTacToe.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(TicTacToe_classesdir)

##### Animator
Animator:  .delete.Animatorclasses.list .report.Animatorclasses $(Animator_class_files) .compile.Animatorclasses .copy.Animatorresources .jar.Animatorclassses

$(Animator_classesdir)/%.class: $(Animator_srcdir)/%.java
	$(AT)echo $? >> .Animatorclasses.list

.report.Animatorclasses:
	@echo "Checking for appmanager app: Animator classes to compile..."

.compile.Animatorclasses:
	$(AT)if [ -s .Animatorclasses.list ] ; then                   \
                echo "Compiling Animator classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(Animator_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(Animator_srcdir)       \
                        @.Animatorclasses.list ;                      \
        fi

.delete.Animatorclasses.list:
	$(AT)$(RM) .Animatorclasses.list

.copy.Animatorresources:
	$(AT)echo "Copying Animator resources..."
	$(AT)cp -f $(Animator_srcdir)/images/*gif $(Animator_classesdir)/images
	$(AT)cp -f $(Animator_icondir)/*.png $(Animator_classesdir)/

.jar.Animatorclassses:
	$(AT)echo "Jarring Animator..."
	$(AT)cd $(Animator_classesdir); $(CVM_JAR) cf ../Animator.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(Animator_classesdir)

##### ArcTest
ArcTest:  .delete.ArcTestclasses.list .report.ArcTestclasses $(ArcTest_class_files) .compile.ArcTestclasses .copy.ArcTestresources .jar.ArcTestclasses

$(ArcTest_classesdir)/%.class: $(ArcTest_srcdir)/%.java
	$(AT)echo $? >> .ArcTestclasses.list

.report.ArcTestclasses:
	@echo "Checking for appmanager app: ArcTest classes to compile..."

.compile.ArcTestclasses:
	$(AT)if [ -s .ArcTestclasses.list ] ; then                   \
                echo "Compiling ArcTest classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(ArcTest_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(ArcTest_srcdir)       \
                        @.ArcTestclasses.list ;                      \
        fi

.delete.ArcTestclasses.list:
	$(AT)$(RM) .ArcTestclasses.list

.copy.ArcTestresources:
	$(AT)echo "Copying ArcTest resources..."
	$(AT)cp -f $(ArcTest_icondir)/*.png $(ArcTest_classesdir)/

.jar.ArcTestclasses:
	$(AT)echo "Jarring ArcTest..."
	$(AT)cd $(ArcTest_classesdir); $(CVM_JAR) cf ../ArcTest.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(ArcTest_classesdir)

##### DitherTest
DitherTest:  .delete.DitherTestclasses.list .report.DitherTestclasses $(DitherTest_class_files) .compile.DitherTestclasses .copy.DitherTestresources .jar.DitherTestclasses

$(DitherTest_classesdir)/%.class: $(DitherTest_srcdir)/%.java
	$(AT)echo $? >> .DitherTestclasses.list

.report.DitherTestclasses:
	@echo "Checking for appmanager app: DitherTest classes to compile..."

.compile.DitherTestclasses:
	$(AT)if [ -s .DitherTestclasses.list ] ; then                   \
                echo "Compiling DitherTest classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(DitherTest_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(DitherTest_srcdir)       \
                        @.DitherTestclasses.list ;                      \
        fi

.delete.DitherTestclasses.list:
	$(AT)$(RM) .DitherTestclasses.list

.copy.DitherTestresources:
	$(AT)echo "Copying DitherTest resources..."
	$(AT)cp -f $(DitherTest_icondir)/*.png $(DitherTest_classesdir)/

.jar.DitherTestclasses:
	$(AT)echo "Jarring DitherTest..."
	$(AT)cd $(DitherTest_classesdir); $(CVM_JAR) cf ../DitherTest.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(DitherTest_classesdir)

##### TextApps
TextApps:  .delete.TextAppsclasses.list .report.TextAppsclasses $(TextApps_class_files) .compile.TextAppsclasses .copy.TextAppsresources .jar.TextAppsclasses

$(TextApps_classesdir)/%.class: $(TextApps_srcdir)/%.java
	$(AT)echo $? >> .TextAppsclasses.list

.report.TextAppsclasses:
	@echo "Checking for appmanager app: TextApps classes to compile..."

.compile.TextAppsclasses:
	$(AT)if [ -s .TextAppsclasses.list ] ; then                   \
                echo "Compiling TextApps classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(TextApps_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(TextApps_srcdir)       \
                        @.TextAppsclasses.list ;                      \
        fi

.delete.TextAppsclasses.list:
	$(AT)$(RM) .TextAppsclasses.list

.copy.TextAppsresources:
	$(AT)echo "Copying TextApps resources..."
	$(AT)cp -f $(TextApps_icondir)/*.png $(TextApps_classesdir)/

.jar.TextAppsclasses:
	$(AT)echo "Jarring TextApps..."
	$(AT)cd $(TextApps_classesdir); $(CVM_JAR) cf ../TextApps.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(TextApps_classesdir)

##### CaffeineMark
CaffeineMark:  .delete.CaffeineMarkclasses.list .report.CaffeineMarkclasses $(CaffeineMark_class_files) .compile.CaffeineMarkclasses .copy.CaffeineMarkresources .jar.CaffeineMarkclassses

$(CaffeineMark_classesdir)/%.class: $(CaffeineMark_srcdir)/%.java
	$(AT)echo $? >> .CaffeineMarkclasses.list

.report.CaffeineMarkclasses:
	@echo "Checking for appmanager app: CaffeineMark classes to compile..."

.compile.CaffeineMarkclasses:
	$(AT)if [ -s .CaffeineMarkclasses.list ] ; then                   \
                echo "Compiling CaffeineMark classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(CaffeineMark_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(CaffeineMark_srcdir)       \
                        @.CaffeineMarkclasses.list ;                      \
        fi

.delete.CaffeineMarkclasses.list:
	$(AT)$(RM) .CaffeineMarkclasses.list

.copy.CaffeineMarkresources:
	$(AT)echo "Copying CaffeineMark resources..."
	$(AT)cp -f $(CaffeineMark_srcdir)/*gif $(CaffeineMark_classesdir)/
	$(AT)cp -f $(CaffeineMark_icondir)/*.png $(CaffeineMark_classesdir)/

.jar.CaffeineMarkclassses:
	$(AT)echo "Jarring CaffeineMark..."
	$(AT)cd $(CaffeineMark_classesdir); $(CVM_JAR) cf ../CaffeineMark.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(CaffeineMark_classesdir)
