/**
    project: cogdevcam
    source file: audio.h
    description: Open/close audio devices using the RtAudio library

    @author Joseph M. Burling
    @version 0.9.0 11/27/2017
*/

#ifndef __COGDEVCAM_AUDIO_H
#define __COGDEVCAM_AUDIO_H

#include "options.h"
#include "tools.h"
#include <RtAudio.h>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <utility>

namespace audio {
namespace wav {

/**
 * convert data to bytes by reinterpreting as char*
 * @tparam T convertible type object
 * @param obj object to be converted
 * @return bytes represented as char*
 */
template<typename T>
char *
to_bytes(T &obj)
{
    return reinterpret_cast<char *>(&obj);
};

struct Header
{
    char     riff[5]              = "RIFF";
    uint32_t wav_size_placeholder = 0;
    char     wave[5]              = "WAVE";
    char     fmt[5]               = "fmt ";
    uint32_t header_size_fmt      = 16;
    uint16_t format_code          = 1;  // 1 for integer pcm, 3 for IEEE float
    uint16_t n_channels           = 0;
    uint32_t sample_rate          = 0;
    uint32_t byte_rate            = 0;
    uint16_t block_bytes          = 0;
    uint32_t bits_per_sample      = 0;  // bit rate, 16, 32, 64 etc...
    char     data[5]              = "data";

    /**
     * Header data used for generating a WAV file format
     * @param _n_channels number of channels
     * @param _sample_rate audio sample rate
     * @param _bits_per_sample bit depth per sample
     * @param _fmt format name to indicate integer or floating point value.
     * Checks "float"
     */
    explicit Header(uint16_t           _n_channels,
                    uint32_t           _sample_rate,
                    uint32_t           _bits_per_sample,
                    const std::string &_fmt)
      : n_channels(_n_channels),
        sample_rate(_sample_rate),
        bits_per_sample(_bits_per_sample)
    {
        format_code = static_cast<uint16_t>(_fmt == "float" ? 3 : 1);
        block_bytes = static_cast<uint16_t>(bits_per_sample * n_channels / 8);
        byte_rate   = sample_rate * bits_per_sample * n_channels / 8;
    };
};

/// Write data to file
void
writeHeader(std::ofstream &file, Header &info)
{
    file.seekp(0, std::ios_base::beg);
    file.write(info.riff, 4);
    file.write(to_bytes(info.wav_size_placeholder), 4);
    file.write(info.wave, 4);
    file.write(info.fmt, 4);
    file.write(to_bytes(info.header_size_fmt), 4);
    file.write(to_bytes(info.format_code), 2);
    file.write(to_bytes(info.n_channels), 2);
    file.write(to_bytes(info.sample_rate), 4);
    file.write(to_bytes(info.byte_rate), 4);
    file.write(to_bytes(info.block_bytes), 2);
    file.write(to_bytes(info.bits_per_sample), 2);
    file.write(info.data, 4);
    file.write(to_bytes(info.wav_size_placeholder), 4);
};

/// Fill in rest of data before file is closed.
void
writeSize(std::ofstream &file)
{
    // go to start of file and get file begin stream position
    file.seekp(0, std::ios::beg);
    std::streampos file_begin = file.tellp();

    // go to end of header data
    // 44 = 36+8 = RIFF + WAVE::fmt + 8
    file.seekp(44, std::ios::beg);
    std::streampos data_begin = file.tellp();

    // go to end of file, get position
    file.seekp(0, std::ios::end);
    std::streampos file_end = file.tellp();

    // total size = end of file position minus start position
    auto     begin_end_size = static_cast<uint32_t>(file_end - file_begin);
    uint32_t file_size      = begin_end_size - 8;

    // data_size = begin_end_size - 44;
    auto data_size = static_cast<uint32_t>(file_end - data_begin);

    // write final chunk size value after "RIFF"
    file.seekp(4, std::ios::beg);
    file.write(to_bytes(file_size), 4);

    // write final chunk size value after "data"
    file.seekp(40, std::ios::beg);
    file.write(to_bytes(data_size), 4);
};

class Wav
{
    bool is_initialized = false;

  public:
    Wav() = default;

    /**
     * Convert raw audio data to WAV file format
     * @param filename Name of output file
     * @param n_channels number of channels
     * @param sample_rate audio sample rate
     * @param bits_per_sample bit depth per sample
     * @param _fmt sample value of either "float" or "integer"
     */
    void
    init(const std::string &filename,
         uint16_t           n_channels,
         uint32_t           sample_rate,
         uint32_t           bits_per_sample,
         const std::string &_fmt)
    {
        misc::makeDirectory(filename);
        std::ofstream file_temp{filename, std::ios_base::binary};
        file = std::move(file_temp);
        Header head(n_channels, sample_rate, bits_per_sample, _fmt);
        writeHeader(file, head);
        is_initialized = true;
    };

    bool
    is_open()
    {
        return is_initialized && file.is_open();
    };

    void
    close()
    {
        if (is_open())
        {
            writeSize(file);
            file.close();
            is_initialized = false;
        }
    }

    bool
    isReady() const
    {
        return is_initialized;
    }

