//
//  EventEmitter.hpp
//  EventEmitter
//
//  Created by BlueCocoa on 2016/8/6.
//  Copyright © 2016 BlueCocoa. All rights reserved.
//

#ifndef EVENTEMITTER_HPP
#define EVENTEMITTER_HPP

#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>
#include <algorithm> //std::forEach()
#include <Functor.hpp>

class EventEmitter {
public:
    /**
     *  @brief EventListener
     *
     *  @note  Callable Function, Call once
     */
    using EventListener = std::tuple<Functor *, bool>;
    
    /**
     *  @brief Deconstructor
     */
    ~EventEmitter() {
        std::for_each(events.begin(), events.end(), [](std::pair<std::string, std::vector<EventListener>> pair) {
            std::vector<EventListener>& listeners = pair.second;
            std::for_each(listeners.begin(), listeners.end(), [](EventListener& listener) {
                delete std::get<0>(listener);
            });
        });
        events.clear();
    }
    
    /**
     *  @brief Event setter
     *
     *  @param event  Event name
     *  @param lambda Callback function when event emitted
     */
    template <typename Function>
    void on(const std::string& event, Function&& lambda) {
        std::unique_lock<std::recursive_mutex> locker(_events_mtx);
        events[event].emplace_back(new Functor{std::forward<Function>(lambda)}, false);
    }
    
    /**
     *  @brief Once event
     *
     *  @param event  Event name
     *  @param lambda Callback function when event emitted
     */
    template <typename Function>
    void once(const std::string& event, Function&& lambda) {
        std::unique_lock<std::recursive_mutex> locker(_events_mtx);
        events[event].emplace_back(new Functor{std::forward<Function>(lambda)}, true);
    }
    
    /**
     *  @brief Event emitter
     *
     *  @param event  Event name
     */
    template <typename ... Arg>
    void emit(const std::string& event, Arg&& ... args) {
        std::unique_lock<std::recursive_mutex> locker(_events_mtx);
        std::vector<EventListener> listeners = events[event];
        std::vector<std::vector<EventListener>::iterator> once_listener;
        for (auto listener = listeners.begin(); listener != listeners.end(); listener++) {
            Functor * on = std::get<0>(*listener);
            bool once = std::get<1>(*listener);
            if (on) {
                (*on) (std::forward<Arg>(args)...);
                if (once) {
                    delete on;
                    std::get<0>(*listener) = nullptr;
                    once_listener.emplace_back(listener);
                }
            }
        }
        std::for_each(once_listener.begin(), once_listener.end(), [&listeners](std::vector<EventListener>::iterator& iterator) {
            listeners.erase(iterator);
        });
        listeners.shrink_to_fit();

		//may event has been removed
		if (events.count(event))
		{
			events[event] = listeners;
		}
    }
    
    /**
     *  @brief Number of listeners
     *
     *  @param event  Event name
     */
    size_t listener_count(const std::string& event) {
        std::unique_lock<std::recursive_mutex> locker(_events_mtx);
        auto event_listeners = events.find(event);
        if (event_listeners == events.end()) return 0;
        return events[event].size();
    }

	void removeAllListeners(std::string event)
	{
		std::unique_lock<std::recursive_mutex> locker(_events_mtx);

		events.erase(event);
	}

    
protected:
    /**
     *  @brief Constructor
     */
    EventEmitter() {
    };
    
    /**
     *  @brief Event name - EventListener
     */
    std::map<std::string, std::vector<EventListener>> events;
    
protected:
    /**
     *  @brief Mutex for events
     */
	std::recursive_mutex _events_mtx;
};

#endif /* EVENTEMITTER_HPP */