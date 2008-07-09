#
# Copyright 1990-2008 Sun Microsystems, Inc. All rights reserved.
# SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
#

$(JSR211_BRIDGE_CLASSES) : $(JSR211_BRIDGE_JAVA_FILES)
	@echo "Compiling $(JSR211_OSGI_BRIDGE_APPLICATION_NAME) bundle classes...";
	$(AT)mkdir -p  $(JSR211_BRIDGE_CLASSES_DIR);
	$(AT)$(JAVAC_CMD) \
		-d $(JSR211_BRIDGE_CLASSES_DIR) \
		-bootclasspath $(CVM_BUILDTIME_CLASSESDIR) 	\
		-classpath $(OSGI_CLASSPATH)$(PS)$(LIB_CLASSESJAR)$(PS)$(CVM_BUILDTIME_CLASSESZIP)$(PS)$(JSR_211_BUILD_DIR)/classes \
		$(JSR211_BRIDGE_JAVA_FILES) \

$(JSR211_BRIDGE_JAR) : $(JSR211_BRIDGE_CLASSES) $(JSR211_BRIDGE_MANIFEST)
	@echo "Creating $(JSR211_BRIDGE_JAR) ...";
	$(AT)mkdir -p  $(OSGI_BUNDLES_DIR);
	$(AT)$(CVM_JAR) cfm \
		$(JSR211_BRIDGE_JAR) \
		$(JSR211_BRIDGE_MANIFEST) \
		-C $(JSR211_BRIDGE_CLASSES_DIR) . \

