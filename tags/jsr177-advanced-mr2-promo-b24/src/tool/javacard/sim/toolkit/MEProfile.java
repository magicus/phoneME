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

//-----------------------------------------------------------------------------
// PACKAGE DEFINITION
//-----------------------------------------------------------------------------
package sim.toolkit;

//-----------------------------------------------------------------------------
// IMPORTS
//-----------------------------------------------------------------------------

/**
 *
 * The <code>MEProfile</code> class contains methods to question the handset
 * profile, regarding the SIM Application Toolkit and supposing that this
 * profile has been set by the <b>Terminal Profile</b> APDU command.
 * The following table gives the index value according to the facility to
 * check. This class contains only static methods, no instance of this class is
 * necessary.<p>
 *
 * <br><br><Table Border="1" Cellpadding="3"><tr bgcolor=#c0c0c0 align=center>
 *     <td> <b>Facility</b>                                  </td>
 * <td> <b>Index</b> </td></tr>
 * <tr><td> Profile download                                 </td>
 * <td>    0         </td></tr>
 * <tr><td> SMS-PP data download                             </td>
 * <td>    1         </td></tr>
 * <tr><td> Cell Broadcast data download                     </td>
 * <td>    2         </td></tr>
 * <tr><td> Menu selection                                   </td>
 * <td>    3         </td></tr>
 * <tr><td> '9E xx' response code for SIM data download error</td>
 * <td>    4         </td></tr>
 * <tr><td> Timer Expiration                                 </td>
 * <td>    5         </td></tr>
 * <tr><td> USSD string data object supported in Call Control</td>
 * <td>    6         </td></tr>
 * <tr><td> Envelope Call Control sent during auto. redial   </td>
 * <td>    7         </td></tr>
 * <tr><td> command result                                   </td>
 * <td>    8         </td></tr>
 * <tr><td> Call Control by SIM                              </td>
 * <td>    9         </td></tr>
 * <tr><td> Cell identity included in Call Control by SIM    </td>
 * <td>    10        </td></tr>
 * <tr><td> MO short message control by SIM                  </td>
 * <td>    11        </td></tr>
 * <tr><td> Handling of the alpha identifier, user indication</td>
 * <td>    12        </td></tr>
 * <tr><td> UCS2 Entry supported                             </td>
 * <td>    13        </td></tr>
 * <tr><td> UCS2 Display supported                           </td>
 * <td>    14        </td></tr>
 * <tr><td> Display of the extension Text                    </td>
 * <td>    15        </td></tr>
 * <tr><td> Proactive SIM: Display Text                      </td>
 * <td>    16        </td></tr>
 * <tr><td> Proactive SIM: Get Inkey                         </td>
 * <td>    17        </td></tr>
 * <tr><td> Proactive SIM: Get Input                         </td>
 * <td>    18        </td></tr>
 * <tr><td> Proactive SIM: More Time                         </td>
 * <td>    19        </td></tr>
 * <tr><td> Proactive SIM: Play Tone                         </td>
 * <td>    20        </td></tr>
 * <tr><td> Proactive SIM: Poll Interval                     </td>
 * <td>    21        </td></tr>
 * <tr><td> Proactive SIM: Polling Off                       </td>
 * <td>    22        </td></tr>
 * <tr><td> Proactive SIM: Refresh                           </td>
 * <td>    23        </td></tr>
 * <tr><td> Proactive SIM: Select Item                       </td>
 * <td>    24        </td></tr>
 * <tr><td> Proactive SIM: Send Short Message                </td>
 * <td>    25        </td></tr>
 * <tr><td> Proactive SIM: Send SS                           </td>
 * <td>    26        </td></tr>
 * <tr><td> Proactive SIM: Send USSD                         </td>
 * <td>    27        </td></tr>
 * <tr><td> Proactive SIM: Set Up Call                       </td>
 * <td>    28        </td></tr>
 * <tr><td> Proactive SIM: Set Up Menu                       </td>
 * <td>    29        </td></tr>
 * <tr><td> Proactive SIM: Provide Local Information         </td>
 * <td>    30        </td></tr>
 * <tr><td> Proactive SIM: Provide Local Information (NMR)   </td>
 * <td>    31        </td></tr>
 * <tr><td> Proactive SIM: Set Up Event List                 </td>
 * <td>    32        </td></tr>
 * <tr><td> Event: MT call                                   </td>
 * <td>    33        </td></tr>
 * <tr><td> Event: Call connected                            </td>
 * <td>    34        </td></tr>
 * <tr><td> Event: Call disconnected                         </td>
 * <td>    35        </td></tr>
 * <tr><td> Event: Location status                           </td>
 * <td>    36        </td></tr>
 * <tr><td> Event: User activity                             </td>
 * <td>    37        </td></tr>
 * <tr><td> Event: Idle screen available                     </td>
 * <td>    38        </td></tr>
 * <tr><td> Event: Card reader status                        </td>
 * <td>    39        </td></tr>
 * <tr><td> Event: Language selection                        </td>
 * <td>    40        </td></tr>
 * <tr><td> Event: Browser termination                       </td>
 * <td>    41        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    42        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    43        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    44        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    45        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    46        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    47        </td></tr>
 * <tr><td> Proactive SIM: Power ON Card                     </td>
 * <td>    48        </td></tr>
 * <tr><td> Proactive SIM: Power OFF Card                    </td>
 * <td>    49        </td></tr>
 * <tr><td> Proactive SIM: Perform Card APDU                 </td>
 * <td>    50        </td></tr>
 * <tr><td> Proactive SIM: Get Reader Status (reader status) </td>
 * <td>    51        </td></tr>
 * <tr><td> Proactive SIM: Get Reader Status (reader ident.) </td>
 * <td>    52        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    53        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    54        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    55        </td></tr>
 * <tr><td> Proactive SIM: Timer Management (start, stop)    </td>
 * <td>    56        </td></tr>
 * <tr><td> Proactive SIM: Timer Management (get cur. value) </td>
 * <td>    57        </td></tr>
 * <tr><td> Proactive SIM: Provide Local Info (date,time...) </td>
 * <td>    58        </td></tr>
 * <tr><td> Binary choice in Get Inkey                       </td>
 * <td>    59        </td></tr>
 * <tr><td> Set Up Idle Mode Text                            </td>
 * <td>    60        </td></tr>
 * <tr><td> Run AT Command                                   </td>
 * <td>    61        </td></tr>
 * <tr><td> 2nd Alpha Id in Set Up Call                      </td>
 * <td>    62        </td></tr>
 * <tr><td> 2nd Capability configuration par. in Call Control</td>
 * <td>    63        </td></tr>
 * <tr><td> Sustained Display Text                           </td>
 * <td>    64        </td></tr>
 * <tr><td> Send DTMF                                        </td>
 * <td>    65        </td></tr>
 * <tr><td> Proactive SIM: Provide Local Info. (BCCH)        </td>
 * <td>    66        </td></tr>
 * <tr><td> Proactive SIM: Provide Local Info. (language)    </td>
 * <td>    67        </td></tr>
 * <tr><td> Proactive SIM: Provide Local Info. (Timing Adv.) </td>
 * <td>    68        </td></tr>
 * <tr><td> Proactive SIM: Language Notification             </td>
 * <td>    69        </td></tr>
 * <tr><td> Proactive SIM: Launch Browser                    </td>
 * <td>    70        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    71        </td></tr>
 * <tr><td> Soft keys support for Select Item                </td>
 * <td>    72        </td></tr>
 * <tr><td> Soft keys support for Set Up Menu                </td>
 * <td>    73        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    74        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    75        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    76        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    77        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    78        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    79        </td></tr>
 * <tr><td> Maximum number of softkeys available (b0)        </td>
 * <td>    80        </td></tr>
 * <tr><td> Maximum number of softkeys available (b1)        </td>
 * <td>    81        </td></tr>
 * <tr><td> Maximum number of softkeys available (b2)        </td>
 * <td>    82        </td></tr>
 * <tr><td> Maximum number of softkeys available (b3)        </td>
 * <td>    83        </td></tr>
 * <tr><td> Maximum number of softkeys available (b4)        </td>
 * <td>    84        </td></tr>
 * <tr><td> Maximum number of softkeys available (b5)        </td>
 * <td>    85        </td></tr>
 * <tr><td> Maximum number of softkeys available (b6)        </td>
 * <td>    86        </td></tr>
 * <tr><td> Maximum number of softkeys available (b7)        </td>
 * <td>    87        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    88        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    89        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    90        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    91        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    92        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    93        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    94        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    95        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    96        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    97        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    98        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    99        </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    100       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    101       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    102       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    103       </td></tr>
 * <tr><td> Nb of characters down ME display (b0)            </td>
 * <td>    104       </td></tr>
 * <tr><td> Nb of characters down ME display (b1)            </td>
 * <td>    105       </td></tr>
 * <tr><td> Nb of characters down ME display (b2)            </td>
 * <td>    106       </td></tr>
 * <tr><td> Nb of characters down ME display (b3)            </td>
 * <td>    107       </td></tr>
 * <tr><td> Nb of characters down ME display (b4)            </td>
 * <td>    108       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    109       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    110       </td></tr>
 * <tr><td> Screen Sizing parameters supported               </td>
 * <td>    111       </td></tr>
 * <tr><td> Nb of characters accross ME display (b0)         </td>
 * <td>    112       </td></tr>
 * <tr><td> Nb of characters accross ME display (b1)         </td>
 * <td>    113       </td></tr>
 * <tr><td> Nb of characters accross ME display (b2)         </td>
 * <td>    114       </td></tr>
 * <tr><td> Nb of characters accross ME display (b3)         </td>
 * <td>    115       </td></tr>
 * <tr><td> Nb of characters accross ME display (b4)         </td>
 * <td>    116       </td></tr>
 * <tr><td> Nb of characters accross ME display (b5)         </td>
 * <td>    117       </td></tr>
 * <tr><td> Nb of characters accross ME display (b6)         </td>
 * <td>    118       </td></tr>
 * <tr><td> Variable size fonts supported                    </td>
 * <td>    119       </td></tr>
 * <tr><td> Display can be resized                           </td>
 * <td>    120       </td></tr>
 * <tr><td> Text Wrapping supported                          </td>
 * <td>    121       </td></tr>
 * <tr><td> Text Scrolling supported                         </td>
 * <td>    122       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    123       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    124       </td></tr>
 * <tr><td> Width reduction when in a menu (b0)              </td>
 * <td>    125       </td></tr>
 * <tr><td> Width reduction when in a menu (b1)              </td>
 * <td>    126       </td></tr>
 * <tr><td> Width reduction when in a menu (b2)              </td>
 * <td>    127       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    128       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    129       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    130       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    131       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    132       </td></tr>
 * <tr><td> RFU                                              </td>
 * <td>    133       </td></tr>
 * </Table><br>
 *
 * Example of use in a standard Toolkit applet:<pre><code>
 * private static final byte PROFILE_USSD = (byte)27;
 *
 * if (MEProfile.check(PROFILE_USSD) == true) {
 *     // USSD available on the handset
 *     sendTheRequest();
 * } else {
 *     // abort applet
 *     return;
 * }
 * </code></pre>
 *
 * @version 8.3.0
 */
