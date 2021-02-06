#include <iostream>
#include <cpp3ds/System/I18n.hpp>
#include <cpp3ds/System/FileInputStream.hpp>
#include <cpp3ds/System/Sleep.hpp>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cpp3ds/Window.hpp>
#include "MusicMP3.hpp"
#include "Notification.hpp"

//Thanks to MaK11-12's ctrmus

#define CHANNEL	0x08

#ifndef EMULATION
	//MPG123 stuff
	size_t* m_buffSize = NULL;
	mpg123_handle *m_mh = NULL;
	int16_t* m_buffer1 = NULL;
	int16_t* m_buffer2 = NULL;
#endif

namespace FreeShop {

MusicMP3::MusicMP3()
: m_thread(&MusicMP3::streamData, this)
, m_isStreaming   (false)
, m_isPaused      (false)
, m_isStopped     (false)
, m_isSongFinished(false)
, m_channelCount  (0)
{
	m_thread.setPriority(0x1A);
}

MusicMP3::~MusicMP3()
{
	stop();
}

bool MusicMP3::openFromFile(const std::string &filename)
{
	if (!cpp3ds::Service::isEnabled(cpp3ds::Audio))
		return false;

	stop();

	if (!m_file.open(filename))
		return false;

	m_isLittleEndian = true; // Default to true so initial reads are not swapped

	//Verify header magic value
	cpp3ds::Uint32 magic;
	m_file.seek(0);
	m_file.read(&magic, 4);

	if (magic != 0x04334449 && magic != 0x6490FBFF)
		return false;

	m_fileName = filename;

#ifndef EMULATION
//Var inits
int err = 0;
int encoding = 0;
m_buffSize = static_cast<size_t*>(linearAlloc(sizeof(size_t)));
*m_buffSize = 0;

//Disable paused and stopped flags
m_isPaused = false;
m_isStopped = false;

//Init the mpg123
if(mpg123_init() != MPG123_OK)
	return false;

//Get the mpg123's handle
if((m_mh = mpg123_new(NULL, &err)) == NULL)
{
	printf("Error: %s\n", mpg123_plain_strerror(err));
	return false;
}

//Open the file and get informations from it
if(mpg123_open(m_mh, filename.c_str()) != MPG123_OK ||
		mpg123_getformat(m_mh, (long *) &m_sampleRate, (int *) &m_channelCount, &encoding) != MPG123_OK)
{
	printf("Trouble with mpg123: %s\n", mpg123_strerror(m_mh));
	return false;
}

/*
 * Ensure that this output format will not change (it might, when we allow
 * it).
 */
mpg123_format_none(m_mh);
mpg123_format(m_mh, m_sampleRate, m_channelCount, encoding);

/*
 * Buffer could be almost any size here, mpg123_outblock() is just some
 * recommendation. The size should be a multiple of the PCM frame size.
 */

*m_buffSize = mpg123_outblock(m_mh) * 16;
#endif

	return true;
}

void MusicMP3::play(bool launchThread)
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
	size_t buffSize = *m_buffSize;

	m_buffer1 = static_cast<int16_t*>(linearAlloc(buffSize * sizeof(int16_t)));
	m_buffer2 = static_cast<int16_t*>(linearAlloc(buffSize * sizeof(int16_t)));

	ndspChnReset(CHANNEL);
	ndspChnWaveBufClear(CHANNEL);
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetInterp(CHANNEL, NDSP_INTERP_POLYPHASE);
	ndspChnSetRate(CHANNEL, m_sampleRate);
	ndspChnSetFormat(CHANNEL,
			m_channelCount == 2 ? NDSP_FORMAT_STEREO_PCM16 :
			NDSP_FORMAT_MONO_PCM16);

	memset(m_waveBuf, 0, sizeof(m_waveBuf));

