#include "NodeApi.h"
#include <string>
#include "StrProInc.h"
#include "MEvaluater.h"
#include "version.h"
#include "WorkManager.h"
#include "TransnodeUtils.h"
#include "adapter.h"

using namespace std;
using namespace StrPro;

const char* TN_GetVersion()
{
	return TS_VERSION;
}

bool TN_Initialize(int maxWorkerCount)
{
	return CWorkManager::GetInstance()->Initialize(maxWorkerCount);
}

void TN_SetState(node_state_t state)
{
	CAdapter::GetInstance()->SetState(state);
}

// int TN_GetCapability()
// {
// 	return CWorkManager::GetInstance()->GetCapability();
// }

ts_node_info_t TN_GetInfo()
{
	return CWorkManager::GetInstance()->GetInfo();
}

bool TN_Stop()
{
	bool ret = CWorkManager::GetInstance()->Stop();
	Sleep(500);
	CAdapter::GetInstance()->SetState(STATE_DISCONNECTED);
	return ret;
}

int TN_AddTask(int globalId, const char *prefs,  char* outFileName/*OUT*/, worker_type_t worker_type)
{
	CWorkManager* pMan = CWorkManager::GetInstance();	
	int id = 0;
	if (prefs == NULL || *prefs == '\0') return AJE_INVALID_PREF;
	if (pMan->CreateWorker(&id, worker_type) == NULL) return AJE_CREATE_WORKER_FAIL;
	return pMan->CreateWorkerTask(id, globalId, prefs, outFileName);
}

int TN_RemoveTask(int globalId)
{
	CWorkManager* pMan = CWorkManager::GetInstance();
	int id = pMan->GlobalIdToLocalId(globalId);

	if (id <= 0) return -1; //invalid task id

	if (!pMan->CancelWorker(id)) return -2; //failed to cancel

	return 0;
}

int TN_StopTask(int globalId) 
{
	CWorkManager* pMan = CWorkManager::GetInstance();
	int id = pMan->GlobalIdToLocalId(globalId);

	if (id <= 0) return -1;					//invalid task id

	if (!pMan->StopWorker(id)) return -2; //failed to stop	

	return 0;
}
int TN_PauseTask(int globalId)
{
	CWorkManager* pMan = CWorkManager::GetInstance();
	int id = pMan->GlobalIdToLocalId(globalId);

	if (id <= 0) return -1; //invalid task id

	if (!pMan->PauseWorker(id)) return -2; //failed to stop

	return 0;
}

int TN_ResumeTask(int globalId)
{
	CWorkManager* pMan = CWorkManager::GetInstance();
	int id = pMan->GlobalIdToLocalId(globalId);
	if (id <= 0) return -1; //invalid task id

	if (!pMan->ResumeWorker(id)) return -2; //failed to stop

	return 0;
}

int TN_QueryTasks(std::vector<ts_task_info_t>& taskInfos)
{
	CWorkManager* pMan = CWorkManager::GetInstance();
	if (!pMan->QueryTaskInfos(taskInfos)) return -2; //failed
	return 0;
}

int TN_GPUPayload()
{
	CWorkManager* pMan = CWorkManager::GetInstance();
	bool useCUDA = pMan->IsUsingCUDA();
	return (useCUDA ? 1 : 0);
}

bool TN_GPUEnable()
{
	CWorkManager* pMan = CWorkManager::GetInstance();
	return pMan->IsCUDAEnable();
}

// bool TN_SetActiveWorkerNum(int number)
// {
// 	CWorkManager* pMan = CWorkManager::GetInstance();
// 	pMan->SetActiveWorkerNum(number);
// 	return true;
// }

bool TN_InitVideoFilter()
{
	return CTransnodeUtils::InitVideoFilter();
}

bool TN_SetUseSingleCore(bool bUseSingleCore)
{
	CTransnodeUtils::SetUseSingleCore(bUseSingleCore);
	return true;
}

bool TN_SetIgnoreErrorCode(int ignoreErrIdx)
{
	CWorkManager* pMan = CWorkManager::GetInstance();
	pMan->SetIgnoreErrorCode(ignoreErrIdx);
	return true;
}

bool TN_GetRunningTasksIdString(char* idString)
{
	CWorkManager* pMan = CWorkManager::GetInstance();
	return pMan->GetTasksIdString(idString);
}