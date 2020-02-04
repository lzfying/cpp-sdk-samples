#include "Application.hpp"
#include "FileUtils.hpp"
#include "FaceDb.hpp"

#include <iostream>
#include <signal.h>

using namespace affdex;
using namespace std;

namespace po = boost::program_options;

#define DATA_DIR_ENV_VAR "AFFECTIVA_VISION_DATA_DIR"

Application::Application(int argsc, char **argsv) : command_line_(argsc, argsv) {}

Application::~Application() {}

void Application::run() {
    // Parse command line and invoke appropriate function
    if (command_line_.cmd_ == "video-register") {
        setCommonOptions(" video-register <identifier> <video file>\n\n"
                         "Registers a face found in the <video> file with the provided <identifier>",
                         command_line_);

        unsigned int frame_sampling_rate = 0;
        options_->add_options()("sample_rate,s", po::value<unsigned int>(&frame_sampling_rate),
                                "sample at most N frames per second (0=all)");

        if (parse(2)) {
            faceDb_->videoRegister(atoi(command_line_.required_[0].c_str()), command_line_.required_[1],
                          frame_sampling_rate);
        }

    } else if (command_line_.cmd_ == "webcam-register") {
        setCommonOptions(" video-register <identifier>\n\n"
                         "Registers a face found using webcam with the provided <identifier>",
                         command_line_);

        int camera_id = 0;
        options_->add_options()("cid,c", po::value<int>(&camera_id),
                                "Camera ID");

        if (parse(1)) {
            faceDb_->webcamRegister(atoi(command_line_.required_[0].c_str()), camera_id);
        }

    } else if (command_line_.cmd_ == "list") {
        setCommonOptions(" list\n\n"
                         "Lists ids for all registered faces", command_line_);
        if (parse(0)) {
            faceDb_->list();
        }

    } else if (command_line_.cmd_ == "unregister") {
        setCommonOptions(" unregister <identifier>\n\n"
                         "Removes registration information from database for provided <identifier>", command_line_);
        if (parse(1)) {
            faceDb_->unregister(atoi(command_line_.required_[0].c_str()));
        }

    } else if (command_line_.cmd_ == "unregister_all") {
        setCommonOptions(" unregister_all\n\n"
                         "Removes all registration information from the database", command_line_);
        if (parse(0)) {
            faceDb_->unregisterAll();
        }

    } else {
        cout << "Application for demoing the Affectiva face registration\n"
             << "\n"
             << "usage " + command_line_.app_name_ + " <video-register|webcam-register|list|unregister|unregister_all> ... " << endl;
        return;
    }
}

void Application::setCommonOptions(const string &usage, const SubCommandParser &command_line) {
    options_.reset(new po::options_description("usage: " + command_line.app_name_
                                               + usage+ " [options]\n\nOptions:"));

    options_->add_options()("help,h", po::bool_switch()->default_value(false), "Display this help message");
    options_->add_options()(
        "data,d", po::value<path>(&data_dir_),
        (string("Path to the data folder. Alternatively, specify the path via the environment variable ") +
         DATA_DIR_ENV_VAR + "=/path/to/data")
        .c_str());

}

bool Application::parse(const int required_param_count){
    bool successful = command_line_.process(*options_, required_param_count);
    if (successful){
        data_dir_ = validatePath(data_dir_, DATA_DIR_ENV_VAR);
        faceDb_.reset(new FaceDb(data_dir_));
    }
    return successful;
}
