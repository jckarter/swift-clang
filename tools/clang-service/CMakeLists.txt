macro(create_framework fmwk)
  set(fmwk ${FMWK_NAME})
  set(fmwk_dir ${LLVM_LIBRARY_DIR}/${fmwk}.framework)
  set(fmwk_A ${fmwk_dir}/Versions/A)

  add_custom_target("create-${fmwk}-layout"
    ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${fmwk_dir}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${fmwk_dir}/Versions
    COMMAND ${CMAKE_COMMAND} -E make_directory ${fmwk_A}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${fmwk_A}/Headers
    COMMAND ${CMAKE_COMMAND} -E make_directory ${fmwk_A}/Resources
    COMMAND ${CMAKE_COMMAND} -E make_directory ${fmwk_A}/XPCServices
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${fmwk_A}
            ${fmwk_dir}/Versions/Current
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${fmwk_A}/Headers
            ${fmwk_dir}/Headers
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${fmwk_A}/Resources
            ${fmwk_dir}/Resources
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${fmwk_A}/XPCServices
            ${fmwk_dir}/XPCServices
  )
endmacro(create_framework)

macro(create_framework_dylib)
  cmake_parse_arguments(DYLIB
    ""
    "PLIST;FMWK"
    "LLVM_COMPONENTS;CLANG_COMPONENTS"
    ${ARGN}
  )
  set(name ${DYLIB_FMWK})
  set(plist ${DYLIB_PLIST})
  set(llvmcom ${DYLIB_LLVM_COMPONENTS})
  set(clangcom ${DYLIB_CLANG_COMPONENTS})

  set(fmwk_dir "${LLVM_LIBRARY_DIR}/${name}.framework")
  set(fmwk_A "${fmwk_dir}/Versions/A")

  add_custom_target("create-${name}-dylib-plist"
    ALL
    COMMAND ${CMAKE_COMMAND} -E copy
            "${plist}"
            "${fmwk_A}/Resources/Info.plist"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  add_dependencies("create-${name}-dylib-plist" "create-${name}-layout")

  add_library(${name} SHARED "${plist}")
  target_link_libraries(${name} ${llvmcom})
  target_link_libraries(${name} ${clangcom})
  target_link_libraries(${name} ${LLVM_COMMON_LIBS})
  set_target_properties(${name} PROPERTIES FRAMEWORK TRUE)
  set_target_properties(${name} PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(${name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${fmwk_A}")
  set_target_properties(${name} PROPERTIES INSTALL_RPATH "${LLVM_LIBRARY_DIR}")
  if(LLVM_COMMON_DEPENDS)
    add_dependencies(${name} ${LLVM_COMMON_DEPENDS})
  endif(LLVM_COMMON_DEPENDS)
  add_dependencies(${name} "create-${name}-dylib-plist")
endmacro(create_framework_dylib)

macro(create_framework_service)
  cmake_parse_arguments(XPC
    ""
    "NAME;PLIST;FMWK"
    "SOURCES;LLVM_COMPONENTS;CLANG_COMPONENTS"
    ${ARGN}
  )
  set(name ${XPC_NAME})
  set(plist ${XPC_PLIST})
  set(fmwk ${XPC_FMWK})
  set(srcs ${XPC_SOURCES})
  set(llvmcom ${XPC_LLVM_COMPONENTS})
  set(clangcom ${XPC_CLANG_COMPONENTS})

  set(fmwk_dir ${LLVM_LIBRARY_DIR}/${fmwk}.framework)
  set(xpc_contents_dir "${fmwk_dir}/Versions/A/XPCServices/${name}.xpc/Contents")
  set(xpc_bin_dir "${xpc_contents_dir}/MacOS")

  add_custom_target("create-${fmwk}-service-plist"
    ALL
    COMMAND ${CMAKE_COMMAND} -E copy
            "${plist}"
            "${xpc_contents_dir}/Info.plist"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  add_dependencies("create-${fmwk}-service-plist" "create-${fmwk}-layout")

  list(APPEND srcs "${plist}")

  set(LLVM_LINK_COMPONENTS ${llvmcom})
  add_clang_executable(${name} ${srcs})
  target_link_libraries(${name} ${clangcom})
  target_link_libraries(${name} ${LLVM_COMMON_LIBS})
  target_link_libraries(${name} ${fmwk})
  set_target_properties(${name} PROPERTIES LINKER_LANGUAGE CXX)
  set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${xpc_bin_dir}")
  set_target_properties(${name} PROPERTIES INSTALL_RPATH "${LLVM_LIBRARY_DIR}")
  if(LLVM_COMMON_DEPENDS)
    add_dependencies(${name} ${LLVM_COMMON_DEPENDS})
  endif(LLVM_COMMON_DEPENDS)
  add_dependencies(${name} "create-${fmwk}-service-plist")
endmacro(create_framework_service)

# FIXME: Install the clang-service headers into the framework.
create_framework(ClangService)

add_clang_subdirectory(lib)

create_framework_dylib(
  PLIST
  Info.plist

  FMWK
  ClangService

  CLANG_COMPONENTS
  clangServiceCore
  clangServiceSupport
  clangServiceXPC
)

add_clang_subdirectory(tools)

if(LLVM_INCLUDE_TESTS)
  add_subdirectory(unittests)
endif()
