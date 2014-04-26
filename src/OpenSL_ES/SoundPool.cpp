/*
 * SoundPool.cpp
 *
 *  Created on: Mar 13, 2014
 *      Author: dawid
 */

#include "SoundPool.h"
#include <cassert>

#include <dlfcn.h>
#include <cstdlib>
#include <string.h>
#include <vector>
#include <climits>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "Log.h"

#define MIN_VOLUME_MILLIBEL -500

#define SIZE( array ) (sizeof(array)/sizeof(array[0]))

namespace KoalaSound
{

int SoundPool::m_idGenerator = 0;

SoundPool::SoundPool ( OpenSLEngine* pSLEngine ) :
	m_samplingRate ( 0 )
	, m_bitrate ( 0 )
	, m_pEngine ( pSLEngine )
	, m_outputMixObject ( nullptr )
	, m_minVolume ( MIN_VOLUME_MILLIBEL )
	, m_maxVolume ( 0 )
{
}

SoundPool::~SoundPool()
{
	unloadStreams();

	unloadResources();
}

bool SoundPool::init ( int maxStreams, SLuint32 samplingRate, SLuint32 bitrate )
{
	KLOG ( "Initializing SoundPool" );

	if ( m_pEngine == nullptr )
	{
		KLOG ( "OpenSLEngine can't be null" );
		assert ( false );
		return false;
	}

	if ( m_pEngine->isInitialized() == false )
	{
		KLOG ( "OpenSLEngine isn't initialized" );
		assert ( false );
		return false;
	}

	if ( maxStreams < 1 )
	{
		KLOG ( "Stream count must be > 1" );
		assert ( false );
		return false;
	}

	m_samplingRate = samplingRate;
	m_bitrate = bitrate;

	// see if OpenSL library is available
	void* handle = dlopen ( "libOpenSLES.so", RTLD_LAZY );

	if ( handle == nullptr )
	{
		KLOG ( "OpenSLES not available" );
		return false;
	}

	KLOG ( "OpenSLES available" );
	KLOG ( "Initializing OpenSLEngine" );

	SLresult result = initializeBufferQueueAudioPlayer ( maxStreams );

	if ( result != SL_RESULT_SUCCESS )
	{
		KLOG ( "Error:%d -> %s", ( int ) result, getErrorMessage ( result ) );
		assert ( result == SL_RESULT_SUCCESS );

		//We can return true only if we create more than 0 streams.
		if ( m_bufferQueues.empty() )
		{
			KLOG ( "Failed to create any streams" );
			return false;
		}
	}

	return true;
}

void SoundPool::unloadStreams()
{
	for ( auto && pElement : m_bufferQueues )
	{
		delete pElement;
	}

	m_bufferQueues.clear();
}

void SoundPool::unloadResources()
{
	for ( auto && pElement : m_samples )
	{
		delete pElement;
	}

	m_samples.clear();
}

void SoundPool::play ( const Sound& sound, float volume, bool isLooped, int priority )
{
	if ( m_bufferQueues.empty() || sound.id == 0 )
	{
		//0 for invalid
		KLOG ( "Invalid sample id 0" );
		return;
	}

	assert ( volume >= 0 && volume <= 1 );
	//I use here specially this name because i don't want make mistake with sampleId (streamId)
	int positionOfStream = 0;
	KLOG ( "Play sample id: %i at volume %f -> position %d with priority %d", sound.id, volume,
		   sound.position, priority );

	// find first available buffer queue
	BufferQueue* pAvailableBuffer = nullptr;
	BufferQueue* pLowestPriority = nullptr;

	for ( positionOfStream = 0; positionOfStream < static_cast<int> ( m_bufferQueues.size() );
			++positionOfStream )
	{
		KLOG ( "Checking %d it has %d  and playing %d", positionOfStream,
			   m_bufferQueues[positionOfStream]->priority, m_bufferQueues[positionOfStream]->playingSoundId );

		if ( m_bufferQueues[positionOfStream]->playingSoundId == 0 )
		{
			pAvailableBuffer = m_bufferQueues[positionOfStream];
			break;
		}
		else if ( pLowestPriority == nullptr ||
				  pLowestPriority->priority > m_bufferQueues[positionOfStream]->priority )
		{
			KLOG ( "Lowest priority: %d", m_bufferQueues[positionOfStream]->priority );
			pLowestPriority = m_bufferQueues[positionOfStream];
		}
	}

	if ( pAvailableBuffer == nullptr )
	{
		if ( pLowestPriority != nullptr && pLowestPriority->priority <= priority )
		{
			KLOG ( "Stoping sound with lower priority id:%d  priority:%d", pLowestPriority->playingSoundId,
				   pLowestPriority->priority );
			pLowestPriority->playingSoundId = 0;
			pLowestPriority->priority = INT_MIN;
			pLowestPriority->isLooped = false;

			pAvailableBuffer = pLowestPriority;
		}
		else
		{
			KLOG ( "No channels available for playback" );
			return;
		}
	}

	KLOG ( "Playing on channel %d", positionOfStream );

	//Check our sample
	if ( sound.position > static_cast<int> ( m_samples.size() ) ||
			m_samples[sound.position] == nullptr )
	{
		KLOG ( "No such sample: %d", sound.id );
		return;
	}

	ResourceBuffer* pResource = m_samples[sound.position];

	SLresult result;

	// convert requested volume 0.0-1.0 to millibels
	SLmillibel newVolume = int ( ( m_maxVolume - m_minVolume ) * volume ) + m_minVolume;

	KLOG ( "Seting volume: %d", newVolume );
	//adjust volume for the buffer queue
	result = ( *pAvailableBuffer->volume )->SetVolumeLevel ( pAvailableBuffer->volume, newVolume );

	if ( result != SL_RESULT_SUCCESS )
	{
		KLOG ( "Error:%d -> %s", ( int ) result, getErrorMessage ( result ) );
		assert ( result == SL_RESULT_SUCCESS );
		return;
	}

	result = ( * ( pAvailableBuffer->queue ) )->Clear ( pAvailableBuffer->queue );
	assert ( SL_RESULT_SUCCESS == result );

	//enqueue the sound
	result = ( *pAvailableBuffer->queue )->Enqueue ( pAvailableBuffer->queue,
			 static_cast<void*> ( pResource->pBuffer ), pResource->size );

	if ( result != SL_RESULT_SUCCESS )
	{
		KLOG ( "Error:%d -> %s", ( int ) result, getErrorMessage ( result ) );
		assert ( result == SL_RESULT_SUCCESS );
		return;
	}

	result = ( * ( pAvailableBuffer->playerPlay ) )->SetPlayState ( pAvailableBuffer->playerPlay,
			 SL_PLAYSTATE_PLAYING );
	assert ( SL_RESULT_SUCCESS == result );

	pAvailableBuffer->playingSoundId = sound.id;
	pAvailableBuffer->priority = priority;
	pAvailableBuffer->isLooped = isLooped;
	pAvailableBuffer->pLastBuffer = pResource->pBuffer;
	pAvailableBuffer->lastSize = pResource->size;
}

Sound SoundPool::load ( char* pBuffer, int length )
{
	ResourceBuffer* pResource = new ResourceBuffer();
	pResource->pBuffer = pBuffer;
	pResource->size = length;
	m_samples.emplace_back ( pResource );

	if ( m_idGenerator == -1 )
	{
		//We don't want 0 value it is used for errors
		++m_idGenerator;
	}

	return Sound ( ++m_idGenerator, m_samples.size() - 1 );
}

void SoundPool::pauseSound ( const Sound& sound )
{
	for ( auto && pElement : m_bufferQueues )
	{
		if ( pElement->playingSoundId == sound.id )
		{
			SLresult result;
			result = ( * ( pElement->playerPlay ) )->SetPlayState ( pElement->playerPlay, SL_PLAYSTATE_PAUSED );
			KLOG ( "Pause sound on buffer position: %d   id:%d", sound.position, sound.id );
			assert ( SL_RESULT_SUCCESS == result );
		}
	}
}

void SoundPool::resumeSound ( const Sound& sound )
{
	for ( auto && pElement : m_bufferQueues )
	{
		if ( pElement->playingSoundId == sound.id )
		{
			SLresult result;
			result = ( * ( pElement->playerPlay ) )->SetPlayState ( pElement->playerPlay,
					 SL_PLAYSTATE_PLAYING );
			KLOG ( "Resume sound on buffer position: %d   id:%d", sound.position, sound.id );
			assert ( SL_RESULT_SUCCESS == result );
		}
	}
}

void SoundPool::stopSound ( const Sound& sound )
{
	for ( auto && pElement : m_bufferQueues )
	{
		if ( pElement->playingSoundId == sound.id )
		{
			SLresult result;
			result = ( * ( pElement->playerPlay ) )->SetPlayState ( pElement->playerPlay,
					 SL_PLAYSTATE_STOPPED );
			pElement->playingSoundId = 0;
			KLOG ( "Stoping sound on buffer position: %d   id:%d", sound.position, sound.id );
			assert ( SL_RESULT_SUCCESS == result );
		}
	}
}

void SoundPool::pauseAllSounds()
{
	KLOG ( "Pause all sounds" );

	for ( auto && pElement : m_bufferQueues )
	{
		if ( pElement->playingSoundId == 0 )
		{
			continue;
		}

		SLresult result;
		result = ( * ( pElement->playerPlay ) )->SetPlayState ( pElement->playerPlay, SL_PLAYSTATE_PAUSED );
		assert ( SL_RESULT_SUCCESS == result );
	}
}

void SoundPool::resumeAllSounds()
{
	KLOG ( "Resume all sounds" );

	for ( auto && pElement : m_bufferQueues )
	{
		if ( pElement->playingSoundId == 0 )
		{
			continue;
		}

		SLresult result;
		result = ( * ( pElement->playerPlay ) )->SetPlayState ( pElement->playerPlay,
				 SL_PLAYSTATE_PLAYING );
		assert ( SL_RESULT_SUCCESS == result );
	}
}


void SoundPool::stopAllSounds()
{
	KLOG ( "Stoping all sounds" );

	for ( auto && pElement : m_bufferQueues )
	{
		if ( pElement->playingSoundId == 0 )
		{
			continue;
		}

		SLresult result;
		result = ( * ( pElement->playerPlay ) )->SetPlayState ( pElement->playerPlay,
				 SL_PLAYSTATE_STOPPED );
		pElement->playingSoundId = 0;
		assert ( SL_RESULT_SUCCESS == result );
	}
}

SLresult SoundPool::initializeBufferQueueAudioPlayer ( int maxStreams )
{
	KLOG ( "Initializing BufferQueueAudioPlayer" );
	SLresult result;

	// configure audio source
	SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
	SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, 1, m_samplingRate, m_bitrate, m_bitrate,
								   SL_SPEAKER_FRONT_CENTER , SL_BYTEORDER_LITTLEENDIAN
								  };

