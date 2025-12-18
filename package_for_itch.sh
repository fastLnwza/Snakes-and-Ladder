#!/bin/bash

# Script to package the game for itch.io distribution
# Usage: ./package_for_itch.sh [platform]
# Platform options: mac, windows, linux (default: mac)

set -e  # Exit on error

PLATFORM=${1:-mac}
PROJECT_NAME="SnakesAndLadders3D"
VERSION="1.0.0"
BUILD_DIR="build"
PACKAGE_DIR="itch_package_${PLATFORM}"

echo "🎮 Packaging ${PROJECT_NAME} for ${PLATFORM}..."

# Clean previous package
if [ -d "$PACKAGE_DIR" ]; then
    echo "Cleaning previous package..."
    rm -rf "$PACKAGE_DIR"
fi

mkdir -p "$PACKAGE_DIR"

# Build the project first
echo "📦 Building project..."
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --config Release

if [ "$PLATFORM" = "mac" ]; then
    echo "🍎 Packaging for macOS..."
    
    # Create .app bundle structure
    APP_NAME="${PROJECT_NAME}.app"
    APP_DIR="${PACKAGE_DIR}/${APP_NAME}"
    CONTENTS_DIR="${APP_DIR}/Contents"
    MACOS_DIR="${CONTENTS_DIR}/MacOS"
    RESOURCES_DIR="${CONTENTS_DIR}/Resources"
    
    mkdir -p "$MACOS_DIR"
    mkdir -p "$RESOURCES_DIR"
    
    # Copy executable
    EXECUTABLE="${BUILD_DIR}/SnakesAndLadder"
    if [ ! -f "$EXECUTABLE" ]; then
        echo "❌ Error: Executable not found at $EXECUTABLE"
        echo "   Make sure the project is built successfully."
        exit 1
    fi
    
    cp "$EXECUTABLE" "$MACOS_DIR/${PROJECT_NAME}"
    chmod +x "$MACOS_DIR/${PROJECT_NAME}"
    
    # Copy shaders
    if [ -d "${BUILD_DIR}/shaders" ]; then
        cp -r "${BUILD_DIR}/shaders" "$RESOURCES_DIR/"
    else
        cp -r "shaders" "$RESOURCES_DIR/"
    fi
    
    # Copy assets
    if [ -d "assets" ]; then
        cp -r "assets" "$RESOURCES_DIR/"
    fi
    
    # Copy font if it exists in build
    if [ -f "${BUILD_DIR}/pixel-game.regular.otf" ]; then
        cp "${BUILD_DIR}/pixel-game.regular.otf" "$RESOURCES_DIR/"
    elif [ -f "assets/fonts/pixel-game.regular.otf" ]; then
        cp "assets/fonts/pixel-game.regular.otf" "$RESOURCES_DIR/"
    fi
    
    # Create Info.plist for macOS app
    cat > "${CONTENTS_DIR}/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>${PROJECT_NAME}</string>
    <key>CFBundleIdentifier</key>
    <string>com.yourname.${PROJECT_NAME}</string>
    <key>CFBundleName</key>
    <string>${PROJECT_NAME}</string>
    <key>CFBundleVersion</key>
    <string>${VERSION}</string>
    <key>CFBundleShortVersionString</key>
    <string>${VERSION}</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.13</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF
    
    # Copy required dynamic libraries (SDL2_mixer, etc.)
    # Check if SDL2_mixer is linked dynamically
    if otool -L "$MACOS_DIR/${PROJECT_NAME}" | grep -q "SDL2"; then
        echo "📚 Copying SDL2 libraries..."
        SDL2_PATH=$(otool -L "$MACOS_DIR/${PROJECT_NAME}" | grep "SDL2" | head -1 | awk '{print $1}' | xargs dirname)
        
        # Try common Homebrew locations
        for BREW_PREFIX in /opt/homebrew /usr/local; do
            if [ -d "${BREW_PREFIX}/lib" ]; then
                for lib in libSDL2.dylib libSDL2_mixer.dylib; do
                    if [ -f "${BREW_PREFIX}/lib/${lib}" ]; then
                        cp "${BREW_PREFIX}/lib/${lib}" "$MACOS_DIR/"
                        # Fix library paths
                        install_name_tool -change "${BREW_PREFIX}/lib/${lib}" "@executable_path/${lib}" "$MACOS_DIR/${PROJECT_NAME}" 2>/dev/null || true
                    fi
                done
            fi
        done
    fi
    
    # Create README for users
    cat > "${PACKAGE_DIR}/README.txt" << EOF
Snakes and Ladders 3D - macOS

HOW TO RUN:
1. Double-click ${APP_NAME} to launch the game
2. If you see a security warning, right-click the app and select "Open"
3. You may need to allow the app in System Preferences > Security & Privacy

