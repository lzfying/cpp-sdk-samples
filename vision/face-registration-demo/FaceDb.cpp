#include "FaceDb.hpp"
#include "VideoReader.hpp"
#include "FileUtils.hpp"

#include <iostream>
#include <iomanip>
#include <signal.h>

using namespace affdex;
using namespace std;

bool FaceDb::exitLoop_ = false;
void FaceDb::interrupt(int sig) {
    exitLoop_ = true;
}

FaceDb::FaceDb(const string data_dir)
    : face_registrar_(data_dir)
{}

string FaceDb::registeredIds() const {
    vector<identity> IDs = face_registrar_.getIdentities();

    string separator;
    string output;
    for (auto id : IDs) {
        output += separator + std::to_string(id);
        separator = " + ";
    }
    return output;
}

void FaceDb::showScore(const int score) const
{
    cout << "\rRegistration Score: " << std::setw(3) << score << " ";
}

int FaceDb::processFrame(const cv::Mat &frame, const int identifier)
{
    vision::Frame image(frame.size().width, frame.size().height, frame.data, vision::Frame::ColorFormat::BGR);

    vision::BoundingBox frame_bb =
        vision::BoundingBox(vision::Point(0, 0), vision::Point(image.getWidth(), image.getHeight()));

    vision::FaceRegistrationResult fr_result = face_registrar_.registerFace(identifier, image, frame_bb);
    if (fr_result.face_found) {
        showScore(fr_result.score);
    };

    return fr_result.score;
}

void FaceDb::videoRegister(const int identifier, path filename,
                                const int frame_sampling_rate) {
    filename = validatePath(filename);
    VideoReader video_reader(filename, frame_sampling_rate);

    cout << endl << "Press ctrl-C to exit, or wait for Registration Score to reach 100" << endl << endl;
    signal(SIGINT, FaceDb::interrupt);

    int score = 0;
    showScore(score);

    cv::Mat frame;
    timestamp timestamp_ms;
    // Iterate over frames of video and attempt to register a face in it.
    while (!exitLoop_ && score < 100 && video_reader.GetFrame(frame, timestamp_ms)) {
        processFrame(frame, identifier);
    }
    cout << endl << endl;
}

void FaceDb::webcamRegister(const int identifier,
                                 const int camera_id) {
    // Connect to the webcam
    cv::VideoCapture webcam;
    int apiID = CV_CAP_ANY;
    webcam.open(camera_id + apiID);
    if (!webcam.isOpened()) {
        throw runtime_error("Error opening webcam");
    }

    cout << endl << "Press ctrl-C or Esc exit, or wait for Registration Score to reach 100" << endl << endl;
    signal(SIGINT, FaceDb::interrupt);

    int score = 0;
    showScore(score);

    cv::Mat frame;
    // Iterate over frames of video and attempt to register a face in it.
    while (!exitLoop_ && score < 100) {
        if (!webcam.read(frame)){
            throw runtime_error("Failed to read frame from webcam");
        }

        imshow("Live", frame);
        score = processFrame(frame, identifier);

        // show live img and check for ESC key
        int key = cv::waitKey(30);
        if (key == 27) { // Esc
            break;
        }
    }
    cout << endl << endl;
}

void FaceDb::list() const {
    cout << "Registered IDs: " << registeredIds() << endl;
}

void FaceDb::unregister(const int identifier) const {
    face_registrar_.unregister(identifier);
    cout << "Unregistered ID: " << identifier << endl;
}

void FaceDb::unregisterAll() const {
    cout << "IDs to unregister: " << registeredIds() << endl;;
    face_registrar_.unregisterAll();
    cout << "Successfully unregistered all IDs." << endl;
}
