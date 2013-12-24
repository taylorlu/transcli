/*****************************************************************************
 * DShowFilter.cpp : DirectShow Filter
 *****************************************************************************
 * Copyright (C) 2010 Broad Intelligence Technologies
 *
 * Author:
 *
 * Notes: It only work on Windows
 *****************************************************************************/

#include "DShowFilter.h"
#include "logger.h"

#define DEBUG_DSHOW 1
#define DEBUG_DSHOW_L1 1

const GUID MEDIASUBTYPE_I420 = {0x30323449, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID MEDIASUBTYPE_HDYC = {0x43594448, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
/* DivX formats */
const GUID MEDIASUBTYPE_DIVX = {0x58564944, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

#define FILTER_NAME  L"Broadintel Capture Filter"
#define PIN_NAME     L"Capture"

#pragma comment(lib, "strmiids")

mtime_t mdate()
{
	return GetTickCount() * 1000LL;
}

void WINAPI FreeMediaType( AM_MEDIA_TYPE& mt )
{
    if( mt.cbFormat != 0 )
    {
        CoTaskMemFree( (PVOID)mt.pbFormat );
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if( mt.pUnk != NULL )
    {
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}

HRESULT WINAPI CopyMediaType( AM_MEDIA_TYPE *pmtTarget,
                              const AM_MEDIA_TYPE *pmtSource )
{
    *pmtTarget = *pmtSource;

    if( !pmtSource || !pmtTarget ) return S_FALSE;

    if( pmtSource->cbFormat && pmtSource->pbFormat )
    {
        pmtTarget->pbFormat = (PBYTE)CoTaskMemAlloc( pmtSource->cbFormat );
        if( pmtTarget->pbFormat == NULL )
        {
            pmtTarget->cbFormat = 0;
            return E_OUTOFMEMORY;
        }
        else
        {
            CopyMemory( (PVOID)pmtTarget->pbFormat, (PVOID)pmtSource->pbFormat,
                        pmtTarget->cbFormat );
        }
    }
    if( pmtTarget->pUnk != NULL )
    {
        pmtTarget->pUnk->AddRef();
    }

    return S_OK;
}

int GetFourCCFromMediaType( const AM_MEDIA_TYPE &media_type )
{
    int i_fourcc = 0;

    if( media_type.majortype == MEDIATYPE_Video )
    {
        /* currently only support this type of video info format */
        if( 1 /* media_type.formattype == FORMAT_VideoInfo */ )
        {
            /* Packed RGB formats */
            if( media_type.subtype == MEDIASUBTYPE_RGB1 )
               i_fourcc = VLC_FOURCC( 'R', 'G', 'B', '1' );
            else if( media_type.subtype == MEDIASUBTYPE_RGB4 )
               i_fourcc = VLC_FOURCC( 'R', 'G', 'B', '4' );
            else if( media_type.subtype == MEDIASUBTYPE_RGB8 )
               i_fourcc = VLC_FOURCC( 'R', 'G', 'B', '8' );
            else if( media_type.subtype == MEDIASUBTYPE_RGB555 )
               i_fourcc = VLC_CODEC_RGB15;
            else if( media_type.subtype == MEDIASUBTYPE_RGB565 )
               i_fourcc = VLC_CODEC_RGB16;
            else if( media_type.subtype == MEDIASUBTYPE_RGB24 )
               i_fourcc = VLC_CODEC_RGB24;
            else if( media_type.subtype == MEDIASUBTYPE_RGB32 )
               i_fourcc = VLC_CODEC_RGB32;
            else if( media_type.subtype == MEDIASUBTYPE_ARGB32 )
               i_fourcc = VLC_CODEC_RGBA;

            /* Planar YUV formats */
            else if( media_type.subtype == MEDIASUBTYPE_I420 )
               i_fourcc = VLC_CODEC_I420;
            else if( media_type.subtype == MEDIASUBTYPE_Y41P )
               i_fourcc = VLC_CODEC_I411;
            else if( media_type.subtype == MEDIASUBTYPE_YV12 )
               i_fourcc = VLC_CODEC_YV12;
            else if( media_type.subtype == MEDIASUBTYPE_IYUV )
               i_fourcc = VLC_CODEC_YV12;
            else if( media_type.subtype == MEDIASUBTYPE_YVU9 )
               i_fourcc = VLC_CODEC_I410;

            /* Packed YUV formats */
            else if( media_type.subtype == MEDIASUBTYPE_YVYU )
               i_fourcc = VLC_CODEC_YVYU;
            else if( media_type.subtype == MEDIASUBTYPE_YUYV )
               i_fourcc = VLC_CODEC_YUYV;
            else if( media_type.subtype == MEDIASUBTYPE_Y411 )
               i_fourcc = VLC_FOURCC( 'I', '4', '1', 'N' );
            else if( media_type.subtype == MEDIASUBTYPE_Y211 )
               i_fourcc = VLC_CODEC_Y211;
            else if( media_type.subtype == MEDIASUBTYPE_YUY2 )
               i_fourcc = VLC_CODEC_YUYV;
            else if( media_type.subtype == MEDIASUBTYPE_UYVY )
               i_fourcc = VLC_CODEC_UYVY;
            /* HDYC uses UYVY sample positions but Rec709 colourimetry */
            /* FIXME: When VLC understands colourspace, something will need
             * to be added / changed here. Until then, just make it behave
             * like UYVY */
            else if( media_type.subtype == MEDIASUBTYPE_HDYC )
                i_fourcc = VLC_CODEC_UYVY;

            /* MPEG2 video elementary stream */
            else if( media_type.subtype == MEDIASUBTYPE_MPEG2_VIDEO )
               i_fourcc = VLC_CODEC_MPGV;

            /* DivX video */
            else if( media_type.subtype == MEDIASUBTYPE_DIVX )
               i_fourcc = VLC_CODEC_MP4V;

            /* DV formats */
            else if( media_type.subtype == MEDIASUBTYPE_dvsl )
               i_fourcc = VLC_CODEC_DV;
            else if( media_type.subtype == MEDIASUBTYPE_dvsd )
               i_fourcc = VLC_CODEC_DV;
            else if( media_type.subtype == MEDIASUBTYPE_dvhd )
               i_fourcc = VLC_CODEC_DV;

            /* MJPEG format */
            else if( media_type.subtype == MEDIASUBTYPE_MJPG )
                i_fourcc = VLC_CODEC_MJPG;

        }
    }
    else if( media_type.majortype == MEDIATYPE_Audio )
    {
        /* currently only support this type of audio info format */
        if( media_type.formattype == FORMAT_WaveFormatEx )
        {
            if( media_type.subtype == MEDIASUBTYPE_PCM )
                i_fourcc = VLC_FOURCC( 'a', 'r', 'a', 'w' );
            else if( media_type.subtype == MEDIASUBTYPE_IEEE_FLOAT )
                i_fourcc = VLC_CODEC_FL32;
        }
    }
    else if( media_type.majortype == MEDIATYPE_Stream )
    {
        if( media_type.subtype == MEDIASUBTYPE_MPEG2_PROGRAM )
            i_fourcc = VLC_FOURCC( 'm', 'p', '2', 'p' );
        else if( media_type.subtype == MEDIASUBTYPE_MPEG2_TRANSPORT )
            i_fourcc = VLC_FOURCC( 'm', 'p', '2', 't' );
    }

    return i_fourcc;
}

/****************************************************************************
 * Implementation of our dummy directshow filter pin class
 ****************************************************************************/

CapturePin::CapturePin( CaptureFilter *_p_filter,
                        AM_MEDIA_TYPE *mt, size_t mt_count )
  : p_filter( _p_filter ),
    p_connected_pin( NULL ),  media_types(mt), media_type_count(mt_count),
    i_ref( 1 )
{
    cx_media_type.majortype = mt[0].majortype;
    cx_media_type.subtype   = GUID_NULL;
    cx_media_type.pbFormat  = NULL;
    cx_media_type.cbFormat  = 0;
    cx_media_type.pUnk      = NULL;
}

CapturePin::~CapturePin()
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::~CapturePin" );
#endif
    for( size_t c=0; c<media_type_count; c++ )
    {
        FreeMediaType(media_types[c]);
    }
    FreeMediaType(cx_media_type);
}

/**
 * Returns the complete queue of samples that have been received so far.
 * Lock the p_sys->lock before calling this function.
 * @param samples_queue [out] Empty queue that will get all elements from
 * the pin queue.
 * @return S_OK if a sample was available, S_FALSE if no sample was
 * available
 */
HRESULT CapturePin::CustomGetSamples( deque<VLCMediaSample> &external_queue )
{
#if 0 //def DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::CustomGetSamples: %d samples in the queue", samples_queue.size());
#endif

    if( !samples_queue.empty() )
    {
        external_queue.swap(samples_queue);
        return S_OK;
    }
    return S_FALSE;
}

/**
 * Returns a sample from its sample queue. Proper locking must be done prior
 * to this call. Current dshow code protects the access to any sample queue
 * (audio and video) with the p_sys->lock
 * @param vlc_sample [out] Address of a sample if sucessfull. Undefined
 * otherwise.
 * @return S_OK if a sample was available, S_FALSE if no sample was
 * available
 */
HRESULT CapturePin::CustomGetSample( VLCMediaSample *vlc_sample )
{
#if 0 //def DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::CustomGetSample" );
#endif

    if( samples_queue.size() )
    {
        *vlc_sample = samples_queue.back();
        samples_queue.pop_back();
        return S_OK;
    }
    return S_FALSE;
}

AM_MEDIA_TYPE &CapturePin::CustomGetMediaType()
{
    return cx_media_type;
}

/* IUnknown methods */
STDMETHODIMP CapturePin::QueryInterface(REFIID riid, void **ppv)
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CapturePin::QueryInterface" );
#endif

    if( riid == IID_IUnknown ||
        riid == IID_IPin )
    {
        AddRef();
        *ppv = (IPin *)this;
        return NOERROR;
    }
    if( riid == IID_IMemInputPin )
    {
        AddRef();
        *ppv = (IMemInputPin *)this;
        return NOERROR;
    }
    else
    {
#ifdef DEBUG_DSHOW_L1
        logger_status( LOGM_GLOBAL, "CapturePin::QueryInterface() failed for: "
                 "%04X-%02X-%02X-%02X%02X%02X%02X%02X%02X%02X%02X",
                 (int)riid.Data1, (int)riid.Data2, (int)riid.Data3,
                 static_cast<int>(riid.Data4[0]), (int)riid.Data4[1],
                 (int)riid.Data4[2], (int)riid.Data4[3],
                 (int)riid.Data4[4], (int)riid.Data4[5],
                 (int)riid.Data4[6], (int)riid.Data4[7] );
#endif
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CapturePin::AddRef()
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CapturePin::AddRef (ref: %i)", i_ref );
#endif

    return i_ref++;
}

STDMETHODIMP_(ULONG) CapturePin::Release()
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CapturePin::Release (ref: %i)", i_ref );
#endif

    if( !InterlockedDecrement(&i_ref) ) delete this;

    return 0;
}

/* IPin methods */
STDMETHODIMP CapturePin::Connect( IPin * pReceivePin,
                                  const AM_MEDIA_TYPE *pmt )
{
    if( State_Running == p_filter->state )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::Connect [not stopped]" );
        return VFW_E_NOT_STOPPED;
    }

    if( p_connected_pin )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::Connect [already connected]" );
        return VFW_E_ALREADY_CONNECTED;
    }

    if( !pmt ) return S_OK;
 
    if( GUID_NULL != pmt->majortype &&
        media_types[0].majortype != pmt->majortype )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::Connect [media major type mismatch]" );
        return S_FALSE;
    }

    if( GUID_NULL != pmt->subtype && !GetFourCCFromMediaType(*pmt) )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::Connect [media subtype type "
                 "not supported]" );
        return S_FALSE;
    }

    if( pmt->pbFormat && pmt->majortype == MEDIATYPE_Video  )
    {
        if( !((VIDEOINFOHEADER *)pmt->pbFormat)->bmiHeader.biHeight ||
            !((VIDEOINFOHEADER *)pmt->pbFormat)->bmiHeader.biWidth )
        {
            logger_status( LOGM_GLOBAL, "CapturePin::Connect "
                     "[video width/height == 0 ]" );
            return S_FALSE;
        }
    }

    logger_status( LOGM_GLOBAL, "CapturePin::Connect [OK]" );
    return S_OK;
}

