#pragma once

#include "SubCommandParser.hpp"
#include "FaceRegistrar.h"
#include <Core.h>

class Application {
  public:
    Application(int argsc, char **argsv);

    void run();

  private:
    void registerFace(const int identifier, affdex::path filename, const int frame_span) const;
    void list() const;
    void unregister(const int identifier) const;
    void unregisterAll() const;

    void setCommonOptions(const std::string &usage,
                          const SubCommandParser &command_line);
    bool parse(const int required_param_count);
    std::string registeredIds() const;

    SubCommandParser command_line_;
    affdex::path data_dir_;
    std::unique_ptr<boost::program_options::options_description> options_;
    std::unique_ptr<affdex::vision::FaceRegistrar> face_registrar_;
};
