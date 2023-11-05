cmake_minimum_required(VERSION 3.6)
project(depth)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV 4 REQUIRED)
find_package(PCL 1.10 REQUIRED)
#find_package(Ceres REQUIRED)
find_package(catkin REQUIRED COMPONENTS
  tf
  roscpp
  sensor_msgs
  nav_msgs
  pcl_ros
  livox_ros_driver2
)
include_directories(
  ${OpenCV_INCLUDE_DIRS}
  ${catkin_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIRS}
  #${CERES_INCLUDE_DIRS}
)
include_directories(
  /usr/include/pcl-1.10
)

generate_messages(
  DEPENDENCIES
  geometry_msgs
  livox_ros_driver2
  nav_msgs
  sensor_msgs
)

add_executable(depth_ori src/main.cpp)
target_link_libraries(depth_ori ${OpenCV_LIBS} ${catkin_LIBRARIES} ${PCL_LIBRARIES} -lpthread -lm yaml-cpp tbb glog)

set(CMAKE_BUILD_TYPE Release)