STDMETHODIMP CapturePin::ReceiveConnection( IPin * pConnector,
                                            const AM_MEDIA_TYPE *pmt )
{
    if( State_Stopped != p_filter->state )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::ReceiveConnection [not stopped]" );
        return VFW_E_NOT_STOPPED;
    }

    if( !pConnector || !pmt )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::ReceiveConnection [null pointer]" );
        return E_POINTER;
    }

    if( p_connected_pin )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::ReceiveConnection [already connected]");
        return VFW_E_ALREADY_CONNECTED;
    }

    if( S_OK != QueryAccept(pmt) )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::ReceiveConnection "
                 "[media type not accepted]" );
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    logger_status( LOGM_GLOBAL, "CapturePin::ReceiveConnection [OK]" );

    p_connected_pin = pConnector;
    p_connected_pin->AddRef();

    FreeMediaType( cx_media_type );
    return CopyMediaType( &cx_media_type, pmt );
}

STDMETHODIMP CapturePin::Disconnect()
{
    if( ! p_connected_pin )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::Disconnect [not connected]" );
        return S_FALSE;
    }

    logger_status( LOGM_GLOBAL, "CapturePin::Disconnect [OK]" );

    /* samples_queue was already flushed in EndFlush() */

    p_connected_pin->Release();
    p_connected_pin = NULL;
    //FreeMediaType( cx_media_type );
    //cx_media_type.subtype = GUID_NULL;

    return S_OK;
}

