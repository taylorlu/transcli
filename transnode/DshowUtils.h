#ifndef __LIBTRANSERVER_DSHOW_UTILS_H__
#define __LIBTRANSERVER_DSHOW_UTILS_H__

#include <string>
#include <list>
#include <dshow.h>

IBaseFilter *FindCaptureDevice( std::string *p_devicename, std::list<std::string> *p_listdevices, bool b_audio );
bool GetCaptureDevice(std::list<std::string> *p_list_devices, bool b_audio);
std::string GetCaptureDeviceXML(bool b_audio);

#endif
