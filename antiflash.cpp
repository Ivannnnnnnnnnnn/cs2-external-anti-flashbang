#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <thread>

DWORD GetProcId(const wchar_t* procName)
{
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 procEntry{};
        procEntry.dwSize = sizeof(procEntry);
        if (Process32First(hSnap, &procEntry))
        {
            do
            {
                if (!_wcsicmp(procEntry.szExeFile, procName))
                {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

uintptr_t GetModuleBase(DWORD procId, const wchar_t* modName)
{
    uintptr_t modBase = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 modEntry{};
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry))
        {
            do
            {
                if (!_wcsicmp(modEntry.szModule, modName))
                {
                    modBase = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBase;
}

int main()
{
    DWORD procId = GetProcId(L"cs2.exe");
    if (!procId)
    {
        std::cout << "CS2 not found!\n";
        return 1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
    if (!hProcess)
    {
        std::cout << "Failed to open process.\n";
        return 1;
    }

    uintptr_t clientBase = GetModuleBase(procId, L"client.dll");
    if (!clientBase)
    {
        std::cout << "Failed to find client.dll\n";
        return 1;
    }

    const DWORD dwLocalPlayerPawn = 0x184C0D0;
    const DWORD m_flFlashBangTime = 0x13F8;

    while (true)
    {
        uintptr_t localPawn = 0;
        ReadProcessMemory(hProcess, (LPCVOID)(clientBase + dwLocalPlayerPawn), &localPawn, sizeof(localPawn), nullptr);

        if (localPawn)
        {
            float flashDuration = 0;
            ReadProcessMemory(hProcess, (LPCVOID)(localPawn + m_flFlashBangTime), &flashDuration, sizeof(flashDuration), nullptr);

            if (flashDuration > 0.0f)
            {
                float zero = 0.0f;
                WriteProcessMemory(hProcess, (LPVOID)(localPawn + m_flFlashBangTime), &zero, sizeof(zero), nullptr);
                std::cout << "[Anti-Flash] Cleared flash duration\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    CloseHandle(hProcess);
    return 0;
}
