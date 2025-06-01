# Process Doppelgänging: Song trùng tiến trình

<p align="center">
  <img src="Malware_logo.png" alt="Logo" width="300"/>
</p>

<p align="center">
  <a href="https://opensource.org/licenses/MIT"><img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License: MIT"></a>
  <a href="https://www.microsoft.com/windows"><img src="https://img.shields.io/badge/Platform-Windows-brightgreen.svg" alt="Platform"></a>
  <a href="https://github.com/fu0nd41/process_doppelganging"><img src="https://img.shields.io/badge/Build-Passing-success.svg" alt="Build Status"></a>
</p>

## Tổng Quan

**Process Doppelgänging** là một kỹ thuật tấn công tinh vi được phát hiện lần đầu tiên vào năm 2017 bởi các nhà nghiên cứu bảo mật tại BlackHat. Dự án này cung cấp một triển khai đầy đủ có tính giáo dục của kỹ thuật này, nhằm mục đích hỗ trợ các chuyên gia bảo mật và nhà nghiên cứu hiểu rõ hơn về các phương pháp tấn công tiên tiến.

Không giống như các kỹ thuật tấn công thông thường, Process Doppelgänging khai thác cơ chế giao dịch NTFS của Windows và các hàm API không được công khai để tạo ra tiến trình ẩn danh. Mã độc được thực thi mà không để lại dấu vết trên đĩa, đồng thời vượt qua đa số giải pháp bảo mật dựa trên giám sát tệp.

---

## Đặc Điểm Kỹ Thuật

Triển khai này mang đến những ưu điểm kỹ thuật vượt trội so với các phương pháp thay thế:

- **Ánh Xạ MEM_IMAGE Tinh Vi**: Payload được ánh xạ vào bộ nhớ dưới dạng `MEM_IMAGE`, không liên kết với bất kỳ tệp nào trên đĩa, khiến các công cụ giám sát tệp truyền thống không thể phát hiện.

- **Tối Ưu Hóa Quyền Truy Cập Bộ Nhớ**: Các phần của payload được ánh xạ với quyền truy cập nguyên bản (không sử dụng phân vùng nhớ RWX - Read-Write-Execute), hạn chế khả năng phát hiện của các giải pháp bảo mật dựa trên heuristic.

- **Tích Hợp PEB Liền Mạch**: Payload được kết nối toàn diện với Process Environment Block (PEB) như module chính của tiến trình, tạo ra vẻ ngoài của một ứng dụng hợp pháp.

- **Tiêm Từ Xa Không Phát Hiện**: Hỗ trợ tiêm mã từ xa vào tiến trình mới được tạo, mở rộng phạm vi ứng dụng của kỹ thuật này.

- **Module Không Tên**: Tiến trình được tạo từ module không có tên rõ ràng (`GetProcessImageFileName` trả về chuỗi rỗng), làm giảm khả năng phát hiện dựa trên phân tích tên tiến trình.

---

## Cơ Chế Hoạt Động

Kỹ thuật Process Doppelgänging hoạt động theo trình tự sau:

1. **Tạo Giao Dịch NTFS**: Bắt đầu bằng việc tạo một giao dịch NTFS mới.

2. **Tạo Tệp Tạm Thời**: Tạo một tệp tạm thời trong giao dịch, nhưng tệp này không được ghi vào đĩa vật lý.

3. **Ghi Payload**: Ghi dữ liệu payload vào tệp giao dịch này.

4. **Tạo Section**: Sử dụng NtCreateSection để tạo một section từ tệp giao dịch.

5. **Hoàn Tác Giao Dịch**: Hoàn tác giao dịch NTFS, xóa tệp tạm thời khỏi đĩa nhưng vẫn giữ section trong bộ nhớ.

6. **Tạo Tiến Trình**: Sử dụng NtCreateProcessEx để tạo một tiến trình mới từ section.

7. **Cấu Hình Tiến Trình**: Thiết lập các tham số tiến trình cần thiết.

8. **Tạo Luồng**: Bắt đầu thực thi payload bằng cách tạo luồng chính.

Cơ chế này đảm bảo rằng payload không bao giờ được ghi vào đĩa dưới dạng tệp hoàn chỉnh, do đó tránh được nhiều hệ thống phát hiện mã độc.

---

## Yêu Cầu Hệ Thống

- **Hệ điều hành**: Windows 7/8/10/11
- **Môi trường phát triển**:
  - Visual Studio 2015 hoặc mới hơn
  - CMake 2.8 hoặc mới hơn
