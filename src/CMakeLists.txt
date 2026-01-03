add_executable(goose_http)

target_sources(goose_http
  PRIVATE
    main.cpp
  PUBLIC
)

target_compile_features(goose_http
  PRIVATE
    cxx_std_26
)

# target_link_libraries(goose_http
#   PUBLIC
#     Boost::headers
#     Boost::system
# )
#
# taget_include_directories(goose_http
#   PUBLIC
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
# )
