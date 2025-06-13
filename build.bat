@echo off
REM Qt的编译可以参考以下网页信息
REM https://wiki.qt.io/Building_Qt_5_from_Git
REM https://wiki.qt.io/Building_Qt_Documentation

REM //////////////////////////////////////////////////////////////////////////////////////////////////////
REM 在src的同级目录下打开cmd，并运行build.bat文件即可完成编译
REM //////////////////////////////////////////////////////////////////////////////////////////////////////

REM //////////////////////////////////////////////////////////////////////////////////////////////////////
REM 1、初始化环境变量和配置Qt的编译选项
REM 2、运行命令nmake进行编译
REM 3、运行命令nmake install进行安装
REM 4、运行命令nmake docs编译文档
REM 5、运行命令nmake install_docs安装文档
REM //////////////////////////////////////////////////////////////////////////////////////////////////////

REM 传进来的第一个参数是源码的根目录，比如E:\qt-master
set _root=%cd%

REM 设置 Microsoft Visual Studio 2019的环境变量，注意这里的路径为你电脑上VS的安装目录
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64

REM 由于vcvarsall.bat是通过注册表来确定Windows SDK版本的，如果当前用户没权限访问注册表，则可能初始化Windows SDK失败
REM 通过set命令看看，INCLUDE，LIB等待变量是否已经包含以下Windows SDK的相关路径，如果没有，则vcvarsall.bat初始化Windows SDK失败
REM 可以使用以下命令手动添加这些目录完成Windows SDK的初始化
REM SET INCLUDE=%INCLUDE%;C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\um
REM SET INCLUDE=%INCLUDE%;C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\cppwinrt
REM SET INCLUDE=%INCLUDE%;C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\shared
REM SET INCLUDE=%INCLUDE%;C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\ucrt
REM SET INCLUDE=%INCLUDE%;C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\winrt
REM SET LIB=%LIB%;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\ucrt\x64
REM SET LIB=%LIB%;C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\um\x64
REM SET PATH=%PATH%;C:\Program Files (x86)\Windows Kits\10\bin\10.0.17763.0\x64
REM SET PATH=%PATH%;E:\qt-master\src\qtbase\bin;E:\qt-master\src\gnuwin32\bin

REM 设置clang以便编译文档
SET PATH=%_root%\tools\libclang-release_140-based-windows-vs2019_64\libclang\bin;%path%
REM SET PATH=%_root%\tools\clang+llvm-18.1.8-x86_64-pc-windows-msvc\bin;%path%
REM SET LLVM_INSTALL_DIR=%_root%\tools\clang+llvm-18.1.8-x86_64-pc-windows-msvc

REM 设置Qt编译时需要依赖的一些工具的路径，比如bison，flex
SET PATH=%_root%\src\qtbase\bin;%_root%\src\gnuwin32\bin;%path%

REM 如果需要向Qt提交代码，需要使用Qt开发的一些git工具，可以打开下面的注释，添加相关工具的路径
REM SET path=%_ROOT%\qtrepotools\bin;%PATH%

REM 创建tmp目录，并进入tmp 目录，以便进行out of source编译
set tmp_dir=%_root%\tmp
if not exist %tmp_dir% (
    mkdir %tmp_dir%
	cd %tmp_dir%
)else (
    cd %tmp_dir% 
)

REM 配置Qt，可以参考这个网页https://felgo.com/doc/qt/configure-options/
REM -prefix %_root%\bin\5.15.2\msvc2019_64，安装目录为ROOT下的bin\5.15.2\msvc2019_64
REM -opensource -confirm-license，使用开源协议
REM -debug-and-release，-force-debug-info，编译debug版和release版，并且release版带有调试信息
REM -opengl dynamic，动态加载OpenGL
REM 带有openssl
REM -openssl -openssl-linked -I E:\Tools\vcpkg\installed\x64-windows\include -L E:\Tools\vcpkg\installed\x64-windows\lib，编译对SSL的支持，network模块的https功能需要使用SSL
REM -qt-sqlite -qt-pcre -qt-zlib -qt-libpng -qt-libjpeg -qt-freetype -qt-harfbuzz， 这些库使用Qt源码中自带的，不要使用当前操作系统的
REM -skip qtconnectivity，不编译qtconnectivity目录下的模块
REM -skip qtwebengine，不编译qtwebengine目录下的模块
REM -nomake tests -nomake examples，不编译tests和examples
call "%_root%\src\configure.bat" -prefix %_root%\bin\5.15.2\msvc2019_64 -opensource -confirm-license -debug-and-release -force-debug-info -mp -optimize-size -strip -opengl dynamic -openssl -openssl-linked -I E:\Tools\vcpkg\installed\x64-windows\include -L E:\Tools\vcpkg\installed\x64-windows\lib -skip qtconnectivity -skip qtwebengine -nomake tests -nomake examples -qt-sqlite -qt-pcre -qt-zlib -qt-libpng -qt-libjpeg -qt-freetype -qt-harfbuzz

REM 编译Qt
nmake

REM 安装Qt
nmake install

REM 复制ssl的dll文件
copy /Y "E:\Tools\vcpkg\installed\x64-windows\bin\libcrypto-3-x64.dll" "%_root%\bin\5.15.2\msvc2019_64\bin\libcrypto-3-x64.dll"
copy /Y "E:\Tools\vcpkg\installed\x64-windows\bin\libssl-3-x64.dll" "%_root%\bin\5.15.2\msvc2019_64\bin\libssl-3-x64.dll"

REM 编译文档
nmake docs

REM 安装文档
nmake install_docs