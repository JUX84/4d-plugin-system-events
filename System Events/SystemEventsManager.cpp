//
//  SystemEventsManager.cpp
//  System Events
//
//  Created by Mehdi Mhiri on 18/05/2016.
//
//

#include <thread>
#include <vector>

#include "4DPluginAPI.h"

#if VERSIONWIN
#include <Windows.h>
#else
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>
#endif

#include "SystemEventsManager.h"
#include "Event.h"

bool SystemEventsManager::running;
long SystemEventsManager::currentCallback;
std::vector<Event> SystemEventsManager::events;

#if VERSIONWIN
void SystemEventsManager::runLoop() {}
void systemEventCallback() {}
void SystemEventsManager::stopLoop(bool forceStop) {}
#else
io_connect_t rootPort;
IONotificationPortRef notifyPortRef;
io_object_t notifierObject;

void systemEventCallback(void* refCon,
                         io_service_t service,
                         natural_t messageType,
                         void* messageArgument) {
    Event event;
    switch (messageType) {
        case kIOMessageCanSystemSleep:
            event = SystemEventsManager::getEvent(SYSTEM_SLEEP);
            
            if (event.isRegistered())
                SystemEventsManager::runCallback(event.getCallback());
            
            if (event.isPrevented())
                IOCancelPowerChange(rootPort, (long)messageArgument);
            else
                IOAllowPowerChange(rootPort, (long)messageArgument);
            break;
        case kIOMessageSystemWillSleep:
            IOAllowPowerChange(rootPort, (long)messageArgument);
            break;
        case kIOMessageSystemWillPowerOn:
            break;
        case kIOMessageSystemHasPoweredOn:
            event = SystemEventsManager::getEvent(SYSTEM_WAKE);
            if (event.isRegistered())
                SystemEventsManager::runCallback(event.getCallback());
            break;
        default:
            break;
    }
}

void SystemEventsManager::runLoop() {
    void* refCon = nullptr;
    
    rootPort = IORegisterForSystemPower(refCon,
                                        &notifyPortRef,
                                        systemEventCallback,
                                        &notifierObject);
    if (rootPort != 0)
    {
        running = true;
        CFRunLoopAddSource(CFRunLoopGetCurrent(),
                           IONotificationPortGetRunLoopSource(notifyPortRef),
                           kCFRunLoopCommonModes);
        CFRunLoopRun();
    }
}

void SystemEventsManager::stopLoop(bool forceStop) {
    if (running && (forceStop || allEventsDisabled())) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(),
                              IONotificationPortGetRunLoopSource(notifyPortRef),
                              kCFRunLoopCommonModes);
        IODeregisterForSystemPower(&notifierObject);
        IOServiceClose(rootPort);
        IONotificationPortDestroy(notifyPortRef);
        
        CFRunLoopStop(CFRunLoopGetCurrent());
        
        running = false;
    }
}
#endif

void SystemEventsManager::executeCallback() {
    PA_ExecuteMethodByID(currentCallback, nullptr, 0);
}

void SystemEventsManager::newProcess(void* params) {
    PA_NewProcess((void*)executeCallback, 0, nullptr);
}

void SystemEventsManager::runCallback(long callback) {
    currentCallback = callback;
    PA_RunInMainProcess(newProcess, nullptr);
}

bool SystemEventsManager::allEventsDisabled() {
    for (int i = 0; i < events.size(); ++i) {
        if (events[i].isEnabled())
            return false;
    }
    return true;
}

void SystemEventsManager::prepareLoop() {
    if (allEventsDisabled()) {
        std::thread loop(runLoop);
        loop.detach();
    }
}

void SystemEventsManager::init() {
    running = false;
    for (int i = 0; i < 3; ++i) {
        events.push_back(Event());
    }
}

void SystemEventsManager::destroy() {
    stopLoop(true);
}

Event SystemEventsManager::getEvent(int eventID) {
    return events[eventID];
}

void SystemEventsManager::setCallback(int event, long methodID) {
    events[event].setCallback(methodID);
}

void SystemEventsManager::registerCallback(int event) {
    prepareLoop();
    events[event].registerCallback();
}

void SystemEventsManager::unregisterCallback(int event) {
    events[event].unregisterCallback();
    stopLoop();
}

void SystemEventsManager::prevent(int event, bool prevent) {
    if (prevent)
        prepareLoop();
    else
        stopLoop();
    events[event].prevent(prevent);
}