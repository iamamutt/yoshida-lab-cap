/**
    project: cogdevcam
    source file: main
    description: main program

    @author Joseph M. Burling
    @version 0.9.1 12/14/2017
*/

#include "cogdevcam.h"

int
main(int argc, const char *const *argv)
{
    try
    {
        cv::Mat    img;
        opts::Pars my_opts(argc, argv);
        CogDevCam  cogdevcam(my_opts);
        cogdevcam.run();
        return cogdevcam.closeAll();
    } catch (const std::exception &err)
    {
        std::cout << err.what() << "\n";
        return 1;
    }
}