STDMETHODIMP CapturePin::ConnectedTo( IPin **pPin )
{
    if( !p_connected_pin )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::ConnectedTo [not connected]" );
        return VFW_E_NOT_CONNECTED;
    }

    p_connected_pin->AddRef();
    *pPin = p_connected_pin;

    logger_status( LOGM_GLOBAL, "CapturePin::ConnectedTo [OK]" );

    return S_OK;
}

STDMETHODIMP CapturePin::ConnectionMediaType( AM_MEDIA_TYPE *pmt )
{
    if( !p_connected_pin )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::ConnectionMediaType [not connected]" );
        return VFW_E_NOT_CONNECTED;
    }

    return CopyMediaType( pmt, &cx_media_type );
}

STDMETHODIMP CapturePin::QueryPinInfo( PIN_INFO * pInfo )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::QueryPinInfo" );
#endif

    pInfo->pFilter = p_filter;
    if( p_filter ) p_filter->AddRef();

    memcpy(pInfo->achName, PIN_NAME, sizeof(PIN_NAME));
    pInfo->dir = PINDIR_INPUT;

    return NOERROR;
}

STDMETHODIMP CapturePin::QueryDirection( PIN_DIRECTION * pPinDir )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::QueryDirection" );
#endif

    *pPinDir = PINDIR_INPUT;
    return NOERROR;
}

