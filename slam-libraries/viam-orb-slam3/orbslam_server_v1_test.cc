#define BOOST_TEST_MODULE orb_grpc_server tests
#include "orbslam_server_v1.h"

#include <boost/filesystem.hpp>
#include <boost/test/included/unit_test.hpp>
#include <exception>
namespace fs = boost::filesystem;

namespace viam {
namespace {

void checkParseAndValidateArgumentsException(const vector<string>& args,
                                             const string& message) {
    SLAMServiceImpl slamService;
    BOOST_CHECK_EXCEPTION(utils::ParseAndValidateArguments(args, slamService),
                          runtime_error, [&message](const runtime_error& ex) {
                              BOOST_CHECK_EQUAL(ex.what(), message);
                              return true;
                          });
}

BOOST_AUTO_TEST_CASE(ParseAndValidateArguments_no_args) {
    const vector<string> args;
    const string message =
        "No args found. Expected: \n"
        "./bin/orb_grpc_server "
        "-data_dir=path_to_data "
        "-config_param={mode=slam_mode,} "
        "-port=grpc_port "
        "-sensors=sensor_name "
        "-data_rate_ms=frame_delay "
        "-map_rate_sec=map_rate_sec";
    checkParseAndValidateArgumentsException(args, message);
}

BOOST_AUTO_TEST_CASE(ParseAndValidateArguments_no_data_dir) {
    const vector<string> args{
        "-config_param={mode=rgbd}", "-port=20000",      "-sensors=color",
        "-data_rate_ms=200",         "-map_rate_sec=60", "-unknown=unknown"};
    const string message = "No data directory given";
    checkParseAndValidateArgumentsException(args, message);
}

BOOST_AUTO_TEST_CASE(ParseAndValidateArguments_no_slam_mode) {
    const vector<string> args{"-data_dir=/path/to", "-config_param={}",
                              "-port=20000",        "-sensors=color",
                              "-data_rate_ms=200",  "-map_rate_sec=60"};
    const string message = "No SLAM mode given";
    checkParseAndValidateArgumentsException(args, message);
}

BOOST_AUTO_TEST_CASE(ParseAndValidateArguments_invalid_slam_mode) {
    const vector<string> args{"-data_dir=/path/to", "-config_param={mode=bad}",
                              "-port=20000",        "-sensors=color",
                              "-data_rate_ms=200",  "-map_rate_sec=60"};
    const string message = "Invalid slam_mode=bad";
    checkParseAndValidateArgumentsException(args, message);
}

BOOST_AUTO_TEST_CASE(ParseAndValidateArguments_no_slam_port) {
    const vector<string> args{"-data_dir=/path/to", "-config_param={mode=rgbd}",
                              "-sensors=color",     "-data_rate_ms=200",
                              "-map_rate_sec=60",   "-unknown=unknown"};
    const string message = "No gRPC port given";
    checkParseAndValidateArgumentsException(args, message);
}

BOOST_AUTO_TEST_CASE(ParseAndValidateArguments_no_camera_data_rate) {
    const vector<string> args{"-data_dir=/path/to", "-config_param={mode=rgbd}",
                              "-port=20000",        "-sensors=color",
                              "-map_rate_sec=60",   "-unknown=unknown"};
    const string message = "No camera data rate specified";
    checkParseAndValidateArgumentsException(args, message);
}

BOOST_AUTO_TEST_CASE(ParseAndValidateArguments_valid_config) {
    const vector<string> args{"-data_dir=/path/to", "-config_param={mode=rgbd}",
                              "-port=20000",        "-sensors=color",
                              "-data_rate_ms=200",  "-map_rate_sec=60"};
    SLAMServiceImpl slamService;
    utils::ParseAndValidateArguments(args, slamService);
    BOOST_TEST(slamService.path_to_vocab == "/path/to/config/ORBvoc.txt");
    BOOST_TEST(slamService.path_to_settings == "/path/to/config");
    BOOST_TEST(slamService.path_to_data == "/path/to/data");
    BOOST_TEST(slamService.path_to_map == "/path/to/map");
    BOOST_TEST(slamService.slam_mode == "rgbd");
    BOOST_TEST(slamService.slam_port == "20000");
    BOOST_TEST(slamService.frame_delay_msec.count() ==
               chrono::milliseconds(200).count());
    BOOST_TEST(slamService.map_rate_sec.count() == chrono::seconds(60).count());
    BOOST_TEST(slamService.camera_name == "color");
    BOOST_TEST(slamService.offlineFlag == false);
}

BOOST_AUTO_TEST_CASE(
    ParseAndValidateArguments_valid_config_capitalized_slam_mode) {
    const vector<string> args{"-data_dir=/path/to", "-config_param={mode=RGBD}",
                              "-port=20000",        "-sensors=color",
                              "-data_rate_ms=200",  "-map_rate_sec=60"};
    SLAMServiceImpl slamService;
    utils::ParseAndValidateArguments(args, slamService);
    BOOST_TEST(slamService.slam_mode == "rgbd");
}

BOOST_AUTO_TEST_CASE(ParseAndValidateArguments_valid_config_no_map_rate_sec) {
    const vector<string> args{"-data_dir=/path/to", "-config_param={mode=rgbd}",
                              "-port=20000",        "-sensors=color",
                              "-data_rate_ms=200",  "-map_rate_sec="};
    SLAMServiceImpl slamService;
    utils::ParseAndValidateArguments(args, slamService);
    BOOST_TEST(slamService.map_rate_sec.count() == chrono::seconds(0).count());
}

BOOST_AUTO_TEST_CASE(ParseAndValidateArguments_valid_config_no_camera) {
    const vector<string> args{
        "-data_dir=/path/to", "-config_param={mode=rgbd}", "-port=20000",
        "-sensors=",          "-data_rate_ms=200",         "-map_rate_sec=60"};
    SLAMServiceImpl slamService;
    utils::ParseAndValidateArguments(args, slamService);
    BOOST_TEST(slamService.camera_name == "");
    BOOST_TEST(slamService.offlineFlag == true);
}

BOOST_AUTO_TEST_CASE(ReadTimeFromFilename) {
    const string filename1 = "2022-01-01T01_00_00.0000";
    const string filename2 = "2022-01-01T01_00_00.0001";
    const string filename3 = "2022-01-01T01_00_01.0000";
    const auto time1 = utils::ReadTimeFromFilename(filename1);
    const auto time2 = utils::ReadTimeFromFilename(filename2);
    const auto time3 = utils::ReadTimeFromFilename(filename3);
    BOOST_TEST(time1 < time2);
    BOOST_TEST(time2 < time3);
}

BOOST_AUTO_TEST_CASE(FindFrameIndex_Closest_no_files) {
    const string configTimeString = "2022-01-01T01_00_00.0000";
    const auto configTime = utils::ReadTimeFromFilename(configTimeString);
    vector<string> files;
    double timeInterest;
    BOOST_TEST(utils::FindFrameIndex(files, "mono", "",
                                     utils::FileParserMethod::Closest,
                                     configTime, &timeInterest) == -1);
}

BOOST_AUTO_TEST_CASE(FindFrameIndex_Closest_ignore_last) {
    const string configTimeString = "2022-01-01T01_00_00.0001";
    const auto configTime = utils::ReadTimeFromFilename(configTimeString);
    vector<string> files{"color_data_2022-01-01T01_00_00.0000",
                         "color_data_2022-01-01T01_00_00.0001",
                         "color_data_2022-01-01T01_00_00.0002"};
    double timeInterest;
    BOOST_TEST(utils::FindFrameIndex(files, "mono", "",
                                     utils::FileParserMethod::Closest,
                                     configTime, &timeInterest) == -1);
}

BOOST_AUTO_TEST_CASE(FindFrameIndex_Closest_found_time) {
    const string configTimeString = "2022-01-01T01_00_00.0000";
    const auto configTime = utils::ReadTimeFromFilename(configTimeString);
    vector<string> files{"color_data_2022-01-01T01_00_00.0000",
                         "color_data_2022-01-01T01_00_00.0001",
                         "color_data_2022-01-01T01_00_00.0002",
                         "color_data_2022-01-01T01_00_00.0003"};
    double timeInterest;
    BOOST_TEST(utils::FindFrameIndex(files, "mono", "",
                                     utils::FileParserMethod::Closest,
                                     configTime, &timeInterest) == 1);
    BOOST_TEST(timeInterest ==
               utils::ReadTimeFromFilename("2022-01-01T01_00_00.0001"));
}

BOOST_AUTO_TEST_CASE(FindFrameIndex_Recent_no_files) {
    const string configTimeString = "2022-01-01T01_00_00.0000";
    const auto configTime = utils::ReadTimeFromFilename(configTimeString);
    vector<string> files;
    double timeInterest;
    BOOST_TEST(utils::FindFrameIndex(files, "mono", "",
                                     utils::FileParserMethod::Recent,
                                     configTime, &timeInterest) == -1);
}

BOOST_AUTO_TEST_CASE(FindFrameIndex_Recent_ignore_last_mono) {
    const string configTimeString = "2022-01-01T01_00_00.0000";
    const auto configTime = utils::ReadTimeFromFilename(configTimeString);
    vector<string> files{"color_data_2022-01-01T01_00_00.0000",
                         "color_data_2022-01-01T01_00_00.0001",
                         "color_data_2022-01-01T01_00_00.0002"};
    double timeInterest;
    BOOST_TEST(utils::FindFrameIndex(files, "mono", "",
                                     utils::FileParserMethod::Recent,
                                     configTime, &timeInterest) == 1);
    BOOST_TEST(timeInterest ==
               utils::ReadTimeFromFilename("2022-01-01T01_00_00.0001"));
}

BOOST_AUTO_TEST_CASE(FindFrameIndex_Recent_ignore_last_mono_fail) {
    const string configTimeString = "2022-01-01T01_00_00.0002";
    const auto configTime = utils::ReadTimeFromFilename(configTimeString);
    vector<string> files{"color_data_2022-01-01T01_00_00.0000",
                         "color_data_2022-01-01T01_00_00.0001",
                         "color_data_2022-01-01T01_00_00.0002"};
    double timeInterest;
    BOOST_TEST(utils::FindFrameIndex(files, "mono", "",
                                     utils::FileParserMethod::Recent,
                                     configTime, &timeInterest) == -1);
}

BOOST_AUTO_TEST_CASE(FindFrameIndex_Recent_ignore_last_rgbd_fail) {
    const string configTimeString = "2022-01-01T01_00_00.0002";
    const auto configTime = utils::ReadTimeFromFilename(configTimeString);
    vector<string> files{"color_data_2022-01-01T01_00_00.0000",
                         "color_data_2022-01-01T01_00_00.0001",
                         "color_data_2022-01-01T01_00_00.0002"};
    double timeInterest;
    BOOST_TEST(utils::FindFrameIndex(files, "rgbd", "",
                                     utils::FileParserMethod::Recent,
                                     configTime, &timeInterest) == -1);
}

BOOST_AUTO_TEST_CASE(FindFrameIndex_Recent_found_mono) {
    const string configTimeString = "2022-01-01T01_00_00.0000";
    const auto configTime = utils::ReadTimeFromFilename(configTimeString);
    vector<string> files{"color_data_2022-01-01T01_00_00.0000",
                         "color_data_2022-01-01T01_00_00.0001",
                         "color_data_2022-01-01T01_00_00.0002",
                         "color_data_2022-01-01T01_00_00.0003",
                         "color_data_2022-01-01T01_00_00.0004"};
    double timeInterest;
    BOOST_TEST(utils::FindFrameIndex(files, "mono", "",
                                     utils::FileParserMethod::Recent,
                                     configTime, &timeInterest) == 3);
    BOOST_TEST(timeInterest ==
               utils::ReadTimeFromFilename("2022-01-01T01_00_00.0003"));
}

BOOST_AUTO_TEST_CASE(FindFrameIndex_Recent_found_time_rgbd) {
    const string configTimeString = "2022-01-01T01_00_00.0000";
    const auto configTime = utils::ReadTimeFromFilename(configTimeString);
    vector<string> files{"color_data_2022-01-01T01_00_00.0000",
                         "color_data_2022-01-01T01_00_00.0001",
                         "color_data_2022-01-01T01_00_00.0002",
                         "color_data_2022-01-01T01_00_00.0003"};
    double timeInterest;
    // Create a unique path in the temp directory
    fs::path tmpdir = fs::temp_directory_path() / fs::unique_path();
    bool ok = fs::create_directory(tmpdir);
    if (!ok) {
        throw std::runtime_error("could not create directory: " +
                                 tmpdir.string());
    }
    // Create the "depth" subdirectory
    fs::path tmpdirDepth = tmpdir / "depth";
    ok = fs::create_directory(tmpdirDepth);
    if (!ok) {
        fs::remove_all(tmpdir);
        throw std::runtime_error("could not create directory: " +
                                 tmpdirDepth.string());
    }

    // Create the file in the temporary directory
    fs::ofstream ofs(tmpdirDepth / "color_data_2022-01-01T01_00_00.0001.png");
    ofs.close();
    BOOST_TEST(utils::FindFrameIndex(files, "rgbd", tmpdir.string(),
                                     utils::FileParserMethod::Recent,
                                     configTime, &timeInterest) == 1);
    BOOST_TEST(timeInterest ==
               utils::ReadTimeFromFilename("2022-01-01T01_00_00.0001"));
    // Close the file and remove the temporary directory and its contents.
    fs::remove_all(tmpdir);
}

}  // namespace
}  // namespace viam