    std::ofstream file;
};
};  // namespace wav

namespace data {

using AudioTimeScale = timing::milli;
using AudioTimeType  = double;
using AudioDuration  = std::chrono::duration<AudioTimeType, AudioTimeScale>;
using AudioClock     = timing::Clock<AudioDuration>;

/// RT Audio usage mode
enum class StreamMode
{
    NONE,             // no audio
    PLAY_ONLY,        // playback only mode
    RECORD_ONLY,      // record only mode
    PLAY_AND_RECORD,  // separate play and recording devices
    DUPLEX            // simultaneous record and playback using same device
};

/// Type of audio output if enabled
enum class PlayMode
{
    NONE,
    PULSE,
    PLAYBACK
};

struct Device
{
    unsigned int              id         = 0;
    bool                      is_output  = false;
    bool                      is_default = false;
    unsigned                  n_duplex   = 0;
    std::string               name       = "";
    std::vector<unsigned int> sample_rates{};
    unsigned int              preferred_rate{48000};
    unsigned int              preferred_channels{0};
    RtAudioFormat             preferred_format{RTAUDIO_SINT32};
    Device() = default;

    /**
     * Brief information about a particular device id
     * @param info Device information struct from RtAudio
     * @param device_id device index from all listed devices
     * @param is_playback if device is a playback device or not (opposed to
     * recording)
     */
    Device(const RtAudio::DeviceInfo &info,
           unsigned int               device_id,
           bool                       is_playback)
      : id(device_id), is_output(is_playback)
    {
        is_default = is_output ? info.isDefaultOutput : info.isDefaultInput;
        name       = info.name;
        n_duplex   = info.duplexChannels;
        preferred_format   = info.nativeFormats;
        preferred_rate     = info.preferredSampleRate;
        sample_rates       = info.sampleRates;
        preferred_channels = is_playback ? info.outputChannels :
                                           info.inputChannels;
    };
};

class AudioTimeFile
{
  public:
    std::shared_ptr<std::ofstream> f_ptr;

    AudioTimeFile() = default;

    /**
     * Initialize file for recording audio stream information such as timestamps
     * @param filename name of output file
     */
    void
    init(const std::string &filename)
    {
        misc::makeDirectory(filename);
        f_ptr = std::make_shared<std::ofstream>(filename, std::ofstream::out);
        f_ptr->setf(std::ios_base::fixed);
        f_ptr->precision(5);
        csvHeader();
    };

    void
    csvHeader()
    {
        std::vector<std::string> col_names = {"buffer",
                                              "size",
                                              "audio_time",
                                              "stream_time",
                                              "master_time",
                                              "status"};
        misc::writeCSVHeaders(*f_ptr, col_names);
    };

    bool
    is_open()
    {
        bool opened;
        opened = f_ptr ? f_ptr->is_open() : false;
        return opened;
    };

    void
    close()
    {
        if (is_open())
        {
            f_ptr->close();
        }
    };

    std::shared_ptr<std::ofstream>
    streamPtr()
    {
        if (is_open())
        {
            return f_ptr;
        } else
        {
            throw err::Runtime("Timestamp file is not opened or already open");
        }
    }
};

struct TimeRow
{
    explicit TimeRow(uint64_t      _buff,
                     unsigned      _size,
                     AudioTimeType _audio,
                     AudioTimeType _stream,
                     AudioTimeType _master,
                     int           _err)
      : buffer(std::move(_buff)),
        size(std::move(_size)),
        audio_time(std::move(_audio)),
        stream_time(std::move(_stream)),
        master_time(std::move(_master)),
        status(std::move(_err)){};
    uint64_t      buffer;
    unsigned      size;
    AudioTimeType audio_time;
    AudioTimeType stream_time;
    AudioTimeType master_time;
    int           status;
};

double
timeRescaleVal()
{
    AudioClock clock;
    return static_cast<double>(clock.getSecondsMultiplier());
};

class CallbackTimestamps
{
  protected:
    CallbackTimestamps() = default;

  public:
    AudioTimeFile        file;
    AudioClock           master_clock;
    AudioClock           stream_clock;
    AudioTimeType        stream_ts     = 0;
    AudioTimeType        master_ts     = 0;
    size_t               flush_buffer  = 1;
    uint64_t             buffer_sample = 1;
    futures::FutureState future;

    explicit CallbackTimestamps(const timing::TimePoint &tp,
                                unsigned                 sample_rate,
                                AudioTimeType            timeout_interval,
                                double                   time_threshold)
      : master_clock(tp),
        stream_clock(tp),
        future(futures::makeFutureValid()),
        timeout_reached(static_cast<double>(timeout_interval)),
        timeout_thresh(static_cast<double>(timeout_interval * time_threshold))
    {
        master_ts, stream_ts, audio_ts, start_time = master_clock.elapsed();
        setSampleTime(sample_rate);
        master_clock.setTimeout(
          audio::data::AudioDuration(timeout_reached), time_threshold);
        master_clock.timeout();
        frame_ts = timeout_reached;
        update();
    };

    void
    update()
    {
        master_clock.update();
        stream_clock.update();
        setElapsed();
    };

    void
    streamSync(double sec, bool resync_clock = false)
    {
        if (resync_clock) stream_clock.syncBaseTime(sec);
        setElapsed();
        setAudioTime(sec);
    };

    void
    setAudioTime(double sec)
    {
        audio_ts = sec * time_scaler;
    };

    void
    setBufferSize(unsigned size)
    {
        last_buff_size = size;
    }

    // Update each sample
    bool
    frameUpdate(unsigned size = 1)
    {
        streamTimeIncrement(size);
        return timeoutDecrement(size);
    }

    void
    streamTimeIncrement(unsigned size = 1)
    {
        audio_ts += size * sample_time_ms;
    }

