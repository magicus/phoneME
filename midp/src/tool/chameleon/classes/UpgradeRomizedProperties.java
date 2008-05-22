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


/**
 * This tool converts RomizedProperties.java from the old format,
 * used in JTWI-HI 1.1.2 release, where properties values are indexed 
 * via properties names (strings) and therefore stored in hashtable 
 * internally by Chameleon, to the new format, where properties values 
 * are indexed by integer constants and stored in arrays.
 * 
 * Apart from changing the way properties are stored and accessed, this 
 * tool also does following conversions:
 *
 * - Assigns default values to the properties which are used by Chameleon,
 *   but aren't present in RomizedProperties.java being converted. Old 
 *   format assumes that if property isn't present in RomizedProperties.java,
 *   then its value is property's name for a string valued property and -1
 *   for integer valued property. Therefore, old format allows omitting 
 *   some properties from RomizedProperties.java. New format forbids that. 
 *   All the properties used by Chameleon must be explicitly given values 
 *   in RomizedProperties.java.
 * - Converts sequence of integer valued properties used by soft button skin
 *   element into a new string valued property, where this new property's 
 *   value consists of values from the sequence separated by comma. 
 *   One example of such sequence is softbtn.button_align_x0, 
 *   softbtn.button_align_x1, softbtn.button_align_x2 and so on properties.
 *
 * Usage:
 *  javac UpgradeRomizedProperties.java
 *  java -cp . UpgradeRomizedProperties <infile> <outfile>
 *
 * Where,
 *  <infile> is path to RomizedProperties.java to convert,
 *  <outfile> is name of converted RomizedProperties.java
 * 
 */

import java.lang.*;
import java.lang.reflect.*;
import java.util.*;
import java.io.*;

/**
 * Represents skin property
 */
class SkinProperty {
    /**
     * Properties types
     */
    
    /** Integer property */
    public static final int INT_T = 0;
    
    /** String property */
    public static final int STRING_T = 1;

    /** Font property */
    public static final int FONT_T = 2;

    /** Image property */ 
    public static final int IMAGE_T = 3;

    /** Composite image property */ 
    public static final int C_IMAGE_T = 4;

    /** Integers sequence property */
    public static final int INT_SEQ_T = 5;

    /** Total numbers of types */
    public static final int TOTAL_T = 6;

    /** Property's name as used in RomizedProperties.java */
    String name;

    /** Property's value */
    String value;

    /** 
     * Name of the integer constant that is used for getting property 
     * value in new format instead of property's name used in old format
     */
    String id;

    /**
     * true, if this property is newly introduced, i.e. isn't used by 
     * RomizedProperties.java. New properties get its values as the 
     * result of conversion of some other properties.
     */
    boolean isNew;

    /** Property type */
    int type;

    /** Number of pieces in composite image */
    int totalPieces;

    /**
     * Constructor
     */
    SkinProperty(String name, String defaultValue, 
            String id, int type) {
        this.name = name;
        this.value = defaultValue;
        this.id = id;
        this.isNew = false;
        this.type = type;
        this.totalPieces = 1;
    }

    /**
     * Constructor
     */
    SkinProperty(String name, String defaultValue, 
            String id, int type, boolean isNew) {
        this.name = name;
        this.value = defaultValue;
        this.id = id;
        this.isNew = isNew;
        this.type = type;
        this.totalPieces = 1;
    }

