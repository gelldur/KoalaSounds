/*
 * OpenSlEngine.cpp
 *
 *  Created on: Mar 13, 2014
 *      Author: dawid
 */

#include "OpenSLEngine.h"
#include <cassert>

namespace KoalaSound
{

OpenSLEngine* OpenSLEngine::m_pInstance = nullptr;

OpenSLEngine::OpenSLEngine() :
	m_engineObject( nullptr )
	, m_engineEngine( nullptr )
	, m_outputMixObject( nullptr )
{
}

OpenSLEngine::~OpenSLEngine()
{
	//Order of destroy is important!
	// destroy output mix object
	if( m_outputMixObject != nullptr )
	{
		( *m_outputMixObject )->Destroy( m_outputMixObject );
		m_outputMixObject = nullptr;
	}

	// destroy engine object, and invalidate all associated interfaces
	if( m_engineObject != nullptr )
	{
		( *m_engineObject )->Destroy( m_engineObject );
		m_engineObject = nullptr;
		m_engineEngine = nullptr;
	}
}

SLresult OpenSLEngine::initializeOpenSLEngine()
{
	//We can have only 1 engine in application
	assert( m_engineObject == nullptr );
	assert( m_engineEngine == nullptr );

	if( m_engineEngine != nullptr || m_engineObject != nullptr )
	{
		return SL_RESULT_OPERATION_ABORTED;
	}

	SLresult result;
	// create engine
	result = slCreateEngine( &m_engineObject, 0, nullptr, 0, nullptr, nullptr );

	assert( SL_RESULT_SUCCESS == result );

	if( result != SL_RESULT_SUCCESS )
	{
		return result;
	}

	// realize the engine
	result = ( *m_engineObject )->Realize( m_engineObject, SL_BOOLEAN_FALSE );
	assert( SL_RESULT_SUCCESS == result );

	if( result != SL_RESULT_SUCCESS )
	{
		return result;
	}

	// get the engine interface, which is needed in order to create other objects
	result = ( *m_engineObject )->GetInterface( m_engineObject, SL_IID_ENGINE, & ( m_engineEngine ) );
	assert( SL_RESULT_SUCCESS == result );

	if( result != SL_RESULT_SUCCESS )
	{
		return result;
	}

	// create output mix, with environmental reverb specified as a non-required interface
	const SLInterfaceID ids[] = {SL_IID_OUTPUTMIX};
	const SLboolean req[] = {SL_BOOLEAN_FALSE};

	result = ( *m_engineEngine )->CreateOutputMix( m_engineEngine, &m_outputMixObject, 1, ids, req );
	assert( SL_RESULT_SUCCESS == result );

	if( result != SL_RESULT_SUCCESS )
	{
		return result;
	}

	// realize the output mix
	result = ( *m_outputMixObject )->Realize( m_outputMixObject, SL_BOOLEAN_FALSE );
	assert( SL_RESULT_SUCCESS == result );

	return result;
}

void OpenSLEngine::purge()
{
	delete m_pInstance;
	m_pInstance = nullptr;
}

bool OpenSLEngine::isInitialized() const
{
	return m_outputMixObject != nullptr;
}

} /* namespace KoalaSound */
