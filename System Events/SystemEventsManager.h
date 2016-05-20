//
//  SystemEventsManager.h
//  System Events
//
//  Created by Mehdi Mhiri on 18/05/2016.
//
//

#ifndef SystemEventsManager_h
#define SystemEventsManager_h

#include "Event.h"

#define SYSTEM_SLEEP 0
#define SYSTEM_WAKE 1
#define SYSTEM_SHUTDOWN 2

class SystemEventsManager {
private:
    static bool running;
    
    static long currentCallback;
    
    static std::vector<Event> events;
    
    static void prepareLoop();
    static void runLoop();
    static void stopLoop(bool = false);
    
    static void executeCallback();
    static void newProcess(void*);
public:
    static void init();
    static void destroy();
    
    static bool allEventsDisabled();
    
    static void runCallback(long);
    
    static Event getEvent(int);
    
    static void setCallback(int, long);
    static void registerCallback(int);
    static void unregisterCallback(int);
    static void prevent(int, bool);
};

#endif /* SystemEventsManager_h */
