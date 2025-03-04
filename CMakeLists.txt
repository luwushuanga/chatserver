cmake_minimum_required(VERSION 3.0)  # cmake默认的最低版本
project(chat)  # 工程名

# 配置编译选项 # 配置编译选项，启用 C++11 支持
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++11)

# #指定搜索子目录  在子目录中寻找CMakelists
# add_subdirectory(testmuduo)

# 配置最终可执行文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/redis)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)


# 加载子目录
add_subdirectory(src)