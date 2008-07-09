package javax.microedition.content;

import com.sun.j2me.content.InvocationImpl;

class Tunnel implements com.sun.j2me.content.Tunnel {
	
	static {
		InvocationImpl.tunnel = new Tunnel();
System.out.println( "InvocationImpl.tunnel is initialized " + InvocationImpl.tunnel );		
	}
	
	static void initialize(){}

	public InvocationImpl getInvocImpl(Invocation invoc) {
		return invoc.getInvocImpl();
	}

	public Invocation newInvocation(InvocationImpl invocationImpl) {
		return new Invocation( invocationImpl );
	}
}
