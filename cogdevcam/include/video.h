/**
    project: cogdevcam
    source file: video
    description:

    @author Joseph M. Burling
    @version 0.9.0 11/26/2017
*/

#ifndef COGDEVCAM_VIDEO_H
#define COGDEVCAM_VIDEO_H

#include "tools.h"
#include <chrono>
#include <opencv2/opencv.hpp>
#include <string>
#include <utility>
#include <vector>

namespace video {
using VideoDuration = std::chrono::duration<double, timing::milli>;
using VideoClock    = timing::Clock<VideoDuration>;

class Properties;

int               fourCCStr2Int(std::string &fourcc);
video::Properties getCaptureProperties(cv::VideoCapture &reader);

class Properties
{
  public:
    Properties() = default;

    Properties(const Properties &properties) { merge(properties); };

    explicit Properties(cv::VideoCapture &reader) { merge(reader); };

    explicit Properties(std::string _fourcc,
                        double      _fps          = 0,
                        int         _frame_width  = 0,
                        int         _frame_height = 0)
    {
        merge(_fourcc, _fps, _frame_width, _frame_height);
    };

    bool
    empty() const
    {
        return emptyFPS() && emptyCodec() && emptySize();
    };

    bool
    emptyFPS() const
    {
        return fps <= 0;
    }

    bool
    emptyCodec() const
    {
        return fourcc.empty() || codec < 0;
    }

    bool
    emptySize() const
    {
        return frame_height == 0 || frame_width == 0;
    }

    void
    check()
    {
        setProps(fourcc, fps, frame_width, frame_height, false);
    };

    void
    merge()
    {
        setProps(__fourcc, __fps, __frame_width, __frame_height, false);
    };

    void
    merge(const std::string &_fourcc,
          double             _fps          = 0,
          int                _frame_width  = 0,
          int                _frame_height = 0,
          bool               skip_checks   = true)
    {
        setProps(_fourcc, _fps, _frame_width, _frame_height, skip_checks);
    };

    void
    merge(const Properties &props, bool skip_checks = true)
    {
        setProps(props, skip_checks);
    };

    void
    merge(cv::VideoCapture &reader, bool skip_checks = true)
    {
        setProps(reader, skip_checks);
    };

    void
    print() const
    {
        std::cout << "- capture property: "
                     "CAP_PROP_FPS "
                  << fps << "\n";
        std::cout << "- capture property: "
                     "CAP_PROP_FRAME_HEIGHT "
                  << frame_height << "\n";
        std::cout << "- capture property: "
                     "CAP_PROP_FRAME_WIDTH "
                  << frame_width << "\n";
        std::cout << "- capture property: "
                     "CAP_PROP_FOURCC: "
                  << fourcc << " : " << std::to_string(codec) << "\n";
    };

    std::string fourcc       = "";
    double      fps          = 0;
    int         frame_height = 0;
    int         frame_width  = 0;
    int         codec        = 0;

  private:
    std::string __fourcc       = "MJPG";
    double      __fps          = 240.0;
    int         __frame_height = 480;
    int         __frame_width  = 640;

    void
    setProps(const std::string &_fourcc,
             double             _fps          = 0,
             int                _frame_width  = 0,
             int                _frame_height = 0,
             bool               skip_checks   = true)
    {
        Properties props;
        props.fourcc       = _fourcc;
        props.fps          = _fps;
        props.frame_width  = _frame_width;
        props.frame_height = _frame_height;
        setProps(props, skip_checks);
    };

    void
    setProps(cv::VideoCapture &reader, bool skip_checks = true)
    {
        auto reader_props = getCaptureProperties(reader);
        setProps(reader_props, skip_checks);
    };

