add_subdirectory(libloom)
add_subdirectory(libloomw)
add_subdirectory(worker)
add_subdirectory(server)

configure_file("loom_define.h.in" "${PROJECT_BINARY_DIR}/loom_define.h")
