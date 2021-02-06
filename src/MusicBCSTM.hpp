#ifndef FREESHOP_MUISCBCSTM_HPP
#define FREESHOP_MUISCBCSTM_HPP

#include <string>
#include <vector>
#include <cpp3ds/Config.hpp>
#include <cpp3ds/Audio/AlResource.hpp>
#include <cpp3ds/System/Lock.hpp>
#include <cpp3ds/System/Mutex.hpp>
#include <cpp3ds/System/Thread.hpp>
#include <cpp3ds/System/FileInputStream.hpp>

#ifdef _3DS
#include <3ds.h>
#endif


namespace FreeShop {

class MusicBCSTM {
public:
	MusicBCSTM();
	~MusicBCSTM();

	bool openFromFile(const std::string &filename);

	void play();
	void pause();
	void stop();

protected:
	void streamData();

	uint8_t read8();
	uint16_t read16();
	uint32_t read32();
	bool fileAdvance(uint64_t byteSize);

	void fillBuffers();

private:
	enum RefType: uint16_t {
		ByteTable      = 0x0100,
		ReferenceTable = 0x0101,
		SampleData     = 0x1F00,
		DSPADPCMInfo   = 0x0300,
		InfoBlock      = 0x4000,
		SeekBlock      = 0x4001,
		DataBlock      = 0x4002,
		StreamInfo     = 0x4100,
		TrackInfo      = 0x4101,
		ChannelInfo    = 0x4102,
	};

	enum {
		BufferCount = 20,
	};

	bool m_isLittleEndian;
	bool m_isStreaming;
	bool m_isPaused;
	cpp3ds::Thread m_thread;
	cpp3ds::Mutex m_mutex;

	cpp3ds::FileInputStream m_file;

	bool m_looping;
	uint32_t m_channelCount;
	uint32_t m_sampleRate;

	uint32_t m_blockLoopStart;
	uint32_t m_blockLoopEnd;
	uint32_t m_blockCount;
	uint32_t m_blockSize;
	uint32_t m_blockSampleCount;
	uint32_t m_lastBlockSize;
	uint32_t m_lastBlockSampleCount;
	uint16_t m_adpcmCoefs[2][16];

	uint32_t m_currentBlock;
	uint32_t m_infoOffset;
	uint32_t m_dataOffset;

#ifdef _3DS
	u16 m_channel[2];
	ndspWaveBuf m_waveBuf[2][BufferCount];
	ndspAdpcmData m_adpcmData[2][2];
	std::vector<u8, cpp3ds::LinearAllocator<u8>> m_bufferData[2][BufferCount];
#endif
};

} // namespace FreeShop

#endif // FREESHOP_MUISCBCSTM_HPP