STDMETHODIMP CapturePin::QueryId( LPWSTR * Id )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::QueryId" );
#endif

    *Id = L"VideoLAN Capture Pin";

    return S_OK;
}
STDMETHODIMP CapturePin::QueryAccept( const AM_MEDIA_TYPE *pmt )
{
    if( State_Stopped != p_filter->state )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::QueryAccept [not stopped]" );
        return S_FALSE;
    }

    if( media_types[0].majortype != pmt->majortype )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::QueryAccept [media type mismatch]" );
        return S_FALSE;
    }

    int i_fourcc = GetFourCCFromMediaType(*pmt);
    if( !i_fourcc )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::QueryAccept "
                 "[media type not supported]" );
        return S_FALSE;
    }

    if( pmt->majortype == MEDIATYPE_Video )
    {
        if( pmt->pbFormat &&
            ( (((VIDEOINFOHEADER *)pmt->pbFormat)->bmiHeader.biHeight == 0) ||
              (((VIDEOINFOHEADER *)pmt->pbFormat)->bmiHeader.biWidth == 0) ) )
        {
            logger_status( LOGM_GLOBAL, "CapturePin::QueryAccept [video size wxh == 0]");
            return S_FALSE;
        }

        logger_status( LOGM_GLOBAL, "CapturePin::QueryAccept [OK] "
                 "(width=%ld, height=%ld, chroma=%4.4s, fps=%f)",
                 ((VIDEOINFOHEADER *)pmt->pbFormat)->bmiHeader.biWidth,
                 ((VIDEOINFOHEADER *)pmt->pbFormat)->bmiHeader.biHeight,
                 (char *)&i_fourcc,
         10000000.0f/((float)((VIDEOINFOHEADER *)pmt->pbFormat)->AvgTimePerFrame) );
    }
    else if( pmt->majortype == MEDIATYPE_Audio )
    {
        logger_status( LOGM_GLOBAL, "CapturePin::QueryAccept [OK] (channels=%d, "
                 "samples/sec=%lu, bits/samples=%d, format=%4.4s)",
                 ((WAVEFORMATEX *)pmt->pbFormat)->nChannels,
                 ((WAVEFORMATEX *)pmt->pbFormat)->nSamplesPerSec,
                 ((WAVEFORMATEX *)pmt->pbFormat)->wBitsPerSample,
                 (char *)&i_fourcc );
    }
    else
    {
        logger_status( LOGM_GLOBAL, "CapturePin::QueryAccept [OK] (stream format=%4.4s)",
                 (char *)&i_fourcc );
    }

    if( p_connected_pin )
    {
        FreeMediaType( cx_media_type );
        CopyMediaType( &cx_media_type, pmt );
    }

    return S_OK;
}

