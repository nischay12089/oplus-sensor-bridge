// libbinder_ndk service/process entry points that the public NDK headers omit. They are exported
// by the platform libbinder_ndk.so (LLNDK), so only the prototypes are needed.
#pragma once
#include <android/binder_ibinder.h>
#include <android/binder_status.h>
#include <stdint.h>

__BEGIN_DECLS
binder_status_t AServiceManager_addService(AIBinder* binder, const char* instance);
bool ABinderProcess_setThreadPoolMaxThreadCount(uint32_t numThreads);
void ABinderProcess_startThreadPool(void);
bool ABinderProcess_isThreadPoolStarted(void);
__END_DECLS
