cmake_minimum_required(VERSION 3.15)
project(SuperMarioBros VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Đầu ra của file thực thi (.exe) sẽ nằm trong thư mục bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Tìm và tích hợp thư viện SFML (Yêu cầu các thành viên phải cài sẵn SFML trên máy)
find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

# Gom tự động các file .hpp trong thư mục include để hiển thị trên các IDE (CLion, VS)
file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h")

# Gom tự động các file .cpp trong src để build
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Tạo file thực thi từ các file source code (bao gồm cả headers để IDE nhận diện cấu trúc thư mục)
add_executable(${PROJECT_NAME} main.cpp ${SOURCES} ${HEADERS})

# Khai báo thư mục include cho target (Modern CMake thay vì dùng include_directories toàn cục)
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Liên kết các module của SFML vào dự án
target_link_libraries(${PROJECT_NAME} sfml-graphics sfml-window sfml-system sfml-audio)

# Lệnh tự động copy thư mục assets vào thư mục build để game đọc được ảnh/nhạc
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

# Tự động copy file DLL của SFML vào thư mục bin trên Windows (đối với liên kết động - Dynamic Link)
if(WIN32)
    # Lấy đường dẫn thư mục bin chứa DLL từ đường dẫn của thư viện SFML được tìm thấy
    # (Nếu tìm thấy qua find_package, SFML_DIR thường trỏ tới <SFML_PATH>/lib/cmake/SFML)
    if(SFML_DIR)
        get_filename_component(SFML_ROOT_DIR "${SFML_DIR}/../../.." ABSOLUTE)
        set(SFML_BIN_DIR "${SFML_ROOT_DIR}/bin")
        
        # Danh sách các file DLL cần copy tương ứng với các component đang dùng
        set(SFML_DLLS
            "sfml-graphics-2.dll"
            "sfml-window-2.dll"
            "sfml-system-2.dll"
            "sfml-audio-2.dll"
            "openal32.dll" # SFML Audio phụ thuộc vào thư viện này
        )
        
        foreach(DLL ${SFML_DLLS})
            # Sao chép DLL sau khi build xong
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SFML_BIN_DIR}/${DLL}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DLL}"
                ERROR_QUIET
            )
        endforeach()
    endif()
endif()