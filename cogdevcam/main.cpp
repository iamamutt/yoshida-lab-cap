/**
    project: cogdevcam
    source file: main
    description: main program

    @author Joseph M. Burling
    @version 0.9.2 12/18/2017
*/

#include "cogdevcam.h"

int
main(int argc, const char *const *argv)
{
    try
    {
        cv::Mat    img;
        opts::Pars cmd_line_opts(argc, argv);
        CogDevCam  cogdevcam(cmd_line_opts);
        cogdevcam.run();
        return cogdevcam.closeAll();
    } catch (const std::exception &err)
    {
        std::cout << err.what() << "\n";
        return 1;
    }
}
