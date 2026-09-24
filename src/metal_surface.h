#pragma once
#include <QtGui/qwindowdefs.h>
void *createMetalLayer(WId view);
void resizeMetalLayer(void *layer,int width,int height,double scale);
void releaseMetalLayer(void *layer);