    void
    setProps(const Properties &obj, bool skip_checks = true)
    {
        if (obj.empty()) return;

        if (obj.fps > 0)
        {
            if (skip_checks)
            {
                fps = obj.fps;
            } else
            {
                if (fps <= 0) fps = obj.fps;
            }
        }
        if (!obj.fourcc.empty())
        {
            if (skip_checks)
            {
                fourcc = obj.fourcc;
                codec  = fourCCStr2Int(fourcc);
            } else
            {
                if (fourcc.empty())
                {
                    fourcc = obj.fourcc;
                    codec  = fourCCStr2Int(fourcc);
                }
            }
        }
        if (obj.codec > 0)
        {
            if (skip_checks)
            {
                codec = obj.codec;
            } else
            {
                if (codec <= 0)
                {
                    codec = obj.codec;
                }
            }
        }
        if (obj.frame_width > 0)
        {
            if (skip_checks)
            {
                frame_width = obj.frame_width;
            } else
            {
                if (frame_width <= 0)
                {
                    frame_width = obj.frame_width;
                }
            }
        }
        if (obj.frame_height > 0)
        {
            if (skip_checks)
            {
                frame_height = obj.frame_height;
            } else
            {
                if (frame_height <= 0)
                {
                    frame_height = obj.frame_height;
                }
            }
        }
    };
};

Properties
getEmptyProps()
{
    Properties props;
    return props;
}

int
fourCCStr2Int(std::string &fourcc)
{
    int codec = 0;
    if (!fourcc.empty() && fourcc.size() == 4)
    {
        codec = cv::VideoWriter::fourcc(
          fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
    } else
    {
        throw err::Runtime("FOUR_CC codec string was empty.");
    }
    return codec;
}

void
setCaptureProperties(cv::VideoCapture &reader, Properties &obj)
{
    if (obj.codec != 0)
    {
        reader.set(cv::CAP_PROP_FOURCC, obj.codec);
    }
    if (obj.fps != 0)
    {
        reader.set(cv::CAP_PROP_FPS, obj.fps);
    }
    if (obj.frame_height != 0)
    {
        reader.set(cv::CAP_PROP_FRAME_HEIGHT, obj.frame_height);
    }
    if (obj.frame_width != 0)
    {
        reader.set(cv::CAP_PROP_FRAME_WIDTH, obj.frame_width);
    }
};

Properties
getCaptureProperties(cv::VideoCapture &reader)
{
    auto        codec  = reader.get(cv::CAP_PROP_FOURCC);
    auto        fps    = reader.get(cv::CAP_PROP_FPS);
    auto        width  = reader.get(cv::CAP_PROP_FRAME_WIDTH);
    auto        height = reader.get(cv::CAP_PROP_FRAME_HEIGHT);
    std::string EXT_str;

    if (codec > 0)
    {
        auto ex = static_cast<int>(codec);
        // Transform from int to char via Bitwise operators
        char EXT[] = {(char)(ex & 0XFF),
                      (char)((ex & 0XFF00) >> 8),
                      (char)((ex & 0XFF0000) >> 16),
                      (char)((ex & 0XFF000000) >> 24),
                      0};
        EXT_str    = static_cast<std::string>(EXT);
    } else
    {
        codec = 0;
    }

    Properties props;
    props.fps          = fps;
    props.frame_height = static_cast<int>(height);
    props.frame_width  = static_cast<int>(width);
    props.codec        = static_cast<int>(codec);
    props.fourcc       = EXT_str;

    return props;
};

struct VideoFile
{
    std::string full_path = "";
    std::string folder    = ".";
    std::string stem      = "";
    std::string ext       = "";
    std::string type      = "";
    int         index     = -1;
};

class Timestamps
{
  public:
    using timets_t = typename VideoClock::ctype;
    Timestamps()   = default;

    void
    setClock(timing::TimePoint start_time_ms)
    {
        timestamp_timer.set(start_time_ms);
    };

    void
    openTimestampStream(VideoFile file_info)
    {
        copyTimestampFileInfo(file_info);
        filename         = timestamp_file_info.full_path;
        std::string stem = timestamp_file_info.stem.empty() ?
                             "" :
                             timestamp_file_info.stem + "/";
        if (filename.empty())
        {
            filename = timestamp_file_info.folder + "/" + stem + "video_ts" +
                       misc::zeroPadStr(file_info.index) + ".txt";
        }
        misc::makeDirectory(filename);
        std::ofstream ts_stream(filename);
        ts_stream << std::fixed << std::setprecision(5);
        timestamp_stream = std::move(ts_stream);
    };

    bool
    isTimeOpen()
    {
        return write_timestmaps && timestamp_stream.is_open();
    };

    timets_t
    getTimestamp()
    {
        return timestamp_timer.elapsed();
    };

    void
    closeTime()
    {
        if (write_timestmaps && isTimeOpen())
        {
            timestamp_stream.close();
        }
    };

    void
    writeTime(timets_t ts = 0)
    {
        if (!write_timestmaps) return;
        if (ts != 0) ts = getTimestamp();
        if (isTimeOpen()) timestamp_stream << ts << "\n";
    };

    bool
    useTimestampWriter() const
    {
        return write_timestmaps;
    }

  protected:
    const VideoFile &
    getTimestampFileInfo() const
    {
        return timestamp_file_info;
    }

    void
    copyTimestampFileInfo(VideoFile _timestamp_file_info)
    {
        timestamp_file_info = std::move(_timestamp_file_info);
        write_timestmaps    = true;
    }

  private:
    video::VideoClock timestamp_timer;
    VideoFile         timestamp_file_info;
    std::string       filename;
    std::ofstream     timestamp_stream;
    bool              write_timestmaps = false;
};

class Reader
{
  protected:
    Reader() = default;

  public:
    explicit Reader(int _input)
      : is_usb(true), dev_id_int(_input), dev_id(std::to_string(_input)){};

    explicit Reader(const std::string &_input)
      : is_usb(false), dev_id_str(_input), dev_id(_input){};

    cv::Mat
    readImage()
    {
        readNextFrame();
        return mat;
    };

    uint64_t
    frame() const
    {
        return frameNumber;
    }

    void
    openReader(const Properties &capture_props = getEmptyProps(),
               size_t            n_attempts    = 10)
    {
        if (!capture_props.empty())
        {
            setReaderProperties(capture_props, false, true);
        }

        bool cap_failed = true;
        for (auto j = 0; j < n_attempts; ++j)
        {
            timing::sleep::sec(0.5);
            if (!openCaptureDevice()) continue;
            if (!readNextFrame()) continue;
            cap_failed = false;
            std::cout << "\n\nSUCCESS!\n\n";
            read_props.merge(read_props);
            read_props.frame_width  = mat.cols;
            read_props.frame_height = mat.rows;
            std::cout << "Size:  W=" << read_props.frame_width
                      << ", H=" << read_props.frame_height << "\n";
            break;
        }

        if (cap_failed)
        {
            throw err::Runtime("Failed too many times trying to open device!");
        }
    };

    void
    closeReader()
    {
        if (reader.isOpened()) reader.release();
    };

    Properties
    getReaderProperties(bool from_capture = false)
    {
        if (from_capture) return getCaptureProperties(reader);
        return read_props;
    };

    void
    setReaderProperties(const Properties &capture_props,
                        bool              skip_checks  = true,
                        bool              also_set_cap = true)
    {
        read_props.merge(capture_props, skip_checks);
        if (also_set_cap) setCaptureProperties(reader, read_props);
    };

  private:
    bool             is_usb      = true;
    int              dev_id_int  = -1;
    std::string      dev_id_str  = "";
    std::string      dev_id      = "";
    uint64_t         frameNumber = 0;
    cv::Mat          mat;
    Properties       read_props;
    cv::VideoCapture reader;

    bool
    openCaptureDevice()
    {
        bool opened = false;
        if (!reader.isOpened())
        {
            std::cout << "\nTrying to open device:\n " << dev_id << "\n";
            opened = is_usb ? reader.open(dev_id_int) : reader.open(dev_id_str);
        }
        if (!opened)
        {
            std::cerr << "Cam not detected with input:\n " << dev_id << "\n";
        }
        return opened;
    };

    bool
    readNextFrame()
    {
        if (reader.isOpened())
        {
            if (!reader.grab())
            {
                std::cerr << "Frame was not grabbed successfully for device:\n "
                          << dev_id << "\n";
                return false;
            }
            if (!reader.retrieve(mat))
            {
                std::cerr << "Frame was not decoded successfully for device:\n "
                          << dev_id << "\n";
                return false;
            }
        } else
        {
            return false;
        }
        frameNumber++;
        return true;
    };
};

class Writer
{
  public:
    explicit Writer(VideoFile file_info)
    {
        setWriterStream(std::move(file_info));
    };

    void
    openWriter(const Properties &writer_properties = getEmptyProps())
    {
        if (!use_writer) return;
        write_props.merge(writer_properties, false);
        if (write_props.empty())
        {
            write_props.merge();
        }
        if (write_props.emptyFPS())
        {
            std::cerr << "Write stream has invalid frames per second\n";
        }
        if (write_props.emptySize())
        {
            std::cerr << "Write stream has invalid image dimensions\n";
        }
        if (write_props.emptyCodec())
        {
            std::cerr << "Write stream has invalid codec info\n";
        }

        write_props.merge();
        openWriteStream();
    };

    void
    writeImage(const cv::Mat &img)
    {
        if (!use_writer || !writer.isOpened()) return;
        writer.write(img);
    };

    void
    closeWriter()
    {
        if (!use_writer) return;
        if (writer.isOpened()) writer.release();
    };

    void
    setWriterProperties(Properties properties  = getEmptyProps(),
                        bool       skip_checks = false)
    {
        if (properties.empty()) properties.merge();
        write_props.merge(properties, skip_checks);
    };

    VideoFile
    getVideoFileInfo()
    {
        return writer_file_info;
    }

  protected:
    Writer() = default;

    void
    copyWriterFileInfo(VideoFile _writer_file_info)
    {
        writer_file_info = std::move(_writer_file_info);
    };

  private:
    std::string     video_out_vid_file = "";
    bool            use_writer         = false;
    Properties      write_props;
    cv::VideoWriter writer;
    VideoFile       writer_file_info;

    void
    setWriterStream(VideoFile file_info)
    {
        use_writer = !file_info.stem.empty();
        if (!use_writer) return;
        copyWriterFileInfo(file_info);
        camFile(writer_file_info);
    };

    void
    openWriteStream(std::string filename = "")
    {
        if (filename.empty()) filename = video_out_vid_file;
        auto img_size = cv::Size(
          write_props.frame_width, write_props.frame_height);
        writer.open(
          filename, write_props.codec, write_props.fps, img_size, true);
        auto opened = writer.isOpened();
        if (!opened)
        {
            write_props.print();
            throw err::Runtime("Could not open file \"" + filename +
                               "\" for write");
        }
    };

    void
    camFile(VideoFile &file_info)
    {
        auto        filename = file_info.full_path;
        auto        ext      = file_info.ext;
        std::string type  = file_info.type.empty() ? "" : "_" + file_info.type;
        auto        index = misc::zeroPadStr(file_info.index);
        std::string stem  = file_info.stem.empty() ? "" : file_info.stem + "/";
        if (filename.empty())
        {
            filename = file_info.folder + "/" + stem + "video" + type + index;
        }
        if (ext.empty()) ext = write_props.fourcc;
        if (ext.empty()) ext = ".avi";
        video_out_vid_file  = filename + ext;
        file_info.full_path = video_out_vid_file;
        misc::makeDirectory(video_out_vid_file);
    };
};

class IO
  : public Timestamps
  , public Reader
  , public Writer
{
    timets_t last_ts = 0;
    cv::Mat  last_img;

  protected:
    IO() = default;

  public:
    template<typename D>
    IO(D device, const VideoFile &writer_file)
      : Timestamps(), Reader(device), Writer(writer_file){};

    void
    setProperties(const Properties &reader_properties,
                  const Properties &writer_properties)
    {
        setReaderProperties(reader_properties, true, false);
        setWriterProperties(writer_properties, true);
    };

    void
    setTimestamp(
      const VideoFile & timestamp_file,
      timing::TimePoint start_time_ms = timing::nowTP<timing::TimePoint>())
    {
        copyTimestampFileInfo(timestamp_file);
        setClock(start_time_ms);
    };

    void
    open(size_t n_attempts = 10)
    {
        openReader(getReaderProperties(), n_attempts);
        auto capture_device_props = getReaderProperties(true);
        openWriter(capture_device_props);
        if (useTimestampWriter()) openTimestampStream(getTimestampFileInfo());
    };

    void
    close()
    {
        closeReader();
        closeWriter();
        closeTime();
    };

    void
    read()
    {
        last_ts  = getTimestamp();
        last_img = readImage();
    };

    void
    write()
    {
        writeImage(last_img);
        if (useTimestampWriter()) writeTime(last_ts);
    };

    void
    write(cv::Mat &img, timets_t t = 0)
    {
        writeImage(img);
        if (useTimestampWriter()) writeTime(t);
    };

    cv::Mat
    getLastImage() const
    {
        return last_img;
    }

    timets_t
    getLastImageTime() const
    {
        return last_ts;
    }
};

namespace factory {
video::VideoFile
makeVideoFile(const opts::Pars &options, int index, std::string url = "")
{
    video::VideoFile vid_file;
    vid_file.type   = url.empty() ? "usb" : "url";
    vid_file.index  = index;
    vid_file.folder = options.basic.root_save_folder;
    vid_file.stem   = options.basic.file_identifier;
    vid_file.ext    = options.video.video_container_ext;
    return vid_file;
};

void
setVideoTimeFile(std::vector<video::IO> &videos, const timing::Clock<> &clock)
{
    for (auto &vid : videos)
    {
        vid.setTimestamp(vid.getVideoFileInfo(), clock.getStartTime());
    }
};

void
setVideoProperties(std::vector<video::IO> &videos, const opts::Pars &options)
{
    std::vector<video::Properties> cap_props(
      videos.size(), video::Properties());
    std::vector<video::Properties> write_props(
      videos.size(), video::Properties());

    for (auto i = 0; i < options.video.set_capture_fourcc.size(); ++i)
    {
        cap_props[i].fourcc = options.video.set_capture_fourcc[i];
    }
    for (auto j = 0; j < options.video.set_capture_fps.size(); ++j)
    {
        cap_props[j].fps = options.video.set_capture_fps[j];
    }
    for (auto h = 0; h < options.video.set_capture_height.size(); ++h)
    {
        cap_props[h].frame_height = options.video.set_capture_height[h];
    }
    for (auto w = 0; w < options.video.set_capture_width.size(); ++w)
    {
        cap_props[w].frame_width = options.video.set_capture_width[w];
    }
    for (auto v = 0; v < videos.size(); ++v)
    {
        write_props[v].merge(
          options.video.four_cc, options.video.frames_per_second, 0, 0, false);
        videos[v].setProperties(cap_props[v], write_props[v]);
    }
};

std::vector<video::IO>
multiIO(const opts::Pars &options, const timing::Clock<> &clock)
{
    std::vector<video::IO> video_devices;
    for (auto id : options.video.device_ids)
    {
        video::VideoFile vid_file = makeVideoFile(options, id);
        video_devices.emplace_back(video::IO(id, vid_file));
    }
    int url_count = 0;
    for (const auto &id : options.video.ip_urls)
    {
        video::VideoFile vid_file = makeVideoFile(options, ++url_count, id);
        video_devices.emplace_back(video::IO(id, vid_file));
    }

    setVideoProperties(video_devices, options);
    setVideoTimeFile(video_devices, clock);

    return video_devices;
};

};  // namespace factory

void videoDisplaySetup(cv::Size &           grid,
                       cv::Size &           imdim,
                       std::vector<cv::Mat> imgs,
                       double &             scaler,
                       int                  c_row,
                       int                  c_col);

void colorizeMat(cv::Mat &img);

cv::Mat imDisplay(const std::string &  title,
                  std::vector<cv::Mat> imgs,
                  cv::Size &           disp_dim,
                  cv::Size &           grid_layout,
                  double &             scaler,
                  int &                rec);

void recSwitch(int recPosition, void *);

void frameReadThread(
  int                                                         idx,
  std::vector<std::vector<cv::Mat>> &                         frames,
  std::vector<std::vector<int>> &                             timestamps,
  std::vector<cv::VideoCapture> &                             cap,
  std::chrono::time_point<std::chrono::high_resolution_clock> start_clock,
  double                                                      stop_clock);

void
frameReadThread(
  int                                                         idx,
  std::vector<std::vector<cv::Mat>> &                         frames,
  std::vector<std::vector<int>> &                             timestamps,
  std::vector<cv::VideoCapture> &                             cap,
  std::chrono::time_point<std::chrono::high_resolution_clock> start_clock,
  double                                                      stop_clock)
{
    timestamps[idx].clear();
    frames[idx].clear();
    double currentLoopTime;
    while (true)
    {
        // grab, timestamp, decode, push to buffer, check time
        cv::Mat temp_frame;
        if (!cap[idx].grab())
        {
            std::cout << "\ngrab failed!"
                      << "\n";
        }
        timestamps[idx].emplace_back(misc::msTimestamp(start_clock));
        if (!cap[idx].retrieve(temp_frame))
        {
            std::cout << "\nretrieve failed!"
                      << "\n";
        };
        frames[idx].emplace_back(temp_frame);
        currentLoopTime = static_cast<double>(misc::msTimestamp(start_clock));
        if (currentLoopTime > stop_clock)
        {
            break;
        }
    }
}

cv::Mat
imDisplay(const std::string &  title,
          std::vector<cv::Mat> imgs,
          cv::Size &           disp_dim,
          cv::Size &           grid_layout,
          double &             scaler,
          int &                rec)
{
    cv::Mat out_img = cv::Mat::zeros(disp_dim.height, disp_dim.width, CV_8UC3);
    auto    n_imgs  = static_cast<int>(imgs.size());
    int     img_idx = 0;
    int     out_h_start = 1;
    int     out_w_start;
    int     out_h_end    = 0;
    int     out_w_end    = 0;
    bool    no_more_imgs = false;
    for (auto m = 0; m < grid_layout.height; m++)
    {
        out_w_start = 1;
        for (auto n = 0; n < grid_layout.width; n++)
        {
            if (img_idx < n_imgs)
            {
                cv::Mat in_img;
                resize(imgs[img_idx],
                       in_img,
                       cv::Size(),
                       scaler,
                       scaler,
                       cv::INTER_NEAREST);
                cv::Size in_img_sz  = in_img.size();
                out_h_end           = out_h_start + (in_img_sz.height - 1);
                out_w_end           = out_w_start + (in_img_sz.width - 1);
                cv::Range in_roi_h  = cv::Range(0, in_img_sz.height - 1);
                cv::Range in_roi_w  = cv::Range(0, in_img_sz.width - 1);
                cv::Range out_roi_w = cv::Range(out_w_start, out_w_end);
                cv::Range out_roi_h = cv::Range(out_h_start, out_h_end);

                //				cout << "\n--IN RECT:" << img_idx << endl;
                //				cout << in_roi_w.start << endl;
                //				cout << in_roi_h.start << endl;
                //				cout << in_roi_w.end << endl;
                //				cout << in_roi_h.end << endl;
                //				cout << "\n--OUT RECT:" << img_idx << endl;
                //				cout << out_roi_w.start << endl;
                //				cout << out_roi_h.start << endl;
                //				cout << out_roi_w.end << endl;
                //				cout << out_roi_h.end << endl;

                in_img(in_roi_h, in_roi_w).copyTo(out_img(out_roi_h, out_roi_w));
            } else
            {
                no_more_imgs = true;
            }
            if (no_more_imgs)
            {
                break;
            }
            img_idx++;
            out_w_start = out_w_end + 2;
        }
        if (no_more_imgs)
        {
            break;
        }
        out_h_start = out_h_end + 2;
    }
    if (rec == 1)
    {
        circle(out_img, cv::Point(15, 15), 14, cv::Scalar(0, 0, 255), -1, 8);
        putText(out_img,
                "REC",
                cv::Point(30, 20),
                cv::FONT_HERSHEY_SIMPLEX,
                0.5,
                cv::Scalar(255, 255, 255));
    }
    cv::namedWindow(title);
    cv::imshow(title, out_img);
    return out_img;
}

void
recSwitch(int recPosition, void *)
{
    std::cout
      << (recPosition == 0 ? "\n...Paused recording" : "\n...Recording started")
      << "\n";
}

void
colorizeMat(cv::Mat &img)
{
    for (auto m = 0; m < img.rows; m++)
    {
        for (auto n = 0; n < img.cols; n++)
        {
            img.at<cv::Vec3b>(m, n)[0] = 0;
            img.at<cv::Vec3b>(m, n)[1] = 0;
            img.at<cv::Vec3b>(m, n)[2] = 255;
        }
    }
}

void
videoDisplaySetup(cv::Size &           grid,
                  cv::Size &           imdim,
                  std::vector<cv::Mat> imgs,
                  double &             scaler,
                  int                  c_row,
                  int                  c_col)
{
    // find number of images for placement
    auto n_imgs = static_cast<int>(imgs.size());
    int  n_cols;
    int  m_rows;

    // calculate m rows and n cols based on num images
    if (c_col == -1)
    {
        n_cols = static_cast<int>(ceil(sqrt(n_imgs)));
    } else
    {
        n_cols = c_col;
    }
    if (c_row == -1)
    {
        m_rows = static_cast<int>(
          ceil(static_cast<double>(n_imgs) / static_cast<double>(n_cols)));
    } else
    {
        m_rows = c_row;
    }
    if (n_cols * m_rows < n_imgs)
    {
        n_cols = static_cast<int>(ceil(sqrt(n_imgs)));
        m_rows = static_cast<int>(
          ceil(static_cast<double>(n_imgs) / static_cast<double>(n_cols)));
    }

    // small matrix which holds x,y dims for later max function
    cv::Mat tmp_heights = cv::Mat_<double>(m_rows, n_cols);
    cv::Mat tmp_widths  = cv::Mat_<double>(m_rows, n_cols);
    int     dev_idx     = 0;
    for (int m = 0; m < m_rows; m++)
    {
        for (int n = 0; n < n_cols; n++)
        {
            if (dev_idx < n_imgs)
            {
                // place x,y size of current image in matrix
                tmp_heights.at<double>(m, n) = imgs[dev_idx].rows * scaler;
                tmp_widths.at<double>(m, n)  = imgs[dev_idx].cols * scaler;
            } else
            {
                tmp_heights.at<double>(m, n) = 0;
                tmp_widths.at<double>(m, n)  = 0;
            }
            dev_idx++;
        }
    }

    // find mat size for each dimension
    double img_height = 0;
    double img_width;

    // max height for each row in grid
    std::vector<double> max_row(static_cast<unsigned long>(m_rows), 0);
    for (int m = 0; m < m_rows; m++)
    {
        minMaxIdx(tmp_heights.row(m), nullptr, &max_row[m]);
        img_height += max_row[m];
    }

    // sum of widths for each row in grid, find max
    std::vector<double> row_sums(static_cast<unsigned long>(m_rows), 0);
    for (int m = 0; m < m_rows; m++)
    {
        for (int n = 0; n < n_cols; n++)
        {
            row_sums[m] += tmp_widths.at<double>(m, n);
        }
    }
    auto max_col_it = std::max_element(
      std::begin(row_sums), std::end(row_sums));
    img_width = *max_col_it;
    imdim     = cv::Size(static_cast<int>(ceil(img_width + n_cols)),
                     static_cast<int>(ceil(img_height + m_rows)));
    grid      = cv::Size(n_cols, m_rows);
    std::cout << "\nDisplayed images frame dimensions:\n  Width=" << imdim.width
              << ", Height=" << imdim.height
              << "\nGrid size:\n  Columns=" << grid.width
              << ", Rows=" << grid.height << "\n";
}
};  // namespace video

#endif  // COGDEVCAM_VIDEO_H
