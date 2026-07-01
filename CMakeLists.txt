cmake_minimum_required(VERSION 3.15)
project(SuperMarioBros VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Đầu ra của file thực thi (.exe) sẽ nằm trong thư mục bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Tìm và tích hợp thư viện SFML (Yêu cầu các thành viên phải cài sẵn SFML trên máy)
find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

# Gom tất cả các file .hpp trong thư mục include
include_directories(include)

# Gom tự động các file .cpp trong src để build
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Tạo file thực thi từ các file source code
add_executable(${PROJECT_NAME} main.cpp ${SOURCES})

# Liên kết các module của SFML vào dự án
target_link_libraries(${PROJECT_NAME} sfml-graphics sfml-window sfml-system sfml-audio)

# Lệnh tự động copy thư mục assets vào thư mục build để game đọc được ảnh/nhạc
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)