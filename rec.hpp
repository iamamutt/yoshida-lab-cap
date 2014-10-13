#ifndef REC_HPP
#define REC_HPP

#include "opencv2/opencv.hpp"
#include <string>
#include <fstream>

void RecSwitch(int recPosition, void*);
void RecDot(cv::Mat img, cv::Point dotxy, int dotRadius);
void UpdateTick(double& ticker);

// Returns duration in milliseconds
void TimeDuration(double& dur, double& tickOn, double& tickOff);
void TimeElapsed(double& recSeconds, double& tickStartRec, double& currentTick);
bool CaptureWait(double& loopTime, double& timeBetweenFrames, int &key);

void VideoDisplaySetup(cv::Size& grid, cv::Size& imdim, std::vector<cv::Mat>& imgs, double& scaler);
std::vector<cv::Mat> InitCam(const std::vector<int> &idx, std::vector<cv::VideoCapture> &devices);
cv::Mat ImDisplay(std::string title, std::vector<cv::Mat>& imgs, cv::Size& imSize, cv::Size& gridSize, double& scaler);
std::string CommonFileName();
void WriterInitialize(std::vector<cv::VideoWriter>& vwrite, std::string &file, double& fps, std::vector<cv::Mat>& img);
void WriteFrames(std::vector<cv::VideoWriter>& vwrite, std::vector<cv::Mat>& imgs);
std::vector<cv::Mat> CaptureMultipleFrames(std::vector<cv::VideoCapture>& devices, double &timestamp);
void DisplayInfo(const std::string &window, std::string& text, double& tstart, double& fps);
void CalcFPS(double& calcfps, double& fps, double& counter, double& tstart, double& tend);
void WriteCSVHeaders(std::ofstream& file, std::vector<std::string>& headers);
void WriteData(std::ofstream& file, double& timestamp, double& duration);

#endif // REC_HPP
