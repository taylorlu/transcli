/*****************************************************************************
 * DShowCapture.cpp :
 *****************************************************************************
 * Copyright (C) 2010 Broad Intelligence Technologies
 *
 * Author:
 *
 * Notes: It only work on Windows
 *****************************************************************************/

#include "DShowCapture.h"
#include "DshowUtils.h"
#include "logger.h"

const GUID MEDIASUBTYPE_I420 = {0x30323449, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * get fourcc priority from arbritary preference, the higher the better
 */
static int GetFourCCPriority( int i_fourcc )
{
	switch( i_fourcc ) {
	case VLC_CODEC_I420:
	case VLC_CODEC_FL32:
		return 9;
	case VLC_CODEC_YV12:
	case VLC_FOURCC('a','r','a','w'):
		return 8;
	case VLC_CODEC_RGB24:
		return 7;
	case VLC_CODEC_YUYV:
	case VLC_CODEC_RGB32:
	case VLC_CODEC_RGBA:
		return 6;
	}

	return 0;
}

static void ShowPropertyPage( IUnknown *obj )
{
	ISpecifyPropertyPages *p_spec;
	CAUUID cauuid;

	HRESULT hr = obj->QueryInterface( IID_ISpecifyPropertyPages,
		(void **)&p_spec );
	if( FAILED(hr) ) return;

	if( SUCCEEDED(p_spec->GetPages( &cauuid )) ) {
		if( cauuid.cElems > 0 ) {
			HWND hwnd_desktop = ::GetDesktopWindow();

			OleCreatePropertyFrame( hwnd_desktop, 30, 30, NULL, 1, &obj,
				cauuid.cElems, cauuid.pElems, 0, 0, NULL );

			CoTaskMemFree( cauuid.pElems );
		}
		p_spec->Release();
	}
}

static void ShowDeviceProperties( ICaptureGraphBuilder2 *p_graph, IBaseFilter *p_device_filter, bool b_audio )
{
	HRESULT hr;

	logger_info( LOGM_GLOBAL, "configuring Device Properties" );

	/*
	* Video or audio capture filter page
	*/
	ShowPropertyPage( p_device_filter );

	/*
	* Audio capture pin
	*/
	if( p_graph && b_audio ) {
		IAMStreamConfig *p_SC;

		logger_info( LOGM_GLOBAL, "showing WDM Audio Configuration Pages" );

		hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
			&MEDIATYPE_Audio, p_device_filter,
			IID_IAMStreamConfig, (void **)&p_SC );
		if( SUCCEEDED(hr) ) {
			ShowPropertyPage(p_SC);
			p_SC->Release();
		}

		/*
		* TV Audio filter
		*/
		IAMTVAudio *p_TVA;
		HRESULT hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
			&MEDIATYPE_Audio, p_device_filter,
			IID_IAMTVAudio, (void **)&p_TVA );
		if( SUCCEEDED(hr) ) {
			ShowPropertyPage(p_TVA);
			p_TVA->Release();
		}
	}

	/*
	* Video capture pin
	*/
	if( p_graph && !b_audio ) {
		IAMStreamConfig *p_SC;

		logger_info( LOGM_GLOBAL, "showing WDM Video Configuration Pages" );

		hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
			&MEDIATYPE_Interleaved, p_device_filter,
			IID_IAMStreamConfig, (void **)&p_SC );
		if( FAILED(hr) ) {
			hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
				&MEDIATYPE_Video, p_device_filter,
				IID_IAMStreamConfig, (void **)&p_SC );
		}

		if( FAILED(hr) ) {
			hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
				&MEDIATYPE_Stream, p_device_filter,
				IID_IAMStreamConfig, (void **)&p_SC );
		}

		if( SUCCEEDED(hr) ) {
			ShowPropertyPage(p_SC);
			p_SC->Release();
		}
	}
}

static void ShowTunerProperties( ICaptureGraphBuilder2 *p_graph,
								IBaseFilter *p_device_filter, bool b_audio )
{
	HRESULT hr;

	logger_info( LOGM_GLOBAL, "configuring Tuner Properties" );

	if( !p_graph || b_audio ) return;

	IAMTVTuner *p_TV;
	hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
		&MEDIATYPE_Interleaved, p_device_filter,
		IID_IAMTVTuner, (void **)&p_TV );
	if( FAILED(hr) ) {
		hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
			&MEDIATYPE_Video, p_device_filter,
			IID_IAMTVTuner, (void **)&p_TV );
	}

	if( FAILED(hr) ) {
		hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE,
			&MEDIATYPE_Stream, p_device_filter,
			IID_IAMTVTuner, (void **)&p_TV );
	}

	if( SUCCEEDED(hr) ) {
		ShowPropertyPage(p_TV);
		p_TV->Release();
	}
}

//Config tuner
static void ConfigTuner( ICaptureGraphBuilder2 *p_graph, IBaseFilter *p_device_filter )
{
	int i_channel, i_country, i_input, i_amtuner_mode;
	long l_modes = 0;
	IAMTVTuner *p_TV;
	HRESULT hr;

	if( !p_graph ) return;

	i_channel = 0; //TODO:
	i_country = 86; //TODO
	i_input = 1; //TODO:
	i_amtuner_mode = 0; //TODO:

	if( !i_channel && !i_country && !i_input ) return; /* Nothing to do */

	logger_info( LOGM_GLOBAL, "tuner config: channel %i, country %i, input type %i",
		i_channel, i_country, i_input );

	hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved,
		p_device_filter, IID_IAMTVTuner,
		(void **)&p_TV );
	if( FAILED(hr) ) {
		hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
			p_device_filter, IID_IAMTVTuner,
			(void **)&p_TV );
	}

	if( FAILED(hr) ) {
		hr = p_graph->FindInterface( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Stream,
			p_device_filter, IID_IAMTVTuner,
			(void **)&p_TV );
	}

	if( FAILED(hr) ) {
		logger_err( LOGM_GLOBAL, "couldn't find tuner interface" );
		return;
	}

	hr = p_TV->GetAvailableModes( &l_modes );
	if( SUCCEEDED(hr) && (l_modes & i_amtuner_mode) ) {
		hr = p_TV->put_Mode( (AMTunerModeType)i_amtuner_mode );
	}

	if( i_input == 1 ) p_TV->put_InputType( 0, TunerInputCable );
	else if( i_input == 2 ) p_TV->put_InputType( 0, TunerInputAntenna );

	p_TV->put_CountryCode( i_country );
	p_TV->put_Channel( i_channel, AMTUNER_SUBCHAN_NO_TUNE,
		AMTUNER_SUBCHAN_NO_TUNE );
	p_TV->Release();
}

