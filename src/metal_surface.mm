#include "metal_surface.h"
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
void *createMetalLayer(WId nativeView){
    NSView *view=reinterpret_cast<NSView *>(nativeView);auto *layer=[CAMetalLayer new];
    view.wantsLayer=YES;view.layer=layer;layer.delegate=(id<CALayerDelegate>)view;return layer;
}
void resizeMetalLayer(void *ptr,int width,int height,double scale){auto *layer=(CAMetalLayer *)ptr;layer.contentsScale=scale;layer.drawableSize=CGSizeMake(width,height);}
void releaseMetalLayer(void *ptr){auto *layer=(CAMetalLayer *)ptr;layer.delegate=nil;[layer release];}
