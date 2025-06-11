## Chi tiết từng thành phần của dự án

### 1. persistence.cpp

File này triển khai các cơ chế duy trì (persistence) sau:

- **Tạo Startup Registry Key với tên Apollo**:

  - Sử dụng hàm `CreateApolloStartupEntry()` để tạo registry key trong `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run`
  - Tự động khởi chạy khi người dùng đăng nhập

- **Scheduled Task sử dụng COM API**:

  - Hàm `CreateStealthyTask()` sử dụng ITaskService COM Interface để tạo tác vụ
  - Đặt thuộc tính `TASK_RUNLEVEL_HIGHEST` để chạy với quyền cao nhất
  - Cấu hình task với thuộc tính `Hidden` và `StartWhenAvailable`
  - Tạo nhiều trigger: LOGON, BOOT, và DAILY
  - Ngụy trang task với mô tả "Updates critical Windows components"

- **Sao chép vào System32**:

  - Hàm `CopyToSystem32()` tạo bản sao tại `C:\Windows\System32\Apollo.exe`
  - Tự động tạo registry key để khởi động cùng hệ thống

- **WMI Event Subscription**:

  - Hàm `CreateWMISubscription()` tạo VBS script ẩn trong temp directory
  - Sử dụng PowerShell để tạo WMI Event Filter dựa trên sự kiện Win32_LocalTime
  - Đăng ký ActiveScriptEventConsumer để thực thi VBS script
  - Tạo FilterToConsumerBinding để liên kết filter và consumer

- **Kỹ thuật ngụy trang và ẩn file**:

  - Hàm `ObfuscateExecutable()` đặt thuộc tính FILE_ATTRIBUTE_HIDDEN và FILE_ATTRIBUTE_SYSTEM
  - Tạo file CMD Launcher ẩn để thực thi backdoor với hàm `CreateCommandLauncher()`
  - Sử dụng nhiều vị trí thay thế để lưu trữ backdoor

- **Hàm tiện ích**:

  - `EnsureDotNetSupport()`: Kiểm tra và kích hoạt .NET Framework
  - `EnsureDirectoryExists()`: Tạo đệ quy thư mục nếu chưa tồn tại
  - `WideToNarrow()` và `NarrowToWide()`: Chuyển đổi giữa chuỗi ký tự Unicode và ANSI

- **Hàm WinMain**:
  - Thử tắt UAC dialog với lệnh registry
  - Tìm file backdoor trong thư mục temp
  - Sao chép vào nhiều vị trí ẩn
  - Triển khai đồng thời nhiều cơ chế persistence

### 2. docx.cpp

File này triển khai một loader và downloader với các kỹ thuật né tránh:

- **Tải file từ internet**:

  - Hàm `DownloadFile()` sử dụng WinInet API để tải từ URL
  - Xáo trộn User-Agent ngẫu nhiên từ danh sách giả mạo trình duyệt
  - Thêm Referer giả mạo để trông giống lưu lượng truy cập bình thường
  - Sử dụng kết nối HTTPS an toàn (INTERNET_DEFAULT_HTTPS_PORT)
  - Thêm độ trễ ngẫu nhiên (500-1500ms) để né tránh phát hiện dựa trên thời gian

- **Mã hóa và bảo vệ dữ liệu**:

  - Hàm `TransformData()` sử dụng thuật toán XOR với khóa động
  - Hàm `GenerateKey()` tạo khóa ngẫu nhiên dựa trên std::mt19937
  - Hàm `DecryptFile()` giải mã file đã tải xuống
  - Lưu trữ dữ liệu tạm thời với tên ngẫu nhiên bằng `GenerateRandomName()`

- **Né tránh phân tích**:

  - Vô hiệu hóa kiểm tra sandbox với `IsRunningInSandbox()`
  - Vô hiệu hóa phát hiện công cụ phân tích với `HasAnalysisTools()`
  - Lưu file tạm thời với tên ngẫu nhiên để tránh phát hiện dựa trên tên file

- **Thực thi trong bộ nhớ**:

  - Tải file vào tệp tạm thời
  - Giải mã trong bộ nhớ
  - Thực thi payload mà không ghi vào đĩa trực tiếp

- **Hàm main**:
  - Ẩn cửa sổ console với `MinimizeConsoleWindow()`
  - Tải file từ internet, giải mã và lưu vào thư mục tạm
  - Thực thi file đã giải mã
  - Tự động xóa file tạm sau khi chạy

### 3. MinLoader.cpp

File này là module tải tối thiểu với nhiều kỹ thuật dự phòng:

- **Giải mã payload đã mã hóa**:

  - Hàm `DecryptData()` đảo ngược 3 lớp mã hóa:
    1. Đảo ngược XOR với mảng khóa cố định
    2. Đảo ngược phép xoay byte (ROR)
    3. Đảo ngược XOR với giá trị 0xA5 và byte tiếp theo
  - Hàm `DecryptToTempLocation()` giải mã file và lưu vào đường dẫn cố định

