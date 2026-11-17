#include "time_manager.hpp"
#include "Windows_api_CPU.hpp"

double creation_time_hprocess(DWORD PID)
{
    HANDLE curr_process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, PID);
    if (curr_process != NULL && curr_process != INVALID_HANDLE_VALUE) {
        FILETIME ftCreation, ftExit, ftKernel, ftUser;

        if (GetProcessTimes(curr_process, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
            ULONGLONG startTime, currentTime;

            startTime = FileTimeToQuad(ftCreation);

            FILETIME ftCurrent;
            GetSystemTimeAsFileTime(&ftCurrent);
            currentTime = FileTimeToQuad(ftCurrent);


            double uptimeSeconds = static_cast<double>(currentTime - startTime) / 10000000.0;
            DWORD processId = GetProcessId(curr_process);

            //std::cout << "PID " << processId << " uptime: " << uptimeSeconds << " seconds.\n";
            return uptimeSeconds;
        }
    }
}
Clock second_to_Clock(double time_in_seconds)
{
    Clock result;
    int time_in_seconds_int = (int)time_in_seconds;
    result.hours = time_in_seconds_int / 3600;
    result.minutes = (time_in_seconds_int % 3600) / 60;
    result.seconds= time_in_seconds_int % 60;
    return result;
}