    bool
    timeoutDecrement(unsigned size = 1)
    {
        frame_ts -= size * sample_time_ms;
        if (frame_ts <= 0)
        {
            roll_over = 1.0 +
                        std::floor(-frame_ts / timeout_reached) *
                          timeout_reached +
                        frame_ts;
            if (roll_over < timeout_thresh || roll_over > timeout_reached)
            {
                roll_over = timeout_reached;
            }
            frame_ts = roll_over;
            return true;
        }
        return false;
    }

    // update pulses
    void
    setElapsed()
    {
        master_ts = master_clock.elapsed();
        stream_ts = stream_clock.elapsed();
    };

    // add/write data
    void
    addTimestamp(int err = 0)
    {
        setElapsed();
        rows_fill.emplace_back(TimeRow(
          buffer_sample, last_buff_size, audio_ts, stream_ts, master_ts, err));
    };

    bool
    swapRowsBuffer(bool force)
    {
        auto n_rows = rows_fill.size();
        if (force || (n_rows >= 0 && n_rows >= flush_buffer))
        {
            // wait for row data to be done writing before clearing
            future.wait();
            rows_write.clear();
            rows_write.swap(rows_fill);
            return true;
        } else
        {
            return false;
        }
    };

    void
    clearBuffers()
    {
        future.wait();
        rows_write.clear();
        rows_fill.clear();
        rows_write.shrink_to_fit();
        rows_fill.shrink_to_fit();
    }

    void
    setFilePtr()
    {
        // wait for ofstream to be done before using stream again
        future.wait();
        file_ptr = file.streamPtr();
    };

    std::shared_ptr<std::ofstream>
    getFilePtr()
    {
        future.wait();
        return file_ptr;
    };

    static void
    writeData(std::shared_ptr<std::ofstream> _file, std::vector<TimeRow> &_rows)
    {
        for (auto &row : _rows)
        {
            *_file << row.buffer << "," << row.size << "," << row.audio_time
                   << "," << row.stream_time << "," << row.master_time << ","
                   << row.status << "\n";
        }
        *_file << std::flush;
    };

    void
    writeCallbackTimes(bool force = false)
    {
        if (!swapRowsBuffer(force)) return;
        future = std::async(
          std::launch::async, [this]() { writeData(file_ptr, rows_write); });
    };

    // misc public
    void
    close()
    {
        writeCallbackTimes(true);
        future.wait();
        file.close();
    };

    double
    elapsedSeconds()
    {
        return timing::convertValue<std::chrono::duration<double, timing::second>,
                                    AudioDuration,
                                    AudioTimeType>(master_clock.elapsed());
    };

    void
    setSampleTime(unsigned sample_rate)
    {
        sample_time_ms = time_scaler / static_cast<double>(sample_rate);
    }

    AudioTimeType
    getElapsed()
    {
        return master_clock.elapsed();
    };

  private:
    std::shared_ptr<std::ofstream> file_ptr;
    std::vector<TimeRow>           rows_fill;
    std::vector<TimeRow>           rows_write;
    AudioTimeType                  start_time      = 0;
    double                         timeout_reached = 0;
    double                         timeout_thresh  = 0;
    double                         audio_ts        = 0;
    double                         sample_time_ms  = 0;
    double                         frame_ts        = 0;
    double                         roll_over       = 0;
    unsigned                       last_buff_size  = 0;
    const double                   time_scaler     = timeRescaleVal();
};

struct CallbackOutputData
{
    bool     in_use = false;
    wav::Wav pcm;
    PlayMode mode           = PlayMode::NONE;
    unsigned pulse_width    = 1;
    int      pulse_count    = 0;
    unsigned n_channels     = 0;
    size_t   byte_size      = 0;
    double   bit_mode_multi = 1;
    double   pulse_amp      = 0.93;
};

struct CallbackInputData
{
    bool     in_use = false;
    wav::Wav pcm;
    unsigned n_channels = 0;
    size_t   byte_size  = 0;
};

struct CallbackData
{
    /**
     * User data passed to RTAudio callback function
     * @param tp Set clock to main clock time.
     */
    explicit CallbackData(const timing::TimePoint &tp,
                          unsigned                 buff_size,
                          AudioTimeType            pulse_interval,
                          double                   thresh = 0.1)
      : ts(tp, buff_size, pulse_interval, thresh){};

