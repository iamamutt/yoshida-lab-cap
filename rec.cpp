#include <iostream>
#include <string>
#include <ctime>
#include "opencv2/opencv.hpp"
#include "rec.hpp"
#include <fstream>
#include <iomanip>

using namespace cv;
using namespace std;

void RecSwitch(int recPosition, void*)
{
    cout << "\nRec = " << recPosition << endl;
}

void RecDot(Mat img, Point dotxy, int dotRadius)
{

    //    Size shrinkImg = Size(frameSize.width / 2, frameSize.height / 2);
    //    Point dotLoc = Point(shrinkImg.width / 10, shrinkImg.height / 10);
    //    int dotSize = shrinkImg.height / 20;

    circle(img, dotxy, dotRadius, Scalar(0, 0, 255), -1, 8);
    putText(img, "REC", Point(dotxy.x + 15, dotxy.y + 15),
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255));
}

void UpdateTick(double& ticker)
{
    ticker = static_cast<double>(getTickCount());
}

void TimeDuration(double& dur, double& tickOn, double& tickOff)
{
    double ms = 1000;
    dur = ms * ((tickOff - tickOn) / getTickFrequency());
}

void TimeElapsed(double& recSeconds, double& tickStartRec, double& currentTick)
{
    recSeconds = (currentTick - tickStartRec) / getTickFrequency();
}

bool CaptureWait(double& loopTime, double& timeBetweenFrames, int& key)
{

    double waitTime = round(timeBetweenFrames - (loopTime + 2));

    if (waitTime > 1)
    {
        // wait for extra time
        if (waitKey(waitTime) == key) return true;
    }
    else
    {
        // wait anyway for 1 ms for key press
        if (waitKey(1) == key) return true;
    }

    return false;
}

void VideoDisplaySetup(Size& grid, Size& imdim, vector<Mat>& imgs, double& scaler)
{

    // find number of images for placement
    int n_imgs = static_cast<int>(imgs.size());

    // calculate m rows and n cols based on num images
    int n_cols = static_cast<int>(ceil(sqrt(n_imgs)));
    int m_rows = static_cast<int>(ceil(n_imgs/n_cols));

    // matrix which holds x,y dims for later max function
    Mat tmp_r = Mat_<double>(m_rows, n_cols);
    Mat tmp_c = Mat_<double>(m_rows, n_cols);

    int img_idx = 0;

    for (int m=0; m < m_rows; m++)
    {
        for (int n=0; n < n_cols; n++)
        {
            if(img_idx < imgs.size())
            {
                // place x/y size of current image in matrix
                tmp_r.at<double>(m,n) = imgs[img_idx].rows * scaler;
                tmp_c.at<double>(m,n) = imgs[img_idx].cols * scaler;
            }
            img_idx++;
        }
    }

    // find max size for each dimension
    vector<double> max_row(m_rows);
    vector<double> max_col(n_cols);

    double img_height = 0;
    double img_width = 0;

    for (int m=0; m < m_rows; m++)
    {
        minMaxIdx(tmp_r, 0, &max_row[m]);
        img_height += ceil(max_row[m]);
    }

    for (int n=0; n < n_cols; n++)
    {
        minMaxIdx(tmp_c, 0, &max_col[n]);
        img_width += ceil(max_col[n]);
    }


    imdim = Size(img_width, img_height);
    grid = Size(n_cols, m_rows);

}

vector<Mat> InitCam(const vector<int>& idx, vector<VideoCapture>& devices)
{
    int n_devices = static_cast<int>(devices.size());
    vector<Mat> cam_imgs (n_devices);
    string winname;

    for (int i=0; i < n_devices; i++)
    {
        cout << "\nTrying device: \n" << idx[i] << endl;
        devices[i].open(idx[i]);
        Mat frame;

        if (!devices[i].read(frame))
        {
            cout << "\nCannot open video device: " << idx[i] << endl;
            throw;
        }

        winname = "test " + to_string(idx[i]);
        namedWindow(winname, WINDOW_AUTOSIZE);
        imshow(winname, frame);
        waitKey(250);
        destroyWindow(winname);
        cam_imgs[i] = frame;
    }

    return cam_imgs;
}

