cmake_minimum_required(VERSION 3.0)

add_executable(ipcaster main.cpp IPCaster.cpp)

target_include_directories(ipcaster PRIVATE ../)

# Windows
if (MSVC)
	set(cpprestsdk_DIR "${HOME}/vcpkg/installed/x86-windows/share/cpprestsdk")
	find_package(cpprestsdk REQUIRED)
	target_link_libraries(ipcaster ${Boost_LIBRARIES} cpprestsdk::cpprest)
	add_definitions(-D _WIN32_WINNT=0x0601)
else()
# Linux
	find_package(OpenSSL REQUIRED)
	#target_link_libraries(ipcaster boost_system boost_program_options pthread event cpprestsdk::cpprest)
	target_link_libraries(ipcaster boost_system boost_program_options pthread event cpprest OpenSSL::SSL)
endif()