public final class MEProfile {

    // ------------------------------- Constructors ---------------------------
    /**
     * constructor
     */
    private MEProfile() {
    }


    // ------------------------------- Public methods -------------------------
    /**
     * Checks a facility in the handset profile.
     *
     * @param index the number of the facility to check, according to
     * the table above.
     *
     * @return true if the facility is supported, false otherwise
     *
     * @exception ToolkitException with the following reason codes:
     *      <ul> <li>ME_PROFILE_NOT_AVAILABLE if Terminal Profile data
     *      are not available</ul> 
     */
    public static boolean check(byte index) throws ToolkitException {
		return false;
    }

    /**
     * Checks a set of facilities in the handset profile.
     * The method checks all the facilities corresponding to bits set to 1 in
     * the mask buffer.
     *
     * <p> Notes:<ul> <li><em>If </em><code>offset</code><em> or
     * </em><code>length</code><em> parameter is negative an
     * </em><code>ArrayIndexOutOfBoundsException</code> <em> exception
     * is thrown and no check is performed.</em> <li><em>If
     * </em><code>offset+length</code><em> is greater than
     * </em><code>mask.length</code><em>, the length of the
     * </em><code>mask</code><em> array an
     * </em><code>ArrayIndexOutOfBoundsException</code><em> exception
     * is thrown and no check is performed.</em> </ul>
     *
     * @param mask a byte array containing the mask to compare with the profile
     * @param offset the starting offset of the mask in the byte array
     * @param length the length of the mask (at least 1)
     *
     * @return true if the set of facilities is supported, false
     * otherwise. If <code>length</code> is equal to <code>0</code>,
     * true is returned.
     *
     * @exception NullPointerException if <code>mask</code> is <code>null</code>
     * @exception ArrayIndexOutOfBoundsException if check would cause
     * access of data outside mask array bounds
     * @exception ToolkitException with the following reason codes:
     *      <ul> <li>ME_PROFILE_NOT_AVAILABLE if Terminal Profile data
     *      are not available</ul> 
     */
    public static boolean check(byte[] mask,
                                short offset,
                                short length) 
	throws NullPointerException,
	       ArrayIndexOutOfBoundsException,
	       ToolkitException {
        return false;
    }

