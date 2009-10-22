/*
 * @(#)Color.java	1.51 06/10/10
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

import java.io.*;
import java.lang.*;
import java.awt.color.ColorSpace;

/**
 * This class encapsulates colors using the RGB format. In RGB
 * format, the red, blue, and green components of a color are each
 * represented by an integer in the range 0-255. The value 0
 * indicates no contribution from this primary color. The value 255
 * indicates the maximum intensity of this color component.
 * <p>
 * Although the <code>Color</code> class is based on the
 * three-component RGB model, the class provides a set of convenience
 * methods for converting between RGB and HSB colors. For a
 * definition of the RGB and HSB color models, see Foley, van&nbsp;Dam,
 * Feiner, and Hughes, <cite>Computer Graphics: Principles
 * and Practice</cite>.
 *
 * @version 	1.46, 08/19/02
 * @author 	Sami Shaio
 * @author 	Arthur van Hoff
 * @since       JDK1.0
 */
public class Color implements java.io.Serializable, Transparency {
    /**
     * The color white.
     */
    public final static Color white = new Color(255, 255, 255);
    /**
     * The color white.  In the default sRGB space.
     */
    public final static Color WHITE = white;
    /**
     * The color light gray.
     */
    public final static Color lightGray = new Color(192, 192, 192);

    /**
     * The color light gray.  In the default sRGB space.
     */
    public final static Color LIGHT_GRAY = lightGray;

    /**
     * The color gray.
     */
    public final static Color gray = new Color(128, 128, 128);

    /**
     * The color gray.  In the default sRGB space.
     */
    public final static Color GRAY = gray;

    /**
     * The color dark gray.
     */
    public final static Color darkGray = new Color(64, 64, 64);

    /**
     * The color dark gray.  In the default sRGB space.
     */
    public final static Color DARK_GRAY = darkGray;

    /**
     * The color black.
     */
    public final static Color black = new Color(0, 0, 0);

    /**
     * The color black.  In the default sRGB space.
     */
    public final static Color BLACK = black;

    /**
     * The color red.
     */
    public final static Color red = new Color(255, 0, 0);

    /**
     * The color red.  In the default sRGB space.
     */
    public final static Color RED = red;

    /**
     * The color pink.
     */
    public final static Color pink = new Color(255, 175, 175);

    /**
     * The color pink.  In the default sRGB space.
     */
    public final static Color PINK = pink;

    /**
     * The color orange.
     */
    public final static Color orange = new Color(255, 200, 0);

    /**
     * The color orange.  In the default sRGB space.
     */
    public final static Color ORANGE = orange;

    /**
     * The color yellow.
     */
    public final static Color yellow = new Color(255, 255, 0);

    /**
     * The color yellow.  In the default sRGB space.
     */
    public final static Color YELLOW = yellow;

    /**
     * The color green.
     */
    public final static Color green = new Color(0, 255, 0);

    /**
     * The color green.  In the default sRGB space.
     */
    public final static Color GREEN = green;

    /**
     * The color magenta.
     */
    public final static Color magenta = new Color(255, 0, 255);

    /**
     * The color magenta.  In the default sRGB space.
     */
    public final static Color MAGENTA = magenta;

    /**
     * The color cyan.
     */
    public final static Color cyan = new Color(0, 255, 255);

    /**
     * The color cyan.  In the default sRGB space.
     */
    public final static Color CYAN = cyan;

    /**
     * The color blue.
     */
    public final static Color blue = new Color(0, 0, 255);

    /**
     * The color blue.  In the default sRGB space.
     */
    public final static Color BLUE = blue;

