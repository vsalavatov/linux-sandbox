add_executable(sandbox
    src/sandbox.cpp
    src/task.cpp
    src/task_constraints.cpp
)
target_include_directories(sandbox PUBLIC include/)
