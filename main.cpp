#include <iostream>
#include <string>
#include "opencv2/opencv.hpp"
#include "rec.hpp"
#include <fstream>
#include <iomanip>

using namespace cv;
using namespace std;


int main()
{

    // global parameters
    int exit_key = 27;
    double resize_disp_imgs = 0.85;
    double resize_rec_imgs = 0.5;
    double frames_per_sec = 30;
    const int eye_cam_ids[] = {0, 1};


    // timing parameters
    double start_rec_tick;
    double timer_refresh;
    double tick_onset;
    double tick_offset;
    double grab_timestamp;
    double sampling_rate_ms = 1000.0 / frames_per_sec;
    double loop_duration = 0;
    double output_rec_time = 1 / frames_per_sec;
    double frame_counter = 0;
    double calc_fps = frames_per_sec;
    double fps_interval = 10;
    double disp_conserve_tick;
    double disp_pause_ms = 1;
    double disp_pause_rec_ms = 1000;

    // misc parameters
    int recPosition = 0;
    const string window_name ("CAM_FEEDS");
    string file_str = CommonFileName();
    string overlay_text;
    vector<string> header_names;
    Size grid_size_disp;
    Size disp_size_disp;
    Size grid_size_rec;
    Size disp_size_rec;

    // start vector of capture constructors given cam indices
    const vector<int> eye_device_vec (eye_cam_ids,
                                      eye_cam_ids + sizeof(eye_cam_ids) /
                                      sizeof(int));


    vector<VideoCapture> eye_track_devices (eye_device_vec.size());


    // initialize devices and return first frames
    vector<Mat> init_images = InitCam(eye_device_vec, eye_track_devices);


    // setup output video file for each camera
    vector<VideoWriter> eye_track_rec (eye_device_vec.size());
    WriterInitialize(eye_track_rec, file_str, frames_per_sec, init_images);


    // setup n cells and frame dimensions for displaying captured video
    VideoDisplaySetup(grid_size_disp, disp_size_disp, init_images, resize_disp_imgs);
    VideoDisplaySetup(grid_size_rec, disp_size_rec, init_images, resize_rec_imgs);


    // Create display window
    namedWindow(window_name, WINDOW_AUTOSIZE);
    createTrackbar("Toggle Recording:", window_name, &recPosition, 1, RecSwitch, NULL);
    cout << "\nPress ESC key on cam window to exit program : " << endl;


    // output csv file
    ofstream out_file ("data__0" + file_str + ".csv");
    header_names.push_back("timestamp");
    header_names.push_back("loopduration");
    WriteCSVHeaders(out_file, header_names);


    // Update clock info
    UpdateTick(start_rec_tick);
    UpdateTick(tick_onset);
    UpdateTick(timer_refresh);
    UpdateTick(disp_conserve_tick);
    tick_offset = tick_onset + (getTickFrequency() / frames_per_sec);


    while (true)
    {
        // calculate loop time duration
        TimeDuration(loop_duration, tick_onset, tick_offset);


        // decide if to pause if loop is too fast
        // if loop is too slow you must lower frame rate capture
        if (CaptureWait(loop_duration, sampling_rate_ms, exit_key))
            break;


        // start loop timer
        UpdateTick(tick_onset);
        ++frame_counter;


        // capture frames from each device and put into mat vector
        vector<Mat> captured_frames = CaptureMultipleFrames(eye_track_devices, grab_timestamp);


        // start recording if slider switched
        if (recPosition == 1)
        {

            // display images at a slower rate when recording
            if (CheckForFrameUpdate(disp_conserve_tick, tick_offset, disp_pause_rec_ms))
            {
                ImDisplay(window_name, captured_frames, disp_size_rec, grid_size_rec, resize_rec_imgs);
                DisplayInfo(window_name, overlay_text, calc_fps);
            }

            // write data
            WriteData(out_file,
                      grab_timestamp,
                      loop_duration);

            // write images to separate files
            WriteFrames(eye_track_rec, captured_frames);
        }
        else
        {
            // display images together in one frame at a faster rate
            if (CheckForFrameUpdate(disp_conserve_tick, tick_offset, disp_pause_ms))
            {
                ImDisplay(window_name, captured_frames, disp_size_disp, grid_size_disp, resize_disp_imgs);
            }
        }

        // update loop data
        CalcFPS(calc_fps, frames_per_sec, frame_counter, timer_refresh, tick_offset, fps_interval);
        TimeElapsed(output_rec_time, start_rec_tick, tick_offset);
        UpdateTick(tick_offset);

    }

    out_file.close();

    return 0;

}
