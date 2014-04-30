/*
 * Log.h
 *
 *  Created on: Mar 14, 2014
 *      Author: dawid
 */

#ifndef LOG_H_KOALA_SOUND
#define LOG_H_KOALA_SOUND

#ifdef DEBUG
#include <android/log.h>
#define LOG_TAG "KoalaSound"
#define KLOG(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

#else
#define KLOG(...)

#endif


const char* getErrorMessage( int errorCode );

#endif /* LOG_H_KOALA_SOUND */
