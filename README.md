# Order Independent Transparency

<a name="cloning"></a>
# Cloning the Repository

To get the repository and all submodules, use the following command:

```
git clone --recursive https://github.com/MikhailGorobets/OrderIndependentTransparency
```

When updating existing repository, don't forget to update all submodules:

```
git pull
git submodule update --init --recursive
```

<a name="build_and_run"></a>
# Build and Run Instructions

<a name="build_and_run_win32"></a>
## Win32

Build prerequisites:

* Windows SDK 10.0.19041.0 or later
* C++ build tools

Use either CMake GUI or command line tool to generate build files. For example, to generate
[Visual Studio 2019](https://www.visualstudio.com/vs/community) 64-bit solution and project files in *build/Win64* folder,
navigate to the engine's root folder and run the following command:

```
cmake -S . -B ./build/Win64 -G "Visual Studio 16 2019" -A x64
```