    /**
     * All the properties used by Chameleon
     */
    static SkinProperty[] properties = {
        new SkinProperty("screen.text_orient", "-1", "SCREEN_TEXT_ORIENT", INT_T), 
        new SkinProperty("screen.pad_form_items", "-1", "SCREEN_PAD_FORM_ITEMS", INT_T), 
        new SkinProperty("screen.pad_label_vert", "-1", "SCREEN_PAD_LABEL_VERT", INT_T), 
        new SkinProperty("screen.pad_label_horiz", "-1", "SCREEN_PAD_LABEL_HORIZ", INT_T), 
        new SkinProperty("screen.color_bg", "-1", "SCREEN_COLOR_BG", INT_T), 
        new SkinProperty("screen.color_hs_bg", "-1", "SCREEN_COLOR_HS_BG", INT_T), 
        new SkinProperty("screen.color_fg", "-1", "SCREEN_COLOR_FG", INT_T), 
        new SkinProperty("screen.color_bg_hl", "-1", "SCREEN_COLOR_BG_HL", INT_T), 
        new SkinProperty("screen.color_fg_hl", "-1", "SCREEN_COLOR_FG_HL", INT_T), 
        new SkinProperty("screen.color_border", "-1", "SCREEN_COLOR_BORDER", INT_T), 
        new SkinProperty("screen.color_border_hl", "-1", "SCREEN_COLOR_BORDER_HL", INT_T), 
        new SkinProperty("screen.color_traverse_ind", "-1", "SCREEN_COLOR_TRAVERSE_IND", INT_T), 
        new SkinProperty("screen.border_style", "-1", "SCREEN_BORDER_STYLE", INT_T), 
        new SkinProperty("screen.scroll_amount", "-1", "SCREEN_SCROLL_AMOUNT", INT_T), 
        new SkinProperty("screen.font_label", "500", "SCREEN_FONT_LABEL", FONT_T), 
        new SkinProperty("screen.font_input_text", "500", "SCREEN_FONT_INPUT_TEXT", FONT_T), 
        new SkinProperty("screen.font_static_text", "500", "SCREEN_FONT_STATIC_TEXT", FONT_T), 
        new SkinProperty("screen.image_wash", "screen.image_wash", "SCREEN_IMAGE_WASH", IMAGE_T), 
        new SkinProperty("screen.image_bg", "screen.image_bg", "SCREEN_IMAGE_BG", IMAGE_T), 
        new SkinProperty("screen.image_bg_w_title", "screen.image_bg_w_title", "SCREEN_IMAGE_BG_W_TITLE", C_IMAGE_T), 
        new SkinProperty("screen.image_bg_wo_title", "screen.image_bg_wo_title", "SCREEN_IMAGE_BG_WO_TITLE", C_IMAGE_T), 
        new SkinProperty("screen.image_hs_bg_tile", "screen.image_hs_bg_tile", "SCREEN_IMAGE_HS_BG_TILE", IMAGE_T), 
        new SkinProperty("screen.image_hs_bg_w_title", "screen.image_hs_bg_w_title", "SCREEN_IMAGE_HS_BG_W_TITLE", C_IMAGE_T), 
        new SkinProperty("screen.image_hs_bg_wo_title", "screen.image_hs_bg_wo_title", "SCREEN_IMAGE_HS_BG_WO_TITLE", C_IMAGE_T), 
        new SkinProperty("scroll.mode", "-1", "SCROLL_MODE", INT_T), 
        new SkinProperty("scroll.width", "-1", "SCROLL_WIDTH", INT_T), 
        new SkinProperty("scroll.color_bg", "-1", "SCROLL_COLOR_BG", INT_T), 
        new SkinProperty("scroll.color_fg", "-1", "SCROLL_COLOR_FG", INT_T), 
        new SkinProperty("scroll.color_frame", "-1", "SCROLL_COLOR_FRAME", INT_T), 
        new SkinProperty("scroll.color_up_arrow", "-1", "SCROLL_COLOR_UP_ARROW", INT_T), 
        new SkinProperty("scroll.color_dn_arrrow", "-1", "SCROLL_COLOR_DN_ARROW", INT_T), 
        new SkinProperty("scroll.image_bg", "scroll.image_bg", "SCROLL_IMAGE_BG", C_IMAGE_T), 
        new SkinProperty("scroll.image_fg", "scroll.image_fg", "SCROLL_IMAGE_FG", C_IMAGE_T), 
        new SkinProperty("scroll.image_up", "scroll.image_up", "SCROLL_IMAGE_UP", IMAGE_T), 
        new SkinProperty("scroll.image_dn", "scroll.image_dn", "SCROLL_IMAGE_DN", IMAGE_T), 
        new SkinProperty("scroll.color_au_bg", "-1", "SCROLL_COLOR_AU_BG", INT_T), 
        new SkinProperty("scroll.color_au_fg", "-1", "SCROLL_COLOR_AU_FG", INT_T), 
        new SkinProperty("scroll.image_au_bg", "scroll.image_au_bg", "SCROLL_IMAGE_AU_BG", C_IMAGE_T), 
        new SkinProperty("scroll.image_au_fg", "scroll.image_au_fg", "SCROLL_IMAGE_AU_FG", C_IMAGE_T), 
        new SkinProperty("scroll.image_au_up", "scroll.image_au_up", "SCROLL_IMAGE_AU_UP", IMAGE_T), 
        new SkinProperty("scroll.image_au_dn", "scroll.image_au_dn", "SCROLL_IMAGE_AU_DN", IMAGE_T), 
        new SkinProperty("softbtn.height", "-1", "SOFTBTN_HEIGHT", INT_T), 
        new SkinProperty("softbtn.num_buttons", "-1", "SOFTBTN_NUM_BUTTONS", INT_T), 
        new SkinProperty("softbtn.button_shd_align", "-1", "SOFTBTN_BUTTON_SHD_ALIGN", INT_T), 
        new SkinProperty("softbtn.color_fg", "-1", "SOFTBTN_COLOR_FG", INT_T), 
        new SkinProperty("softbtn.color_fg_shd", "-1", "SOFTBTN_COLOR_FG_SHD", INT_T), 
        new SkinProperty("softbtn.color_bg", "-1", "SOFTBTN_COLOR_BG", INT_T), 
        new SkinProperty("softbtn.color_mu_fg", "-1", "SOFTBTN_COLOR_MU_FG", INT_T), 
        new SkinProperty("softbtn.color_mu_fg_shd", "-1", "SOFTBTN_COLOR_MU_FG_SHD", INT_T), 
        new SkinProperty("softbtn.color_mu_bg", "-1", "SOFTBTN_COLOR_MU_BG", INT_T), 
        new SkinProperty("softbtn.color_au_fg", "-1", "SOFTBTN_COLOR_AU_FG", INT_T), 
        new SkinProperty("softbtn.color_au_fg_shd", "-1", "SOFTBTN_COLOR_AU_FG_SHD", INT_T), 
        new SkinProperty("softbtn.color_au_bg", "-1", "SOFTBTN_COLOR_AU_BG", INT_T), 
        new SkinProperty("softbtn.font", "500", "SOFTBTN_FONT", FONT_T), 
        new SkinProperty("softbtn.text_menucmd", "softbtn.text_menucmd", "SOFTBTN_TEXT_MENUCMD", STRING_T), 
        new SkinProperty("softbtn.text_backcmd", "softbtn.text_backcmd", "SOFTBTN_TEXT_BACKCMD", STRING_T), 
        new SkinProperty("softbtn.image_bg", "softbtn.image_bg", "SOFTBTN_IMAGE_BG", C_IMAGE_T), 
        new SkinProperty("softbtn.image_mu_bg", "softbtn.image_mu_bg", "SOFTBTN_IMAGE_MU_BG", C_IMAGE_T), 
        new SkinProperty("softbtn.image_au_bg", "softbtn.image_au_bg", "SOFTBTN_IMAGE_AU_BG", C_IMAGE_T), 
        new SkinProperty("ticker.height", "-1", "TICKER_HEIGHT", INT_T), 
        new SkinProperty("ticker.align", "-1", "TICKER_ALIGN", INT_T), 
        new SkinProperty("ticker.direction", "-1", "TICKER_DIRECTION", INT_T), 
        new SkinProperty("ticker.rate", "-1", "TICKER_RATE", INT_T), 
        new SkinProperty("ticker.speed", "-1", "TICKER_SPEED", INT_T), 
        new SkinProperty("ticker.text_anchor_y", "-1", "TICKER_TEXT_ANCHOR_Y", INT_T), 
        new SkinProperty("ticker.text_shd_align", "-1", "TICKER_TEXT_SHD_ALIGN", INT_T), 
        new SkinProperty("ticker.color_bg", "-1", "TICKER_COLOR_BG", INT_T), 
        new SkinProperty("ticker.color_fg", "-1", "TICKER_COLOR_FG", INT_T), 
        new SkinProperty("ticker.color_fg_shd", "-1", "TICKER_COLOR_FG_SHD", INT_T), 
        new SkinProperty("ticker.font", "500", "TICKER_FONT", FONT_T), 
        new SkinProperty("ticker.image_bg", "ticker.image_bg", "TICKER_IMAGE_BG", C_IMAGE_T), 
        new SkinProperty("ticker.image_au_bg", "ticker.image_au_bg", "TICKER_IMAGE_AU_BG", C_IMAGE_T), 

        new SkinProperty("pti.height", "-1", "PTI_HEIGHT", INT_T), 
        new SkinProperty("pti.margin", "-1", "PTI_MARGIN", INT_T), 
        new SkinProperty("pti.color_bg", "-1", "PTI_COLOR_BG", INT_T), 
        new SkinProperty("pti.color_fg", "-1", "PTI_COLOR_FG", INT_T), 
        new SkinProperty("pti.color_bg_hl", "-1", "PTI_COLOR_BG_HL", INT_T), 
        new SkinProperty("pti.color_fg_hl", "-1", "PTI_COLOR_FG_HL", INT_T), 
        new SkinProperty("pti.color_bdr", "-1", "PTI_COLOR_BDR", INT_T), 
        new SkinProperty("pti.font", "500", "PTI_FONT", FONT_T), 
        new SkinProperty("pti.image_bg", "pti.image_bg", "PTI_IMAGE_BG", C_IMAGE_T), 
        new SkinProperty("pti.left_arrow", "pti.left_arrow", "PTI_LEFT_ARROW", IMAGE_T), 
        new SkinProperty("pti.right_arrow", "pti.right_arrow", "PTI_RIGHT_ARROW", IMAGE_T), 

        new SkinProperty("inputmode.margin", "-1", "INPUT_MODE_MARGIN", INT_T), 
        new SkinProperty("inputmode.color_bg", "-1", "INPUT_MODE_COLOR_BG", INT_T), 
        new SkinProperty("inputmode.color_fg", "-1", "INPUT_MODE_COLOR_FG", INT_T), 
        new SkinProperty("inputmode.color_bdr", "-1", "INPUT_MODE_COLOR_BDR", INT_T), 
        new SkinProperty("inputmode.font", "500", "INPUT_MODE_FONT", FONT_T), 
        new SkinProperty("inputmode.image_bg", "inputmode.image_bg", "INPUT_MODE_IMAGE_BG", C_IMAGE_T), 

        new SkinProperty("title.height", "-1", "TITLE_HEIGHT", INT_T), 
        new SkinProperty("title.margin", "-1", "TITLE_MARGIN", INT_T), 
        new SkinProperty("title.text_align_x", "-1", "TITLE_TEXT_ALIGN_X", INT_T), 
        new SkinProperty("title.text_shd_align", "-1", "TITLE_TEXT_SHD_ALIGN", INT_T), 
        new SkinProperty("title.color_fg", "-1", "TITLE_COLOR_FG", INT_T), 
        new SkinProperty("title.color_fg_shd", "-1", "TITLE_COLOR_FG_SHD", INT_T), 
        new SkinProperty("title.color_bg", "-1", "TITLE_COLOR_BG", INT_T), 
        new SkinProperty("title.font", "500", "TITLE_FONT", FONT_T), 
        new SkinProperty("title.image_bg", "title.image_bg", "TITLE_IMAGE_BG", C_IMAGE_T), 
        new SkinProperty("alert.width", "-1", "ALERT_WIDTH", INT_T), 
        new SkinProperty("alert.height", "-1", "ALERT_HEIGHT", INT_T), 
        new SkinProperty("alert.align_x", "-1", "ALERT_ALIGN_X", INT_T), 
        new SkinProperty("alert.align_y", "-1", "ALERT_ALIGN_Y", INT_T), 
        new SkinProperty("alert.margin_h", "-1", "ALERT_MARGIN_H", INT_T), 
        new SkinProperty("alert.margin_v", "-1", "ALERT_MARGIN_V", INT_T), 
        new SkinProperty("alert.title_align", "-1", "ALERT_TITLE_ALIGN", INT_T), 
        new SkinProperty("alert.title_height", "-1", "ALERT_TITLE_HEIGHT", INT_T), 
        new SkinProperty("alert.title_margin", "-1", "ALERT_TITLE_MARGIN", INT_T), 
        new SkinProperty("alert.text_title_info", "alert.text_title_info", "ALERT_TEXT_TITLE_INFO", STRING_T), 
        new SkinProperty("alert.text_title_warn", "alert.text_title_warn", "ALERT_TEXT_TITLE_WARN", STRING_T), 
        new SkinProperty("alert.text_title_errr", "alert.text_title_errr", "ALERT_TEXT_TITLE_ERRR", STRING_T), 
        new SkinProperty("alert.text_title_alrm", "alert.text_title_alrm", "ALERT_TEXT_TITLE_ALRM", STRING_T), 
        new SkinProperty("alert.text_title_cnfm", "alert.text_title_cnfm", "ALERT_TEXT_TITLE_CNFM", STRING_T), 
        new SkinProperty("alert.pad_horiz", "-1", "ALERT_PAD_HORIZ", INT_T), 
        new SkinProperty("alert.pad_vert", "-1", "ALERT_PAD_VERT", INT_T), 
        new SkinProperty("alert.scroll_amount", "-1", "ALERT_SCROLL_AMOUNT", INT_T), 
        new SkinProperty("alert.timeout", "-1", "ALERT_TIMEOUT", INT_T), 
        new SkinProperty("alert.color_bg", "-1", "ALERT_COLOR_BG", INT_T), 
        new SkinProperty("alert.color_title", "-1", "ALERT_COLOR_TITLE", INT_T), 
        new SkinProperty("alert.color_fg", "-1", "ALERT_COLOR_FG", INT_T), 
        new SkinProperty("alert.font_title", "500", "ALERT_FONT_TITLE", FONT_T), 
        new SkinProperty("alert.font_text", "500", "ALERT_FONT_TEXT", FONT_T), 
        new SkinProperty("alert.image_bg", "alert.image_bg", "ALERT_IMAGE_BG", C_IMAGE_T), 
        new SkinProperty("alert.image_icon_info", "alert.image_icon_info", "ALERT_IMAGE_ICON_INFO", IMAGE_T), 
        new SkinProperty("alert.image_icon_warn", "alert.image_icon_warn", "ALERT_IMAGE_ICON_WARN", IMAGE_T), 
        new SkinProperty("alert.image_icon_errr", "alert.image_icon_errr", "ALERT_IMAGE_ICON_ERRR", IMAGE_T), 
        new SkinProperty("alert.image_icon_alrm", "alert.image_icon_alrm", "ALERT_IMAGE_ICON_ALRM", IMAGE_T), 
        new SkinProperty("alert.image_icon_cnfm", "alert.image_icon_cnfm", "ALERT_IMAGE_ICON_CNFM", IMAGE_T), 
        new SkinProperty("busycrsr.width", "-1", "BUSYCRSR_WIDTH", INT_T), 
        new SkinProperty("busycrsr.height", "-1", "BUSYCRSR_HEIGHT", INT_T), 
        new SkinProperty("busycrsr.num_frames", "-1", "BUSYCRSR_NUM_FRAMES", INT_T), 
        new SkinProperty("busycrsr.frame_x", "-1", "BUSYCRSR_FRAME_X", INT_T), 
        new SkinProperty("busycrsr.frame_y", "-1", "BUSYCRSR_FRAME_Y", INT_T), 
        new SkinProperty("busycrsr.frame_sequ", "busycrsr.frame_sequ", "BUSYCRSR_FRAME_SEQU", INT_SEQ_T), 
        new SkinProperty("busycrsr.image_bg", "busycrsr.image_bg", "BUSYCRSR_IMAGE_BG", IMAGE_T), 
        new SkinProperty("busycrsr.image_frame", "busycrsr.image_frame", "BUSYCRSR_IMAGE_FRAME", C_IMAGE_T), 
        new SkinProperty("choice.width_image", "-1", "CHOICE_WIDTH_IMAGE", INT_T), 
        new SkinProperty("choice.height_image", "-1", "CHOICE_HEIGHT_IMAGE", INT_T), 
        new SkinProperty("choice.width_scroll", "-1", "CHOICE_WIDTH_SCROLL", INT_T), 
        new SkinProperty("choice.width_thumb", "-1", "CHOICE_WIDTH_THUMB", INT_T), 
        new SkinProperty("choice.height_thumb", "-1", "CHOICE_HEIGHT_THUMB", INT_T), 
        new SkinProperty("choice.pad_h", "-1", "CHOICE_PAD_H", INT_T), 
        new SkinProperty("choice.pad_v", "-1", "CHOICE_PAD_V", INT_T), 
        new SkinProperty("choice.color_fg", "-1", "CHOICE_COLOR_FG", INT_T), 
        new SkinProperty("choice.color_bg", "-1", "CHOICE_COLOR_BG", INT_T), 
        new SkinProperty("choice.color_brdr", "-1", "CHOICE_COLOR_BRDR", INT_T), 
        new SkinProperty("choice.color_brdr_shd", "-1", "CHOICE_COLOR_BRDR_SHD", INT_T), 
        new SkinProperty("choice.color_scroll", "-1", "CHOICE_COLOR_SCROLL", INT_T), 
        new SkinProperty("choice.color_thumb", "-1", "CHOICE_COLOR_THUMB", INT_T), 
        new SkinProperty("choice.font", "500", "CHOICE_FONT", FONT_T), 
        new SkinProperty("choice.font_focus", "500", "CHOICE_FONT_FOCUS", FONT_T), 
        new SkinProperty("choice.image_radio", "choice.image_radio", "CHOICE_IMAGE_RADIO", C_IMAGE_T), 
        new SkinProperty("choice.image_chkbx", "choice.image_chkbx", "CHOICE_IMAGE_CHKBX", C_IMAGE_T), 
        new SkinProperty("choice.image_bg", "choice.image_bg", "CHOICE_IMAGE_BG", C_IMAGE_T), 
        new SkinProperty("choice.image_btn_bg", "choice.image_btn_bg", "CHOICE_IMAGE_BTN_BG", C_IMAGE_T), 
        new SkinProperty("choice.image_btn_icon", "choice.image_btn_icon", "CHOICE_IMAGE_BTN_ICON", IMAGE_T), 
        new SkinProperty("choice.image_popup_bg", "choice.image_popup_bg", "CHOICE_IMAGE_POPUP_BG", C_IMAGE_T), 
        new SkinProperty("dateeditor.height", "-1", "DATEEDITOR_HEIGHT", INT_T), 
        new SkinProperty("dateeditor.height_popups", "-1", "DATEEDITOR_HEIGHT_POPUPS", INT_T), 
        new SkinProperty("dateeditor.width_d", "-1", "DATEEDITOR_WIDTH_D", INT_T), 
        new SkinProperty("dateeditor.width_t", "-1", "DATEEDITOR_WIDTH_T", INT_T), 
        new SkinProperty("dateeditor.width_dt", "-1", "DATEEDITOR_WIDTH_DT", INT_T), 
        new SkinProperty("dateeditor.color_bg", "-1", "DATEEDITOR_COLOR_BG", INT_T), 
        new SkinProperty("dateeditor.color_popups_bg", "-1", "DATEEDITOR_COLOR_POPUPS_BG", INT_T), 
        new SkinProperty("dateeditor.color_brdr", "-1", "DATEEDITOR_COLOR_BRDR", INT_T), 
        new SkinProperty("dateeditor.color_trav_ind", "-1", "DATEEDITOR_COLOR_TRAV_IND", INT_T), 
        new SkinProperty("dateeditor.color_clk_lt", "-1", "DATEEDITOR_COLOR_CLK_LT", INT_T), 
        new SkinProperty("dateeditor.color_clk_dk", "-1", "DATEEDITOR_COLOR_CLK_DK", INT_T), 
        new SkinProperty("dateeditor.font_popups", "500", "DATEEDITOR_FONT_POPUPS", FONT_T), 
        new SkinProperty("dateeditor.image_bg", "dateeditor.image_bg", "DATEEDITOR_IMAGE_BG", C_IMAGE_T), 
        new SkinProperty("dateeditor.image_mon_bg", "dateeditor.image_mon_bg", "DATEEDITOR_IMAGE_MON_BG", IMAGE_T), 
        new SkinProperty("dateeditor.image_yr_bg", "dateeditor.image_yr_bg", "DATEEDITOR_IMAGE_YR_BG", IMAGE_T), 
        new SkinProperty("dateeditor.image_cal_bg", "dateeditor.image_cal_bg", "DATEEDITOR_IMAGE_CAL_BG", IMAGE_T), 
        new SkinProperty("dateeditor.image_dates", "dateeditor.image_dates", "DATEEDITOR_IMAGE_DATES", IMAGE_T),    
	new SkinProperty("dateeditor.image_days", "dateeditor.image_days", "DATEEDITOR_IMAGE_DAYS", IMAGE_T), 
        new SkinProperty("dateeditor.image_time_bg", "dateeditor.image_time_bg", "DATEEDITOR_IMAGE_TIME_BG", IMAGE_T), 
        new SkinProperty("dateeditor.image_radio", "dateeditor.image_radio", "DATEEDITOR_IMAGE_RADIO", C_IMAGE_T), 
        new SkinProperty("dateeditor.image_ampm", "dateeditor.image_ampm", "DATEEDITOR_IMAGE_AMPM", IMAGE_T), 
        new SkinProperty("dateeditor.image_clock_bg", "dateeditor.image_clock_bg", "DATEEDITOR_IMAGE_CLOCK_BG", IMAGE_T), 
        new SkinProperty("datefield.pad_h", "-1", "DATEFIELD_PAD_H", INT_T), 
        new SkinProperty("datefield.pad_v", "-1", "DATEFIELD_PAD_V", INT_T), 
        new SkinProperty("datefield.btn_brdr_w", "-1", "DATEFIELD_BTN_BRDR_W", INT_T), 
        new SkinProperty("datefield.font", "500", "DATEFIELD_FONT", FONT_T), 
        new SkinProperty("datefield.color_fg", "-1", "DATEFIELD_COLOR_FG", INT_T), 
        new SkinProperty("datefield.color_bg", "-1", "DATEFIELD_COLOR_BG", INT_T), 
        new SkinProperty("datefield.color_brdr", "-1", "DATEFIELD_COLOR_BRDR", INT_T), 
        new SkinProperty("datefield.color_brdr_lt", "-1", "DATEFIELD_COLOR_BRDR_LT", INT_T), 
        new SkinProperty("datefield.color_brdr_dk", "-1", "DATEFIELD_COLOR_BRDR_DK", INT_T), 
        new SkinProperty("datefield.color_brdr_shd", "-1", "DATEFIELD_COLOR_BRDR_SHD", INT_T), 
        new SkinProperty("datefield.image_bg", "datefield.image_bg", "DATEFIELD_IMAGE_BG", C_IMAGE_T), 
        new SkinProperty("datefield.image_btn_bg", "datefield.image_btn_bg", "DATEFIELD_IMAGE_BTN_BG", C_IMAGE_T), 
        new SkinProperty("datefield.image_icon_date", "datefield.image_icon_date", "DATEFIELD_IMAGE_ICON_DATE", IMAGE_T), 
        new SkinProperty("datefield.image_icon_time", "datefield.image_icon_time", "DATEFIELD_IMAGE_ICON_TIME", IMAGE_T), 
        new SkinProperty("datefield.image_icon_datetime", "datefield.image_icon_datetime", "DATEFIELD_IMAGE_ICON_DATETIME", IMAGE_T), 
        new SkinProperty("gauge.orient", "-1", "GAUGE_ORIENT", INT_T), 
        new SkinProperty("gauge.width", "-1", "GAUGE_WIDTH", INT_T), 
        new SkinProperty("gauge.height", "-1", "GAUGE_HEIGHT", INT_T), 
        new SkinProperty("gauge.meter_x", "-1", "GAUGE_METER_X", INT_T), 
        new SkinProperty("gauge.meter_y", "-1", "GAUGE_METER_Y", INT_T), 
        new SkinProperty("gauge.inc_btn_x", "-1", "GAUGE_INC_BTN_X", INT_T), 
        new SkinProperty("gauge.inc_btn_y", "-1", "GAUGE_INC_BTN_Y", INT_T), 
        new SkinProperty("gauge.dec_btn_x", "-1", "GAUGE_DEC_BTN_X", INT_T), 
        new SkinProperty("gauge.dec_btn_y", "-1", "GAUGE_DEC_BTN_Y", INT_T), 
        new SkinProperty("gauge.value_x", "-1", "GAUGE_VALUE_X", INT_T), 
        new SkinProperty("gauge.value_y", "-1", "GAUGE_VALUE_Y", INT_T), 
        new SkinProperty("gauge.value_width", "-1", "GAUGE_VALUE_WIDTH", INT_T), 
        new SkinProperty("gauge.image_bg", "gauge.image_bg", "GAUGE_IMAGE_BG", IMAGE_T), 
        new SkinProperty("gauge.image_mtr_empty", "gauge.image_mtr_empty", "GAUGE_IMAGE_MTR_EMPTY", IMAGE_T), 
        new SkinProperty("gauge.image_mtr_full", "gauge.image_mtr_full", "GAUGE_IMAGE_MTR_FULL", IMAGE_T), 
        new SkinProperty("gauge.image_inc_btn", "gauge.image_inc_btn", "GAUGE_IMAGE_INC_BTN", IMAGE_T), 
        new SkinProperty("gauge.image_dec_btn", "gauge.image_dec_btn", "GAUGE_IMAGE_DEC_BTN", IMAGE_T), 
        new SkinProperty("gauge.image_values", "gauge.image_values", "GAUGE_IMAGE_VALUES", IMAGE_T), 
        new SkinProperty("imageitem.color_bg_lnk_foc", "-1", "IMAGEITEM_COLOR_BG_LNK_FOC", INT_T), 
        new SkinProperty("imageitem.color_bg_btn", "-1", "IMAGEITEM_COLOR_BG_BTN", INT_T), 
        new SkinProperty("imageitem.color_border_lt", "-1", "IMAGEITEM_COLOR_BORDER_LT", INT_T), 
        new SkinProperty("imageitem.color_border_dk", "-1", "IMAGEITEM_COLOR_BORDER_DK", INT_T), 
        new SkinProperty("imageitem.pad_lnk_h", "-1", "IMAGEITEM_PAD_LNK_H", INT_T), 
        new SkinProperty("imageitem.pad_lnk_v", "-1", "IMAGEITEM_PAD_LNK_V", INT_T), 
        new SkinProperty("imageitem.pad_btn_h", "-1", "IMAGEITEM_PAD_BTN_H", INT_T), 
        new SkinProperty("imageitem.pad_btn_v", "-1", "IMAGEITEM_PAD_BTN_V", INT_T), 
        new SkinProperty("imageitem.btn_border_w", "-1", "IMAGEITEM_BTN_BORDER_W", INT_T), 
        new SkinProperty("imageitem.image_lnk_h", "imageitem.image_lnk_h", "IMAGEITEM_IMAGE_LNK_H", IMAGE_T), 
        new SkinProperty("imageitem.image_lnk_v", "imageitem.image_lnk_v", "IMAGEITEM_IMAGE_LNK_V", IMAGE_T), 
        new SkinProperty("imageitem.image_button", "imageitem.image_button", "IMAGEITEM_IMAGE_BUTTON", C_IMAGE_T), 
        new SkinProperty("menu.width", "-1", "MENU_WIDTH", INT_T), 
        new SkinProperty("menu.height", "-1", "MENU_HEIGHT", INT_T), 
        new SkinProperty("menu.align_x", "-1", "MENU_ALIGN_X", INT_T), 
        new SkinProperty("menu.align_y", "-1", "MENU_ALIGN_Y", INT_T), 
        new SkinProperty("menu.title_x", "-1", "MENU_TITLE_X", INT_T), 
        new SkinProperty("menu.title_y", "-1", "MENU_TITLE_Y", INT_T), 
        new SkinProperty("menu.title_maxwidth", "-1", "MENU_TITLE_MAXWIDTH", INT_T), 
        new SkinProperty("menu.title_align", "-1", "MENU_TITLE_ALIGN", INT_T), 
        new SkinProperty("menu.max_items", "-1", "MENU_MAX_ITEMS", INT_T), 
        new SkinProperty("menu.item_height", "-1", "MENU_ITEM_HEIGHT", INT_T), 
        new SkinProperty("menu.item_topoffset", "-1", "MENU_ITEM_TOPOFFSET", INT_T), 
        new SkinProperty("menu.item_index_anchor_x", "-1", "MENU_ITEM_INDEX_ANCHOR_X", INT_T), 
        new SkinProperty("menu.item_anchor_x", "-1", "MENU_ITEM_ANCHOR_X", INT_T), 
        new SkinProperty("menu.color_bg", "-1", "MENU_COLOR_BG", INT_T), 
        new SkinProperty("menu.color_bg_sel", "-1", "MENU_COLOR_BG_SEL", INT_T), 
        new SkinProperty("menu.color_title", "-1", "MENU_COLOR_TITLE", INT_T), 
        new SkinProperty("menu.color_index", "-1", "MENU_COLOR_INDEX", INT_T), 
        new SkinProperty("menu.color_index_sel", "-1", "MENU_COLOR_INDEX_SEL", INT_T), 
        new SkinProperty("menu.color_item", "-1", "MENU_COLOR_ITEM", INT_T), 
        new SkinProperty("menu.color_item_sel", "-1", "MENU_COLOR_ITEM_SEL", INT_T), 
        new SkinProperty("menu.text_title", "menu.text_title", "MENU_TEXT_TITLE", STRING_T), 
        new SkinProperty("menu.font_title", "500", "MENU_FONT_TITLE", FONT_T), 
        new SkinProperty("menu.font_item", "500", "MENU_FONT_ITEM", FONT_T), 
        new SkinProperty("menu.font_item_sel", "500", "MENU_FONT_ITEM_SEL", FONT_T), 
        new SkinProperty("menu.image_bg", "menu.image_bg", "MENU_IMAGE_BG", C_IMAGE_T), 
        new SkinProperty("menu.image_item_sel_bg", "menu.image_item_sel_bg", "MENU_IMAGE_ITEM_SEL_BG", C_IMAGE_T), 
        new SkinProperty("menu.image_submenu", "menu.image_submenu", "MENU_IMAGE_SUBMENU", IMAGE_T), 
        new SkinProperty("menu.image_submenu_hl", "menu.image_submenu_hl", "MENU_IMAGE_SUBMENU_HL", IMAGE_T), 
        new SkinProperty("pbar.orient", "-1", "PBAR_ORIENT", INT_T), 
        new SkinProperty("pbar.width", "-1", "PBAR_WIDTH", INT_T), 
        new SkinProperty("pbar.height", "-1", "PBAR_HEIGHT", INT_T), 
        new SkinProperty("pbar.meter_x", "-1", "PBAR_METER_X", INT_T), 
        new SkinProperty("pbar.meter_y", "-1", "PBAR_METER_Y", INT_T), 
        new SkinProperty("pbar.value_x", "-1", "PBAR_VALUE_X", INT_T), 
        new SkinProperty("pbar.value_y", "-1", "PBAR_VALUE_Y", INT_T), 
        new SkinProperty("pbar.value_width", "-1", "PBAR_VALUE_WIDTH", INT_T), 
        new SkinProperty("pbar.image_bg", "pbar.image_bg", "PBAR_IMAGE_BG", IMAGE_T), 
        new SkinProperty("pbar.image_mtr_empty", "pbar.image_mtr_empty", "PBAR_IMAGE_MTR_EMPTY", IMAGE_T), 
        new SkinProperty("pbar.image_mtr_full", "pbar.image_mtr_full", "PBAR_IMAGE_MTR_FULL", IMAGE_T), 
        new SkinProperty("pbar.image_values", "pbar.image_values", "PBAR_IMAGE_VALUES", IMAGE_T), 
        new SkinProperty("pbar.image_percents", "pbar.image_percents", "PBAR_IMAGE_PERCENTS", IMAGE_T), 
        new SkinProperty("stringitem.pad_button_h", "-1", "STRINGITEM_PAD_BUTTON_H", INT_T), 
        new SkinProperty("stringitem.pad_button_v", "-1", "STRINGITEM_PAD_BUTTON_V", INT_T), 
        new SkinProperty("stringitem.button_border_w", "-1", "STRINGITEM_BUTTON_BORDER_W", INT_T), 
        new SkinProperty("stringitem.color_fg_lnk", "-1", "STRINGITEM_COLOR_FG_LNK", INT_T), 
        new SkinProperty("stringitem.color_fg_lnk_foc", "-1", "STRINGITEM_COLOR_FG_LNK_FOC", INT_T), 
        new SkinProperty("stringitem.color_bg_lnk_foc", "-1", "STRINGITEM_COLOR_BG_LNK_FOC", INT_T), 
        new SkinProperty("stringitem.color_fg_btn", "-1", "STRINGITEM_COLOR_FG_BTN", INT_T), 
        new SkinProperty("stringitem.color_bg_btn", "-1", "STRINGITEM_COLOR_BG_BTN", INT_T), 
        new SkinProperty("stringitem.color_border_lt", "-1", "STRINGITEM_COLOR_BORDER_LT", INT_T), 
        new SkinProperty("stringitem.color_border_dk", "-1", "STRINGITEM_COLOR_BORDER_DK", INT_T), 
        new SkinProperty("stringitem.font", "500", "STRINGITEM_FONT", FONT_T), 
        new SkinProperty("stringitem.font_lnk", "500", "STRINGITEM_FONT_LNK", FONT_T), 
        new SkinProperty("stringitem.font_btn", "500", "STRINGITEM_FONT_BTN", FONT_T), 
        new SkinProperty("stringitem.image_lnk", "stringitem.image_lnk", "STRINGITEM_IMAGE_LNK", IMAGE_T), 
        new SkinProperty("stringitem.image_btn", "stringitem.image_btn", "STRINGITEM_IMAGE_BTN", C_IMAGE_T), 
        new SkinProperty("textfield.pad_h", "-1", "TEXTFIELD_PAD_H", INT_T), 
        new SkinProperty("textfield.pad_v", "-1", "TEXTFIELD_PAD_V", INT_T), 
        new SkinProperty("textfield.box_margin", "-1", "TEXTFIELD_BOX_MARGIN", INT_T), 
        new SkinProperty("textfield.width_caret", "-1", "TEXTFIELD_WIDTH_CARET", INT_T), 
        new SkinProperty("textfield.scrl_rate", "-1", "TEXTFIELD_SCRL_RATE", INT_T), 
        new SkinProperty("textfield.scrl_spd", "-1", "TEXTFIELD_SCRL_SPD", INT_T), 
        new SkinProperty("textfield.color_fg", "-1", "TEXTFIELD_COLOR_FG", INT_T), 
        new SkinProperty("textfield.color_bg", "-1", "TEXTFIELD_COLOR_BG", INT_T), 
        new SkinProperty("textfield.color_brdr", "-1", "TEXTFIELD_COLOR_BRDR", INT_T), 
        new SkinProperty("textfield.color_brdr_shd", "-1", "TEXTFIELD_COLOR_BRDR_SHD", INT_T), 
        new SkinProperty("textfield.color_fg_ue", "-1", "TEXTFIELD_COLOR_FG_UE", INT_T), 
        new SkinProperty("textfield.color_bg_ue", "-1", "TEXTFIELD_COLOR_BG_UE", INT_T), 
        new SkinProperty("textfield.color_brdr_ue", "-1", "TEXTFIELD_COLOR_BRDR_UE", INT_T), 
        new SkinProperty("textfield.color_brdr_shd_ue", "-1", "TEXTFIELD_COLOR_BRDR_SHD_UE", INT_T), 
        new SkinProperty("textfield.image_bg", "textfield.image_bg", "TEXTFIELD_IMAGE_BG", C_IMAGE_T), 
        new SkinProperty("textfield.image_bg_ue", "textfield.image_bg_ue", "TEXTFIELD_IMAGE_BG_UE", C_IMAGE_T), 
        new SkinProperty("updatebar.width", "-1", "UPDATEBAR_WIDTH", INT_T), 
        new SkinProperty("updatebar.height", "-1", "UPDATEBAR_HEIGHT", INT_T), 
        new SkinProperty("updatebar.num_frames", "-1", "UPDATEBAR_NUM_FRAMES", INT_T), 
        new SkinProperty("updatebar.frame_x", "-1", "UPDATEBAR_FRAME_X", INT_T), 
        new SkinProperty("updatebar.frame_y", "-1", "UPDATEBAR_FRAME_Y", INT_T), 
        new SkinProperty("updatebar.frame_sequ", "updatebar.frame_sequ", "UPDATEBAR_FRAME_SEQU", INT_SEQ_T), 
        new SkinProperty("updatebar.image_bg", "updatebar.image_bg", "UPDATEBAR_IMAGE_BG", IMAGE_T), 
        new SkinProperty("updatebar.image_frame", "updatebar.image_frame", "UPDATEBAR_IMAGE_FRAME", C_IMAGE_T),

        // new properties
        new SkinProperty("softbtn.button_align_x", "softbtn.button_align_x", "SOFTBTN_BUTTON_ALIGN_X", INT_SEQ_T, true),
        new SkinProperty("softbtn.button_max_width", "softbtn.button_max_width", "SOFTBTN_BUTTON_MAX_WIDTH", INT_SEQ_T, true), 
        new SkinProperty("softbtn.button_anchor_x", "softbtn.button_anchor_x", "SOFTBTN_BUTTON_ANCHOR_X", INT_SEQ_T, true), 
        new SkinProperty("softbtn.button_anchor_y", "softbtn.button_anchor_y", "SOFTBTN_BUTTON_ANCHOR_Y", INT_SEQ_T, true), 
    };
}