// Helper function to associate a crossbar pin name with the type.
static const char * GetPhysicalPinName(long lType)
{
	switch (lType)
	{
	case PhysConn_Video_Tuner:            return "Video Tuner";
	case PhysConn_Video_Composite:        return "Video Composite";
	case PhysConn_Video_SVideo:           return "S-Video";
	case PhysConn_Video_RGB:              return "Video RGB";
	case PhysConn_Video_YRYBY:            return "Video YRYBY";
	case PhysConn_Video_SerialDigital:    return "Video Serial Digital";
	case PhysConn_Video_ParallelDigital:  return "Video Parallel Digital";
	case PhysConn_Video_SCSI:             return "Video SCSI";
	case PhysConn_Video_AUX:              return "Video AUX";
	case PhysConn_Video_1394:             return "Video 1394";
	case PhysConn_Video_USB:              return "Video USB";
	case PhysConn_Video_VideoDecoder:     return "Video Decoder";
	case PhysConn_Video_VideoEncoder:     return "Video Encoder";

	case PhysConn_Audio_Tuner:            return "Audio Tuner";
	case PhysConn_Audio_Line:             return "Audio Line";
	case PhysConn_Audio_Mic:              return "Audio Microphone";
	case PhysConn_Audio_AESDigital:       return "Audio AES/EBU Digital";
	case PhysConn_Audio_SPDIFDigital:     return "Audio S/PDIF";
	case PhysConn_Audio_SCSI:             return "Audio SCSI";
	case PhysConn_Audio_AUX:              return "Audio AUX";
	case PhysConn_Audio_1394:             return "Audio 1394";
	case PhysConn_Audio_USB:              return "Audio USB";
	case PhysConn_Audio_AudioDecoder:     return "Audio Decoder";

	default:                              return "Unknown Type";
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
CDShowCapture::CDShowCapture():
				m_p_graph(NULL), m_p_capture_graph_builder2(NULL), m_p_control(NULL),
				m_p_video_stream(NULL), m_p_audio_stream(NULL), m_p_stream_stream(NULL),
				m_i_crossbar_route_depth(0),
				m_videoWidth(0 /*DEFAULT_VIDEO_WIDTH*/), m_videoHeight(0/*DEFAULT_VIDEO_HEIGHT*/),
				m_audioChannels(0), m_audioSamplerate(0), m_audioBitspersample(0),
				m_chroma(0), m_bChroma(false)
{

}

CDShowCapture::~CDShowCapture()
{
	Close();
}

/* Get/parse options and open device(s) */
// if psz_video_dev == null, try to open the first video device
// if psz_video_dev == "none", no video device is opened
// the same to psz_audio_dev

//TODO: Set Width, height, chroma and 
bool CDShowCapture::Open(const char *psz_video_dev, const char *psz_audio_dev)
{
	bool b_video, b_audio;

	if (psz_video_dev != NULL && strcmp(psz_video_dev, "none") == 0) b_video = false;
	if (psz_audio_dev != NULL && strcmp(psz_audio_dev, "none") == 0) b_audio = false;

	if (!b_video && !b_audio) {
		logger_err(LOGM_GLOBAL, "Open(): video/audio dev can not both none");
		return false;
	}

    /* Initialize OLE/COM */
    CoInitialize( 0 );

	/* Build directshow graph */
	CreateDirectShowGraph();

    //TODO: mutex_init( &p_sys->lock );
    //TODO: cond_init( &p_sys->wait );

	if (!b_video) {
		logger_info(LOGM_GLOBAL, "Open(): skipping video device");
	}
	else if (!OpenDevice(psz_video_dev, false)) {
		logger_err(LOGM_GLOBAL, "Open(): Failed to open video device: %s", psz_video_dev? psz_video_dev:"Default");
		return false;
	}

	if (!b_audio) {
		logger_info(LOGM_GLOBAL, "Open(): skipping audio device");
	}
	else if (!OpenDevice(psz_audio_dev, true)) {
		logger_err(LOGM_GLOBAL, "Open(): Failed to open audio device: %s", psz_audio_dev?psz_audio_dev:"Default");
		return false;
	}

	//TODO!!
	return true;
}

bool CDShowCapture::Close()
{
//TODO
	return true;
}

bool CDShowCapture::Start()
{
//TODO
	return true;
}

bool CDShowCapture::Stop()
{
//TODO
	return true;
}

#define MAX_MEDIA_TYPES 32
// if psz_dev == null, open the first device
bool CDShowCapture::OpenDevice(const char *psz_dev, bool b_audio)
{
	string devicename;
	list<string> list_devices;

	/* Enumerate devices and display their names */
	FindCaptureDevice( NULL, &list_devices, b_audio );
	if( !list_devices.size() ) {
		logger_err(LOGM_GLOBAL, "No %s capture device is found.\n", b_audio?"audio":"video");
		return false;
	}

	list<string>::iterator iter;
	for( iter = list_devices.begin(); iter != list_devices.end(); iter++ )
		logger_info( LOGM_GLOBAL, "found device: %s", iter->c_str() );

	/* If no device name was specified, pick the 1st one */
	if( psz_dev == NULL ) {
		/* When none selected */
		devicename = *list_devices.begin();
		logger_info( LOGM_GLOBAL, "asking for default device: %s", devicename.c_str() ) ;
    }
    else {
		devicename = psz_dev;
		logger_info( LOGM_GLOBAL, "asking for device: %s", psz_dev ) ;
	}

	// Use the system device enumerator and class enumerator to find
	// a capture/preview device, such as a desktop USB video camera.
	IBaseFilter *p_device_filter =
		FindCaptureDevice( &devicename, NULL, b_audio );

	if( p_device_filter == NULL ) {
		logger_err( LOGM_GLOBAL, "can't use device: %s, unsupported device type",
					devicename.c_str() );
		return false;
	}

	logger_info( LOGM_GLOBAL, "using device: %s", devicename.c_str() );

	// Retreive acceptable media types supported by device
	AM_MEDIA_TYPE media_types[MAX_MEDIA_TYPES];
	size_t media_count =
		EnumDeviceCaps( p_device_filter, b_audio ? 0 : m_chroma,
						m_videoWidth, m_videoHeight,
						b_audio ? m_audioChannels: 0,
						b_audio ? m_audioSamplerate: 0,
						b_audio ? m_audioBitspersample: 0,
						media_types, MAX_MEDIA_TYPES );

	AM_MEDIA_TYPE *mt = NULL;

	if( media_count <= 0 ) {
		logger_err( LOGM_GLOBAL, "capture device '%s' does not support required parameters !", devicename.c_str() );
		return false;
	}

	mt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE) * media_count);

	// Order and copy returned media types according to arbitrary
	// fourcc priority
	for( size_t c = 0; c < media_count; c++ ) {
		int slot_priority =
			GetFourCCPriority(GetFourCCFromMediaType(media_types[c]));
		size_t slot_copy = c;

		for( size_t d = c+1; d < media_count; d++ ) {
			int priority =
				GetFourCCPriority(GetFourCCFromMediaType(media_types[d]));
			if( priority > slot_priority ) {
				slot_priority = priority;
				slot_copy = d;
			}
		}

		if( slot_copy != c ) {
			mt[c] = media_types[slot_copy];
			media_types[slot_copy] = media_types[c];
		}
		else {
			mt[c] = media_types[c];
		}
	}

	/* Create and add our capture filter */
	CaptureFilter *p_capture_filter =
		new CaptureFilter( /*FIXME:this*/mt, media_count );
	m_p_graph->AddFilter( p_capture_filter, 0 );

	/* Add the device filter to the graph (seems necessary with VfW before
	* accessing pin attributes). */
	m_p_graph->AddFilter( p_device_filter, 0 );

	/* Attempt to connect one of this device's capture output pins */
	logger_info( LOGM_GLOBAL, "connecting filters" );
	if( ConnectFilters( p_device_filter, p_capture_filter ) )
	{
		/* Success */
		logger_info( LOGM_GLOBAL, "filters connected successfully !" );

		dshow_stream_t **pp_stream = NULL;
		dshow_stream_t dshow_stream;
		dshow_stream.b_pts = false;
		dshow_stream.mt =
			p_capture_filter->CustomGetPin()->CustomGetMediaType();


		/* Show Device properties. Done here so the VLC stream is setup with
		* the proper parameters. */
		if( false /*dshow-config*/ )
		{
			ShowDeviceProperties( m_p_capture_graph_builder2, p_device_filter, b_audio );
		}

		ConfigTuner( m_p_capture_graph_builder2,
			p_device_filter );

		if( false /*dshow-config*/ && dshow_stream.mt.majortype != MEDIATYPE_Stream )
		{
			/* FIXME: we do MEDIATYPE_Stream later so we don't do it twice. */
			ShowTunerProperties( m_p_capture_graph_builder2, p_device_filter, b_audio );
		}

		dshow_stream.mt =
			p_capture_filter->CustomGetPin()->CustomGetMediaType();

		dshow_stream.i_fourcc = GetFourCCFromMediaType( dshow_stream.mt );
		if( dshow_stream.i_fourcc )
		{
			if( dshow_stream.mt.majortype == MEDIATYPE_Video )
			{
				dshow_stream.header.video =
					*(VIDEOINFOHEADER *)dshow_stream.mt.pbFormat;
				logger_info( LOGM_GLOBAL, "MEDIATYPE_Video" );
				logger_info( LOGM_GLOBAL, "selected video pin accepts format: %4.4s",
					(char *)&dshow_stream.i_fourcc);
				pp_stream = &m_p_video_stream;
			}
			else if( dshow_stream.mt.majortype == MEDIATYPE_Audio )
			{
				dshow_stream.header.audio =
					*(WAVEFORMATEX *)dshow_stream.mt.pbFormat;
				logger_info( LOGM_GLOBAL, "MEDIATYPE_Audio" );
				logger_info( LOGM_GLOBAL, "selected audio pin accepts format: %4.4s",
					(char *)&dshow_stream.i_fourcc);
				pp_stream = &m_p_audio_stream;
			}
			else if( dshow_stream.mt.majortype == MEDIATYPE_Stream )
			{
				logger_info( LOGM_GLOBAL, "MEDIATYPE_Stream" );
				logger_info( LOGM_GLOBAL, "selected stream pin accepts format: %4.4s",
					(char *)&dshow_stream.i_fourcc);
				pp_stream = &m_p_stream_stream;
			}
			else
			{
				logger_err( LOGM_GLOBAL, "unknown stream majortype" );
				goto fail;
			}

			/* Add directshow elementary stream to our list */
			dshow_stream.p_device_filter = p_device_filter;
			dshow_stream.p_capture_filter = p_capture_filter;

			if (pp_stream) {
				if (*pp_stream) delete (*pp_stream);
				*pp_stream = new dshow_stream_t;
				**pp_stream = dshow_stream;
			}

			return true;
		}
	}

fail:
	/* Remove filters from graph */
	m_p_graph->RemoveFilter( p_device_filter );
	m_p_graph->RemoveFilter( p_capture_filter );

	/* Release objects */
	p_device_filter->Release();
	p_capture_filter->Release();

	return false;
}

