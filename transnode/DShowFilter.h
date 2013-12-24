/*****************************************************************************
 * DShowFilter.h :
 *****************************************************************************
 * Copyright (C) 2010 Broad Intelligence Technologies
 *
 * Author:
 *
 * Notes: It only work on Windows
 *****************************************************************************/

#ifndef __DSHOWFILTER_H__
#define __DSHOWFILTER_H__

#include <string>
#include <list>
#include <deque>
using namespace std;

#include <dshow.h>

typedef __int64 mtime_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;

#define VLC_FOURCC( a, b, c, d ) \
        ( ((uint32_t)a) | ( ((uint32_t)b) << 8 ) \
           | ( ((uint32_t)c) << 16 ) | ( ((uint32_t)d) << 24 ) )

#define VLC_CODEC_Y211      VLC_FOURCC('Y','2','1','1')
#define VLC_CODEC_MPGV      VLC_FOURCC('m','p','g','v')
#define VLC_CODEC_DV        VLC_FOURCC('d','v',' ',' ')
#define VLC_CODEC_MJPG      VLC_FOURCC('M','J','P','G')
#define VLC_CODEC_MP4V      VLC_FOURCC('m','p','4','v')
#define VLC_CODEC_UYVY      VLC_FOURCC('U','Y','V','Y')
#define VLC_CODEC_YV12      VLC_FOURCC('Y','V','1','2')
/* 8 bits RGB */
#define VLC_CODEC_RGB8      VLC_FOURCC('R','G','B','2')
/* 15 bits RGB stored on 16 bits */
#define VLC_CODEC_RGB15     VLC_FOURCC('R','V','1','5')
/* 16 bits RGB store on a 16 bits */
#define VLC_CODEC_RGB16     VLC_FOURCC('R','V','1','6')
/* 24 bits RGB */
#define VLC_CODEC_RGB24     VLC_FOURCC('R','V','2','4')
/* 32 bits RGB */
#define VLC_CODEC_RGB32     VLC_FOURCC('R','V','3','2')
/* 32 bits VLC RGBA */
#define VLC_CODEC_RGBA      VLC_FOURCC('R','G','B','A')
/* Planar YUV 4:2:0 Y:V:U */
#define VLC_CODEC_YV12      VLC_FOURCC('Y','V','1','2')
/* Planar YUV 4:1:0 Y:U:V */
#define VLC_CODEC_I410      VLC_FOURCC('I','4','1','0')
/* Planar YUV 4:1:1 Y:U:V */
#define VLC_CODEC_I411      VLC_FOURCC('I','4','1','1')
/* Planar YUV 4:2:0 Y:U:V */
#define VLC_CODEC_I420      VLC_FOURCC('I','4','2','0')
/* Planar YUV 4:2:2 Y:U:V */
#define VLC_CODEC_I422      VLC_FOURCC('I','4','2','2')
/* Planar YUV 4:4:0 Y:U:V */
#define VLC_CODEC_I440      VLC_FOURCC('I','4','4','0')
/* Planar YUV 4:4:4 Y:U:V */
#define VLC_CODEC_I444      VLC_FOURCC('I','4','4','4')
/* Packed YUV 4:2:2, Y:V:Y:U */
#define VLC_CODEC_YVYU      VLC_FOURCC('Y','V','Y','U')
/* Packed YUV 4:2:2, Y:U:Y:V */
#define VLC_CODEC_YUYV      VLC_FOURCC('Y','U','Y','2')

#define VLC_CODEC_FL32      VLC_FOURCC('f','l','3','2')

typedef struct VLCMediaSample
{
    IMediaSample *p_sample;
    mtime_t i_timestamp;

} VLCMediaSample;

class CaptureFilter;

void WINAPI FreeMediaType( AM_MEDIA_TYPE& mt );
HRESULT WINAPI CopyMediaType( AM_MEDIA_TYPE *pmtTarget,
                              const AM_MEDIA_TYPE *pmtSource );

int GetFourCCFromMediaType(const AM_MEDIA_TYPE &media_type);

/****************************************************************************
 * Declaration of our dummy directshow filter pin class
 ****************************************************************************/
class CapturePin: public IPin, public IMemInputPin
{
    friend class CaptureEnumMediaTypes;

    CaptureFilter  *p_filter;

    IPin *p_connected_pin;

    AM_MEDIA_TYPE *media_types;
    size_t media_type_count;

    AM_MEDIA_TYPE cx_media_type;

    deque<VLCMediaSample> samples_queue;

    long i_ref;

  public:
    CapturePin( CaptureFilter *_p_filter,
                AM_MEDIA_TYPE *mt, size_t mt_count );
    virtual ~CapturePin();

