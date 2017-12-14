/**
    project: cogdevcam
    source file: cogdevcam
    description:

    @author Joseph M. Burling
    @version 0.9.1 12/14/2017
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
        n_devices     = program_opts.video.n_devices;
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
        fillFutures(future_state);
        for (int j = 0; j < n_devices; ++j)
        {
            video_streams[j].open(program_opts.video.n_open_attempts);
            video_streams[j].timerTimedOut();
        }
        audio_stream.open(master_clock.getStartTime());
        return isOpen();
    };

    void
    run()
    {
        record_mode = false;
        exit_task   = false;
        setDisplay();
        while (true)
        {
            previewModeOn();
            if (exit_task) break;
            writeModeOn();
            if (exit_task) break;
        }

        record_mode = false;
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
            if (checkRunning())
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
        if (capture_initiated)
        {
            audio_stream.toggleSave(true);

        } else
        {
            // set start of timestamp to zero if first time slider is set
            audio_stream.toggleSave(true, 0);
        }

        capture_initiated = true;
        while (true)
        {
            if (!display_out.isSliderSet())
            {
                record_mode = false;
                break;
            }
            showDisplayImages();
            if (checkRunning())
            {
                exit_task = true;
                break;
            }
            displayImageInterrupt();
        }
    };
    bool
    checkRunning()
    {
        return cv::waitKey(1) == exit_key || !audio_stream.isRunning();
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
    bool                          capture_initiated = false;
    bool                          exit_task         = false;
    int                           exit_key          = 27;
    size_t                        n_devices         = 0;
    size_t                        display_fps       = 30;

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
    setDisplay()
    {
        if (!display_out.isWindowOpen())
        {
            display_out.openWindow(program_opts.video.display_feed_name);
        }

        if (!display_out.isDisplaySet())
        {
            if (img_set.empty())
            {
                resetBuffers();
            }
            display_out.videoDisplaySetup(
              img_set,
              program_opts.video.display_feed_cols,
              program_opts.video.display_feed_rows,
              program_opts.video.display_feed_scale);
        }
    };

    void
    resetBuffers()
    {
        if (!isVideoOpen()) openDevices();
        if (record_mode) throw err::Runtime("Video devices already in use.");
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
    };

    void
    displayImageInterrupt()
    {
        if (display_clock.timeout())
        {
            std::cout << display_clock.stopwatch() << std::endl;
            for (size_t n = 0; n < n_devices; ++n)
            {
                pause_threads[n] = true;
                future_state[n].wait();
                img_set[n] = fill_buffer[n].img.clone();
                //                 img_set[n] = fill_buffer[n].img;
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
