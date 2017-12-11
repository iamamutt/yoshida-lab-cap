/**
    project: cogdevcam
    source file: options
    description: _

    @author Joseph M. Burling
    @version 0.9.0 11/2/2017
*/

#ifndef __COGDEVCAM_OPTIONS_H
#define __COGDEVCAM_OPTIONS_H

#include "tools.h"
#include <boost/program_options.hpp>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace param {
namespace po = boost::program_options;
namespace helper {

/// Add a single option to an options descriptions subsection
void
appendOpt(po::options_description & help,
          const char *              arg,
          const po::value_semantic *val,
          const char *              info)
{
    help.add_options()(arg, val, info);
};

std::string
makeArgStr(const std::string &__arg, const std::string &arg = "")
{
    std::string new_str = arg.empty() ? __arg : __arg + "," + arg;
    return new_str;
};

void
newEmptyOption(po::options_description &help,
               const std::string &      long_arg,
               const char *             info,
               const std::string &      short_arg = "")
{
    help.add_options()(makeArgStr(long_arg, short_arg).c_str(), info);
};

void
newBoolOption(po::options_description &help,
              const std::string &      long_arg,
              bool &                   val,
              const char *             info,
              const std::string &      short_arg = "")
{
    help.add_options()(makeArgStr(long_arg, short_arg).c_str(),
                       po::value<bool>(&val)->implicit_value(true),
                       info);
};

template<typename T>
void
newDefaultOption(po::options_description &help,
                 const std::string &      long_arg,
                 T &                      val,
                 const char *             info,
                 const std::string &      short_arg = "")
{
    appendOpt(help,
              makeArgStr(long_arg, short_arg).c_str(),
              po::value<T>(&val)->default_value(val),
              info);
};

template<typename T>
void
newVectorOption(po::options_description &help,
                const std::string &      long_arg,
                T &                      val,
                const char *             info,
                const std::string &      short_arg = "")
{
    //    help.add_options()(makeArgStr(long_arg, short_arg).c_str(),
    //                       po::value<T>(&val)->multitoken(), info);
    appendOpt(help,
              makeArgStr(long_arg, short_arg).c_str(),
              po::value<T>(&val)->multitoken(),
              info);
};

const unsigned
cmd_line_width()
{
    return 125;
};

const unsigned
cmd_line_textw()
{
    return 92;
};

/// parse command line opts and return parser object. Not yet stored.
po::command_line_parser
argsParser(int argc, const char *const *argv)
{
    return po::command_line_parser(argc, argv)
      .style(po::command_line_style::unix_style);
}
}  // namespace helper

namespace data {
class base
{
  public:
    base() = default;

    base(const int argc, const char *const *argv)
      : parser(helper::argsParser(argc, argv)){};

    po::positional_options_description positional;
    po::options_description            main{
      helper::cmd_line_width(), helper::cmd_line_textw()};
    po::variables_map       map{};
    po::command_line_parser parser{
      1, reinterpret_cast<const char *const *>("a")};

    void
    fillParser(const int argc, const char *const *argv)
    {
        parser = helper::argsParser(argc, argv);
    };
};

template<typename DataStorage>
class subsection
{
  public:
    explicit subsection(const std::string &subtitle_text,
                        unsigned           max_w = 140,
                        unsigned           txt_w = 75)
      : help(subtitle_text, max_w, txt_w){};

    po::options_description help{""};
    DataStorage             store{};
};

/// Contains global program options
struct general
{
    std::string file_identifier  = "";
    std::string root_save_folder = ".";
    bool        verbose          = false;
};
/// Contains user defined audio options and defaults
struct audio
{
    /// audio device specific parameters
    struct stream
    {
        int      device_id     = -1;
        unsigned n_channels    = 0;
        unsigned first_channel = 0;
        unsigned sample_rate   = 0;
    };
    stream      playback{};
    stream      record{};
    std::string playback_mode       = "";
    bool        min_latency         = true;
    bool        linux_default       = false;
    bool        hog_device          = false;
    unsigned    api_enumerator      = 0;
    unsigned    buffer_size         = 0;
    unsigned    bit_depth           = 0;
    double      pulse_rate          = 60;
    unsigned    pulse_width         = 1;
    double      record_duration_sec = 0;
    bool        save_playback       = false;
};
/// Contains user defined video options and defaults
struct video
{
    size_t                   n_usb                = 0;
    size_t                   n_url                = 0;
    size_t                   n_devices            = 0;
    size_t                   n_open_attempts      = 5;
    std::string              four_cc              = "HYUV";
    std::string              video_container_ext  = ".avi";
    double                   frames_per_second    = 30;
    int                      frame_buffer_length  = 12;
    std::string              display_feed_name    = "VIDEO_FEED";
    double                   display_feed_scale   = 0.5;
    unsigned int             display_feed_rows    = 0;
    unsigned int             display_feed_cols    = 0;
    int                      min_frame_buffer_len = 2;
    int                      max_frame_buffer_len = 120;
    std::vector<int>         device_ids;
    std::vector<std::string> ip_urls;
    std::vector<double>      set_capture_fps;
    std::vector<int>         set_capture_height;
    std::vector<int>         set_capture_width;
    std::vector<std::string> set_capture_fourcc;
};
}  // namespace data

