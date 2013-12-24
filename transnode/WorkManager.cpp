#include "MEvaluater.h"
#include "WorkManager.h"
#include "logger.h"
#include "util.h"
#include "TransWorkerSeperate.h"
//#include "TransWorkerUnify.h"
#include "TransnodeUtils.h"
#include <time.h>
//const char* PrefSavePath = "pref.xml";

CWorkManager::CWorkManager(void):m_workerSet(NULL),m_maxCount(1000),
								m_bIsCudaEnable(false), m_bDecodeMultiAudio(false)
{
#ifdef ENABLE_CUDA
	m_bIsCudaEnable = true;
#else
	m_bIsCudaEnable = false;
#endif
}

CWorkManager::~CWorkManager(void)
{
	CleanUp();
}

CWorkManager* CWorkManager::GetInstance()
{
	static CWorkManager aManager;
	return &aManager;
}

bool CWorkManager::Initialize(int maxWorkerCount /* = 300 */)
{
	CTransnodeUtils::Initialize();
	//Init Cuda vf
	//CTransnodeUtils::InitVideoFilter();

	m_cpuCoreNum = GetCpuCoreNum();

	// Set Current Directory
#ifndef ENABLE_SINGLE_MODE
	char curDir[MAX_PATH] = {0};
	if (GetAppDir(curDir, MAX_PATH) > 0) SetCurrentDirectory(curDir);
#endif
	
	m_maxCount = maxWorkerCount;
	m_workerSet = new CTransWorker*[m_maxCount+1];
	if(!m_workerSet) return false;

	for (int i=0; i<=m_maxCount; ++i) {
		m_workerSet[i] = NULL;
	}

	logger_info(LOGM_TS_WM, "Worker manager initialize successfully!\n");
	return true;
}

CTransWorker* CWorkManager::CreateWorker(int* pId, worker_type_t worker_type)
{
	int smallestId = 100000;
	bool bCreateWorker = false;
	for (int i = 1; i<=m_maxCount; ++i) {
		if (m_workerSet[i] == NULL) {
			if(smallestId > i) {
				smallestId = i;
				bCreateWorker = true;
			}
		}
		else if (m_workerSet[i]->GetState() == TASK_REUSABLE){
			if(worker_type == m_workerSet[i]->GetWorkType()) {
				m_workerSet[i]->CleanUp();
				if(smallestId > i) {
					smallestId = i;
					bCreateWorker = false;
					logger_info(LOGM_TS_WM, "worker %d is reused.\n", i);
				}
			} else {
				delete m_workerSet[i];
				m_workerSet[i] = NULL;
				logger_info(LOGM_TS_WM, "worker %d is released.\n", i);
				if(smallestId > i) {
					smallestId = i;
					bCreateWorker = true;
				}
			}
		}
	}

	if(smallestId <= m_maxCount) {
		if(bCreateWorker) {
			switch (worker_type) {
			//case WORKER_TYPE_UNIFY:
			//	m_workerSet[smallestId] = new CTransWorkerUnify;
			//	break;
			//case WORKER_TYPE_REMOTE:
			//	m_workerSet[smallestId] = new CTransWorkerRemote;
			//	break;
			case WORKER_TYPE_SEP:
			default:
				m_workerSet[smallestId] = new CTransWorkerSeperate;
				break;
			}
		}
		
		*pId = smallestId;
		m_workerSet[smallestId]->SetId(smallestId);
		m_workerSet[smallestId]->SetWorkType(worker_type);
		return m_workerSet[smallestId];
	}
	
	logger_info(LOGM_TS_WM, "Create trancode worker failed!");
	return NULL;
}

/*if success, return the id otherwise return 0*/
int CWorkManager::CreateWorkerTask(int id, int globalId, const char *prefs,  char* outFileName)
{
	if(!CheckId(id) || !m_workerSet) return 0;
	//decreaseCapacity();
	CTransWorker* aWorker = m_workerSet[id];
	aWorker->SetGlobalId(globalId);
	int ret = aWorker->CreateTask(prefs, outFileName);
	if(ret != 0) {	//Error
		logger_err(LOGM_TS_WM, "Worker %d start failed!\n", id);
	}
	return ret;
}

/*
CTransWorker* CWorkManager::GetWorkerByLocalId(int id)
{
	if(CheckId(id)) {
		return m_workerSet[id];
	} 

	return NULL;
}

CTransWorker*  CWorkManager::GetWorkerByGlobalId(int uuid)
{
	int id = GlobalIdToLocalId(uuid);
	if (id > 0) return m_workerSet[id];
	return NULL;
}

//only cleanup the worker, the worker object will be reused.
void CWorkManager::DestroyWorker(int id) 
{
	if(CheckId(id) && m_workerSet[id] != NULL) {
		m_workerSet[id]->CleanUp();
	}
}*/

