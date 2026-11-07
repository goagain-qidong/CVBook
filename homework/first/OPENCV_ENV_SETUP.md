# Windows OpenCV C++ 环境配置

已在本机完成 OpenCV 4.14.0 的下载、校验、解压，以及 MSVC x64 的 Release 和 Debug 编译与运行验证。

## 本机环境

| 项目 | 实际配置 |
| --- | --- |
| 工作区 | `D:\CVBook\homework\first` |
| 测试项目 | `D:\CVBook\homework\first\opencv_env_test` |
| MSVC | Visual Studio 2022 Build Tools，编译器版本 19.44.35225 |
| 编译器 | `D:\VS2022\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe` |
| Windows SDK | 10.0.26100.0 |
| CMake | 3.31.6-msvc6 |
| 生成器与架构 | `Visual Studio 17 2022`，`x64` |
| VS Code | 1.117.0 x64 |
| VS Code 扩展 | `ms-vscode.cpptools`、`ms-vscode.cmake-tools` 均已安装 |
| OpenCV | 4.14.0，Windows x64 动态库 |
| 解压目录 | `D:\OpenCV\opencv` |
| OpenCV_DIR | `D:\OpenCV\opencv\build` |
| 头文件 | `D:\OpenCV\opencv\build\include\opencv2\opencv.hpp` |
| 库目录 | `D:\OpenCV\opencv\build\x64\vc16\lib` |
| DLL 目录 | `D:\OpenCV\opencv\build\x64\vc16\bin` |