    /**
     * Private data.
     */
    transient private int pData;
    /**
     * The color value.
     */
    int value;
    /**
     * The color value in the default sRGB <code>ColorSpace</code> as
     * <code>float</code> components (no alpha).
     * If <code>null</code> after object construction, this must be an
     * sRGB color constructed with 8-bit precision, so compute from the
     * <code>int</code> color value.
     * @serial
     * @see #getRGBColorComponents
     * @see #getRGBComponents
     */
    private float frgbvalue[] = null;
    /**
     * The color value in the native <code>ColorSpace</code> as
     * <code>float</code> components (no alpha).
     * If <code>null</code> after object construction, this must be an
     * sRGB color constructed with 8-bit precision, so compute from the
     * <code>int</code> color value.
     * @serial
     * @see #getRGBColorComponents
     * @see #getRGBComponents
     */
    private float fvalue[] = null;
    /**
     * The alpha value as a <code>float</code> component.
     * If <code>frgbvalue</code> is <code>null</code>, this is not valid
     * data, so compute from the <code>int</code> color value.
     * @serial
     * @see #getRGBComponents
     * @see #getComponents
     */
    private float falpha = 0.0f;
    /**
     * The <code>ColorSpace</code>.  If <code>null</code>, then it's
     * default is sRGB.
     * @serial
     * @see #getColor
     * @see #getColorSpace
     * @see #getColorComponents
     */
    private ColorSpace cs = null;
    /*
     * JDK 1.1 serialVersionUID
     */
    private static final long serialVersionUID = 118526816881161077L;
    /**
     * Checks the color integer components supplied for validity.
     * Throws an IllegalArgumentException if the value is out of range.
     * @param r the Red component
     * @param g the Green component
     * @param b the Blue component
     **/
    private static void testColorValueRange(int r, int g, int b, int a) {
        boolean rangeError = false;
        String badComponentString = "";
        if (a < 0 || a > 255) {
            rangeError = true;
            badComponentString = badComponentString + " Alpha";
        }
        if (r < 0 || r > 255) {
            rangeError = true;
            badComponentString = badComponentString + " Red";
        }
        if (g < 0 || g > 255) {
            rangeError = true;
            badComponentString = badComponentString + " Green";
        }
        if (b < 0 || b > 255) {
            rangeError = true;
            badComponentString = badComponentString + " Blue";
        }
        if (rangeError == true) {
            throw new IllegalArgumentException("Color parameter outside of expected range:"
                    + badComponentString);
        }
    }

    /**
     * Checks the color float components supplied for validity.
     * Throws an IllegalArgumentException if the value is out of range.
     * @param r the Red component
     * @param g the Green component
     * @param b the Blue component
     **/
    private static void testColorValueRange(float r, float g, float b, float a) {
        boolean rangeError = false;
        String badComponentString = "";
        if (a < 0 || a > 1.0) {
            rangeError = true;
            badComponentString = badComponentString + " Alpha";
        }
        if (r < 0.0 || r > 1.0) {
            rangeError = true;
            badComponentString = badComponentString + " Red";
        }
        if (g < 0.0 || g > 1.0) {
            rangeError = true;
            badComponentString = badComponentString + " Green";
        }
        if (b < 0.0 || b > 1.0) {
            rangeError = true;
            badComponentString = badComponentString + " Blue";
        }
        if (rangeError == true) {
            throw new IllegalArgumentException("Color parameter outside of expected range:"
                    + badComponentString);
        }
    }

    /**
     * Creates a color with the specified red, green, and blue
     * components. The three arguments must each be in the range
     * 0-255.
     * <p>
     * The actual color used in rendering depends on finding the best
     * match given the color space available for a given output device.
     * @param       r the red component.
     * @param       g the green component.
     * @param       b the blue component.
     * @see         java.awt.Color#getRed
     * @see         java.awt.Color#getGreen
     * @see         java.awt.Color#getBlue
     * @see         java.awt.Color#getRGB
     * @since       JDK1.0
     */
    public Color(int r, int g, int b) {
        this(((r & 0xFF) << 16) | ((g & 0xFF) << 8) | ((b & 0xFF) << 0));
        testColorValueRange(r, g, b, 255);
    }

    /**
     * Creates an sRGB color with the specified red, green, blue, and alpha
     * values in the range (0 - 255).
     * @param r the red component
     * @param g the green component
     * @param b the blue component
     * @param a the alpha component
     * @see #getRed
     * @see #getGreen
     * @see #getBlue
     * @see #getAlpha
     * @see #getRGB
     */
    public Color(int r, int g, int b, int a) {
        value = ((a & 0xFF) << 24) |
                ((r & 0xFF) << 16) |
                ((g & 0xFF) << 8) |
                ((b & 0xFF) << 0);
        testColorValueRange(r, g, b, a);
    }

