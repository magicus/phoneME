
package com.sun.pisces;

public interface PiscesRendererFactory {
    
    PiscesRenderer createPiscesRenderer(Object o);
    
    GraphicsSurfaceIface createGraphicsSurface();
    
}