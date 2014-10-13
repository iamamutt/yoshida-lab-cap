#ifndef AXIS_HPP
#define AXIS_HPP

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <string>

std::string AxisURL(std::string id, std::string pwd, std::string ip, double fps);
void InitIPCam(cv::VideoCapture& cap, const std::string& url);

#endif // AXIS_HPP