namespace helper {
/// set options for parser
void
storeParsed(param::data::base &main)
{
    po::store(main.parser.options(main.main).positional(main.positional).run(),
              main.map);
};

/// check if help option was specified, show and exit if so.
void
checkHelp(param::data::base &main)
{
    po::notify(main.map);
    if (main.map.count("help") != 0)
    {
        std::cout
          << "------------------------------------------------------------\n"
             "| Command line help for CogDevLab camera and audio capture |\n"
             "------------------------------------------------------------\n"
             "                                <Author: Joseph M. Burling> \n"
             "\nUsage:: cogdevcam [options]\n"
             "           cogdevcam [options] cam_options.cfg\n\n";
        std::cout << main.main << std::endl;
        std::exit(0);
    }
};

/// check if config file is valid, map parsed values directly to storage map.
void
checkConfigFile(param::data::base &main, const std::string &config_arg = "cfg")
{
    if (main.map.count(config_arg))
    {
        auto cfg_file = main.map[config_arg].as<std::string>();
        if (!cfg_file.empty() && !misc::fileExists(cfg_file))
        {
            throw err::Runtime("Path does not exist for configuration file: " +
                               cfg_file);
        }
        std::ifstream config_file_stream(cfg_file.c_str());
        if (config_file_stream)
        {
            // if file exists
            po::store(
              po::parse_config_file(config_file_stream, main.main), main.map);
            po::notify(main.map);
        }
    }
};

/// add options to main options list
void
appendMain(param::data::base &main, const po::options_description &desc)
{
    main.main.add(desc);
};
};  // namespace helper

/// options initializer class
class Foundation
{
  protected:
    /// initialize the command line options list and store general help lines
    Foundation() = default;

    void
    initialized()
    {
        base.main.add_options()("help,h",
                                "Show this help documentation for "
                                "description of command line arguments.\n");
        base.main.add_options()(
          "cfg,c",
          po::value<std::string>(&config_file)->default_value(config_file),
          "A file path point to a configuration (.cfg) file."
          " You can save program parameters in this file"
          " and contents will be added to any "
          "command line arguments (no overwrite)."
          "\n\n  e.g., --config=options.cfg or -c \"my pars.txt\"\n\n"
          "Inside the file should be a list of arguments, one per line.\n\n"
          "  usb = 0\n"
          "  aud = 0\n"
          "  url = http://username:password@ipaddress/\n"
          "        axis-cgi/mjpg/video.cgi?resolution=1280x800&.mjpg\n");

        base.positional.add("cfg", 1);
    };

  public:
    void
    build(const int argc, const char *const *argv)
    {
        if (!command_line_parsed)
        {
            base.fillParser(argc, argv);
            initialized();
            command_line_parsed = true;
        }
        helper::appendMain(base, getSubDescription());
    };

    /// finish up options parsing. Command line options have first priority over
    /// config file.
    void
    finalize()
    {
        helper::storeParsed(base);
        helper::checkHelp(base);
        helper::checkConfigFile(base);
    };

    virtual po::options_description getSubDescription() = 0;
    unsigned                        line_width = helper::cmd_line_width();
    unsigned                        desc_width = helper::cmd_line_textw();

  private:
    // base/top-level options container
    param::data::base base;

    bool        command_line_parsed = false;
    std::string config_file         = "";
};

/// Basic usage class subsection
template<typename Storage = data::general>
class General : virtual public Foundation
{
  public:
    explicit General(const int argc, const char *const *argv)
      : general("General options:", line_width, desc_width)
    {
        build(argc, argv);
    };

    data::subsection<Storage> general;

    po::options_description
    getSubDescription() override
    {
        generalHelpList();
        return general.help;
    };

