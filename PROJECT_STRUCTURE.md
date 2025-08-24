# Cấu trúc Dự án Keylogger Research Project

## Tổng quan

Dự án này là một keylogger nghiên cứu được viết bằng C++ cho Windows, bao gồm các tính năng ghi lại phím, chuột, chụp màn hình và các cơ chế bảo mật, ẩn danh.

## Chi tiết các thành phần

### 📁 src/

#### 📄 main.cpp

- Điểm vào chính của chương trình
- Khởi tạo ứng dụng và kiểm tra môi trường
- Xử lý chế độ chạy như service hoặc ứng dụng thường

#### 📁 core/

- **Application.cpp**: Quản lý vòng đời chính của ứng dụng
- **Configuration.cpp**: Đọc và quản lý cấu hình từ file/registry
- **Logger.cpp**: Hệ thống logging với mã hóa và xoay file log

#### 📁 hooks/

- **KeyHook.cpp**: Hook bàn phím để ghi lại thao tác gõ phím
- **MouseHook.cpp**: Hook chuột để theo dõi di chuyển và click
- **SystemHook.cpp**: Theo dõi sự kiện hệ thống như thay đổi cửa sổ

#### 📁 communication/

- **CommsManager.cpp**: Quản lý các phương thức truyền dữ liệu
- **HttpComms.cpp**: Truyền dữ liệu qua HTTP/HTTPS
- **FtpComms.cpp**: Truyền dữ liệu qua FTP
- **DnsComms.cpp**: Truyền dữ liệu ẩn qua DNS

#### 📁 data/

- **DataManager.cpp**: Quản lý lưu trữ dữ liệu thu thập được
- **KeyData.cpp**: Định nghĩa cấu trúc dữ liệu phím
- **SystemData.cpp**: Thu thập thông tin hệ thống
- **Screenshot.cpp**: Chụp và xử lý ảnh màn hình

#### 📁 persistence/

- **PersistenceManager.cpp**: Quản lý cơ chế duy trì hoạt động
- **RegistryPersistence.cpp**: Duy trì qua registry
- **SchedulePersistence.cpp**: Duy trì qua task scheduler
- **ServicePersistence.cpp**: Duy trì qua Windows service

#### 📁 security/

- **Encryption.cpp**: Mã hóa dữ liệu (AES, XOR, SHA256)
- **AntiAnalysis.cpp**: Phát hiện và chống phân tích
- **Obfuscation.cpp**: Làm rối code và string
- **PrivilegeEscalation.cpp**: Leo thang đặc quyền

#### 📁 utils/

- **FileUtils.cpp**: Thao tác với file hệ thống
- **StringUtils.cpp**: Xử lý string và encoding
- **SystemUtils.cpp**: Lấy thông tin hệ thống
- **TimeUtils.cpp**: Xử lý thời gian và timestamp

### 📁 include/

Chứa các file header tương ứng với các file trong src/

### 📁 build_scripts/

- **build.bat**: Build project trên Windows
- **build.sh**: Build trên Linux
- **sign.bat**: Ký file thực thi
- **obfuscate.bat**: Làm rối mã

### 📁 resources/

- **config/default.cfg**: Cấu hình mặc định
- **certificates/dummy_cert.pem**: Certificate mẫu để ký

### 📁 cmake/

- **FindCustomDependencies.cmake**: Tìm các dependency custom

### 📄 CMakeLists.txt

Cấu hình build system cho project