    CallbackOutputData play;
    CallbackInputData  rec;
    CallbackTimestamps ts;
    size_t             format_sizeof      = 0;
    unsigned           buffer_max_allowed = 0;
    unsigned           buffer_len_now     = 0;
    uint64_t           stop_after         = 0;
    bool               verbose            = false;
    bool               aborted            = false;
    bool               write              = false;
};
};  // namespace data

namespace rt {

RtAudioFormat
int2format(unsigned format_size)
{
    RtAudioFormat bit_rate;
    switch (format_size)
    {
        case 1: bit_rate = RTAUDIO_SINT8; break;
        case 2: bit_rate = RTAUDIO_SINT16; break;
        case 4: bit_rate = RTAUDIO_SINT24; break;
        case 8: bit_rate = RTAUDIO_SINT32; break;
        case 16: bit_rate = RTAUDIO_FLOAT32; break;
        case 32: bit_rate = RTAUDIO_FLOAT64; break;
        default: bit_rate = RTAUDIO_SINT32; break;
    }
    return bit_rate;
};

std::size_t
format2sizeof(RtAudioFormat fmt)
{
    std::size_t size;
    switch (fmt)
    {
        case RTAUDIO_SINT8: size = sizeof(char); break;
        case RTAUDIO_SINT16: size = sizeof(signed short); break;
        case RTAUDIO_SINT24: size = sizeof(S24); break;
        case RTAUDIO_SINT32: size = sizeof(int); break;
        case RTAUDIO_FLOAT32: size = sizeof(float); break;
        case RTAUDIO_FLOAT64: size = sizeof(double); break;
        default: throw err::Runtime("Unknown RtAudioFormat");
    }
    return size;
};

uint32_t
format2bits(RtAudioFormat fmt)
{
    uint32_t bit_depth;
    switch (fmt)
    {
        case RTAUDIO_SINT8: bit_depth = 8; break;
        case RTAUDIO_SINT16: bit_depth = 16; break;
        case RTAUDIO_SINT24: bit_depth = 24; break;
        case RTAUDIO_SINT32: bit_depth = 32; break;
        case RTAUDIO_FLOAT32: bit_depth = 32; break;
        case RTAUDIO_FLOAT64: bit_depth = 64; break;
        default: throw err::Runtime("Unknown RtAudioFormat");
    }
    return bit_depth;
};

double
format2scale(RtAudioFormat fmt)
{
    double scale;
    switch (fmt)
    {
        case RTAUDIO_SINT8: scale = 127.0; break;
        case RTAUDIO_SINT16: scale = 32767.0; break;
        case RTAUDIO_SINT24: scale = 8388607.0; break;
        case RTAUDIO_SINT32: scale = 2147483647.0; break;
        case RTAUDIO_FLOAT32: scale = 1.0; break;
        case RTAUDIO_FLOAT64: scale = 1.0; break;
        default: throw err::Runtime("Unknown RtAudioFormat");
    }
    return scale;
};

std::string
format2string(RtAudioFormat fmt)
{
    std::string size;
    switch (fmt)
    {
        case RTAUDIO_SINT8:
        case RTAUDIO_SINT16:
        case RTAUDIO_SINT24:
        case RTAUDIO_SINT32: size = "sint"; break;
        case RTAUDIO_FLOAT32:
        case RTAUDIO_FLOAT64: size = "float"; break;
        default: throw err::Runtime("Unknown RtAudioFormat");
    }
    return size;
};

int
saveRecorded(audio::data::CallbackData *data, void *buffer_data)
{
    auto *byte_buffer_out = static_cast<char *>(buffer_data);
    auto  size            = data->rec.byte_size * data->buffer_len_now;
    if (data->rec.pcm.isReady() && data->write)
    {
        data->rec.pcm.file.write(byte_buffer_out, size);
    }
    return 0;
};

int
savePlayback(audio::data::CallbackData *data, void *buffer_data)
{
    auto *byte_buffer_out = static_cast<char *>(buffer_data);
    auto  size            = data->play.byte_size * data->buffer_len_now;
    if (data->play.pcm.isReady() && data->write)
    {
        data->play.pcm.file.write(byte_buffer_out, size);
    }
    return 0;
};

template<typename T>
int
playPulse(audio::data::CallbackData *data, T *byte_buffer_ptr)
{
    unsigned frame = 0;
    unsigned chan  = 0;
    T        sample;
    auto     on       = static_cast<T>(data->play.pulse_amp);
    auto     off      = static_cast<T>(-data->play.pulse_amp);
    auto     channels = static_cast<unsigned>(data->play.n_channels *
                                          data->play.bit_mode_multi);

    for (frame = 0; frame < data->buffer_len_now; ++frame)
    {
        if (data->ts.frameUpdate())
        {
            data->play.pulse_count = data->play.pulse_width;
            if (data->write) data->ts.addTimestamp();
        }
        if (data->play.pulse_count > 0)
        {
            sample = on;
            data->play.pulse_count -= 1;
        } else
        {
            sample = off;
        }
        for (chan = 0; chan < channels; ++chan)
        {
            *byte_buffer_ptr++ = sample;
        }
    }

    return 0;
};

int
playBack(audio::data::CallbackData *data, void *in, void *out)
{
    data->ts.addTimestamp();
    auto size = data->play.byte_size * data->buffer_len_now;
    if (data->rec.n_channels == data->play.n_channels)
    {
        memcpy(out, in, size);
    } else
    {
        throw err::Runtime(
          "Cannot copy audio input/output with different channel numbers");
        // TODO: handle different channels for input/output
    }
    return 0;
};

/**
 * To continue normal stream operation, the RtAudioCallback function should
 * return a value of zero. To stop the stream and drain the output buffer, the
 * function should return a value of one. To abort the stream immediately, the
 * client should return a value of two.
 * @param outputBuffer raw output buffer
 * @param inputBuffer raw input buffer
 * @param nFrames number of frames in buffer
 * @param streamTime current stream time value
 * @param status buffer under/overflow error
 * @param userData data passed to callback to be casted
 * @return error code (1= stop and clear, 2= stop abruptly)
 */
int
callback(void *              outputBuffer,
         void *              inputBuffer,
         unsigned int        nFrames,
         double              streamTime,
         RtAudioStreamStatus status,
         void *              userData)
{
    auto *data = static_cast<audio::data::CallbackData *>(userData);
    data->ts.streamSync(streamTime);
    data->ts.setBufferSize(nFrames);
    data->buffer_len_now = nFrames;
    int return_value     = nFrames > data->buffer_max_allowed ? 1 : 0;
    data->ts.addTimestamp(1);
    if (data->rec.in_use)
    {
        if (status == RTAUDIO_INPUT_OVERFLOW)
        {
            if (data->verbose)
            {
                std::cout << "Input data was discarded because of an overflow "
                             "condition at the driver.\n";
            }
            data->ts.addTimestamp(-1);
        }
        return_value += saveRecorded(data, inputBuffer);
    }
    if (data->play.in_use)
    {
        if (status == RTAUDIO_OUTPUT_UNDERFLOW)
        {
            if (data->verbose)
            {
                std::cout << "The output buffer ran low, "
                             "likely producing a break in the output sound.\n";
            }
            data->ts.addTimestamp(-2);
            data->play.pulse_count = 0;
        }
        if (data->play.mode == audio::data::PlayMode::PULSE)
        {
            if (data->format_sizeof < 32)
            {
                return_value += audio::rt::playPulse(
                  data, static_cast<float *>(outputBuffer));
            } else
            {
                return_value += audio::rt::playPulse(
                  data, static_cast<double *>(outputBuffer));
            }
        } else if (data->play.mode == audio::data::PlayMode::PLAYBACK)
        {
            return_value += audio::rt::playBack(
              data, inputBuffer, outputBuffer);
        }
        savePlayback(data, outputBuffer);
    }

    if (data->write)
    {
        data->ts.writeCallbackTimes();
        ++data->ts.buffer_sample;
    } else
    {
        data->ts.clearBuffers();
    }

    if (data->ts.buffer_sample == data->stop_after)
    {
        data->aborted = true;
        return 1;
    }

    return return_value;
};
};  // namespace rt

/**
 * determine if recording, playing, or both
 * @param _play
 * @param _rec
 * @return
 */
audio::data::StreamMode
getAudioMode(const int &_play, const int &_rec)
{
    audio::data::StreamMode mode;
    bool                    use_output    = _play != -1;
    bool                    use_input     = _rec != -1;
    bool                    play_only     = use_output && !use_input;
    bool                    rec_only      = use_input && !use_output;
    bool                    play_rec_mode = use_input && use_output;
    if (play_rec_mode)
    {
        if (_play == _rec)
        {
            mode = audio::data::StreamMode::DUPLEX;
        } else
        {
            mode = audio::data::StreamMode::PLAY_AND_RECORD;
        }
    } else if (rec_only)
    {
        mode = audio::data::StreamMode::RECORD_ONLY;
    } else if (play_only)
    {
        mode = audio::data::StreamMode::PLAY_ONLY;
    } else
    {
        mode = audio::data::StreamMode::NONE;
    }
    return mode;
};

/**
 * return compiled API depending on platform
 * @param api
 * @return
 */
RtAudio::Api
getAPI(unsigned api)
{
    auto rt_api = static_cast<RtAudio::Api>(api);
    if (rt_api == RtAudio::UNSPECIFIED)
    {
        return rt_api;
    }
    std::vector<RtAudio::Api> compiled_apis;
    RtAudio::getCompiledApi(compiled_apis);
    bool is_compiled = false;
    for (auto compiled_api : compiled_apis)
    {
        if (rt_api == compiled_api)
        {
            is_compiled = true;
            break;
        }
    }
    return is_compiled ? rt_api : RtAudio::Api::RTAUDIO_DUMMY;
};

/**
 * Information about a specific audio device number. Can be a recording device
 * or playback device.
 * @param _play
 * @param _rec
 * @param audio
 * @return
 */
std::vector<audio::data::Device>
getRtDeviceInfo(int _play, int _rec, RtAudio *audio)
{
    // check if any audio devices
    auto n_devices = audio->getDeviceCount();
    if (n_devices == 0) throw err::Runtime("No audio devices found!!");
    std::cout << "\n  - rtaudio_api: " << audio->getCurrentApi()
              << "\n  - max_audio_device_index: " << n_devices - 1;

    audio::data::Device              play_device;
    audio::data::Device              rec_device;
    std::vector<audio::data::Device> info(2);
    std::vector<int>                 device_nums = {_play, _rec};

    for (int j = 0; j < 2; ++j)
    {
        bool                 is_output_device = j == 0;
        auto                 proposed_device  = device_nums[j];
        audio::data::Device *info_ptr         = &info[j];

        if (proposed_device == -1)
        {
            continue;
        }
        if (proposed_device >= 0)
        {
            auto device_id = static_cast<unsigned int>(proposed_device);
            std::cout << "\nChecking device number " << device_id << "\n";
            auto returned_info = audio->getDeviceInfo(device_id);
            if (returned_info.probed)
            {
                *info_ptr = audio::data::Device(
                  returned_info, device_id, is_output_device);
            } else
            {
                throw err::Runtime(
                  misc::strAppend({"Device number:",
                                   std::to_string(device_id),
                                   "was busy or unavailable!"}));
            }
        } else
        {
            throw err::Runtime(misc::strAppend(
              {"Invalid device number:", std::to_string(proposed_device)}));
        }
    }

    return info;
};

/**
 * Check if devices have been set to value >= 0
 * @param mode
 * @return
 */
bool
devicesOn(const audio::data::StreamMode &mode)
{
    auto device_nums_set = true;
    if (mode == audio::data::StreamMode::NONE)
    {
        std::cout << "\nAll audio devices are set to -1 (off)\n";
        device_nums_set = false;
    }
    return device_nums_set;
}

bool
apiCompiled(RtAudio::Api &api)
{
    auto api_found = true;
    if (api == RtAudio::Api::RTAUDIO_DUMMY)
    {
        std::cout << "\nNo RtAudio API was compiled!\n";
        api_found = false;
    }
    return api_found;
};

void
fillStreamParameters(const param::data::audio::stream &pars,
                     const data::Device &              info,
                     RtAudio::StreamParameters *       stream_ref)
{
    stream_ref->deviceId     = info.id;
    stream_ref->firstChannel = pars.first_channel;
    if (pars.n_channels == 0)
    {
        stream_ref->nChannels = info.preferred_channels;
    } else
    {
        stream_ref->nChannels = pars.n_channels;
    }
};

/**
 * Check for different values in common parameters
 * @param play_rate
 * @param rec_rate
 * @param mode
 * @param play_info
 * @param rec_info
 * @return
 */
unsigned int
chooseSampleRate(unsigned int               play_rate,
                 unsigned int               rec_rate,
                 audio::data::StreamMode    mode,
                 const audio::data::Device &play_info,
                 const audio::data::Device &rec_info)
{
    unsigned int sample_rate = 0;
    switch (mode)
    {
        case audio::data::StreamMode::PLAY_AND_RECORD:
        case audio::data::StreamMode::DUPLEX:
        {
            auto best_play     = misc::max(play_info.sample_rates);
            auto best_rec      = misc::max(rec_info.sample_rates);
            auto best_i_can_do = misc::min<unsigned int>({best_play, best_rec});
            auto min_preferred = misc::min<unsigned int>(
              {play_info.preferred_rate, rec_info.preferred_rate});
            auto both_zero = play_rate == 0 && rec_rate == 0;
            if (both_zero)
            {
                sample_rate = min_preferred;
            } else if (play_rate == rec_rate)
            {
                sample_rate = play_rate;
            } else if (play_rate > rec_rate)
            {
                sample_rate = play_rate;
            } else if (rec_rate > play_rate)
            {
                sample_rate = rec_rate;
            } else
            {
                sample_rate = best_i_can_do;
            }
            if (sample_rate > best_i_can_do)
            {
                sample_rate = min_preferred;
            }
        }
        break;
        case audio::data::StreamMode::PLAY_ONLY: { sample_rate = play_rate;
        }
        break;
        case audio::data::StreamMode::RECORD_ONLY: { sample_rate = rec_rate;
        }
        break;
        case audio::data::StreamMode::NONE: { sample_rate = 0;
        }
        break;
    }
    return sample_rate;
};

RtAudioFormat
chooseBitDepth(unsigned                   bit_depth,
               const audio::data::Device &play_info,
               const audio::data::Device &rec_info)
{
    return bit_depth > 0 ?
             rt::int2format(bit_depth) :
             misc::min<RtAudioFormat>(
               {play_info.preferred_format, rec_info.preferred_format});
};

void
setFlagsOptions(const param::data::audio &audio,
                RtAudio::StreamOptions *  options)
{
    RtAudioStreamFlags flags{0};

    if (audio.min_latency) flags = flags | RTAUDIO_MINIMIZE_LATENCY;
    if (audio.linux_default) flags = flags | RTAUDIO_ALSA_USE_DEFAULT;
    if (audio.hog_device) flags = flags | RTAUDIO_HOG_DEVICE;

    options->numberOfBuffers = 2;
    options->flags           = flags;
};

audio::data::PlayMode
getPlayMode(const std::string &playback_mode)
{
    if (playback_mode == "pulse" || playback_mode == "square")
    {
        return audio::data::PlayMode::PULSE;
    }
    if (playback_mode == "record" || playback_mode == "input" ||
        playback_mode == "playback")
    {
        return audio::data::PlayMode::PLAYBACK;
    }
    return audio::data::PlayMode::NONE;
};

std::string
makeFilename(const std::string &save_dir,
             const std::string &type,
             RtAudioFormat      format,
             unsigned int       sample_rate,
             unsigned int       channels,
             const std::string &file_id,
             const std::string &file_ext)
{
    return save_dir + "/" + file_id + "/audio" + type + " --" +
           std::to_string(audio::rt::format2bits(format)) + "bit-" +
           audio::rt::format2string(format) + " --" +
           std::to_string(sample_rate) + "Hz" + " --" +
           std::to_string(channels) + "ch" + " " + file_id + file_ext;
}

bool
isRecording(const audio::data::StreamMode &stream_mode)
{
    bool is_rec;
    switch (stream_mode)
    {
        case audio::data::StreamMode::RECORD_ONLY:
        case audio::data::StreamMode::PLAY_AND_RECORD:
        case audio::data::StreamMode::DUPLEX: is_rec = true; break;
        default: is_rec = false;
    }
    return is_rec;
};

void
checkStreamValues(RtAudio::StreamParameters *out,
                  RtAudio::StreamParameters *in,
                  const unsigned &           srate)
{
    if (out != nullptr)
    {
        if (out->nChannels <= 0)
        {
            throw err::Runtime("Incorrect number of channels for output");
        }
    }
    if (in != nullptr)
    {
        if (in->nChannels <= 0)
        {
            throw err::Runtime("Incorrect number of channels for input");
        }
    }
    if (srate <= 0)
    {
        throw err::Runtime("Invalid sample rate");
    }
};

class StreamData
{
  public:
    StreamData() = default;

