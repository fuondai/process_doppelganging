#include "binary_parser.h"

// Hàm nội bộ để lấy thông tin header NT từ dữ liệu PE
BYTE *get_nt_headers(const BYTE *pe_data)
{
    if (pe_data == NULL)
        return NULL;

    // Truy cập DOS header
    IMAGE_DOS_HEADER *dos_header = (IMAGE_DOS_HEADER *)pe_data;
    if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
    {
        return NULL;
    }

    // Giới hạn offset tối đa cho header PE
    const LONG max_offset = 1024;
    LONG pe_offset = dos_header->e_lfanew;

    if (pe_offset > max_offset)
        return NULL;

    // Truy cập header NT
    IMAGE_NT_HEADERS32 *nt_header = (IMAGE_NT_HEADERS32 *)(pe_data + pe_offset);
    if (nt_header->Signature != IMAGE_NT_SIGNATURE)
    {
        return NULL;
    }
    return (BYTE *)nt_header;
}

// Xác định kiến trúc của tệp PE (32-bit/64-bit)
WORD get_binary_architecture(const BYTE *pe_data)
{
    void *header_ptr = get_nt_headers(pe_data);
    if (header_ptr == NULL)
        return 0;

    IMAGE_NT_HEADERS32 *nt_header = static_cast<IMAGE_NT_HEADERS32 *>(header_ptr);
    return nt_header->FileHeader.Machine;
}

// Lấy địa chỉ điểm vào tương đối (RVA) của tệp thực thi
DWORD get_entry_point_offset(const BYTE *pe_data)
{
    WORD architecture = get_binary_architecture(pe_data);
    BYTE *nt_header_ptr = get_nt_headers(pe_data);
    if (nt_header_ptr == NULL)
    {
        return 0;
    }

    DWORD entry_point_addr = 0;

    // Xử lý khác nhau cho 32-bit và 64-bit
    if (architecture == IMAGE_FILE_MACHINE_AMD64)
    {
        // 64-bit PE format
        IMAGE_NT_HEADERS64 *nt_header64 = (IMAGE_NT_HEADERS64 *)nt_header_ptr;
        entry_point_addr = nt_header64->OptionalHeader.AddressOfEntryPoint;
    }
    else
    {
        // 32-bit PE format
        IMAGE_NT_HEADERS32 *nt_header32 = (IMAGE_NT_HEADERS32 *)nt_header_ptr;
        entry_point_addr = static_cast<ULONGLONG>(nt_header32->OptionalHeader.AddressOfEntryPoint);
    }

    return entry_point_addr;
}