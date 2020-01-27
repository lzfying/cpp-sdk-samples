#include "FileUtils.hpp"

#include <Core.h>
#include <boost/filesystem/operations.hpp>
#include <stdexcept>

using namespace std;

affdex::path validatePath(affdex::path path, const string &env_variable) {
    // set path to environment variable value if not supplied
#ifdef _WIN32
    wchar_t *env_value = _wgetenv(env_variable.c_str());
#else
    char *env_value = getenv(env_variable.c_str());
#endif

    if (path.empty() && env_value != nullptr) {
        path = affdex::path(env_value);
    }

    if (path.empty()) {
        throw runtime_error("Data directory not specified via command line or env var: " + env_variable);
    }

    if (!boost::filesystem::exists(path)) {
        throw runtime_error("Path doesn't exist: " + path);
    }

    return path;
}
