/*
 * OggDecoder.cpp
 *
 *  Created on: Mar 14, 2014
 *      Author: dawid
 */

#include "decoders/OggDecoder.h"

#include <vorbis/vorbisfile.h>

namespace KoalaSound
{

OggDecoder::OggDecoder() :
	m_convertBufferSize( 4096 )
	, m_convertBuffer( new ogg_int16_t[m_convertBufferSize] )
{
	static_assert( sizeof( unsigned short ) == 2, "Wrong size!" );
	static_assert( sizeof( signed int ) == 4, "Wrong size!" );
	static_assert( sizeof( unsigned int ) == 4, "Wrong size!" );
	static_assert( sizeof( long long int ) == 8, "Wrong size!" );
}

OggDecoder::~OggDecoder()
{
	delete[] m_convertBuffer;
}

Data OggDecoder::decode( const char* pData, size_t size )
{
	/*
	 * This source code is from: http://svn.xiph.org/trunk/vorbis/examples/decoder_example.c
	 * Little bit modified for reading and writing data from buffers also refactored.
	 */
	ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
	ogg_stream_state os; /* take physical pages, weld into a logical
	                          stream of packets */
	ogg_page         og; /* one Ogg bitstream page. Vorbis packets are inside */
	ogg_packet       op; /* one raw packet of data for decode */

	vorbis_info      vi; /* struct that stores all the static vorbis bitstream
	                          settings */
	vorbis_comment   vc; /* struct that stores all the bitstream user comments */
	vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
	vorbis_block     vb; /* local working space for packet->PCM decode */

	Data outputData;//Out output data

	std::stringstream encodedStream;
	encodedStream.write( pData, size );  //write our encoded data to stream.
	encodedStream.seekg( 0, encodedStream.beg );

	std::stringstream decodedStream;

	char* buffer;
	int  bytes;

	/********** Decode setup ************/

	ogg_sync_init( &oy );  /* Now we can read pages */

	const int block4k = 4096;
	int convsize = block4k;

	while( 1 )  /* we repeat if the bitstream is chained */
	{
		int eos = 0;
		int i;

		/* grab some data at the head of the stream. We want the first page
		   (which is guaranteed to be small and only contain the Vorbis
		   stream initial header) We need the first page to get the stream
		   serialno. */


		/* submit a 4k block to libvorbis' Ogg layer */
		buffer = ogg_sync_buffer( &oy, block4k );

		encodedStream.read( buffer, block4k );
		bytes = encodedStream.gcount();

		ogg_sync_wrote( &oy, bytes );

		/* Get the first page. */
		if( ogg_sync_pageout( &oy, &og ) != 1 )
		{
			/* have we simply run out of data?  If so, we're done. */
			if( bytes < block4k ) { break; }

			/* error case.  Must not be Vorbis data */
			KLOG( "Input does not appear to be an Ogg bitstream.\n" );
			assert( false );
			return Data();
		}

		/* Get the serial number and set up the rest of decode. */
		/* serialno first; use it to set up a logical stream */
		ogg_stream_init( &os, ogg_page_serialno( &og ) );

		/* extract the initial header from the first page and verify that the
		   Ogg bitstream is in fact Vorbis data */

		/* I handle the initial header first instead of just having the code
		   read all three Vorbis headers at once because reading the initial
		   header is an easy way to identify a Vorbis bitstream and it's
		   useful to see that functionality seperated out. */

		vorbis_info_init( &vi );
		vorbis_comment_init( &vc );

		if( ogg_stream_pagein( &os, &og ) < 0 )
		{
			/* error; stream version mismatch perhaps */
			KLOG( "Error reading first page of Ogg bitstream data.\n" );
			assert( false );
			return Data();
		}

		if( ogg_stream_packetout( &os, &op ) != 1 )
		{
			/* no page? must not be vorbis */
			KLOG( "Error reading initial header packet.\n" );
			assert( false );
			return Data();
		}

		if( vorbis_synthesis_headerin( &vi, &vc, &op ) < 0 )
		{
			/* error case; not a vorbis header */
			KLOG( "This Ogg bitstream does not contain Vorbis audio data.\n" );
			assert( false );
			return Data();
		}

		/* At this point, we're sure we're Vorbis. We've set up the logical
		   (Ogg) bitstream decoder. Get the comment and codebook headers and
		   set up the Vorbis decoder */

		/* The next two packets in order are the comment and codebook headers.
		   They're likely large and may span multiple pages. Thus we read
		   and submit data until we get our two packets, watching that no
		   pages are missing. If a page is missing, error out; losing a
		   header page is the only place where missing data is fatal. */

		i = 0;

		while( i < 2 )
		{
			while( i < 2 )
			{
				int result = ogg_sync_pageout( &oy, &og );

				if( result == 0 ) { break; }  /* Need more data */

				/* Don't complain about missing or corrupt data yet. We'll
				   catch it at the packet output phase */
				if( result == 1 )
				{
					ogg_stream_pagein( &os, &og ); /* we can ignore any errors here
	                                         as they'll also become apparent
	                                         at packetout */

					while( i < 2 )
					{
						result = ogg_stream_packetout( &os, &op );

						if( result == 0 ) { break; }

						if( result < 0 )
						{
							/* Uh oh; data at some point was corrupted or missing!
							   We can't tolerate that in a header.  Die. */
							KLOG( "Corrupt secondary header.  Exiting.\n" );
							assert( false );
							return Data();
						}

						result = vorbis_synthesis_headerin( &vi, &vc, &op );

						if( result < 0 )
						{
							KLOG( "Corrupt secondary header.  Exiting.\n" );
							assert( false );
							return Data();
						}

						i++;
					}
				}
			}

			/* no harm in not checking before adding more */
			buffer = ogg_sync_buffer( &oy, block4k );

			encodedStream.read( buffer, block4k );
			bytes = encodedStream.gcount();

			if( bytes == 0 && i < 2 )
			{
				KLOG( "End of file before finding all Vorbis headers!\n" );
				assert( false );
				return Data();
			}

			ogg_sync_wrote( &oy, bytes );
		}

		/* Throw the comments plus a few lines about the bitstream we're
		   decoding */
		{
			char** ptr = vc.user_comments;

			while( *ptr )
			{
				KLOG( "%s\n", *ptr );
				++ptr;
			}

			outputData.bitrate = vi.rate;
			outputData.channelsCount = vi.channels;

			KLOG( "\nBitstream is %d channel, %ldHz\n", vi.channels, vi.rate );
			KLOG( "Encoded by: %s\n\n", vc.vendor );
		}

		convsize = block4k / vi.channels;

		/* OK, got and parsed all three headers. Initialize the Vorbis
		   packet->PCM decoder. */
		if( vorbis_synthesis_init( &vd, &vi ) == 0 )   /* central decode state */
		{
			vorbis_block_init( &vd, &vb );          /* local state for most of the decode
	                                              so multiple block decodes can
	                                              proceed in parallel. We could init
	                                              multiple vorbis_block structures
	                                              for vd here */

			/* The rest is just a straight decode loop until end of stream */
			while( !eos )
			{
				while( !eos )
				{
					int result = ogg_sync_pageout( &oy, &og );

					if( result == 0 ) { break; }  /* need more data */

					if( result < 0 )  /* missing or corrupt data at this page position */
					{
						KLOG( "Corrupt or missing data in bitstream;\ncontinuing...\n" );
					}
					else
					{
						ogg_stream_pagein( &os, &og ); /* can safely ignore errors at
	                                           this point */

						while( 1 )
						{
							result = ogg_stream_packetout( &os, &op );

							if( result == 0 ) { break; }  /* need more data */

							if( result < 0 )  /* missing or corrupt data at this page position */
							{
								/* no reason to complain; already complained above */
							}
							else
							{
								/* we have a packet.  Decode it */
								float** pcm;
								int samples;

								if( vorbis_synthesis( &vb, &op ) == 0 )   /* test for success! */
								{
									vorbis_synthesis_blockin( &vd, &vb );
								}

								/*

								**pcm is a multichannel float vector.  In stereo, for
								example, pcm[0] is left, and pcm[1] is right.  samples is
								the size of each channel.  Convert the float values
								(-1.<=range<=1.) to whatever PCM format and write it out */

								while( ( samples = vorbis_synthesis_pcmout( &vd, &pcm ) ) > 0 )
								{
									int j;
									int clipflag = 0;
									int bout = ( samples < convsize ? samples : convsize );

									/* convert floats to 16 bit signed ints (host order) and
									   interleave */
									for( i = 0; i < vi.channels; i++ )
									{
										ogg_int16_t* ptr = m_convertBuffer + i;
										float*  mono = pcm[i];

										for( j = 0; j < bout; j++ )
										{
#if 1
											int val = floor( mono[j] * 32767.f + .5f );
#else /* optional dither */
											int val = mono[j] * 32767.f + drand48() - 0.5f;
#endif

											/* might as well guard against clipping */
											if( val > 32767 )
											{
												val = 32767;
												clipflag = 1;
											}

											if( val < -32768 )
											{
												val = -32768;
												clipflag = 1;
											}

											*ptr = val;
											ptr += vi.channels;
										}
									}

									if( clipflag )
									{
										KLOG( "Clipping in frame %ld\n", ( long )( vd.sequence ) );
									}



									decodedStream.write( reinterpret_cast<char*>( m_convertBuffer ), 2 * vi.channels * bout );

									vorbis_synthesis_read( &vd, bout ); /* tell libvorbis how
	                                                      many samples we
	                                                      actually consumed */
								}
							}
						}

						if( ogg_page_eos( &og ) ) { eos = 1; }
					}
				}

				if( !eos )
				{
					buffer = ogg_sync_buffer( &oy, block4k );
					encodedStream.read( buffer, block4k );
					bytes = encodedStream.gcount();

					ogg_sync_wrote( &oy, bytes );

					if( bytes == 0 ) { eos = 1; }
				}
			}

			/* ogg_page and ogg_packet structs always point to storage in
			   libvorbis.  They're never freed or manipulated directly */

			vorbis_block_clear( &vb );
			vorbis_dsp_clear( &vd );
		}
		else
		{
			KLOG( "Error: Corrupt header during playback initialization.\n" );
		}

		/* clean up this logical bitstream; before exit we see if we're
		   followed by another [chained] */

		ogg_stream_clear( &os );
		vorbis_comment_clear( &vc );
		vorbis_info_clear( &vi );  /* must be called last */
	}

	/* OK, clean up the framer */
	ogg_sync_clear( &oy );


	decodedStream.seekg( 0, decodedStream.end );
	outputData.size = decodedStream.tellg();
	decodedStream.seekg( 0, decodedStream.beg );

	if( outputData.size < 1 )
	{
		KLOG( "Problems with decoded stream!" );
		assert( false );
		return Data();
	}

	outputData.pData = new char[outputData.size];

	decodedStream.read( outputData.pData, outputData.size );

	KLOG( "Done.\n" );

	return outputData;
}

} /* namespace KoalaSound */
