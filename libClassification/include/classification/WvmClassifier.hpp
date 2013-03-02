/*
 * WvmClassifier.hpp
 *
 *  Created on: 21.12.2012
 *      Author: poschmann & huber
 */
#pragma once

#ifndef WVMCLASSIFIER_HPP_
#define WVMCLASSIFIER_HPP_

#include "classification/VectorMachineClassifier.hpp"
#include "opencv2/core/core.hpp"
#include <string>
#include <vector>

using cv::Mat;
using std::string;
using std::vector;

namespace classification {

class IImg;

/**
 * Classifier based on a Wavelet Reduced Vector Machine.
 */
class WvmClassifier : public VectorMachineClassifier {
public:

	/**
	 * Constructs a new WVM classifier.
	 */
	WvmClassifier();

	~WvmClassifier();

	bool classify(const Mat& featureVector) const;

	/**
	 * Determines the classification result given the distance of a feature vector to the decision hyperplane.
	 *
	 * @param[in] levelAndDistance The index of the last used filter and distance of that filter level.
	 * @return True if feature vectors of the given distance would be classified positively, false otherwise.
	 */
	bool classify(pair<int, double> levelAndDistance) const;

	/**
	 * Computes the approximate distance of a feature vector to the decision hyperplane. This is the real distance
	 * without any influence by the offset for configuring the operating point of the SVM.
	 *
	 * @param[in] featureVector The feature vector.
	 * @return A pair with the index of the last used filter and the distance to the decision hyperplane of that filter level.
	 */
	pair<int, double> computeHyperplaneDistance(const Mat& featureVector) const;

	/**
	 * Creates a new WVM classifier from the parameters given in some Matlab file.
	 *
	 * @param[in] classifierFilename TODO
	 * @param[in] thresholdsFilename TODO
	 */
	static shared_ptr<WvmClassifier> loadMatlab(const string& classifierFilename, const string& thresholdsFilename); // TODO: Re-work this. Should also pass a Kernel.

	int getNumUsedFilters(void);
	void setNumUsedFilters(int);			 ///< Change the number of currently used wavelet-vectors
	float getLimitReliabilityFilter(void);
	void setLimitReliabilityFilter(float);	///< Rewrites the hierarchicalThresholds vector with the new thresholds

protected:

	float linEvalWvmHisteq64(int, int, float*, float*, const IImg*, const IImg*) const;

	int filter_size_x;	///< We need this for the integral image. Better solution maybe later...
	int filter_size_y;	///< We need this for the integral image. Better solution maybe later...
	float basisParam;	///< The Rbf-Kernel parameter. Maybe better solution later to encapsulate it, like in the SVM.

	float**  linFilters;      ///< points to the filter array (this is support_hk%d in the .mat-file). These are the actual vectors.
	float**  hkWeights;       ///< weights[i] contains the weights of the kernels of the i hierarchical kernel.


	int numUsedFilters;			///< Read from the matlab config. 0=use all, >0 && <numLinFilters ==> don't use all, only numUsedFilters.
	int numLinFilters;			///< Read from detector .mat. Used in the WVM loop (run over all vectors). (e.g. 280)

	int	numFiltersPerLevel;		///< number of filters per level (e.g. 14)
	int	numLevels;			///< number of levels with filters (e.g. 20)


	float* lin_thresholds;	///< arrays of the thresholds (the SVM's b). All values of this array (e.g. 280) are set to nonlin_threshold read from the detector .mat on load().
	// This is then used inside WvmEvalHistEq64(). The same for all vectors. This could be just a float. Actually it can only be a float because it comes from param_nonlin1 in the .mat.

	vector<float> hierarchicalThresholds;	///< a pixel whose correlation with filter i is > hierarchicalThresholds[i] 
	// is retained for further investigation, if it is lower, it is classified as being not a face 
	vector<float> hierarchicalThresholdsFromFile;	///< This is the same as hierarchicalThresholds. Checked on 17.11.12 - this is really not needed.
	// hierarchicalThresholdsFromFile is only used for reading from the config, then not used anymore.

	float limitReliabilityFilter;	///< This is added to hierarchicalThresholds on startup (read from the config, FD.limitReliabilityFilter), then not used anymore.


	typedef struct _rec {
		int x1,x2,y1,y2,uull,uur,dll,dr;
	} TRec, *PRec;

	class Area
	{
	public:
		Area(void);
		Area(int, int*);
		~Area(void);

		void dump(char*);
		int		cntval;
		double	*val;
		TRec	**rec;
		int		*cntrec;
		int		cntallrec;
	};

	Area** area;	///< rectangles and gray values of the appr. rsv
	double	*app_rsv_convol;	///< convolution of the appr. rsv (pp)

	float *filter_output;		///< temporary output of each filter level
	float *u_kernel_eval;		///< temporary cache, size=numFiltersPerLevel (or numLevels?)

};

} /* namespace classification */
#endif /* WVMCLASSIFIER_HPP_ */