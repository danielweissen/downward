set(CMAKE_C_COMPILER "/scicore/soft/apps/GCCcore/8.3.0/bin/gcc")
set(CMAKE_C_COMPILER_ARG1 "")
set(CMAKE_C_COMPILER_ID "GNU")
set(CMAKE_C_COMPILER_VERSION "8.3.0")
set(CMAKE_C_COMPILER_VERSION_INTERNAL "")
set(CMAKE_C_COMPILER_WRAPPER "")
set(CMAKE_C_STANDARD_COMPUTED_DEFAULT "11")
set(CMAKE_C_COMPILE_FEATURES "c_std_90;c_function_prototypes;c_std_99;c_restrict;c_variadic_macros;c_std_11;c_static_assert")
set(CMAKE_C90_COMPILE_FEATURES "c_std_90;c_function_prototypes")
set(CMAKE_C99_COMPILE_FEATURES "c_std_99;c_restrict;c_variadic_macros")
set(CMAKE_C11_COMPILE_FEATURES "c_std_11;c_static_assert")

set(CMAKE_C_PLATFORM_ID "Linux")
set(CMAKE_C_SIMULATE_ID "")
set(CMAKE_C_COMPILER_FRONTEND_VARIANT "")
set(CMAKE_C_SIMULATE_VERSION "")



set(CMAKE_AR "/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/bin/ar")
set(CMAKE_C_COMPILER_AR "/scicore/soft/apps/GCCcore/8.3.0/bin/gcc-ar")
set(CMAKE_RANLIB "/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/bin/ranlib")
set(CMAKE_C_COMPILER_RANLIB "/scicore/soft/apps/GCCcore/8.3.0/bin/gcc-ranlib")
set(CMAKE_LINKER "/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/bin/ld")
set(CMAKE_MT "")
set(CMAKE_COMPILER_IS_GNUCC 1)
set(CMAKE_C_COMPILER_LOADED 1)
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_C_ABI_COMPILED TRUE)
set(CMAKE_COMPILER_IS_MINGW )
set(CMAKE_COMPILER_IS_CYGWIN )
if(CMAKE_COMPILER_IS_CYGWIN)
  set(CYGWIN 1)
  set(UNIX 1)
endif()

set(CMAKE_C_COMPILER_ENV_VAR "CC")

if(CMAKE_COMPILER_IS_MINGW)
  set(MINGW 1)
endif()
set(CMAKE_C_COMPILER_ID_RUN 1)
set(CMAKE_C_SOURCE_FILE_EXTENSIONS c;m)
set(CMAKE_C_IGNORE_EXTENSIONS h;H;o;O;obj;OBJ;def;DEF;rc;RC)
set(CMAKE_C_LINKER_PREFERENCE 10)

# Save compiler ABI information.
set(CMAKE_C_SIZEOF_DATA_PTR "8")
set(CMAKE_C_COMPILER_ABI "ELF")
set(CMAKE_C_LIBRARY_ARCHITECTURE "")

if(CMAKE_C_SIZEOF_DATA_PTR)
  set(CMAKE_SIZEOF_VOID_P "${CMAKE_C_SIZEOF_DATA_PTR}")
endif()

if(CMAKE_C_COMPILER_ABI)
  set(CMAKE_INTERNAL_PLATFORM_ABI "${CMAKE_C_COMPILER_ABI}")
endif()

if(CMAKE_C_LIBRARY_ARCHITECTURE)
  set(CMAKE_LIBRARY_ARCHITECTURE "")
endif()

set(CMAKE_C_CL_SHOWINCLUDES_PREFIX "")
if(CMAKE_C_CL_SHOWINCLUDES_PREFIX)
  set(CMAKE_CL_SHOWINCLUDES_PREFIX "${CMAKE_C_CL_SHOWINCLUDES_PREFIX}")
endif()





set(CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES "/scicore/soft/apps/cURL/7.66.0-GCCcore-8.3.0/include;/scicore/soft/apps/Python/3.7.4-GCCcore-8.3.0/include;/scicore/soft/apps/libffi/3.2.1-GCCcore-8.3.0/include;/scicore/soft/apps/GMP/6.1.2-GCCcore-8.3.0/include;/scicore/soft/apps/XZ/5.2.4-GCCcore-8.3.0/include;/scicore/soft/apps/SQLite/3.29.0-GCCcore-8.3.0/include;/scicore/soft/apps/Tcl/8.6.9-GCCcore-8.3.0/include;/scicore/soft/apps/libreadline/8.0-GCCcore-8.3.0/include;/scicore/soft/apps/ncurses/6.1-GCCcore-8.3.0/include;/scicore/soft/apps/bzip2/1.0.8-GCCcore-8.3.0/include;/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/include;/scicore/soft/apps/zlib/1.2.11-GCCcore-8.3.0/include;/scicore/soft/apps/GCCcore/8.3.0/lib/gcc/x86_64-pc-linux-gnu/8.3.0/include;/scicore/soft/apps/GCCcore/8.3.0/include;/scicore/soft/apps/GCCcore/8.3.0/lib/gcc/x86_64-pc-linux-gnu/8.3.0/include-fixed;/usr/include")
set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "gcc;gcc_s;c;gcc;gcc_s")
set(CMAKE_C_IMPLICIT_LINK_DIRECTORIES "/scicore/soft/apps/libffi/3.2.1-GCCcore-8.3.0/lib64;/scicore/soft/apps/GCCcore/8.3.0/lib/gcc/x86_64-pc-linux-gnu/8.3.0;/scicore/soft/apps/GCCcore/8.3.0/lib64;/lib64;/usr/lib64;/scicore/soft/apps/cURL/7.66.0-GCCcore-8.3.0/lib;/scicore/soft/apps/Python/3.7.4-GCCcore-8.3.0/lib;/scicore/soft/apps/libffi/3.2.1-GCCcore-8.3.0/lib;/scicore/soft/apps/GMP/6.1.2-GCCcore-8.3.0/lib;/scicore/soft/apps/XZ/5.2.4-GCCcore-8.3.0/lib;/scicore/soft/apps/SQLite/3.29.0-GCCcore-8.3.0/lib;/scicore/soft/apps/Tcl/8.6.9-GCCcore-8.3.0/lib;/scicore/soft/apps/libreadline/8.0-GCCcore-8.3.0/lib;/scicore/soft/apps/ncurses/6.1-GCCcore-8.3.0/lib;/scicore/soft/apps/bzip2/1.0.8-GCCcore-8.3.0/lib;/scicore/soft/apps/binutils/2.32-GCCcore-8.3.0/lib;/scicore/soft/apps/zlib/1.2.11-GCCcore-8.3.0/lib;/scicore/soft/apps/GCCcore/8.3.0/lib")
set(CMAKE_C_IMPLICIT_LINK_FRAMEWORK_DIRECTORIES "")
