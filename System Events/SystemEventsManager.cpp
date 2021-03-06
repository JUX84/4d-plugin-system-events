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

bool SystemEventsManager::systemEventLoopRunning;
bool SystemEventsManager::callbackLoopRunning;
long SystemEventsManager::callbackMethodID;
long SystemEventsManager::callbackProcessID;
std::vector<Event> SystemEventsManager::events;

#if VERSIONWIN
HWND hWin;

LRESULT CALLBACK systemEventCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	Event event;
	switch (uMsg)
	{
	case WM_CREATE:
		return false;

	case WM_DESTROY:
		return false;

	case WM_QUERYENDSESSION:
		event = SystemEventsManager::getEvent(SYSTEM_SHUTDOWN);
		
		if (event.isPrevented()) {
			return false;
		} else {
			if (event.isRegistered())
				SystemEventsManager::executeCallback(event.getCallback());
			return true;
		}

	case WM_POWERBROADCAST:
		switch (wParam) {
		case PBT_APMSUSPEND:
			event = SystemEventsManager::getEvent(SYSTEM_SLEEP);

			if (event.isRegistered())
				SystemEventsManager::executeCallback(event.getCallback());
			break;
		case PBT_APMRESUMEAUTOMATIC:
			event = SystemEventsManager::getEvent(SYSTEM_WAKE);

			if (event.isRegistered())
				SystemEventsManager::executeCallback(event.getCallback());
			break;
		default:
			break;
		}
		return true;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return false;
}

void SystemEventsManager::runLoop() {
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = systemEventCallback;
	const wchar_t name[] = L"SystemEvent";
	wc.lpszClassName = name;
	RegisterClass(&wc);
	hWin = CreateWindow(name, name, 0, 0, 0, 0, 0, NULL, NULL, NULL, 0);

	systemEventLoopRunning = true;
	MSG msg = { 0 };
	BOOL status;

	try {
		while ((status = GetMessage(&msg, NULL, 0, 0)) != 0)
		{
			if (status == -1) {
				break;
			}
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	} catch (...) {}
    stopLoop();
}

void SystemEventsManager::stopLoop(bool forceStop) {
	if (systemEventLoopRunning && (forceStop || allEventsDisabled())) {
		DestroyWindow(hWin);
		systemEventLoopRunning = false;
	}
}
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

			if (event.isPrevented()) {
				IOCancelPowerChange(rootPort, (long)messageArgument);
			} else {
				if (event.isRegistered())
					SystemEventsManager::executeCallback(event.getCallback());
				IOAllowPowerChange(rootPort, (long)messageArgument);
			}
            break;
        case kIOMessageSystemWillSleep:
            IOAllowPowerChange(rootPort, (long)messageArgument);
            break;
        case kIOMessageSystemWillPowerOn:
            break;
        case kIOMessageSystemHasPoweredOn:
            event = SystemEventsManager::getEvent(SYSTEM_WAKE);
            if (event.isRegistered())
                SystemEventsManager::executeCallback(event.getCallback());
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
        systemEventLoopRunning = true;
        CFRunLoopAddSource(CFRunLoopGetCurrent(),
                           IONotificationPortGetRunLoopSource(notifyPortRef),
                           kCFRunLoopCommonModes);
        CFRunLoopRun();
    }
}

void SystemEventsManager::stopLoop(bool forceStop) {
    if (systemEventLoopRunning && (forceStop || allEventsDisabled())) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(),
                              IONotificationPortGetRunLoopSource(notifyPortRef),
                              kCFRunLoopCommonModes);
        IODeregisterForSystemPower(&notifierObject);
        IOServiceClose(rootPort);
        IONotificationPortDestroy(notifyPortRef);
        
        CFRunLoopStop(CFRunLoopGetCurrent());
        
        systemEventLoopRunning = false;
    }
}
#endif

void SystemEventsManager::executeCallback(long callback) {
    callbackMethodID = callback;
    PA_UnfreezeProcess(callbackProcessID);
}

void SystemEventsManager::runCallbackLoop() {
    while (callbackLoopRunning) {
        PA_YieldAbsolute();
        PA_FreezeProcess(callbackProcessID);
        if (callbackMethodID != 0) {
            PA_ExecuteMethodByID(callbackMethodID, nullptr, 0);
        }
    }
    PA_KillProcess();
}

void SystemEventsManager::prepareCallbackLoop() {
    callbackLoopRunning = true;
    callbackProcessID = PA_NewProcess((void*)runCallbackLoop, 0, nullptr);
}

void SystemEventsManager::stopCallbackLoop() {
    callbackLoopRunning = false;
    PA_UnfreezeProcess(callbackProcessID);
}

bool SystemEventsManager::allEventsDisabled() {
    for (unsigned int i = 0; i < events.size(); ++i) {
        if (events[i].isEnabled())
            return false;
    }
    return true;
}

void SystemEventsManager::prepareLoop() {
    if (allEventsDisabled()) {
        std::thread loop(runLoop);
        loop.detach();
        prepareCallbackLoop();
    }
}

void SystemEventsManager::init() {
    callbackLoopRunning = false;
    callbackMethodID = 0;
	events.clear();
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
	if (prevent) {
		prepareLoop();
#if VERSIONWIN
		if (event == SYSTEM_SLEEP)
			SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
#endif
	}
    events[event].prevent(prevent);
	if (!prevent) {
		stopLoop();
#if VERSIONWIN
		if (event == SYSTEM_SLEEP)
			SetThreadExecutionState(ES_CONTINUOUS);
#endif
	}
}