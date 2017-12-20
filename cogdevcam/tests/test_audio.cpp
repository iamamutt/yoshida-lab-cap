/**
    project: cogdevcam
    source file: test_audio
    description:

    @author Joseph M. Burling
    @version 0.9.2 12/19/2017
*/

#include "audio.h"

/*!
 * Test audio portion of program.
 *   test_audio -d test -f temp -a 0 --aout 0 --aomode pulse --apulse 10 --aplaywav
 * @param argc
 * @param argv
 * @return
 */
int
main(int argc, const char *const *argv)
{
    timing::Clock<timing::unit_sec_flt> clock;
    try
    {
        opts::Pars     cmd_line_opts(argc, argv);
        audio::Streams audio_stream(cmd_line_opts);
        if (audio_stream.callback.stop_after <= 0)
        {
            audio_stream.setRunDuration(5);
        }
        std::cout << "\n...Opening test stream\n";
        audio_stream.open();
        std::cout << "\n...Running test stream. Play only\n";
        clock.reset();
        while (clock.elapsed() < 2.5)
        {
            timing::sleep::sec(0.1);
        };
        std::cout << "\n...Running test stream. Write\n";
        audio_stream.toggleSave(true);
        while (audio_stream.isRunning())
        {
            timing::sleep::sec(0.1);
        };
        std::cout << "\n...Closing test stream\n";
        audio_stream.close();
        std::cout << "\n...End test\n";
    } catch (const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        return 1;
    }
}
