install(
    TARGETS compiler_exe
    RUNTIME COMPONENT compiler_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