CONTROLS:
- Space: Roll dice / Confirm actions
- +/-: Adjust volume
- T: Debug teleport mode
- 0-9 + Enter: Warp to tile (debug mode)

SYSTEM REQUIREMENTS:
- macOS 10.13 or later
- OpenGL 3.3+ compatible graphics card

Enjoy the game!
EOF
    
    echo "✅ macOS package created at: ${PACKAGE_DIR}/${APP_NAME}"
    echo "📦 You can zip this folder for upload to itch.io"
    
elif [ "$PLATFORM" = "windows" ]; then
    echo "🪟 Packaging for Windows..."
    
    # Windows executable
    EXECUTABLE="${BUILD_DIR}/SnakesAndLadder.exe"
    if [ ! -f "$EXECUTABLE" ]; then
        echo "❌ Error: Windows executable not found. Build on Windows first."
        exit 1
    fi
    
    cp "$EXECUTABLE" "$PACKAGE_DIR/"
    
    # Copy shaders
    if [ -d "${BUILD_DIR}/shaders" ]; then
        cp -r "${BUILD_DIR}/shaders" "$PACKAGE_DIR/"
    else
        cp -r "shaders" "$PACKAGE_DIR/"
    fi
    
    # Copy assets
    if [ -d "assets" ]; then
        cp -r "assets" "$PACKAGE_DIR/"
    fi
    
    # Copy DLLs if needed
    echo "⚠️  Note: You may need to copy required DLLs (SDL2.dll, SDL2_mixer.dll, etc.)"
    echo "   Check your build directory for DLL files."
    
    # Create README
    cat > "${PACKAGE_DIR}/README.txt" << EOF
Snakes and Ladders 3D - Windows

HOW TO RUN:
1. Double-click SnakesAndLadder.exe to launch the game
2. If Windows Defender blocks it, click "More info" then "Run anyway"

CONTROLS:
- Space: Roll dice / Confirm actions
- +/-: Adjust volume
- T: Debug teleport mode
- 0-9 + Enter: Warp to tile (debug mode)

SYSTEM REQUIREMENTS:
- Windows 10 or later
- OpenGL 3.3+ compatible graphics card

Enjoy the game!
EOF
    
    echo "✅ Windows package created at: ${PACKAGE_DIR}/"
    
elif [ "$PLATFORM" = "linux" ]; then
    echo "🐧 Packaging for Linux..."
    
    EXECUTABLE="${BUILD_DIR}/SnakesAndLadder"
    if [ ! -f "$EXECUTABLE" ]; then
        echo "❌ Error: Executable not found at $EXECUTABLE"
        exit 1
    fi
    
    cp "$EXECUTABLE" "$PACKAGE_DIR/"
    chmod +x "$PACKAGE_DIR/SnakesAndLadder"
    
    # Copy shaders
    if [ -d "${BUILD_DIR}/shaders" ]; then
        cp -r "${BUILD_DIR}/shaders" "$PACKAGE_DIR/"
    else
        cp -r "shaders" "$PACKAGE_DIR/"
    fi
    
    # Copy assets
    if [ -d "assets" ]; then
        cp -r "assets" "$PACKAGE_DIR/"
    fi
    
    # Create run script
    cat > "${PACKAGE_DIR}/run.sh" << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"
./SnakesAndLadder
EOF
    chmod +x "${PACKAGE_DIR}/run.sh"
    
    # Create README
    cat > "${PACKAGE_DIR}/README.txt" << EOF
Snakes and Ladders 3D - Linux

HOW TO RUN:
1. Make sure you have required libraries installed:
   sudo apt-get install libgl1-mesa-glx libglfw3 libsdl2-mixer-2.0-0
   (or equivalent for your distribution)
2. Run: ./run.sh
   Or: ./SnakesAndLadder

CONTROLS:
- Space: Roll dice / Confirm actions
- +/-: Adjust volume
- T: Debug teleport mode
- 0-9 + Enter: Warp to tile (debug mode)

SYSTEM REQUIREMENTS:
- Linux with OpenGL 3.3+ support
- Required libraries: GLFW, SDL2_mixer, OpenGL

Enjoy the game!
EOF
    
    echo "✅ Linux package created at: ${PACKAGE_DIR}/"
fi

# Create zip file
ZIP_NAME="${PROJECT_NAME}_${PLATFORM}_v${VERSION}.zip"
cd "$PACKAGE_DIR"
if [ "$PLATFORM" = "mac" ]; then
    zip -r "../${ZIP_NAME}" "${APP_NAME}" README.txt
else
    zip -r "../${ZIP_NAME}" .
fi
cd ..

echo ""
echo "🎉 Package complete!"
echo "📦 Zip file: ${ZIP_NAME}"
echo ""
echo "📤 Ready to upload to itch.io!"
echo "   File size: $(du -h "${ZIP_NAME}" | cut -f1)"

