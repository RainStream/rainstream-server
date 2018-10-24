#include "RainStream.hpp"
#include "plugins/ActiveSpeakerDetector.hpp"
#include "Room.hpp"
#include "Producer.hpp"
#include "Logger.hpp"

namespace rs
{
	const int REQUIRED_MIN_AUDIO_LEVEL = -80; // dB.
	const int REQUIRED_MAX_NUM_REPORTS = 4;

	ActiveSpeakerDetector::ActiveSpeakerDetector(Room* room)
		: _room(room)
	{
		DLOG(INFO) << "constructor()";

// 		// Bind the "audiolevels" listener so we can remove it on closure.
// 		this->_onRoomAudioLevels = this->_onRoomAudioLevels.bind(this);

		this->_handleRoom();
	}

	bool ActiveSpeakerDetector::closed()
	{
		return this->_closed;
	}

	void ActiveSpeakerDetector::close()
	{
		DLOG(INFO) << "close()";

		if (this->_closed)
			return;

		this->_closed = true;

		if (!this->_room->closed())
			this->_room->removeListener("audiolevels", std::bind(&ActiveSpeakerDetector::_onRoomAudioLevels, 
				this, std::placeholders::_1));

		//this->safeEmit("close");
	}

	void ActiveSpeakerDetector::_handleRoom()
	{
		auto room = this->_room;

		room->addEventListener("close", [=](Json data) { this->close(); });
		room->addEventListener("audiolevels", std::bind(&ActiveSpeakerDetector::_onRoomAudioLevels, 
			this, std::placeholders::_1));
	}

	void ActiveSpeakerDetector::_onRoomAudioLevels(Json levels)
	{
		if (this->_closed)
			return;

		Producer* previousActiveProducer = this->_activeProducer;
		int requiredNumReports;

// 		if (
// 			previousActiveProducer &&
// 			!previousActiveProducer->closed() &&
// 			!previousActiveProducer->paused()
// 			)
// 		{
// 			requiredNumReports = REQUIRED_MAX_NUM_REPORTS;
// 		}
// 		else
// 		{
// 			previousActiveProducer = nullptr;
// 			requiredNumReports = 1;
// 			this->_reports = [];
// 		}

// 		const reports = this->_reports;
// 
// 		// Insert the loudest audio level report into the reports array.
// 		// NOTE: This may be undefined, meaning that nobody is producing audio.
// 		reports.push(levels[0]);
// 
// 		// No more than REQUIRED_MAX_NUM_REPORTS.
// 		if (reports.length > REQUIRED_MAX_NUM_REPORTS)
// 		{
// 			reports.shift();
// 		}
// 		// No less than requiredNumReports.
// 		else if (reports.length < requiredNumReports)
// 		{
// 			this->_mayUpdateActiveSpeaker(previousActiveProducer);
// 
// 			return;
// 		}
// 
// 		// If the same Producer was the loudest speaker in the last N reports, make
// 		// it the active speaker.
// 
// 		Producer* currentActiveProducer;
// 		const numReports = reports.length;
// 
// 		for (int i = numReports - 1; i >= numReports - requiredNumReports; --i)
// 		{
// 			const report = reports[i];
// 
// 			if (!report)
// 			{
// 				currentActiveProducer = nullptr;
// 
// 				break;
// 			}
// 
// 			const { producer, audioLevel } = report;
// 
// 			if (producer.closed || producer.paused || audioLevel < REQUIRED_MIN_AUDIO_LEVEL)
// 			{
// 				currentActiveProducer = nullptr;
// 
// 				break;
// 			}
// 
// 			if (currentActiveProducer && producer != currentActiveProducer)
// 			{
// 				currentActiveProducer = nullptr;
// 
// 				break;
// 			}
// 
// 			currentActiveProducer = producer;
// 		}
// 
// 		// If there is an effective new audio Producer, make it the active one.
// 		if (currentActiveProducer)
// 		{
// 			this->_mayUpdateActiveSpeaker(currentActiveProducer);
// 		}
// 		// Otherwise check the previous active speaker.
// 		else
// 		{
// 			this->_mayUpdateActiveSpeaker(previousActiveProducer);
// 		}
	}

	void ActiveSpeakerDetector::_mayUpdateActiveSpeaker(Producer * activeProducer)
	{
// 		if (this->_activeProducer == activeProducer)
// 			return;
// 
// 		this->_activeProducer = activeProducer;
// 
// 		if (this->_activeProducer)
// 		{
// 			const peer = this->_activeProducer.peer;
// 
// 			DLOG(INFO)(
// 				"emitting \"activespeakerchange\" [peerName:\"%s\"]", peer.name);
// 
// 			this->safeEmit(
// 				"activespeakerchange", peer, this->_activeProducer);
// 		}
// 		else
// 		{
// 			DLOG(INFO)<<
// 				"emitting \"activespeakerchange\" [peerName:null]";
// 
// 			this->safeEmit("activespeakerchange", undefined, undefined);
// 		}
	}

}

