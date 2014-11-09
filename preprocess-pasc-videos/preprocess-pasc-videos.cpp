/*
 * preprocess-pasc-videos.cpp
 *
 *  Created on: 08.11.2014
 *      Author: Patrik Huber
 *
 * Example:
 * preprocess-pasc-videos -v -s "C:\Users\Patrik\Documents\GitHub\data\PaSC\Protocol\PaSC_20130611\PaSC\metadata\sigsets\pasc_video_handheld.xml" -d "Z:\datasets\multiview02\PaSC\video" -l "C:\Users\Patrik\Documents\GitHub\data\PaSC\pasc_video_pittpatt_detections.txt" -o out
 *   
 */

// For memory leak debugging: http://msdn.microsoft.com/en-us/library/x98tx3cf(v=VS.100).aspx
//#define _CRTDBG_MAP_ALLOC
#include <cstdlib>

#ifdef WIN32
	#include <SDKDDKVer.h>
#endif

/*	// There's a bug in boost/optional.hpp that prevents us from using the debug-crt with it
	// in debug mode in windows. It works in release mode, but as we need debugging, let's
	// disable the windows-memory debugging for now.
#ifdef WIN32
	#include <crtdbg.h>
#endif

#ifdef _DEBUG
	#ifndef DBG_NEW
		#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
		#define new DBG_NEW
	#endif
#endif  // _DEBUG
*/

#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <iomanip>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#ifdef WIN32
	#define BOOST_ALL_DYN_LINK	// Link against the dynamic boost lib. Seems to be necessary because we use /MD, i.e. link to the dynamic CRT.
	#define BOOST_ALL_NO_LIB	// Don't use the automatic library linking by boost with VS2010 (#pragma ...). Instead, we specify everything in cmake.
#endif
#include "boost/program_options.hpp"
#include "boost/algorithm/string.hpp"
#include "boost/filesystem.hpp"
#include "boost/archive/text_iarchive.hpp"

#include "facerecognition/pasc.hpp"
#include "facerecognition/utils.hpp"
#include "facerecognition/frameassessment.hpp"
#include "facerecognition/alignment.hpp"
#include "facerecognition/ThreadPool.hpp"

#include "logging/LoggerFactory.hpp"

using namespace facerecognition;
namespace po = boost::program_options;
namespace fs = boost::filesystem;
using logging::Logger;
using logging::LoggerFactory;
using logging::LogLevel;
using cv::Mat;
using boost::filesystem::path;
using std::string;
using std::cout;
using std::endl;
using std::make_shared;
using std::shared_ptr;
using std::vector;
//using std::pair;
using std::tuple;
using std::future;