/**
 * Utility class that does the conversion.
 */
class Converter {
    /** All the properties used by Chameleon */
    Hashtable allProps;
    SkinProperty[] allPropsArray;

    /** Properties from RomizedProperties.java being converted */
    Hashtable romizedProps;
    
    /** 
     * For reporting purposes. Properties that are used by 
     * Chameleon, but arent present in RomizedProperties.java
     * being converted.
     */
    Vector missingProps;

    /**
     * For reporting purposes. Properties that are present 
     * in RomizedProperties.java being converted, but aren't
     * used by Chameleon.
     */
    Vector unknownProps;

    /**
     * Constructor
     *
     * @param allProps All properties used by Chameleon
     * @param romizedProps Properties from RomizedProperties.java
     * being converted
     */
    Converter(SkinProperty[] allProps, Hashtable romizedProps) {
        this.allProps = new Hashtable();
        for (int i = 0; i < allProps.length; ++i) {
            SkinProperty p = allProps[i];
            this.allProps.put(p.name, p);
        }

        this.allPropsArray = allProps;
        this.romizedProps = romizedProps;
        this.missingProps = new Vector();
        this.unknownProps = new Vector();
    }

    /**
     * Do the conversion. Mostly its about assigning values from 
     * RomizedProperties.java to the corresponding properties in 
     * "all Chameleon properties" table, but also there is some 
     * real conversion as well.
     */
    void convert() {
        checkForUnknownProps();
        assignPropsValues();

        Enumeration keys = allProps.keys();
        while (keys.hasMoreElements()) {
            String key = (String)keys.nextElement();
            SkinProperty p = (SkinProperty)allProps.get(key);
            if (p.isNew) {
                convertPropsSequenceToString(p);
            }
        }

        assignPiecesCounts();
    }
    