STDMETHODIMP CapturePin::EnumMediaTypes( IEnumMediaTypes **ppEnum )
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CapturePin::EnumMediaTypes" );
#endif

    *ppEnum = new CaptureEnumMediaTypes( this, NULL );

    if( *ppEnum == NULL ) return E_OUTOFMEMORY;

    return NOERROR;
}

STDMETHODIMP CapturePin::QueryInternalConnections( IPin* *apPin, ULONG *nPin )
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CapturePin::QueryInternalConnections" );
#endif
    return E_NOTIMPL;
}

STDMETHODIMP CapturePin::EndOfStream( void )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::EndOfStream" );
#endif
    return S_OK;
}

STDMETHODIMP CapturePin::BeginFlush( void )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::BeginFlush" );
#endif
    return S_OK;
}

STDMETHODIMP CapturePin::EndFlush( void )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::EndFlush" );
#endif

    VLCMediaSample vlc_sample;

//FIXME:    vlc_mutex_lock( &p_sys->lock );
    while( samples_queue.size() )
    {
        vlc_sample = samples_queue.back();
        samples_queue.pop_back();
        vlc_sample.p_sample->Release();
    }
//FIXME:    vlc_mutex_unlock( &p_sys->lock );

    return S_OK;
}

STDMETHODIMP CapturePin::NewSegment( REFERENCE_TIME tStart,
                                     REFERENCE_TIME tStop,
                                     double dRate )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::NewSegment" );
#endif
    return S_OK;
}

/* IMemInputPin methods */
STDMETHODIMP CapturePin::GetAllocator( IMemAllocator **ppAllocator )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::GetAllocator" );
#endif

    return VFW_E_NO_ALLOCATOR;
}

STDMETHODIMP CapturePin::NotifyAllocator( IMemAllocator *pAllocator,
                                          BOOL bReadOnly )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::NotifyAllocator" );
#endif

    return S_OK;
}

STDMETHODIMP CapturePin::GetAllocatorRequirements( ALLOCATOR_PROPERTIES *pProps )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::GetAllocatorRequirements" );
#endif

    return E_NOTIMPL;
}

STDMETHODIMP CapturePin::Receive( IMediaSample *pSample )
{
#if 0 //def DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::Receive" );
#endif

    pSample->AddRef();
    mtime_t i_timestamp = mdate() * 10;
    VLCMediaSample vlc_sample = {pSample, i_timestamp};

//FIXME:    vlc_mutex_lock( &p_sys->lock );
    samples_queue.push_front( vlc_sample );

    /* Make sure we don't cache too many samples */
    if( samples_queue.size() > 10 )
    {
        vlc_sample = samples_queue.back();
        samples_queue.pop_back();
        logger_status( LOGM_GLOBAL, "CapturePin::Receive trashing late input sample" );
        vlc_sample.p_sample->Release();
    }

//FIXME:    vlc_cond_signal( &p_sys->wait );
//FIXME:    vlc_mutex_unlock( &p_sys->lock );

    return S_OK;
}

