//
//  Event.cpp
//  System Events
//
//  Created by Mehdi Mhiri on 18/05/2016.
//
//

#include "Event.h"

Event::Event() {
    registered = false;
    prevented = false;
    callback = -1;
}

bool Event::isRegistered() {
    return registered;
}

bool Event::isPrevented() {
    return prevented;
}

bool Event::isEnabled() {
    return registered || prevented;
}

void Event::setCallback(long methodID) {
    callback = methodID;
}

long Event::getCallback() {
    return callback;
}

void Event::registerCallback() {
    if (callback != -1)
        registered = true;
}

void Event::unregisterCallback() {
    registered = false;
}

void Event::prevent(bool prevent) {
    prevented = prevent;
}