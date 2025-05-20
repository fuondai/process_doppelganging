# Bộ công cụ Process Doppelgänging

Bộ công cụ này triển khai các kỹ thuật né tránh phát hiện nâng cao sử dụng Process Doppelgänging với các lớp che giấu và khả năng phục hồi bổ sung.

## Tổng quan

Process Doppelgänging là một kỹ thuật tiêm mã không cần file, tận dụng giao dịch NTFS để tải mã độc đồng thời né tránh sự phát hiện của nhiều sản phẩm bảo mật. Triển khai này mở rộng kỹ thuật với:

- Nén UPX
- Mã hóa/che giấu tùy chỉnh
- Mô hình thực thi nhiều giai đoạn
- Cơ chế dự phòng để đảm bảo thực thi đáng tin cậy

## Các thành phần

1. **process_doppelganging.exe**: Tệp thực thi chính triển khai kỹ thuật Process Doppelgänging
2. **persistence.exe**: Tệp thực thi mục tiêu cần được thực thi (payload độc hại của bạn)
3. **persistence_packed.exe**: Phiên bản nén UPX của persistence.exe
4. **persistence_obfuscated.exe**: Phiên bản đã mã hóa của tệp thực thi đã nén
5. **MinLoader.exe**: Giải mã và khởi chạy payload với nhiều cơ chế dự phòng
6. **Obfuscator.exe**: Công cụ để mã hóa payload đã nén

## Hướng dẫn sử dụng

### Bước 1: Nén bằng UPX

Đầu tiên, nén tệp thực thi của bạn bằng UPX để giảm kích thước và làm cho nó khó phân tích hơn:

```
upx.exe --best persistence.exe -o persistence_packed.exe
```

### Bước 2: Che giấu tệp thực thi đã nén

Sau khi nén, mã hóa tệp thực thi bằng công cụ mã hóa tùy chỉnh:

```
Obfuscator.exe persistence_packed.exe persistence_obfuscated.exe
```

### Bước 3: Triển khai và thực thi

Có hai phương pháp thực thi chính:

#### Phương pháp A: Process Doppelgänging trực tiếp trên MinLoader

Cách tiếp cận này có MinLoader tìm và giải mã tệp persistence:

```
process_doppelganging.exe MinLoader.exe
```

#### Phương pháp B: Process Doppelgänging với chỉ định đường dẫn

Để đạt độ tin cậy tối đa, cung cấp đường dẫn đầy đủ đến tệp đã được che giấu:

```
process_doppelganging.exe MinLoader.exe "C:\Users\a\Desktop\persistence_obfuscated.exe"
```

## Chi tiết kỹ thuật

### Các lớp che giấu

Bộ công cụ sử dụng nhiều lớp bảo vệ:

1. **Nén UPX**: Nén và che giấu tệp thực thi gốc
2. **Mã hóa tùy chỉnh**:
   - XOR với khóa tùy chỉnh
   - Xoay byte (ROL)
   - XOR bổ sung với các byte lân cận
3. **Giải mã**: MinLoader thực hiện quy trình ngược lại

### Tính năng của MinLoader

MinLoader được thiết kế với khả năng phục hồi cực cao:

- Nhiều chiến lược xác định vị trí tệp
- Năm phương pháp thực thi khác nhau
- Ghi tệp dự phòng
- Ghi log toàn diện
- Nỗ lực vượt qua UAC
- Tự động dọn dẹp các tệp tạm thời

### Triển khai Process Doppelgänging

Kỹ thuật này tạo một tệp giao dịch, thay thế nội dung của nó bằng payload, sau đó khởi chạy tiến trình từ giao dịch này trước khi xác nhận thay đổi, vượt qua nhiều cơ chế bảo mật.

## Vị trí đặt tệp

Để hoạt động tối ưu:

- Đặt tất cả các tệp ở một vị trí chung (ví dụ: Desktop)
- Hoặc chỉ định đường dẫn tuyệt đối khi thực thi

## Xử lý sự cố

Nếu việc thực thi thất bại:

- Kiểm tra nhật ký tại `C:\Users\a\AppData\Local\Temp\minloader_debug.log`
- Đảm bảo các tệp mục tiêu tồn tại ở các vị trí dự kiến
- Thử chạy với đường dẫn rõ ràng

## Lưu ý về bảo mật

Bộ công cụ này được thiết kế chỉ cho mục đích giáo dục và kiểm tra bảo mật hợp pháp. Sử dụng trái phép đối với các hệ thống mà không có sự cho phép rõ ràng là bất hợp pháp và phi đạo đức.
