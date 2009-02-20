package com.sun.pisces;

public class PiscesRendererFactoryMIDPImpl implements PiscesRendererFactory {
    
    public PiscesRenderer getPiscesRenderer(Object o) {
        return new PiscesRendererMIDPImpl((AbstractSurface) o);
    
    }
    
    
}