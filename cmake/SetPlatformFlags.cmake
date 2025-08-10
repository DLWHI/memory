function(LOAD_VARS FILE)
    set(OPTS "Following configuration was loaded from ${FILE}")
    file(STRINGS ${FILE} CONTENTS)
    foreach(NAME_AND_VALUE ${CONTENTS})
      # Strip leading spaces
      string(REGEX REPLACE "^[ ]+" "" NAME_AND_VALUE "${NAME_AND_VALUE}")
      if (NAME_AND_VALUE MATCHES "^#")
        continue()
      endif()
      # Find variable name
      string(REGEX MATCH "^[^=]+" NAME "${NAME_AND_VALUE}")
      # Find the value
      string(REPLACE "${NAME}=" "" VALUE "${NAME_AND_VALUE}")
      # Set the variable
      set(${NAME} "${VALUE}" CACHE STRING "" FORCE)
      set(OPTS "${OPTS}\n${NAME}=${VALUE}")
    endforeach()
    message(STATUS "${OPTS}")
endfunction()

load_vars(project.cfg)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  load_vars(opt_msvc.cfg)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
       OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  load_vars(opt_unix.cfg)
endif()