    /**
     * Check if romized properties being convertedhave some properties 
     * that aren't used by Chameleon (most likely it means that those 
     * properties come from some modified version of Chameleon).
     */
    void checkForUnknownProps() {
        Enumeration keys = romizedProps.keys();
        while (keys.hasMoreElements()) {
            String key = (String)keys.nextElement();
            SkinProperty p = (SkinProperty)allProps.get(key);
            if (p == null) {
                String propValue = (String)romizedProps.get(key);
                p = new SkinProperty(key, propValue, null, 
                        SkinProperty.STRING_T);
            }
        }
    }
    
    /**
     * Assign values from romized properties being converted to the 
     * corresponding properties in "all Chameleon properties"
     * table. If some property used by Chameleon isn't present in romized
     * properties, then this property will keep its default value.
     */
    void assignPropsValues() {
        Enumeration keys = allProps.keys();
        while (keys.hasMoreElements()) {
            String key = (String)keys.nextElement();
            SkinProperty p = (SkinProperty)allProps.get(key);
            String propValue = (String)romizedProps.get(key);
            if (propValue != null) {
                p.value = propValue;
            } else {
                if (!p.isNew) {
                    missingProps.add(p);
                }
            }
        }
    }

    /**
     * Convert sequence of integer valued properties from
     * romized properties being converted, like
     * softbtn.button_align_x0, softbtn.button_align_x1,
     * softbtn.button_align_x2 and so on (thats the sequence
     * with softbtn.button_align_x being a prefix), into a 
     * single property with string value, where string holds 
     * all integer values from the sequence separated by 
     * comma.
     *
     * @param newProp Property that will get new string value.
     * The name of this property is used as a prefix for the
     * sequence being converted.
     */
    void convertPropsSequenceToString(SkinProperty newProp) {
        String newPropValue = null;
        String seqPropNamePrefix = newProp.name;
        int index = 0;

        String seqPropName = seqPropNamePrefix + index;
        String seqPropValue = (String)romizedProps.get(seqPropName);
        while (seqPropValue != null) {
            if (newPropValue == null) {
                newPropValue = seqPropValue;
            } else {
                newPropValue += "," + seqPropValue;
            }

            index += 1;
            seqPropName = seqPropNamePrefix + index;
            seqPropValue = (String)romizedProps.get(seqPropName);
        }

        newProp.value = newPropValue;
    }

