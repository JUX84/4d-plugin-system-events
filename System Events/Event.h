//
//  Event.hpp
//  System Events
//
//  Created by Mehdi Mhiri on 18/05/2016.
//
//

#ifndef Event_hpp
#define Event_hpp

class Event {
private:
    bool registered;
    bool prevented;
    
    long callback;
    
public:
    Event();
    
    bool isRegistered();
    bool isPrevented();
    
    bool isEnabled();
    
    void setCallback(long);
    long getCallback();
    
    void registerCallback();
    void unregisterCallback();
    
    void prevent(bool);
};

#endif /* Event_hpp */
