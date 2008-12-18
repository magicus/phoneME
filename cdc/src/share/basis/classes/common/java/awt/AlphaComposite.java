/*
 * @(#)AlphaComposite.java	1.7 06/10/10
 *
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
 *
 */

package java.awt;

public final class AlphaComposite implements Composite {
    /**
     * Porter-Duff Clear rule.
     * Both the color and the alpha of the destination are cleared.
     * Neither the source nor the destination is used as input.
     *<p>
     * Fs = 0 and Fd = 0, thus:
     *<pre>
     *  Cd = 0
     *  Ad = 0
     *</pre>
     *
     */
    public static final int CLEAR = 1;
    /**
     * Porter-Duff Source rule.
     * The source is copied to the destination.
     * The destination is not used as input.
     *<p>
     * Fs = 1 and Fd = 0, thus:
     *<pre>
     *  Cd = Cs
     *  Ad = As
     *</pre>
     */
    public static final int SRC = 2;
    /**
     * Porter-Duff Source Over Destination rule.
     * The source is composited over the destination.
     *<p>
     * Fs = 1 and Fd = (1-As), thus:
     *<pre>
     *  Cd = Cs + Cd*(1-As)
     *  Ad = As + Ad*(1-As)
     *</pre>
     */
    public static final int SRC_OVER = 3;
    /**
     * <code>AlphaComposite</code> object that implements the opaque CLEAR rule
     * with an alpha of 1.0f.
     * @see #CLEAR
     */
    public static final AlphaComposite Clear = new AlphaComposite(CLEAR);
    /**
     * <code>AlphaComposite</code> object that implements the opaque SRC rule
     * with an alpha of 1.0f.
     * @see #SRC
     */
    public static final AlphaComposite Src = new AlphaComposite(SRC);
    /**
     * <code>AlphaComposite</code> object that implements the opaque SRC_OVER rule
     * with an alpha of 1.0f.
     * @see #SRC_OVER
     */
    public static final AlphaComposite SrcOver = new AlphaComposite(SRC_OVER);
    private static final int MIN_RULE = CLEAR;
    private static final int MAX_RULE = SRC_OVER;
    float extraAlpha;
    int rule;
    private AlphaComposite(int rule) {
        this(rule, 1.0f);
    }

    private AlphaComposite(int rule, float alpha) {
        if (alpha < 0.0f || alpha > 1.0f) {
            throw new IllegalArgumentException("alpha value out of range");
        }
        if (rule < MIN_RULE || rule > MAX_RULE) {
            throw new IllegalArgumentException("unknown composite rule");
        }
        this.rule = rule;
        this.extraAlpha = alpha;
    }

    /**
     * Creates an <code>AlphaComposite</code> object with the specified rule.
     * @param rule the compositing rule
     * @throws IllegalArgumentException if <code>rule</code> is not one of
     *         the following:  {@link #CLEAR}, {@link #SRC}, {@link #DST},
     *         {@link #SRC_OVER}, {@link #DST_OVER}, {@link #SRC_IN},
     *         {@link #DST_IN}, {@link #SRC_OUT}, {@link #DST_OUT},
     *         {@link #SRC_ATOP}, {@link #DST_ATOP}, or {@link #XOR}
     */
    public static AlphaComposite getInstance(int rule) {
        switch (rule) {
        case CLEAR:
            return Clear;

        case SRC:
            return Src;

        case SRC_OVER:
            return SrcOver;

        default:
            throw new IllegalArgumentException("unknown composite rule");
        }
    }

    /**
     * Creates an <code>AlphaComposite</code> object with the specified rule and
     * the constant alpha to multiply with the alpha of the source.
     * The source is multiplied with the specified alpha before being composited
     * with the destination.
     * @param rule the compositing rule
     * @param alpha the constant alpha to be multiplied with the alpha of
     * the source. <code>alpha</code> must be a floating point number in the
     * inclusive range [0.0,&nbsp;1.0].
     */
    public static AlphaComposite getInstance(int rule, float alpha) {
        if (alpha == 1.0f) {
            return getInstance(rule);
        }
        return new AlphaComposite(rule, alpha);
    }

    /**
     * Returns the alpha value of this<code>AlphaComposite</code>.  If this
     * <code>AlphaComposite</code> does not have an alpha value, 1.0 is returned.
     * @return the alpha value of this <code>AlphaComposite</code>.
     */
    public float getAlpha() {
        return extraAlpha;
    }

    /**
     * Returns the compositing rule of this <code>AlphaComposite</code>.
     * @return the compositing rule of this <code>AlphaComposite</code>.
     */
    public int getRule() {
        return rule;
    }

    /**
     * Returns the hashcode for this composite.
     * @return      a hash code for this composite.
     */
    public int hashCode() {
        return (Float.floatToIntBits(extraAlpha) * 31 + rule);
    }

    /**
     * Tests if the specified {@link Object} is equal to this
     * <code>AlphaComposite</code> object.
     * @param obj the <code>Object</code> to test for equality
     * @return <code>true</code> if <code>obj</code> equals this
     * <code>AlphaComposite</code>; <code>false</code> otherwise.
     */
    public boolean equals(Object obj) {
        if (!(obj instanceof AlphaComposite)) {
            return false;
        }
        AlphaComposite ac = (AlphaComposite) obj;
        if (rule != ac.rule) {
            return false;
        }
        if (extraAlpha != ac.extraAlpha) {
            return false;
        }
        return true;
    }
}