    /**
     * For properties that corresponds to composite images,
     * assign images pieces counts.
     */
    void assignPiecesCounts() {
        SkinProperty p;
        
        p = (SkinProperty)allProps.get("alert.image_bg");
        if (p != null) {
            p.totalPieces = 9;
        }
        
        p = (SkinProperty)allProps.get("busycrsr.image_frame");
        if (p != null) {
            SkinProperty parts = (SkinProperty)allProps.get("busycrsr.num_frames");
            if (parts != null) {
                p.totalPieces = Integer.parseInt(parts.value);
            }
        }

        p = (SkinProperty)allProps.get("choice.image_radio");
        if (p != null) {
            p.totalPieces = 2;
        }

        p = (SkinProperty)allProps.get("choice.image_chkbx");
        if (p != null) {
            p.totalPieces = 2;
        }

        p = (SkinProperty)allProps.get("choice.image_bg");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("choice.image_btn_bg");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("choice.image_popup_bg");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("dateeditor.image_bg");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("dateeditor.image_radio");
        if (p != null) {
            p.totalPieces = 2;
        }

        p = (SkinProperty)allProps.get("datefield.image_bg");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("datefield.image_btn_bg");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("imageitem.image_button");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("menu.image_bg");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("menu.image_item_sel_bg");
        if (p != null) {
            p.totalPieces = 3;
        }

        p = (SkinProperty)allProps.get("screen.image_bg_w_title");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("screen.image_bg_wo_title");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("screen.image_hs_bg_w_title");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("screen.image_hs_bg_wo_title");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("scroll.image_bg");
        if (p != null) {
            p.totalPieces = 3;
        }

        p = (SkinProperty)allProps.get("scroll.image_fg");
        if (p != null) {
            p.totalPieces = 3;
        }

        p = (SkinProperty)allProps.get("scroll.image_au_bg");
        if (p != null) {
            p.totalPieces = 3;
        }
        
        p = (SkinProperty)allProps.get("scroll.image_au_fg");
        if (p != null) {
            p.totalPieces = 3;
        }

        p = (SkinProperty)allProps.get("softbtn.image_bg");
        if (p != null) {
            p.totalPieces = 3;
        }

        p = (SkinProperty)allProps.get("softbtn.image_mu_bg");
        if (p != null) {
            p.totalPieces = 3;
        }

        p = (SkinProperty)allProps.get("softbtn.image_au_bg");
        if (p != null) {
            p.totalPieces = 3;
        }

        p = (SkinProperty)allProps.get("stringitem.image_btn");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("textfield.image_bg");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("textfield.image_bg_ue");
        if (p != null) {
            p.totalPieces = 9;
        }

        p = (SkinProperty)allProps.get("ticker.image_bg");
        if (p != null) {
            p.totalPieces = 3;
        }

        p = (SkinProperty)allProps.get("ticker.image_au_bg");
        if (p != null) {
            p.totalPieces = 3;
        }

        p = (SkinProperty)allProps.get("title.image_bg");
        if (p != null) {
            p.totalPieces = 3;
        }

        p = (SkinProperty)allProps.get("updatebar.image_frame");
        if (p != null) {
            SkinProperty parts = (SkinProperty)allProps.get("updatebar.num_frames");
            p.totalPieces = Integer.parseInt(parts.value);
        }
    }
}