bool CWorkManager::CancelWorker(int id)
{
	if(CheckId(id) && m_workerSet[id] != NULL) {
		return m_workerSet[id]->Cancel();
	}

	return false;
}
bool CWorkManager::StopWorker(int id) 
{
	if(CheckId(id) && m_workerSet[id] != NULL) {
		bool success =  m_workerSet[id]->Stop();
		//m_workerSet[id]->TerminateAllCLIs(true);
		return success;
	}

	return false;
}

bool CWorkManager::PauseWorker(int id) 
{
	if(CheckId(id) && m_workerSet[id] != NULL) {
		return m_workerSet[id]->Pause();
	}
	return false;
}

bool CWorkManager::ResumeWorker(int id) 
{
	if(CheckId(id) && m_workerSet[id] != NULL) {
		return m_workerSet[id]->Resume();
	}
	return false;
}

bool CWorkManager::QueryTaskInfos(std::vector<ts_task_info_t>& taskInfos)
{
	for (int i = 1; i <= m_maxCount; ++i) {
		task_state_t tkState = TASK_NOTSET;
		if(m_workerSet[i]) {
			tkState = m_workerSet[i]->GetState();
		}
		if(tkState >= TASK_ENCODING && tkState != TASK_REUSABLE) {
			ts_task_info_t tkInfo;
			if(m_workerSet[i]->GetTaskInfo(&tkInfo)) {
				taskInfos.push_back(tkInfo);
			}
		}
	}
	if(taskInfos.size() > 0) return true;
	return false;
}

void CWorkManager::CleanUp()
{
	if(m_workerSet) {
		for (int i=1; i<=m_maxCount; ++i) {
			delete m_workerSet[i];
			m_workerSet[i] = NULL;
		}
		delete[] m_workerSet;
		m_workerSet = NULL;
	}

	CTransnodeUtils::UnInitalize();
	//StopWatchdog();
	
	logger_status(LOGM_TS_WM, "Cleanup finished\n");
}

int CWorkManager::LocalIdToGlobalId(int id)
{
	if (id >= 0 && 
		id <= m_maxCount &&
		m_workerSet != NULL &&
		m_workerSet[id] != NULL) return m_workerSet[id]->GetGlobalId();

	return 0;
}

int CWorkManager::GlobalIdToLocalId(int globalId)
{
	if(globalId > 0 && m_workerSet) {
		for (int i = 1; i <= m_maxCount; ++i) {
			if(m_workerSet[i] && m_workerSet[i]->GetGlobalId() == globalId) {
				return i;
			}
		}
	}

	return 0;
}

bool CWorkManager::CheckId(int id)
{
	if(id>=1 && id<=m_maxCount) {
		return true;
	} else {
		logger_err(LOGM_TS_WM, "Invalid worker id: %d, id range(1<=id<=%d), please check again!\n", id, m_maxCount);
		return false;
	}
}

bool CWorkManager::Stop()
{
	bool ret = true;
	if(!m_workerSet) {
		for (int i=1; i<=m_maxCount; ++i) {
			if(m_workerSet[i]) {
				m_workerSet[i]->SetState(TASK_ERROR);
				ret = ret && m_workerSet[i]->Stop();
			}
		}
	}
	return ret;
}

error_code_t CWorkManager::GetErrorCode(int workerId)
{
	if(m_workerSet && CheckId(workerId)) {
		return m_workerSet[workerId]->GetErrorCode();
	}
	return EC_NO_ERROR;
}

void CWorkManager::SetErrorCode(error_code_t errCode, int workerId) 
{
	if(m_workerSet && CheckId(workerId)) {
		CTransWorker* pWorker = m_workerSet[workerId];
		if(pWorker) {
			pWorker->SetErrorCode(errCode);
		}
	}
}

/*
bool CWorkManager::IsUsingCUDA()
{
	if(m_workerSet) {
		for (int i = 1; i <= m_maxCount; ++i) {
			if(m_workerSet[i]) {
				bool useCUDA = m_workerSet[i]->IsUsingCUDA();
				if(useCUDA) return true;
			}
		}
	}
	
	return false;
}*/

