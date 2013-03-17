/*
 * mergePlaygroundApp.cpp
 *
 *  Created on: 17.02.2013
 *      Author: Patrik Huber
 */

// For memory leak debugging: http://msdn.microsoft.com/en-us/library/x98tx3cf(v=VS.100).aspx
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>

#ifdef WIN32
	#include <crtdbg.h>
#endif

#ifdef _DEBUG
   #ifndef DBG_NEW
      #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
      #define new DBG_NEW
   #endif
#endif  // _DEBUG

#include "classification/RbfKernel.hpp"
#include "classification/SvmClassifier.hpp"
#include "classification/WvmClassifier.hpp"
#include "classification/ProbabilisticWvmClassifier.hpp"
#include "classification/ProbabilisticSvmClassifier.hpp"

#include "imageprocessing/ImagePyramid.hpp"
#include "imageprocessing/ImagePyramidLayer.hpp"
#include "detection/SlidingWindowDetector.hpp"
#include "imageprocessing/GrayscaleFilter.hpp"
#include "imageprocessing/Patch.hpp"

#include "imageprocessing/FilteringFeatureTransformer.hpp"
#include "imageprocessing/IdentityFeatureTransformer.hpp"
#include "imageprocessing/RowFeatureTransformer.hpp"
#include "imageprocessing/MultipleImageFilter.hpp"
#include "imageprocessing/HistEq64Filter.hpp"

#include "logging/LoggerFactory.hpp"
#include "logging/FileAppender.hpp"

#include "configparser/ConfigParser.hpp"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "mat.h"
#include <iostream>
#include <sstream>

#ifdef WIN32
	#define BOOST_ALL_DYN_LINK	// Link against the dynamic boost lib. Seems to be necessary because we use /MD, i.e. link to the dynamic CRT.
	#define BOOST_ALL_NO_LIB	// Don't use the automatic library linking by boost with VS2010 (#pragma ...). Instead, we specify everything in cmake.
#endif							// We could also use add_definitions( -DBOOST_ALL_NO_LIB ) (+the other one) in the cmake file instead of here.

using namespace std;
using namespace imageprocessing;
using namespace detection;
using namespace classification;
using logging::Logger;
using logging::LoggerFactory;
using logging::loglevel;

