#pragma once

#include <Windows.h>

// Lấy kiến trúc của tệp PE (32-bit or 64-bit)
WORD get_binary_architecture(const BYTE *pe_data);

// Lấy địa chỉ điểm vào tương đối của tệp PE
DWORD get_entry_point_offset(const BYTE *pe_data);