    /**
     * Creates a color with the specified RGB value, where the red
     * component is in bits 16-23 of the argument, the green
     * component is in bits 8-15 of the argument, and the blue
     * component is in bits 0-7. The value zero indicates no
     * contribution from the primary color component.
     * <p>
     * The actual color used in rendering depends on finding the best
     * match given the color space available for a given output device.
     * @param       rgb   an integer giving the red, green, and blue components.
     * @see         java.awt.image.ColorModel#getRGBdefault
     * @see         java.awt.Color#getRed
     * @see         java.awt.Color#getGreen
     * @see         java.awt.Color#getBlue
     * @see         java.awt.Color#getRGB
     * @since       JDK1.0
     */
    public Color(int rgb) {
        value = 0xff000000 | rgb;
    }

    /**
     * Creates an sRGB color with the specified combined RGBA value consisting
     * of the alpha component in bits 24-31, the red component in bits 16-23,
     * the green component in bits 8-15, and the blue component in bits 0-7.
     * If the <code>hasalpha</code> argument is <code>false</code>, alpha
     * is defaulted to 255.
     * @param rgba the combined RGBA components
     * @param hasalpha <code>true</code> if the alpha bits are valid;
     * <code>false</code> otherwise
     * @see java.awt.image.ColorModel#getRGBdefault
     * @see #getRed
     * @see #getGreen
     * @see #getBlue
     * @see #getAlpha
     * @see #getRGB
     */
    public Color(int rgba, boolean hasalpha) {
        if (hasalpha) {
            value = rgba;
        } else {
            value = 0xff000000 | rgba;
        }
    }

    /**
     * Creates a color with the specified red, green, and blue values,
     * where each of the values is in the range 0.0-1.0. The value
     * 0.0 indicates no contribution from the primary color component.
     * The value 1.0 indicates the maximum intensity of the primary color
     * component.
     * <p>
     * The actual color used in rendering depends on finding the best
     * match given the color space available for a given output device.
     * @param       r the red component
     * @param       g the red component
     * @param       b the red component
     * @see         java.awt.Color#getRed
     * @see         java.awt.Color#getGreen
     * @see         java.awt.Color#getBlue
     * @see         java.awt.Color#getRGB
     * @since       JDK1.0
     */
    public Color(float r, float g, float b) {
        this(r, g, b, 1.0f);
    }

    /**
     * Creates an sRGB color with the specified red, green, blue, and
     * alpha values in the range (0.0 - 1.0).  The actual color
     * used in rendering depends on finding the best match given the
     * color space available for a particular output device.
     * @param r the red component
     * @param g the green component
     * @param b the blue component
     * @param a the alpha component
     * @see #getRed
     * @see #getGreen
     * @see #getBlue
     * @see #getAlpha
     * @see #getRGB
     */
    public Color(float r, float g, float b, float a) {
        this((int) (r * 255 + 0.5), (int) (g * 255 + 0.5), (int) (b * 255 + 0.5), (int) (a * 255 + 0.5));
        testColorValueRange(r, g, b, a);
        frgbvalue = new float[3];
        frgbvalue[0] = r;
        frgbvalue[1] = g;
        frgbvalue[2] = b;
        falpha = a;
        fvalue = frgbvalue;
    }

    /**
     * Gets the red component of this color. The result is
     * an integer in the range 0 to 255.
     * @return        the red component of this color.
     * @see           java.awt.Color#getRGB
     * @since         JDK1.0
     */
    public int getRed() {
        return (getRGB() >> 16) & 0xFF;
    }

    /**
     * Gets the green component of this color. The result is
     * an integer in the range 0 to 255.
     * @return        the green component of this color.
     * @see           java.awt.Color#getRGB
     * @since         JDK1.0
     */
    public int getGreen() {
        return (getRGB() >> 8) & 0xFF;
    }

    /**
     * Gets the blue component of this color. The result is
     * an integer in the range 0 to 255.
     * @return        the blue component of this color.
     * @see           java.awt.Color#getRGB
     * @since         JDK1.0
     */
    public int getBlue() {
        return (getRGB() >> 0) & 0xFF;
    }

    /**
     * Returns the alpha component in the range 0-255.
     * @return the alpha component.
     * @see #getRGB
     */
    public int getAlpha() {
        return (getRGB() >> 24) & 0xff;
    }

