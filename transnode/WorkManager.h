#ifndef WORK_MANAGER_H
#define WORK_MANAGER_H

#include <map>

#include "BiThread.h"
#include "TransWorker.h"

class CXMLPref;
class CTransWorker;

typedef enum {
	THREAD_STATE_IDLE = 0,
	THREAD_STATE_RUNNING,
	THREAD_STATE_EXITING,
	THREAD_STATE_ERROR,
} thread_state_t;

class CWorkManager
{
public:
	static CWorkManager* GetInstance();
	bool Initialize(int maxWorkerCount = 30);
	
	// if success return worker id else return 0 (id starts from 1)
	CTransWorker* CreateWorker(int* pId, worker_type_t worker_type);
	int CreateWorkerTask(int id, int globalId, const char *prefs,  char* outFileName);

	void SetIgnoreErrorCode(int ignoreErrIdx){m_ignoreErrIdx = ignoreErrIdx;}
	int  GetIgnoreErrorCode() {return m_ignoreErrIdx;}

	//CTransWorker*  GetWorkerByLocalId(int id);		//local Id
	//CTransWorker*  GetWorkerByGlobalId(int uuid );  //Global Id
	
	//void WorkerCompleteReport(int id, int retCode);		// Before a worker thread exit, notify manager
	//void DestroyWorker(int id);	

	bool CancelWorker(int id);
	bool StopWorker(int id);	
	bool PauseWorker(int id);
	bool ResumeWorker(int id);
	bool QueryTaskInfos(std::vector<ts_task_info_t>& taskInfos);

	error_code_t GetErrorCode(int workerId);
	void SetErrorCode(error_code_t errCode, int workerId);

	bool Stop();
	void CleanUp();

	int LocalIdToGlobalId(int id);
	int GlobalIdToLocalId(int globalId);
	bool CheckId(int id);
	bool CheckGlobalId(int globalId) { return GlobalIdToLocalId(globalId) > 0; }
	ts_node_info_t GetInfo();
	bool IsUsingCUDA();
	bool IsCUDAEnable(){return m_bIsCudaEnable;}
	bool GetTasksIdString(char* tasksIdStr);
	int  GetCPUCoreNum() {return m_cpuCoreNum;}
	~CWorkManager(void);
private:
	CWorkManager(void);

	CTransWorker**			m_workerSet;
	int						m_maxCount;
	bool					m_bIsCudaEnable;
	bool					m_bDecodeMultiAudio;
	//ts_node_info_t			m_nodeInfo;
	//PFNCOMPLETEDCALLBACK	m_pfnCompleted;
	worker_type_t			m_defaultWorkerType;
	int						m_cpuCoreNum;
	int						m_ignoreErrIdx;
};

#endif