    /**
     * Base data class that does all the processing of command line options and
     * collects data to store in this class
     * @param opts Command line options object
     */
    explicit StreamData(const opts::Pars &opts)
      : main_audio(getAPI(opts.audio.api_enumerator))
    {
        verbose = opts.basic.verbose;
        if (verbose) main_audio.showWarnings(true);
        api           = main_audio.getCurrentApi();
        playback_mode = getPlayMode(opts.audio.playback_mode);
        audio_mode    = getAudioMode(
          opts.audio.playback.device_id, opts.audio.record.device_id);
        if (devicesOn(audio_mode) && apiCompiled(api))
        {
            info = getRtDeviceInfo(opts.audio.playback.device_id,
                                   opts.audio.record.device_id,
                                   &main_audio);
            fixConflictingOptions(opts.audio);
            setFlagsOptions(opts.audio, &options);
            use_input_device  = isRecording(audio_mode);
            use_output_device = playback_mode != audio::data::PlayMode::NONE;

            // store misc options
            buffer_size         = opts.audio.buffer_size;
            pulse_rate          = opts.audio.pulse_rate;
            pulse_width         = opts.audio.pulse_width;
            record_duration_sec = opts.audio.record_duration_sec;
            save_playback       = opts.audio.save_playback;
        } else
        {
            use_audio = false;
        }

        audioFileName(opts.basic.root_save_folder, opts.basic.file_identifier);
    };