    /**
     * Gets the RGB value representing the color in the default RGB ColorModel.
     * The red, green, and blue components of the color are each scaled to be
     * a value between 0 (abscence of the color) and 255 (complete saturation).
     * Bits 24-31 of the returned integer are 0xff, bits 16-23 are the red
     * value, bit 8-15 are the green value, and bits 0-7 are the blue value.
     * @see java.awt.image.ColorModel#getRGBdefault
     * @see #getRed
     * @see #getGreen
     * @see #getBlue
     * @since JDK1.0
     */
    public int getRGB() {
        return value;
    }
    private static final int FACTOR = 70;
    /**
     * Creates a brighter version of this color.
     * <p>
     * This method applies an arbitrary scale factor to each of the three RGB
     * components of the color to create a brighter version of the same
     * color. Although <code>brighter</code> and <code>darker</code> are
     * inverse operations, the results of a series of invocations of
     * these two methods may be inconsistent because of rounding errors.
     * @return     a new <code>Color</code> object,
     *                            a brighter version of this color.
     * @see        java.awt.Color#darker
     * @since      JDK1.0
     */
    public Color brighter() {
        int r = getRed();
        int g = getGreen();
        int b = getBlue();
        /* From 2D group:
         * 1. black.brighter() should return grey
         * 2. applying brighter to blue will always return blue, brighter
         * 3. non pure color (non zero rgb) will eventually return white
         */
        int i = (int) (100 / (100 - FACTOR));
        if (r == 0 && g == 0 && b == 0) {
            return new Color(i, i, i);
        }
        if (r > 0 && r < i) r = i;
        if (g > 0 && g < i) g = i;
        if (b > 0 && b < i) b = i;
        return new Color(Math.min((r * 100) / FACTOR, 255),
                Math.min((g * 100) / FACTOR, 255),
                Math.min((b * 100) / FACTOR, 255));
    }

    /**
     * Creates a darker version of this color.
     * <p>
     * This method applies an arbitrary scale factor to each of the three RGB
     * components of the color to create a darker version of the same
     * color. Although <code>brighter</code> and <code>darker</code> are
     * inverse operations, the results of a series of invocations of
     * these two methods may be inconsistent because of rounding errors.
     * @return  a new <code>Color</code> object,
     *                              a darker version of this color.
     * @see        java.awt.Color#brighter
     * @since      JDK1.0
     */
    public Color darker() {
        return new Color(Math.max((getRed() * FACTOR) / 100, 0),
                Math.max((getGreen() * FACTOR) / 100, 0),
                Math.max((getBlue() * FACTOR) / 100, 0));
    }

    /**
     * Computes the hash code for this color.
     * @return     a hash code value for this object.
     * @since      JDK1.0
     */
    public int hashCode() {
        return value;
    }

    /**
     * Determines whether another object is equal to this color.
     * <p>
     * The result is <code>true</code> if and only if the argument is not
     * <code>null</code> and is a <code>Color</code> object that has the same
     * red, green, and blue values as this object.
     * @param       obj   the object to compare with.
     * @return      <code>true</code> if the objects are the same;
     *                             <code>false</code> otherwise.
     * @since   JDK1.0
     */
    public boolean equals(Object obj) {
        return obj instanceof Color && ((Color) obj).value == this.value;
    }

    /**
     * Creates a string that represents this color and indicates the
     * values of its RGB components.
     * @return     a representation of this color as a
     *                           <code>String</code> object.
     * @since      JDK1.0
     */
    public String toString() {
        return getClass().getName() + "[r=" + getRed() + ",g=" + getGreen() + ",b=" + getBlue() + "]";
    }

    /**
     * Converts a string to an integer and returns the
     * specified color. This method handles string formats that
     * are used to represent octal and hexidecimal numbers.
     * @param      nm a string that represents
     *                            a color as a 24-bit integer.
     * @return     the new color
     * @see        java.lang.Integer#decode
     * @exception  NumberFormatException  if the specified string cannot
     *                      be interpreted as a decimal,
     *                      octal, or hexidecimal integer.
     * @since      JDK1.1
     */
    public static Color decode(String nm) throws NumberFormatException {
        Integer intval = Integer.decode(nm);
        int i = intval.intValue();
        return new Color((i >> 16) & 0xFF, (i >> 8) & 0xFF, i & 0xFF);
    }