/**
 * The main tool class.
 */
public class UpgradeRomizedProperties {
    static PrintWriter writer;
    
    public static void main(String args[]) {
        if (args.length != 2) {
            String u = 
            "Usage: java -cp . UpgradeRomizedProperties <infile> <outfile>";
            System.out.println(u);
            System.exit(0);
        }
        
        Runtime rt = Runtime.getRuntime();
        String cmd = 
            "javac -d " + System.getProperty("user.dir") + " " + args[0];
        int exitCode;
        try {
            Process p = rt.exec(cmd); 
            exitCode = p.waitFor();
        } catch (Exception e) {
            System.err.println(e);
            System.exit(1);
        }


        Hashtable rp = new Hashtable();
        Object[] argz = { rp };
        try {
            String cn = 
                "com.sun.midp.chameleon.skins.resources.RomizedProperties";
            Class clazz = Class.forName(cn);
            Method loadProperties;
            loadProperties = 
                clazz.getMethod("load", new Class[] { Hashtable.class });
            loadProperties.invoke(null, argz);
        } catch (Exception e) {
            System.err.println(e);
            System.exit(1);
        }

        Converter converter = new Converter(SkinProperty.properties, rp);
        converter.convert();
        
        try {
            FileOutputStream fout = new FileOutputStream(args[1]);
            OutputStreamWriter w = new OutputStreamWriter(fout);
            writer = new PrintWriter(w);
        } catch (Exception e) {
            System.err.println(e);
            System.exit(1);
        }

        printHeader();
        printSkinProperties();
        printFooter();
        writer.close();

        reportUnknownProperties(converter.unknownProps);
        reportMissingProperties(converter.missingProps);
    }

