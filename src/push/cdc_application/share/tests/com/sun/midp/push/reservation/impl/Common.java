package com.sun.midp.push.gcf.impl;

import java.io.IOException;

import com.sun.midp.push.gcf.ConnectionReservation;
import com.sun.midp.push.gcf.DataAvailableListener;
import com.sun.midp.push.gcf.PermissionCallback;
import com.sun.midp.push.gcf.ReservationDescriptor;

/** Common test utils. */
final class Common {
    /** Utility class. */
    private Common() { }

    /** Stub <code>ReservationDescriptor</code> implementation. */
    static final ReservationDescriptor STUB_RESERVATION_DESCR =
        new ReservationDescriptor() {
            /** {@inheritDoc} */
            public String getConnectionName() {
                throw new RuntimeException("Shouldn't get here");
            }

            /** {@inheritDoc} */
            public String getFilter() {
                throw new RuntimeException("Shouldn't get here");
            }

            /** {@inheritDoc} */
            public ConnectionReservation reserve(
                    final int midletSuiteId,
                    final String midletClassName,
                    final DataAvailableListener dataAvailableListener)
                        throws IOException {
                throw new RuntimeException("Shouldn't get here");
            }
    };

    /** Stub <code>ProtocolFactory</code> implementation. */
    static final ProtocolFactory STUB_PROTOCOL_FACTORY =
        new ProtocolFactory() {
            /** {@inheritDoc} */
            public ReservationDescriptor createDescriptor(
                    final String protocol, final String targetAndParams,
                    final String filter,
                    final PermissionCallback permissionCallback)
                        throws IllegalArgumentException, SecurityException {
                return STUB_RESERVATION_DESCR;
            }
    };

    static final String VALID_ONE_CHAR_PROTOCOL = "f";

    static final String VALID_SECOND_ALPHA_CHAR_PROTOCOL = "fg";

    static final String VALID_SECOND_DIGIT_CHAR_PROTOCOL = "f7";

    static final String VALID_SECOND_PLUS_CHAR_PROTOCOL = "f+";

    static final String VALID_SECOND_MINUS_CHAR_PROTOCOL = "f-";

    static final String VALID_SECOND_DOT_CHAR_PROTOCOL = "f.";

    static final String VALID_PROTOCOL = "two+3-17th.well-cool.";
}