    /**
     * Finds a color in the system properties.
     * <p>
     * The argument is treated as the name of a system property to
     * be obtained. The string value of this property is then interpreted
     * as an integer which is then converted to a color.
     * <p>
     * If the specified property is not found, or could not be parsed as
     * an integer, then <code>null</code> is returned.
     * @param    nm the name of the color property
     * @return   the color value of the property.
     * @see      java.lang.System#getProperty(java.lang.String)
     * @see      java.lang.Integer#getInteger(java.lang.String)
     * @see      java.awt.Color#Color(int)
     * @since    JDK1.0
     */
    public static Color getColor(String nm) {
        return getColor(nm, null);
    }

    /**
     * Finds a color in the system properties.
     * <p>
     * The first argument is treated as the name of a system property to
     * be obtained. The string value of this property is then interpreted
     * as an integer which is then converted to a color.
     * <p>
     * If the specified property is not found, or cannot be parsed as
     * an integer, then the color specified by the second argument is
     * returned instead.
     * @param    nm the name of the color property
     * @param    v    the default color value.
     * @return   the color value of the property.
     * @see      java.lang.System#getProperty(java.lang.String)
     * @see      java.lang.Integer#getInteger(java.lang.String)
     * @see      java.awt.Color#Color(int)
     * @since    JDK1.0
     */
    public static Color getColor(String nm, Color v) {
        Integer intval = Integer.getInteger(nm);
        if (intval == null) {
            return v;
        }
        int i = intval.intValue();
        return new Color((i >> 16) & 0xFF, (i >> 8) & 0xFF, i & 0xFF);
    }

    /**
     * Finds a color in the system properties.
     * <p>
     * The first argument is treated as the name of a system property to
     * be obtained. The string value of this property is then interpreted
     * as an integer which is then converted to a color.
     * <p>
     * If the specified property is not found, or could not be parsed as
     * an integer, then the integer value <code>v</code> is used instead,
     * and is converted to a color.
     * @param    nm  the name of the color property.
     * @param    v   the default color value, as an integer.
     * @return   the color value of the property.
     * @see      java.lang.System#getProperty(java.lang.String)
     * @see      java.lang.Integer#getInteger(java.lang.String)
     * @see      java.awt.Color#Color(int)
     * @since    JDK1.0
     */
    public static Color getColor(String nm, int v) {
        Integer intval = Integer.getInteger(nm);
        int i = (intval != null) ? intval.intValue() : v;
        return new Color((i >> 16) & 0xFF, (i >> 8) & 0xFF, (i >> 0) & 0xFF);
    }

    /**
     * Converts the components of a color, as specified by the HSB
     * model, to an equivalent set of values for the RGB model.
     * <p>
     * The integer that is returned by <code>HSBtoRGB</code> encodes the
     * value of a color in bits 0&endash;23 of an integer value, the same
     * format used by the method <code>getRGB</code>. This integer can be
     * supplied as an argument to the <code>Color</code> constructor that
     * takes a single integer argument.
     * @param     hue   the hue component of the color.
     * @param     saturation   the saturation of the color.
     * @param     brightness   the brightness of the color.
     * @return    the RGB value of the color with the indicated hue,
     *                            saturation, and brightness.
     * @see       java.awt.Color#getRGB()
     * @see       java.awt.Color#Color(int)
     * @since     JDK1.0
     */
    public static int HSBtoRGB(float hue, float saturation, float brightness) {
        int r = 0, g = 0, b = 0;
        if (saturation == 0) {
            r = g = b = (int) (brightness * 255);
        } else {
            double h = (hue - Math.floor(hue)) * 6.0;
            double f = h - java.lang.Math.floor(h);
            double p = brightness * (1.0 - saturation);
            double q = brightness * (1.0 - saturation * f);
            double t = brightness * (1.0 - (saturation * (1.0 - f)));
            switch ((int) h) {
            case 0:
                r = (int) (brightness * 255);
                g = (int) (t * 255);
                b = (int) (p * 255);
                break;

            case 1:
                r = (int) (q * 255);
                g = (int) (brightness * 255);
                b = (int) (p * 255);
                break;

            case 2:
                r = (int) (p * 255);
                g = (int) (brightness * 255);
                b = (int) (t * 255);
                break;

            case 3:
                r = (int) (p * 255);
                g = (int) (q * 255);
                b = (int) (brightness * 255);
                break;

            case 4:
                r = (int) (t * 255);
                g = (int) (p * 255);
                b = (int) (brightness * 255);
                break;

            case 5:
                r = (int) (brightness * 255);
                g = (int) (p * 255);
                b = (int) (q * 255);
                break;
            }
        }
        return 0xff000000 | (r << 16) | (g << 8) | (b << 0);
    }