    // Data needed for RtAudio open/close streams
    RtAudio                    main_audio{};
    audio::data::StreamMode    audio_mode{audio::data::StreamMode::NONE};
    audio::data::PlayMode      playback_mode{audio::data::PlayMode::NONE};
    RtAudio::Api               api = RtAudio::UNSPECIFIED;
    RtAudio::StreamParameters  playback{};
    RtAudio::StreamParameters  record{};
    RtAudio::StreamParameters *playback_ptr = nullptr;
    RtAudio::StreamParameters *record_ptr   = nullptr;
    RtAudio::StreamOptions     options{};
    RtAudioFormat              rt_format           = RTAUDIO_SINT32;
    unsigned int               sample_rate         = 44100;
    unsigned                   buffer_size         = 512;
    double                     record_duration_sec = 0;
    double                     pulse_rate          = 0;
    unsigned                   pulse_width         = 2;
    bool                       use_audio           = true;
    bool                       use_input_device    = false;
    bool                       use_output_device   = false;
    bool                       save_playback       = false;
    bool                       verbose             = false;
    std::string                timestamp_filename  = "";
    std::string                recording_filename  = "";
    std::string                playback_filename   = "";

  private:
    // fill in rest of options
    void
    fixConflictingOptions(const param::data::audio &audio)
    {
        sample_rate = chooseSampleRate(audio.playback.sample_rate,
                                       audio.record.sample_rate,
                                       audio_mode,
                                       info[0],
                                       info[1]);
        rt_format   = chooseBitDepth(audio.bit_depth, info[0], info[1]);
        fillStreamParameters(audio.playback, info[0], &playback);
        fillStreamParameters(audio.record, info[1], &record);
    };

