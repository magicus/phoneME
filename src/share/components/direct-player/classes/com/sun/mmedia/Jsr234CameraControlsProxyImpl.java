/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.mmedia;

import javax.microedition.media.Control;
import com.sun.amms.directcontrol.*;

public class Jsr234CameraControlsProxyImpl implements
        Jsr234CameraControlsProxy {

    private DirectCamera _cam;
    private DirectCameraControl _cameraControl;
    private DirectExposureControl _exposureControl;
    private DirectFlashControl _flashControl;
    private DirectFocusControl _focusControl;
    private DirectSnapshotControl _snapshotControl;
    private DirectZoomControl _zoomControl; 
    
    public Jsr234CameraControlsProxyImpl() {}
    
    public void setCameraPlayer(DirectCamera cam) {
        _cam = cam;
    }

    public synchronized Control getJsr234CameraControl( String control_name ) {
        System.out.println("getJsr234CameraControl(" + control_name + ")");
        if (_cam == null) {
            throw new
                RuntimeException("CameraControlsProxy: No player assigned!");
        }
        if (control_name.equals(
            "javax.microedition.amms.control.camera.CameraControl")) {
            if (null == _cameraControl) {
                _cameraControl = DirectCameraControl.createInstance(_cam);
            }
            return _cameraControl;
        }
        if (control_name.equals(
            "javax.microedition.amms.control.camera.ExposureControl")) {
            if (null == _exposureControl) {
                _exposureControl = DirectExposureControl.createInstance(
                    _cam.getNativeHandle());
            }
            return _exposureControl;
        }
        if (control_name.equals(
            "javax.microedition.amms.control.camera.FlashControl")) {
            if (null == _flashControl) {
                _flashControl = DirectFlashControl.createInstance(
                    _cam.getNativeHandle());
            }
            return _flashControl;
        }
        if (control_name.equals(
            "javax.microedition.amms.control.camera.FocusControl")) {
            if (null == _focusControl) {
                _focusControl = DirectFocusControl.createInstance(_cam);
            }
            return _focusControl;
        }
        if (control_name.equals(
            "javax.microedition.amms.control.camera.SnapshotControl")) {
            if (null == _snapshotControl) {
                _snapshotControl = DirectSnapshotControl.createInstance(_cam);
            }
            return _snapshotControl;
        }
        if (control_name.equals(
            "javax.microedition.amms.control.camera.ZoomControl")) {
            if (null == _zoomControl) {
                _zoomControl = DirectZoomControl.createInstance(
                    _cam.getNativeHandle());
            }
            return _zoomControl;
        }

        return null;
    }
}
