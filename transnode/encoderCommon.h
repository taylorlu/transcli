#ifndef _ENCODER_COMMON_H_
#define _ENCODER_COMMON_H_

#include <string>
#include "tsdef.h"
#include "logger.h"
#include "BiThread.h"

class CXMLPref;
class CVideoFilter;
class CWaterMarkManager;
class CThumbnailFilter;
class CStreamOutput;

const char* GetFFMpegAudioCodecName(int id);
const char* GetFFMpegVideoCodecName(int id);
const char* GetFFMpegFormatName(int id);
const char* GetFFMpegOutFileExt(int id, bool ifAudioOnly = false);
int GetAudioBitrateForAVEnc(CXMLPref* pAudioPref);

bool parseWaterMarkInfo(CWaterMarkManager*& pWatermarkMan,CXMLPref* videoPref,video_info_t* pVinfo);
bool parseThumbnailInfo(CThumbnailFilter*& pThumbnail, CXMLPref* videoPref,
	                    video_info_t* pVinfo, std::string destFileName);

// Generate dest file name according to src file and naming rules 
std::string genDestFileName(const char* fileTitle, CXMLPref* muxerPref, const char* outExt,
			const char* relativeDir, size_t destIndex=0, const char* srcDir=NULL);	


/**
	*  Convert an unsigned char buffer holding 8 or 16 bit PCM values
	*  with channels interleaved to a short int buffer, still
	*  with channels interleaved.
	*
	*  @param bitsPerSample the number of bits per sample in the input
	*  @param pcmBuffer the input buffer
	*  @param lenPcmBuffer the number of samples total in pcmBuffer
	*                      (e.g. if 2 channel input, this is twice the
	*                       number of sound samples)
	*  @param outBuffer the output buffer, must be big enough
	*  @param isBigEndian true if the input is big endian, false otherwise
	*/
	void conv (  uint32_t       bitsPerSample,
						uint8_t        *pcmBuffer,
						uint32_t       lenPcmBuffer,
						int16_t        * outBuffer,
						bool           isBigEndian = false );


	/**
	*  Convert a short buffer holding PCM values with channels interleaved
	*  to one or more float buffers, one for each channel
	*
	*  @param shortBuffer the input buffer
	*  @param lenShortBuffer total length of the input buffer
	*  @param floatBuffers an array of float buffers, each
	*                      (lenShortBuffer / channels) long
	*  @param channels number of channels to separate the input to
	*/
	void conv ( int16_t         * shortBuffer,
		uint32_t        lenShortBuffer,
		float            ** floatBuffers,
		uint32_t        channels )   ; 

	/**
	*  Convert a char buffer holding 8 bit PCM values to a short buffer
	*
	*  @param pcmBuffer buffer holding 8 bit PCM audio values,
	*                   channels are interleaved
	*  @param lenPcmBuffer length of pcmBuffer
	*  @param leftBuffer put the left channel here (must be big enough)
	*  @param rightBuffer put the right channel here (not touched if mono,
	*                     must be big enough)
	*  @param channels number of channels (1 = mono, 2 = stereo)
	*/
	void conv8 ( uint8_t     * pcmBuffer,
		uint32_t        lenPcmBuffer,
		int16_t         * leftBuffer,
		int16_t         * rightBuffer,
		uint32_t        channels );

	/**
	*  Convert a char buffer holding 16 bit PCM values to two short buffer
	*
	*  @param pcmBuffer buffer holding 16 bit PCM audio values,
	*                   channels are interleaved
	*  @param lenPcmBuffer length of pcmBuffer
	*  @param leftBuffer put the left channel here (must be big enough)
	*  @param rightBuffer put the right channel here (not touched if mono,
	*                     must be big enough)
	*  @param channels number of channels (1 = mono, 2 = stereo)
	*  @param isBigEndian true if input is big endian, false otherwise
	*/
	void conv16 ( uint8_t     * pcmBuffer,
		uint32_t        lenPcmBuffer,
		int16_t         * leftBuffer,
		int16_t         * rightBuffer,
		uint32_t        channels,
		bool                isBigEndian = false);

	/**
	*  Convert a char buffer holding 16 bit PCM values to two float buffer
	*
	*  @param pcmBuffer buffer holding 16 bit PCM audio values,
	*                   channels are interleaved
	*  @param lenPcmBuffer length of pcmBuffer
	*  @param leftBuffer put the left channel here (must be big enough)
	*  @param rightBuffer put the right channel here (not touched if mono,
	*                     must be big enough)
	*  @param channels number of channels (1 = mono, 2 = stereo)
	*  @param isBigEndian true if input is big endian, false otherwise
	*/
	void conv16Float ( uint8_t     * pcmBuffer,
		uint32_t        lenPcmBuffer,
		double			* leftBuffer,
		double			* rightBuffer,
		uint32_t        channels,
		bool                isBigEndian = false);

#endif