    // make filenames for output stream and timestamp files
    void
    audioFileName(const std::string &folder, const std::string &name)
    {

        if (use_audio)
        {
            timestamp_filename = makeFilename(
              folder,
              "_ts",
              rt_format,
              sample_rate,
              record.nChannels + playback.nChannels,
              name,
              ".csv");
            if (use_input_device)
            {
                recording_filename = makeFilename(folder,
                                                  "_input",
                                                  rt_format,
                                                  sample_rate,
                                                  record.nChannels,
                                                  name,
                                                  ".wav");
            }
            if (use_output_device && save_playback)
            {
                playback_filename = makeFilename(folder,
                                                 "_output",
                                                 rt_format,
                                                 sample_rate,
                                                 playback.nChannels,
                                                 name,
                                                 ".wav");
            }
        }
    };

    std::vector<audio::data::Device> info{2};
};

/**
 * Main class to interact with the audio input/output devices
 */
class Streams : public StreamData
{
  public:
    audio::data::CallbackData callback;

    /**
     * Construct object using user-specified command line options
     * @param opts Class holding the audio options subsection
     * @param t Start the stream time point at the value of some main clock
     */
    explicit Streams(
      const opts::Pars &       opts,
      const timing::TimePoint &tp = timing::nowTP<timing::TimePoint>())
      : StreamData(opts),
        callback(tp,
                 sample_rate,
                 static_cast<audio::data::AudioTimeType>(
                   audio::data::timeRescaleVal() / pulse_rate),
                 0.1)
    {
        init();
    };

    void
    open(double start_time = 0)
    {
        if (!use_audio) return;
        openStream();
        updateCallback();
        start(start_time);
    };

    void
    open(const timing::TimePoint &tp)
    {
        if (!use_audio) return;
        callback.ts.stream_clock.set(tp);
        callback.ts.master_clock.set(tp);
        callback.ts.streamSync(0, true);
        open(0);
    };

    void
    start(double start_time = -1)
    {
        if (!use_audio) return;
        if (main_audio.isStreamOpen())
        {
            if (main_audio.isStreamRunning())
            {
                if (verbose) std::cout << "\nStream already started.\n";
                return;
            }
            try
            {
                setTime(start_time);
                main_audio.startStream();
            } catch (RtAudioError &err)
            {
                err.printMessage();
                throw err::Runtime(err);
            }
        } else
        {
            throw err::Runtime("Tried to start but stream not open.");
        }
    };

