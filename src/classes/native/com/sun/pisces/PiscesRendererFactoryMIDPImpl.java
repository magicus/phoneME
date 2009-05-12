package com.sun.pisces;

public class PiscesRendererFactoryMIDPImpl implements PiscesRendererFactory {
    
    public PiscesRenderer createPiscesRenderer(Object o) {
        return new PiscesRendererMIDPImpl((AbstractSurface) o);
    
    }
    
    public GraphicsSurfaceIface createGraphicsSurface() {
        return new GraphicsSurface();
    }
    
}