// Might rename to "assessQualitySimple(video...)" or assessFrames(...)
// Assesses all or part of the frames in the video (e.g. not those without metadata)
// and returns a score for each.
// Returns: tuple<Frame, Videoname-Framenum, Score>
std::vector<std::tuple<cv::Mat, path, float>> selectFrameSimple(path inputDirectoryVideos, const facerecognition::FaceRecord& video, const vector<facerecognition::PascVideoDetection>& pascVideoDetections)
{
	auto logger = Loggers->getLogger("frameselect-simple");

	auto frames = facerecognition::utils::getFrames(inputDirectoryVideos / video.dataPath);
	vector<float> headBoxSizes;
	vector<float> ieds;
	vector<float> yaws;
	vector<float> sharpnesses;
	vector<float> laplModif;
	vector<float> laplVari;
	vector<int> frameIds;
	for (int frameNum = 0; frameNum < frames.size(); ++frameNum) {
		string frameName = facerecognition::getPascFrameName(video.dataPath, frameNum + 1);
		logger.debug("Processing frame " + frameName);
		auto landmarks = std::find_if(begin(pascVideoDetections), end(pascVideoDetections), [frameName](const facerecognition::PascVideoDetection& d) { return (d.frame_id == frameName); });
		// If we were to run it on the training-videos, we'd have to test if we got the eyes and facebox?
		// The face box is always given. Only the eyes are missing sometimes.
		// For the test-videos, the whole line is just missing for frames without annotations.
		if (landmarks == std::end(pascVideoDetections)) {
			logger.debug("Frame has no PittPatt detections in the metadata file.");
			continue;
		}
		int tlx = landmarks->fcen_x - landmarks->fwidth / 2.0;
		int tly = landmarks->fcen_y - landmarks->fheight / 2.0;
		int w = landmarks->fwidth;
		int h = landmarks->fheight;
		if (tlx < 0 || tlx + w >= frames[frameNum].cols || tly < 0 || tly + h >= frames[frameNum].rows) {
			// patch has some regions outside the image
			logger.debug("Throwing away patch because it goes outside the image bounds.");
			continue;
		}

		// In the training videos metadata, some eye coordinates are missing. If any coordinate is missing, set the score to 0:
		if (!landmarks->le_x || !landmarks->le_y || !landmarks->re_x || !landmarks->re_y) {
			//ieds.emplace_back(0);
			// Hmm, but then, the normalisation we do later goes wrong. We just skip these frames for now.
			logger.debug("Throwing away patch because some of the eye coordinates are missing.");
			continue;
		}

		cv::Rect roi(tlx, tly, w, h);
		Mat croppedFace = frames[frameNum](roi);

		headBoxSizes.emplace_back((landmarks->fwidth + landmarks->fheight) / 2.0f);
		// If we reach here, we got valid eye coordinates
		ieds.emplace_back(cv::norm(cv::Vec2f(landmarks->le_x.get(), landmarks->le_y.get()), cv::Vec2f(landmarks->re_x.get(), landmarks->re_y.get()), cv::NORM_L2));
		yaws.emplace_back(landmarks->fpose_y);
		sharpnesses.emplace_back(sharpnessScoreCanny(croppedFace));
		frameIds.emplace_back(frameNum);

		laplModif.emplace_back(modifiedLaplacian(croppedFace));
		laplVari.emplace_back(varianceOfLaplacian(croppedFace));
		/*{
			cv::imwrite("out/out_" + std::to_string(frameIds.size() - 1) + "_" + std::to_string(frameNum) + ".png", croppedFace);
			Mat cannyEdges;
			cv::Canny(croppedFace, cannyEdges, 225.0, 175.0); // threshold1, threshold2
			int numEdgePixels = cv::countNonZero(cannyEdges); // throws if 0 nonZero? Check first?
			float sharpness = numEdgePixels * 1000.0f / (cannyEdges.rows * cannyEdges.cols);
			cv::imwrite("out/out_" + std::to_string(frameIds.size() - 1) + "_" + std::to_string(frameNum) + "_" + std::to_string(sharpness) + ".png", cannyEdges);
			}*/
	}

	if (headBoxSizes.size() == 0) {
		// We don't have ANY frame with PaSC landmarks (or we threw it away because it's out of image borders).
		// Around 25-40 videos for handheld / control.
		// Instead of outputting the first, we could return 'n' random frames. But we can't make use of them without PittPatt eyes, so...
		logger.warn("No metadata available for this video, or threw all available frames away.");
		int idOfBestFrame = 0;
		cv::Mat bestFrame = frames[idOfBestFrame];

		path bestFrameName = video.dataPath.stem();
		std::ostringstream ss;
		ss << std::setw(3) << std::setfill('0') << idOfBestFrame + 1;
		bestFrameName.replace_extension(ss.str() + ".png");
		return vector<std::tuple<Mat, path, float>>{ std::make_tuple(bestFrame, bestFrameName, 0.0f) };
	}

	cv::Mat headBoxScores(minMaxFitTransformLinear(headBoxSizes), true); // function returns a temporary, so we need to copy the data
	cv::Mat interEyeDistanceScores(minMaxFitTransformLinear(ieds), true);
	cv::Mat yawPoseScores(getVideoNormalizedYawPoseScores(yaws), true);
	cv::Mat cannySharpnessScores(minMaxFitTransformLinear(sharpnesses), true);
	cv::Mat modifiedLaplacianInFocusScores(minMaxFitTransformLinear(laplModif), true);
	cv::Mat varianceOfLaplacianInFocusScores(minMaxFitTransformLinear(laplVari), true);
	
	// Weights:
	//  0.2 head box size
	//  0.1 IED
	//  0.4 yaw pose
	//  0.1 canny edges blur measurement
	//  0.1 modified laplace
	//  0.1 laplace variance
	cv::Mat scores = 0.15f * headBoxScores + 0.15f * interEyeDistanceScores + 0.6f * yawPoseScores + 0.03f * cannySharpnessScores + 0.04f * modifiedLaplacianInFocusScores + 0.03f * varianceOfLaplacianInFocusScores;

	/*
	double minScore, maxScore;
	int minIdx[2], maxIdx[2];
	cv::minMaxIdx(scores, &minScore, &maxScore, &minIdx[0], &maxIdx[0]); // can use NULL if argument not needed
	int idOfBestFrame = frameIds[maxIdx[0]]; // scores is a single column so it will be M x 1, i.e. we need maxIdx[0]
	cv::Mat bestFrame = frames[idOfBestFrame];
	//int idOfWorstFrame = frameIds[minIdx];
	//cv::Mat worstFrame = frames[idOfWorstFrame];
	*/

	vector<std::tuple<Mat, path, float>> assessedFrames;
	for (int i = 0; i < scores.rows; ++i) {
		path bestFrameName = video.dataPath.stem();
		std::ostringstream ss;
		ss << std::setw(3) << std::setfill('0') << frameIds[i] + 1;
		bestFrameName.replace_extension(ss.str() + ".png");
		assessedFrames.emplace_back(std::make_tuple(frames[frameIds[i]], bestFrameName, scores.at<float>(i)));
	}
	return assessedFrames;
}

