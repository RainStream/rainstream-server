#pragma once

#include "EventEmitter.hpp"

namespace rs
{
	class Room;
	class Producer;

	class ActiveSpeakerDetector : public EventEmitter
	{
	public:
		ActiveSpeakerDetector(Room* room);

		bool closed();

		void close();

		void _handleRoom();

		void _onRoomAudioLevels(Json levels);

		void _mayUpdateActiveSpeaker(Producer* activeProducer);

	private:
		// Closed flag.
		// @type {Boolean}
		bool _closed = false;

		// Handled Room.
		// @type {Room}
		Room* _room{ nullptr };

		// Latest max audio level reports.
		// @type {Array}
		std::vector<int> _reports;

		// Current active Producer.
		// @type {Producer}
		Producer* _activeProducer{ nullptr };

	};
}
