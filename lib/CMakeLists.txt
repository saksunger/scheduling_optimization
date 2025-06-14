add_library(
  commonlib STATIC AntColonyOptimization.cpp CommonFuncs.cpp Constants.cpp
                   HarmonySearch.cpp Modes.cpp RandomUtils.cpp)

target_include_directories(commonlib PUBLIC ${CMAKE_CURRENT_LIST_DIR})