  protected:
    virtual void
    generalHelpList()
    {
        helper::newDefaultOption<std::string>(
          general.help,
          "dir",
          general.store.root_save_folder,
          "ROOT OUTPUT DIRECTORY: "
          "Path to location for where to save the output files."
          "\n\n  e.g., --dir=data or -d data\n",
          "d");
        helper::newDefaultOption<std::string>(
          general.help,
          "fname",
          general.store.file_identifier,
          "COMMON FILE NAME/PREFIX: "
          "Typically a date and/or a timestamp string."
          "\n\n  e.g., --fname=2018_01_10\n",
          "f");
        helper::newBoolOption(
          general.help,
          "verbose",
          general.store.verbose,
          "VERBOSE MODE: "
          "print additional information to command line interface\n"
          "\n\n  e.g., --verbose or -v\n",
          "v");
    };

    void
    checkGeneralOptions()
    {
        if (general.store.file_identifier.empty())
        {
            general.store.file_identifier = misc::filePrefix();
        }
        std::string parent;
        std::string stem;
        std::tie(parent, stem) = misc::makeDirectory(
          general.store.root_save_folder, general.store.file_identifier);
        general.store.root_save_folder = parent;
        general.store.file_identifier  = stem;
    };
};

/// Audio parameters class subsection
template<typename Storage = data::audio>
class Audio : virtual public Foundation
{
  public:
    explicit Audio(const int argc, const char *const *argv)
      : audio("Audio options:", line_width, desc_width)
    {
        build(argc, argv);
    };

    data::subsection<Storage> audio;

    po::options_description
    getSubDescription() override
    {
        audioHelpList();
        with_audio = true;
        return audio.help;
    };

  protected:
    bool with_audio = false;

