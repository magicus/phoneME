/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 * 
 * This source file is specific for Qt-based configurations.
 */

#include <qpainter.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <kni.h>
#include <midpStorage.h>
#include <anc_indicators.h>
#include <anc_qte_indicator.h>

/**
 * @file
 *
 * Native code to handle indicator status.
 */

/**
 * Platform handling code for turning off or on
 * indicators for signed MIDlet.
 *
 * IMPL_NOTE:Currently indicator does nothing for Java
 * and platform widget modules as we are waiting for
 * UI input.
 */
extern "C"
void anc_show_trusted_indicator(jboolean isTrusted) {
    IndicatorBar::setTrustedOn((bool)isTrusted);
}

/**
 * Porting implementation for network indicator.
 * It controls the LED as the network indicator, it
 * ONLY works on device. There is no equivalent in emulator.
 */
extern "C"
void anc_set_network_indicator(int counter) {
    IndicatorBar::setNetworkOn((bool)counter);
}

/**
 * Implement home icon on/off porting interface.
 */
extern "C"
void anc_toggle_home_icon(jboolean isHomeOn) {
    IndicatorBar::setHomeOn((bool)isHomeOn);
}

/**
 *  Turn on or off the backlight, or toggle it.
 *  The backlight will be turned on to the system configured level.
 *  This function is only valid if QT's COP and QWS is available.
 *
 *  @param mode if <code>mode</code> is:
 *              <code>ANC_BACKLIGHT_ON</code> - turn on the backlight
 *              <code>ANC_BACKLIGHT_OFF</code> - turn off the backlight
 *              <code>ANC_BACKLIGHT_TOGGLE</code> - toggle the backlight
 *              <code>ANC_BACKLIGHT_SUPPORTED<code> - do nothing
 *              (this is used to determine if backlight control is
 *              supported on a system without  changing the state of
 *              the backlight.)
 *  @return <code>KNI_TRUE</code> if the system supports backlight
 *              control, or <code>KNI_FALSE</code> otherwise.
 */
extern "C" jboolean
anc_show_backlight(AncBacklightState mode) {
    (void)mode;
    return KNI_FALSE;
}

/** Declare static field */
IndicatorBar* IndicatorBar::singleton; // = NULL

/**
 * Construct a bar that shows indicators.
 */
IndicatorBar::IndicatorBar(QWidget* parent) : QWidget(parent) {

    // Set fixed size
    setFixedSize(FULLWIDTH, 18);

    QString qiconPath;
    const pcsl_string * iconPath = storage_get_config_root(INTERNAL_STORAGE_ID);
    jint iconPath_len = pcsl_string_length(iconPath);
    const jchar * iconPath_data = pcsl_string_get_utf16_data(iconPath);

    if (NULL != iconPath_data) {
        qiconPath.setUnicodeCodes((const ushort *)iconPath_data, iconPath_len);
    } // else {
    // The qiconPath string will remain null.
    // If this happens, most likely, it may show as not found resources
    // }

    // Bar image is set as background , maintained by QWidget
    setBackgroundPixmap(QPixmap(qiconPath+"indicator_bar.png"));

    homeIcon    = new QPixmap(qiconPath+"indicator_home.png");
    trustedIcon = new QPixmap(qiconPath+"indicator_trusted.png");
    networkIcon = new QPixmap(qiconPath+"indicator_network.png");

    // Initialize paint flags for icons to false
    homeOn = KNI_FALSE;
    networkOn = KNI_FALSE;
    trustedOn = KNI_FALSE;

    // Remember this instance as the singleton
    singleton = this;

    pcsl_string_release_utf16_data(iconPath_data, iconPath);
}

/**
 * Destruct the bar that shows indicators.
 */
IndicatorBar::~IndicatorBar() {
    delete homeIcon;
    delete trustedIcon;
    delete networkIcon;
    singleton = NULL;
}

/**
 * Override QWidget to paint indicators.
 */
void IndicatorBar::paintEvent(QPaintEvent *e) {

    // Paint background first
    QWidget::paintEvent(e);

    QPainter painter(this);

    if (homeOn) {
	painter.drawPixmap(HOME_ICON_X, 2, *homeIcon);
    }

    if (trustedOn) {
	painter.drawPixmap(TRUSTED_ICON_X, 2, *trustedIcon);
    }

    if (networkOn) {
	painter.drawPixmap(NETWORK_ICON_X, 2, *networkIcon);
    }
}

/**
 * Return the singleton indicator bar object.
 * @param parent parent widget in case of first call. Otherwise, not used.
 * @return the singleton instance
 */
IndicatorBar* IndicatorBar::createSingleton(QWidget* parent) {
    if (singleton != NULL) {
	return NULL; // Disallow creation of multiple instances
    }

    new IndicatorBar(parent); // singleton is set inside constructor
    return singleton;
}

/**
 * Turn home indicator on or off.
 */
void IndicatorBar::setHomeOn(bool isOn) {
    if (singleton != NULL) {
	singleton->homeOn = isOn;
	singleton->repaint();
    }
}

/**
 * Turn trusted indicator on or off.
 */
void IndicatorBar::setTrustedOn(bool isOn) {
    if (singleton != NULL) {
	singleton->trustedOn = isOn;
	singleton->repaint();
    }
}

/**
 * Turn network indicator on or off.
 */
void IndicatorBar::setNetworkOn(bool isOn) {
    if (singleton != NULL) {
	singleton->networkOn = isOn;
	singleton->repaint();
    }
}
