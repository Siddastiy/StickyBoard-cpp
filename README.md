# StickyBoard-cpp
StickyBoard but c++
# Building Requirements:
C++ Compiler supporting C++ 17 or newer

Cmake

Vcpkg

Git

# Build Instructions:
Clone the repository:
git clone 
CD into the newly cloned directory
cd ./StickyBoard-cpp
Install dependencies with Vcpkg
vcpkg install
Get the vcpkg cmake toolchain file path
vcpkg integrate install
This will output something like : CMake projects should use: "-DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
Create a build directory
mkdir build
Configure project with CMake
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
Use the vcpkg cmake toolchain path from above
Build the project
cmake --build build
Run the program
The executables will be located in the build folder
