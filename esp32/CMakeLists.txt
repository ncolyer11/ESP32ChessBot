set(ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

idf_component_register(
    SRCS
        main_esp32.cpp
        ${ROOT}/src/app.cpp
        ${ROOT}/src/board.cpp
        ${ROOT}/src/boardRepresentation.cpp
        ${ROOT}/src/fen.cpp
        ${ROOT}/src/move.cpp
        ${ROOT}/src/movegen.cpp
        ${ROOT}/src/util.cpp
        ${ROOT}/src/ai/chess_search.cpp
        ${ROOT}/src/ai/eval.cpp
    INCLUDE_DIRS
        ${ROOT}/include
    REQUIRES driver esp_driver_uart esp_driver_usb_serial_jtag vfs esp_timer esp_hw_support heap
)

# the engine uses designated init, constexpr lambdas etc
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++20)
