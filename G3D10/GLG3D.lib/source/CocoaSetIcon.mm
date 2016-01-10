/**
  \file CocoaSetIcon.h

  \maintainer Michael Mara, http://illuminationcodified.com
  @created 2014-08-19
  @edited  2014-08-19
*/
#import <AppKit/NSImage.h>
#import <AppKit/NSApplication.h>
#import <Foundation/Foundation.h>
#import <IOKit/graphics/IOGraphicsLib.h>
#import <IOKit/graphics/IOGraphicsLib.h>
#import <ApplicationServices/ApplicationServices.h>

#import "GLG3D/CocoaInterface.h"
namespace G3D {
bool setCocoaIcon(const char* filename)
{
    NSString * str = [NSString  stringWithCString:filename encoding:NSUTF8StringEncoding];
    NSImage * im   = [[NSImage  alloc] initWithContentsOfFile:str];
    
    bool successLoadingImage = im != nil;
    // set the image in the image view
    if (successLoadingImage) {
        [NSApp setApplicationIconImage:im];
    }
    return successLoadingImage;
    
}
}
