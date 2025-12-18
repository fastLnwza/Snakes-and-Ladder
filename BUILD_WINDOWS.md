# คู่มือการ Build สำหรับ Windows

## วิธีที่ 1: ใช้ GitHub Actions (แนะนำ - ไม่ต้องมี Windows)

วิธีนี้จะ build บน Windows อัตโนมัติผ่าน GitHub Actions

### ขั้นตอน:

1. **Push code ไปยัง GitHub:**
   ```bash
   git add .
   git commit -m "Add Windows build support"
   git push origin main
   ```

2. **สร้าง Tag สำหรับ Release:**
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

3. **หรือใช้ Manual Workflow:**
   - ไปที่ GitHub repository
   - คลิก "Actions" tab
   - เลือก "Build and Package for itch.io"
   - คลิก "Run workflow"
   - เลือก platform: "windows" หรือ "all"
   - คลิก "Run workflow"

4. **ดาวน์โหลด Build:**
   - ไปที่ "Actions" tab
   - คลิก workflow run ที่เสร็จแล้ว
   - ดาวน์โหลด artifact "windows-build"

## วิธีที่ 2: Build บน Windows โดยตรง

### Prerequisites:

1. **Visual Studio 2022** (Community Edition ฟรี)
   - ดาวน์โหลด: https://visualstudio.microsoft.com/downloads/
   - ติดตั้ง "Desktop development with C++" workload

2. **CMake** 3.20 หรือสูงกว่า
   - ดาวน์โหลด: https://cmake.org/download/
   - หรือใช้ `winget install Kitware.CMake`

3. **SDL2_mixer** (สำหรับ audio)
   - ใช้ vcpkg: `vcpkg install sdl2-mixer:x64-windows`
   - หรือดาวน์โหลดจาก: https://www.libsdl.org/projects/SDL_mixer/

### Build Steps:

1. **เปิด Command Prompt หรือ PowerShell**

2. **Clone repository (ถ้ายังไม่มี):**
   ```bash
   git clone https://github.com/fastLnwza/Snakes-and-Ladder.git
   cd Snakes-and-Ladder
   ```

3. **Configure CMake:**
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   ```

4. **Build:**
   ```bash
   cmake --build build --config Release
   ```

5. **Package สำหรับ itch.io:**
   ```bash
   # สร้างโฟลเดอร์ package
   mkdir itch_package_windows
   
   # Copy executable
   copy build\Release\SnakesAndLadder.exe itch_package_windows\
   
   # Copy shaders
   xcopy /E /I shaders itch_package_windows\shaders
   
   # Copy assets
   xcopy /E /I assets itch_package_windows\assets
   
   # Copy font
   if exist build\pixel-game.regular.otf copy build\pixel-game.regular.otf itch_package_windows\
   if exist assets\fonts\pixel-game.regular.otf copy assets\fonts\pixel-game.regular.otf itch_package_windows\
   
   # Copy DLLs (ถ้ามี)
   # SDL2.dll และ SDL2_mixer.dll อาจต้อง copy จาก vcpkg หรือ SDL2 installation
   ```

6. **สร้าง ZIP:**
   ```powershell
   Compress-Archive -Path itch_package_windows\* -DestinationPath SnakesAndLadders3D_windows_v1.0.0.zip
   ```

## วิธีที่ 3: ใช้ Cross-Compilation จาก Mac (ขั้นสูง)

วิธีนี้ซับซ้อนกว่า ต้องติดตั้ง MinGW-w64:

```bash
# ติดตั้ง MinGW-w64 (ใช้ Homebrew)
brew install mingw-w64

# Configure สำหรับ Windows
cmake -S . -B build-windows \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-windows
```

**หมายเหตุ:** Cross-compilation อาจมีปัญหาเรื่อง dependencies (SDL2_mixer, etc.) แนะนำให้ใช้ GitHub Actions แทน

## สิ่งที่ต้องรวมใน Package

- ✅ `SnakesAndLadder.exe` (executable)
- ✅ `shaders/` (โฟลเดอร์ทั้งหมด)
- ✅ `assets/` (โฟลเดอร์ทั้งหมด)
- ✅ `pixel-game.regular.otf` (font file)
- ⚠️ `SDL2.dll` และ `SDL2_mixer.dll` (ถ้าไม่ได้ static link)

## การแก้ปัญหา

### ปัญหา: หา DLL ไม่เจอ
**แก้ไข:** Copy DLL files จาก:
- vcpkg: `C:\tools\vcpkg\installed\x64-windows\bin\`
- หรือ SDL2 installation directory

### ปัญหา: หา assets ไม่เจอ
**แก้ไข:** ตรวจสอบว่า executable อยู่ในโฟลเดอร์เดียวกับ `assets/` และ `shaders/`

### ปัญหา: Build error
**แก้ไข:** 
- ตรวจสอบว่า Visual Studio 2022 ติดตั้งครบ
- ตรวจสอบว่า CMake version 3.20+
- ตรวจสอบว่า SDL2_mixer ติดตั้งถูกต้อง

## อัปโหลดไปยัง itch.io

1. ไปที่ itch.io game page
2. คลิก "New upload"
3. เลือก platform: "Windows"
4. อัปโหลดไฟล์ ZIP
5. ตั้งค่า architecture: "64-bit (x86_64)"
6. บันทึกและเผยแพร่

---

**คำแนะนำ:** ใช้ GitHub Actions (วิธีที่ 1) จะง่ายและสะดวกที่สุด ไม่ต้องมี Windows machine!

