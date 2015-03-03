#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <future>
#include <chrono>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace cv;
using namespace std;
using namespace std::chrono;
using namespace boost;
namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace pt = boost::posix_time;

void colorizeMat(Mat &img)
{
    for (int m = 0; m < img.rows; m++) {
        for (int n = 0; n < img.cols; n++) {
            img.at<Vec3b>(m,n)[0] = 255;
            img.at<Vec3b>(m,n)[1] = 0;
            img.at<Vec3b>(m,n)[2] = 0;
        }
    }
}

int64 findNearestTimeStamp(double tick, vector<int> ticks)
{
    vector<double> tickDiff(ticks.size());
    for (int i = 0; i < ticks.size(); i++) {
        tickDiff[i] = abs(static_cast<double>(ticks[i]-tick));
    }
    vector<double>::iterator mins = min_element(tickDiff.begin(), tickDiff.end());
    int64 min_index = distance(tickDiff.begin(), mins);
    return min_index;
}


Mat imDisplay(string title, vector<Mat>& imgs,
              Size& imSize, Size& gridSize, double& scaler, int &rec)
{
    Mat out_img = Mat::zeros(imSize.height, imSize.width, CV_8UC3);
    int n_imgs = static_cast<int>(imgs.size());
    int img_idx = 0;
    int out_h_start = 0;
    int out_w_start = 0;
    int out_h_end = 0;
    int out_w_end = 0;

    for (int m=0; m < gridSize.height; m++)
    {
        out_w_start = 0;
        for (int n=0; n < gridSize.width; n++)
        {
            if (img_idx < n_imgs)
            {
                Mat in_img;

                resize(imgs[img_idx], in_img,
                       Size(), scaler, scaler, INTER_NEAREST);

                Size in_img_sz = in_img.size();

                out_h_end = out_h_start + (in_img_sz.height-1);
                out_w_end = out_w_start + (in_img_sz.width-1);

                Range in_roi_h = Range(0, in_img_sz.height - 1);
                Range in_roi_w = Range(0, in_img_sz.width - 1);

                Range out_roi_w = Range(out_w_start, out_w_end);
                Range out_roi_h = Range(out_h_start, out_h_end);

                //cout << "\n--IN RECT:" << img_idx << endl;
                //cout << in_roi_w.start << endl;
                //cout << in_roi_h.start << endl;
                //cout << in_roi_w.end << endl;
                //cout << in_roi_h.end << endl;
                //cout << "\n--OUT RECT:" << img_idx << endl;
                //cout << out_roi_w.start << endl;
                //cout << out_roi_h.start << endl;
                //cout << out_roi_w.end << endl;
                //cout << out_roi_h.end << endl;

                in_img(in_roi_h, in_roi_w).copyTo(out_img(out_roi_h, out_roi_w));
            }
            img_idx++;
            out_w_start = out_w_end + 1;
        }
        out_h_start = out_h_end + 1;
    }

    if (rec == 1) {
        circle(out_img, Point(15, 15), 14, Scalar(0, 0, 255), -1, 8);
        putText(out_img, "REC", Point(30, 20), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255));
    }
    namedWindow(title);
    imshow(title, out_img);
    return out_img;
}

vector<VideoCapture> initCapDevices(const vector<int> usb_ids, const vector<string> url_ids, vector<Mat> &imgs)
{
    size_t n_usb = usb_ids.size();
    size_t n_url = url_ids.size();
    vector<VideoCapture> devices (n_usb + n_url);
    string url;
    int device_counter = 0;

    if (n_usb != 0) {
        for (int i=0; i < n_usb; i++){
            cout << "\nTrying USB device: \n" << usb_ids[i] << endl;
            devices[device_counter].open(usb_ids[i]);

            if (!devices[device_counter].read(imgs[device_counter])){
                cout << "\nCannot open video device: " << usb_ids[i] << endl;
                throw;
            }
            colorizeMat(imgs[device_counter]);
            device_counter++;
        }
    }

    if (n_url != 0) {
        for (int i=0; i < n_url; i++){
            cout << "\nTrying URL: \n" << url_ids[i] << endl;
            devices[device_counter].open(url_ids[i]);

            if (!devices[device_counter].read(imgs[device_counter])){
                cout << "\nCannot open video device: " << url_ids[i] << endl;
                throw;
            }
            colorizeMat(imgs[device_counter]);
            device_counter++;
        }
    }

    return devices;
}

