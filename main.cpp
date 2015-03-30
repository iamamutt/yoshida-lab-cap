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
#include <RtAudio.h>

using namespace cv;
using namespace std;
using namespace std::chrono;
using namespace boost;
namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace pt = boost::posix_time;

struct AudioStruct {
    unsigned int channels;
    unsigned int samplerate;
    unsigned int buffersize;
    std::ofstream file;
};

void colorizeMat(Mat &img)
{
    for (int m = 0; m < img.rows; m++) {
        for (int n = 0; n < img.cols; n++) {
            img.at<Vec3b>(m,n)[0] = 0;
            img.at<Vec3b>(m,n)[1] = 0;
            img.at<Vec3b>(m,n)[2] = 255;
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
            out_w_start = out_w_end + 2;
        }
        out_h_start = out_h_end + 2;
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
    int device_counter = 0;

    if (n_usb != 0) {
        for (int i=0; i < n_usb; i++){
            cout << "\nTrying to load USB device: " << usb_ids[i] << endl;

            Mat tmpImg;
            int capCounter = 0;
            bool capFail = true;

            while (capFail) {
                ++capCounter;

                if (capCounter > 10) {
                    cerr << "failed to open device " << usb_ids[i] << " too many times!" << endl;
                    throw;
                }

                devices[device_counter].open(usb_ids[i]);

                if (!devices[device_counter].isOpened()) {
                    cerr << "Cam not found with ID: " << usb_ids[i] << endl;
                    continue;
                }

                if (!devices[device_counter].grab()) {
                    cerr << "Frame was not grabbed successfully for USB cam: " << usb_ids[i] << endl;
                    devices[device_counter].release();
                    continue;
                }

                if (!devices[device_counter].retrieve(tmpImg)){
                    cerr << "Frame was not decoded successfully for USB cam: " << usb_ids[i] << endl;
                    devices[device_counter].release();
                    continue;
                }

                cout << "Loading of usb device: " << usb_ids[i] << " ...successful!" << endl;
                capFail = false;

            }

            imgs[device_counter] = tmpImg.clone();
            device_counter++;
        }
    }

    if (n_url != 0) {
        for (int i=0; i < n_url; i++){
            cout << "\nTrying IP url:\n" << url_ids[i] << endl;

            Mat tmpImg;
            int capCounter = 0;
            bool capFail = true;

            while (capFail) {
                ++capCounter;

                if (capCounter > 10) {
                    cerr << "failed to open url too many times:\n" << url_ids[i] << endl;
                    throw;
                }

                devices[device_counter].open(url_ids[i]);

                if (!devices[device_counter].isOpened()) {
                    cerr << "Cam not found with url:\n" << url_ids[i] << endl;
                    continue;
                }

                if (!devices[device_counter].grab()) {
                    cerr << "Frame was not grabbed successfully for IP cam: " << url_ids[i] << endl;
                    devices[device_counter].release();
                    continue;
                }

                if (!devices[device_counter].retrieve(tmpImg)){
                    cerr << "Frame was not decoded successfully for IP cam: " << url_ids[i] << endl;
                    devices[device_counter].release();
                    continue;
                }

                cout << "Loading of IP url successful!:\n" << url_ids[i] << endl;
                capFail = false;

            }

            imgs[device_counter] = tmpImg.clone();
            device_counter++;
        }
    }

    return devices;
}

vector<VideoWriter> initWriteDevices(vector<VideoCapture>& capDevices, vector<string>& paths, double& fps, string &four_cc, string &ext)
{

    vector<VideoWriter> vidWriteVec(capDevices.size());
    int codec = VideoWriter::fourcc(four_cc[0], four_cc[1], four_cc[2], four_cc[3]);
    string fileExt ("." + ext);

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

void loadAudioDevices(vector<AudioStruct> &aud_strct,
                      vector<RtAudio::StreamParameters> &stream_pram, vector<int> &dev_vec,
                      unsigned int &sr, unsigned int &ac, unsigned int &ab,
                      vector<string> &file_str)
{
    for (int i=0; i < dev_vec.size(); i++) {
        aud_strct[i].samplerate = sr;
        aud_strct[i].channels = ac;
        aud_strct[i].buffersize = ab;
        aud_strct[i].file.open(file_str[0] + "/audio_" + to_string(dev_vec[i]) + "_32bit_" + to_string(sr) + "Hz_" + to_string(ac) + "ch_" + file_str[1] + ".pcm", std::ios::binary);
        stream_pram[i].deviceId = dev_vec[i];
        stream_pram[i].nChannels = ac;
        stream_pram[i].firstChannel = 0;
    }
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
    cout << "\nRecording = " << recPosition << endl;
}

int recordAudio(void *outputBuffer, void *inputBuffer,
                unsigned int nBufferFrames, double streamTime,
                RtAudioStreamStatus status, void *data)
{
    if (status) {
        std::cout << "Stream overflow detected!" << std::endl;
    }

    AudioStruct *inputData = (AudioStruct *) data;

    //std::cout << inputData->samplerate << std::endl;
    inputData->file.write((char *) inputBuffer, sizeof(float) * nBufferFrames * inputData->channels);

    return 0;
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
    vector<double> row_sums(m_rows, 0);
    for (int m=0; m < m_rows; m++) {
        for (int n=0; n < n_cols; n++) {
            row_sums[m] += tmp_widths.at<double>(m,n);
        }
    }
    auto max_col_it = std::max_element(std::begin(row_sums), std::end(row_sums));
    img_width = *max_col_it;

    imdim = Size(static_cast<int>(ceil(img_width+n_cols-1)), static_cast<int>(ceil(img_height+m_rows-1)));
    grid = Size(n_cols, m_rows);

    cout << "Display dim: "
         << imdim.width << " x " << imdim.height
         << "\nGrid size: "
         << grid.width << " x " << grid.height
         << endl;
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
    string window_name;
    string four_cc;
    string file_extension;
    vector<int> aud_idx;
    unsigned int sample_rate = 44100;
    unsigned int audio_channels;
    unsigned int audio_buffer;

    try {
        po::options_description generic_ops("Generic Options");
        generic_ops.add_options()
                ("help","Show help for usage of application options")
                ("config,c", po::value<string>(&config_file)->default_value("cam_opts.cfg"),
                 "CONFIG FILE:\n-Path to location of a configuration (.cfg) file. You can set common usage parameters in this file. Uses cam_opts.cfg by default.")
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
                ("fbl,b", po::value<int>(&fbl)->default_value(10),
                 "FRAME BUFFER LENGTH:\n-Buffer size for holding consecutive frames. This will affect imshow latency.\n-Example: --fbl=10 or -b 10")
                ("res,r", po::value<double>(&resize_disp_value)->default_value(0.5),
                 "RESIZE SCALER:\n-Value corresponding to how much to resize the display image.\n-Example: --res=.5 or -r .5")
                ("win,w", po::value<string>(&window_name)->default_value("YOSHIDA-VIEW"),
                 "WINDOW NAME:\n-Name to display at top of window.\n-Example: --win=stuff or -w stuff")
                ("codec", po::value<string>(&four_cc)->default_value("MJPG"),
                 "CODEC:\n-Upper case four letter code for the codec used to export video.\n-Example: --codec=MP4V")
                ("ext", po::value<string>(&file_extension)->default_value("avi"),
                 "EXTENSION:\n-Lower case three letter extension/wrapper for the video file.\n-Example: --ext=m4v")
                ("aud,a", po::value<vector<int>>(),
                 "AUDIO ID:\n-Integer value of the AUDIO device index. May be used multiple times.\n-Examples: --aud=0 or -a 1")
                ("srate", po::value<unsigned int>(&sample_rate)->default_value(44100),
                 "AUDIO SAMPLE RATE:\n-Sample rate for all audio input devices.\n-Example: -srate=48000")
                ("channels", po::value<unsigned int>(&audio_channels)->default_value(2),
                 "AUDIO CHANNELS:\n-Number of channels for each audio input device.\n-Example: -channels=2")
                ("abuffer", po::value<unsigned int>(&audio_buffer)->default_value(256),
                 "AUDIO BUFFER:\n-Audio buffer size.\n-Example: -abuffer=512")
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
            cout << "Could not open config file. Make sure cam_opts.cfg exists along with this program or that you specify a path to a custom .cfg file using the --cfg argument\n";
            return 0;
        }
        else {
            store(parse_config_file(ifs, config_file_options), vm);
            notify(vm);
        }

        if (vm.count("help")) {
            cout << "Help documentation for Cogdev Lab Cam Sync:\nAuthor: Joseph M. Burling\n";
            cout << cmdline_options;
            return 0;
        }

        if (vm.count("usb")) {
            usb_idx = vm["usb"].as< vector<int> >();
        }

        if (vm.count("url")) {
            ip_url = vm["url"].as< vector<string> >();
        }

        if (vm.count("aud")) {
            aud_idx = vm["aud"].as< vector<int> >();
        }

        if (four_cc.size() != 4) {
            cout << "Please enter a codec with at least four characeters. see FOURCC.org." << endl;
            return 0;
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
    double loopBreakTime;
    int showCounter;
    int recPosition = 0;
    bool REC_FLAG = false;
    int exit_key = 27;
    Size grid_size_disp;
    Size disp_size_disp;

    // audio objects
    int nAuds = static_cast<int>(aud_idx.size());
    vector<RtAudio> audio(nAuds);
    vector<AudioStruct> audio_parameters(nAuds);
    vector<RtAudio::StreamParameters> stream_parameters(nAuds);

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

        if (waitKey(static_cast<int>(frame_duration)) == exit_key) {
            break;
        }
    }

    if (REC_FLAG) {
        // make output directory
        vector<string> file_str = makeDirectory(save_path);

        // initialize writer
        vector<VideoWriter> writeVec = initWriteDevices(capVec, file_str, fps, four_cc, file_extension);

        // data writer
        ofstream cam_ts (file_str[0] + "/cameraTimeStamps_0_" + file_str[1] + ".csv");
        writeCSVHeaders(cam_ts, "camts", nCams+1);

        // audio data file
        ofstream aud_ts (file_str[0] + "/audioTimeStamps_0_" + file_str[1] + ".csv");
        writeCSVHeaders(aud_ts, "audts", nAuds+1);

        // make buffers
        int initBufferSize = 1;
        vector<vector<Mat>> presentBuffer(nCams, vector<Mat>(initBufferSize));
        vector<vector<Mat>> pastBuffer(nCams, vector<Mat>(initBufferSize));
        vector<vector<int>> presentTimeStamps(nCams, vector<int>(initBufferSize));
        vector<vector<int>> pastTimeStamps(nCams, vector<int>(initBufferSize));

        // audio timestamps
        vector<int64> audio_timestamps(nAuds+1, 0);

        // start main clock
        time_point<high_resolution_clock> startTime;
        startTime = high_resolution_clock::now();

        // fill past buffer before main loop
        int pastTick = 0;
        for (int i = 0; i < initBufferSize; i++) {
            for (int j = 0; j < nCams; j++) {
                Mat tempImg;
                capVec[j] >> tempImg;
                pastTimeStamps[j][i] = 0;
                colorizeMat(tempImg);
                pastBuffer[j][i] = tempImg;
            }
        }

        //initialize audio devices
        if (nAuds > 0) {
            unsigned int n_devices = audio[0].getDeviceCount();
            cout << "Number of Audio Devices: " << n_devices-1 << endl;

            if (n_devices < 1) {
                std::cout << "\nNo audio input devices found!\n";
                return 0;
            }

            loadAudioDevices(audio_parameters, stream_parameters,
                             aud_idx, sample_rate, audio_channels,
                             audio_buffer, file_str);

            for (int i=0; i < nAuds; ++i) {
                try {
                    audio[i].openStream(NULL, &stream_parameters[i], RTAUDIO_FLOAT32,
                                        sample_rate, &audio_buffer, &recordAudio, (void *)&audio_parameters[i]);
                    //audio[i].setStreamTime(static_cast<double>(msTimeStamp(startTime))/1000);
                    audio[i].setStreamTime(0);

                }
                catch (RtAudioError& e) {
                    e.printMessage();
                    return 0;
                }
            }
        }

        // start looping through threads
        while (true) {

            // hold frames and timestamps for next loop iteration
            vector<future<void>> presentBufferThread;
            presentTick = msTimeStamp(startTime); // main start timestamp
            loopBreakTime = static_cast<double>(presentTick) + read_duration_ms-2;

            // Start threaded buffer fill until reach end time
            for (int i = 0; i < nCams; i++) {
                presentBuffer[i].clear();
                presentTimeStamps[i].clear();
                presentBufferThread.emplace_back(
                            async(launch::async,[
                                  &capVec,
                                  &presentBuffer,
                                  &startTime,
                                  &presentTimeStamps,
                                  &loopBreakTime
                                  ](int j)
                {
                    double currentLoopTime;
                    for (;;) {
                        // grab, timestamp, decode, push to buffer, check time
                        Mat placeHolderImg;

                        capVec[j].grab();
                        presentTimeStamps[j].emplace_back(msTimeStamp(startTime));
                        capVec[j].retrieve(placeHolderImg);
                        presentBuffer[j].emplace_back(placeHolderImg.clone());
                        currentLoopTime = static_cast<double>(msTimeStamp(startTime));
                        if (currentLoopTime >= loopBreakTime) {
                            break;
                        }
                    }
                }, i));
            }

            if (recPosition == 1) { // if 0 then pause recording

                // deal with audio data
                if (nAuds > 0) {

                    vector<future<void>> audio_ts_thread;

                    for (int i=0; i < nAuds; ++i) {

                        // async timestamp collection
                        audio_ts_thread.emplace_back(
                                    async(launch::async,[
                                          &audio,
                                          &audio_timestamps
                                          ](int j)
                        {
                            // start stream if not running
                            if (audio[j].isStreamOpen() && !audio[j].isStreamRunning()) {
                                try {
                                    audio[j].startStream();
                                }
                                catch ( RtAudioError& e ) {
                                    e.printMessage();
                                }
                            }
                            audio_timestamps[j+1] = static_cast<int64>(round(audio[j].getStreamTime() * 1000));
                        }, i));
                    }

                    // audio timestamp since startTime
                    audio_timestamps[0] = msTimeStamp(startTime);

                    for (int i=0; i < nAuds; ++i) {
                        audio_ts_thread[i].wait();
                    }

                    writeTimeStampData(aud_ts, audio_timestamps);
                }

                // first slot is the common time stamp, loop time
                vector<int64> collectedCamTs(nCams+1);
                showCounter = 0;

                // process previous frames using timestamps
                for (double t = static_cast<double>(pastTick);
                     t < static_cast<double>(pastTick+read_duration_ms);
                     t = t+frame_duration) {

                    showCounter++; // for copying only the last image to view
                    collectedCamTs[0] = static_cast<int64>(t);
                    vector<future<void>> readWriteBuffer;

                    // write asynchronously
                    for (int i = 0; i < nCams; i++) {
                        readWriteBuffer.emplace_back(
                                    async(launch::async,[
                                          &pastBuffer,
                                          &pastTimeStamps,
                                          &collectedCamTs,
                                          &t,
                                          &writeVec,
                                          &showImages,
                                          &showCounter
                                          ](int j)
                        {
                            int64 nearestTimeStamp = findNearestTimeStamp(t, pastTimeStamps[j]);
                            collectedCamTs[j+1] = pastTimeStamps[j][nearestTimeStamp];
                            writeVec[j] << pastBuffer[j][nearestTimeStamp];
                            if (showCounter == 1) {
                                showImages[j] = pastBuffer[j].back();
                            }
                        }, i));
                    }

                    // wait for all devices to write one frame
                    for (int i = 0; i < nCams; i++) {
                        readWriteBuffer[i].wait();
                    }

                    // write timestamp data
                    writeTimeStampData(cam_ts, collectedCamTs);
                }

            } else {
                // just make viewing images only
                for (int i = 0; i < nCams; i++) {
                    showImages[i] = pastBuffer[i].back();
                }


                // pause stream
                for (int i=0; i < nAuds; ++i) {
                    if (audio[i].isStreamOpen() && audio[i].isStreamRunning()) {
                        audio[i].stopStream();
                    }
                }
            }

            // show last image from buffer
            imDisplay(window_name, showImages, disp_size_disp, grid_size_disp, resize_disp_value, recPosition);

            for (int i = 0; i < nCams; i++) {
                presentBufferThread[i].wait();
            }

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
        cam_ts.close();

        // close audio
        if (nAuds > 0) {
            aud_ts.close();
            for (int i=0; i < aud_idx.size(); ++i) {
                if (audio[i].isStreamOpen()) {
                    if (audio[i].isStreamRunning()) audio[i].stopStream();
                    audio[i].closeStream();
                    audio_parameters[i].file.close();
                }
            }
        }
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
