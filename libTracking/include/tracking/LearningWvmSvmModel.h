/*
 * LearningWvmSvmModel.h
 *
 *  Created on: 20.09.2012
 *      Author: poschmann
 */

#ifndef LEARNINGWVMSVMMODEL_H_
#define LEARNINGWVMSVMMODEL_H_

#include "tracking/PatchDuplicateFilter.h"
#include "tracking/LearningMeasurementModel.h"
#include "boost/shared_ptr.hpp"
#include <string>

class VDetectorVectorMachine;
class OverlapElimination;

using boost::shared_ptr;

namespace tracking {

class SvmTraining;
class ChangableDetectorSvm;

/**
 * Measurement model that trains a dynamic SVM. Additionally, it uses a WVM for quick * elimination and
 * evaluates the samples that remain after that. When using the static SVM, an overlap elimination further
 * reduces the amount of samples. A dynamic SVM will be trained from the samples given to the update method.
 * Depending on the quality of the training, the dynamic or static SVM will be used. The weight of the
 * samples will be the product of the certainties from the WVM and SVM, they will be regarded as being
 * independent (although they are not). The certainties for the SVM of samples that are not evaluated by it
 * will be chosen to be 0.5 (unknown).
 */
class LearningWvmSvmModel : public LearningMeasurementModel, public PatchDuplicateFilter {
public:

	/**
	 * Constructs a new learning WVM SVM measurement model. The machines and algorithm
	 * must have been initialized.
	 *
	 * @param[in] wvm The fast WVM.
	 * @param[in] staticSvm The slower static SVM.
	 * @param[in] dynamicSvm The dynamic SVM that will be re-trained.
	 * @param[in] oe The overlap elimination algorithm.
	 * @param[in] svmTraining The SVM training algorithm.
	 */
	explicit LearningWvmSvmModel(shared_ptr<VDetectorVectorMachine> wvm,
			shared_ptr<VDetectorVectorMachine> staticSvm, shared_ptr<ChangableDetectorSvm> dynamicSvm,
			shared_ptr<OverlapElimination> oe, shared_ptr<SvmTraining> svmTraining);

	/**
	 * Constructs a new learning WVM SVM measurement model with default SVMs and overlap
	 * elimination algorithm.
	 *
	 * @param[in] configFilename The name of the Matlab config file containing the parameters.
	 * @param[in] negativesFilename The name of the file containing the static negative samples.
	 */
	explicit LearningWvmSvmModel(std::string configFilename, std::string negativesFilename);

	~LearningWvmSvmModel();

	void evaluate(FdImage* image, std::vector<Sample>& samples);

	void reset();

	void update();

	void update(FdImage* image, std::vector<Sample>& positiveSamples, std::vector<Sample>& negativeSamples);

	inline bool isUsingDynamicModel() {
		return usingDynamicSvm;
	}

private:

	/**
	 * Creates patches of samples.
	 *
	 * @param[in] samples The samples to create patches from.
	 */
	std::vector<FdPatch*> getPatches(FdImage* image, std::vector<Sample>& samples);

	shared_ptr<VDetectorVectorMachine> wvm;       ///< The fast WVM.
	shared_ptr<VDetectorVectorMachine> staticSvm; ///< The slower static SVM.
	shared_ptr<ChangableDetectorSvm> dynamicSvm;  ///< The dynamic SVM that will be re-trained.
	shared_ptr<OverlapElimination> oe;            ///< The overlap elimination algorithm.
	shared_ptr<SvmTraining> svmTraining;          ///< The SVM training algorithm.
	bool usingDynamicSvm; ///< Flag that indicates whether the dynamic SVM is used.
};

} /* namespace tracking */
#endif /* LEARNINGWVMSVMMODEL_H_ */
