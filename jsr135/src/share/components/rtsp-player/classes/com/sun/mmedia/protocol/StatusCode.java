/*
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
package com.sun.mmedia.rtsp.protocol;

public class StatusCode {
    public final static int CONTINUE = 100;
    public final static int OK = 200;
    public final static int CREATED = 201;
    public final static int LOW_ON_STORAGE_SPACE = 250;
    public final static int MULTIPLE_CHOICES = 300;
    public final static int MOVED_PERMANENTLY = 301;
    public final static int MOVED_TEMPORARILY = 302;
    public final static int SEE_OTHER = 303;
    public final static int NOT_MODIFIED = 304;
    public final static int USE_PROXY = 305;
    public final static int BAD_REQUEST = 400;
    public final static int UNAUTHORIZED = 401;
    public final static int PAYMENT_REQUIRED = 402;
    public final static int FORBIDDEN = 403;
    public final static int NOT_FOUND = 404;
    public final static int METHOD_NOT_ALLOWED = 405;
    public final static int NOT_ACCEPTABLE = 406;
    public final static int PROXY_AUTHENTICATION_REQUIRED = 407;
    public final static int REQUEST_TIMED_OUT = 408;
    public final static int GONE = 410;
    public final static int LENGTH_REQUIRED = 411;
    public final static int PRECONDITION_FAILED = 412;
    public final static int REQUEST_ENTITY_TOO_LARGE = 413;
    public final static int REQUEST_URI_TOO_LARGE = 414;
    public final static int UNSUPPORTED_MEDIA_TYPE = 415;
    public final static int PARAMETER_NOT_UNDERSTOOD = 451;
    public final static int CONFERENCE_NOT_FOUND = 452;
    public final static int NOT_ENOUGH_BANDWIDTH = 453;
    public final static int SESSION_NOT_FOUND = 454;
    public final static int METHOD_NOT_VALID_IN_THIS_STATE = 455;
    public final static int HEADER_FIELD_NOT_VALID = 456;
    public final static int INVALID_RANGE = 457;
    public final static int PARAMETER_IS_READ_ONLY = 458;
    public final static int AGGREGATE_OPERATION_NOT_ALLOWED = 459;
    public final static int ONLY_AGGREGATE_OPERATION_ALLOWED = 460;
    public final static int UNSUPPORTED_TRANSPORT = 461;
    public final static int DESTINATION_UNREACHABLE = 462;
    public final static int INTERNAL_SERVER_ERROR = 500;
    public final static int NOT_IMPLEMENTED = 501;
    public final static int BAD_GATEWAY = 502;
    public final static int SERVICE_UNAVAILABLE = 503;
    public final static int GATEWAY_TIME_OUT = 504;
    public final static int RTSP_VERSION_NOT_SUPPORTED = 505;
    public final static int OPTION_NOT_SUPPORTED = 551;

    private int code;

    public StatusCode(int code) {
        this.code = code;
    }

    public static String getStatusText(int code) {
        String text;

        switch (code) {
            case CONTINUE:
                text = "Continue";
                break;
            case OK:
                text = "Ok";
                break;
            case CREATED:
                text = "Created";
                break;
            case LOW_ON_STORAGE_SPACE:
                text = "Low on storage space";
                break;
            case MULTIPLE_CHOICES:
                text = "Multiple choices";
                break;
            case MOVED_PERMANENTLY:
                text = "Moved permanently";
                break;
            case MOVED_TEMPORARILY:
                text = "Moved temporarily";
                break;
            case SEE_OTHER:
                text = "See other";
                break;
            case NOT_MODIFIED:
                text = "Not modified";
                break;
            case USE_PROXY:
                text = "Use proxy";
                break;
            case BAD_REQUEST:
                text = "Bad request";
                break;
            case UNAUTHORIZED:
                text = "Unauthorized";
                break;
            case PAYMENT_REQUIRED:
                text = "Payment required";
                break;
            case FORBIDDEN:
                text = "Forbidden";
                break;
            case NOT_FOUND:
                text = "Not found";
                break;
            case METHOD_NOT_ALLOWED:
                text = "Method not allowed";
                break;
            case NOT_ACCEPTABLE:
                text = "Not acceptable";
                break;
            case PROXY_AUTHENTICATION_REQUIRED:
                text = "Proxy authentication required";
                break;
            case REQUEST_TIMED_OUT:
                text = "Request timed out";
                break;
            case GONE:
                text = "Gone";
                break;
            case LENGTH_REQUIRED:
                text = "Length required";
                break;
            case PRECONDITION_FAILED:
                text = "Precondition failed";
                break;
            case REQUEST_ENTITY_TOO_LARGE:
                text = "Request entity too large";
                break;
            case REQUEST_URI_TOO_LARGE:
                text = "Request URI too large";
                break;
            case UNSUPPORTED_MEDIA_TYPE:
                text = "Unsupported media type";
                break;
            case PARAMETER_NOT_UNDERSTOOD:
                text = "Parameter not understood";
                break;
            case CONFERENCE_NOT_FOUND:
                text = "Conference not found";
                break;
            case NOT_ENOUGH_BANDWIDTH:
                text = "Not enough bandwidth";
                break;
            case SESSION_NOT_FOUND:
                text = "Session not found";
                break;
            case METHOD_NOT_VALID_IN_THIS_STATE:
                text = "Method not valid in this state";
                break;
            case HEADER_FIELD_NOT_VALID:
                text = "Header field not valid";
                break;
            case INVALID_RANGE:
                text = "Invalid range";
                break;
            case PARAMETER_IS_READ_ONLY:
                text = "Parameter is read only";
                break;
            case AGGREGATE_OPERATION_NOT_ALLOWED:
                text = "Aggregate operation not allowed";
                break;
            case ONLY_AGGREGATE_OPERATION_ALLOWED:
                text = "Only aggregate operation allowed";
                break;
            case UNSUPPORTED_TRANSPORT:
                text = "Unsupported transport";
                break;
            case DESTINATION_UNREACHABLE:
                text = "Destination unreachable";
                break;
            case INTERNAL_SERVER_ERROR:
                text = "Internal server error";
                break;
            case NOT_IMPLEMENTED:
                text = "Not implemented";
                break;
            case BAD_GATEWAY:
                text = "Bad gateway";
                break;
            case SERVICE_UNAVAILABLE:
                text = "Service unavailable";
                break;
            case GATEWAY_TIME_OUT:
                text = "Gateway time-out";
                break;
            case RTSP_VERSION_NOT_SUPPORTED:
                text = "RTSP version not supported";
                break;
            case OPTION_NOT_SUPPORTED:
                text = "Option not supported";
                break;
            default:
                text = "Unknown status code: " + code;
        }

        return text;
    }
}
