package com.sun.midp.io.pipe;

import javax.microedition.io.StreamConnection;

public interface PipeConnection extends StreamConnection {

    String getRequestedServerVersion();

    String getServerName();
}