    virtual void
    audioHelpList()
    {
        /// Audio general
        helper::newDefaultOption<unsigned>(
          audio.help,
          "aapi",
          audio.store.api_enumerator,
          "RtAudio API enumerator. "
          "Integer value of the api index."
          "\n\n  e.g., --aapi=5\n\n"
          " ENUM: int\n"
          "  0=Search for a working compiled API.\n"
          "  1=The Advanced Linux Sound Architecture API.\n"
          "  2=The Linux PulseAudio API.\n"
          "  3=The Linux Open Sound System API.\n"
          "  4=The Jack Low-Latency Audio Server API.\n"
          "  5=Macintosh OS-X Core Audio API.\n"
          "  6=The Microsoft WASAPI API.\n"
          "  7=The Steinberg Audio stream I/O API.\n"
          "  8=The Microsoft Direct Sound API.\n"
          "  9=A compilable but non-functional API.\n");
        helper::newDefaultOption<unsigned>(
          audio.help,
          "abuffer",
          audio.store.buffer_size,
          "AUDIO INPUT BUFFER: "
          "Audio buffer size. "
          "Higher values effects output latency but "
          "increases stability."
          "\n\n  e.g., --ain_buffer=512\n");
        helper::newDefaultOption<unsigned>(
          audio.help,
          "abits",
          audio.store.bit_depth,
          "AUDIO SAMPLE BIT DEPTH SIZE: "
          "Size corresponding to "
          "the number of bits per sample. "
          "Use only the following positive values.\n"
          "1  = 8-bit signed integer.\n"
          "2  = 16-bit signed integer.\n"
          "4  = 24-bit signed integer.\n"
          "8  = 32-bit signed integer.\n"
          "16 = Normalized between plus/minus 1.0.\n"
          "32 = Normalized between plus/minus 1.0.\n"
          "\n  e.g., --abits=32 --abits=16\n");
        helper::newDefaultOption<double>(
          audio.help,
          "aforsec",
          audio.store.record_duration_sec,
          "AUDIO STREAM OPEN DURATION: "
          "Open the audio stream for x amount of seconds only"
          "\n\n  e.g., --aforsec=5\n");
        helper::newBoolOption(audio.help,
                              "ahog",
                              audio.store.hog_device,
                              "AUDIO HOG USE: "
                              "Attempt to grab device for exclusive use "
                              "by setting the flag RTAUDIO_HOG_DEVICE."
                              "\n\n  e.g., --ahog\n");
        helper::newBoolOption(audio.help,
                              "alate",
                              audio.store.min_latency,
                              "AUDIO LATENCY REDUCTION: "
                              "Try to reduce latency by setting "
                              "the flag RTAUDIO_MINIMIZE_LATENCY."
                              "\n\n  e.g., --alate\n");
        helper::newBoolOption(audio.help,
                              "aalsa",
                              audio.store.linux_default,
                              "AUDIO ALSA DEFAULT: "
                              "Choose default device when using ALSA mode "
                              "on Linux systems by setting the "
                              "flag RTAUDIO_ALSA_USE_DEFAULT."
                              "\n\n  e.g., --aalsa\n");

        // Application param: Audio Input
        helper::newDefaultOption<int>(
          audio.help,
          "ain",
          audio.store.record.device_id,
          "AUDIO INPUT DEVICE INDEX: "
          "Integer value of the AUDIO device index."
          "\n\n  e.g., --ain=2, -a 0 (=use default), -a -1 (=off)\n",
          "a");

        helper::newDefaultOption<unsigned>(
          audio.help,
          "aisr",
          audio.store.record.sample_rate,
          "AUDIO INPUT SAMPLE RATE: "
          "Sample rate for all audio input devices."
          "\n\n  e.g., --aisr=48000, -aisr=44100\n");
        helper::newDefaultOption<unsigned>(
          audio.help,
          "aich",
          audio.store.record.n_channels,
          "AUDIO INPUT CHANNELS: "
          "Number of channels to use from the audio input device."
          "\n\n  e.g., --aich=1\n");

        helper::newDefaultOption<unsigned>(
          audio.help,
          "aichfirst",
          audio.store.record.first_channel,
          "AUDIO INPUT FIRST CHANNEL NUMBER/INDEX: "
          "Determines the channel to use if Mono (L/R) "
          "or first channel to write to if stereo."
          "\n\n  e.g., --aichfirst=1\n");

        // Application param: Audio Output
        helper::newDefaultOption<int>(audio.help,
                                      "aout",
                                      audio.store.playback.device_id,
                                      "AUDIO OUTPUT DEVICE INDEX: "
                                      "see audio input option description");
        helper::newDefaultOption<unsigned>(
          audio.help,
          "aosr",
          audio.store.playback.sample_rate,
          "AUDIO OUTPUT SAMPLE RATE: "
          "see audio input option description");
        helper::newDefaultOption<unsigned>(
          audio.help,
          "aoch",
          audio.store.playback.n_channels,
          "AUDIO OUTPUT CHANNELS: "
          "see audio input option description");
        helper::newDefaultOption<unsigned>(
          audio.help,
          "aochfirst",
          audio.store.playback.first_channel,
          "AUDIO OUTPUT FIRST CHANNEL NUMBER/INDEX: "
          "see audio input option description");

        // additional audio output options
        helper::newDefaultOption<std::string>(
          audio.help,
          "aomode",
          audio.store.playback_mode,
          "AUDIO OUTPUT PLAYBACK MODE: "
          "A string indicating type of "
          "audio playback from capture device. "
          "Can be \"pulse\" or \"input\""
          "\n\n  e.g., --aomode=pulse\n");
        helper::newDefaultOption<double>(
          audio.help,
          "apulse",
          audio.store.pulse_rate,
          "AUDIO PULSE RATE: "
          "Rate for how often to send a pulse signal in hz. "
          "Only is used if playback type is set to \"pulse\""
          "\n\n  e.g., --apulse=90\n");
        helper::newDefaultOption<unsigned>(audio.help,
                                           "apwidth",
                                           audio.store.pulse_width,
                                           "AUDIO PULSE WIDTH: "
                                           "Number of samples for 'on' pulse"
                                           "\n\n  e.g., --apwidth=3\n");
        helper::newBoolOption(audio.help,
                              "aplaywav",
                              audio.store.save_playback,
                              "AUDIO PLAYBACK WAV: "
                              "Save the playback output buffer to a wav file."
                              "\n\n  e.g., --aplaywav");
    };

    void
    checkAudioOptions()
    {
        if (!with_audio)
        {
            return;
        }
        if (!audio.store.playback_mode.empty() &&
            audio.store.playback.device_id == -1)
        {
            std::cout
              << "\nPlayback type entered but device not set! Setting device to 0\n";
            audio.store.playback.device_id = 0;
        }
    }
};

/// Basic usage class subsection
template<typename Storage = data::video>
class Video : virtual public Foundation
{
  public:
    explicit Video(const int argc, const char *const *argv)
      : video("Video options:", line_width, desc_width)
    {
        build(argc, argv);
    };

    data::subsection<Storage> video;

    po::options_description
    getSubDescription() override
    {
        videoHelpList();
        with_video = true;
        return video.help;
    };