从 [OpenCV 的 SourceForge 发布目录](https://sourceforge.net/projects/opencvlibrary/files/4.14.0/opencv-4.14.0-windows.exe/download) 下载了 203,030,799 字节的自解压包，原文件保存在 `D:\OpenCV\downloads\opencv-4.14.0-windows.exe`。用 `Get-FileHash -Algorithm SHA256` 得到 `5f266a8b73bed535962d7e861a6457e32a0dd5f463ad0a7cf8707a135469be63`，与发布页一致。随后用 Windows 自带的 `tar.exe` 解压。

发布包实际提供 `vc16` 库，OpenCV 自带的 CMake 配置在 MSVC 2022 下自动选中这套库，本机编译和运行已验证兼容。Release 使用 `opencv_world4140.lib` 和 `opencv_world4140.dll`，Debug 使用带 `d` 后缀的一套。CMake 构建后将对应 DLL 复制到 EXE 旁，运行依赖由项目目录提供。

## 在 VS Code 中使用

用 VS Code 打开 `D:\CVBook\homework\first`。在 CMake Tools 选择配置预设 `msvc-x64` 和构建预设 `release`。C/C++ 扩展通过 CMake Tools 获取项目头文件路径和编译参数。按 Ctrl+Shift+B 构建 Release，或在「运行和调试」选择 `OpenCV (MSVC x64 Release)` 后按 F5。F5 的前置任务会先配置并构建，再启动调试器。工作区任务对应的预设已在命令行验证，VS Code 调试界面未实际启动。

## 编译与运行

在普通 PowerShell 中执行以下命令。Visual Studio 生成器会发现已安装的 Build Tools。

```powershell
Set-Location 'D:\CVBook\homework\first\opencv_env_test'
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\opencv_env_test.exe
$LASTEXITCODE
```

也可以依次执行 `cmake --preset msvc-x64`、`cmake --build --preset release` 和同一条运行命令。需要 Debug 时使用 `cmake --build --preset debug`，生成文件位于 `build\Debug`。

## 实测结果

- Release 和 Debug 均编译、链接并运行成功，退出码均为 0。
- 运行输出 OpenCV 4.14.0、HighGUI WIN32 后端和 500 个 ORB 关键点。
- `cv::invert(..., cv::DECOMP_SVD)` 通过矩阵伪逆回代检查，`cv::undistortPoints` 通过无畸变恒等检查。
- Release 生成 `opencv_env_test\orb_keypoints.png`，尺寸 640 × 480，大小 445,220 字节。程序重新读取 PNG 并核对了全部像素。
- 在仅包含 Windows 系统目录的临时 PATH 下运行成功。二进制检查确认 Release EXE 依赖 `opencv_world4140.dll`，两者均为 x64，复制后的 DLL 与发布包内 DLL 的 SHA256 一致。

## 新建 OpenCV 项目

将下面的 `CMakeLists.txt` 和 `CMakePresets.json` 复制到新项目，修改 `project`、`add_executable` 和相关目标名。保留组件声明和 DLL 复制规则。同一台电脑可直接复用预设中的 `OpenCV_DIR`。如果将新项目作为 VS Code 根目录打开，将 `cmake.sourceDirectory` 改为 `${workspaceFolder}`，并同步修改构建任务工作目录、启动程序路径及目标名称。

## 完整配置与测试源码

### `opencv_env_test\CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.24)
project(OpenCVEnvironmentTest LANGUAGES CXX)

if(NOT MSVC OR NOT CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
    message(FATAL_ERROR "Configure with Visual Studio 17 2022 and -A x64.")
endif()

find_package(OpenCV 4.14.0 EXACT REQUIRED CONFIG
    COMPONENTS core imgproc imgcodecs highgui features2d calib3d
    HINTS "D:/OpenCV/opencv/build")

add_executable(opencv_env_test main.cpp)
target_compile_features(opencv_env_test PRIVATE cxx_std_17)
target_include_directories(opencv_env_test PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(opencv_env_test PRIVATE ${OpenCV_LIBS})

add_custom_command(TARGET opencv_env_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:opencv_env_test>
        $<TARGET_FILE_DIR:opencv_env_test>
    COMMAND_EXPAND_LISTS
    VERBATIM)
```

### `opencv_env_test\CMakePresets.json`

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "msvc-x64",
      "generator": "Visual Studio 17 2022",
      "architecture": "x64",
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "OpenCV_DIR": "D:/OpenCV/opencv/build"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "release",
      "configurePreset": "msvc-x64",
      "configuration": "Release"
    },
    {
      "name": "debug",
      "configurePreset": "msvc-x64",
      "configuration": "Debug"
    }
  ]
}
```

### `opencv_env_test\main.cpp`

```cpp
#include <opencv2/opencv.hpp>

#include <exception>
#include <filesystem>
#include <iostream>
#include <vector>

int main()
{
    try {
        std::cout << "OpenCV " << cv::getVersionString() << '\n';
        std::cout << "HighGUI backend: " << cv::currentUIFramework() << '\n';

        cv::Mat image(480, 640, CV_8UC3, cv::Scalar(30, 25, 20));
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 12; ++col) {
                const cv::Point topLeft(62 + col * 43, 66 + row * 43);
                const cv::Scalar color = (row + col) % 2 == 0
                    ? cv::Scalar(190, 225, 245) : cv::Scalar(55, 100, 150);
                cv::rectangle(image, topLeft, topLeft + cv::Point(29, 29),
                              color, cv::FILLED);
            }
        }
        cv::putText(image, "OpenCV / ORB", {62, 444},
                    cv::FONT_HERSHEY_SIMPLEX, 1.2, {250, 210, 80}, 2, cv::LINE_AA);

        cv::Mat gray, descriptors, rendered;
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
        std::vector<cv::KeyPoint> keypoints;
        cv::ORB::create()->detectAndCompute(gray, cv::noArray(), keypoints, descriptors);
        if (keypoints.empty() || descriptors.empty()) {
            std::cerr << "ORB produced no descriptors.\n";
            return 1;
        }
        cv::drawKeypoints(image, keypoints, rendered, {60, 240, 80},
                          cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

        const cv::Mat matrix = (cv::Mat_<double>(3, 2) << 1, 0, 0, 1, 1, 1);
        cv::Mat pseudoinverse;
        cv::invert(matrix, pseudoinverse, cv::DECOMP_SVD);
        if (cv::norm(matrix * pseudoinverse * matrix - matrix) > 1e-10) {
            std::cerr << "SVD pseudoinverse check failed.\n";
            return 1;
        }

        const std::vector<cv::Point2f> points{{0, 0}, {2, 3}};
        std::vector<cv::Point2f> corrected;
        cv::undistortPoints(points, corrected, cv::Mat::eye(3, 3, CV_64F),
                            cv::Mat::zeros(1, 5, CV_64F));
        if (cv::norm(points, corrected, cv::NORM_INF) > 1e-6) {
            std::cerr << "calib3d identity check failed.\n";
            return 1;
        }

        const char* output = "orb_keypoints.png";
        if (!cv::imwrite(output, rendered)) {
            std::cerr << "Image write failed.\n";
            return 1;
        }
        const cv::Mat saved = cv::imread(output);
        if (saved.size() != rendered.size() || saved.type() != rendered.type() ||
            cv::norm(saved, rendered, cv::NORM_INF) != 0) {
            std::cerr << "PNG round-trip check failed.\n";
            return 1;
        }
        std::cout << "ORB keypoints: " << keypoints.size() << '\n'
                  << "PNG: " << std::filesystem::absolute(output) << '\n'
                  << "SVD, calib3d and PNG round-trip passed.\n"
                  << "OpenCV environment test passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
```

### `.vscode\settings.json`

```json
{
  "cmake.sourceDirectory": "${workspaceFolder}/opencv_env_test",
  "cmake.useCMakePresets": "always",
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
  "C_Cpp.default.intelliSenseMode": "windows-msvc-x64",
  "C_Cpp.default.compilerPath": "D:/VS2022/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe"
}
```

### `.vscode\launch.json`

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "OpenCV (MSVC x64 Release)",
      "type": "cppvsdbg",
      "request": "launch",
      "program": "${workspaceFolder}/opencv_env_test/build/Release/opencv_env_test.exe",
      "cwd": "${workspaceFolder}/opencv_env_test",
      "preLaunchTask": "OpenCV: build Release"
    }
  ]
}
```

### `.vscode\tasks.json`

```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "OpenCV: configure MSVC x64",
      "type": "process",
      "command": "cmake",
      "args": [
        "--preset",
        "msvc-x64"
      ],
      "options": {
        "cwd": "${workspaceFolder}/opencv_env_test"
      },
      "problemMatcher": []
    },
    {
      "label": "OpenCV: build Release",
      "type": "process",
      "command": "cmake",
      "args": [
        "--build",
        "--preset",
        "release"
      ],
      "options": {
        "cwd": "${workspaceFolder}/opencv_env_test"
      },
      "dependsOn": "OpenCV: configure MSVC x64",
      "group": {
        "kind": "build",
        "isDefault": true
      },
      "problemMatcher": [
        "$msCompile"
      ]
    }
  ]
}
```

