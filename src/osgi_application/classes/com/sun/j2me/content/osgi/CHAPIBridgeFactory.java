package com.sun.j2me.content.osgi;

import java.util.Dictionary;
import java.util.Hashtable;

import org.osgi.framework.*;

class MidletContentHandler implements CHAPIBridge {
    
	static final String supportedInterfaces[] = {
    	CHAPIBridge.class.getName()
    };

	MidletContentHandler( BundleContext context ){
	}

	public void invocationRequestNotify(String contentHandlerServer, String appID) {
	}

	public void invocationResponceNotify(String registry, String appID) {
	}
}

public class CHAPIBridgeFactory implements ServiceFactory {
    static final String SERVICE_PID = "service.pid";
    
    static final String supportedInterfaces[] = {
    	CHAPIBridgeFactory.class.getName()
    };

    static final Dictionary props = new Hashtable();
    static {
        // props initializer

    }
    
	final BundleContext context;
	
	CHAPIBridgeFactory( BundleContext context ){
		this.context = context;
	}
	
    public Object getService( Bundle bundle, ServiceRegistration registration ) {
        return new MidletContentHandler(context);
    }

    public void ungetService( Bundle bundle, ServiceRegistration reg, Object service ) {
    }
}
