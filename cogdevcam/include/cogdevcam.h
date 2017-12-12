/**
    project: cogdevcam
    source file: cogdevcam
    description:

    @author Joseph M. Burling
    @version 0.9.0 12/10/2017
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
        stop_thread(options.video.n_devices),
        record_mode(false)
    {
        n_devices   = program_opts.video.n_devices;
        display_fps = program_opts.video.display_feed_fps;

        display_clock.setTimeout(
          timing::unit_ms_flt(static_cast<typename timing::unit_ms_flt::rep>(
            display_clock.getSecondsMultiplier() / display_fps)));

        if (program_opts.video.video_sync)
        {
            sync_fps = static_cast<typename timing::unit_ms_flt::rep>(
              master_clock.getSecondsMultiplier() /
              program_opts.video.frames_per_second);

            master_clock.setTimeout(timing::unit_ms_flt(sync_fps));

            waitkey_ms = static_cast<int>(sync_fps * .85);
        }
        master_clock.update();
        display_clock.update();
    };

    bool
    openDevices()
    {
        fillFutures(future_state);
        fillFutures(future_write);
        for (int j = 0; j < n_devices; ++j)
        {
            video_streams[j].open(program_opts.video.n_open_attempts);
        }
        audio_stream.open(master_clock.getStartTime());
        return isOpen();
    };

    void
    run()
    {
        setDisplay();
        while (true)
        {
            preview_off = display_out.isSliderSet();
            pingCapture();
            getDisplayImages();
            showDisplayImages();
            writeAudio();

            // terminate from key press
            if (cv::waitKey(waitkey_ms) == exit_key) break;

            // terminate from audio duration setting
            if (!audio_stream.isRunning()) break;
        }
        preview_off = false;
        record_mode = false;
    };

    int
    closeAll()
    {
        futureWait(future_state, true);
        futureWait(future_write);
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
    std::vector<cv::Mat>              img_set;
    std::vector<Image>                fill_buffer;
    std::vector<Image>                access_buffer;
    std::vector<FutureImage>          future_write;
    std::vector<FutureImage>          future_state;
    std::vector<std::atomic_bool>     stop_thread;
    std::atomic_bool                  record_mode;
    bool                              preview_off       = false;
    bool                              capture_initiated = false;
    int                               exit_key          = 27;
    size_t                            n_devices         = 0;
    size_t                            display_fps       = 30;
    typename timing::unit_ms_flt::rep sync_fps          = -1;
    int                               waitkey_ms        = 1;

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
        access_buffer.clear();
        fill_buffer.clear();
        img_set.clear();
        for (auto d = 0; d < n_devices; ++d)
        {
            Image img(video_streams[d].readTemp(), 0);
            display_out.colorizeMat(img.img);
            access_buffer.push_back(img);
            fill_buffer.emplace_back(img);
            img_set.push_back(img.img);
        }
    };

    void
    pingCapture()
    {
        if (sync_fps > 0)
        {
            auto timed_out = master_clock.timeout();
            if (timed_out)
            {
                pauseForDisplay();
            }
            record_mode = preview_off && timed_out;
        } else
        {
            record_mode = preview_off;
            if (display_clock.timeout())
            {
                pauseForDisplay();
            }
        }
    };

    void
    pauseForDisplay()
    {
        for (size_t n = 0; n < n_devices; ++n)
        {
            stop_thread[n] = true;
            future_state[n].wait();
            // access_buffer[n].img = fill_buffer[n].img.clone();
            access_buffer[n].img = fill_buffer[n].img;
            stop_thread[n]       = false;
            fillBuffer(n);
        }
    };

    void
    writeAudio()
    {
        if (display_out.isSliderSet())
        {
            if (capture_initiated)
            {
                audio_stream.toggleSave(true);

            } else
            {
                audio_stream.toggleSave(true, 0);
            }
            capture_initiated = true;

        } else
        {
            audio_stream.toggleSave(false);
        }
    };

    void
    getDisplayImages()
    {
        img_set.clear();
        for (auto &n : access_buffer) img_set.emplace_back(n.img);
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
             const std::atomic_bool &stop,
             const std::atomic_bool &write_img) {
              while (!stop)
              {
                  device.read();
                  buff = Image(std::move(device.getLastImage()),
                               device.getLastImageTime());

                  if (write_img) device.write(buff.img, buff.ts);
              }
          },
          std::ref(video_streams[index]),
          std::ref(fill_buffer[index]),
          std::ref(stop_thread[index]),
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
            if (pauseThread) stop_thread[f] = true;
            _futures[f].wait();
            if (pauseThread) stop_thread[f] = false;
        }
    };
};

#endif  // COGDEVCAM_COGDEVCAM_H
