#define WIN32_LEAN_AND_MEAN
//#include "Windows_api_RAM.hpp"
#include "Windows_api_CPU.hpp"
using namespace std;

 ULONGLONG FileTimeToQuad(const FILETIME& ft) {
    return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime; //bit manip smart storing time 
}
bool Get_system_CPU_info(CpuSample& sample)
{
    return GetSystemTimes(&sample.idle, &sample.kernel, &sample.user) != 0;
}

 bool Get_process_CPU_info(HANDLE curr_process, CpuSample& sample)
{


    FILETIME ftCreation, ftExit, ftKernel, ftUser, ftSystem; //measeuring time of the processes (creationtime,exittime, kernel time,user time,ftKernel + ftUser)

    //First measuerment
    GetSystemTimeAsFileTime(&ftSystem);
    if (!GetProcessTimes(curr_process, &ftCreation, &ftExit, &ftKernel, &ftUser)) {
        return false;
    }

    sample.kernel = ftKernel;
    sample.user = ftUser;
    return true;
}
 double Compute_Cpu_Usage(const CpuSample& s1, const CpuSample& s2,
    const FILETIME& sysBefore, const FILETIME& sysAfter)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD numProcessors = sysInfo.dwNumberOfProcessors;

    ULONGLONG k1 = FileTimeToQuad(s1.kernel);
    ULONGLONG k2 = FileTimeToQuad(s2.kernel);
    ULONGLONG u1 = FileTimeToQuad(s1.user);
    ULONGLONG u2 = FileTimeToQuad(s2.user);

    // stops underflow ( PID reused )
    if (k2 < k1 || u2 < u1) {
        cerr << "WARNING: time went back (k1=" << k1 << " k2=" << k2
            << " u1=" << u1 << " u2=" << u2 << ")" << endl;
        return 0.0;
    }

    ULONGLONG kernelTime = k2 - k1;
    ULONGLONG userTime = u2 - u1;
    ULONGLONG systemTime = FileTimeToQuad(sysAfter) - FileTimeToQuad(sysBefore);

    if (systemTime == 0) return 0.0;

    ULONGLONG totalProcTime = kernelTime + userTime;
    double result = (double)totalProcTime / (double)(systemTime * numProcessors) * 100.0;

    if (result > 100.0 * numProcessors) {
        //cerr << "WARNING: result over maximum?: " << result << endl;
        return 0.0;
    }

    return result;
}
 double Compute_System_Cpu_Usage(const CpuSample& s1, const CpuSample& s2)
{
    ULONGLONG idle1 = FileTimeToQuad(s1.idle);
    ULONGLONG kernel1 = FileTimeToQuad(s1.kernel);
    ULONGLONG user1 = FileTimeToQuad(s1.user);

    ULONGLONG idle2 = FileTimeToQuad(s2.idle);
    ULONGLONG kernel2 = FileTimeToQuad(s2.kernel);
    ULONGLONG user2 = FileTimeToQuad(s2.user);

    if (idle2 < idle1 || kernel2 < kernel1 || user2 < user1) {
        return 0.0;
    }

    ULONGLONG deltaIdle = idle2 - idle1;
    ULONGLONG deltaKernel = kernel2 - kernel1;
    ULONGLONG deltaUser = user2 - user1;

    ULONGLONG deltaTotal = deltaKernel + deltaUser; // kernel  idle
    if (deltaTotal == 0) return 0.0;

    return (1.0 - (double)deltaIdle / (double)deltaTotal) * 100.0;
}