    /**
     * Converts the components of a color, as specified by the RGB
     * model, to an equivalent set of values for hue, saturation, and
     * brightness, the three components of the HSB model.
     * <p>
     * If the <code>hsbvals</code> argument is <code>null</code>, then a
     * new array is allocated to return the result. Otherwise, the method
     * returns the array <code>hsbvals</code>, with the values put into
     * that array.
     * @param     r   the red component of the color.
     * @param     g   the green component of the color.
     * @param     b   the blue component of the color.
     * @param     hsbvals  the array to be used to return the
     *                     three HSB values, or <code>null</code>.
     * @return    an array of three elements containing the hue, saturation,
     *                     and brightness (in that order), of the color with
     *                     the indicated red, green, and blue components.
     * @see       java.awt.Color#getRGB()
     * @see       java.awt.Color#Color(int)
     * @since     JDK1.0
     */
    public static float[] RGBtoHSB(int r, int g, int b, float[] hsbvals) {
        float hue, saturation, brightness;
        if (hsbvals == null) {
            hsbvals = new float[3];
        }
        int cmax = (r > g) ? r : g;
        if (b > cmax) cmax = b;
        int cmin = (r < g) ? r : g;
        if (b < cmin) cmin = b;
        brightness = ((float) cmax) / 255.0f;
        if (cmax != 0)
            saturation = ((float) (cmax - cmin)) / ((float) cmax);
        else
            saturation = 0;
        if (saturation == 0)
            hue = 0;
        else {
            float redc = ((float) (cmax - r)) / ((float) (cmax - cmin));
            float greenc = ((float) (cmax - g)) / ((float) (cmax - cmin));
            float bluec = ((float) (cmax - b)) / ((float) (cmax - cmin));
            if (r == cmax)
                hue = bluec - greenc;
            else if (g == cmax)
                hue = 2.0f + redc - bluec;
            else
                hue = 4.0f + greenc - redc;
            hue = hue / 6.0f;
            if (hue < 0)
                hue = hue + 1.0f;
        }
        hsbvals[0] = hue;
        hsbvals[1] = saturation;
        hsbvals[2] = brightness;
        return hsbvals;
    }

    /**
     * Creates a <code>Color</code> object based on values supplied
     * for the HSB color model.
     * <p>
     * Each of the three components should be a floating-point
     * value between zero and one (a number in the range
     * <code>0.0</code>&nbsp;&le;&nbsp;<code>h</code>, <code>s</code>,
     * <code>b</code>&nbsp;&le;&nbsp;<code>1.0). </code>
     * @param  h   the hue component.
     * @param  s   the saturation of the color.
     * @param  b   the brightness of the color.
     * @return  a <code>Color</code> object with the specified hue,
     *                                 saturation, and brightness.
     * @since   JDK1.0
     */
    public static Color getHSBColor(float h, float s, float b) {
        return new Color(HSBtoRGB(h, s, b));
    }

    /**
     * Returns a <code>float</code> array containing the color and alpha
     * components of the <code>Color</code>, as represented in the default
     * sRGB color space.
     * If <code>compArray</code> is <code>null</code>, an array of length
     * 4 is created for the return value.  Otherwise,
     * <code>compArray</code> must have length 4 or greater,
     * and it is filled in with the components and returned.
     * @param compArray an array that this method fills with
     *			color and alpha components and returns
     * @return the RGBA components in a <code>float</code> array.
     */
    public float[] getRGBComponents(float[] compArray) {
        float[] f;
        if (compArray == null) {
            f = new float[4];
        } else {
            f = compArray;
        }
        if (frgbvalue == null) {
            f[0] = ((float) getRed()) / 255f;
            f[1] = ((float) getGreen()) / 255f;
            f[2] = ((float) getBlue()) / 255f;
            f[3] = ((float) getAlpha()) / 255f;
        } else {
            f[0] = frgbvalue[0];
            f[1] = frgbvalue[1];
            f[2] = frgbvalue[2];
            f[3] = falpha;
        }
        return f;
    }