vector<VideoWriter> initWriteDevices(vector<VideoCapture>& capDevices, vector<string>& paths, double& fps)
{
    vector<VideoWriter> vidWriteVec(capDevices.size());
    int codec = VideoWriter::fourcc('M','P','4','V');
    string fileExt (".m4v");
    //int codec = VideoWriter::fourcc('M','J','P','G');
    //string fileExt (".avi");

    string fileStr;

    for (int i=0; i < capDevices.size(); i++)
    {
        Mat tempImg;
        capDevices[i] >> tempImg;
        fileStr = paths[0] + "/camera_" + to_string(i) + "_" + paths[1] + fileExt;
        vidWriteVec[i].open(fileStr, codec, fps, tempImg.size(), true);
    }
    return vidWriteVec;
}

vector<string> makeDirectory(string &rootPath)
{
    pt::ptime now = pt::second_clock::local_time();
    std::stringstream ts;
    ts << now.date().year() << "_"
       << static_cast<int>(now.date().month()) << "_"
       << now.date().day() << "-at-"
       << now.time_of_day().hours() << "_"
       << now.time_of_day().minutes();

    std::cout << ts.str() << std::endl;
    string file_prefix (ts.str());
    string folder_name = {rootPath + file_prefix};

    const fs::path boostPath (folder_name);

    try
    {
        fs::create_directories(boostPath);
    } catch (const fs::filesystem_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    vector<string> paths;
    paths.push_back(folder_name);
    paths.push_back(file_prefix);

    return paths;
}


int msTimeStamp(time_point<high_resolution_clock> &tick)
{
    time_point<high_resolution_clock> tock = high_resolution_clock::now();
    duration<double> tdur = tock-tick;
    milliseconds ms = duration_cast<milliseconds>(tdur);
    return (int)ms.count();
}

void printTime(string idx, time_point<high_resolution_clock> ts)
{
    cout << idx << ": " << msTimeStamp(ts) << endl;
}

void recSwitch(int recPosition, void*)
{
    cout << "\nRec = " << recPosition << endl;
}

void removeDirectory(string &folder)
{
    const fs::path boostPath (folder);

    try
    {
        boost::uintmax_t nr = fs::remove_all(boostPath);
    } catch (const fs::filesystem_error& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void videoDisplaySetup(Size& grid, Size& imdim, vector<Mat>& imgs, double& scaler)
{
    // find number of images for placement
    int n_imgs = static_cast<int>(imgs.size());

    // calculate m rows and n cols based on num images
    int n_cols = static_cast<int>(ceil(sqrt(n_imgs)));
    int m_rows = static_cast<int>(ceil(static_cast<double>(n_imgs)/static_cast<double>(n_cols)));

    // small matrix which holds x,y dims for later max function
    Mat tmp_heights = Mat_<double>(m_rows, n_cols);
    Mat tmp_widths = Mat_<double>(m_rows, n_cols);

    int dev_idx = 0;

    for (int m=0; m < m_rows; m++)
    {
        for (int n=0; n < n_cols; n++)
        {
            if (dev_idx < n_imgs)
            {
                // place x,y size of current image in matrix
                tmp_heights.at<double>(m,n) = imgs[dev_idx].rows * scaler;
                tmp_widths.at<double>(m,n) = imgs[dev_idx].cols * scaler;
            } else {
                tmp_heights.at<double>(m,n) = 0;
                tmp_widths.at<double>(m,n) = 0;
            }
            dev_idx++;
        }
    }

    // find mat size for each dimension
    double img_height = 0;
    double img_width = 0;

    // max height for each row in grid
    vector<double> max_row(m_rows, 0);
    for (int m=0; m < m_rows; m++){
        minMaxIdx(tmp_heights.row(m), 0, &max_row[m]);
        img_height += max_row[m];
    }

    // sum of widths for each row in grid, find max
    Mat tmp_w_max = Mat_<double>(m_rows, 1);
    for (int m=0; m < m_rows; m++) {
        tmp_w_max.at<double>(m,1) = 0;
    }
    for (int m=0; m < m_rows; m++) {
        for (int n=0; n < n_cols; n++) {
            tmp_w_max.at<double>(m,1) = tmp_w_max.at<double>(m,1) + tmp_widths.at<double>(m,n);
        }
    }
    minMaxIdx(tmp_w_max, 0, &img_width);

    imdim = Size(static_cast<int>(round(img_width+n_cols-1)), static_cast<int>(round(img_height+m_rows-1)));
    grid = Size(n_cols, m_rows);
}

void waitMilliseconds(int t)
{
    std::chrono::milliseconds ms (t);
    std::this_thread::sleep_for(ms);
}

void writeCSVHeaders(ofstream& file, string name, int n)
{
    for (int i=0; i < n; i++){
        file << name + to_string(i);

        if (i < n-1){
            file << ",";
        }
    }

    file << endl;
}

void writeTimeStampData(ofstream &file, vector<int64> &timestamps)
{
    for (int i=0; i < timestamps.size(); i++) {
        file << timestamps[i];
        if (i < timestamps.size()-1){
            file << ",";
        }
    }

    file << endl;
}

int main(int argc, char* argv[])
{
    // handle program options
    string config_file;
    vector<int> usb_idx;
    vector<string> ip_url;
    string save_path;
    double fps;
    int fbl;
    double resize_disp_value;

    try {
        po::options_description generic_ops("Generic Options");
        generic_ops.add_options()
                ("help","Show help for usage of application options")
                ("config,c", po::value<string>(&config_file)->default_value("cam_opts.cfg"),
                 "Path to location of a configuration (.cfg) file. You can set common usage parameters in this file. Uses cam_opts.cfg by default.")
                ;

        po::options_description config_ops("Commande Line or Configuration File Options");
        config_ops.add_options()
                ("usb,u", po::value<vector<int>>(),
                 "USB ID:\n-Integer value of the USB camera index. May be used multiple times.\n-Examples: --usb=0 or -u 1")
                ("url,i", po::value<vector<string>>(),
                 "URL ID:\n-String of the full url for IP cameras, no quotes. May be used multiple times.\n-Examples: --url=http://112.0.0.1 or -i http://...")
                ("out,o", po::value<string>(&save_path)->default_value("data/"),
                 "OUT PATH:\n-Path to location for where to save the output files.\n-Example: --out=data/ or -o data/")
                ("fps,f", po::value<double>(&fps)->default_value(25),
                 "FRAMES PER SECOND:\n-Frames per second for all output videos, no matter the input fps.\n-Example: --fps=25 or -f 25")
                ("fbl,b", po::value<int>(&fbl)->default_value(25),
                 "FRAME BUFFER LENGTH:\n-Buffer size for holding consecutive frames. This will affect imshow latency.\n-Example: --fbl=10 or -b 10")
                ("res,r", po::value<double>(&resize_disp_value)->default_value(0.33),
                 "RESIZE SCALER:\n-Value corresponding to how much to resize the display image.\n-Example: --res=.5 or -r .5")
                ;

        po::options_description cmdline_options;
        cmdline_options.add(generic_ops).add(config_ops);

        po::options_description config_file_options;
        config_file_options.add(config_ops);

        po::positional_options_description p;
        p.add("usb", -1);

        po::variables_map vm;
        store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
        po::notify(vm);

        ifstream ifs(config_file.c_str());
        if (!ifs) {
            cout << "Could not open config file: " << config_file << "\n";
            return 0;
        }
        else {
            store(parse_config_file(ifs, config_file_options), vm);
            notify(vm);
        }

        if (vm.count("help")) {
            cout << "Help documentation for Cogdev Lab Cam Sync: Joseph M. Burling\n";
            cout << cmdline_options;
            return 0;
        }

        if (vm.count("usb")) {
            usb_idx = vm["usb"].as< vector<int> >();
        }

        if (vm.count("url")) {
            ip_url = vm["url"].as< vector<string> >();
        }

    }
    catch(std::exception& e)
    {
        cout << e.what() << "\n";
        return 1;
    }

    // collect number if input cameras
    int nCams = static_cast<int>(usb_idx.size() + ip_url.size());
    if (nCams == 0) {
        cout << "No camera ids or urls entered. Ending program." << endl;
        return 0;
    }

    // some additional parameters
    double frame_duration = 1000 / fps;
    double read_duration_ms = frame_duration * fbl;
    int presentTick;
    int recPosition = 0;
    bool REC_FLAG = false;
    int exit_key = 27;
    Size grid_size_disp;
    Size disp_size_disp;
    string window_name ("yoshida-view");

    // initialize cameras, get first frames
    vector<Mat> showImages(nCams);
    vector<VideoCapture> capVec = initCapDevices(usb_idx, ip_url, showImages);
    videoDisplaySetup(grid_size_disp, disp_size_disp, showImages, resize_disp_value);
    namedWindow(window_name, WINDOW_AUTOSIZE);
    createTrackbar("REC", window_name, &recPosition, 1, recSwitch, NULL);

    while (true) {
        for (int i = 0; i < nCams; i++) {
            capVec[i] >> showImages[i];
        }

        imDisplay(window_name, showImages, disp_size_disp, grid_size_disp, resize_disp_value, recPosition);

        if (recPosition == 1) {
            REC_FLAG = true;
            break;
        }

        if (waitKey(15) == exit_key) {
            break;
        }
    }

    if (REC_FLAG) {
        // make output directory
        vector<string> file_str = makeDirectory(save_path);

        // initialize writer
        vector<VideoWriter> writeVec = initWriteDevices(capVec, file_str, fps);

        // data writer
        ofstream out_file (file_str[0] + "/timeStamps_0_" + file_str[1] + ".csv");
        writeCSVHeaders(out_file, "timestamp", nCams+1);

        // make buffers
        int initBufferSize = 1;
        vector<vector<Mat>> presentBuffer(nCams, vector<Mat>(initBufferSize));
        vector<vector<Mat>> pastBuffer(nCams, vector<Mat>(initBufferSize));
        vector<vector<int>> presentTimeStamps(nCams, vector<int>(initBufferSize));
        vector<vector<int>> pastTimeStamps(nCams, vector<int>(initBufferSize));

        // start main clock
        time_point<high_resolution_clock> startTime;
        startTime = high_resolution_clock::now();

        // fill past buffer before main loop
        int pastTick = msTimeStamp(startTime);
        for (int i = 0; i < initBufferSize; i++) {
            for (int j = 0; j < nCams; j++) {
                Mat tempImg;
                capVec[j] >> tempImg;
                pastTimeStamps[j][i] = msTimeStamp(startTime);
                colorizeMat(tempImg);
                pastBuffer[j][i] = tempImg;
            }
        }

        // start looping through threads
        while (true) {

            // hold frames and timestamps for next loop iteration
            vector<future<void>> presentBufferThread;
            presentTick = msTimeStamp(startTime);
            for (int i = 0; i < nCams; i++)
            {
                presentBuffer[i].clear();
                presentTimeStamps[i].clear();
                presentBufferThread.emplace_back(async(launch::async,[&capVec,&presentBuffer,&startTime,&presentTimeStamps,&presentTick,&read_duration_ms,&frame_duration](int j)
                {
                    double loopBreakTime = static_cast<double>(presentTick) + read_duration_ms-2;
                    double currentLoopTime;
                    for (;;) {
                        capVec[j].grab();
                        presentTimeStamps[j].emplace_back(msTimeStamp(startTime));
                        Mat placeHolderImg;
                        capVec[j].retrieve(placeHolderImg);
                        presentBuffer[j].emplace_back(placeHolderImg);
                        currentLoopTime = static_cast<double>(msTimeStamp(startTime));
                        if (currentLoopTime >= loopBreakTime) {
                            break;
                        }
                    }
                }, i));
            }

            if (recPosition == 1) {
                vector<int64> capturedTimeStamps(nCams+1);
                int showCounter = 0;

                // process previous frames using timestamps
                for (double t = static_cast<double>(pastTick);
                     t < static_cast<double>(pastTick+read_duration_ms);
                     t = t+frame_duration) {

                    capturedTimeStamps[0] = static_cast<int64>(t); // loop time
                    vector<future<void>> readWriteBuffer;
                    showCounter++;

                    for (int i = 0; i < nCams; i++) {
                        readWriteBuffer.emplace_back(async(launch::async,[&pastBuffer,&pastTimeStamps,&capturedTimeStamps,&t,&writeVec,&showImages,&showCounter](int j)
                        {
                            int64 nearestTimeStamp = findNearestTimeStamp(t, pastTimeStamps[j]);
                            capturedTimeStamps[j+1] = pastTimeStamps[j][nearestTimeStamp];
                            Mat closestFrame = pastBuffer[j][nearestTimeStamp];
                            writeVec[j] << closestFrame;
                            if (showCounter == 1) {
                                showImages[j] = closestFrame;
                            }
                        }, i));
                    }

                    for (int i = 0; i < nCams; i++) {
                        readWriteBuffer[i].wait();
                    }

                    writeTimeStampData(out_file, capturedTimeStamps);
                }
            } else {
                for (int i = 0; i < nCams; i++) {
                    Mat lastFrame = pastBuffer[i].back();
                    showImages[i] = lastFrame;
                }
            }

            // show last image from buffer
            imDisplay(window_name, showImages, disp_size_disp, grid_size_disp, resize_disp_value, recPosition);

            //int junkTick = msTimeStamp(startTime);
            for (int i = 0; i < nCams; i++) {
                presentBufferThread[i].wait();
            }
            //cout << msTimeStamp(startTime)-junkTick << endl;

            if (waitKey(1) == exit_key) {
                destroyWindow(window_name);
                break;
            }

            pastBuffer = presentBuffer;
            pastTick = presentTick;
            pastTimeStamps = presentTimeStamps;
        }

        // release video devices
        for (int i=0; i < capVec.size(); i++) {
            writeVec[i].release();
        }

        // close data file
        out_file.close();
    }

    // release video devices
    for (int i=0; i < capVec.size(); i++) {
        capVec[i].release();

    }

    // try to rm empty dir if not recording
    //    if (REC_FLAG == false) {
    //        cout << "Removing Temp Folder: " << file_str[0] << endl;
    //        waitMilliseconds(1000);
    //        removeDirectory(file_str[0]);
    //    }

    return 0;
}
