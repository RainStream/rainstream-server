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
#include <algorithm>
#include <Functor.hpp>

#define Infinity 0

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
		std::unique_lock<std::recursive_mutex> locker(_events_mtx);
		std::for_each(events.begin(), events.end(), [](std::pair<std::string, std::vector<EventListener>> pair) {
			std::vector<EventListener>& listeners = pair.second;
			std::for_each(listeners.begin(), listeners.end(), [](EventListener& listener) {
				//delete std::get<0>(listener);
				//std::get<0>(listener) = nullptr;
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
		events[event].emplace_back(new Functor{ std::forward<Function>(lambda) }, false);
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
		events[event].emplace_back(new Functor{ std::forward<Function>(lambda) }, true);
	}

	/**
	*  @brief Event emitter
	*
	*  @param event  Event name
	*/
	template <typename ... Arg>
	void emit(const std::string& event, Arg&& ... args) {
		std::unique_lock<std::recursive_mutex> locker(_events_mtx);
		auto event_listeners = events.find(event);
		if (event_listeners == events.end()) return;

		std::vector<Functor *> functors;

		std::vector<EventListener>& listeners = events[event];
		for (auto listener = listeners.begin(); listener != listeners.end();) {
			Functor * on = std::get<0>(*listener);
			bool once = std::get<1>(*listener);
			functors.push_back(on);
			
			if (once) {
				//			delete on;
				//std::get<0>(*listener) = nullptr;
				listener = listeners.erase(listener);
			}
			else
			{
				++listener;
			}
		}
		listeners.shrink_to_fit();

		for (auto &it:functors)
		{
			Functor * on = it;
			if (on) {
				(*on) (std::forward<Arg>(args)...);
			}
		}
	}

	/**
	*  @brief Number of listeners
	*
	*  @param event  Event name
	*/
	int listenerCount(const std::string& event) {
		std::unique_lock<std::recursive_mutex> locker(_events_mtx);
		auto event_listeners = events.find(event);
		if (event_listeners == events.end()) return 0;
		return events[event].size();;
	}
	/**
	*  @brief Remove all listeners
	*
	*  @param event  Event name
	*/
	void removeAllListeners(const std::string& event){
		std::unique_lock<std::recursive_mutex> locker(_events_mtx);
		std::vector<EventListener>& listeners = events[event];
		std::for_each(listeners.begin(), listeners.end(), [](EventListener& listener) {
			//delete std::get<0>(listener);
			//std::get<0>(listener) = nullptr;
		});

		listeners.clear();
		events.erase(event);
	}

	void setMaxListeners(int listenerCount)
	{

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

private:
	/**
	*  @brief Mutex for events
	*/
	std::recursive_mutex _events_mtx;
};

#endif /* EVENTEMITTER_HPP */