    /**
     * Returns a <code>float</code> array containing only the color
     * components of the <code>Color</code>, in the default sRGB color
     * space.  If <code>compArray</code> is <code>null</code>, an array of
     * length 3 is created for the return value.  Otherwise,
     * <code>compArray</code> must have length 3 or greater, and it is
     * filled in with the components and returned.
     * @param compArray an array that this method fills with color
     *		components and returns
     * @return the RGB components in a <code>float</code> array.
     */
    public float[] getRGBColorComponents(float[] compArray) {
        float[] f;
        if (compArray == null) {
            f = new float[3];
        } else {
            f = compArray;
        }
        if (frgbvalue == null) {
            f[0] = ((float) getRed()) / 255f;
            f[1] = ((float) getGreen()) / 255f;
            f[2] = ((float) getBlue()) / 255f;
        } else {
            f[0] = frgbvalue[0];
            f[1] = frgbvalue[1];
            f[2] = frgbvalue[2];
        }
        return f;
    }

    /**
     * Returns a <code>float</code> array containing the color and alpha
     * components of the <code>Color</code>, in the
     * <code>ColorSpace</code> of the <code>Color</code>.
     * If <code>compArray</code> is <code>null</code>, an array with
     * length equal to the number of components in the associated
     * <code>ColorSpace</code> plus one is created for
     * the return value.  Otherwise, <code>compArray</code> must have at
     * least this length and it is filled in with the components and
     * returned.
     * @param compArray an array that this method fills with the color and
     *		alpha components of this <code>Color</code> in its
     *		<code>ColorSpace</code> and returns
     * @return the color and alpha components in a <code>float</code>
     * 		array.
     */
    public float[] getComponents(float[] compArray) {
        if (fvalue == null)
            return getRGBComponents(compArray);
        float[] f;
        int n = fvalue.length;
        if (compArray == null) {
            f = new float[n + 1];
        } else {
            f = compArray;
        }
        for (int i = 0; i < n; i++) {
            f[i] = fvalue[i];
        }
        f[n] = falpha;
        return f;
    }

    /**
     * Returns a <code>float</code> array containing only the color
     * components of the <code>Color</code>, in the
     * <code>ColorSpace</code> of the <code>Color</code>.
     * If <code>compArray</code> is <code>null</code>, an array with
     * length equal to the number of components in the associated
     * <code>ColorSpace</code> is created for
     * the return value.  Otherwise, <code>compArray</code> must have at
     * least this length and it is filled in with the components and
     * returned.
     * @param compArray an array that this method fills with the color
     *		components of this <code>Color</code> in its
     *		<code>ColorSpace</code> and returns
     * @return the color components in a <code>float</code> array.
     */
    public float[] getColorComponents(float[] compArray) {
        if (fvalue == null)
            return getRGBColorComponents(compArray);
        float[] f;
        int n = fvalue.length;
        if (compArray == null) {
            f = new float[n];
        } else {
            f = compArray;
        }
        for (int i = 0; i < n; i++) {
            f[i] = fvalue[i];
        }
        return f;
    }

    /**
     * Returns the transparency mode for this <code>Color</code>.  This is
     * required to implement the <code>Paint</code> interface.
     * @return this <code>Color</code> object's transparency mode.
     * @see Paint
     * @see Transparency
     * @see #createContext
     */
    public int getTransparency() {
        int alpha = getAlpha();
        if (alpha == 0xff) {
            return Transparency.OPAQUE;
        } else if (alpha == 0) {
            return Transparency.BITMASK;
        } else {
            return Transparency.TRANSLUCENT;
        }
    }

    /**
     * Returns the <code>ColorSpace</code> of this <code>Color</code>.
     * @return this <code>Color</code> object's <code>ColorSpace</code>.
     */
    public ColorSpace getColorSpace() {
        if (cs == null) {
            cs = ColorSpace.getInstance(ColorSpace.CS_sRGB);
        }
        return cs;
    }
}