int main(int argc, char *argv[])
{
	#ifdef WIN32
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF ); // dump leaks at return
	//_CrtSetBreakAlloc(22978);
	#endif
	
	Logger root = Loggers->getLogger("root");
	root.addAppender(make_shared<logging::FileAppender>(loglevel::INFO, "C:\\Users\\Patrik\\Documents\\GitHub\\logfile.txt"));
	root.addAppender(make_shared<logging::ConsoleAppender>(loglevel::WARN));

	Loggers->getLogger("classification").addAppender(make_shared<logging::FileAppender>(loglevel::DEBUG, "C:\\Users\\Patrik\\Documents\\GitHub\\logfile.txt"));
	Loggers->getLogger("classification").addAppender(make_shared<logging::ConsoleAppender>(loglevel::TRACE));

	// Todo bool isLogLevelEnabled(...)

	root.log(loglevel::PANIC, "Test: Logging PANIC");
	root.log(loglevel::ERROR, "Test: Logging ERROR");
	root.log(loglevel::WARN, "Test: Logging WARN");
	root.log(loglevel::INFO, "Test: Logging INFO");
	root.log(loglevel::DEBUG, "Test: Logging DEBUG");
	root.log(loglevel::TRACE, "Test: Logging TRACE");

	Loggers->getLogger("classification").log(loglevel::DEBUG, "CLASSIF TEST");

	cout << "Starting tests..." << endl;

	
	Mat fvp = cv::imread("C:/Users/Patrik/Documents/GitHub/patchpos.png");
	Mat fvn = cv::imread("C:/Users/Patrik/Documents/GitHub/patchneg.png");

	cv::cvtColor(fvp, fvp, CV_BGR2GRAY);
	cv::cvtColor(fvn, fvn, CV_BGR2GRAY);
	
	shared_ptr<SvmClassifier> svm;
	try	{
		svm = SvmClassifier::loadMatlab("C:/Users/Patrik/Documents/GitHub/config/WRVM/fd_web/fnf-hq64-wvm_big-outnew02-hq64SVM/fd_hq64-fnf_wvm_r0.04_c1_o8x8_n14l20t10_hcthr0.72-0.27,0.36-0.14--With-outnew02-HQ64SVM.mat", "C:/Users/Patrik/Documents/GitHub/config/fdetection/WRVM/fd_web/fnf-hq64-wvm_big-outnew02-hq64SVM/fd_hq64-fnf_wvm_r0.04_c1_o8x8_n14l20t10_hcthr0.72-0.27,0.36-0.14--ts107742-hq64_thres_0.005--with-outnew02HQ64SVM.mat");
	} catch (std::invalid_argument& e) {
		root.log(loglevel::ERROR, e.what());
	}

	shared_ptr<WvmClassifier> wvm = WvmClassifier::loadMatlab("C:/Users/Patrik/Documents/GitHub/config/WRVM/fd_web/fnf-hq64-wvm_big-outnew02-hq64SVM/fd_hq64-fnf_wvm_r0.04_c1_o8x8_n14l20t10_hcthr0.72-0.27,0.36-0.14--With-outnew02-HQ64SVM.mat", "C:/Users/Patrik/Documents/GitHub/config/WRVM/fd_web/fnf-hq64-wvm_big-outnew02-hq64SVM/fd_hq64-fnf_wvm_r0.04_c1_o8x8_n14l20t10_hcthr0.72-0.27,0.36-0.14--ts107742-hq64_thres_0.005--with-outnew02HQ64SVM.mat");

	bool res = svm->classify(fvp);
	bool res2 = svm->classify(fvn);

	res = wvm->classify(fvp);
	res2 = wvm->classify(fvn);

	cout << "The end." << endl;

	/*
	Mat img = cv::imread("C:/Users/Patrik/Documents/GitHub/data/firstrun/ws_115.png");
	cv::namedWindow("src", CV_WINDOW_AUTOSIZE); cv::imshow("src", img);

	//configparser::ConfigParser cp = configparser::ConfigParser();
	//cp.parse("D:\\FeatureDetection\\config\\facedet.txt");
	
	shared_ptr<ImagePyramid> pyr = make_shared<ImagePyramid>(0.09, 0.25, 0.9);	// (0.09, 0.25, 0.9) is the same as old 90, 9, 0.9
	shared_ptr<ImageFilter> imgFil = make_shared<GrayscaleFilter>();
	pyr->addImageFilter(imgFil);
	pyr->update(img);

	shared_ptr<ProbabilisticWvmClassifier> pwvm = ProbabilisticWvmClassifier::loadMatlab("C:/Users/Patrik/Documents/GitHub/config/WRVM/fd_web/fnf-hq64-wvm_big-outnew02-hq64SVM/fd_hq64-fnf_wvm_r0.04_c1_o8x8_n14l20t10_hcthr0.72-0.27,0.36-0.14--With-outnew02-HQ64SVM.mat", "C:/Users/Patrik/Documents/GitHub/config/WRVM/fd_web/fnf-hq64-wvm_big-outnew02-hq64SVM/fd_hq64-fnf_wvm_r0.04_c1_o8x8_n14l20t10_hcthr0.72-0.27,0.36-0.14--ts107742-hq64_thres_0.001--with-outnew02HQ64SVM.mat");
	
	shared_ptr<IdentityFeatureTransformer> idTransform = make_shared<IdentityFeatureTransformer>();
	shared_ptr<MultipleImageFilter> patchFilter = make_shared<MultipleImageFilter>();
	shared_ptr<HistEq64Filter> histEq64Filter = make_shared<HistEq64Filter>();
	patchFilter->add(histEq64Filter);
	shared_ptr<FilteringFeatureTransformer> fft = make_shared<FilteringFeatureTransformer>(idTransform, patchFilter);
	
	shared_ptr<SlidingWindowDetector> det = make_shared<SlidingWindowDetector>(pwvm, fft);
	vector<pair<shared_ptr<Patch>, pair<bool, double>>> resultingPatches = det->detect(pyr);

	Mat rgbimg = img.clone();
	std::vector<pair<shared_ptr<Patch>, pair<bool, double>>>::iterator pit = resultingPatches.begin();
	for(; pit != resultingPatches.end(); pit++) {
		cv::rectangle(rgbimg, cv::Point(pit->first->getX()-pit->first->getWidth()/2, pit->first->getY()-pit->first->getHeight()/2), cv::Point(pit->first->getX()+pit->first->getWidth()/2, pit->first->getY()+pit->first->getHeight()/2), cv::Scalar(0, 0, (float)255 * ((pit->second.second)/1.0)   ));
	}
	cv::namedWindow("final", CV_WINDOW_AUTOSIZE); cv::imshow("final", rgbimg);
	cv::imwrite("wvm_newest2.png", rgbimg);

	shared_ptr<ProbabilisticSvmClassifier> psvm = ProbabilisticSvmClassifier::loadMatlab("C:/Users/Patrik/Documents/GitHub/config/WRVM/fd_web/fnf-hq64-wvm_big-outnew02-hq64SVM/fd_hq64-fnf_wvm_r0.04_c1_o8x8_n14l20t10_hcthr0.72-0.27,0.36-0.14--With-outnew02-HQ64SVM.mat", "C:/Users/Patrik/Documents/GitHub/config/WRVM/fd_web/fnf-hq64-wvm_big-outnew02-hq64SVM/fd_hq64-fnf_wvm_r0.04_c1_o8x8_n14l20t10_hcthr0.72-0.27,0.36-0.14--ts107742-hq64_thres_0.001--with-outnew02HQ64SVM.mat");

	vector<pair<shared_ptr<Patch>, pair<bool, double>>> resultingPatchesAfterSVM;
	Mat svmimg = img.clone();
	Mat svmimg2 = img.clone();
	pit = resultingPatches.begin();
	psvm->getSvm()->setLimitReliability(-1.2f);
	for(; pit != resultingPatches.end(); pit++) {
		pair<bool, double> res = psvm->classify(pit->first->getData());
		cv::rectangle(svmimg2, cv::Point(pit->first->getX()-pit->first->getWidth()/2, pit->first->getY()-pit->first->getHeight()/2), cv::Point(pit->first->getX()+pit->first->getWidth()/2, pit->first->getY()+pit->first->getHeight()/2), cv::Scalar(0, 0, (float)255 * ((res.second)/1.0)   ));
		if(res.first)
			cv::rectangle(svmimg, cv::Point(pit->first->getX()-pit->first->getWidth()/2, pit->first->getY()-pit->first->getHeight()/2), cv::Point(pit->first->getX()+pit->first->getWidth()/2, pit->first->getY()+pit->first->getHeight()/2), cv::Scalar(0, 0, (float)255 * ((res.second)/1.0)   ));
	}
	cv::namedWindow("svm", CV_WINDOW_AUTOSIZE); cv::imshow("svm", svmimg);
	imwrite("svm_after2_m1.2.png", svmimg);
	cv::namedWindow("svmall", CV_WINDOW_AUTOSIZE); cv::imshow("svmall", svmimg2);
	imwrite("svmall_after2_m1.2.png", svmimg2);

	cv::waitKey(0);
	*/
	return 0;
}
