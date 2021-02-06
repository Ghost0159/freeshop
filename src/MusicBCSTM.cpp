#include <iostream>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileInputStream.hpp>
#include <cpp3ds/System/Sleep.hpp>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cpp3ds/Window.hpp>
#include "MusicBCSTM.hpp"


namespace FreeShop {

MusicBCSTM::MusicBCSTM()
: m_thread(&MusicBCSTM::streamData, this)
, m_isStreaming (false)
, m_isPaused    (false)
, m_channelCount(0)
{
	m_thread.setPriority(0x1A);
}

MusicBCSTM::~MusicBCSTM()
{
	stop();
}

bool MusicBCSTM::openFromFile(const std::string &filename)
{
	if (!cpp3ds::Service::isEnabled(cpp3ds::Audio))
		return false;

	stop();

	if (!m_file.open(filename))
		return false;

	m_isLittleEndian = true; // Default to true so initial reads are not swapped

	// Verify header magic value and get endianness
	cpp3ds::Uint32 magic = read32();
	if (!(m_isLittleEndian = (read16() == 0xFEFF)))
		magic = htonl(magic);
	if (magic != 0x4D545343) // "CSTM"
		return false;

	//
	m_file.seek(0x10);
	uint16_t sectionBlockCount = read16();

	read16();

	m_dataOffset = 0;
	m_infoOffset = 0;

	for (int i = 0; i < sectionBlockCount; i++)
	{
		int sec = read16();
		read16(); // padding
		uint32_t off = read32();
		read32(); // size

		if (sec == InfoBlock)
			m_infoOffset = off;
		else if (sec == DataBlock)
			m_dataOffset = off;
	}

	if (!m_infoOffset || !m_dataOffset)
		return false;

	m_file.seek(m_infoOffset + 0x20);
	if (read8() != 2)
	{
		cpp3ds::err() << "Encoding isn't DSP ADPCM: " << filename << std::endl;
		return false;
	}

	m_looping = read8();
	m_channelCount = read8();
	if (m_channelCount > 2)
	{
		cpp3ds::err() << "Channel count (" << m_channelCount << ") isn't supported: " << filename << std::endl;
		return false;
	}

	m_file.seek(m_infoOffset + 0x24);
	m_sampleRate = read32();
	uint32_t loopPos = read32();
	uint32_t loopEnd = read32();
	m_blockCount = read32();
	m_blockSize = read32();
	m_blockSampleCount = read32();
	read32(); // last block used bytes
	m_lastBlockSampleCount = read32();
	m_lastBlockSize = read32();

	m_blockLoopStart = loopPos / m_blockSampleCount;
	m_blockLoopEnd = (loopEnd % m_blockSampleCount ? m_blockCount : loopEnd / m_blockSampleCount);

	while (read32() != ChannelInfo);
	fileAdvance(read32() + m_channelCount*8 - 12);

#ifdef _3DS
	// Get ADPCM data
	for (int i = 0; i < m_channelCount; i++)
	{
		m_file.read(m_adpcmCoefs[i], sizeof(uint16_t) * 16);
		m_file.read(&m_adpcmData[i][0], sizeof(ndspAdpcmData)); // Beginning Context
		m_file.read(&m_adpcmData[i][1], sizeof(ndspAdpcmData)); // Loop context
		read16(); // skip padding
	}
#endif

	m_currentBlock = 0;
	m_file.seek(m_dataOffset + 0x20);

	return true;
}

void MusicBCSTM::play()
{
	if (m_isPaused)
	{
#ifdef _3DS
		for (int i = 0; i < m_channelCount; ++i)
			ndspChnSetPaused(m_channel[i], false);
#endif
		m_isPaused = false;
		return;
	}

	if (m_isStreaming)
		stop();

#ifdef _3DS
	for (int i = 0; i < m_channelCount; ++i)
	{
		{
			cpp3ds::Lock lock(cpp3ds::g_activeNdspChannelsMutex);
			m_channel[i] = 0;
			while (m_channel[i] < 24 && ((cpp3ds::g_activeNdspChannels >> m_channel[i]) & 1))
				m_channel[i]++;
			if (m_channel[i] == 24)
			{
				cpp3ds::err() << "Failed to play audio stream: all channels are in use." << std::endl;
				return;
			}
			cpp3ds::g_activeNdspChannels |= 1 << m_channel[i];
			ndspChnWaveBufClear(m_channel[i]);
		}

		static float mix[16];
		ndspChnSetFormat(m_channel[i], NDSP_FORMAT_ADPCM | NDSP_3D_SURROUND_PREPROCESSED);
		ndspChnSetRate(m_channel[i], m_sampleRate);

		if (m_channelCount == 1)
			mix[0] = mix[1] = 0.5f;
		else if (m_channelCount == 2)
		{
			if (i == 0)
			{
				mix[0] = 0.8f;
				mix[1] = 0.0f;
				mix[2] = 0.2f;
				mix[3] = 0.0f;
			} else
			{
				mix[0] = 0.0f;
				mix[1] = 0.8f;
				mix[2] = 0.0f;
				mix[3] = 0.2f;
			}
		}
		ndspChnSetMix(m_channel[i], mix);
		ndspChnSetAdpcmCoefs(m_channel[i], m_adpcmCoefs[i]);

		for (int j = 0; j < BufferCount; j ++)
		{
			memset(&m_waveBuf[i][j], 0, sizeof(ndspWaveBuf));
			m_waveBuf[i][j].status = NDSP_WBUF_DONE;

			m_bufferData[i][j].resize(m_blockSize);
		}
	}
#endif

	m_isStreaming = true;

	m_thread.launch();
}

void MusicBCSTM::pause()
{
	cpp3ds::Lock lock(m_mutex);
	if (!m_isStreaming)
		return;
	m_isPaused = true;

#ifdef _3DS
	for (int i = 0; i < m_channelCount; ++i)
		ndspChnSetPaused(m_channel[i], true);
#endif
}

void MusicBCSTM::stop()
{
	if (!m_isStreaming)
		return;

	{
		cpp3ds::Lock lock(m_mutex);
		m_isStreaming = false;
	}

	m_thread.wait();

#ifdef _3DS
	cpp3ds::Lock lock(cpp3ds::g_activeNdspChannelsMutex);
	for (int i = 0; i < m_channelCount; ++i)
	{
		ndspChnWaveBufClear(m_channel[i]);
		cpp3ds::g_activeNdspChannels &= ~(1 << m_channel[i]);
	}
#endif
}

void MusicBCSTM::streamData()
{
	bool isPaused = false;

	while (1)
	{
		{
			cpp3ds::Lock lock(m_mutex);
			isPaused = m_isPaused;
			if (!m_isStreaming)
				break;
		}

		if (!isPaused)
		{
			fillBuffers();
		}

		cpp3ds::sleep(cpp3ds::milliseconds(100));
	}
}

uint32_t MusicBCSTM::read32()
{
	uint32_t v;
	m_file.read(&v, sizeof(v));
	return (m_isLittleEndian ? v : htonl(v));
}

uint16_t MusicBCSTM::read16()
{
	uint16_t v;
	m_file.read(&v, sizeof(v));
	return (m_isLittleEndian ? v : htons(v));
}

uint8_t MusicBCSTM::read8()
{
	uint8_t v;
	m_file.read(&v, sizeof(v));
	return v;
}

void MusicBCSTM::fillBuffers()
{
#ifdef _3DS
	for (int bufIndex = 0; bufIndex < BufferCount; ++bufIndex)
	{
		if (m_waveBuf[0][bufIndex].status != NDSP_WBUF_DONE)
			continue;
		if (m_channelCount == 2 && m_waveBuf[1][bufIndex].status != NDSP_WBUF_DONE)
			continue;

		if (m_currentBlock == m_blockLoopEnd)
		{
			m_currentBlock = m_blockLoopStart;
			m_file.seek(m_dataOffset + 0x20 + m_blockSize*m_channelCount*m_blockLoopStart);
		}

		for (int channelIndex = 0; channelIndex < m_channelCount; ++channelIndex)
		{
			ndspWaveBuf *buf = &m_waveBuf[channelIndex][bufIndex];

			memset(buf, 0, sizeof(ndspWaveBuf));
			buf->data_adpcm = m_bufferData[channelIndex][bufIndex].data();

			m_file.read(buf->data_adpcm, (m_currentBlock == m_blockCount-1) ? m_lastBlockSize : m_blockSize);
			DSP_FlushDataCache(buf->data_adpcm, m_blockSize);

			if (m_currentBlock == 0)
				buf->adpcm_data = &m_adpcmData[channelIndex][0];
			else if (m_currentBlock == m_blockLoopStart)
				buf->adpcm_data = &m_adpcmData[channelIndex][1];

			if (m_currentBlock == m_blockCount-1)
				buf->nsamples = m_lastBlockSampleCount;
			else
				buf->nsamples = m_blockSampleCount;

			ndspChnWaveBufAdd(m_channel[channelIndex], buf);
		}

		m_currentBlock++;
	}
#endif
}

bool MusicBCSTM::fileAdvance(uint64_t byteSize)
{
	uint64_t seekPosition = m_file.tell() + byteSize;
	return (m_file.seek(seekPosition) == seekPosition);
}


} // namespace FreeShop
