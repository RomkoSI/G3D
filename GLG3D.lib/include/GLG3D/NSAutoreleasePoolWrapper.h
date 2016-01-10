/**
  @file NSAutoreleasePoolWrapper.h

  @maintainer Morgan McGuire, http://graphics.cs.williams.edu, Casey O'Donnell, caseyodonnell@gmail.com
  @created 2006-08-27
  @edited  2006-08-31
*/

#ifndef NSAUTORELEASEPOOLWRAPPER_H
#define NSAUTORELEASEPOOLWRAPPER_H

class NSApplicationWrapper {
public:
    NSApplicationWrapper();
    virtual ~NSApplicationWrapper();

private:
protected:
};

class NSAutoreleasePoolWrapper {
public:
    NSAutoreleasePoolWrapper();
    virtual ~NSAutoreleasePoolWrapper();

private:
    void* _pool;

protected:

};

#endif
