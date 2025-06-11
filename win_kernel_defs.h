#ifndef __WIN_KERNEL_DEFS_H__
#define __WIN_KERNEL_DEFS_H__

#ifdef __cplusplus
extern "C"
{
#endif
#include <Windows.h>

#ifdef _NTDDK_
#error Không thể kết hợp tệp này với NTDDK
#endif

#pragma warning(disable : 4201) // cảnh báo về phần mở rộng không chuẩn: cấu trúc/union không tên

#pragma warning(push)
#pragma warning(disable : 4005)
#include <ntstatus.h>
#pragma warning(pop)

    // Định nghĩa cho NTSTATUS
    typedef long NTSTATUS;

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#endif

#ifndef ASSERT
#ifdef _DEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x) /* x */
#endif
#endif

    // Định nghĩa thêm một số kiểu dữ liệu
    typedef LONG KPRIORITY;

    // Cấu trúc dữ liệu
    typedef enum _EVENT_TYPE
    {
        NotificationEvent,
        SynchronizationEvent
    } EVENT_TYPE;

// Chuỗi ANSI là chuỗi ký tự 8-bit có đếm. Nếu chúng kết thúc bằng NULL, Length không bao gồm NULL cuối cùng.
#ifndef _NTSECAPI_
    typedef struct _STRING
    {
        USHORT Length;
        USHORT MaximumLength;
        PCHAR Buffer;
    } STRING, *PSTRING;

    // Chuỗi Unicode là chuỗi ký tự 16-bit có đếm. Nếu chúng kết thúc bằng NULL, Length không bao gồm NULL cuối cùng.
    typedef struct _UNICODE_STRING
    {
        USHORT Length;
        USHORT MaximumLength;
        PWSTR Buffer;
    } UNICODE_STRING, *PUNICODE_STRING;
#endif // _NTSECAPI_

    typedef STRING ANSI_STRING;
    typedef PSTRING PANSI_STRING;

    typedef STRING OEM_STRING;
    typedef PSTRING POEM_STRING;
    typedef CONST STRING *PCOEM_STRING;

    typedef const UNICODE_STRING *PCUNICODE_STRING;

#define UNICODE_NULL ((WCHAR)0) // winnt

// Giá trị hợp lệ cho trường Attributes
#ifndef OBJ_CASE_INSENSITIVE
#define OBJ_INHERIT 0x00000002L
#define OBJ_PERMANENT 0x00000010L
#define OBJ_EXCLUSIVE 0x00000020L
#define OBJ_CASE_INSENSITIVE 0x00000040L
#define OBJ_OPENIF 0x00000080L
#define OBJ_OPENLINK 0x00000100L
#define OBJ_KERNEL_HANDLE 0x00000200L
#define OBJ_FORCE_ACCESS_CHECK 0x00000400L
#define OBJ_VALID_ATTRIBUTES 0x000007F2L

    // Cấu trúc Object Attributes
    typedef struct _OBJECT_ATTRIBUTES
    {
        ULONG Length;
        HANDLE RootDirectory;
        PUNICODE_STRING ObjectName;
        ULONG Attributes;
        PVOID SecurityDescriptor;       // Trỏ đến kiểu SECURITY_DESCRIPTOR
        PVOID SecurityQualityOfService; // Trỏ đến kiểu SECURITY_QUALITY_OF_SERVICE
    } OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
#endif // OBJ_CASE_INSENSITIVE

    // IO_STATUS_BLOCK
    typedef struct _IO_STATUS_BLOCK
    {
        union
        {
            NTSTATUS Status;
            PVOID Pointer;
        };
        ULONG_PTR Information;
    } IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

    // ClientId
    typedef struct _CLIENT_ID
    {
        HANDLE UniqueProcess;
        HANDLE UniqueThread;
    } CLIENT_ID, *PCLIENT_ID;

    // Cấu trúc CURDIR
    typedef struct _CURDIR
    {
        UNICODE_STRING DosPath;
        HANDLE Handle;
    } CURDIR, *PCURDIR;

// Macro
// INIT_UNICODE_STRING là thay thế cho RtlInitUnicodeString
#ifndef INIT_UNICODE_STRING
#define INIT_UNICODE_STRING(us, wch)                   \
    us.MaximumLength = (USHORT)sizeof(wch);            \
    us.Length = (USHORT)(wcslen(wch) * sizeof(WCHAR)); \
    us.Buffer = wch
#endif

#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes(p, n, a, r, s) \
    {                                             \
        (p)->Length = sizeof(OBJECT_ATTRIBUTES);  \
        (p)->RootDirectory = r;                   \
        (p)->Attributes = a;                      \
        (p)->ObjectName = n;                      \
        (p)->SecurityDescriptor = s;              \
        (p)->SecurityQualityOfService = NULL;     \
    }
#endif

    // Hàm đường dẫn Unicode
    NTSYSAPI
    VOID
        NTAPI
        RtlInitUnicodeString(
            PUNICODE_STRING DestinationString,
            PCWSTR SourceString);

    // Cấu trúc PEB_FREE_BLOCK
    typedef struct _PEB_FREE_BLOCK
    {
        struct _PEB_FREE_BLOCK *Next;
        ULONG Size;
    } PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

    // Cấu trúc PEB_LDR_DATA
    typedef struct _PEB_LDR_DATA
    {
        ULONG Length;
        BOOLEAN Initialized;
        HANDLE SsHandle;
        LIST_ENTRY InLoadOrderModuleList;   // Trỏ đến các module đã nạp (thường là EXE chính)
        LIST_ENTRY InMemoryOrderModuleList; // Trỏ đến tất cả các module (EXE và tất cả DLL)
        LIST_ENTRY InInitializationOrderModuleList;
        PVOID EntryInProgress;
    } PEB_LDR_DATA, *PPEB_LDR_DATA;

    // Cấu trúc LDR_DATA_TABLE_ENTRY
    typedef struct _LDR_DATA_TABLE_ENTRY
    {
        LIST_ENTRY InLoadOrderLinks;
        LIST_ENTRY InMemoryOrderLinks;
        LIST_ENTRY InInitializationOrderLinks;
        PVOID DllBase; // Địa chỉ cơ sở của module
        PVOID EntryPoint;
        ULONG SizeOfImage;
        UNICODE_STRING FullDllName;
        UNICODE_STRING BaseDllName;
        ULONG Flags;
        USHORT LoadCount;
        USHORT TlsIndex;
        LIST_ENTRY HashLinks;
        PVOID SectionPointer;
        ULONG CheckSum;
        ULONG TimeDateStamp;
        PVOID LoadedImports;
        PVOID EntryPointActivationContext;
        PVOID PatchInformation;
        PVOID Unknown1;
        PVOID Unknown2;
        PVOID Unknown3;
    } LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

    // Cấu trúc RTL_DRIVE_LETTER_CURDIR
    typedef struct _RTL_DRIVE_LETTER_CURDIR
    {
        USHORT Flags;
        USHORT Length;
        ULONG TimeStamp;
        STRING DosPath;
    } RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

    // Cấu trúc RTL_USER_PROCESS_PARAMETERS
    typedef struct _RTL_USER_PROCESS_PARAMETERS
    {
        ULONG MaximumLength; // Nên được thiết lập trước khi gọi RtlCreateProcessParameters
        ULONG Length;        // Độ dài của cấu trúc hợp lệ
        ULONG Flags;         // Hiện tại chỉ có PPF_NORMALIZED (1) là đã biết:
        // - Nghĩa là cấu trúc được chuẩn hóa bởi lệnh gọi RtlNormalizeProcessParameters
        ULONG DebugFlags;

        PVOID ConsoleHandle; // HWND tới cửa sổ console liên kết với tiến trình (nếu có).
        ULONG ConsoleFlags;
        HANDLE StandardInput;
        HANDLE StandardOutput;
        HANDLE StandardError;

        CURDIR CurrentDirectory;      // Đường dẫn ký hiệu kiểu DOS, ví dụ: "C:/WinNT/SYSTEM32"
        UNICODE_STRING DllPath;       // Đường dẫn kiểu DOS được phân tách bằng ';' nơi hệ thống tìm kiếm các tệp DLL.
        UNICODE_STRING ImagePathName; // Đường dẫn đầy đủ ở định dạng kiểu DOS đến tệp hình ảnh của tiến trình.
        UNICODE_STRING CommandLine;   // Dòng lệnh
        PVOID Environment;            // Con trỏ đến khối môi trường (xem RtlCreateEnvironment)
        ULONG StartingX;
        ULONG StartingY;
        ULONG CountX;
        ULONG CountY;
        ULONG CountCharsX;
        ULONG CountCharsY;
        ULONG FillAttribute; // Thuộc tính điền cho cửa sổ console
        ULONG WindowFlags;
        ULONG ShowWindowFlags;
        UNICODE_STRING WindowTitle;
        UNICODE_STRING DesktopInfo; // Tên của các đối tượng WindowStation và Desktop nơi tiến trình được gán
        UNICODE_STRING ShellInfo;
        UNICODE_STRING RuntimeData;
        RTL_DRIVE_LETTER_CURDIR CurrentDirectores[0x20];
        ULONG EnvironmentSize;
    } RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

    // Cấu trúc PEB
    typedef struct _PEB
    {
        BOOLEAN InheritedAddressSpace;    // Bốn trường này không thể thay đổi trừ khi
        BOOLEAN ReadImageFileExecOptions; // cấu trúc INITIAL_PEB cũng được cập nhật.
        BOOLEAN BeingDebugged;            //
        BOOLEAN SpareBool;                //
        HANDLE Mutant;                    //

        PVOID ImageBaseAddress;
        PPEB_LDR_DATA Ldr;
        PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
        PVOID SubSystemData;
        PVOID ProcessHeap;
        PVOID FastPebLock;
        PVOID FastPebLockRoutine;
        PVOID FastPebUnlockRoutine;
        ULONG EnvironmentUpdateCount;
        PVOID KernelCallbackTable;
        HANDLE SystemReserved;
        PVOID AtlThunkSListPtr32;
        PPEB_FREE_BLOCK FreeList;
        ULONG TlsExpansionCounter;
        PVOID TlsBitmap;
        ULONG TlsBitmapBits[2]; // liên quan đến TLS_MINIMUM_AVAILABLE
        PVOID ReadOnlySharedMemoryBase;
        PVOID ReadOnlySharedMemoryHeap;
        PVOID *ReadOnlyStaticServerData;
        PVOID AnsiCodePageData;
        PVOID OemCodePageData;
        PVOID UnicodeCaseTableData;

        // Thông tin hữu ích cho LdrpInitialize
        ULONG NumberOfProcessors;
        ULONG NtGlobalFlag;

        // Được truyền từ MmCreatePeb từ khóa registry Trình quản lý phiên
        LARGE_INTEGER CriticalSectionTimeout;
        ULONG HeapSegmentReserve;
        ULONG HeapSegmentCommit;
        ULONG HeapDeCommitTotalFreeThreshold;
        ULONG HeapDeCommitFreeBlockThreshold;

        // Nơi trình quản lý heap theo dõi tất cả các heap được tạo cho một tiến trình
        // Các trường được khởi tạo bởi MmCreatePeb. ProcessHeaps được khởi tạo
        // để trỏ đến byte trống đầu tiên sau PEB và MaximumNumberOfHeaps
        // được tính toán từ kích thước trang được sử dụng để giữ PEB, trừ đi
        // kích thước cố định của cấu trúc dữ liệu này.
        ULONG NumberOfHeaps;
        ULONG MaximumNumberOfHeaps;
        PVOID *ProcessHeaps;

        PVOID GdiSharedHandleTable;
        PVOID ProcessStarterHelper;
        PVOID GdiDCAttributeList;
        PVOID LoaderLock;

        // Các trường sau được điền bởi MmCreatePeb từ các giá trị hệ thống và/hoặc
        // header hình ảnh. Các trường này đã thay đổi kể từ Windows NT 4.0,
        // vì vậy hãy sử dụng cẩn thận
        ULONG OSMajorVersion;
        ULONG OSMinorVersion;
        USHORT OSBuildNumber;
        USHORT OSCSDVersion;
        ULONG OSPlatformId;
        ULONG ImageSubsystem;
        ULONG ImageSubsystemMajorVersion;
        ULONG ImageSubsystemMinorVersion;
        ULONG ImageProcessAffinityMask;
        ULONG GdiHandleBuffer[34];
    } PEB, *PPEB;

    // Cấu trúc TEB
    typedef struct _TEB
    {
        NT_TIB NtTib;
        PVOID EnvironmentPointer;
        CLIENT_ID ClientId;
        PVOID ActiveRpcHandle;
        PVOID ThreadLocalStoragePointer;
        PPEB ProcessEnvironmentBlock;
        ULONG LastErrorValue;
        ULONG CountOfOwnedCriticalSections;
        PVOID CsrClientThread;
        PVOID Win32ThreadInfo;
        // Không đầy đủ
    } TEB, *PTEB;

    // Cấu trúc PROCESS_BASIC_INFORMATION
    typedef struct _PROCESS_BASIC_INFORMATION
    {
        NTSTATUS ExitStatus;
        PPEB PebBaseAddress;
        ULONG_PTR AffinityMask;
        KPRIORITY BasePriority;
        ULONG_PTR UniqueProcessId;
        ULONG_PTR InheritedFromUniqueProcessId;
    } PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

    // Hàm đọc/ghi bộ nhớ ảo
    NTSYSAPI
    NTSTATUS
    NTAPI
    NtReadVirtualMemory(
        IN HANDLE ProcessHandle,
        IN PVOID BaseAddress,
        OUT PVOID Buffer,
        IN ULONG NumberOfBytesToRead,
        OUT PULONG NumberOfBytesRead OPTIONAL);

// Định nghĩa PS_INHERIT_HANDLES
#define PS_INHERIT_HANDLES 4

    // Định nghĩa enum _PROCESSINFOCLASS
    typedef enum _PROCESSINFOCLASS
    {
        ProcessBasicInformation,
        ProcessQuotaLimits,
        ProcessIoCounters,
        ProcessVmCounters,
        ProcessTimes,
        ProcessBasePriority,
        ProcessRaisePriority,
        ProcessDebugPort,
        ProcessExceptionPort,
        ProcessAccessToken,
        ProcessLdtInformation,
        ProcessLdtSize,
        ProcessDefaultHardErrorMode,
        ProcessIoPortHandlers,
        ProcessPooledUsageAndLimits,
        ProcessWorkingSetWatch,
        ProcessUserModeIOPL,
        ProcessEnableAlignmentFaultFixup,
        ProcessPriorityClass,
        ProcessWx86Information,
        ProcessHandleCount,
        ProcessAffinityMask,
        ProcessPriorityBoost,
        MaxProcessInfoClass
    } PROCESSINFOCLASS;

    // Hàm tạo Section
    NTSYSAPI
    NTSTATUS
    NTAPI
    NtCreateSection(
        OUT PHANDLE SectionHandle,
        IN ACCESS_MASK DesiredAccess,
        IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
        IN PLARGE_INTEGER MaximumSize OPTIONAL,
        IN ULONG SectionPageProtection,
        IN ULONG AllocationAttributes,
        IN HANDLE FileHandle OPTIONAL);

    // Hàm truy vấn thông tin tiến trình
    NTSYSAPI
    NTSTATUS
    NTAPI
    NtQueryInformationProcess(
        IN HANDLE ProcessHandle,
        IN PROCESSINFOCLASS ProcessInformationClass,
        OUT PVOID ProcessInformation,
        IN ULONG ProcessInformationLength,
        OUT PULONG ReturnLength OPTIONAL);

// Lấy handle của tiến trình hiện tại
#ifndef NtCurrentProcess
#define NtCurrentProcess() ((HANDLE)(LONG_PTR) - 1)
#endif

#ifdef __cplusplus
}
#endif

#endif // __WIN_KERNEL_DEFS_H__