/* (c) 2016, abc@telekom.ru */
/* License: GPL */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>

#include <vmGuestLib.h>

#define MAX_MESSAGE 2048

static void log_message(int level, const char *fmt, ...)
{
    char buf[MAX_MESSAGE];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    printf("%s\n", buf);
    syslog(level, "%s", buf);
}

volatile sig_atomic_t _running = 1;

void sigterm_handler(int arg)
{
    _running = 0;
}

static int output_stat(VMGuestLibHandle glHandle)
{
    VMGuestLibError glError;
    int exitStatus = 0;
    static VMSessionId sid = 0;
    static uint64 old_elapsedMs;
    static uint64 old_cpuStolenMs;
    static uint64 old_cpuUsedMs;

    glError = VMGuestLib_UpdateInfo(glHandle);
    if (glError) {
	if (glError == VMGUESTLIB_ERROR_MEMORY)
	    return 0;
	log_message(LOG_ERR, VMGuestLib_GetErrorText(glError));
	return -1; /* permanent error */
    }

    VMSessionId new_sid;
    glError = VMGuestLib_GetSessionId(glHandle, &new_sid);
    if (glError)
	return 0;

    uint32 mhz;
    glError = VMGuestLib_GetHostProcessorSpeed(glHandle, &mhz);

    if (new_sid != sid) {
	if (sid)
	    log_message(LOG_INFO, "Session id is changed, reset stat.");

	uint32 cpuReservationMHz;
	glError = VMGuestLib_GetCpuReservationMHz(glHandle, &cpuReservationMHz);

	uint32 cpuLimitMHz;
	glError = VMGuestLib_GetCpuLimitMHz(glHandle, &cpuLimitMHz);

	uint32 cpuShares;
	glError = VMGuestLib_GetCpuShares(glHandle, &cpuShares);

	log_message(LOG_INFO, "Host freq %d MHz, Reserved %d MHz, Limit %d MHz, CPU shares %d",
	    mhz, cpuReservationMHz, cpuLimitMHz, cpuShares);

	sid             = new_sid;
	old_elapsedMs   = 0;
	old_cpuStolenMs = 0;
	old_cpuUsedMs   = 0;
    }

    uint64 elapsedMs;
    glError = VMGuestLib_GetElapsedMs(glHandle, &elapsedMs);

    uint64 cpuStolenMs;
    glError = VMGuestLib_GetCpuStolenMs(glHandle, &cpuStolenMs);

    uint64 cpuUsedMs;
    glError = VMGuestLib_GetCpuUsedMs(glHandle, &cpuUsedMs);

    if (elapsedMs == old_elapsedMs)
	return 0;

    double usedCpu      = (cpuUsedMs   - old_cpuUsedMs)   * 100.0 / (elapsedMs - old_elapsedMs);
    double stolenCpu    = (cpuStolenMs - old_cpuStolenMs) * 100.0 / (elapsedMs - old_elapsedMs);
    double effectiveMhz = (cpuUsedMs   - old_cpuUsedMs)   * mhz   / (elapsedMs - old_elapsedMs);
    log_message(LOG_INFO, "CPU used %3.2f%%, stolen %3.2f%%, effective freq %3.0f MHz",
       	usedCpu, stolenCpu, effectiveMhz);

    old_elapsedMs   = elapsedMs;
    old_cpuStolenMs = cpuStolenMs;
    old_cpuUsedMs   = cpuUsedMs;

    return 0;
}

int main(int argc, char **argv)
{
    VMGuestLibHandle glHandle;
    VMGuestLibError glError;

    openlog("vmguest-stat", LOG_PID, LOG_DAEMON);

    glError = VMGuestLib_OpenHandle(&glHandle);
    if (glError) {
	log_message(LOG_ERR, VMGuestLib_GetErrorText(glError));
	exit(1);
    }

    signal(SIGTERM, sigterm_handler);
    
    while (_running) {
	if (output_stat(glHandle))
	    break;
	sleep(10);
    }

    printf("error\n");
    VMGuestLib_CloseHandle(glHandle);
    closelog();
    return 0;
}
