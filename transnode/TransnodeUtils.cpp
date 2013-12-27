#ifdef WIN32
#include <Windows.h>
#else
#include <sched.h>
#include <unistd.h>
#endif

#include "TransnodeUtils.h"
#include "logger.h"

bool CTransnodeUtils::m_Initalized = false;
bool CTransnodeUtils::m_bUseSingleCore = false;
HMODULE CTransnodeUtils::m_hDllVf = NULL;
PFN_vfNew CTransnodeUtils::m_pfnNewVf = NULL;
PFN_vfDelete CTransnodeUtils::m_pfnDeleteVf = NULL;
//std::map<std::string, CPlaylistGenerator*> g_playlistQueue;

void CTransnodeUtils::Initialize()
{
	if (!m_Initalized) {
		m_bUseSingleCore = false;
		m_hDllVf = NULL;
		m_pfnNewVf = NULL;
		m_pfnDeleteVf = NULL;
		//g_playlistQueue.clear();
		m_Initalized = true;
	}
}

void CTransnodeUtils::SetUseSingleCore(bool bUseSingleCore)
{
#if defined(WIN32)
	HANDLE hProcess = GetCurrentProcess();
	if(hProcess) {
		m_bUseSingleCore = bUseSingleCore;
		
		if(bUseSingleCore) {
			SetProcessAffinityMask(hProcess, 0x00000001);	// Bind to cpu 0
		} else {
			DWORD processAffinityMask, sysAffinityMask;
			GetProcessAffinityMask(hProcess, &processAffinityMask, &sysAffinityMask);
			SetProcessAffinityMask(hProcess, sysAffinityMask);			// Cancel binding to cpu 0(bind to all)
		}
	}
#elif defined(HAVE_LINUX)
    cpu_set_t mask;

    m_bUseSingleCore = bUseSingleCore; 
    if (bUseSingleCore) {
        CPU_ZERO(&mask);
        CPU_SET(0/*CPU 0*/, &mask);
    }
    else {
        int nr = sysconf(_SC_NPROCESSORS_CONF);
        CPU_ZERO(&mask);
        for (int i = 0; i < nr; i++) {
            CPU_SET(i, &mask);
        }
    }

    sched_setaffinity(0/*current process*/, sizeof(mask), &mask);
    sleep(0); //rescheduling
#else
    #warning "TODO:Set CPU Affinity"
#endif
}

// Video Filter functions
bool CTransnodeUtils::InitVideoFilter(const char* vfdllPath)
{
	if (vfdllPath == NULL || *vfdllPath == '\0') return false;

	if(m_hDllVf) return true;	// Prevent duplicate init
    m_hDllVf = LoadLibraryA(vfdllPath);

	if (m_hDllVf == NULL) {
		logger_err(LOGM_TS_WM, "Video Filter %s can no be loaded.\n", vfdllPath);
		return false;
	}

	m_pfnNewVf = (PFN_vfNew)GetProcAddress(m_hDllVf, "vfNew");
	m_pfnDeleteVf = (PFN_vfDelete)GetProcAddress(m_hDllVf, "vfDelete");
	if(m_pfnDeleteVf && m_pfnNewVf) {
		return true;
	}

	FreeLibrary(m_hDllVf);
	m_hDllVf = NULL;

	logger_err(LOGM_TS_WM, "InitVideoFilter failed.\n");
	return false;
}

bool CTransnodeUtils::InitVideoFilter()
{
	bool b_ret = false;

#ifdef WIN32
#ifdef ENABLE_CUDA
	b_ret = InitVideoFilter("vfcuda2.dll");
#else
	b_ret = InitVideoFilter("vfcpu.dll");
#endif
#else
	b_ret = InitVideoFilter("./libvfcuda2.so");
	if (!b_ret) {
		b_ret = InitVideoFilter("libvfcuda2.so");
	}
#endif

	return b_ret;
}

CVideoFilter* CTransnodeUtils::CreateVideoFilter()
{
	if(m_pfnNewVf)
		return m_pfnNewVf();
	return NULL;
}

void CTransnodeUtils::DeleteVideoFilter(CVideoFilter* pFilter)
{
	if(m_pfnDeleteVf)
		m_pfnDeleteVf(pFilter);
}

/*
// Play list generate
CPlaylistGenerator* CTransnodeUtils::CreatePlaylist(const char *key, const char* listName)
{
	if (key == NULL || *key == '\0' ||
		listName == NULL || *listName == '\0') return NULL;

	CPlaylistGenerator* playlist = GetPlaylist(key);
	if (playlist != NULL) {
		playlist->FlushPlaylist();
		return playlist;
	}
	playlist = new CPlaylistGenerator;
	if(!playlist) {
		logger_err(LOGM_TS_WM, "Create playlist failed.\n");
		return NULL;
	}
	playlist->SetPlaylistName(listName);
	g_playlistQueue[key] = playlist;

	return playlist;
}

CPlaylistGenerator* CTransnodeUtils::GetPlaylist(const std::string &key)
{
	std::map<std::string, CPlaylistGenerator*>::iterator it;

	it = g_playlistQueue.find(key);
	if (it == g_playlistQueue.end()) return NULL;

	return  it->second;
}

bool CTransnodeUtils::RemovePlaylistGenerator(const std::string &key)
{
	std::map<std::string, CPlaylistGenerator*>::iterator it;

	it = g_playlistQueue.find(key);
	if (it == g_playlistQueue.end()) return false;

	if (it->second) delete it->second;

	g_playlistQueue.erase(it);
	return  true;
}
*/
void CTransnodeUtils::UnInitalize()
{
	if (m_hDllVf) {
		logger_status(LOGM_TS_WM, "Uninit Video filter...\n");
		FreeLibrary(m_hDllVf);
		m_hDllVf = NULL;
	}
}