STDMETHODIMP CapturePin::ReceiveMultiple( IMediaSample **pSamples,
                                          long nSamples,
                                          long *nSamplesProcessed )
{
    HRESULT hr = S_OK;

    *nSamplesProcessed = 0;
    while( nSamples-- > 0 )
    {
         hr = Receive( pSamples[*nSamplesProcessed] );
         if( hr != S_OK ) break;
         (*nSamplesProcessed)++;
    }
    return hr;
}

STDMETHODIMP CapturePin::ReceiveCanBlock( void )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CapturePin::ReceiveCanBlock" );
#endif

    return S_FALSE; /* Thou shalt not block */
}

/****************************************************************************
 * Implementation of our dummy directshow filter class
 ****************************************************************************/
CaptureFilter::CaptureFilter( AM_MEDIA_TYPE *mt, size_t mt_count )
  : p_pin( new CapturePin( this, mt, mt_count ) ),
    state( State_Stopped ), i_ref( 1 )
{
}

CaptureFilter::~CaptureFilter()
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::~CaptureFilter" );
#endif
    p_pin->Release();
}

/* IUnknown methods */
STDMETHODIMP CaptureFilter::QueryInterface( REFIID riid, void **ppv )
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureFilter::QueryInterface" );
#endif

    if( riid == IID_IUnknown )
    {
        AddRef();
        *ppv = (IUnknown *)this;
        return NOERROR;
    }
    if( riid == IID_IPersist )
    {
        AddRef();
        *ppv = (IPersist *)this;
        return NOERROR;
    }
    if( riid == IID_IMediaFilter )
    {
        AddRef();
        *ppv = (IMediaFilter *)this;
        return NOERROR;
    }
    if( riid == IID_IBaseFilter )
    {
        AddRef();
        *ppv = (IBaseFilter *)this;
        return NOERROR;
    }
    else
    {
#ifdef DEBUG_DSHOW_L1
        logger_status( LOGM_GLOBAL, "CaptureFilter::QueryInterface() failed for: "
                 "%04X-%02X-%02X-%02X%02X%02X%02X%02X%02X%02X%02X",
                 (int)riid.Data1, (int)riid.Data2, (int)riid.Data3,
                 static_cast<int>(riid.Data4[0]), (int)riid.Data4[1],
                 (int)riid.Data4[2], (int)riid.Data4[3],
                 (int)riid.Data4[4], (int)riid.Data4[5],
                 (int)riid.Data4[6], (int)riid.Data4[7] );
#endif
        *ppv = NULL;
        return E_NOINTERFACE;
    }
};
STDMETHODIMP_(ULONG) CaptureFilter::AddRef()
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureFilter::AddRef (ref: %i)", i_ref );
#endif

    return i_ref++;
};
STDMETHODIMP_(ULONG) CaptureFilter::Release()
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureFilter::Release (ref: %i)", i_ref );
#endif

    if( !InterlockedDecrement(&i_ref) ) delete this;

    return 0;
};

/* IPersist method */
STDMETHODIMP CaptureFilter::GetClassID(CLSID *pClsID)
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::GetClassID" );
#endif
    return E_NOTIMPL;
};

/* IMediaFilter methods */
STDMETHODIMP CaptureFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::GetState %i", state );
#endif

    *State = state;
    return S_OK;
};
STDMETHODIMP CaptureFilter::SetSyncSource(IReferenceClock *pClock)
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::SetSyncSource" );
#endif

    return S_OK;
};
STDMETHODIMP CaptureFilter::GetSyncSource(IReferenceClock **pClock)
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::GetSyncSource" );
#endif

    *pClock = NULL;
    return NOERROR;
};
STDMETHODIMP CaptureFilter::Stop()
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::Stop" );
#endif

    p_pin->EndFlush();

    state = State_Stopped;
    return S_OK;
};
STDMETHODIMP CaptureFilter::Pause()
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::Pause" );
#endif

    state = State_Paused;
    return S_OK;
};
STDMETHODIMP CaptureFilter::Run(REFERENCE_TIME tStart)
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::Run" );
#endif

    state = State_Running;
    return S_OK;
};

