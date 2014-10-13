#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <string>
#include "axis.hpp"

using namespace cv;
using namespace std;

//InitIPCam(ip_vid_1, ipcam1_url);
//InitIPCam(ip_vid_2, ipcam2_url);


//    string ipcam1_url;
//    string ipcam2_url;

//    ipcam1_url = AxisURL(
//                "root",
//                "root",
//                "172.26.99.185:8001",
//                frames_per_sec
//                );

//    ipcam2_url = AxisURL(
//                "root",
//                "root",
//                "172.26.99.185:8002",
//                frames_per_sec
//                );

string AxisURL(string id, string pwd, string ip, double fps)
{

    string opts;
    string url;

    opts = "/axis-cgi/mjpg/video.cgi?resolution=640x400&req_fps=" +
            to_string(fps) + "&.mjpg";

    url = "http://" + id + ":" + pwd +
            "@" + ip + opts;

    return url;

}

void InitIPCam(VideoCapture& cap, const string& url)
{

    cout << "\nTrying cam: \n" << url << endl;

    cap.open(url);

    Mat frame;
    bool is_found = cap.read(frame);

    if (!is_found)
    {
        cout << "\nCannot open video cams" << url << endl;
        throw;
    }
}
