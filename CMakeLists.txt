cmake_minimum_required(VERSION 3.4.1)

project(main)

#检索当前目录下的所有源文件
aux_source_directory(./ all_source)

message("all_souece" ${all_source})

#设置库文件的搜索路径
set(SYSROOT_PATH  /usr/local/arm/1808-sdk-v1.3.1/host/aarch64-buildroot-linux-gnu/sysroot)
set(CMAKE_SYSROOT "${SYSROOT_PATH}")

#设置编译器 
#set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc")
set(CMAKE_CXX_COMPILER "aarch64-linux-gnu-g++")

#添加头文件 
include_directories(/usr/local/arm/1808-sdk/host/aarch64-buildroot-linux-gnu/sysroot/usr/include/drm/)
include_directories(${PROJECT_SOURCE_DIR}/inc/)
include_directories(/mnt/c/Users/Administrator/Desktop/RK1808/已完成项目/RK1808车牌识别_END/rk1808_show_bmp/rockx-rk1808-Linux/include)
include_directories(/home/gec/sql_arm/include)

add_executable(${PROJECT_NAME} ${all_source})

#查找库
find_library(RKNN NAMES librknn_api.so PATHS ${PROJECT_SOURCE_DIR}/rockx-rk1808-Linux/lib64 NO_DEFAULT_PATH)
message("all_souece" ${RKNN})

find_library(DRM NAMES libdrm.so)
message("all_souece" ${DRM})

find_library(ROCKX NAMES librockx.so PATHS ${PROJECT_SOURCE_DIR}/rockx-rk1808-Linux/lib64 NO_DEFAULT_PATH)
message("all_souece" ${ROCKX})

#设置库
set(SQL_API_LIB /home/gec/sql_arm/lib/libsqlite3.so)
message("all_souece" ${SQL_API_LIB})

set(DRM_API_LIB ${PROJECT_SOURCE_DIR}/lib/libDRMwrap.so)
message("all_souece" ${DRM_API_LIB})

target_link_libraries(main ${RKNN} ${DRM} ${ROCKX} ${SQL_API_LIB} ${DRM_API_LIB})

#指定编译器
#cmake .. -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc  -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++