	SLDataSource audioSource = {&loc_bufq, &format_pcm};

	// create audio player
	const SLInterfaceID player_ids[] = {SL_IID_BUFFERQUEUE, SL_IID_PLAY, SL_IID_VOLUME};
	const SLboolean player_req[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

	static_assert ( SIZE ( player_ids ) == SIZE ( player_req ), "Set on both values" );

	KLOG ( "Creating %i streams", maxStreams );

	for ( int i = 0; i < maxStreams; ++i )
	{
		BufferQueue* pBufferQueue = new BufferQueue();

		// configure audio sink
		SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, m_pEngine->getOutputMixObject() };
		SLDataSink audioSnk = {&loc_outmix, NULL};

		//SLVolumeItf volume;
		KLOG ( "Creating SLAndroidSimpleBufferQueueItf" );

		result = ( *m_pEngine->getEngine() )->CreateAudioPlayer ( m_pEngine->getEngine(),
				 &pBufferQueue->player, &audioSource, &audioSnk, SIZE ( player_ids ), player_ids, player_req );

		if ( result != SL_RESULT_SUCCESS )
		{
			delete pBufferQueue;
			KLOG ( "Can't create more players" );
			break;
		}

		result = pBufferQueue->realize();

		if ( result != SL_RESULT_SUCCESS )
		{
			delete pBufferQueue;
			KLOG ( "Can't realize player" );
			break;
		}

		result = ( *pBufferQueue->playerPlay )->SetPlayState ( pBufferQueue->playerPlay,
				 SL_PLAYSTATE_PLAYING );

		if ( result != SL_RESULT_SUCCESS )
		{
			delete pBufferQueue;
			KLOG ( "Can't set player state" );
			break;
		}

		m_bufferQueues.push_back ( pBufferQueue );
		KLOG ( "Created stream %d", i );
	}

