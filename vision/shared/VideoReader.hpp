#pragma once

#include <Core.h>

#include <boost/filesystem.hpp>
#include <opencv2/highgui/highgui.hpp>

class VideoReader {
  public:
    VideoReader(const boost::filesystem::path &file_path, const unsigned int sampling_frame_rate);

    bool GetFrame(cv::Mat &bgr_frame, affdex::timestamp &timestamp_ms);
    bool GetFrameData(cv::Mat &bgr_frame, affdex::timestamp &timestamp_ms);

  private:
    cv::VideoCapture cap;
    affdex::timestamp last_timestamp_ms;
    unsigned int sampling_frame_rate;
};
