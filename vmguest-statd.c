/* Periodically log VMware CPU stats to syslog.
 *
 * (c) 2017, abc@telekom.ru
 *
 * Based on Python implementation of vmguest-stat by Dag Wieers.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>

#include <vmGuestLib.h>

#define MAX_MESSAGE 2048

static int _isatty = 0;
static volatile sig_atomic_t _running = 1;

static void log_message(int level, const char *fmt, ...)
{
    char buf[MAX_MESSAGE];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (_isatty)
	printf("%s\n", buf);
    else
	syslog(level, "%s", buf);
}

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

    uint32 cpuReservationMHz;
    glError = VMGuestLib_GetCpuReservationMHz(glHandle, &cpuReservationMHz);

    uint32 cpuLimitMHz;
    glError = VMGuestLib_GetCpuLimitMHz(glHandle, &cpuLimitMHz);

    uint32 cpuShares;
    glError = VMGuestLib_GetCpuShares(glHandle, &cpuShares);

    if (new_sid != sid) {
	if (sid)
	    log_message(LOG_INFO, "Session id is changed, reset stat.");

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

#define HOUR_MS (1000 * 60 * 60)
    if (!old_elapsedMs || (old_elapsedMs / HOUR_MS) != (elapsedMs / HOUR_MS))
	log_message(LOG_INFO, "Host freq %d MHz, Reserved %d MHz, Limit %d MHz, CPU shares %d",
	    mhz, cpuReservationMHz, cpuLimitMHz, cpuShares);

    if (elapsedMs == old_elapsedMs)
	return 0;

    double usedCpu      = (cpuUsedMs   - old_cpuUsedMs)   * 100.0 / (elapsedMs - old_elapsedMs);
    double stolenCpu    = (cpuStolenMs - old_cpuStolenMs) * 100.0 / (elapsedMs - old_elapsedMs);
    double effectiveMhz = (cpuUsedMs   - old_cpuUsedMs)   * mhz   / (elapsedMs - old_elapsedMs);
    char oflimit[32] = { 0 };
    if (mhz) {
	double effectiveCpu = effectiveMhz * 100.0 / mhz;
	snprintf(oflimit, sizeof(oflimit), " (%.0f%% of limit)", effectiveCpu);
    }
    log_message(LOG_INFO, "CPU used %5.2f%%, stolen %5.2f%%, effective freq %4.0f MHz%s",
	usedCpu, stolenCpu, effectiveMhz, oflimit);

    old_elapsedMs   = elapsedMs;
    old_cpuStolenMs = cpuStolenMs;
    old_cpuUsedMs   = cpuUsedMs;

    return 0;
}

int main(int argc, char **argv)
{
    VMGuestLibHandle glHandle;
    VMGuestLibError glError;

    _isatty = isatty(1);
    if (!_isatty) {
	daemon(0, 0);
    }

    openlog("vmguest-stat", 0, LOG_DAEMON);

    glError = VMGuestLib_OpenHandle(&glHandle);
    if (glError) {
	log_message(LOG_ERR, VMGuestLib_GetErrorText(glError));
	exit(1);
    }

    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);
    
    while (_running) {
	if (output_stat(glHandle))
	    break;
	sleep(_isatty? 10 : 60);
    }

    VMGuestLib_CloseHandle(glHandle);
    closelog();
    return 0;
}
