#ifndef __TRANSNODE_UTILS_H__
#define __TRANSNODE_UTILS_H__
#include <map>
#include <string>

#include "vfhead.h"
#include "bit_osdep.h"

class CVideoFilter;
//class CPlaylistGenerator;

class CTransnodeUtils
{
public:
	static void Initialize();
	static void UnInitalize();

	// set/get single core
	static void SetUseSingleCore(bool bUseSingleCore);
	static bool GetUseSingleCore() { return m_bUseSingleCore; }

	// vf stuff
	static bool InitVideoFilter(const char* vfdllPath);
	static bool InitVideoFilter();
	static CVideoFilter* CreateVideoFilter();
	static void DeleteVideoFilter(CVideoFilter* pFilter);

	// Play list generate
	//static CPlaylistGenerator* CreatePlaylist(const char *key, const char* listName);
	//static CPlaylistGenerator* GetPlaylist(const std::string &key);
	//static bool RemovePlaylistGenerator(const std::string &key);

private:

	static bool m_Initalized;
	static bool m_bUseSingleCore;

	// vf stuff
	static HMODULE m_hDllVf;
	static PFN_vfNew m_pfnNewVf;
	static PFN_vfDelete m_pfnDeleteVf;

};

#endif //__TRANSNODE_UTILS_H__
