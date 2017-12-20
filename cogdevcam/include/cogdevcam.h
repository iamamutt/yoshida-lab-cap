/**
    project: cogdevcam
    source file: cogdevcam
    description:

    @author Joseph M. Burling
    @version 0.9.2 12/19/2017
*/

#ifndef COGDEVCAM_COGDEVCAM_H
#define COGDEVCAM_COGDEVCAM_H

#include "audio.h"
#include "imagegui.h"
#include "video.h"

struct Image
{
    cv::Mat              img;
    video::VideoTimeType ts = 0;
    Image()                 = default;
    Image(cv::Mat _img, video::VideoTimeType _ts)
      : img(std::move(_img)), ts(_ts){};
};

class CogDevCam
{
    timing::Clock<timing::unit_ms_flt> master_clock;
    timing::Clock<timing::unit_ms_flt> display_clock;
    imagegui::WinShow                  display_out;
    audio::Streams                     audio_stream;
    std::vector<video::IO>             video_streams;
    const opts::Pars                   program_opts;
    bool                               use_audio;
    bool                               use_video;

  public:
    using FutureImage = std::future<void>;

    explicit CogDevCam(const opts::Pars &options)
      : program_opts(options),
        audio_stream(options),
        video_streams(
          std::move(video::factory::multiIO(options, master_clock))),
        pause_threads(options.video.n_devices),
        record_mode(false)
    {
        n_devices = program_opts.video.n_devices;
        use_video = n_devices > 0;
        use_audio = audio_stream.use_audio;

        display_fps   = program_opts.video.display_feed_fps;
        auto img_wait = static_cast<timing::unit_ms_flt::rep>(
          display_clock.getSecondsMultiplier() / display_fps);
        double thresh = 1;
        display_clock.setTimeout(img_wait, thresh);
        if (program_opts.video.video_sync)
        {
            video::VideoClock vid_clock;
            auto              fps_dbl = static_cast<double>(
              vid_clock.getSecondsMultiplier() /
              program_opts.video.frames_per_second);

            for (int v = 0; v < n_devices; ++v)
            {
                video_streams[v].timerSetTimeout(fps_dbl, .95);
            }
        }
    };

    bool
    openDevices()
    {
        if (use_video && !isVideoOpen())
        {
            fillFutures(future_state);
            for (int j = 0; j < n_devices; ++j)
            {
                video_streams[j].open(program_opts.video.n_open_attempts);
                video_streams[j].timerTimedOut();
            }
        }

        if (use_audio && !isAudioOpen())
        {
            audio_stream.open(master_clock.getStartTime());
            while (!audio_stream.isRunning())
            {
                timing::sleep::sec(0.01);
            }
        }

        return isOpen();
    };

    void
    run()
    {
        record_mode = false;
        exit_task   = false;

        openDevices();
        setDisplay();

        while (true)
        {
            previewModeOn();
            if (exit_task) break;

            writeModeOn();
            if (exit_task) break;
        }

        record_mode = false;
        exit_task   = false;
    };

    void
    previewModeOn()
    {
        // preview mode, no touching record_mode
        audio_stream.toggleSave(false);

        while (true)
        {
            if (display_out.isSliderSet())
            {
                record_mode = true;
                break;
            }
            showDisplayImages();
            if (breakRunProcess())
            {
                exit_task = true;
                break;
            }
            displayImageInterrupt();
        }
    };

    void
    writeModeOn()
    {
        // recording mode
        if (audio_capture_init)
        {
            audio_stream.toggleSave(true);

        } else
        {
            // set start of timestamp to zero if first time slider is set
            audio_stream.toggleSave(true, 0);
        }
        audio_capture_init = true;

        while (true)
        {
            if (!display_out.isSliderSet())
            {
                record_mode = false;
                break;
            }
            showDisplayImages();
            if (breakRunProcess())
            {
                exit_task = true;
                break;
            }
            displayImageInterrupt();
        }
    };
    bool
    breakRunProcess()
    {
        bool key_pressed     = cv::waitKey(1) == exit_key;
        bool break_for_audio = false;

        if (use_audio)
        {
            break_for_audio = !audio_stream.isRunning();
        }

        return key_pressed || break_for_audio;
    };

