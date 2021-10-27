#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUGGING

#ifdef DEBUGGING
#define DEBUGF(args...) Serial1.printf(args)
#define DEBUGW(buf, size) Serial1.write(buf, size)
#define DEBUG(x) Serial1.println(x)
#else
#define DEBUGF(args...)
#define DEBUGW(buf, size)
#define DEBUG(x)
#endif

#endif