#pragma once

#include "FaceRegistrar.h"
#include <Core.h>

class FaceDb {
  public:
    FaceDb(const std::string data_dir);

    void videoRegister(const int identifier,
                       affdex::path filename,
                       const int frame_span);
    void webcamRegister(const int identifier,
                        const int camera_id);

    void list() const;
    void unregister(const int identifier) const;
    void unregisterAll() const;

private:
    std::string registeredIds() const;
    void showScore(const int score) const;
    int processFrame(const cv::Mat &frame, const int identifier);

    affdex::path data_dir_;
    mutable affdex::vision::FaceRegistrar face_registrar_;

    static void interrupt(int sig);
    static bool exitLoop_;
};
