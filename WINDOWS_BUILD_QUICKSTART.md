# 🪟 Windows Build - Quick Start

## วิธีที่ง่ายที่สุด: ใช้ GitHub Actions

### 1. สร้าง Release Tag
```bash
git tag v1.0.0
git push origin v1.0.0
```

### 2. หรือใช้ Manual Workflow
1. ไปที่ GitHub repository: https://github.com/fastLnwza/Pacman-OpenGL
2. คลิก **Actions** tab
3. เลือก **Build and Package for itch.io**
4. คลิก **Run workflow**
5. เลือก platform: **windows** หรือ **all**
6. คลิก **Run workflow**

### 3. ดาวน์โหลด Build
- รอ workflow เสร็จ (ประมาณ 5-10 นาที)
- คลิก workflow run ที่เสร็จแล้ว
- ดาวน์โหลด artifact **windows-build**
- จะได้ไฟล์ `SnakesAndLadders3D_windows_v1.0.0.zip`

### 4. อัปโหลดไปยัง itch.io
- ไปที่ itch.io game page
- คลิก **New upload**
- เลือก platform: **Windows**
- อัปโหลดไฟล์ ZIP
- ตั้งค่า architecture: **64-bit (x86_64)**
- บันทึกและเผยแพร่

---

## วิธีอื่นๆ

ดูรายละเอียดเพิ่มเติมใน [BUILD_WINDOWS.md](BUILD_WINDOWS.md)

---

**หมายเหตุ:** GitHub Actions จะ build บน Windows อัตโนมัติ ไม่ต้องมี Windows machine!

