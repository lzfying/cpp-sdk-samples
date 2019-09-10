#include "PlottingImageListener.h"
#include "StatusListener.h"

#include <Platform.h>
#include <FrameDetector.h>
#include <SyncFrameDetector.h>

#include <opencv2/highgui/highgui.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <iomanip>

static const std::string DATA_DIR_ENV_VAR = "AFFECTIVA_VISION_DATA_DIR";
#ifdef _WIN32
static const std::wstring WIDE_DATA_DIR_ENV_VAR=L"AFFECTIVA_VISION_DATA_DIR";
#endif

using namespace std;
using namespace affdex;

class VideoReader {
public:
    VideoReader(const boost::filesystem::path& file_path, const unsigned int sampling_frame_rate) :
        sampling_frame_rate(sampling_frame_rate) {

        if (sampling_frame_rate < 0)
            throw runtime_error("Specified sampling rate is < 0");

        last_timestamp_ms = sampling_frame_rate == 0 ? -1 : (0 - 1000 / sampling_frame_rate); // Initialize so that with sampling, we always process the first frame.


        std::set<boost::filesystem::path> SUPPORTED_EXTS = {
            // Videos
            boost::filesystem::path(".avi"),
            boost::filesystem::path(".mov"),
            boost::filesystem::path(".flv"),
            boost::filesystem::path(".webm"),
            boost::filesystem::path(".wmv"),
            boost::filesystem::path(".mp4"),
        };

        boost::filesystem::path ext = file_path.extension();
        if (SUPPORTED_EXTS.find(ext) == SUPPORTED_EXTS.end()) {
            throw runtime_error("Unsupported file extension: " + ext.string());
        }

        cap.open(file_path.string());
        if (!cap.isOpened())
            throw runtime_error("Error opening video/image file: " + file_path.string());
    }

    bool GetFrame(cv::Mat &bgr_frame, timestamp& timestamp_ms) {
        bool frame_data_loaded;

        do {
            frame_data_loaded = GetFrameData(bgr_frame, timestamp_ms);
        } while ((sampling_frame_rate > 0)
            && (timestamp_ms > 0)
            && ((timestamp_ms - last_timestamp_ms) < 1000 / sampling_frame_rate)
            && frame_data_loaded);

        last_timestamp_ms = timestamp_ms;
        return frame_data_loaded;
    }

    bool GetFrameData(cv::Mat &bgr_frame, timestamp& timestamp_ms) {
        static const int MAX_ATTEMPTS = 2;
        timestamp prev_timestamp_ms = cap.get(::CV_CAP_PROP_POS_MSEC);
        bool frame_found = cap.grab();
        bool frame_retrieved = cap.retrieve(bgr_frame);
        timestamp_ms = cap.get(::CV_CAP_PROP_POS_MSEC);

        // Two conditions result in failure to decode (grab/retrieve) a video frame (timestamp reports 0):
        // (1) error on a particular frame
        // (2) end of the video file
        //
        // This workaround double-checks that a subsequent frame can't be decoded, in the absence
        // of better reporting on which case has been encountered.
        //
        // In the case of reading an image, first attempt will not return a new frame, but the second one will
        // succeed. So as a workaround, the new timestamp must be greater than the previous one.
        int n_attempts = 0;
        while (!(frame_found && frame_retrieved) && n_attempts++ < MAX_ATTEMPTS) {
            frame_found = cap.grab();
            frame_retrieved = cap.retrieve(bgr_frame);
            timestamp_ms = cap.get(::CV_CAP_PROP_POS_MSEC);
        }

        if (frame_found && frame_retrieved && n_attempts > 0) {
            if (timestamp_ms <= prev_timestamp_ms) {
                frame_found = false;
            }
        }

        return frame_found && frame_retrieved;
    }

private:

    cv::VideoCapture cap;
    timestamp last_timestamp_ms;
    unsigned int sampling_frame_rate;
};