    /**
     * Checks a facility in the handset profile.
     *
     * @param index the number of the facility to check, according to
     * the table above.
     *
     * @return true if the facility is supported, false otherwise
     *
     * @exception ToolkitException with the following reason codes:
     *      <ul> <li>ME_PROFILE_NOT_AVAILABLE if Terminal Profile data
     *      are not available</ul> 
     */
    public static boolean check(short index) throws ToolkitException {
        return false;
    }

    /**
     * Returns the binary value of a parameter, delimited by two
     * indexes, from the handset profile.
     *
     * @param indexMSB index of the Most Significant Bit of the
     * handset profile .
     * @param indexLSB index of the Lowest Significant Bit of the
     * handset profile .
     *
     * @return binary value of the data field indicated in the handset profile.
     *
     * @exception ToolkitException with the following reason codes:
     *      <ul> <li>ME_PROFILE_NOT_AVAILABLE if Terminal Profile data
     *      are not available <li>BAD_INPUT_PARAMETER if (indexMSB >
     *      indexLSB +16) or (indexMSB < indexLSB) or (indexMSB < 0)
     *      or (indexLSB < 0) </ul> 
     */
    public static short getValue(short indexMSB, short indexLSB)
	throws ToolkitException {
        return 0;
    }

    /**
     * Copies a part of the handset profile in a buffer.
     *
     * <p> Notes:<ul> <li><em>If </em><code>dstOffset</code><em> or
     * </em><code>dstLength</code><em> parameter is negative an
     * </em><code>ArrayIndexOutOfBoundsException</code> <em> exception
     * is thrown and no copy is performed.</em> <li><em>If
     * </em><code>dstOffset+dstLength</code><em> is greater than
     * </em><code>dstBuffer.length</code><em>, the length of the
     * </em><code>dstBuffer</code><em> array an
     * </em><code>ArrayIndexOutOfBoundsException</code><em> exception
     * is thrown and no copy is performed.</em> </ul>
     *
     * @param startOffset offset of the handset profile first byte to be copied 
     * @param dstBuffer destination byte array
     * @param dstOffset offset within destination byte array to start copy into
     * @param dstLength byte length to be copy
     *
     * @return dstOffset + dstLength 
     *
     * @exception ArrayIndexOutOfBoundsException if copy would cause
     * access of data outside array bounds
     * @exception NullPointerException if <code>dstBuffer<code> is null
     * @exception ToolkitException with the following reason codes:
     *      <ul> <li>ME_PROFILE_NOT_AVAILABLE if Terminal Profile data
     *      are not available</ul> 
     */
    public static short copy(short  startOffset, byte[] dstBuffer,
			     short dstOffset, short dstLength) 
        throws ArrayIndexOutOfBoundsException, 
	       NullPointerException, ToolkitException {
        return 0;
    }
}
