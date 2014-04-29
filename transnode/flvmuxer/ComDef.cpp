#include "ComDef.h"
#include <string>

#ifdef _WIN32
bool  CheckCreateFolder(const char *folder)
{
	if (folder && strlen("c:\\") <= strlen(folder)) {
		char path[1024] = {0}, tmp[1024] = {0};
		strcpy(path, folder);
// 		for (int i = strlen(path) - 1; i>=0; i--) {
// 			if ('.' == path[i]) {
// 				path[i] = '_';
// 				break;
// 			} else if ('\\' == path[i] || '/' == path[i]) {
// 				break;
// 			}
// 		}
		for (int i = strlen("c:\\"); i< strlen(path); i++) {
			if ('\\' == path[i] || '/' == path[i]) {
				memcpy(tmp, path, i);
				if (0 != mkdir(tmp) && EEXIST != errno) {
					return false;
				}
			}
		}
		if (0 != _mkdir(path) && EEXIST != errno) {
			return false;
		}

		return true;
	} 
	return false;
}
#endif


#ifdef _WIN32
#include <Windows.h>
bool  GetConvertStorePath(char *buf, int length)
{
	char str_all_user_path[1024]={0};
	GetEnvironmentVariableA("APPDATA",str_all_user_path, 1024);
	strcat(str_all_user_path, "\\YIConvert");
	if (CheckCreateFolder(str_all_user_path) && length <= strlen(str_all_user_path)) {
		return false;
	}
	strcpy(buf, str_all_user_path);
	return true;
}
#endif

#ifdef _WIN32
#include <Windows.h>
bool CheckDiskInfo(char* tS, char& dS, ULONGLONG& lFS)
{
	ULARGE_INTEGER lpFreeBytesAvailable;
	ULARGE_INTEGER lpTotalNumberOfBytes;
	ULARGE_INTEGER lpTotalNumberOfFreeBytes;
	bool result = GetDiskFreeSpaceExA(tS, &lpFreeBytesAvailable, &lpTotalNumberOfBytes, &lpTotalNumberOfFreeBytes);
	if(true == result)
	{
		if(lFS < lpFreeBytesAvailable.QuadPart)
		{
			lFS = lpFreeBytesAvailable.QuadPart;
			dS = tS[0];
		}
		return true;
	}
	return false;
}
char  GetTheLargestDisksSymbol()
{
	char diskSymbol = NULL;
	ULONGLONG largetFreeSpace = 0;
	char dS[3];
	dS[0] = 'D';dS[1] = ':';dS[2] = NULL;
	for(char i = 'D'; i < 'Z' + 1;i++)
	{
		dS[0] = i;
		CheckDiskInfo(dS, diskSymbol, largetFreeSpace);
	}
	return diskSymbol;
}

float GetDefaultBitrate( float fOriginBitrate, int iOutWidth, int iOutHeight )
{
	int iArea = iOutWidth * iOutHeight;
	if(iArea < 1280 * 720) return min(fOriginBitrate, 1000.0);
	if(iArea == 1280 * 720) return min(fOriginBitrate, 1500.0);
	if(iArea < 1920 * 1080) return min(fOriginBitrate, 2000.0);
	return max(fOriginBitrate, 4000.0);
}

#endif