- **Nhiều cách thực thi file**:

  - Hàm `RunExecutableWithMultipleMethods()` thử 5 phương pháp khác nhau:
    1. ShellExecuteEx với verb "runas" (để lấy quyền admin)
    2. ShellExecuteEx thông thường
    3. CreateProcess với quyền cao nhất
    4. CreateProcess ẩn với CreateNoWindow
    5. WinExec với SW_HIDE

- **Cơ chế tìm file dự phòng**:

  - Hàm `CheckFileExistence()` tìm file trong nhiều vị trí
  - Kiểm tra cả đường dẫn tuyệt đối và tương đối
  - Tìm kiếm trong các thư mục hệ thống khác nhau

- **Ghi log chi tiết**:

  - Hàm `LogToFile()` và `LogToFileW()` ghi log vào vị trí cố định
  - Ghi chi tiết từng bước thực thi và lỗi gặp phải
  - Hỗ trợ cả chuỗi ASCII và Unicode

- **Xử lý lỗi mạnh mẽ**:

  - Sử dụng khối try-catch để bắt mọi ngoại lệ
  - Thử các phương pháp khác nếu một phương pháp thất bại
  - Xử lý lỗi tại mỗi bước

- **Tự động dọn dẹp**:

  - Hàm `DeleteFilesDelayed()` tạo script để xóa file tạm
  - Triển khai cơ chế xóa file trì hoãn để tránh bị khóa

- **Hàm WinMain**:
  - Khởi tạo file log
  - Tìm và giải mã file persistence đã mã hóa
  - Thử nhiều phương pháp thực thi
  - Tự động dọn dẹp khi hoàn thành

### 4. Obfuscator.cpp

Công cụ độc lập để mã hóa các tệp thực thi với nhiều lớp bảo vệ:

- **Mã hóa nhiều lớp**:

  - Hàm `EncryptData()` áp dụng 3 lớp mã hóa:
    1. XOR với mảng khóa cố định (16 byte)
    2. Xoay byte sang trái (ROL 3 bits)
    3. XOR bổ sung với byte tiếp theo và giá trị 0xA5

- **Khóa mã hóa cố định**:

  - Sử dụng mảng KEY được định nghĩa tĩnh
  - Khóa phải giống với khóa trong MinLoader.cpp

- **Xử lý file nhị phân**:

  - Đọc file nhị phân vào vector<unsigned char>
  - Thực hiện mã hóa trực tiếp trên buffer
  - Ghi lại dữ liệu đã mã hóa

- **Hàm main**:
  - Nhận đường dẫn file đầu vào và đầu ra từ tham số
  - Đọc toàn bộ file vào bộ nhớ
  - Mã hóa dữ liệu
  - Ghi file đã mã hóa
  - Hiển thị thông tin về kích thước file

## Kỹ thuật Process Doppelgänging

Process Doppelgänging là một kỹ thuật tận dụng giao dịch NTFS và các API Windows cấp thấp để thực thi code độc hại mà không bị phát hiện. Kỹ thuật này hoạt động qua các bước sau:

1. **Tạo giao dịch NTFS (NtCreateTransaction)**:

   - Tạo một giao dịch NTFS để thực hiện các thay đổi tạm thời

2. **Mở tệp hợp pháp trong giao dịch (NtCreateFile với TransactionContext)**:

   - Mở một file hợp pháp đã tồn tại (như notepad.exe) trong giao dịch này

3. **Thay thế nội dung trong giao dịch (NtWriteFile)**:

   - Ghi đè nội dung của file hợp pháp bằng mã độc trong phạm vi giao dịch

4. **Tạo section và memory mapping (NtCreateSection)**:

   - Tạo một section object từ file đã được thay đổi

5. **Tạo process từ section (NtCreateProcessEx)**:

   - Tạo tiến trình mới từ section đã tạo (chứa mã độc)
   - Khởi tạo các thread với NtCreateThreadEx

6. **Đóng giao dịch mà không commit (NtRollbackTransaction)**:
   - Hủy bỏ giao dịch, loại bỏ mọi thay đổi trên đĩa
   - Chỉ còn tiến trình đang chạy trong bộ nhớ

Kỹ thuật này vượt qua được nhiều giải pháp bảo mật vì:

- Không có tệp độc hại nào được ghi vào đĩa
- Không sử dụng kỹ thuật injection truyền thống
- Quét file tĩnh không phát hiện được mã độc
- Các công cụ theo dõi hệ thống chỉ thấy một file hợp pháp đang chạy

## Chỉ dành cho mục đích giáo dục

Mã nguồn này được cung cấp CHỈ cho mục đích nghiên cứu bảo mật, để hiểu rõ hơn về cách thức hoạt động của các kỹ thuật tấn công tiên tiến và cách phát hiện/phòng chống chúng.