    /**
     * Short-hand for printint a line into the output file
     */
    static void pl(String s) {
        writer.println(s);
    }

    static void printHeader() {
        pl("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
        pl("<!DOCTYPE configuration SYSTEM \"../configuration.dtd\">");
        pl("<!--");
        pl("        	");
        pl("");
        pl("        Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.");
        pl("        DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER");
        pl("        ");
        pl("        This program is free software; you can redistribute it and/or");
        pl("        modify it under the terms of the GNU General Public License version");
        pl("        2 only, as published by the Free Software Foundation.");
        pl("        ");
        pl("        This program is distributed in the hope that it will be useful, but");
        pl("        WITHOUT ANY WARRANTY; without even the implied warranty of");
        pl("        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU");
        pl("        General Public License version 2 for more details (a copy is");
        pl("        included at /legal/license.txt).");
        pl("        ");
        pl("        You should have received a copy of the GNU General Public License");
        pl("        version 2 along with this work; if not, write to the Free Software");
        pl("        Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA");
        pl("        02110-1301 USA");
        pl("        ");
        pl("        Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa");
        pl("        Clara, CA 95054 or visit www.sun.com if you need additional");
        pl("        information or have any questions.");
        pl("-->");
        pl("<configuration>");
        pl("<skin>");
        pl("<skin_properties KeysClass=" + 
                "\"com.sun.midp.chameleon.skins.SkinPropertiesIDs\">");
    }
    
    
    static void printFooter() {
        pl("</skin_properties>");
        pl("</skin>");
        pl("</configuration>");
    }

    static void printSkinProperties() {
        for (int i = 0; i < SkinProperty.properties.length; ++i) {
            SkinProperty p = SkinProperty.properties[i];
            switch (p.type) {
                case SkinProperty.INT_T:
                    pl("  <integer");
                    pl("        Key=\"" + p.id + "\"");
                    pl("      Value=\"" + p.value + "\"/>");
                    break;

                case SkinProperty.INT_SEQ_T:
                    pl("  <integer_seq");
                    pl("        Key=\"" + p.id + "\"");
                    pl("      Value=\"" + p.value + "\"/>");
                    break;

                case SkinProperty.STRING_T:
                    pl("  <string");
                    pl("        Key=\"" + p.id + "\"");
                    pl("      Value=\"" + p.value + "\"/>");
                    break;

                case SkinProperty.FONT_T:
                    pl("  <font");
                    pl("        Key=\"" + p.id + "\"");
                    pl("      Value=\"" + p.value + "\"/>");
                    break;

                case SkinProperty.IMAGE_T:
                    pl("  <image");
                    pl("        Key=\"" + p.id + "\"");
                    pl("      Value=\"" + p.value + "\"");
                    pl("    Romized=\"false\"/>");
                    break;

                case SkinProperty.C_IMAGE_T:
                    pl("  <composite_image");
                    pl("        Key=\"" + p.id + "\"");
                    pl("      Value=\"" + p.value + "\"");
                    pl("     Pieces=\"" + p.totalPieces + "\"");
                    pl("    Romized=\"false\"/>");
                    break;
            }

            pl("");
        }
    }

    static void reportMissingProperties(Vector props) {
        String msg = 
"Following properties are used by current Chameleon code, but aren't present\n" +
"in romized properties being converted. Old RomizedProperties.java format\n" +
"allows omitting some properties. New format forbids that. All the properties\n" +
"used by Chameleon must be explicitly given values in RomizedProperties.java.\n" +
"Therefore, those properties have been explicitly given default values:";
        
        if (props.size() > 0) {
            System.out.println(msg);

            for (int i = 0; i < props.size(); ++i) {
                SkinProperty p = (SkinProperty)props.get(i);
                System.out.println("  " + p.name + ": " + '"' + p.value + '"');
            }
        }
    }

    static void reportUnknownProperties(Vector props) {
        String msg = 
"Following properties aren't used by current Chameleon code, but are present\n" +
"in romized properties being converted. Probably you are using modified\n" + 
"Chameleon code. You'll have to add these properties manually to the\n" + 
"results of conversion and then modify your Chameleon code accordingly:";

        if (props.size() > 0) {
            System.out.println(msg);

            for (int i = 0; i < props.size(); ++i) {
                SkinProperty p = (SkinProperty)props.get(i);
                System.out.println("  " + p.name + ": " + '"' + p.value + '"');
            }
        }
    }
}