	if ( m_bufferQueues.empty() )
	{
		return result;
	}

	assert ( m_bufferQueues.empty() == false );
	BufferQueue* pBufferQueue = m_bufferQueues.front();

	KLOG ( "Getting max volume level" );
	result = ( *pBufferQueue->volume )->GetMaxVolumeLevel ( pBufferQueue->volume, &m_maxVolume );

	if ( result != SL_RESULT_SUCCESS )
	{
		return result;
	}

	KLOG ( "createBufferQueueAudioPlayer done" );
	return result;
}

SLresult BufferQueue::realize()
{
	SLresult result;
	// realize the player
	KLOG ( "Realize player" );
	result = ( *player )->Realize ( player, SL_BOOLEAN_FALSE );

	if ( result != SL_RESULT_SUCCESS )
	{
		return result;
	}

	// get the play interface
	KLOG ( "Get Player interface" );
	result = ( *player )->GetInterface ( player, SL_IID_PLAY, &playerPlay );

	if ( result != SL_RESULT_SUCCESS )
	{
		return result;
	}

	// get the buffer queue interface
	KLOG ( "Get buffer queue interface" );
	result = ( *player )->GetInterface ( player, SL_IID_BUFFERQUEUE, &queue );

	if ( result != SL_RESULT_SUCCESS )
	{
		return result;
	}

	// register callback on the buffer queue
	KLOG ( "Register callback for buffer queue" );
	result = ( *queue )->RegisterCallback ( queue, BufferQueue::playerCallback, this );

	if ( result != SL_RESULT_SUCCESS )
	{
		return result;
	}

	KLOG ( "Get interface for volume" );
	result = ( *player )->GetInterface ( player, SL_IID_VOLUME, &volume );

	return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

ResourceBuffer::ResourceBuffer() :
	pBuffer ( nullptr )
	, size ( 0 )
{
}

ResourceBuffer::~ResourceBuffer()
{
	assert ( pBuffer != nullptr );
	free ( pBuffer );
	pBuffer = nullptr;
	size = 0;
}

BufferQueue::BufferQueue() :
	queue ( nullptr )
	, player ( nullptr )
	, playerPlay ( nullptr )
	, volume ( nullptr )
	, playingSoundId ( 0 )
	, priority ( INT_MIN )
	, isLooped ( false )
	, pLastBuffer ( nullptr )
	, lastSize ( 0 )
{
}

BufferQueue::~BufferQueue()
{
	SLresult result;

	if ( playerPlay != nullptr )
	{
		result = ( * ( playerPlay ) )->SetPlayState ( playerPlay, SL_PLAYSTATE_STOPPED );
		assert ( SL_RESULT_SUCCESS == result );
	}

	if ( player != nullptr )
	{
		( *player )->Destroy ( player );
	}

	player = nullptr;
	playerPlay = nullptr;
	queue = nullptr;
	volume = nullptr;
	isLooped = false;
	playingSoundId = 0;
	priority = INT_MIN;
}

void BufferQueue::playerCallback ( SLBufferQueueItf bufferQueue, void* pContext )
{
	assert ( pContext );
	BufferQueue* pBufferContext = static_cast<BufferQueue*> ( pContext );
	KLOG ( "Playing ended %d with priority %d", pBufferContext->playingSoundId ,
		   pBufferContext->priority );

	if ( pBufferContext->isLooped )
	{
		//enqueue the sound
		SLresult result = ( *pBufferContext->queue )->Enqueue ( pBufferContext->queue,
						  static_cast<void*> ( pBufferContext->pLastBuffer ), pBufferContext->lastSize );
		assert ( result == SL_RESULT_SUCCESS );

		result = ( * ( pBufferContext->playerPlay ) )->SetMarkerPosition ( pBufferContext->playerPlay, 0 );
		assert ( SL_RESULT_SUCCESS == result );
		result = ( *pBufferContext->playerPlay )->SetPlayState ( pBufferContext->playerPlay,
				 SL_PLAYSTATE_PLAYING );
		assert ( SL_RESULT_SUCCESS == result );
	}
	else
	{
		pBufferContext->playingSoundId = 0;
		pBufferContext->priority = INT_MIN;
	}
}

} /* namespace KoalaSound */