  protected:
    bool with_video = false;

    virtual void
    videoHelpList()
    {
        // Application param: Video
        helper::newVectorOption<std::vector<int>>(
          video.help,
          "usb",
          video.store.device_ids,
          "USB ID: "
          "Integer value of the USB camera index. "
          "Different APIs are in increments of 100. May be used multiple times."
          "\n\n  e.g., --usb=0 or -u 1\n",
          "u");
        helper::newVectorOption<std::vector<std::string>>(
          video.help,
          "url",
          video.store.ip_urls,
          "URL ID: "
          "String of the full url for IP cameras, no quotes. "
          "May be used multiple times."
          "\n\n  e.g., --url=http://112.0.0.1 or -i http://...\n",
          "i");
        helper::newDefaultOption<std::string>(
          video.help,
          "codec",
          video.store.four_cc,
          "CODEC: "
          "Upper case four letter code for the "
          "codec used to export video.store."
          "\n\n  e.g., --codec=MP4V --codec=MJPG -k HYUV\n",
          "k");
        helper::newDefaultOption<std::string>(
          video.help,
          "ext",
          video.store.video_container_ext,
          "EXTENSION: "
          "Lower case three letter extension/wrapper for the video file."
          "\n\n  e.g., --ext=m4v --ext=avi\n",
          "e");
        helper::newDefaultOption<double>(
          video.help,
          "fps",
          video.store.frames_per_second,
          "FRAMES PER SECOND: "
          "Frames per second for all output videos, "
          "no matter the input fps."
          "\n\n  e.g., --fps=25, -f 30\n",
          "f");
        helper::newDefaultOption<int>(
          video.help,
          "fbl",
          video.store.frame_buffer_length,
          "FRAME BUFFER LENGTH: "
          "Buffer size for holding consecutive frames. "
          "This will affect imshow latency."
          "\n\n  e.g., --fbl=10 or -b 10\n",
          "b");
        helper::newDefaultOption<size_t>(
          video.help,
          "vtries",
          video.store.n_open_attempts,
          "NUM OPEN TRIES: "
          "Number of attempts to try and open video capture device."
          "\n\n  e.g., --vtries=10\n");

        // Display param
        po::options_description misc_help(
          "Misc. video/display options", line_width, desc_width);
        helper::newDefaultOption<std::string>(
          misc_help,
          "win",
          video.store.display_feed_name,
          "WINDOW NAME: "
          "Name to display at top of window."
          "\n\n  e.g., --win=stuff or -w myFeed\n",
          "w");
        helper::newDefaultOption<double>(misc_help,
                                         "res",
                                         video.store.display_feed_scale,
                                         "RESIZE SCALER: "
                                         "Value corresponding to how "
                                         "much to resize the captured images."
                                         "\n\n  e.g., --res=.5 or -r .5\n",
                                         "r");
        helper::newDefaultOption<unsigned int>(
          misc_help,
          "rows",
          video.store.display_feed_rows,
          "CUSTOMIZE DISPLAY FEED ROWS: "
          "Number of rows for output display."
          "\n\n  e.g., --rows=2\n");
        helper::newDefaultOption<unsigned int>(
          misc_help,
          "cols",
          video.store.display_feed_cols,
          "CUSTOMIZE DISPLAY FEED COLUMNS: "
          "Number of cols for output display."
          "\n\n  e.g., --cols=1\n");

        /// Misc options
        helper::newVectorOption<std::vector<double>>(
          misc_help,
          "CAP_PROP_FPS",
          video.store.set_capture_fps,
          "SET OPENCV CAPTURE DEVICE'S INTERNAL PROPERTY: "
          "frames per second.\n"
          "Order according to USB then URL device order."
          "\n\n  e.g., --CAP_PROP_FPS=30\n");
        helper::newVectorOption<std::vector<int>>(
          misc_help,
          "CAP_PROP_FRAME_HEIGHT",
          video.store.set_capture_height,
          "SET OPENCV CAPTURE DEVICE'S INTERNAL PROPERTY: "
          "frame height.\n"
          "Order according to USB then URL device order."
          "\n\n  e.g., --CAP_PROP_FRAME_HEIGHT=800\n");
        helper::newVectorOption<std::vector<int>>(
          misc_help,
          "CAP_PROP_FRAME_WIDTH",
          video.store.set_capture_width,
          "SET OPENCV CAPTURE DEVICE'S INTERNAL PROPERTY: "
          "frame width.\n"
          "Order according to USB then URL device order."
          "\n\n  e.g., --CAP_PROP_FRAME_WIDTH=1280\n");
        helper::newVectorOption<std::vector<std::string>>(
          misc_help,
          "CAP_PROP_FOURCC",
          video.store.set_capture_fourcc,
          "SET OPENCV CAPTURE DEVICE'S INTERNAL PROPERTY: "
          "fourcc.\n"
          "Order according to USB then URL device order."
          "\n\n  e.g., --CAP_PROP_FOURCC=MJPG\n");
        video.help.add(misc_help);
    };

