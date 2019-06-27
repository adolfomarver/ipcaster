cmake_minimum_required(VERSION 3.0)

#Jsoncpp amalgamated file
set(jsoncpp ${CMAKE_CURRENT_LIST_DIR}/../third_party/jsoncpp/jsoncpp.cpp)

add_executable(ipcaster main.cpp ${jsoncpp})

target_include_directories(ipcaster PRIVATE ../third_party/jsoncpp/json)
target_include_directories(ipcaster PRIVATE ../)
#target_include_directories(ipcaster PRIVATE "C:/Program Files (x86)/boost_1_70_0")

target_link_libraries(ipcaster boost_system boost_program_options pthread event)
#target_link_libraries(ipcaster boost_system boost_program_options)
#link_directories("C:/Program Files (x86)/boost_1_70_0/stage/lib")


