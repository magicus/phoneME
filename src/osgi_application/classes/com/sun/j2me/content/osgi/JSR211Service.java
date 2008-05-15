/*
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
 */

package com.sun.j2me.content.osgi;

import java.util.Dictionary;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Map;

import org.osgi.framework.Bundle;
import org.osgi.framework.BundleActivator;
import org.osgi.framework.BundleContext;
import org.osgi.framework.InvalidSyntaxException;
import org.osgi.framework.ServiceFactory;
import org.osgi.framework.ServiceReference;
import org.osgi.framework.ServiceRegistration;
import org.osgi.service.application.ApplicationDescriptor;

import com.sun.j2me.content.AMSGate;
import com.sun.j2me.content.AppProxy;
import com.sun.j2me.content.ContentHandlerImpl;
import com.sun.j2me.content.ContentHandlerPersistentData;
import com.sun.j2me.content.RegistryGate;

interface CHAPIBridge {
	void invocationRequestNotify( String contentHandlerServer, String appID );
	void invocationResponceNotify( String registry, String appID );
}

public class JSR211Service implements RegistryGate, AMSGate, ServiceFactory {
	
	class ContentHandlerHandle implements ContentHandlerImpl.Handle {
		private final ServiceReference	handlerService;
		private ContentHandlerImpl 	created = null;
		
		private final String	handlerID;
		private final int 		handlerSuiteId;
		
		ContentHandlerHandle( ServiceReference sr ){
			handlerService = sr;
			handlerID = (String)handlerService.getProperty("service.pid");
			handlerSuiteId =  
				((Integer)handlerService.getProperty("ContentHandler.suiteId")).intValue();
		}

		public ContentHandlerImpl get(){
			if( created == null ){
				created = new ContentHandlerImpl(this){{
					this.ID = handlerID; 
					this.storageId = handlerSuiteId;
					this.classname = 
						(String)handlerService.getProperty("ContentHandler.classname"); 
					this.registrationMethod = 
						((Integer)handlerService.getProperty("ContentHandler.registrationMethod")).intValue();
				}};
				// initialize fields
			}
			return created;
		}
		
		public String getID() { return handlerID; }
		public int getSuiteId() { return handlerSuiteId; }

		public String[] getArrayField(int fieldId) {
			String[] result;
			switch( fieldId ){
				case FIELD_TYPES:
					result = (String[])handlerService.getProperty("ContentHandler.types");
					break;
				case FIELD_SUFFIXES:
					result = (String[])handlerService.getProperty("ContentHandler.suffixes");
					break;
				case FIELD_ACTIONS:
					result = (String[])handlerService.getProperty("ContentHandler.actions");
					break;
				case FIELD_LOCALES:
					result = (String[])handlerService.getProperty("ContentHandler.locales");
					break;
				case FIELD_ACTION_MAP:
					result = (String[])handlerService.getProperty("ContentHandler.action_map");
					break;
				case FIELD_ACCESSES:
					result = (String[])handlerService.getProperty("ContentHandler.accesses");
					break;
				default:
					throw new InternalError();
			}
			return result;
		}
	}

    public static final String supportedInterfaces[] = {
    	RegistryGate.class.getName(), 
    	AMSGate.class.getName(),
    };

    public static final Dictionary props = new Hashtable();
    static {
        // props initializer

    }
    
    protected BundleContext context;
    
    JSR211Service( BundleContext context ){
    	this.context = context;
    }

    // Service Factory

    public Object getService( Bundle bundle, ServiceRegistration registration ) {
        return this;
    }

    public void ungetService( Bundle bundle, ServiceRegistration reg, Object service ) {
    }

    // BundleActivator

    static public class Activator implements BundleActivator {

        public void start( BundleContext context ) throws Exception {
            context.registerService( JSR211Service.supportedInterfaces, 
            				new JSR211Service(context), JSR211Service.props );
            context.registerService( CHAPIBridgeFactory.supportedInterfaces, 
            				new CHAPIBridgeFactory(context), CHAPIBridgeFactory.props ); 
            AppProxy.setRegistry( 
                    (RegistryGate)context.getService( 
                            context.getServiceReference( RegistryGate.class.getName() ) ) );
            AppProxy.setAMS( 
                    (AMSGate)context.getService( 
                            context.getServiceReference( AMSGate.class.getName() ) ) );
            
            testCode(context);
        }

        private void testCode(BundleContext context) throws InvalidSyntaxException {
            ServiceReference[] services;
            services = context.getServiceReferences(ApplicationDescriptor.class.getName(), null);
            System.out.println( "ApplicationDescriptors = " + services );
            if (services != null && services.length > 0) {
                for (int i = 0; i < services.length; i++) {
                    // handles[i].getApplicationDescriptor();
                    ApplicationDescriptor appD = (ApplicationDescriptor)context.getService(services[i]);
                    System.out.println( "appD is " + appD );
                    Map props = appD.getProperties(null);
					String appName = (String)props.get(ApplicationDescriptor.APPLICATION_NAME);
                    System.out.println( "app[" + i + "].name = " + appName );
                    Iterator keys = props.keySet().iterator();
                    while( keys.hasNext() ){
                    	Object key = keys.next();
                    	Object value = props.get(key);
                        System.out.println( "\t" + key.toString() + ": '" + value.toString() + "'" );
                    }
                }
            }
		}

		public void stop( BundleContext context ) throws Exception {
            AppProxy.setRegistry( (RegistryGate)null );
            AppProxy.setAMS( (AMSGate)null );
        }
    }

    // RegistryGate methods
    
	public ContentHandlerImpl.Handle register(int suiteId, String classname, 
										ContentHandlerPersistentData handlerData) {
		// all parameters must not be null
		
	    final Dictionary props = new Hashtable();
	    props.put(CHAPIBridgeFactory.SERVICE_PID, handlerData.getID());
	    
	    props.put("ContentHandler.suiteId", new Integer(suiteId));
	    props.put("ContentHandler.classname", classname);
	    props.put("ContentHandler.registrationMethod", new Integer(handlerData.getRegistrationMethod()));
	    props.put("ContentHandler.types", handlerData.getTypes());
	    props.put("ContentHandler.suffixes", handlerData.getSuffixes());
	    props.put("ContentHandler.actions", handlerData.getActions());
	    props.put("ContentHandler.locales", null); // TODO
	    props.put("ContentHandler.action_map", null);
	    props.put("ContentHandler.accesses", handlerData.getAccessRestricted());
	    return new ContentHandlerHandle( 
	    			context.registerService( MidletContentHandler.supportedInterfaces, 
								new MidletContentHandler( context ), props ).getReference() );
	}

	public boolean unregister(String handlerId) {
		// TODO Auto-generated method stub
		return false;
	}
	
	public void enumJavaHandlers(ContentHandlerHandle.Receiver r) {
		Bundle javaBundle = context.getBundle();
		ServiceReference[] services = javaBundle.getRegisteredServices();
		if( services != null ){
			for( int i = 0; i < services.length; i++){
				ServiceReference service = services[i];
				if( service.isAssignableTo(javaBundle, 
								MidletContentHandler.class.getName()) ){
					r.push(new ContentHandlerHandle( service ));
				}
			}
		}
	}

	public void enumHandlers(ContentHandlerHandle.Receiver r) {
		enumJavaHandlers( r );
	}

    // AMSGate methods
}