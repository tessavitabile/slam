// This is an Experimental variation of cartographer. It has not yet been
// integrated into RDK.
#include "config.h"

#include <iostream>

#include "glog/logging.h"

namespace viam {
namespace slam_service {
namespace config {

DEFINE_string(data_dir, "",
              "Directory in which sensor data and maps are expected.");
DEFINE_string(config_param, "", "Config parameters for cartographer.");
DEFINE_string(port, "", "GRPC port");
DEFINE_string(sensors, "", "Array of sensors.");
DEFINE_int64(data_rate_ms, 200, "Frequency at which we grab/save data.");
DEFINE_int64(
    map_rate_sec, 60,
    "Frequency at which we want to print map pictures while cartographer "
    "is running.");
DEFINE_string(input_file_pattern, "", "Input file pattern");

// Parses and validates the command line arguments. Sets the log level. Throws
// an exception if the arguments are malformed.
int ParseAndValidateConfigParams(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_data_dir.empty()) {
        LOG(ERROR) << "-data_dir is missing.\n";
        return EXIT_FAILURE;
    } else if (FLAGS_config_param.empty()) {
        LOG(ERROR) << "-config_param is missing.\n";
        return EXIT_FAILURE;
    } else if (FLAGS_port.empty()) {
        LOG(ERROR) << "-port is missing.\n";
        return EXIT_FAILURE;
    } else if (FLAGS_sensors.empty()) {
        LOG(ERROR) << "-sensors is missing.\n";
        // return EXIT_FAILURE;
    }
    LOG(INFO) << "data_dir: " << FLAGS_data_dir << "\n";
    LOG(INFO) << "config_param: " << FLAGS_config_param << "\n";
    LOG(INFO) << "port: " << FLAGS_port << "\n";
    LOG(INFO) << "sensors: " << FLAGS_sensors << "\n";
    LOG(INFO) << "data_rate_ms: " << FLAGS_data_rate_ms << "\n";
    LOG(INFO) << "map_rate_sec: " << FLAGS_map_rate_sec << "\n";

    return 0;
}

// Parse a config parameter map for a specific variable name and return the
// value as a string. Returns empty if the variable is not found within the map.
std::string ConfigParamParser(std::string map, std::string varName) {
    std::string strVal;
    size_t loc = std::string::npos;

    std::stringstream ss(map.substr(map.find("{") + 1, map.find("}") - 1));
    while (ss.good()) {
        std::string substr;
        getline(ss, substr, ',');
        loc = substr.find(varName);
        if (loc != std::string::npos) {
            strVal = substr.substr(loc + varName.size());
            break;
        }
    }

    return strVal;
}

}  // namespace config
}  // namespace slam_service
}  // namespace viam
