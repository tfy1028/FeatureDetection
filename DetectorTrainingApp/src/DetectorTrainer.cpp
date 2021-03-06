/*
 * DetectorTrainer.cpp
 *
 *  Created on: 21.10.2015
 *      Author: poschmann
 */

#include "Annotations.hpp"
#include "DetectorTrainer.hpp"
#include "classification/LinearKernel.hpp"
#include "classification/SvmClassifier.hpp"
#include "imageprocessing/ImagePyramid.hpp"
#include "imageprocessing/Patch.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

using classification::LinearKernel;
using classification::SvmClassifier;
using cv::Mat;
using cv::Rect;
using cv::Size;
using detection::AggregatedFeaturesDetector;
using detection::NonMaximumSuppression;
using imageio::RectLandmark;
using imageprocessing::ImageFilter;
using imageprocessing::Patch;
using imageprocessing::extraction::AggregatedFeaturesExtractor;
using libsvm::LibSvmClassifier;
using std::back_inserter;
using std::copy_if;
using std::make_shared;
using std::runtime_error;
using std::shared_ptr;
using std::string;
using std::vector;

DetectorTrainer::DetectorTrainer(bool printProgressInformation) :
		printProgressInformation(printProgressInformation),
		aspectRatio(1),
		aspectRatioInv(1),
		noSuppression(make_shared<NonMaximumSuppression>(1.0)),
		generator(std::random_device()()) {}

shared_ptr<AggregatedFeaturesDetector> DetectorTrainer::getDetector(shared_ptr<NonMaximumSuppression> nms) const {
	return getDetector(nms, featureParams.octaveLayerCount);
}

shared_ptr<AggregatedFeaturesDetector> DetectorTrainer::getDetector(shared_ptr<NonMaximumSuppression> nms, int octaveLayerCount) const {
	if (!classifier->isUsable())
		throw runtime_error("DetectorTrainer: must train a classifier first");
	if (!imageFilter)
		return make_shared<AggregatedFeaturesDetector>(filter, featureParams.cellSizeInPixels,
				featureParams.windowSizeInCells, octaveLayerCount, classifier->getSvm(), nms);
	else
		return make_shared<AggregatedFeaturesDetector>(imageFilter, filter, featureParams.cellSizeInPixels,
				featureParams.windowSizeInCells, octaveLayerCount, classifier->getSvm(), nms);
}

void DetectorTrainer::storeClassifier(const string& filename) const {
	std::ofstream stream(filename);
	classifier->getSvm()->store(stream);
	stream.close();
}

Mat DetectorTrainer::getWeightVector() const {
	return classifier->getSvm()->getSupportVectors().front();
}

void DetectorTrainer::setTrainingParameters(TrainingParams params) {
	trainingParams = params;
}

void DetectorTrainer::setFeatures(FeatureParams params, const shared_ptr<ImageFilter>& filter, const shared_ptr<ImageFilter>& imageFilter) {
	featureParams = params;
	aspectRatio = params.windowAspectRatio();
	aspectRatioInv = 1.0 / aspectRatio;
	this->imageFilter = imageFilter;
	this->filter = filter;
	if (!imageFilter)
		featureExtractor = make_shared<AggregatedFeaturesExtractor>(filter,
				params.windowSizeInCells, params.cellSizeInPixels, params.octaveLayerCount);
	else
		featureExtractor = make_shared<AggregatedFeaturesExtractor>(imageFilter, filter,
				params.windowSizeInCells, params.cellSizeInPixels, params.octaveLayerCount);
}

void DetectorTrainer::train(vector<LabeledImage> images) {
	createEmptyClassifier();
	collectInitialTrainingExamples(images);
	trainClassifier();
	collectHardTrainingExamples(images);
	trainClassifier();
}

void DetectorTrainer::createEmptyClassifier() {
	classifier = LibSvmClassifier::createBinarySvm(make_shared<LinearKernel>(), trainingParams.C, trainingParams.compensateImbalance);
}

void DetectorTrainer::collectInitialTrainingExamples(vector<LabeledImage> images) {
	if (printProgressInformation)
		std::cout << "collecting initial training examples" << std::endl;
	collectTrainingExamples(images, true);
}

void DetectorTrainer::collectHardTrainingExamples(vector<LabeledImage> images) {
	if (printProgressInformation)
		std::cout << "collecting additional hard training examples" << std::endl;
	createHardNegativesDetector();
	collectTrainingExamples(images, false);
}

void DetectorTrainer::createHardNegativesDetector() {
	classifier->getSvm()->setThreshold(trainingParams.negativeScoreThreshold);
	hardNegativesDetector = make_shared<AggregatedFeaturesDetector>(featureExtractor, classifier->getSvm(), noSuppression);
	classifier->getSvm()->setThreshold(0);
}

void DetectorTrainer::collectTrainingExamples(vector<LabeledImage> images, bool initial) {
	for (LabeledImage labeledImage : images) {
		vector<RectLandmark> landmarks = adjustAspectRatio(labeledImage.landmarks);
		addTrainingExamples(labeledImage.image, landmarks, initial);
		if (trainingParams.mirrorTrainingData)
			addMirroredTrainingExamples(labeledImage.image, landmarks, initial);
	}
}

vector<RectLandmark> DetectorTrainer::adjustAspectRatio(const vector<RectLandmark>& landmarks) const {
	vector<RectLandmark> adjustedLandmarks;
	adjustedLandmarks.reserve(landmarks.size());
	for (const RectLandmark& landmark : landmarks)
		adjustedLandmarks.push_back(adjustAspectRatio(landmark));
	return adjustedLandmarks;
}

