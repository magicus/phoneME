/*
 * %W% %E%
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 */

package com.sun.jump.module.lifecycle;

import com.sun.jump.executive.JUMPIsolateProxy;
import com.sun.jump.executive.JUMPApplicationProxy;
import com.sun.jump.module.JUMPModule;
import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.common.JUMPProcessProxy;
import com.sun.jump.common.JUMPApplication;

/**
 * <code>JUMPApplicationLifecycleModule</code> is an executive module
 * that performs application lifecycle operations.
 */
public interface JUMPApplicationLifecycleModule extends JUMPModule {
    /**
     * Launch or return a running application for an installed
     * application, according to the lifecycle policy for this
     * lifecycle module.
     */
    public JUMPApplicationProxy launchApplication(JUMPApplication app,
            String args[]);
    
    /**
     * Returns any running instances of a given JUMPApplication.
     */
    public JUMPApplicationProxy[] getApplications(JUMPApplication app);
    
    /**
     * Returns any running application instances for this lifecycle module
     */
    public JUMPApplicationProxy[] getApplications();
    
}