	m_waveBuf[0].nsamples = decodeMP3(&m_buffer1[0]) / m_channelCount;
	m_waveBuf[0].data_vaddr = &m_buffer1[0];
	ndspChnWaveBufAdd(CHANNEL, &m_waveBuf[0]);

	m_waveBuf[1].nsamples = decodeMP3(&m_buffer2[0]) / m_channelCount;
	m_waveBuf[1].data_vaddr = &m_buffer2[0];
	ndspChnWaveBufAdd(CHANNEL, &m_waveBuf[1]);
#endif

	m_isStreaming = true;
	m_isSongFinished = false;

	if (launchThread)
		m_thread.launch();
}

void MusicMP3::pause()
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

void MusicMP3::stop()
{
	if (!m_isStreaming)
		return;

	{
		cpp3ds::Lock lock(m_mutex);
		m_isStreaming = false;
		m_isStopped = true;
	}

	m_thread.wait();

#ifdef _3DS
	cpp3ds::Lock lock(cpp3ds::g_activeNdspChannelsMutex);

	ndspChnWaveBufClear(CHANNEL);

	linearFree(m_buffer1);
	linearFree(m_buffer2);

	mpg123_close(m_mh);
	mpg123_delete(m_mh);
	mpg123_exit();
#endif
}

void MusicMP3::streamData()
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

		m_isSongFinished = true;

		cpp3ds::Lock lock(m_mutex);
		m_isStreaming = false;

		if (!m_isStopped) {
#ifdef _3DS
			ndspChnWaveBufClear(CHANNEL);

			linearFree(m_buffer1);
			linearFree(m_buffer2);

			mpg123_close(m_mh);
			mpg123_delete(m_mh);
			mpg123_exit();
#endif

			openFromFile(m_fileName);
			play(false);
		} else {
			break;
		}

		cpp3ds::sleep(cpp3ds::milliseconds(100));
	}
}

void MusicMP3::fillBuffers()
{
#ifndef EMULATION
bool lastbuf = false;
bool songFinished = false;
m_isSongFinished = false;

while(!m_isStopped && !songFinished && m_isStreaming)
{
	svcSleepThread(100 * 1000);

	/* When the last buffer has finished playing, break. */
	if(lastbuf == true && m_waveBuf[0].status == NDSP_WBUF_DONE && m_waveBuf[1].status == NDSP_WBUF_DONE) {
		songFinished = true;
		break;
	}

		if(ndspChnIsPaused(CHANNEL) == true || lastbuf == true)
		continue;

		if(m_waveBuf[0].status == NDSP_WBUF_DONE)
		{
			size_t read = decodeMP3(&m_buffer1[0]);

			if(read <= 0)
			{
				lastbuf = true;
				continue;
			}
			else if(read < *m_buffSize)
			m_waveBuf[0].nsamples = read / m_channelCount;

			ndspChnWaveBufAdd(CHANNEL, &m_waveBuf[0]);
		}

		if(m_waveBuf[1].status == NDSP_WBUF_DONE)
		{
			size_t read = decodeMP3(&m_buffer2[0]);

			if(read <= 0)
			{
				lastbuf = true;
				continue;
			}
			else if(read < *m_buffSize)
			m_waveBuf[1].nsamples = read / m_channelCount;

			ndspChnWaveBufAdd(CHANNEL, &m_waveBuf[1]);
		}

		DSP_FlushDataCache(m_buffer1, *m_buffSize * sizeof(int16_t));
		DSP_FlushDataCache(m_buffer2, *m_buffSize * sizeof(int16_t));
	}
#endif
}

#ifndef EMULATION
uint64_t MusicMP3::decodeMP3(void* buffer)
{
	size_t done = 0;
	mpg123_read(m_mh, static_cast<unsigned char*>(buffer), *m_buffSize, &done);
	return done / (sizeof(int16_t));
}
#endif

bool MusicMP3::getSongFinished()
{
	return m_isSongFinished;
}

} // namespace FreeShop