    /* IUnknown methods */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    /* IPin methods */
    STDMETHODIMP Connect( IPin * pReceivePin, const AM_MEDIA_TYPE *pmt );
    STDMETHODIMP ReceiveConnection( IPin * pConnector,
                                    const AM_MEDIA_TYPE *pmt );
    STDMETHODIMP Disconnect();
    STDMETHODIMP ConnectedTo( IPin **pPin );
    STDMETHODIMP ConnectionMediaType( AM_MEDIA_TYPE *pmt );
    STDMETHODIMP QueryPinInfo( PIN_INFO * pInfo );
    STDMETHODIMP QueryDirection( PIN_DIRECTION * pPinDir );
    STDMETHODIMP QueryId( LPWSTR * Id );
    STDMETHODIMP QueryAccept( const AM_MEDIA_TYPE *pmt );
    STDMETHODIMP EnumMediaTypes( IEnumMediaTypes **ppEnum );
    STDMETHODIMP QueryInternalConnections( IPin* *apPin, ULONG *nPin );
    STDMETHODIMP EndOfStream( void );

    STDMETHODIMP BeginFlush( void );
    STDMETHODIMP EndFlush( void );
    STDMETHODIMP NewSegment( REFERENCE_TIME tStart, REFERENCE_TIME tStop,
                             double dRate );

    /* IMemInputPin methods */
    STDMETHODIMP GetAllocator( IMemAllocator **ppAllocator );
    STDMETHODIMP NotifyAllocator(  IMemAllocator *pAllocator, BOOL bReadOnly );
    STDMETHODIMP GetAllocatorRequirements( ALLOCATOR_PROPERTIES *pProps );
    STDMETHODIMP Receive( IMediaSample *pSample );
    STDMETHODIMP ReceiveMultiple( IMediaSample **pSamples, long nSamples,
                                  long *nSamplesProcessed );
    STDMETHODIMP ReceiveCanBlock( void );

    /* Custom methods */
    HRESULT CustomGetSample( VLCMediaSample * );
    HRESULT CustomGetSamples( deque<VLCMediaSample> &external_queue );

    AM_MEDIA_TYPE &CustomGetMediaType();
};

/****************************************************************************
 * Declaration of our dummy directshow filter class
 ****************************************************************************/
class CaptureFilter : public IBaseFilter
{
    friend class CapturePin;

    CapturePin     *p_pin;
    IFilterGraph   *p_graph;
    //AM_MEDIA_TYPE  media_type;
    FILTER_STATE   state;

    long i_ref;

  public:
    CaptureFilter( AM_MEDIA_TYPE *mt, size_t mt_count );
    virtual ~CaptureFilter();

    /* IUnknown methods */
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    /* IPersist method */
    STDMETHODIMP GetClassID(CLSID *pClsID);

    /* IMediaFilter methods */
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);
    STDMETHODIMP SetSyncSource(IReferenceClock *pClock);
    STDMETHODIMP GetSyncSource(IReferenceClock **pClock);
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);

    /* IBaseFilter methods */
    STDMETHODIMP EnumPins( IEnumPins ** ppEnum );
    STDMETHODIMP FindPin( LPCWSTR Id, IPin ** ppPin );
    STDMETHODIMP QueryFilterInfo( FILTER_INFO * pInfo );
    STDMETHODIMP JoinFilterGraph( IFilterGraph * pGraph, LPCWSTR pName );
    STDMETHODIMP QueryVendorInfo( LPWSTR* pVendorInfo );

    /* Custom methods */
    CapturePin *CustomGetPin();
};

/****************************************************************************
 * Declaration of our dummy directshow enumpins class
 ****************************************************************************/
class CaptureEnumPins : public IEnumPins
{
    CaptureFilter  *p_filter;

    int i_position;
    long i_ref;

public:
    CaptureEnumPins( CaptureFilter *_p_filter,
                     CaptureEnumPins *pEnumPins );
    virtual ~CaptureEnumPins();

    // IUnknown
    STDMETHODIMP QueryInterface( REFIID riid, void **ppv );
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumPins
    STDMETHODIMP Next( ULONG cPins, IPin ** ppPins, ULONG * pcFetched );
    STDMETHODIMP Skip( ULONG cPins );
    STDMETHODIMP Reset();
    STDMETHODIMP Clone( IEnumPins **ppEnum );
};

/****************************************************************************
 * Declaration of our dummy directshow enummediatypes class
 ****************************************************************************/
class CaptureEnumMediaTypes : public IEnumMediaTypes
{
    CapturePin     *p_pin;
    AM_MEDIA_TYPE cx_media_type;

    size_t i_position;
    long i_ref;

public:
    CaptureEnumMediaTypes( CapturePin *_p_pin,
                           CaptureEnumMediaTypes *pEnumMediaTypes );

    virtual ~CaptureEnumMediaTypes();

    // IUnknown
    STDMETHODIMP QueryInterface( REFIID riid, void **ppv );
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IEnumMediaTypes
    STDMETHODIMP Next( ULONG cMediaTypes, AM_MEDIA_TYPE ** ppMediaTypes,
                       ULONG * pcFetched );
    STDMETHODIMP Skip( ULONG cMediaTypes );
    STDMETHODIMP Reset();
    STDMETHODIMP Clone( IEnumMediaTypes **ppEnum );
};

#endif