package com.sun.cdc.io;

import java.io.IOException;
import javax.microedition.io.*;

public interface InternalConnector {
    
    public Connection open(String name, int mode, boolean timeouts)
        throws IOException;
}