/* IBaseFilter methods */
STDMETHODIMP CaptureFilter::EnumPins( IEnumPins ** ppEnum )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::EnumPins" );
#endif

    /* Create a new ref counted enumerator */
    *ppEnum = new CaptureEnumPins( this, NULL );
    return *ppEnum == NULL ? E_OUTOFMEMORY : NOERROR;
};
STDMETHODIMP CaptureFilter::FindPin( LPCWSTR Id, IPin ** ppPin )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::FindPin" );
#endif
    return E_NOTIMPL;
};
STDMETHODIMP CaptureFilter::QueryFilterInfo( FILTER_INFO * pInfo )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::QueryFilterInfo" );
#endif

    memcpy(pInfo->achName, FILTER_NAME, sizeof(FILTER_NAME));

    pInfo->pGraph = p_graph;
    if( p_graph ) p_graph->AddRef();

    return NOERROR;
};
STDMETHODIMP CaptureFilter::JoinFilterGraph( IFilterGraph * pGraph,
                                             LPCWSTR pName )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::JoinFilterGraph" );
#endif

    p_graph = pGraph;

    return NOERROR;
};
STDMETHODIMP CaptureFilter::QueryVendorInfo( LPWSTR* pVendorInfo )
{
#ifdef DEBUG_DSHOW
    logger_status( LOGM_GLOBAL, "CaptureFilter::QueryVendorInfo" );
#endif
    return E_NOTIMPL;
};

/* Custom methods */
CapturePin *CaptureFilter::CustomGetPin()
{
    return p_pin;
}

/****************************************************************************
 * Implementation of our dummy directshow enumpins class
 ****************************************************************************/

CaptureEnumPins::CaptureEnumPins( CaptureFilter *_p_filter,
                                  CaptureEnumPins *pEnumPins )
  : p_filter( _p_filter ), i_ref( 1 )
{
    /* Hold a reference count on our filter */
    p_filter->AddRef();

    /* Are we creating a new enumerator */

    if( pEnumPins == NULL )
    {
        i_position = 0;
    }
    else
    {
        i_position = pEnumPins->i_position;
    }
}

CaptureEnumPins::~CaptureEnumPins()
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumPins::~CaptureEnumPins" );
#endif
    p_filter->Release();
}

/* IUnknown methods */
STDMETHODIMP CaptureEnumPins::QueryInterface( REFIID riid, void **ppv )
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumPins::QueryInterface" );
#endif

    if( riid == IID_IUnknown ||
        riid == IID_IEnumPins )
    {
        AddRef();
        *ppv = (IEnumPins *)this;
        return NOERROR;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
};
STDMETHODIMP_(ULONG) CaptureEnumPins::AddRef()
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumPins::AddRef (ref: %i)", i_ref );
#endif

    return i_ref++;
};
STDMETHODIMP_(ULONG) CaptureEnumPins::Release()
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumPins::Release (ref: %i)", i_ref );
#endif

    if( !InterlockedDecrement(&i_ref) ) delete this;

    return 0;
};

/* IEnumPins */
STDMETHODIMP CaptureEnumPins::Next( ULONG cPins, IPin ** ppPins,
                                    ULONG * pcFetched )
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumPins::Next" );
#endif

    unsigned int i_fetched = 0;

    if( i_position < 1 && cPins > 0 )
    {
        IPin *pPin = p_filter->CustomGetPin();
        *ppPins = pPin;
        pPin->AddRef();
        i_fetched = 1;
        i_position++;
    }

    if( pcFetched ) *pcFetched = i_fetched;

    return (i_fetched == cPins) ? S_OK : S_FALSE;
}

STDMETHODIMP CaptureEnumPins::Skip( ULONG cPins )
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumPins::Skip" );
#endif

    i_position += cPins;

    if( i_position > 1 )
    {
        return S_FALSE;
    }

    return S_OK;
}

STDMETHODIMP CaptureEnumPins::Reset()
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumPins::Reset" );
#endif

    i_position = 0;
    return S_OK;
}

STDMETHODIMP CaptureEnumPins::Clone( IEnumPins **ppEnum )
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumPins::Clone" );
#endif

    *ppEnum = new CaptureEnumPins( p_filter, this );
    if( *ppEnum == NULL ) return E_OUTOFMEMORY;

    return NOERROR;
}

/****************************************************************************
 * Implementation of our dummy directshow enummediatypes class
 ****************************************************************************/