    void
    stop(bool close_stream = false)
    {
        if (!use_audio) return;
        if (main_audio.isStreamOpen())
        {
            if (main_audio.isStreamRunning())
            {
                if (verbose) std::cout << "\nAttempting to stop stream.\n";
                try
                {
                    main_audio.stopStream();
                } catch (RtAudioError &err)
                {
                    throw err::Runtime(err);
                }
            }
            if (close_stream)
            {
                if (verbose) std::cout << "\nClosing stream.\n";
                try
                {
                    main_audio.closeStream();
                } catch (RtAudioError &err)
                {
                    throw err::Runtime(err);
                }
            }
        }
    };

    void
    toggleSave(bool save_state, double start_time = -1)
    {
        if (!use_audio) return;
        if (verbose) std::cout << "\nChanging save state.\n";
        if (main_audio.isStreamOpen())
        {
            stop();
            callback.write = save_state;
            start(start_time);
        } else
        {
            callback.write = save_state;
            setTime(start_time);
        }
    };

    void
    close()
    {
        if (!use_audio) return;
        stop(true);
        callback.ts.close();
        callback.rec.pcm.close();
        callback.play.pcm.close();
    };

    bool
    running()
    {
        if (!use_audio) return false;
        if (callback.aborted)
        {
            close();
            return false;
        }
        return main_audio.isStreamRunning();
    };

  private:
    void
    init()
    {
        if (!use_audio) return;
        callback.verbose = verbose;
        callback.ts.file.init(timestamp_filename);
        callback.ts.setFilePtr();
        callback.format_sizeof      = audio::rt::format2sizeof(rt_format);
        callback.buffer_max_allowed = static_cast<unsigned int>(
          buffer_size + std::round(buffer_size * .25));
        if (use_input_device)
        {
            record_ptr              = &record;
            callback.rec.n_channels = record.nChannels;
            callback.rec.pcm.init(
              recording_filename,
              static_cast<uint16_t>(callback.rec.n_channels),
              sample_rate,
              audio::rt::format2bits(rt_format),
              audio::rt::format2string(rt_format));
        }
        if (use_output_device)
        {
            playback_ptr              = &playback;
            callback.play.n_channels  = playback.nChannels;
            callback.play.pulse_width = pulse_width;
            if (save_playback)
            {
                callback.play.pcm.init(
                  playback_filename,
                  static_cast<uint16_t>(callback.play.n_channels),
                  sample_rate,
                  audio::rt::format2bits(rt_format),
                  audio::rt::format2string(rt_format));
            }
        }
    };

    void
    openStream()
    {
        if (!use_audio) return;
        stop(true);
        checkStreamValues(playback_ptr, record_ptr, sample_rate);
        try
        {
            main_audio.openStream(
              playback_ptr,
              record_ptr,
              rt_format,
              sample_rate,
              &buffer_size,
              static_cast<RtAudioCallback>(&audio::rt::callback),
              &callback,
              &options,
              nullptr);
            if (main_audio.isStreamOpen())
            {
                std::cout
                  << "\n  - output_device: " << playback.deviceId
                  << "\n  - input_device: " << record.deviceId
                  << "\n  - stream_time_sec: " << main_audio.getStreamTime()
                  << "\n  - stream_latency: " << main_audio.getStreamLatency()
                  << "\n  - stream_sample_rate: "
                  << main_audio.getStreamSampleRate()
                  << "\n  - stream_num_buffers: " << options.numberOfBuffers
                  << std::endl;
            } else
            {
                std::cout << "\nStream not opened!\n";
            }
        } catch (RtAudioError &err)
        {
            err.printMessage();
            throw err::Runtime(err);
        }
    };

    double
    setTime(double sec = -1)
    {
        if (sec >= 0) main_audio.setStreamTime(sec);
        auto stream_time_now = main_audio.getStreamTime();
        callback.ts.streamSync(stream_time_now);
        return stream_time_now;
    };

    /**
     * Make changes to callback/stream parameters after updates from opening the
     * audio device
     */
    void
    updateCallback()
    {
        sample_rate             = main_audio.getStreamSampleRate();
        callback.rec.in_use     = use_input_device;
        callback.play.in_use    = use_output_device;
        callback.play.mode      = playback_mode;
        callback.rec.byte_size  = getByteSize(callback.rec.n_channels);
        callback.play.byte_size = getByteSize(callback.play.n_channels);
        callback.stop_after     = timeToBuffers();
        callback.ts.setSampleTime(sample_rate);
        playBackUpdates();
    }

    void
    playBackUpdates()
    {
        if (callback.play.mode == audio::data::PlayMode::PULSE)
        {
            callback.ts.flush_buffer = static_cast<size_t>(
              std::round(pulse_rate));

            if (rt_format == RTAUDIO_FLOAT64)
            {
                callback.play.bit_mode_multi = 2;
                callback.play.pulse_amp *= 2;
            }
            if (sample_rate / pulse_rate < 2)
            {
                throw err::Runtime("Pulse rate too high for on/off samples");
            }
            if ((sample_rate / (pulse_width + 1)) / pulse_rate < 1)
            {
                throw err::Runtime("Pulse width too high for given pulse rate");
            }
        }
    };

    size_t
    getByteSize(unsigned num_channels) const
    {
        return callback.format_sizeof * num_channels;
    }

    uint64_t
    timeToBuffers() const
    {
        return static_cast<uint64_t>(
          std::ceil(static_cast<double>(sample_rate) * record_duration_sec /
                    static_cast<double>(buffer_size)));
    };
};
};  // namespace audio

#endif  // __COGDEVCAM_AUDIO_H