/*
ts_node_info_t CWorkManager::GetInfo()
{
	m_nodeInfo.cpu_usage = m_sysPerf.GetCpuUsage();
	m_nodeInfo.phymem_usage = m_sysPerf.GetPhyMemUsage();
	m_nodeInfo.run_time = clock()/CLOCKS_PER_SEC;
	return m_nodeInfo;
}*/

//void CWorkManager::WorkerCompleteReport(int id, int retCode)
//{
	//increaseCapacity();
//	if (retCode == 0) {
//		m_nodeInfo.completed_task_num++;
//	} else {
//		m_nodeInfo.error_task_num++;
		//m_nodeInfo.current_task_num--;		// Newly add (2009-1012)
//	}
	
//	if (m_pfnCompleted) m_pfnCompleted(id, retCode);
//}


// void CWorkManager::increaseCapacity()
// {
// 	m_curWorkingCount--;
// 	if(m_curWorkingCount < m_activeCount) {
// 		m_capacity = MAX_CAPABILITY;
// 	} else {
// 		m_capacity = 0;
// 	}
// }
// 
// void CWorkManager::decreaseCapacity()
// {
// 	m_curWorkingCount++;
// 	if(m_curWorkingCount >= m_activeCount) {
// 		m_capacity = 0;
// 	} else {
// 		m_capacity = MAX_CAPABILITY;
// 	}
// }

// bool CWorkManager::StartWatchdog()
// {
// 	if (m_watchdogState == THREAD_STATE_RUNNING) {
// 		logger_warn(LOGM_TS_WM, "Watchdog is already running.\n");
// 		return false;
// 	}
// 
// 	int ret = CBIThread::Create(m_hWatchdog, (CBIThread::ThreadFunc)CWorkManager::WatchdogThreadEntry, this);
// 
// 	if (ret != 0 || !m_hWatchdog) {
// 		logger_warn(LOGM_TS_WM, "Failed to start watchdog.\n");
// 	}
// 
// 	return ret != 0;
// }
// 
// void CWorkManager::StopWatchdog()
// {
// 	if (m_watchdogState == THREAD_STATE_RUNNING) {
// 		m_watchdogState = THREAD_STATE_EXITING;
// 		if (m_hWatchdog) CBIThread::Join(m_hWatchdog);
// 	}
// 
// 	m_hWatchdog = 0;
// }
// 
// #define WORKER_INACTIVE_DUR 60000*10		// 10 Minutes
// unsigned long CWorkManager::WatchdogThread()
// {
// 	m_watchdogState = THREAD_STATE_RUNNING;
// 
// 	while (m_watchdogState == THREAD_STATE_RUNNING) {
// 		if(!m_workerSet) break;
// 
// 		for (int i = 1; i <= m_maxCount; ++i) {
// 			if(!m_workerSet[i]) continue;
// 			if(m_workerSet[i]->GetLastActiveTime() == 0) continue;
// 			__int64 diff = GetTickCount() - m_workerSet[i]->GetLastActiveTime();
// 			// Only kill the worker that hang when encoding
// 			if (diff > WORKER_INACTIVE_DUR && m_workerSet[i]->GetState() < TASK_DONE) { 
// 				logger_warn(LOGM_TS_WM, "The worker %d is unactive for about 10 minute, kill it!\n", i);
// 				delete m_workerSet[i];
// 				m_workerSet[i] = NULL;
// 			}
// 
// 			msleep(200);
// 		}
// 
// 		for (int i = 0; i < 5 &&
// 			m_watchdogState == THREAD_STATE_RUNNING; i++) msleep(500); //sleep 2.5 second
// 	}
// 
// 	m_watchdogState = THREAD_STATE_IDLE;
// 	return 0;
// }

bool CWorkManager::GetTasksIdString(char* tasksIdStr)
{
	if(m_workerSet) {
		std::string tempStr;
		for (int i = 1; i <= m_maxCount; ++i) {
			if(m_workerSet[i] && m_workerSet[i]->GetState() >= TASK_ENCODING && 
			 m_workerSet[i]->GetState() <= TASK_DONE) {
				 char idstr[32] = {0};
				 sprintf(idstr, "%d,", m_workerSet[i]->GetGlobalId());
				 tempStr += idstr;
			}
		}
		if(!tempStr.empty()) {
			tempStr.pop_back();
			strcpy(tasksIdStr, tempStr.c_str());
			return true;
		}
	}
	return false;
}
//CXMLPref* CWorkManager::CreateXMLPref()
//{
//	if(!m_gobalPrefs) return NULL;
//	CXMLPref* pref = m_gobalPrefs->Clone();
//	m_xmlPrefs.push_back(pref);
//	return pref;
//}
