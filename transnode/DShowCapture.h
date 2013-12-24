/*****************************************************************************
 * DShowCapture.h :
 *****************************************************************************
 * Copyright (C) 2010 Broad Intelligence Technologies
 *
 * Author:
 *
 * Notes: It only work on Windows
 *****************************************************************************/

#ifndef __DSHOW_CAPTURE_H__
#define __DSHOW_CAPTURE_H__

#include <string>
#include <list>
#include <deque>
using namespace std;

#include <dshow.h>
#include "DShowFilter.h"

#define DEFAULT_VIDEO_WIDTH 800
#define DEFAULT_VIDEO_HEIGHT 600

typedef struct dshow_stream_t
{
	string          devicename;
	IBaseFilter     *p_device_filter;
	CaptureFilter   *p_capture_filter;
	AM_MEDIA_TYPE   mt;

	union
	{
		VIDEOINFOHEADER video;
		WAVEFORMATEX    audio;
	} header;

	int             i_fourcc;
	bool      b_pts;

	deque<VLCMediaSample> samples_queue;
} dshow_stream_t;

/****************************************************************************
 * Crossbar stuff
 ****************************************************************************/
#define MAX_CROSSBAR_DEPTH 10

typedef struct CrossbarRouteRec
{
    IAMCrossbar *pXbar;
    LONG        VideoInputIndex;
    LONG        VideoOutputIndex;
    LONG        AudioInputIndex;
    LONG        AudioOutputIndex;

} CrossbarRoute;

class CDShowCapture
{
public:
	CDShowCapture();
	~CDShowCapture();

	bool Open(const char *psz_video_dev, const char *psz_audio_dev);
	bool Close();

	bool Start();
	bool Stop();

private:
	bool OpenDevice(const char *psz_dev, bool b_audio);
	static size_t EnumDeviceCaps( IBaseFilter *p_filter,
                              int i_fourcc, int i_width, int i_height,
                              int i_channels, int i_samplespersec,
                              int i_bitspersample, AM_MEDIA_TYPE *mt,
                              size_t mt_max );
	void CreateDirectShowGraph();
	bool ConnectFilters( IBaseFilter *p_filter,
                            CaptureFilter *p_capture_filter );

	void DeleteCrossbarRoutes();
	HRESULT FindCrossbarRoutes(IPin *p_input_pin, LONG physicalType, int depth = 0);

private:
//	lock_t lock;
//	cond_t  wait;
	IFilterGraph           *m_p_graph;
	ICaptureGraphBuilder2  *m_p_capture_graph_builder2;
	IMediaControl          *m_p_control;

	/* list of elementary streams */
	dshow_stream_t *m_p_video_stream;
	dshow_stream_t *m_p_audio_stream;
	dshow_stream_t *m_p_stream_stream;

	/* Crossbar stuff */
    int                     m_i_crossbar_route_depth;
    CrossbarRoute           m_crossbar_routes[MAX_CROSSBAR_DEPTH];

	/* misc properties */
	int            m_videoWidth;
	int            m_videoHeight;

	int            m_audioChannels;
	int            m_audioSamplerate;
	int            m_audioBitspersample;

	int            m_chroma;
	bool           m_bChroma; /* Force a specific chroma on the dshow input */
};

#endif