int main(int argsc, char ** argsv) {

    const int precision = 2;
    std::cerr << std::fixed << std::setprecision(precision);
    std::cout << std::fixed << std::setprecision(precision);

    // cmd line args
    affdex::path data_dir;
    affdex::path video_path;
    unsigned int sampling_frame_rate;
    bool draw_display;
    unsigned int num_faces;
    bool loop = false;
    bool draw_id = false;
    bool disable_logging = false;

    namespace po = boost::program_options; // abbreviate namespace

    po::options_description description("Project for demoing the Affectiva FrameDetector class (processing video files).");
    description.add_options()
        ("help,h", po::bool_switch()->default_value(false), "Display this help message.")
#ifdef _WIN32
        ("data,d", po::wvalue<affdex::path>(&data_dir),
            std::string("Path to the data folder. Alternatively, specify the path via the environment variable "
                + DATA_DIR_ENV_VAR + R"(=\path\to\data)").c_str())
        ("input,i", po::wvalue<affdex::path>(&video_path)->required(), "Video file to processs")
#else // _WIN32
        ("data,d", po::value< affdex::path >(&data_dir),
            (std::string("Path to the data folder. Alternatively, specify the path via the environment variable ")
            + DATA_DIR_ENV_VAR + "=/path/to/data").c_str())
        ("input,i", po::value< affdex::path >(&video_path)->required(), "Video file to processs")
#endif // _WIN32
        ("sfps", po::value<unsigned int>(&sampling_frame_rate)->default_value(0), "Input sampling frame rate. Default is 0, which means the app will respect the video's FPS and read all frames")
        ("draw", po::value<bool>(&draw_display)->default_value(true), "Draw video on screen.")
        ("numFaces", po::value<unsigned int>(&num_faces)->default_value(1), "Number of faces to be tracked.")
        ("loop", po::bool_switch(&loop)->default_value(false), "Loop over the video being processed.")
        ("face_id", po::bool_switch(&draw_id)->default_value(false), "Draw face id on screen. Note: Drawing to screen should be enabled.")
        ("quiet,q", po::bool_switch(&disable_logging)->default_value(false), "Disable logging to console")
        ;

    po::variables_map args;

    try {
        po::store(po::command_line_parser(argsc, argsv).options(description).run(), args);
        if (args["help"].as<bool>()) {
            std::cout << description << std::endl;
            return 0;
        }
        po::notify(args);
    }
    catch (po::error& e) {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cerr << "For help, use the -h option." << std::endl << std::endl;
        return 1;
    }

    // set data_dir to env_var if not set on cmd line
#ifdef _WIN32
    wchar_t* vision_env = _wgetenv(WIDE_DATA_DIR_ENV_VAR.c_str());
#else
    char* vision_env = std::getenv(DATA_DIR_ENV_VAR.c_str());
#endif
    if (data_dir.empty() && vision_env != nullptr) {
        data_dir = affdex::path(vision_env);
        std::cout << "Using value " << std::string(data_dir.begin(), data_dir.end()) << " from env var "
            << DATA_DIR_ENV_VAR << std::endl;
    }

    if (data_dir.empty() ) {
        std::cerr << "Data directory not specified via command line or env var: " << DATA_DIR_ENV_VAR << std::endl;
        std::cerr << description << std::endl;
        return 1;
    }

    if (!boost::filesystem::exists(data_dir)) {
        std::cerr << "Data directory doesn't exist: " << std::string(data_dir.begin(), data_dir.end()) << std::endl;
        std::cerr << description << std::endl;
        return 1;
    }

    if (draw_id && !draw_display) {
        std::cerr << "Can't draw face id while drawing to screen is disabled" << std::endl;
        std::cerr << description << std::endl;
        return 1;
    }

    unique_ptr<vision::SyncFrameDetector> detector;
    try {
        //initialize the output file
        boost::filesystem::path csv_path(video_path);
        csv_path.replace_extension(".csv");
        std::ofstream csv_file_stream(csv_path.c_str());

        if (!csv_file_stream.is_open()) {
            std::cerr << "Unable to open csv file " << csv_path << std::endl;
            return 1;
        }

        // create the FrameDetector
        detector = std::unique_ptr<vision::SyncFrameDetector>(new vision::SyncFrameDetector(data_dir, num_faces));

        // configure the FrameDetector by enabling features
        detector->enable(vision::ALL_FEATURES);

        // prepare listeners
        PlottingImageListener image_listener(csv_file_stream, draw_display, !disable_logging, draw_id);
        StatusListener status_listener;

        // configure the FrameDetector by assigning listeners
        detector->setImageListener(&image_listener);
        detector->setProcessStatusListener(&status_listener);

        // start the detector
        detector->start();

        do {
            // the VideoReader will handle decoding frames from the input video file
            VideoReader video_reader(video_path, sampling_frame_rate);

            cv::Mat mat;
            timestamp timestamp_ms;
            while (video_reader.GetFrame(mat, timestamp_ms)) {
                // create a Frame from the video input and process it with the FrameDetector
                vision::Frame f(mat.size().width, mat.size().height, mat.data, vision::Frame::ColorFormat::BGR, timestamp_ms);
                detector->process(f);
                image_listener.processResults();
            }

            cout << "******************************************************************" << endl
            << "Processed Frame count: " << image_listener.getProcessedFrames() << endl
            << "Frames w/faces: " << image_listener.getFramesWithFaces() << endl
            << "Percent of frames w/faces: " << image_listener.getFramesWithFacesPercent() << "%" << endl
            << "******************************************************************" << endl;

            detector->reset();
            image_listener.reset();

        } while (loop);

        detector->stop();
        csv_file_stream.close();

        std::cout << "Output written to file: " << csv_path << std::endl;
    }
    catch (std::exception& ex) {
        std::cerr << ex.what();

        // if video_reader couldn't load the video/image, it will throw. Since the detector was started before initializing the video_reader, We need to call `detector->stop()` to avoid crashing
        detector->stop();
        return 1;
    }

    return 0;
}
