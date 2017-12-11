/**
    project: cogdevcam
    source file: main
    description: main program

    @author Joseph M. Burling
    @version 0.9.0 12/10/2017
*/

#include "audio.h"
#include "video.h"

class CogDevCam
{
    timing::Clock<>        master_clock;
    audio::Streams         audio_stream;
    std::vector<video::IO> video_streams;

  public:
    explicit CogDevCam(const opts::Pars &options)
      : audio_stream(options),
        video_streams(
          std::move(video::factory::multiIO(options, master_clock))){};

    int
    run(const opts::Pars &options)
    {
        cv::namedWindow("test");
        try
        {
            cv::Mat img;
            timing::sleep::sec(0.5);
            master_clock.reset();
            video_streams[0].open(options.video.n_open_attempts);
            audio_stream.open(master_clock.getStartTime());
            master_clock.update();
            auto t = master_clock.stopwatch();
            while (true)
            {
                t += master_clock.stopwatch();
                video_streams[0].read();
                cv::imshow("test", video_streams[0].getLastImage());
                cv::waitKey(1);
                if (t > timing::nano_int * 3 || !audio_stream.running()) break;
            }
            audio_stream.toggleSave(true, 0);
            while (true)
            {
                video_streams[0].read();
                cv::imshow("test", video_streams[0].getLastImage());
                if (cv::waitKey(1) == 27) break;
                video_streams[0].write();
                if (!audio_stream.running()) break;
            }
            video_streams[0].close();
            audio_stream.close();
            cv::destroyAllWindows();
        } catch (const std::exception &err)
        {
            std::cout << err.what() << "\n";
            return 1;
        }
        return 0;
    };
};

int
main(int argc, const char *const *argv)
{
    opts::Pars my_opts(argc, argv);
    CogDevCam  cogdevcam(my_opts);
    return cogdevcam.run(my_opts);
}