size_t CDShowCapture::EnumDeviceCaps( IBaseFilter *p_filter,
                              int i_fourcc, int i_width, int i_height,
                              int i_channels, int i_samplespersec,
                              int i_bitspersample, AM_MEDIA_TYPE *mt,
                              size_t mt_max )
{
	IEnumPins *p_enumpins;
	IPin *p_output_pin;
	IEnumMediaTypes *p_enummt;
	size_t mt_count = 0;

	LONGLONG i_AvgTimePerFrame = 0;
	float r_fps = 0; //TODO: configurable
	if( r_fps )
		i_AvgTimePerFrame = 10000000000LL/(LONGLONG)(r_fps*1000.0f);

	if( FAILED(p_filter->EnumPins( &p_enumpins )) )
	{
		logger_err( LOGM_GLOBAL, "EnumDeviceCaps failed: no pin enumeration !");
		return 0;
	}

	while( S_OK == p_enumpins->Next( 1, &p_output_pin, NULL ) )
	{
		PIN_INFO info;

		if( S_OK == p_output_pin->QueryPinInfo( &info ) )
		{
			logger_info( LOGM_GLOBAL, "EnumDeviceCaps: %s pin: %S",
				info.dir == PINDIR_INPUT ? "input" : "output",
				info.achName );
			if( info.pFilter ) info.pFilter->Release();
		}

		p_output_pin->Release();
	}

	p_enumpins->Reset();

	while( !mt_count && p_enumpins->Next( 1, &p_output_pin, NULL ) == S_OK )
	{
		PIN_INFO info;

		if( S_OK == p_output_pin->QueryPinInfo( &info ) )
		{
			if( info.pFilter ) info.pFilter->Release();
			if( info.dir == PINDIR_INPUT )
			{
				p_output_pin->Release();
				continue;
			}
			logger_info( LOGM_GLOBAL, "EnumDeviceCaps: trying pin %S", info.achName );
		}

		AM_MEDIA_TYPE *p_mt;

		/*
		** Configure pin with a default compatible media if possible
		*/

		IAMStreamConfig *pSC;
		if( SUCCEEDED(p_output_pin->QueryInterface( IID_IAMStreamConfig,
			(void **)&pSC )) )
		{
			int piCount, piSize;
			if( SUCCEEDED(pSC->GetNumberOfCapabilities(&piCount, &piSize)) )
			{
				BYTE *pSCC= (BYTE *)CoTaskMemAlloc(piSize);
				if( NULL != pSCC )
				{
					int i_priority = -1;
					for( int i=0; i<piCount; ++i )
					{
						if( SUCCEEDED(pSC->GetStreamCaps(i, &p_mt, pSCC)) )
						{
							int i_current_fourcc = GetFourCCFromMediaType( *p_mt );
							int i_current_priority = GetFourCCPriority(i_current_fourcc);

							if( (i_fourcc && (i_current_fourcc != i_fourcc))
								|| (i_priority > i_current_priority) )
							{
								// unwanted chroma, try next media type
								FreeMediaType( *p_mt );
								CoTaskMemFree( (PVOID)p_mt );
								continue;
							}

							if( MEDIATYPE_Video == p_mt->majortype
								&& FORMAT_VideoInfo == p_mt->formattype )
							{
								VIDEO_STREAM_CONFIG_CAPS *pVSCC = reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(pSCC);
								VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(p_mt->pbFormat);

								if( i_AvgTimePerFrame )
								{
									if( pVSCC->MinFrameInterval > i_AvgTimePerFrame
										|| i_AvgTimePerFrame > pVSCC->MaxFrameInterval )
									{
										// required frame rate not compatible, try next media type
										FreeMediaType( *p_mt );
										CoTaskMemFree( (PVOID)p_mt );
										continue;
									}
									pVih->AvgTimePerFrame = i_AvgTimePerFrame;
								}

								if( i_width )
								{
									if((   !pVSCC->OutputGranularityX
										&& i_width != pVSCC->MinOutputSize.cx
										&& i_width != pVSCC->MaxOutputSize.cx)
										||
										(   pVSCC->OutputGranularityX
										&& ((i_width % pVSCC->OutputGranularityX)
										|| pVSCC->MinOutputSize.cx > i_width
										|| i_width > pVSCC->MaxOutputSize.cx )))
									{
										// required width not compatible, try next media type
										FreeMediaType( *p_mt );
										CoTaskMemFree( (PVOID)p_mt );
										continue;
									}
									pVih->bmiHeader.biWidth = i_width;
								}

								if( i_height )
								{
									if((   !pVSCC->OutputGranularityY
										&& i_height != pVSCC->MinOutputSize.cy
										&& i_height != pVSCC->MaxOutputSize.cy)
										||
										(   pVSCC->OutputGranularityY
										&& ((i_height % pVSCC->OutputGranularityY)
										|| pVSCC->MinOutputSize.cy > i_height
										|| i_height > pVSCC->MaxOutputSize.cy )))
									{
										// required height not compatible, try next media type
										FreeMediaType( *p_mt );
										CoTaskMemFree( (PVOID)p_mt );
										continue;
									}
									pVih->bmiHeader.biHeight = i_height;
								}

								// Set the sample size and image size.
								// (Round the image width up to a DWORD boundary.)
								p_mt->lSampleSize = pVih->bmiHeader.biSizeImage =
									((pVih->bmiHeader.biWidth + 3) & ~3) *
									pVih->bmiHeader.biHeight * (pVih->bmiHeader.biBitCount>>3);

								// no cropping, use full video input buffer
								memset(&(pVih->rcSource), 0, sizeof(RECT));
								memset(&(pVih->rcTarget), 0, sizeof(RECT));

								// select this format as default
								if( SUCCEEDED( pSC->SetFormat(p_mt) ) )
								{
									i_priority = i_current_priority;
									if( i_fourcc )
										// no need to check any more media types
										i = piCount;
								}
							}
							else if( p_mt->majortype == MEDIATYPE_Audio
								&& p_mt->formattype == FORMAT_WaveFormatEx )
							{
								AUDIO_STREAM_CONFIG_CAPS *pASCC = reinterpret_cast<AUDIO_STREAM_CONFIG_CAPS*>(pSCC);
								WAVEFORMATEX *pWfx = reinterpret_cast<WAVEFORMATEX*>(p_mt->pbFormat);

								if( i_current_fourcc && (WAVE_FORMAT_PCM == pWfx->wFormatTag) )
								{
									int val = i_channels;
									if( ! val )
										val = 2;

									if( (   !pASCC->ChannelsGranularity
										&& (unsigned int)val != pASCC->MinimumChannels
										&& (unsigned int)val != pASCC->MaximumChannels)
										||
										(   pASCC->ChannelsGranularity
										&& ((val % pASCC->ChannelsGranularity)
										|| (unsigned int)val < pASCC->MinimumChannels
										|| (unsigned int)val > pASCC->MaximumChannels)))
									{
										// required number channels not available, try next media type
										FreeMediaType( *p_mt );
										CoTaskMemFree( (PVOID)p_mt );
										continue;
									}
									pWfx->nChannels = val;

									val = i_samplespersec;
									if( ! val )
										val = 44100;

									if( (   !pASCC->SampleFrequencyGranularity
										&& (unsigned int)val != pASCC->MinimumSampleFrequency
										&& (unsigned int)val != pASCC->MaximumSampleFrequency)
										||
										(   pASCC->SampleFrequencyGranularity
										&& ((val % pASCC->SampleFrequencyGranularity)
										|| (unsigned int)val < pASCC->MinimumSampleFrequency
										|| (unsigned int)val > pASCC->MaximumSampleFrequency )))
									{
										// required sampling rate not available, try next media type
										FreeMediaType( *p_mt );
										CoTaskMemFree( (PVOID)p_mt );
										continue;
									}
									pWfx->nSamplesPerSec = val;

									val = i_bitspersample;
									if( ! val )
									{
										if( VLC_CODEC_FL32 == i_current_fourcc )
											val = 32;
										else
											val = 16;
									}

									if( (   !pASCC->BitsPerSampleGranularity
										&& (unsigned int)val != pASCC->MinimumBitsPerSample
										&&	(unsigned int)val != pASCC->MaximumBitsPerSample )
										||
										(   pASCC->BitsPerSampleGranularity
										&& ((val % pASCC->BitsPerSampleGranularity)
										|| (unsigned int)val < pASCC->MinimumBitsPerSample
										|| (unsigned int)val > pASCC->MaximumBitsPerSample )))
									{
										// required sample size not available, try next media type
										FreeMediaType( *p_mt );
										CoTaskMemFree( (PVOID)p_mt );
										continue;
									}

									pWfx->wBitsPerSample = val;
									pWfx->nBlockAlign = (pWfx->wBitsPerSample * pWfx->nChannels)/8;
									pWfx->nAvgBytesPerSec = pWfx->nSamplesPerSec * pWfx->nBlockAlign;

									// select this format as default
									if( SUCCEEDED( pSC->SetFormat(p_mt) ) )
									{
										i_priority = i_current_priority;
									}
								}
							}
							FreeMediaType( *p_mt );
							CoTaskMemFree( (PVOID)p_mt );
						}
					}
					CoTaskMemFree( (LPVOID)pSCC );
					if( i_priority >= 0 )
						logger_info( LOGM_GLOBAL, "EnumDeviceCaps: input pin default format configured");
				}
			}
			pSC->Release();
		}

		/*
		** Probe pin for available medias (may be a previously configured one)
		*/

		if( FAILED( p_output_pin->EnumMediaTypes( &p_enummt ) ) )
		{
			p_output_pin->Release();
			continue;
		}

		while( p_enummt->Next( 1, &p_mt, NULL ) == S_OK )
		{
			int i_current_fourcc = GetFourCCFromMediaType( *p_mt );
			if( i_current_fourcc && p_mt->majortype == MEDIATYPE_Video
				&& p_mt->formattype == FORMAT_VideoInfo )
			{
				int i_current_width = ((VIDEOINFOHEADER *)p_mt->pbFormat)->bmiHeader.biWidth;
				int i_current_height = ((VIDEOINFOHEADER *)p_mt->pbFormat)->bmiHeader.biHeight;
				LONGLONG i_current_atpf = ((VIDEOINFOHEADER *)p_mt->pbFormat)->AvgTimePerFrame;

				if( i_current_height < 0 )
					i_current_height = -i_current_height;

				logger_info( LOGM_GLOBAL, "EnumDeviceCaps: input pin "
					"accepts chroma: %4.4s, width:%i, height:%i, fps:%f",
					(char *)&i_current_fourcc, i_current_width,
					i_current_height, (10000000.0f/((float)i_current_atpf)) );

				if( ( !i_fourcc || i_fourcc == i_current_fourcc ) &&
					( !i_width || i_width == i_current_width ) &&
					( !i_height || i_height == i_current_height ) &&
					( !i_AvgTimePerFrame || i_AvgTimePerFrame == i_current_atpf ) &&
					mt_count < mt_max )
				{
					/* Pick match */
					mt[mt_count++] = *p_mt;
				}
				else FreeMediaType( *p_mt );
			}
			else if( i_current_fourcc && p_mt->majortype == MEDIATYPE_Audio
				&& p_mt->formattype == FORMAT_WaveFormatEx)
			{
				int i_current_channels =
					((WAVEFORMATEX *)p_mt->pbFormat)->nChannels;
				int i_current_samplespersec =
					((WAVEFORMATEX *)p_mt->pbFormat)->nSamplesPerSec;
				int i_current_bitspersample =
					((WAVEFORMATEX *)p_mt->pbFormat)->wBitsPerSample;

				logger_info( LOGM_GLOBAL, "EnumDeviceCaps: input pin "
					"accepts format: %4.4s, channels:%i, "
					"samples/sec:%i bits/sample:%i",
					(char *)&i_current_fourcc, i_current_channels,
					i_current_samplespersec, i_current_bitspersample);

				if( (!i_channels || i_channels == i_current_channels) &&
					(!i_samplespersec ||
					i_samplespersec == i_current_samplespersec) &&
					(!i_bitspersample ||
					i_bitspersample == i_current_bitspersample) &&
					mt_count < mt_max )
				{
					/* Pick  match */
					mt[mt_count++] = *p_mt;

					/* Setup a few properties like the audio latency */
					IAMBufferNegotiation *p_ambuf;
					if( SUCCEEDED( p_output_pin->QueryInterface(
						IID_IAMBufferNegotiation, (void **)&p_ambuf ) ) )
					{
						ALLOCATOR_PROPERTIES AllocProp;
						AllocProp.cbAlign = -1;

						/* 100 ms of latency */
						AllocProp.cbBuffer = i_current_channels *
							i_current_samplespersec *
							i_current_bitspersample / 8 / 10;

						AllocProp.cbPrefix = -1;
						AllocProp.cBuffers = -1;
						p_ambuf->SuggestAllocatorProperties( &AllocProp );
						p_ambuf->Release();
					}
				}
				else FreeMediaType( *p_mt );
			}
			else if( i_current_fourcc && p_mt->majortype == MEDIATYPE_Stream )
			{
				logger_info( LOGM_GLOBAL, "EnumDeviceCaps: input pin "
					"accepts stream format: %4.4s",
					(char *)&i_current_fourcc );

				if( ( !i_fourcc || i_fourcc == i_current_fourcc ) &&
					mt_count < mt_max )
				{
					/* Pick match */
					mt[mt_count++] = *p_mt;
					i_fourcc = i_current_fourcc;
				}
				else FreeMediaType( *p_mt );
			}
			else
			{
				const char * psz_type = "unknown";
				if( p_mt->majortype == MEDIATYPE_Video ) psz_type = "video";
				if( p_mt->majortype == MEDIATYPE_Audio ) psz_type = "audio";
				if( p_mt->majortype == MEDIATYPE_Stream ) psz_type = "stream";
				logger_info( LOGM_GLOBAL, "EnumDeviceCaps: input pin media: unsupported format "
					"(%s %4.4s)", psz_type, (char *)&p_mt->subtype );

				FreeMediaType( *p_mt );
			}
			CoTaskMemFree( (PVOID)p_mt );
		}

		if( !mt_count && p_enummt->Reset() == S_OK )
		{
			// did not find any supported MEDIATYPE for this output pin.
			// However the graph builder might insert converter filters in
			// the graph if we use a different codec in filter input pin.
			// however, in order to avoid nasty surprises, make use of this
			// facility only for known unsupported codecs.

			while( !mt_count && p_enummt->Next( 1, &p_mt, NULL ) == S_OK )
			{
				// the first four bytes of subtype GUID contains the codec FOURCC
				const char *pfcc = (char *)&p_mt->subtype;
				int i_current_fourcc = VLC_FOURCC(pfcc[0], pfcc[1], pfcc[2], pfcc[3]);
				if( VLC_FOURCC('H','C','W','2') == i_current_fourcc
					&& p_mt->majortype == MEDIATYPE_Video )
				{
					// output format for 'Hauppauge WinTV PVR PCI II Capture'
					// try I420 as an input format
					i_current_fourcc = VLC_CODEC_I420;
					if( !i_fourcc || i_fourcc == i_current_fourcc )
					{
						// return alternative media type
						AM_MEDIA_TYPE mtr;
						VIDEOINFOHEADER vh;

						mtr.majortype            = MEDIATYPE_Video;
						mtr.subtype              = MEDIASUBTYPE_I420;
						mtr.bFixedSizeSamples    = TRUE;
						mtr.bTemporalCompression = FALSE;
						mtr.pUnk                 = NULL;
						mtr.formattype           = FORMAT_VideoInfo;
						mtr.cbFormat             = sizeof(vh);
						mtr.pbFormat             = (BYTE *)&vh;

						memset(&vh, 0, sizeof(vh));

						vh.bmiHeader.biSize   = sizeof(vh.bmiHeader);
						vh.bmiHeader.biWidth  = i_width > 0 ? i_width :
							((VIDEOINFOHEADER *)p_mt->pbFormat)->bmiHeader.biWidth;
						vh.bmiHeader.biHeight = i_height > 0 ? i_height :
							((VIDEOINFOHEADER *)p_mt->pbFormat)->bmiHeader.biHeight;
						vh.bmiHeader.biPlanes      = 3;
						vh.bmiHeader.biBitCount    = 12;
						vh.bmiHeader.biCompression = VLC_CODEC_I420;
						vh.bmiHeader.biSizeImage   = vh.bmiHeader.biWidth * 12 *
							vh.bmiHeader.biHeight / 8;
						mtr.lSampleSize            = vh.bmiHeader.biSizeImage;

						logger_info( LOGM_GLOBAL, "EnumDeviceCaps: input pin media: using 'I420' in place of unsupported format 'HCW2'");

						if( SUCCEEDED(CopyMediaType(mt+mt_count, &mtr)) )
							++mt_count;
					}
				}
				FreeMediaType( *p_mt );
			}
		}

		p_enummt->Release();
		p_output_pin->Release();
	}

	p_enumpins->Release();
	return mt_count;
}

