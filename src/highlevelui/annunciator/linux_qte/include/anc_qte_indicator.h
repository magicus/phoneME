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

#ifndef _MIDP_INDICATOR_IMPL_H_
#define _MIDP_INDICATOR_IMPL_H_

#include <qwidget.h>
#include <qpixmap.h>

/**
 * @file
 *
 * Platform specific (Sharp Zaurus) indicators implementation.
 */

#define HOME_ICON_X		FULLWIDTH-80
#define TRUSTED_ICON_X		(HOME_ICON_X+20)
#define NETWORK_ICON_X		(TRUSTED_ICON_X+20)

/**
 * A bar that shows indicators.
 */
class IndicatorBar : public QWidget {

    QPixmap* homeIcon;
    QPixmap* trustedIcon;
    QPixmap* networkIcon;

    bool homeOn;    // = false
    bool trustedOn; // = false
    bool networkOn; // = false

    static IndicatorBar* singleton; // = NULL

protected:
    /**
     * Construct a bar that shows indicators.
     */
    IndicatorBar(QWidget* parent);

    /**
     * Destruct the bar.
     */
    ~IndicatorBar();
    
    /**
     * Override QWidget to paint indicators.
     */
    void paintEvent(QPaintEvent *e);

public:

    /**
     * Create the singleton indicator bar object.
     */
    static IndicatorBar* createSingleton(QWidget* parent);

    /**
     * Turn home indicator on or off.
     */
    static void setHomeOn(bool isOn);

    /**
     * Turn trusted indicator on or off.
     */
    static void setTrustedOn(bool isOn);

    /**
     * Turn network indicator on or off.
     */
    static void setNetworkOn(bool isOn);
};

#endif /* _MIDP_INDICATOR_IMPL_H_ */
