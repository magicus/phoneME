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

import com.sun.j2me.content.AMS;
import com.sun.j2me.content.AppProxy;
import com.sun.j2me.content.Registry;

public class JSR211Service implements Registry, AMS, ServiceFactory {

    public static final String supportedInterfaces[] = {
        Registry.class.getName(), 
        AMS.class.getName()
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
            JSR211Service service = new JSR211Service(context);
            context.registerService( JSR211Service.supportedInterfaces, service, JSR211Service.props );

            AppProxy.setRegistry( 
                    (Registry)context.getService( 
                            context.getServiceReference( Registry.class.getName() ) ) );
            AppProxy.setAMS( 
                    (AMS)context.getService( 
                            context.getServiceReference( AMS.class.getName() ) ) );
            
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
            AppProxy.setRegistry( (Registry)null );
            AppProxy.setAMS( (AMS)null );
        }
    }
}