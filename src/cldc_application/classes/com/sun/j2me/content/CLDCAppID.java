package com.sun.j2me.content;

public class CLDCAppID implements ApplicationID {
	public int		suiteID;
	public String	className;
	
	public CLDCAppID( int suiteID, String classname) {
		this.suiteID = suiteID;
		this.className = classname;
	}
	
	public CLDCAppID() {
		this(AppProxy.EXTERNAL_SUITE_ID, null);
	}
	
	public boolean isNative() {
		return suiteID == AppProxy.EXTERNAL_SUITE_ID;
	}
	
	public ApplicationID duplicate() {
		return new CLDCAppID(suiteID, className);
	}

	public static CLDCAppID from(ApplicationID appID) {
		return (CLDCAppID)appID;
	}
	
	public int hashCode() {
		return suiteID + className.hashCode();
	}
	
	public boolean equals(Object appID) {
		if( !(appID instanceof CLDCAppID) )
			return false;
		return suiteID == ((CLDCAppID)appID).suiteID &&
					className.equals(((CLDCAppID)appID).className);
	}
	
	public String toString(){
		if( AppProxy.LOGGER != null )
			return "{" + suiteID + ", " + className + "}";
		return super.toString();
	}
}