    void
    checkVideoOptions()
    {
        if (!with_video)
        {
            return;
        }
        video.store.n_url     = video.store.ip_urls.size();
        video.store.n_usb     = video.store.device_ids.size();
        video.store.n_devices = video.store.device_ids.size() +
                                video.store.ip_urls.size();
        if (video.store.device_ids.empty())
        {
            video.store.device_ids.emplace_back(-1);
        }
        if (video.store.ip_urls.empty())
        {
            video.store.ip_urls.emplace_back("");
        }
        if (video.store.set_capture_fps.empty())
        {
            video.store.set_capture_fps.assign(video.store.n_devices, 0);
        }
        if (video.store.set_capture_height.empty())
        {
            video.store.set_capture_height.assign(video.store.n_devices, 0);
        }
        if (video.store.set_capture_width.empty())
        {
            video.store.set_capture_width.assign(video.store.n_devices, 0);
        }
        if (video.store.set_capture_fourcc.empty())
        {
            video.store.set_capture_fourcc.assign(video.store.n_devices, "");
        }
        if (video.store.n_devices > 0)
        {
            if (!video.store.four_cc.empty() && video.store.four_cc.size() != 4)
            {
                throw err::Runtime(
                  "Please enter a codec with at least four characters. "
                  "see FOURCC.org.");
            }
            if (video.store.frame_buffer_length <
                video.store.min_frame_buffer_len)
            {
                misc::strPrint(
                  {"Warning. Minimum buffer length set to",
                   std::to_string(video.store.min_frame_buffer_len),
                   "frames"});
                video.store.frame_buffer_length = video.store
                                                    .min_frame_buffer_len;
            }
            if (video.store.frame_buffer_length >
                video.store.max_frame_buffer_len)
            {
                misc::strPrint(
                  {"Warning. Maximum buffer length set to",
                   std::to_string(video.store.max_frame_buffer_len),
                   "frames"});
                video.store.frame_buffer_length = video.store
                                                    .max_frame_buffer_len;
            }
        }
    };
};

/// combined options subsections into a single class
class Options
  : public General<data::general>
  , public Video<data::video>
  , public Audio<data::audio>
{
  public:
    explicit Options(const int argc, const char *const *argv)
      : General<data::general>(argc, argv),
        Video<data::video>(argc, argv),
        Audio<data::audio>(argc, argv)
    {
        finalize();
        checkGeneralOptions();
        checkAudioOptions();
        checkVideoOptions();
    };

    // needs final override function
    po::options_description
    getSubDescription() override
    {
        return po::options_description();
    };
};
}  // namespace param

namespace opts {

/**
 *  Interface for collecting command line input arguments from all sections.
 *
 *  Access: [param::data::basic] basic, [param::data::audio] audio,
 * [param::data::video] video
 *
 *  e.g.,
 *  opts::Pars my_opts{argc, argv};
 *  auto folder = my_options.main.root_save_folder;
 *  auto usb = my_options.video.device_ids[0];
 */
class Pars
{
  public:
    param::data::general basic;
    param::data::audio   audio;
    param::data::video   video;

    Pars(const int _argc, const char *const *_argv)
    {
        reprintCmd(_argc, _argv);
        runOpts(_argc, _argv);
    };

  private:
    void
    reprintCmd(const int _argc, const char *const *_argv)
    {
        std::stringstream cin;
        cin << "command entered: ";
        for (auto a = 0; a < _argc; a++)
        {
            cin << " " << _argv[a];
        }
        std::cout << cin.str() << "\n";
    };

    void
    runOpts(const int _argc, const char *const *_argv)
    {
        try
        {
            param::Options opt(_argc, _argv);
            basic = opt.general.store;
            audio = opt.audio.store;
            video = opt.video.store;
        } catch (const std::exception &err)
        {
            throw err::Runtime(
              "error while attempting to collect program parameters", err);
        }
    };
};
};  // namespace opts

#endif  // __COGDEVCAM_OPTIONS_H
