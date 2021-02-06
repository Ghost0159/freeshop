#ifndef FREESHOP_MUSICMP3_HPP
#define FREESHOP_MUSICMP3_HPP

#include <string>
#include <vector>
#include <cpp3ds/Config.hpp>
#include <cpp3ds/Audio/AlResource.hpp>
#include <cpp3ds/System/Lock.hpp>
#include <cpp3ds/System/Mutex.hpp>
#include <cpp3ds/System/Thread.hpp>
#include <cpp3ds/System/FileInputStream.hpp>
#include <mpg123.h>

#ifdef _3DS
#include <3ds.h>
#endif


namespace FreeShop {

class MusicMP3 {
public:
	MusicMP3();
	~MusicMP3();

	bool openFromFile(const std::string &filename);

	void play(bool launchThread = true);
	void pause();
	void stop();

	bool getSongFinished();

protected:
	void streamData();

	void fillBuffers();

private:
	bool m_isLittleEndian;
	bool m_isStreaming;
	bool m_isPaused;
	bool m_isStopped;
	bool m_isSongFinished;
	cpp3ds::Thread m_thread;
	cpp3ds::Mutex m_mutex;

	cpp3ds::FileInputStream m_file;
	std::string m_fileName;

	bool m_looping;
	uint32_t m_channelCount;
	uint32_t m_sampleRate;

#ifdef _3DS
	u16 m_channel[2];
	ndspWaveBuf m_waveBuf[2];
	ndspAdpcmData m_adpcmData[2][2];
	std::vector<u8> m_bufferData[2];

	uint64_t decodeMP3(void* buffer);
#endif
};

} // namespace FreeShop

#endif // FREESHOP_MUSICMP3_HPP
