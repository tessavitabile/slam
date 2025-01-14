// This is an Experimental variation of cartographer. It has not yet been
// integrated into RDK.
#ifndef VIAM_READ_FROM_FILE_H_
#define VIAM_READ_FROM_FILE_H_

#include <inttypes.h>

#include <chrono>
#include <ostream>
#include <ratio>
#include <string>

#include "cartographer/sensor/timed_point_cloud_data.h"

namespace viam {
namespace io {

class ReadFile {
   public:
    std::vector<std::string> listFilesInDirectory(std::string data_directory);
    cartographer::sensor::TimedPointCloudData timedPointCloudDataFromPCDBuilder(
        std::string file_path, std::string initial_filename);
    int removeFile(std::string);
};

}  // namespace io
}  // namespace viam

#endif  // VIAM_READ_FROM_FILE_H_
