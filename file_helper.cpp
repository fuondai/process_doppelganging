#include "file_helper.h"
#include <iostream>

// Hàm đọc nội dung tệp vào bộ nhớ để xử lý
BYTE *load_file_to_memory(wchar_t *filepath, OUT size_t &output_size)
{
    HANDLE file_handle = CreateFileW(filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file_handle == INVALID_HANDLE_VALUE)
    {
#ifdef _DEBUG
        std::cerr << "Failed to open file!" << std::endl;
#endif
        return nullptr;
    }
    HANDLE file_mapping = CreateFileMapping(file_handle, 0, PAGE_READONLY, 0, 0, 0);
    if (!file_mapping)
    {
#ifdef _DEBUG
        std::cerr << "Failed to create file mapping!" << std::endl;
#endif
        CloseHandle(file_handle);
        return nullptr;
    }
    BYTE *file_data = (BYTE *)MapViewOfFile(file_mapping, FILE_MAP_READ, 0, 0, 0);
    if (file_data == nullptr)
    {
#ifdef _DEBUG
        std::cerr << "Failed to map view of file" << std::endl;
#endif
        CloseHandle(file_mapping);
        CloseHandle(file_handle);
        return nullptr;
    }
    output_size = GetFileSize(file_handle, 0);
    BYTE *memory_copy = (BYTE *)VirtualAlloc(NULL, output_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (memory_copy == NULL)
    {
        std::cerr << "Failed to allocate memory in the current process" << std::endl;
        return nullptr;
    }
    memcpy(memory_copy, file_data, output_size);
    UnmapViewOfFile(file_data);
    CloseHandle(file_mapping);
    CloseHandle(file_handle);
    return memory_copy;
}

// Giải phóng vùng nhớ đã cấp phát
void release_memory(BYTE *memory_block, size_t block_size)
{
    if (memory_block == NULL)
        return;
    VirtualFree(memory_block, block_size, MEM_DECOMMIT);
}

// Trích xuất tên tệp từ đường dẫn đầy đủ
wchar_t *extract_filename(wchar_t *full_path)
{
    size_t path_length = wcslen(full_path);
    for (size_t i = path_length - 2; i >= 0; i--)
    {
        if (full_path[i] == '\\' || full_path[i] == '/')
        {
            return full_path + (i + 1);
        }
    }
    return full_path;
}

// Trích xuất phần thư mục từ đường dẫn đầy đủ
wchar_t *extract_directory(IN wchar_t *full_path, OUT wchar_t *output_buffer, IN const size_t buffer_size)
{
    memset(output_buffer, 0, buffer_size);
    memcpy(output_buffer, full_path, buffer_size);

    wchar_t *filename_ptr = extract_filename(output_buffer);
    if (filename_ptr != nullptr)
    {
        *filename_ptr = '\0'; // Cắt phần tên tệp
    }
    return output_buffer;
}

// Lấy đường dẫn đến ứng dụng máy tính (calculator)
bool get_calculator_path(LPWSTR output_path, DWORD path_size, bool is_target_32bit)
{
    if (is_target_32bit)
    {
#ifdef _WIN64
        ExpandEnvironmentStringsW(L"%SystemRoot%\\SysWoW64\\calc.exe", output_path, path_size);
#else
        ExpandEnvironmentStringsW(L"%SystemRoot%\\system32\\calc.exe", output_path, path_size);
#endif
    }
    else
    {
        ExpandEnvironmentStringsW(L"%SystemRoot%\\system32\\calc.exe", output_path, path_size);
    }
    return true;
}