#include "Application.hpp"
#include "FileUtils.hpp"
#include "VideoReader.hpp"

#include <iostream>

using namespace affdex;
using namespace std;

namespace po = boost::program_options;

#define DATA_DIR_ENV_VAR "AFFECTIVA_VISION_DATA_DIR"

Application::Application(int argsc, char **argsv) : command_line_(argsc, argsv) {}

void Application::run() {
    if (command_line_.cmd_ == "register") {
        setCommonOptions(" register <identifier> <video file>\n\n"
                         "Registers a face found in the <video> file with the provided <identifier>",
                         command_line_);

        unsigned int frame_sampling_rate = 0;
        options_->add_options()("sample_rate,s", po::value<unsigned int>(&frame_sampling_rate),
                                "sample every n frames (0=all)");

        if (parse(2)) {
            registerFace(atoi(command_line_.required_[0].c_str()), command_line_.required_[1],
                         frame_sampling_rate);
        }
    } else if (command_line_.cmd_ == "list") {
        setCommonOptions(" list\n\n"
                         "Lists ids for all registered faces", command_line_);
        if (parse(0)) {
            list();
        }
    } else if (command_line_.cmd_ == "unregister") {
        setCommonOptions(" unregister <identifier>\n\n"
                         "Removes registration information from database for provided <identifier>", command_line_);
        if (parse(1)) {
            unregister(atoi(command_line_.required_[0].c_str()));
        }

    } else if (command_line_.cmd_ == "unregister_all") {
        setCommonOptions(" unregister_all\n\n"
                         "Removes all registration information from the database", command_line_);
        if (parse(0)) {
            unregisterAll();
        }

    } else {
        cout << "Application for demoing the Affectiva face registration\n"
             << "\n"
             << "usage " + command_line_.app_name_ + " <register|list|unregister|unregister_all> ... " << endl;
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
        face_registrar_.reset(new vision::FaceRegistrar(data_dir_));
    }
    return successful;
}
string Application::registeredIds() const {
    vector<identity> IDs = face_registrar_->getIdentities();

    string separator;
    string output;
    for (auto id : IDs) {
        output += separator + std::to_string(id);
        separator = " + ";
    }
    return output;
}

void Application::registerFace(const int identifier, path filename,
                               const int frame_sampling_rate) const {
    filename = validatePath(filename);
    VideoReader video_reader(filename, frame_sampling_rate);

    cv::Mat mat;
    timestamp timestamp_ms;

    // Iterate over frames of video and attempt to register a face in it.
    while (video_reader.GetFrame(mat, timestamp_ms)) {
        vision::Frame image(mat.size().width, mat.size().height, mat.data, vision::Frame::ColorFormat::BGR,
                            timestamp_ms);

        vision::BoundingBox frame_bb =
            vision::BoundingBox(vision::Point(0, 0), vision::Point(image.getWidth(), image.getHeight()));

        vision::FaceRegistrationResult fr_result = face_registrar_->registerFace(identifier, image, frame_bb);
        if (fr_result.face_found) {
            cout << "Registered face at timestamp " << timestamp_ms << endl;
        };
    }
}

void Application::list() const {
    cout << "Registered IDs: " << registeredIds() << endl;
}

void Application::unregister(const int identifier) const {
    face_registrar_->unregister(identifier);
    cout << "Unregistered ID: " << identifier << endl;
}

void Application::unregisterAll() const {
    cout << "IDs to unregister: " << registeredIds() << endl;;
    face_registrar_->unregisterAll();
    cout << "Successfully unregistered all IDs." << endl;
}
