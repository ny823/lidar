#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/PointCloud2.h>
#include <opencv2/opencv.hpp>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/range_image/range_image.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/common/common.h>
#include <pcl/common/transforms.h>
#include <pcl/registration/icp.h>
#include <pcl/io/pcd_io.h>
#include <pcl/filters/filter.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/filters/crop_box.h>
#include <pcl_conversions/pcl_conversions.h>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <yaml-cpp/yaml.h>
#include <Eigen/Geometry>
#include "livox_ros_driver2/CustomMsg.h"
#include <tf/transform_broadcaster.h>

typedef pcl::PointXYZI PointType;

sensor_msgs::Image depth_img;
ros::Publisher depth_img_pub;

pcl::PointCloud<PointType>::Ptr depthCloud(new pcl::PointCloud<PointType>());

double left_intrinsics[4];
Eigen::Affine3f tran_lidar2cam;
Eigen::Matrix3d R_camera_lidar; // 旋转矩阵
Eigen::Matrix4d lidar2camera;
Eigen::Vector3d T_camera_lidar;
int width, height;

void Init(std::string file_path)
{
    YAML::Node yaml_node = YAML::LoadFile(file_path);
    width = yaml_node["Width"].as<double>();
    height = yaml_node["Height"].as<double>();
    for (int i = 0; i < 4; i++)
    {
        left_intrinsics[i] = yaml_node["Cam"]["intrinsics"][i].as<double>();
    }
    for (int i = 0; i < 16; i++)
    {
        lidar2camera(i / 4, i % 4) = yaml_node["T_lidar_camera"][i / 4][i % 4].as<double>();
    }
    R_camera_lidar << lidar2camera(0, 0), lidar2camera(0, 1), lidar2camera(0, 2),
        lidar2camera(1, 0), lidar2camera(1, 1), lidar2camera(1, 2),
        lidar2camera(2, 0), lidar2camera(2, 1), lidar2camera(2, 2); // world坐标到imu坐标的转换R
    // 得到imu到camera坐标的转换
    T_camera_lidar = R_camera_lidar.eulerAngles(2, 1, 0);
    tran_lidar2cam = pcl::getTransformation(lidar2camera(0, 3), lidar2camera(1, 3), lidar2camera(2, 3),
                                            T_camera_lidar[2], T_camera_lidar[1], T_camera_lidar[0]);
    depth_img.header.frame_id = "cam0";
    depth_img.height = height;
    depth_img.width = width;
    depth_img.encoding = "mono16";
    depth_img.is_bigendian = false;
    depth_img.step = width * 2;
    depth_img.data.resize(depth_img.step * height);
    memset(reinterpret_cast<uchar *>(&depth_img.data[0]), 0, height * width * 2);
}

void lidar_callback(const livox_ros_driver2::CustomMsgConstPtr &lidar_msg)
{
    // std::cout << "1" << std::endl;
    // 一.产生pcl点云
    depthCloud->clear();
    memset(reinterpret_cast<uchar *>(&depth_img.data[0]), 0, height * width * 2);
    for (int i = 0; i < lidar_msg->points.size(); i++)
    {
        PointType pt;
        pt.x = lidar_msg->points[i].x;
        pt.y = lidar_msg->points[i].y;
        pt.z = lidar_msg->points[i].z;
        depthCloud->push_back(pt);
    }
    // 二.产生深度图
    pcl::PointCloud<PointType>::Ptr laser_cloud_offset(new pcl::PointCloud<PointType>());
    pcl::transformPointCloud(*depthCloud, *laser_cloud_offset, tran_lidar2cam);

    // 2.三维点映射为像素，并绘制depth图
    int point = 0;
    for (int i = 0; i < (int)laser_cloud_offset->size(); ++i)
    {
        if (laser_cloud_offset->points[i].z > 0)
        {
            int u = left_intrinsics[0] * laser_cloud_offset->points[i].x / laser_cloud_offset->points[i].z + left_intrinsics[2];
            int v = left_intrinsics[1] * laser_cloud_offset->points[i].y / laser_cloud_offset->points[i].z + left_intrinsics[3];
            if (u >= width || v >= height || u < 0 || v < 0)
            {
                continue;
            }
            if ((depth_img.data[v * width * 2 + u * 2] == 0 && depth_img.data[v * width * 2 + u * 2 + 1] == 0) ||
                depth_img.data[v * width * 2 + u * 2 + 1] > ((int)(laser_cloud_offset->points[i].z * 2048)) >> 8)
            {
                point++;
                int depth_2048 = (int)(laser_cloud_offset->points[i].z * 2048);
                // *(reinterpret_cast<int *>(&depth_img.data[0]) + v * width + u) = (int)(laser_cloud_offset->points[i].z * 2048);
                depth_img.data[v * width * 2 + u * 2] = depth_2048;
                depth_img.data[v * width * 2 + u * 2 + 1] = depth_2048 >> 8;
                // std::cout << (int)depth_img.at<uchar>(v, u) << std::endl;
            }
        }
    }
    std::cout << "point:" << point << std::endl;
    depth_img.header.stamp = lidar_msg->header.stamp;
    depth_img_pub.publish(depth_img);
}
