#pragma once

#include <Windows.h>

// Hàm đọc tệp vào bộ nhớ
BYTE *load_file_to_memory(wchar_t *filepath, OUT size_t &output_size);
// Hàm giải phóng bộ nhớ đã cấp phát
void release_memory(BYTE *memory_block, size_t block_size);

// Lấy tên tệp từ đường dẫn đầy đủ
wchar_t *extract_filename(wchar_t *full_path);

// Lấy thư mục từ đường dẫn đầy đủ
wchar_t *extract_directory(IN wchar_t *full_path, OUT wchar_t *output_buffer, IN const size_t buffer_size);

// Lấy đường dẫn đến ứng dụng máy tính
bool get_calculator_path(LPWSTR output_path, DWORD path_size, bool is_target_32bit);