int main(int argc, char *argv[])
{
	#ifdef WIN32
	//_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); // dump leaks at return
	//_CrtSetBreakAlloc(287);
	#endif
	
	string verboseLevelConsole;
	path sigsetPath, inputDirectoryVideos, landmarksPath;
	int numFramesPerVideo;
	int numThreads;
	path outputPath;


	try {
		po::options_description desc("Allowed options");
		desc.add_options()
			("help,h",
				"produce help message")
			("verbose,v", po::value<string>(&verboseLevelConsole)->implicit_value("DEBUG")->default_value("INFO","show messages with INFO loglevel or below."),
				"specify the verbosity of the console output: PANIC, ERROR, WARN, INFO, DEBUG or TRACE")
			("sigset,s", po::value<path>(&sigsetPath)->required(),
				"PaSC video sigset")
			("data,d", po::value<path>(&inputDirectoryVideos)->required(),
				"path to the videos")
			("landmarks,l", po::value<path>(&landmarksPath)->required(),
				"PaSC landmarks for the videos in boost::serialization text format")
			("num-frames,n", po::value<int>(&numFramesPerVideo)->default_value(1),
				"number of frames per video (maximum - actual number of assessed frames might be smaller")
			("threads,t", po::value<int>(&numThreads)->default_value(2),
				"number of threads, i.e. number of video that will be processed in parallel")
			("output,o", po::value<path>(&outputPath)->default_value("."),
				"path to an output folder. In addition to the frames, a CSV-file 'frameselect-simple.txt' will be written to this location")
		;

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm); // style(po::command_line_style::unix_style | po::command_line_style::allow_long_disguise)
		if (vm.count("help")) {
			cout << "Usage: preprocess-pasc-videos [options]" << endl;
			cout << desc;
			return EXIT_SUCCESS;
		}
		po::notify(vm);

	}
	catch (po::error& e) {
		cout << "Error while parsing command-line arguments: " << e.what() << endl;
		cout << "Use --help to display a list of options." << endl;
		return EXIT_SUCCESS;
	}

	LogLevel logLevel;
	if(boost::iequals(verboseLevelConsole, "PANIC")) logLevel = LogLevel::Panic;
	else if(boost::iequals(verboseLevelConsole, "ERROR")) logLevel = LogLevel::Error;
	else if(boost::iequals(verboseLevelConsole, "WARN")) logLevel = LogLevel::Warn;
	else if(boost::iequals(verboseLevelConsole, "INFO")) logLevel = LogLevel::Info;
	else if(boost::iequals(verboseLevelConsole, "DEBUG")) logLevel = LogLevel::Debug;
	else if(boost::iequals(verboseLevelConsole, "TRACE")) logLevel = LogLevel::Trace;
	else {
		cout << "Error: Invalid LogLevel." << endl;
		return EXIT_FAILURE;
	}
	
	Loggers->getLogger("imageio").addAppender(make_shared<logging::ConsoleAppender>(logLevel));
	Loggers->getLogger("facerecognition").addAppender(make_shared<logging::ConsoleAppender>(logLevel));
	Loggers->getLogger("app").addAppender(make_shared<logging::ConsoleAppender>(logLevel));
	Logger appLogger = Loggers->getLogger("app");

	appLogger.debug("Verbose level for console output: " + logging::logLevelToString(logLevel));

	// Read the video detections metadata (eyes, face-coords):
	vector<facerecognition::PascVideoDetection> pascVideoDetections;
	{
		std::ifstream ifs(landmarksPath.string());
		boost::archive::text_iarchive ia(ifs);
		ia >> pascVideoDetections;
	} // archive and stream closed when destructors are called

	// Read the training-video xml sigset and the training-still sigset to get the subject-id metadata:
	auto videoSigset = facerecognition::utils::readPascSigset(sigsetPath, true);

	// Create the output directory if it doesn't exist yet:
	if (!fs::exists(outputPath)) {
		fs::create_directory(outputPath);
	}

	// Read all videos:
	if (!fs::exists(inputDirectoryVideos)) {
		appLogger.error("The given input files directory doesn't exist. Aborting.");
		return EXIT_FAILURE;
	}

	// Write all frames and scores to a file:
	std::ofstream framesListFile((outputPath / "frames.txt").string());

	// If we don't want to loop over all videos: (e.g. to get a quick Matlab output)
	//auto videoIter = std::find_if(begin(videoSigset), end(videoSigset), [](const facerecognition::FaceRecord& d) { return (d.dataPath == "05795d567.mp4"); });
	//auto video = *videoIter; {
	for (auto& video : videoSigset) {
		appLogger.info("Starting to process " + video.dataPath.string());

		// Shouldn't be necessary, but there are 5 videos in the xml sigset that we don't have.
		// Does it happen for the test-videos? No?
		if (!fs::exists(inputDirectoryVideos / video.dataPath)) {
			appLogger.warn("Video in the sigset not found on the filesystem!");
			continue;
		}

		auto assessedFrames = selectFrameSimple(inputDirectoryVideos, video, pascVideoDetections);
		// Sort by score, higher ones first:
		using std::get;
		std::sort(begin(assessedFrames), end(assessedFrames), [](tuple<Mat, path, float> lhs, tuple<Mat, path, float> rhs) { return get<2>(rhs) < get<2>(lhs); });
		int minNumFramesToKeep = 15;
		if (assessedFrames.size() > minNumFramesToKeep) {
			// Start from rear, remove all non-frontal frames as long as we still have 15:
			for (int i = assessedFrames.size() - 1; (assessedFrames.size() > minNumFramesToKeep) && (i >= 0); --i) {
				auto framePath = get<1>(assessedFrames[i]);
				string frameName = facerecognition::getPascFrameName(framePath.stem().stem(), boost::lexical_cast<int>(framePath.stem().extension().string().substr(1, string::npos)));
				auto landmarks = std::find_if(begin(pascVideoDetections), end(pascVideoDetections), [frameName](const facerecognition::PascVideoDetection& d) { return (d.frame_id == frameName); });
				if (landmarks->fpose_y > 5.0f) {
					assessedFrames.erase(begin(assessedFrames) + i);
				}
			}
		}
		int cnt = 1;
		for (auto& frame : assessedFrames) {
			auto f = get<0>(frame);
			auto framePath = get<1>(frame); // Hmm we're repeating that find_if a bit often
			cv::imwrite((outputPath / framePath).string(), f);

			// Write to frames.txt:
			path frameName = outputPath / get<1>(frame); // The filename is already 1-based (PaSC format)
			path frameNameCsv = frameName.stem();
			frameNameCsv.replace_extension(".mp4");
			string frameNumCsv = frameName.stem().extension().string();
			frameNumCsv.erase(std::remove(frameNumCsv.begin(), frameNumCsv.end(), '.'), frameNumCsv.end());
			framesListFile << frameNameCsv.string() << "," << frameNumCsv << "," << get<2>(frame) << endl;

			if (cnt > minNumFramesToKeep) {
				break;
			}
			++cnt;
		}
		/*
		for (auto& frame : assessedFrames) {
			auto framePath = get<1>(frame); // Hmm we're repeating that find_if a bit often
			string frameName = facerecognition::getPascFrameName(framePath.stem().stem(), boost::lexical_cast<int>(framePath.stem().extension().string().substr(1, string::npos)));
			auto landmarks = std::find_if(begin(pascVideoDetections), end(pascVideoDetections), [frameName](const facerecognition::PascVideoDetection& d) { return (d.frame_id == frameName); });
			cv::Vec2f re(landmarks->re_x.get(), landmarks->re_y.get());
			cv::Vec2f le(landmarks->le_x.get(), landmarks->le_y.get());
			Mat rotationMatrix = facerecognition::getRotationMatrixFromEyePairs(re, le);
			cv::Mat rotatedFrame;
			cv::warpAffine(get<0>(frame), rotatedFrame, rotationMatrix, rotatedFrame.size(), cv::INTER_LANCZOS4, cv::BORDER_CONSTANT);
			Mat croppedFace = facerecognition::cropAligned(rotatedFrame, re, le, 1.1f, 0.8f); // total w = 2.2, total h = 2.4
			auto f = get<0>(frame);
			auto s = get<2>(frame);
			cv::imwrite((outputPath / framePath).string(), f);
		}
		*/
		//if (frames.size() < numFramesPerVideo) {
		//	numFramesPerVideo = frames.size(); // we got fewer frames than desired
		//}
		//for (int i = 0; i < numFramesPerVideo; ++i)
	}
	framesListFile.clear();
	appLogger.info("Finished processing all videos.");

	return EXIT_SUCCESS;
}