RectLandmark DetectorTrainer::adjustAspectRatio(const RectLandmark& landmark) const {
	const string& name = landmark.getName();
	float x = landmark.getX();
	float y = landmark.getY();
	float width = landmark.getWidth();
	float height = landmark.getHeight();
	if (width < aspectRatio * height)
		width = aspectRatio * height;
	else if (width > aspectRatio * height)
		height = width * aspectRatioInv;
	return RectLandmark(name, x, y, width, height);
}

void DetectorTrainer::addMirroredTrainingExamples(const Mat& image, const vector<RectLandmark>& landmarks, bool initial) {
	Mat mirroredImage = flipHorizontally(image);
	vector<RectLandmark> mirroredLandmarks = flipHorizontally(landmarks, image.cols);
	addTrainingExamples(mirroredImage, mirroredLandmarks, initial);
}

Mat DetectorTrainer::flipHorizontally(const Mat& image) {
	Mat flippedImage;
	cv::flip(image, flippedImage, 1);
	return flippedImage;
}

vector<RectLandmark> DetectorTrainer::flipHorizontally(const vector<RectLandmark>& landmarks, int imageWidth) {
	vector<RectLandmark> flippedLandmarks;
	flippedLandmarks.reserve(landmarks.size());
	for (const RectLandmark& landmark : landmarks)
		flippedLandmarks.push_back(flipHorizontally(landmark, imageWidth));
	return flippedLandmarks;
}

RectLandmark DetectorTrainer::flipHorizontally(const RectLandmark& landmark, int imageWidth) {
	const string& name = landmark.getName();
	float x = landmark.getX();
	float y = landmark.getY();
	float width = landmark.getWidth();
	float height = landmark.getHeight();
	float mirroredX = imageWidth - x - 1;
	return RectLandmark(name, mirroredX, y, width, height);
}

void DetectorTrainer::addTrainingExamples(const Mat& image, const vector<RectLandmark>& landmarks, bool initial) {
	addTrainingExamples(image, Annotations(landmarks), initial);
}

void DetectorTrainer::addTrainingExamples(const Mat& image, const Annotations& annotations, bool initial) {
	setImage(image);
	if (initial) {
		addPositiveExamples(annotations.positives);
		addRandomNegativeExamples(annotations.nonNegatives);
	} else {
		addHardNegativeExamples(annotations.nonNegatives);
	}
}

void DetectorTrainer::setImage(const Mat& image) {
	this->image = image;
	imageSize.width = image.cols;
	imageSize.height = image.rows;
	featureExtractor->update(image);
}

void DetectorTrainer::addPositiveExamples(const vector<Rect>& positiveBoxes) {
	for (const Rect& bounds : positiveBoxes) {
		shared_ptr<Patch> patch = featureExtractor->extract(bounds);
		if (patch)
			positiveTrainingExamples.push_back(patch->getData());
	}
}

void DetectorTrainer::addRandomNegativeExamples(const vector<Rect>& nonNegativeBoxes) {
	int addedCount = 0;
	while (addedCount < trainingParams.randomNegativeCount) {
		if (addNegativeIfNotOverlapping(createRandomBounds(), nonNegativeBoxes))
			++addedCount;
	}
}

Rect DetectorTrainer::createRandomBounds() const {
	typedef std::uniform_int_distribution<int> uniform_int;
	int minWidth = featureParams.windowSizeInPixels().width;
	int maxWidth = std::min(imageSize.width, static_cast<int>(imageSize.height * aspectRatio));
	int width = uniform_int{minWidth, maxWidth}(generator);
	int height = static_cast<int>(std::round(width * aspectRatioInv));
	int x = uniform_int{0, imageSize.width - width}(generator);
	int y = uniform_int{0, imageSize.height - height}(generator);
	return Rect(x, y, width, height);
}

void DetectorTrainer::addHardNegativeExamples(const vector<Rect>& nonNegativeBoxes) {
	vector<Rect> detections = hardNegativesDetector->detect(image);
	for (const Rect& detection : detections)
		addNegativeIfNotOverlapping(detection, nonNegativeBoxes);
}

bool DetectorTrainer::addNegativeIfNotOverlapping(Rect candidate, const vector<Rect>& nonNegativeBoxes) {
	shared_ptr<Patch> patch = featureExtractor->extract(candidate);
	if (!patch || isOverlapping(patch->getBounds(), nonNegativeBoxes))
		return false;
	negativeTrainingExamples.push_back(patch->getData());
	return true;
}

bool DetectorTrainer::isOverlapping(Rect boxToTest, const vector<Rect>& otherBoxes) const {
	for (Rect otherBox : otherBoxes) {
		if (computeOverlap(boxToTest, otherBox) > trainingParams.overlapThreshold) {
			return true;
		}
	}
	return false;
}

double DetectorTrainer::computeOverlap(Rect a, Rect b) const {
	double intersectionArea = (a & b).area();
	double unionArea = a.area() + b.area() - intersectionArea;
	return intersectionArea / unionArea;
}

void DetectorTrainer::trainClassifier() {
	if (printProgressInformation)
		std::cout << "training classifier (adding " << positiveTrainingExamples.size() << " positives and " << negativeTrainingExamples.size() << " negatives)" << std::endl;
	if (!classifier->retrain(positiveTrainingExamples, negativeTrainingExamples))
		throw runtime_error("DetectorTrainer: SVM is not usable after training");
	if (classifier->getSvm()->getSupportVectors().size() != 1) // should never happen because of linear kernel
		throw runtime_error("DetectorTrainer: the amount of support vectors has to be one (w)");
	positiveTrainingExamples.clear();
	negativeTrainingExamples.clear();
}
