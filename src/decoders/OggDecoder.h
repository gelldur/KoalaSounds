/*
 * OggDecoder.h
 *
 *  Created on: Mar 14, 2014
 *      Author: dawid
 */

#ifndef OGGLOADER_H_
#define OGGLOADER_H_

#include <vector>

#include "ogg/ogg.h"

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <cstring>
#include <sstream>

#include <vorbis/codec.h>

#include "Log.h"

namespace KoalaSound
{

struct Data
{
	Data() :
		pData( nullptr )
		, size( 0 )
		, channelsCount( 0 )
		, bitrate( 0 )
	{
	}
	/**
	 * Decoded data
	 */
	char* pData;
	/**
	 * Size of decoded data
	 */
	size_t size;

	/**
	 * Channels count
	 */
	int channelsCount;

	/**
	 * Bitrate
	 */
	int bitrate;
};

class OggDecoder
{
public:
	OggDecoder();
	~OggDecoder();

	/**
	 * Decode .ogg file.
	 * @param pData encoded ogg file data. Simply read all file to buffer and pass it here.
	 * @param size size of the buffer ( ogg file size)
	 * @return decoded ogg as PCM in simple structure. If any error occurs empty Data structure is returned (Data::pData i nullptr , Data::size == 0...)
	 */
	Data decode( const char* pData, size_t size );

private:
	const int m_convertBufferSize;
	/* take 8k (4096) out of the data segment, not the stack */
	ogg_int16_t* m_convertBuffer;
};

} /* namespace KoalaSound */

#endif /* OGGLOADER_H_ */
