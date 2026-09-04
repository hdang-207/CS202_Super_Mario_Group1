cmake_minimum_required(VERSION 3.15)
project(SuperMarioBros VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Đầu ra của file thực thi (.exe) sẽ nằm trong thư mục bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Tự động hỗ trợ tìm SFML_DIR trên các đường dẫn phổ biến nếu chưa khai báo
if(NOT SFML_DIR)
    if(EXISTS "D:/Programs/SFML-3.1.0/lib/cmake/SFML")
        set(SFML_DIR "D:/Programs/SFML-3.1.0/lib/cmake/SFML" CACHE PATH "Path to SFML CMake config")
    elseif(EXISTS "C:/SFML/lib/cmake/SFML")
        set(SFML_DIR "C:/SFML/lib/cmake/SFML" CACHE PATH "Path to SFML CMake config")
    endif()
endif()

# Bật liên kết tĩnh (Static Link) cho SFML trên MinGW để tránh lỗi xung đột DLL ABI (0xc0e90002)
if(MINGW)
    set(SFML_STATIC_LIBRARIES TRUE)
endif()

# Tìm và tích hợp thư viện SFML (Yêu cầu các thành viên phải cài sẵn SFML trên máy)
find_package(SFML 3.0 COMPONENTS Graphics Window System Audio REQUIRED)

include(CTest)

# Gom tự động các file .hpp trong thư mục include để hiển thị trên các IDE (CLion, VS)
file(GLOB_RECURSE HEADERS "include/*.hpp" "include/*.h")

# Gom tự động các file .cpp trong src để build
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Tạo file thực thi từ các file source code (bao gồm cả headers để IDE nhận diện cấu trúc thư mục)
add_executable(${PROJECT_NAME} main.cpp ${SOURCES} ${HEADERS})

# Khai báo thư mục include cho target (Modern CMake thay vì dùng include_directories toàn cục)
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Đường dẫn source tree dùng làm phương án dự phòng cuối cùng khi tìm thư mục assets
# (xem Systems::resourcePath) - nhờ vậy không ai phải hardcode đường dẫn máy mình nữa.
target_compile_definitions(${PROJECT_NAME} PRIVATE MARIO_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# Liên kết các module của SFML vào dự án
target_link_libraries(${PROJECT_NAME} PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)

if(MINGW)
    target_link_options(${PROJECT_NAME} PRIVATE -mconsole)
    
    # Tự động sao chép các DLL runtime của MinGW (libgcc, libstdc++, libwinpthread) vào thư mục bin
    # để tránh lỗi Bad Image (0xc0e90002) hoặc thiếu DLL khi chạy trực tiếp trên Windows
    get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    set(MINGW_RUNTIME_DLLS
        "libgcc_s_seh-1.dll"
        "libstdc++-6.dll"
        "libwinpthread-1.dll"
    )
    foreach(DLL ${MINGW_RUNTIME_DLLS})
        if(EXISTS "${MINGW_BIN_DIR}/${DLL}")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${MINGW_BIN_DIR}/${DLL}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DLL}"
            )
        endif()
    endforeach()
endif()

# Trên macOS: SFML tạo surface OpenGL ở mật độ 1x nên màn Retina bị phóng to làm mờ.
# File .mm dưới đây (Objective-C++) bật surface đúng độ phân giải màn hình.
# Windows/Linux không cần, phần thân hàm nằm ở HighDpi.cpp và chỉ trả về 1.
if(APPLE)
    enable_language(OBJCXX)
    target_sources(${PROJECT_NAME} PRIVATE src/Systems/HighDpi.mm)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework AppKit" "-framework OpenGL")
endif()

# Luôn đồng bộ assets khi build target game. POST_BUILD chỉ chạy khi executable
# được link lại, nên thay đổi riêng PNG sẽ không tới được thư mục bin/.
add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets"
    COMMENT "Synchronizing game assets"
)
add_dependencies(${PROJECT_NAME} copy_assets)

if(BUILD_TESTING)
    add_executable(physics_tests
        tests/physics/PhysicsRegressionTests.cpp
        src/Physics/PhysicsBody.cpp
        src/Physics/PhysicsSystem.cpp
        src/Physics/Broadphase.cpp
        src/Physics/PlatformContact.cpp
    )

    target_include_directories(physics_tests PRIVATE include)
    target_link_libraries(physics_tests PRIVATE SFML::System)

    add_test(
        NAME PhysicsRegressionTests
        COMMAND physics_tests
    )

    add_executable(input_handler_tests
        tests/input/InputHandlerTests.cpp
        src/Input/InputHandler.cpp
    )

    target_include_directories(input_handler_tests PRIVATE include)
    target_link_libraries(input_handler_tests PRIVATE SFML::Window)

    add_test(
        NAME InputHandlerTests
        COMMAND input_handler_tests
    )

    add_executable(duel_rules_tests
        tests/states/DuelRulesTests.cpp
    )

    target_include_directories(duel_rules_tests PRIVATE include)
    target_link_libraries(duel_rules_tests PRIVATE SFML::System)

    add_test(
        NAME DuelRulesTests
        COMMAND duel_rules_tests
    )

    add_executable(duel_arena_tests
        tests/states/DuelArenasTests.cpp
        src/Systems/MapParser.cpp
        src/Systems/ResourcePath.cpp
    )

    target_include_directories(duel_arena_tests PRIVATE include)
    target_compile_definitions(duel_arena_tests
        PRIVATE MARIO_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

    add_test(
        NAME DuelArenaTests
        COMMAND duel_arena_tests
    )

    add_executable(duel_player_collision_tests
        tests/states/DuelPlayerCollisionTests.cpp
        src/States/DuelPlayerCollision.cpp
        src/Physics/PhysicsBody.cpp
    )

    target_include_directories(duel_player_collision_tests PRIVATE include)
    target_link_libraries(duel_player_collision_tests PRIVATE SFML::System)

    add_test(
        NAME DuelPlayerCollisionTests
        COMMAND duel_player_collision_tests
    )
endif()

# Tự động copy file DLL của SFML vào thư mục bin trên Windows (đối với liên kết động - Dynamic Link)
if(WIN32)
    # Lấy đường dẫn thư mục bin chứa DLL từ đường dẫn của thư viện SFML được tìm thấy
    # (Nếu tìm thấy qua find_package, SFML_DIR thường trỏ tới <SFML_PATH>/lib/cmake/SFML)
    if(SFML_DIR)
        get_filename_component(SFML_ROOT_DIR "${SFML_DIR}/../../.." ABSOLUTE)
        set(SFML_BIN_DIR "${SFML_ROOT_DIR}/bin")
        
        # Danh sách các file DLL cần copy tương ứng với các component đang dùng (SFML 3 Release & Debug)
        set(SFML_DLLS
            "sfml-graphics-3.dll"
            "sfml-graphics-d-3.dll"
            "sfml-window-3.dll"
            "sfml-window-d-3.dll"
            "sfml-system-3.dll"
            "sfml-system-d-3.dll"
            "sfml-audio-3.dll"
            "sfml-audio-d-3.dll"
            "openal32.dll" # SFML Audio phụ thuộc vào thư viện này
        )
        
        foreach(DLL ${SFML_DLLS})
            # Sao chép DLL nếu tồn tại trong thư mục bin của SFML
            if(EXISTS "${SFML_BIN_DIR}/${DLL}")
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${SFML_BIN_DIR}/${DLL}"
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DLL}"
                )
            endif()
        endforeach()
    endif()
endif()
