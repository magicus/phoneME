package com.sun.midp.io.pipe;

import javax.microedition.io.StreamConnectionNotifier;

public interface PipeServerConnection extends StreamConnectionNotifier {
    String getName();

    String getVersion();
}