CaptureEnumMediaTypes::CaptureEnumMediaTypes( CapturePin *_p_pin, 
	CaptureEnumMediaTypes *pEnumMediaTypes )
  : p_pin( _p_pin ), i_ref( 1 )
{
    /* Hold a reference count on our filter */
    p_pin->AddRef();

    /* Are we creating a new enumerator */
    if( pEnumMediaTypes == NULL )
    {
        CopyMediaType(&cx_media_type, &p_pin->cx_media_type);
        i_position = 0;
    }
    else
    {
        CopyMediaType(&cx_media_type, &pEnumMediaTypes->cx_media_type);
        i_position = pEnumMediaTypes->i_position;
    }
}

CaptureEnumMediaTypes::~CaptureEnumMediaTypes()
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumMediaTypes::~CaptureEnumMediaTypes" );
#endif
    FreeMediaType(cx_media_type);
    p_pin->Release();
}

/* IUnknown methods */
STDMETHODIMP CaptureEnumMediaTypes::QueryInterface( REFIID riid, void **ppv )
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumMediaTypes::QueryInterface" );
#endif

    if( riid == IID_IUnknown ||
        riid == IID_IEnumMediaTypes )
    {
        AddRef();
        *ppv = (IEnumMediaTypes *)this;
        return NOERROR;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CaptureEnumMediaTypes::AddRef()
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumMediaTypes::AddRef (ref: %i)", i_ref );
#endif

    return i_ref++;
}

STDMETHODIMP_(ULONG) CaptureEnumMediaTypes::Release()
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumMediaTypes::Release (ref: %i)", i_ref );
#endif

    if( !InterlockedDecrement(&i_ref) ) delete this;

    return 0;
}

/* IEnumMediaTypes */
STDMETHODIMP CaptureEnumMediaTypes::Next( ULONG cMediaTypes,
                                          AM_MEDIA_TYPE ** ppMediaTypes,
                                          ULONG * pcFetched )
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumMediaTypes::Next " );
#endif
    ULONG copied = 0;
    ULONG offset = 0;
    ULONG max = p_pin->media_type_count;

    if( ! ppMediaTypes )
        return E_POINTER;

    if( (! pcFetched)  && (cMediaTypes > 1) )
       return E_POINTER;

    /*
    ** use connection media type as first entry in iterator if it exists
    */
    copied = 0;
    if( cx_media_type.subtype != GUID_NULL )
    {
        ++max;
        if( i_position == 0 )
        {
            ppMediaTypes[copied] =
                (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
            if( CopyMediaType(ppMediaTypes[copied], &cx_media_type) != S_OK )
                return E_OUTOFMEMORY;
            ++i_position;
            ++copied;
        }
    }

    while( (copied < cMediaTypes) && (i_position < max)  )
    {
        ppMediaTypes[copied] =
            (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        if( CopyMediaType( ppMediaTypes[copied],
                           &p_pin->media_types[i_position-offset]) != S_OK )
            return E_OUTOFMEMORY;

        ++copied;
        ++i_position;
    }

    if( pcFetched )  *pcFetched = copied;

    return (copied == cMediaTypes) ? S_OK : S_FALSE;
}

STDMETHODIMP CaptureEnumMediaTypes::Skip( ULONG cMediaTypes )
{
    ULONG max =  p_pin->media_type_count;
    if( cx_media_type.subtype != GUID_NULL )
    {
        max = 1;
    }
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumMediaTypes::Skip" );
#endif

    i_position += cMediaTypes;
    return (i_position < max) ? S_OK : S_FALSE;
}

STDMETHODIMP CaptureEnumMediaTypes::Reset()
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumMediaTypes::Reset" );
#endif

    FreeMediaType(cx_media_type);
    CopyMediaType(&cx_media_type, &p_pin->cx_media_type);
    i_position = 0;
    return S_OK;
}

STDMETHODIMP CaptureEnumMediaTypes::Clone( IEnumMediaTypes **ppEnum )
{
#ifdef DEBUG_DSHOW_L1
    logger_status( LOGM_GLOBAL, "CaptureEnumMediaTypes::Clone" );
#endif

    *ppEnum = new CaptureEnumMediaTypes( p_pin, this );
    if( *ppEnum == NULL ) return E_OUTOFMEMORY;

    return NOERROR;
};
