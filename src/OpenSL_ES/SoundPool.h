/*
 * SoundPool.h
 *
 *  Created on: Mar 13, 2014
 *      Author: dawid
 */

#ifndef SOUNDPOOL_H_
#define SOUNDPOOL_H_

#include <vector>

#include "OpenSLEngine.h"

namespace KoalaSound
{
struct Sound
{
	friend class SoundPool;

public:
	static Sound invalidSound()
	{
		return Sound ( 0, 0 );
	}

private:

	Sound ( int id, int position ) :
		id ( id )
		, position ( position )
	{
	}

	int id;
	int position;
};

class ResourceBuffer;
class BufferQueue;

class SoundPool
{
public:
	friend class BufferQueue;

	SoundPool ( OpenSLEngine* pSLEngine );

	/**
	 * This should be called only once
	 * @param maxStreams maximum number of streams used to play. This is only suggested streams count
	 * 			because maybe we can't create so much streams. You can check that if you getMaxStreams().
	 * 			On android you have limit for 32 audio player so you probably will have ~25 max.
	 * @param samplingRate
	 * @param bitrate
	 * @return true if everything is ok, false otherwise
	 */
	bool init ( int maxStreams, SLuint32 samplingRate = SL_SAMPLINGRATE_44_1,
				SLuint32 bitrate = SL_PCMSAMPLEFORMAT_FIXED_16 );

	void unloadStreams();
	void unloadResources();

	~SoundPool();

	//We want block them
	SoundPool ( SoundPool const& ) = delete;
	void operator= ( SoundPool const& ) = delete;

	/**
	 * Play sound in this pool
	 * @param sampleId this is returned by load method. If sampleId == 0 it do nothing and 0 is returned.
	 * @param volume volume in range [0,1]
	 * @param priority priority of our sound. INT_MAX is the highest priority. Sound with lowest or equal priority
	 * 			will be stopped if we don't have any free audio player.
	 * @return stream id on which sound is played. 0 is returned if any error occurs.
	 */
	void play ( const Sound& sound, float volume, bool isLooped = false , int priority = 0 );

	/**
	 * @param pBuffer
	 * @param length
	 * @return sample id which is used to other actions on this sound pool. 0 is returned if any error occurs.
	 * 			0 is invalid sample id and it won't be played
	 */
	Sound load ( char* pBuffer, int length );

	/**
	 * @return maximum streams count. This can be different value that you pass in init method. Even 0!
	 */
	inline int getMaxStreams() const
	{
		return m_bufferQueues.size();
	}

	void pauseSound ( const Sound& sound );
	void resumeSound ( const Sound& sound );
	void stopSound ( const Sound& sound );

	void pauseAllSounds();
	void resumeAllSounds();
	void stopAllSounds();

private:
	SLuint32 m_samplingRate;
	SLuint32 m_bitrate;

	// engine
	OpenSLEngine* m_pEngine;

	// output mix interfaces
	SLObjectItf m_outputMixObject;

	// device specific min and max volumes
	SLmillibel m_minVolume;
	SLmillibel m_maxVolume;

	static int m_idGenerator;

	// vector for BufferQueues (one for each channel)
	std::vector<BufferQueue*> m_bufferQueues;

	// vector for samples
	std::vector<ResourceBuffer*> m_samples;

	SLresult initializeBufferQueueAudioPlayer ( int maxStreams );
};

class ResourceBuffer
{
public:
	ResourceBuffer();
	~ResourceBuffer();
	char* pBuffer;
	int size;
};

class BufferQueue
{
public:
	BufferQueue();
	~BufferQueue();
	SLBufferQueueItf queue;
	SLObjectItf player;
	SLPlayItf playerPlay;
	SLVolumeItf volume;
	/**
	 * playingSoundId is set to 0 if no sound is playing. If there is other value than 0 it means
	 * that sound with this ID is played
	 */
	int playingSoundId;
	/**
	 * Priority of currently played audio. Default INT_MIN
	 */
	int priority;
	bool isLooped;
	char* pLastBuffer;
	int lastSize;

	SLresult realize();

	static void playerCallback ( SLBufferQueueItf bufferQueue, void* pContext );
};

} /* namespace KoalaSound */

#endif /* SOUNDPOOL_H_ */
