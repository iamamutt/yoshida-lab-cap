/**
    project: cogdevcam
    source file: test_video
    description:

    @author Joseph M. Burling
    @version 0.9.0 12/11/2017
*/

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <sstream>
#include <string>

using namespace cv;
using namespace std;

bool
openCaptureDevice(VideoCapture &reader, int dev)
{
    bool opened = false;
    try
    {
        if (!reader.isOpened())
        {
            std::cout << "\nTrying to open device:\n " << dev << "\n";
            opened = reader.open(dev);
        }
    } catch (...)
    {
        std::cerr << "Stopping search. Cam not detected with input:\n " << dev
                  << "\n";
        reader.release();
        opened = false;
    }

    if (!opened)
    {
        reader.release();
    }

    return opened;
};

int
countCameras(int maxTested = 5)
{
    VideoCapture cap;
    string       win_name;
    bool         cam_is_open;

    cout << "Starting search for cams..." << endl;

    for (int i = 0; i < maxTested; i++)
    {
        cam_is_open = openCaptureDevice(cap, i);
        if (cam_is_open)
        {
            cout << "Cam Found: " << i << endl;
            win_name = "test: " + to_string(i);
            namedWindow(win_name, WINDOW_AUTOSIZE);
            for (int j = 0; j < 5; j++)
            {
                Mat frame;
                cap.read(frame);
                imshow(win_name, frame);
                if (waitKey(33) == 27) break;
            }
            destroyWindow(win_name);
            cap.release();
        } else
        {
            return i - 1;
        }
    }

    if (cap.isOpened()) cap.release();

    return 0;
}

bool
readNextFrame(VideoCapture &reader, Mat &mat)
{
    if (reader.isOpened())
    {
        if (!reader.grab())
        {
            std::cerr << "Frame was not grabbed successfully for device:\n";
            return false;
        }
        if (!reader.retrieve(mat))
        {
            std::cerr << "Frame was not decoded successfully for device:\n";
            return false;
        }
    } else
    {
        return false;
    }
    return true;
};

int
main(int argc, char *argv[])
{
    int    cam_id_num;
    int    window_wait_ms = 33;
    string write_file;

    // handle input arguments
    if (argc == 4)
    {
        stringstream arg1(argv[1]);
        stringstream arg2(argv[2]);
        stringstream arg3(argv[3]);
        arg1 >> cam_id_num;
        arg2 >> window_wait_ms;
        arg3 >> write_file;
    } else if (argc == 3)
    {  // cam id and wait time
        stringstream arg1(argv[1]);
        stringstream arg2(argv[2]);
        arg1 >> cam_id_num;
        arg2 >> window_wait_ms;
    } else if (argc == 2)
    {  // cam id only
        stringstream arg1(argv[1]);
        arg1 >> cam_id_num;
    } else
    {  // no args
        cam_id_num = countCameras();
        if (cam_id_num == -1)
        {
            cerr << "No Cameras Were Found!" << endl;
            return -1;
        }
    }

    cout << "\n----------------\nUsing camera: " << cam_id_num << endl;

    // construct capture object
    VideoCapture cap;
    VideoWriter  write;

    // start window
    stringstream cam_name;
    cam_name << "Camera " << cam_id_num << ": Press ESC on window to quit";
    namedWindow(cam_name.str(), WINDOW_AUTOSIZE);
    moveWindow(cam_name.str(), 10, 10);

    // open device number
    cap.open(cam_id_num);

    // make sure device number is intialized
    if (!cap.isOpened())
    {
        cout << "Cam not found with that ID" << endl;
        return -1;
    }

    Mat frame;
    readNextFrame(cap, frame);

    if (!write_file.empty())
    {
        auto fps = cap.get(CAP_PROP_FPS);
        if (fps == 0) fps = 30;
        auto fourcc = write.fourcc('M', 'P', '4', 'V');
        write.open(write_file, fourcc, fps, frame.size(), true);
    }

    // start frame capture and show
    while (true)
    {

        if (!readNextFrame(cap, frame)) return -1;
        imshow(cam_name.str(), frame);
        if (waitKey(window_wait_ms) == 27) break;  // escape key
        if (write.isOpened()) write.write(frame);
    }

    cap.release();
    write.release();
    return 0;
}