void CDShowCapture::CreateDirectShowGraph()
{
	/* Create directshow filter graph */
	if( SUCCEEDED( CoCreateInstance( CLSID_FilterGraph, 0, CLSCTX_INPROC,
		(REFIID)IID_IFilterGraph, (void **)&m_p_graph) ) ) {
		/* Create directshow capture graph builder if available */
		if( SUCCEEDED( CoCreateInstance( CLSID_CaptureGraphBuilder2, 0,
			CLSCTX_INPROC, (REFIID)IID_ICaptureGraphBuilder2,
			(void **)&m_p_capture_graph_builder2 ) ) ) {

			m_p_capture_graph_builder2->
				SetFiltergraph((IGraphBuilder *)m_p_graph);
		}

		m_p_graph->QueryInterface( IID_IMediaControl,
			(void **)&m_p_control );
	}
}

bool CDShowCapture::ConnectFilters( IBaseFilter *p_filter,
	CaptureFilter *p_capture_filter )
{
	CapturePin *p_input_pin = p_capture_filter->CustomGetPin();

	AM_MEDIA_TYPE mediaType = p_input_pin->CustomGetMediaType();

	if( m_p_capture_graph_builder2 ) {
		if( FAILED(m_p_capture_graph_builder2->
			RenderStream( &PIN_CATEGORY_CAPTURE, &mediaType.majortype,
			p_filter, 0, (IBaseFilter *)p_capture_filter )) )
		{
			return false;
		}

		// Sort out all the possible video inputs
		// The class needs to be given the capture filters ANALOGVIDEO input pin
		IEnumPins *pins = NULL;
		if( ( mediaType.majortype == MEDIATYPE_Video ||
			mediaType.majortype == MEDIATYPE_Stream ) &&
			SUCCEEDED(p_filter->EnumPins(&pins)) ) {
			IPin        *pP = NULL;
			ULONG        n;
			PIN_INFO     pinInfo;
			BOOL         Found = FALSE;
			IKsPropertySet *pKs = NULL;
			GUID guid;
			DWORD dw;

			while( !Found && ( S_OK == pins->Next(1, &pP, &n) ) ) {
				if( S_OK == pP->QueryPinInfo(&pinInfo) ) {
					// is this pin an ANALOGVIDEOIN input pin?
					if( pinInfo.dir == PINDIR_INPUT &&
						pP->QueryInterface( IID_IKsPropertySet,
						(void **)&pKs ) == S_OK ) {
						if( pKs->Get( AMPROPSETID_Pin,
							AMPROPERTY_PIN_CATEGORY, NULL, 0,
							&guid, sizeof(GUID), &dw ) == S_OK ) {
							if( guid == PIN_CATEGORY_ANALOGVIDEOIN ) {
								// recursively search crossbar routes
								FindCrossbarRoutes( pP, 0 );
								// found it
								Found = TRUE;
							}
						}
						pKs->Release();
					}
					pinInfo.pFilter->Release();
				}
				pP->Release();
			}
			pins->Release();
			logger_info( LOGM_GLOBAL, "ConnectFilters: graph_builder2 available.") ;
			if ( !Found )
				logger_warn( LOGM_GLOBAL, "ConnectFilters: No crossBar routes found (incobatible pin types)" ) ;
		}
		return true;
	}
	else {
		IEnumPins *p_enumpins;
		IPin *p_pin;

		if( S_OK != p_filter->EnumPins( &p_enumpins ) ) return false;

		while( S_OK == p_enumpins->Next( 1, &p_pin, NULL ) ) {
			PIN_DIRECTION pin_dir;
			p_pin->QueryDirection( &pin_dir );

			if( pin_dir == PINDIR_OUTPUT &&
				m_p_graph->ConnectDirect( p_pin, (IPin *)p_input_pin, 0 ) == S_OK ) {
				p_pin->Release();
				p_enumpins->Release();
				return true;
			}
			p_pin->Release();
		}

		p_enumpins->Release();
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
//crossbar stuff

/*****************************************************************************
* RouteCrossbars (Does not AddRef the returned *Pin)
*****************************************************************************/
static HRESULT GetCrossbarIPinAtIndex( IAMCrossbar *pXbar, LONG PinIndex,
	BOOL IsInputPin, IPin ** ppPin )
{
	LONG         cntInPins, cntOutPins;
	IPin        *pP = NULL;
	IBaseFilter *pFilter = NULL;
	IEnumPins   *pins = NULL;
	ULONG        n;

	if( !pXbar || !ppPin ) return E_POINTER;

	*ppPin = 0;

	if( S_OK != pXbar->get_PinCounts(&cntOutPins, &cntInPins) ) return E_FAIL;

	LONG TrueIndex = IsInputPin ? PinIndex : PinIndex + cntInPins;

	if( pXbar->QueryInterface(IID_IBaseFilter, (void **)&pFilter) == S_OK ) {
		if( SUCCEEDED(pFilter->EnumPins(&pins)) ) {
			LONG i = 0;
			while( pins->Next(1, &pP, &n) == S_OK ) {
				pP->Release();
				if( i == TrueIndex ) {
					*ppPin = pP;
					break;
				}
				i++;
			}
			pins->Release();
		}
		pFilter->Release();
	}

	return *ppPin ? S_OK : E_FAIL;
}

/*****************************************************************************
* GetCrossbarIndexFromIPin: Find corresponding index of an IPin on a crossbar
*****************************************************************************/
static HRESULT GetCrossbarIndexFromIPin( IAMCrossbar * pXbar, LONG * PinIndex,
	BOOL IsInputPin, IPin * pPin )
{
	LONG         cntInPins, cntOutPins;
	IPin        *pP = NULL;
	IBaseFilter *pFilter = NULL;
	IEnumPins   *pins = NULL;
	ULONG        n;
	BOOL         fOK = FALSE;

	if(!pXbar || !PinIndex || !pPin )
		return E_POINTER;

	if( S_OK != pXbar->get_PinCounts(&cntOutPins, &cntInPins) )
		return E_FAIL;

	if( pXbar->QueryInterface(IID_IBaseFilter, (void **)&pFilter) == S_OK ) {
		if( SUCCEEDED(pFilter->EnumPins(&pins)) ) {
			LONG i=0;

			while( pins->Next(1, &pP, &n) == S_OK ) {
				pP->Release();
				if( pPin == pP ) {
					*PinIndex = IsInputPin ? i : i - cntInPins;
					fOK = TRUE;
					break;
				}
				i++;
			}
			pins->Release();
		}
		pFilter->Release();
	}

	return fOK ? S_OK : E_FAIL;
}

void CDShowCapture::DeleteCrossbarRoutes()
{
	/* Remove crossbar filters from graph */
	for( int i = 0; i < m_i_crossbar_route_depth; i++ ) {
		m_crossbar_routes[i].pXbar->Release();
	}
	m_i_crossbar_route_depth = 0;
}

HRESULT CDShowCapture::FindCrossbarRoutes( IPin *p_input_pin, LONG physicalType, int depth )
{
	HRESULT result = S_FALSE;

	IPin *p_output_pin;
	if( FAILED(p_input_pin->ConnectedTo(&p_output_pin)) ) return S_FALSE;

	// It is connected, so now find out if the filter supports IAMCrossbar
	PIN_INFO pinInfo;
	if( FAILED(p_output_pin->QueryPinInfo(&pinInfo)) ||
		PINDIR_OUTPUT != pinInfo.dir )
	{
		p_output_pin->Release ();
		return S_FALSE;
	}

	IAMCrossbar *pXbar = NULL;
	if( FAILED(pinInfo.pFilter->QueryInterface(IID_IAMCrossbar,
		(void **)&pXbar)) ) {
		pinInfo.pFilter->Release();
		p_output_pin->Release ();
		return S_FALSE;
	}

	LONG inputPinCount, outputPinCount;
	if( FAILED(pXbar->get_PinCounts(&outputPinCount, &inputPinCount)) ) {
		pXbar->Release();
		pinInfo.pFilter->Release();
		p_output_pin->Release ();
		return S_FALSE;
	}

	LONG inputPinIndexRelated, outputPinIndexRelated;
	LONG inputPinPhysicalType = 0, outputPinPhysicalType;
	LONG inputPinIndex = 0, outputPinIndex;
	if( FAILED(GetCrossbarIndexFromIPin( pXbar, &outputPinIndex,
		FALSE, p_output_pin )) ||
		FAILED(pXbar->get_CrossbarPinInfo( FALSE, outputPinIndex,
		&outputPinIndexRelated,
		&outputPinPhysicalType )) ) {
		pXbar->Release();
		pinInfo.pFilter->Release();
		p_output_pin->Release ();
		return S_FALSE;
	}

	/*
	** if physical type is 0, then use default/existing route to physical connector
	*/
	if( physicalType == 0 ) {
		/* use following as default connector type if we fail to find an existing route */
		physicalType = PhysConn_Video_Tuner;
		if( SUCCEEDED(pXbar->get_IsRoutedTo(outputPinIndex, &inputPinIndex)) ) {

			if( SUCCEEDED( pXbar->get_CrossbarPinInfo( TRUE,  inputPinIndex,
				&inputPinIndexRelated, &inputPinPhysicalType )) ) {
				// remember connector type
				physicalType = inputPinPhysicalType;

				logger_info( LOGM_GLOBAL, "found existing route for output %ld (type %s) to input %ld (type %s)",
					outputPinIndex, GetPhysicalPinName( outputPinPhysicalType ),
					inputPinIndex, GetPhysicalPinName( inputPinPhysicalType ) );

				// fall through to for loop, note 'inputPinIndex' is set to the pin we are looking for
				// hence, loop iteration should not wind back

			}
		}
		else {
			// reset to first pin for complete loop iteration
			inputPinIndex = 0;
		}
	}

	//
	// for all input pins
	//
	for( /* inputPinIndex has been set */ ; (S_OK != result) && (inputPinIndex < inputPinCount); ++inputPinIndex ) {
		if( FAILED(pXbar->get_CrossbarPinInfo( TRUE,  inputPinIndex,
			&inputPinIndexRelated, &inputPinPhysicalType )) ) continue;

		// Is this pin matching required connector physical type?
		if( inputPinPhysicalType != physicalType ) continue;

		// Can we route it?
		if( FAILED(pXbar->CanRoute(outputPinIndex, inputPinIndex)) ) continue;


		IPin *pPin;
		if( FAILED(GetCrossbarIPinAtIndex( pXbar, inputPinIndex,
			TRUE, &pPin)) ) continue;

		result = FindCrossbarRoutes( pPin, physicalType, depth+1 );

		if( S_OK == result || (S_FALSE == result &&
			physicalType == inputPinPhysicalType &&
			(m_i_crossbar_route_depth = depth+1) < MAX_CROSSBAR_DEPTH) ) {
			// hold on crossbar, will be released when graph is destroyed
			pXbar->AddRef();

			// remember crossbar route
			m_crossbar_routes[depth].pXbar = pXbar;
			m_crossbar_routes[depth].VideoInputIndex = inputPinIndex;
			m_crossbar_routes[depth].VideoOutputIndex = outputPinIndex;
			m_crossbar_routes[depth].AudioInputIndex = inputPinIndexRelated;
			m_crossbar_routes[depth].AudioOutputIndex = outputPinIndexRelated;

			logger_info( LOGM_GLOBAL, "crossbar at depth %d, found route for "
				"output %ld (type %s) to input %ld (type %s)", depth,
				outputPinIndex, GetPhysicalPinName( outputPinPhysicalType ),
				inputPinIndex, GetPhysicalPinName( inputPinPhysicalType ) );

			result = S_OK;
		}
	}

	pXbar->Release();
	pinInfo.pFilter->Release();
	p_output_pin->Release ();

	return result;
}
