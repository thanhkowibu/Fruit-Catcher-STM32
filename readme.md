# 🍎🍌🍇FruitCatcher STM32 Project🍎🍌🍇

## Mô tả dự án
FruitCatcher là một game tương tác được phát triển trên vi điều khiển STM32F429I-DISC1 sử dụng TouchGFX framework. Người chơi sẽ điều khiển để bắt trái cây rơi xuống với giao diện đồ họa sinh động.

## Phần cứng yêu cầu

### Bo mạch chính
- **STM32F429I-DISC1**: Kit phát triển STM32F429ZI với màn hình LCD cảm ứng 2.4 inch

### Phần cứng mở rộng
- **Mạch nút bấm**: 2 nút bấm điều khiển game
  - Nút 1: Nối với chân **PG2** và **GND**
  - Nút 2: Nối với chân **PG3** và **GND**
  
- **Buzzer**: Tạo âm thanh cho game
  - Buzzer 5V nối với chân **PA9** và **GND**

## Phần mềm yêu cầu

### Môi trường phát triển
- **STM32CubeIDE**: Version 1.17.0
- **TouchGFX Designer**: Version 4.25.0

### Hệ điều hành hỗ trợ
- Windows 10/11
- Linux Ubuntu 18.04 LTS trở lên
- macOS 10.15 trở lên

## Cách cài đặt và chạy project

### Bước 1: Chuẩn bị môi trường
1. Tải và cài đặt **STM32CubeIDE 1.17.0** từ trang chủ ST
2. Tải và cài đặt **TouchGFX Designer 4.25.0** (nếu cần chỉnh sửa giao diện)

### Bước 2: Clone project
```bash
git clone https://github.com/thanhkowibu/Fruit-Catcher-STM32
```

### Bước 3: Import project vào STM32CubeIDE
1. Vào thư mục `Fruit-Catcher-STM32`
4. **Double click** vào file `.project` để import project vào IDE
5. Chờ IDE load project hoàn tất

### Bước 4: Clean project
1. Trong STM32CubeIDE, chọn menu **Project** > **Clean**
2. Chọn **Clean** để xóa các file build cũ
3. Chờ quá trình clean hoàn tất

### Bước 5: Cấu hình Run Configuration
1. Chọn menu **Run** > **Run Configurations**
2. Trong tab **STM32 C/C++ Application**, chọn **STM32F429I_DISCO_REV_D01**
3. Chuyển sang tab **Debugger**
4. Tick vào ô **ST-LINK S/N**
5. Nhấn nút **Scan** để tìm ST-LINK
6. Nhấn **Apply** để lưu cấu hình

### Bước 6: Kết nối phần cứng
1. Kết nối mạch nút bấm theo sơ đồ:
   - Nút trái: PG2 → GND
   - Nút phải: PG3 → GND
2. Kết nối buzzer:
   - Buzzer (+): PA9
   - Buzzer (-): GND
3. Kết nối bo mạch STM32F429I-DISC1 với máy tính qua USB

### Bước 7: Chạy project
1. Nhấn nút **Run** hoặc nhấn **F11**
2. Chờ quá trình build và nạp code vào bo mạch
3. Game sẽ tự động khởi chạy trên màn hình LCD

## Cấu trúc project

```
FruitCatcher/
├── Core/                    # Core STM32 files
│   ├── Inc/                # Header files
│   ├── Src/                # Source files
│   └── Startup/            # Startup files
├── TouchGFX/               # TouchGFX application
│   ├── App/                # Application layer
│   ├── assets/             # Images, fonts, texts
│   ├── generated/          # Generated code
│   └── gui/                # GUI implementation
├── Drivers/                # HAL drivers
├── Middlewares/           # Third-party libraries
└── STM32CubeIDE/          # IDE project files
```

## Cách chơi game

1. **Khởi động**: Game sẽ hiển thị màn hình chính
2. **Điều khiển**: 
   - Nút PG2: Di chuyển trái
   - Nút PG3: Di chuyển phải
3. **Mục tiêu**: Bắt những trái cây rơi xuống để ghi điểm
4. **Âm thanh**: Buzzer sẽ phát ra âm thanh khi bắt được trái cây
