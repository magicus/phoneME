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
 */

package javax.microedition.lcdui.game;

import com.sun.midp.i3test.*;
import javax.microedition.lcdui.*;
import javax.microedition.midlet.*;

public class TestSprite extends TestCase {
    public void runTests() {
        declare("testInitialSize");
        testInitialSize();

        declare("testTransformNone");
        testTransformNone();

        declare("testTransformRot90");
        testTransformRot90();

        declare("testTransformRot180");
        testTransformRot180();

        declare("testTransformRot270");
        testTransformRot270();

        declare("testTransformMirror");
        testTransformMirror();

        declare("testTransformMirrorRot90");
        testTransformMirrorRot90();

        declare("testTransformMirrorRot180");
        testTransformMirrorRot180();

        declare("testTransformMirrorRot270");
        testTransformMirrorRot270();
    }

    public void testInitialSize() {
        Image image = Image.createImage(7, 4);
        Sprite sprite = new Sprite(image);

        assertEquals(image.getWidth(), sprite.getWidth());
        assertEquals(image.getHeight(), sprite.getHeight());
    }

    public void testTransformNone() {
        testTransform(0, 0, Sprite.TRANS_NONE, 11, 19, 4, 0, 2, 3);
        testTransform(1, 2, Sprite.TRANS_NONE, 10, 17, 4, 0, 2, 3);
    }

    public void testTransformRot90() {
        testTransform(0, 0, Sprite.TRANS_ROT90, 8, 19, 1, 4, 3, 2);
        testTransform(1, 2, Sprite.TRANS_ROT90, 10, 18, 1, 4, 3, 2);
    }

    public void testTransformRot180() {
        testTransform(0, 0, Sprite.TRANS_ROT180, 5, 16, 1, 1, 2, 3);
        testTransform(1, 2, Sprite.TRANS_ROT180, 6, 18, 1, 1, 2, 3);
    }

    public void testTransformRot270() {
        testTransform(0, 0, Sprite.TRANS_ROT270, 11, 13, 0, 1, 3, 2);
        testTransform(1, 2, Sprite.TRANS_ROT270, 9, 14, 0, 1, 3, 2);
    }

    public void testTransformMirror() {
        testTransform(0, 0, Sprite.TRANS_MIRROR, 5, 19, 1, 0, 2, 3);
        testTransform(1, 2, Sprite.TRANS_MIRROR, 6, 17, 1, 0, 2, 3);
    }

    public void testTransformMirrorRot90() {
        testTransform(0, 0, Sprite.TRANS_MIRROR_ROT90, 8, 13, 1, 1, 3, 2);
        testTransform(1, 2, Sprite.TRANS_MIRROR_ROT90, 10, 14, 1, 1, 3, 2);
    }

    public void testTransformMirrorRot180() {
        testTransform(0, 0, Sprite.TRANS_MIRROR_ROT180, 11, 16, 4, 1, 2, 3);
        testTransform(1, 2, Sprite.TRANS_MIRROR_ROT180, 10, 18, 4, 1, 2, 3);
    }

    public void testTransformMirrorRot270() {
        testTransform(0, 0, Sprite.TRANS_MIRROR_ROT270, 11, 19, 0, 4, 3, 2);
        testTransform(1, 2, Sprite.TRANS_MIRROR_ROT270, 9, 18, 0, 4, 3, 2);
    }

    private void testTransform(int refX, int refY, int transform,
            int expectedX, int expectedY, int expColRectX, int expColRectY,
            int expColRectWidth, int expColRectHeight) {

        Sprite sprite = new Sprite(Image.createImage(7, 4));

        sprite.defineReferencePixel(refX, refY);
        sprite.setRefPixelPosition(11, 19);
        sprite.defineCollisionRectangle(4, 0, 2, 3);
        sprite.setTransform(transform);

        assertEquals("Collision rectangle X", 4, sprite.collisionRectX);
        assertEquals("Collision rectangle Y", 0, sprite.collisionRectY);
        assertEquals("Collision rectangle width", 2,
                     sprite.collisionRectWidth);
        assertEquals("Collision rectangle height", 3,
                     sprite.collisionRectHeight);

        assertEquals("X", expectedX, sprite.getX());
        assertEquals("Y", expectedY, sprite.getY());
        assertEquals("Transform collision rectangle X", expColRectX,
                     sprite.t_collisionRectX);
        assertEquals("Transform Collision rectangle Y", expColRectY,
                     sprite.t_collisionRectY);
        assertEquals("Transform collision rectangle width", expColRectWidth,
                     sprite.t_collisionRectWidth);
        assertEquals("Transform collision rectangle height", expColRectHeight,
                     sprite.t_collisionRectHeight);
    }
}