- **Thư viện bắt buộc**:
  - KtmW32.lib
  - Ntdll.lib

---

## Cài Đặt & Biên Dịch

### Sử dụng CMake

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### Sử dụng Visual Studio

1. Mở Visual Studio và chọn "Open a local folder"
2. Chọn thư mục của dự án
3. Nhấp chuột phải vào file CMakeLists.txt và chọn "Configure All Projects"
4. Chọn "Build All" để biên dịch

---

## Cách Sử Dụng

Sau khi biên dịch thành công, bạn có thể sử dụng công cụ này qua dòng lệnh:

```
process_doppelganging_poc.exe <đường_dẫn_payload> [*đường_dẫn_đích]
```

Trong đó:

- `<đường_dẫn_payload>`: Đường dẫn đến tệp thực thi PE (EXE) cần thực thi ẩn danh
- `[*đường_dẫn_đích]`: (Tùy chọn) Đường dẫn đến quy trình đích. Nếu không cung cấp, chương trình sẽ sử dụng calc.exe mặc định của hệ thống.

### Ví dụ:

```
process_doppelganging_poc.exe C:\path\to\payload.exe
```

Hoặc với đường dẫn đích cụ thể:

```
process_doppelganging_poc.exe C:\path\to\payload.exe C:\Windows\System32\notepad.exe
```

---

## Kiến Trúc Mã Nguồn

Dự án được tổ chức thành các module chức năng riêng biệt để dễ dàng bảo trì và mở rộng:

| Tệp                   | Mô tả                                                                                               |
| --------------------- | --------------------------------------------------------------------------------------------------- |
| `main.cpp`            | Điểm vào chính và triển khai logic cốt lõi của kỹ thuật Process Doppelgänging                       |
| `file_helper.h/cpp`   | Cung cấp các hàm tiện ích để thao tác với tệp và đường dẫn hệ thống                                 |
| `win_api_ext.h/cpp`   | Mở rộng Windows API không được công khai, định nghĩa và khởi tạo các hàm API cần thiết từ ntdll.dll |
| `win_structs.h`       | Định nghĩa các cấu trúc dữ liệu Windows không được công khai cần thiết cho việc triển khai          |
| `win_kernel_defs.h`   | Định nghĩa các cấu trúc và hằng số kernel Windows quan trọng                                        |
| `binary_parser.h/cpp` | Công cụ phân tích định dạng tệp PE (Portable Executable) để trích xuất thông tin cần thiết          |
| `proc_context.h/cpp`  | Quản lý ngữ cảnh tiến trình và các thao tác với PEB (Process Environment Block)                     |

---

## Ứng Dụng Nghiên Cứu

Dự án này được phát triển với mục đích:

1. **Nghiên cứu Học thuật**: Hiểu sâu hơn về các cơ chế nội bộ của Windows và cách chúng có thể bị khai thác
2. **Kiểm thử Bảo mật**: Đánh giá hiệu quả của các giải pháp bảo mật trước các kỹ thuật tấn công tiên tiến
3. **Phát triển Phòng thủ**: Nghiên cứu các cơ chế phòng thủ mới chống lại các kỹ thuật tấn công này

> **Cảnh báo Pháp lý**: Việc sử dụng công cụ này để mục đích độc hại là bất hợp pháp. Tác giả và các nhà phát triển không chịu trách nhiệm về việc sử dụng sai mục đích.

---

## Tài Liệu Tham Khảo

1. Tal Liberman & Eugene Kogan. "Lost in Transaction: Process Doppelgänging." BlackHat Europe, 2017.
2. Microsoft Windows Internals, 7th Edition. Pavel Yosifovich, et al. Microsoft Press, 2017.
3. Alex Ionescu & Yarden Shafir. "Windows Process Injection in 2019." Blackhat USA, 2019.
4. "Windows Native API Reference." MSDN, Microsoft Corporation.

---

## Tác Giả

**fu0nd41**

[![GitHub](https://img.shields.io/badge/GitHub-fu0nd41-181717?style=flat-square&logo=github)](https://github.com/fuondai)

---

## Giấy Phép

Dự án này được phân phối dưới Giấy phép MIT. Xem tệp `LICENSE` để biết thêm thông tin.

---

<div align="center">
  <sub>⚠️ CHỈ SỬ DỤNG VỚI MỤC ĐÍCH NGHIÊN CỨU VÀ GIÁO DỤC ⚠️</sub>
</div>
