/*****************************************************************************
 * DshowUtils.cpp : DirectShow Helper functions
 *****************************************************************************
 * Copyright (C) 2010 Broad Intelligence Technologies
 *
 * Author:
 *
 * Notes: It only work on Windows
 *****************************************************************************/
#include <sstream>

#include "logger.h"
#include "DshowUtils.h"

using namespace std;

#pragma comment(lib, "strmiids.lib")

static inline char *FromWide (const wchar_t *wide)
{
    size_t len = WideCharToMultiByte (CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (len == 0)
        return NULL;

    char *out = (char *)malloc (len);

    if (out)
        WideCharToMultiByte (CP_UTF8, 0, wide, -1, out, len, NULL, NULL);
    return out;
}

/* FindCaptureDevices:: This Function had two purposes :
    Returns the list of capture devices when p_listdevices != NULL
    Creates an IBaseFilter when p_devicename corresponds to an existing devname
   These actions *may* be requested whith a single call.
*/
IBaseFilter *
FindCaptureDevice( string *p_devicename,
                   list<string> *p_listdevices, bool b_audio )
{
    IBaseFilter *p_base_filter = NULL;
    IMoniker *p_moniker = NULL;
    ULONG i_fetched;
    HRESULT hr;
    list<string> devicelist;

    /* Create the system device enumerator */
    ICreateDevEnum *p_dev_enum = NULL;

    hr = CoCreateInstance( CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
                           IID_ICreateDevEnum, (void **)&p_dev_enum );
    if( FAILED(hr) )
    {
        logger_err( LOGM_GLOBAL, "failed to create the device enumerator (0x%lx)", hr);
        return NULL;
    }

    /* Create an enumerator for the video capture devices */
    IEnumMoniker *p_class_enum = NULL;
    if( !b_audio )
        hr = p_dev_enum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory,
                                                &p_class_enum, 0 );
    else
        hr = p_dev_enum->CreateClassEnumerator( CLSID_AudioInputDeviceCategory,
                                                &p_class_enum, 0 );
    p_dev_enum->Release();
    if( FAILED(hr) )
    {
        logger_err( LOGM_GLOBAL,  "failed to create the class enumerator (0x%lx)", hr );
        return NULL;
    }

    /* If there are no enumerators for the requested type, then
     * CreateClassEnumerator will succeed, but p_class_enum will be NULL */
    if( p_class_enum == NULL )
    {
        logger_err( LOGM_GLOBAL, "no %s capture device was detected", ( b_audio ? "audio" : "video" ) );
        return NULL;
    }

    /* Enumerate the devices */

    /* Note that if the Next() call succeeds but there are no monikers,
     * it will return S_FALSE (which is not a failure). Therefore, we check
     * that the return code is S_OK instead of using SUCCEEDED() macro. */

    while( p_class_enum->Next( 1, &p_moniker, &i_fetched ) == S_OK )
    {
        /* Getting the property page to get the device name */
        IPropertyBag *p_bag;
        hr = p_moniker->BindToStorage( 0, 0, IID_IPropertyBag,
                                       (void **)&p_bag );
        if( SUCCEEDED(hr) )
        {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = p_bag->Read( L"FriendlyName", &var, NULL );
            p_bag->Release();
            if( SUCCEEDED(hr) )
            {
                char *p_buf = FromWide( var.bstrVal );
                string devname = string(p_buf);
                free( p_buf) ;

                int dup = 0;
                /* find out if this name is already used by a previously found device */
                list<string>::const_iterator iter = devicelist.begin();
                list<string>::const_iterator end = devicelist.end();
                string ordevname = devname ;
                while ( iter != end )
                {
                    if( 0 == (*iter).compare( devname ) )
                    { /* devname is on the list. Try another name with sequence
                         number apended and then rescan until a unique entry is found*/
						char seq[16] = {0};
                        _snprintf(seq, 16, " #%d", ++dup);
                        devname = ordevname + seq;
                        iter = devicelist.begin();
                    }
                    else
                         ++iter;
                }
                devicelist.push_back( devname );

                if( p_devicename && *p_devicename == devname )
                {
                    logger_status( LOGM_GLOBAL, "asked for %s, binding to %s", p_devicename->c_str() , devname.c_str() ) ;
                    /* NULL possibly means we don't need BindMoniker BindCtx ?? */
                    hr = p_moniker->BindToObject( NULL, 0, IID_IBaseFilter,
                                                  (void **)&p_base_filter );
                    if( FAILED(hr) )
                    {
                        logger_err( LOGM_GLOBAL, "couldn't bind moniker to filter "
                                 "object (0x%lx)", hr );
                        p_moniker->Release();
                        p_class_enum->Release();
                        return NULL;
                    }
                    p_moniker->Release();
                    p_class_enum->Release();
                    return p_base_filter;
                }
            }
        }

        p_moniker->Release();
    }

    p_class_enum->Release();

    if( p_listdevices ) {
		devicelist.sort();
		*p_listdevices = devicelist;
    }
    return NULL;
}

bool GetCaptureDevice(list<string> *p_list_devices, bool b_audio)
{
	if (p_list_devices == NULL) return false;

	/* Initialize OLE/COM */
	CoInitialize( 0 );

	FindCaptureDevice( NULL, p_list_devices, b_audio );

	/* Uninitialize OLE/COM */
	CoUninitialize();
	return true;
}

string GetCaptureDeviceXML(bool b_audio)
{
	list<string> list_devices;
	std::stringstream s;

	GetCaptureDevice(&list_devices, b_audio);

	s << "<devices>";
	for (list<string>::iterator it = list_devices.begin(); 
								it != list_devices.end(); it++) {\
		s << "<device>" << *it << "</device>";
	}

	s << "</devices>";

	return s.str();
}