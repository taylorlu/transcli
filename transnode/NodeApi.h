#ifndef __NODE_API_H__
#define __NODE_API_H__

#include <vector>
#include "tsdef.h"

const char* TN_GetVersion();

bool TN_Initialize(int maxWorkerCount);

void TN_SetState(node_state_t state);

//int TN_GetCapability();

ts_node_info_t TN_GetInfo();

bool TN_Stop();

int TN_AddTask(int globalId, const char *prefs, char* outFileName, worker_type_t worker_type = WORKER_TYPE_DEFAULT);

int TN_RemoveTask(int globalId);

int TN_PauseTask(int globalId);

int TN_ResumeTask(int globalId);

int TN_StopTask(int globalId);

int TN_QueryTasks(std::vector<ts_task_info_t>& taskInfos);

int TN_GPUPayload();

bool TN_GPUEnable();

//bool TN_SetActiveWorkerNum(int number);

bool TN_InitVideoFilter();
bool TN_SetUseSingleCore(bool bUseSingleCore);
bool TN_SetIgnoreErrorCode(int ignoreErrIdx);
bool TN_GetRunningTasksIdString(char* idString);

#endif
