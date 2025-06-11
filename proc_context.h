#pragma once

#include <windows.h>
#include "win_kernel_defs.h"

// Đọc thông tin PEB của tiến trình từ xa
bool read_remote_peb(HANDLE process_handle, PROCESS_BASIC_INFORMATION &process_info, OUT PEB &peb_data);

// Thiết lập tham số cho tiến trình mới
bool configure_process_parameters(HANDLE process_handle, PROCESS_BASIC_INFORMATION &process_info, LPWSTR target_path);