    int
    closeAll()
    {
        futureWait(future_state, true);
        audio_stream.close();
        for (auto &vid : video_streams)
        {
            vid.close();
        }
        display_out.closeWindow();
        for (auto &vid : video_streams)
        {
            // delete empty file
            if (vid.getWriterFrame() == 0)
            {
                misc::removeFile(vid.getWriterFilename());
                misc::removeFile(vid.getTimestampFilename());
            }
        }
        if (audio_stream.getBufferCount() == 1)
        {
            misc::removeFile(audio_stream.timestamp_filename);
            misc::removeFile(audio_stream.recording_filename);
            misc::removeFile(audio_stream.playback_filename);
        }
        return 0;
    };

  private:
    std::vector<cv::Mat>          img_set;
    std::vector<Image>            fill_buffer;
    std::vector<FutureImage>      future_state;
    std::vector<std::atomic_bool> pause_threads;
    std::atomic_bool              record_mode;
    bool                          audio_capture_init = false;
    bool                          exit_task          = false;
    int                           exit_key           = 27;
    size_t                        n_devices          = 0;
    size_t                        display_fps        = 30;

    bool
    isOpen()
    {
        return isAudioOpen() && isVideoOpen();
    };

    bool
    isAudioOpen()
    {
        return audio_stream.isOpen();
    };

    bool
    isVideoOpen()
    {
        for (auto &j : video_streams)
        {
            if (!j.isOpen()) return false;
        }
        return true;
    };

    void
    setDisplay(int ncols = -1, int nrows = -1, double rescale = -1)
    {
        if (!display_out.isWindowOpen())
        {
            display_out.openWindow(program_opts.video.display_feed_name);
        }

        if (!use_video)
        {
            ncols   = 1;
            nrows   = 1;
            rescale = 1;
        }

        if (!display_out.isDisplaySet())
        {
            if (img_set.empty())
            {
                resetBuffers();
            }
            if (ncols < 0) ncols = program_opts.video.display_feed_cols;
            if (nrows < 0) nrows = program_opts.video.display_feed_rows;
            if (rescale < 0) rescale = program_opts.video.display_feed_scale;
            display_out.videoDisplaySetup(img_set, ncols, nrows, rescale);
        }
    };

    void
    resetBuffers()
    {
        if (use_video)
        {
            if (record_mode)
                throw err::Runtime("Video devices already in use.");
            futureWait(future_state, true);
            fill_buffer.clear();
            img_set.clear();
            for (auto d = 0; d < n_devices; ++d)
            {
                Image img(video_streams[d].readTemp(), 0);
                display_out.colorizeMat(img.img);
                fill_buffer.emplace_back(img);
                img_set.push_back(img.img.clone());
            }
        } else
        {
            // make blank image
            cv::Mat mat(cv::Size(640, 480), CV_8UC3);
            display_out.colorizeMat(mat);
            img_set.push_back(mat);
        }
    };

    void
    displayImageInterrupt()
    {
        if (use_video && display_clock.timeout())
        {
            for (size_t n = 0; n < n_devices; ++n)
            {
                pause_threads[n] = true;
                future_state[n].wait();
                img_set[n] = fill_buffer[n].img.clone();
                // img_set[n] = fill_buffer[n].img;
                pause_threads[n] = false;
                fillBuffer(n);
            }
        }
    };

    void
    showDisplayImages()
    {
        if (!img_set.empty()) display_out.showImages(img_set);
    };

    void
    fillBuffer(size_t index)
    {
        future_state[index] = std::async(
          std::launch::async,
          [](video::IO &             device,
             Image &                 buff,
             const std::atomic_bool &pause_for_update,
             const std::atomic_bool &record_switch_on) {
              while (!pause_for_update)
              {
                  // TODO: add buffer to video class and match with time for sync mode
                  device.read();
                  buff = Image(std::move(device.getLastImage()),
                               device.getLastImageTime());
                  if (record_switch_on && device.timerTimedOut())
                  {
                      device.write(buff.img, buff.ts);
                  }
              }
          },
          std::ref(video_streams[index]),
          std::ref(fill_buffer[index]),
          std::ref(pause_threads[index]),
          std::ref(record_mode));
    };

    void
    fillFutures(std::vector<FutureImage> &_futures)
    {
        _futures.clear();
        for (int j = 0; j < n_devices; ++j)
        {
            _futures.emplace_back(
              futures::makeFutureValid<FutureImage, void>());
            _futures[j].wait();
        }
    };

    void
    futureWait(std::vector<FutureImage> &_futures, bool pauseThread = false)
    {
        for (int f = 0; f < _futures.size(); ++f)
        {
            if (pauseThread) pause_threads[f] = true;
            _futures[f].wait();
            if (pauseThread) pause_threads[f] = false;
        }
    };
};

#endif  // COGDEVCAM_COGDEVCAM_H
