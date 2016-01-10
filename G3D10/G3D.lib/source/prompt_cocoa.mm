#include "prompt_cocoa.h"
#import <Cocoa/Cocoa.h>



int prompt_cocoa(
    const char*     windowTitle,
    const char*     promptx,
    const char**    choice,
    int             numChoices) {
  int i;
  NSAlert *alert = [[NSAlert alloc] init];
  for (i = 0; i < numChoices; ++i) {
    [alert addButtonWithTitle:[ NSString stringWithUTF8String:choice[i] ] ];
  }
  [alert setMessageText:[ NSString stringWithUTF8String:windowTitle]];
  [alert setInformativeText:[ NSString stringWithUTF8String:promptx]];
  [alert setAlertStyle:NSWarningAlertStyle];
  NSInteger answer = [alert runModal] - NSAlertFirstButtonReturn;
  [alert release];
  alert = nil;
  return (int)answer;
}