Mat ImDisplay(string title, vector<Mat>& imgs,
              Size& imSize, Size& gridSize, double& scaler)
{
    Mat out_img = Mat::zeros(imSize.height, imSize.width, CV_8UC3);
    int n_imgs = static_cast<int>(imgs.size());
    int img_idx = 0;
    int out_row_start = 0;
    int out_row_end = 0;
    int out_col_start = 0;
    int out_col_end = 0;

    for (int m=0; m < gridSize.height; m++)
    {
        for (int n=0; n < gridSize.width; n++)
        {
            if (img_idx < n_imgs)
            {
                Mat in_img;

                resize(imgs[img_idx], in_img,
                       Size(), scaler, scaler, INTER_NEAREST);

                Size in_img_sz = in_img.size();

                out_row_end = out_row_start + (in_img_sz.height-1);
                out_col_end = out_col_start + (in_img_sz.width-1);

                Range in_roi_x = Range(0, in_img_sz.height - 1);
                Range in_roi_y = Range(0, in_img_sz.width - 1);

                Range out_roi_x = Range(out_row_start, out_row_end);
                Range out_roi_y = Range(out_col_start, out_col_end);

                //                cout << "\n--IN RECT:" << img_idx << endl;
                //                cout << in_roi_x.start << endl;
                //                cout << in_roi_y.start << endl;
                //                cout << in_roi_x.end << endl;
                //                cout << in_roi_y.end << endl;

                //                cout << "\n--OUT RECT:" << img_idx << endl;
                //                cout << out_roi_x.start << endl;
                //                cout << out_roi_y.start << endl;
                //                cout << out_roi_x.end << endl;
                //                cout << out_roi_y.end << endl;

                in_img(in_roi_x, in_roi_y).copyTo(out_img(out_roi_x, out_roi_y));

            }
            img_idx++;
            out_col_start = out_col_end + 1;
        }
        out_row_start = out_row_end + 1;
    }

    namedWindow(title);
    imshow(title, out_img);
    return out_img;
}

string CommonFileName()
{
    time_t rawtime;
    struct tm * timeinfo;
    char buffer [80];

    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime(buffer, sizeof(buffer), "-on-%Y_%m_%d-at-%H_%M", timeinfo);
    string filestr (buffer);

    return filestr;
}

void WriterInitialize(vector<VideoWriter>& vwrite, string& file, double& fps, vector<Mat>& img)
{

    int n_devices = static_cast<int>(vwrite.size());
    int codec = VideoWriter::fourcc('M','P','4','V');

    string fileExt (".m4v");
    string fileStr;

    for (int i=0; i < n_devices; i++)
    {
        fileStr = "camera__" + to_string(i) + file + fileExt;
        vwrite[i].open(fileStr, codec, fps, Size(img[i].cols, img[i].rows), true);
    }

}


void WriteFrames(vector<VideoWriter>& vwrite, vector<Mat>& imgs)
{

    int n_imgs = static_cast<int>(imgs.size());

    for (int i=0; i < n_imgs; i++)
    {
        vwrite[i].write(imgs[i]);
    }

}

vector<Mat> CaptureMultipleFrames(vector<VideoCapture>& devices, double& timestamp)
{

    int n_devices = static_cast<int>(devices.size());
    double tick;
    vector<Mat> imgs_captured (n_devices);

    UpdateTick(tick);

    // grab images before trying to decode
    for (int i=0; i < n_devices; i++)
    {
        devices[i].grab();
    }

    timestamp = tick / getTickFrequency();

    // decode grabbed images
    for (int i=0; i < n_devices; i++)
    {
        bool frame_found = devices[i].retrieve(imgs_captured[i]);

        if (!frame_found)
        {
            cout << "\nWARNING: Cannot read frames from video " << endl;
        }

    }

    return imgs_captured;
}

void CalcFPS(double& calcfps, double& fps, double& counter, double& tstart, double& tend, double& sec)
{
    if (counter > fps * sec)
    {
        double t_elapse;
        TimeElapsed(t_elapse, tstart, tend);
        calcfps = counter / t_elapse;
        counter = 1;
        UpdateTick(tstart);
        // cout << "FPS: " << calcfps << endl;
    }

}

void DisplayInfo(const string& window, string& text, double& fps)
{
    text = "FPS: " + to_string(fps);
    displayOverlay(window, text, 0);
    displayStatusBar(window, "RECORDING", 0);
}

void WriteCSVHeaders(ofstream& file, vector<string>& headers)
{
    int ncols = static_cast<int>(headers.size());

    for (int i=0; i < ncols; i++)
    {

        file << headers[i];

        if (i != ncols)
        {
            file << ",";
        }

    }

    file << endl;
}

void WriteData(ofstream& file, double& timestamp, double& duration)
{
    file << std::fixed << std::setprecision(5) << timestamp << ","
         << std::fixed << std::setprecision(2) << duration << ","
         << endl;
}

bool CheckForFrameUpdate(double& tick, double& tock, double& ms)
{
    bool go = false;
    double dur;
    TimeDuration(dur, tick, tock);

    if (dur > ms)
    {
        go = true;
        UpdateTick(tick);
    }



    return go;
}
