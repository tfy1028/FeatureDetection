/*
 * WvmSvmModel.h
 *
 *  Created on: 25.07.2012
 *      Author: poschmann
 */

#ifndef WVMSVMMODEL_H_
#define WVMSVMMODEL_H_

#include "condensation/MeasurementModel.h"
#include <memory>
#include <string>

using std::shared_ptr;

namespace imageprocessing {
class FeatureExtractor;
}
using imageprocessing::FeatureExtractor;

namespace classification {
class ProbabilisticWvmClassifier;
class ProbabilisticSvmClassifier;
}
using classification::ProbabilisticWvmClassifier;
using classification::ProbabilisticSvmClassifier;

namespace condensation {

/**
 * Measurement model that uses a WVM for quick elimination and evaluates the samples that remain after an
 * overlap elimination with a SVM. The weight of the samples will be the product of the certainties from
 * the two detectors, they will be regarded as being independent (although they are not). The certainties
 * for the SVM of samples that are not evaluated by it will be chosen to be 0.5 (unknown).
 */
class WvmSvmModel : public MeasurementModel {
public:

	/**
	 * Constructs a new WVM-SVM measurement model.
	 *
	 * @param[in] featureExtractor The feature extractor.
	 * @param[in] wvm The fast WVM.
	 * @param[in] svm The slower SVM.
	 * TODO overlap elimination?
	 */
	WvmSvmModel(shared_ptr<FeatureExtractor> featureExtractor,
			shared_ptr<ProbabilisticWvmClassifier> wvm, shared_ptr<ProbabilisticSvmClassifier> svm);

	~WvmSvmModel();

	void evaluate(shared_ptr<VersionedImage> image, vector<Sample>& samples);

private:

	shared_ptr<FeatureExtractor> featureExtractor; ///< The feature extractor.
	shared_ptr<ProbabilisticWvmClassifier> wvm;    ///< The fast WVM.
	shared_ptr<ProbabilisticSvmClassifier> svm;    ///< The slower SVM.
	//shared_ptr<OverlapElimination> oe;      ///< The overlap elimination algorithm. TODO
};

} /* namespace condensation */
#endif /* WVMSVMMODEL_H_ */
