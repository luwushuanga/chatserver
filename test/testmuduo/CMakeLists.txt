
# 配置头文件搜索路径
include_directories(/usr/include)
# 配置库文件搜索路径
link_directories(/usr/lib)

# 设置需要编译的源文件列表
set(SRC_LIST test.cpp)

# 设置可执行文件最终存储路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)


# 把.指定路径下的所有源文件名字放入变量名SRC_LIST里面
# aux_source_directory(. SRC_LIST)

# 生成可执行文件  由SRC_LIST变量所定义的源文件编译而来
add_executable(server ${SRC_LIST}) 

# server 这个目标程序需要连链接的库
target_link_libraries(server muduo_net muduo_base pthread)

# # 指定搜索的子目录
# add_subdirectory(testmuduo)