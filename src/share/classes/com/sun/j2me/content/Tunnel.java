package com.sun.j2me.content;

import javax.microedition.content.Invocation;

public interface Tunnel {

	InvocationImpl getInvocImpl(Invocation invoc);

	Invocation newInvocation(InvocationImpl invocationImpl);

}
