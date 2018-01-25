# Windows cross-compile toolchain file
# cmake -DCMAKE_TOOLCHAIN_FILE=../Windows-toolchain.cmake ..

#target OS
set ( CMAKE_SYSTEM_NAME Windows )
 
# specify the cross compiler
set ( CMAKE_C_COMPILER x86_64-w64-mingw32-gcc )
set ( CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++ )

# Location of target environment
SET ( CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32/sys-root/mingw/lib )

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set ( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )
set ( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set ( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
