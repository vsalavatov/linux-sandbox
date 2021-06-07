add_executable(sandbox
    src/sandbox.cpp
    src/task.cpp
    src/task_constraints.cpp
    src/exceptions.cpp
)
target_include_directories(sandbox PUBLIC include/)
add_custom_command(TARGET sandbox POST_BUILD
    COMMAND sudo setcap cap_sys_admin+ep $<TARGET_FILE:sandbox>
    COMMENT "adding cap_sys_admin to sandbox binary..."
)
