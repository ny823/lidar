#include <iostream>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include "../include/depth_ori.hpp"

int main(int argc, char **argv)
{
    ros::init(argc, argv, "depth_ori");
    ros::NodeHandle n;
    Init("/Code/lidar_ws/src/Depth/depth.yaml");
    depth_img_pub = n.advertise<sensor_msgs::Image>("/cam/depth", 1000);
    ros::Subscriber sub_lidar = n.subscribe("/livox/lidar", 10, lidar_callback);
    ros::spin();
    return 0;
}