/*
 * OpenSlEngine.h
 *
 *  Created on: Mar 13, 2014
 *      Author: dawid
 */

#ifndef OPENSLENGINE_H_
#define OPENSLENGINE_H_

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

namespace KoalaSound
{

class OpenSLEngine
{
public:
	inline static OpenSLEngine* getInstance()
	{
		if( m_pInstance == nullptr )
		{
			m_pInstance = new OpenSLEngine();
		}

		return m_pInstance;
	}

	void purge();

	~OpenSLEngine();

	//We want block them
	OpenSLEngine( OpenSLEngine const& ) = delete;
	void operator= ( OpenSLEngine const& ) = delete;

	/**
	 * You should call this only once
	 * @return result of initialization SLEngine
	 */
	SLresult initializeOpenSLEngine();

	inline SLEngineItf& getEngine()
	{
		return m_engineEngine;
	}

	inline SLObjectItf& getOutputMixObject()
	{
		return m_outputMixObject;
	}

	bool isInitialized() const;

private:
	static OpenSLEngine* m_pInstance;

	// engine interfaces
	SLObjectItf m_engineObject;
	SLEngineItf m_engineEngine;

	// output mix interfaces
	SLObjectItf m_outputMixObject;

	OpenSLEngine();
};

} /* namespace KoalaSound */

#endif /* OPENSLENGINE_H_ */
