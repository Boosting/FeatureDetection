/*
 * SvmClassifier.cpp
 *
 *  Created on: 17.02.2013
 *      Author: Patrik Huber
 */

#include "classification/SvmClassifier.hpp"
#include "classification/RbfKernel.hpp"
#include "mat.h"
#include <iostream>

using std::make_shared;
using std::make_pair;
using std::cout;

namespace classification {

SvmClassifier::SvmClassifier()
{
	numSV = 0;
	support = NULL;
	alpha = NULL;
	limitReliability = 0.0f;
}

SvmClassifier::~SvmClassifier()
{
	if(support != 0) {
		for(int i = 0; i < numSV; ++i)
			delete[] support[i];
	}
	delete[] support;
	delete[] alpha;
}

bool SvmClassifier::classify(const Mat& featureVector) const {
	return classify(computeHyperplaneDistance(featureVector));
}

bool SvmClassifier::classify(double hyperplaneDistance) const {
	return hyperplaneDistance >= limitReliability;
}

double SvmClassifier::computeHyperplaneDistance(const Mat& featureVector) const {
	unsigned int featureVectorLength = featureVector.rows * featureVector.cols;
	unsigned char* data = new unsigned char[featureVectorLength];

	int w = featureVector.cols;
	int h = featureVector.rows;
	for (int i=0; i<featureVector.rows; i++)
	{
		for (int j=0; j<featureVector.cols; j++)
		{
			data[i*w+j] = featureVector.at<uchar>(i, j); // (y, x) !!! i=row, j=column (matrix)
		}
	}

	//for (unsigned int i = 0; i < featureVectorLength; ++i)
		//data[i] = (featureVector.at<unsigned char>(i));
		//data[i] = (unsigned char)(255 * featureVector.at<float>(i) + 0.5f);
	// TODO Check if UChar 1Chan ? Float? 
	
	float res = -this->nonlinThreshold;	// TODO ASK MR Why minus??? He didn't know. Look in his PhD...?
	// TODO add up limitReliability/nonlinThresh, sort that out!
	for (int i = 0; i != this->numSV; ++i) {
		//res += this->alpha[i] * kernel->compute(featureVector.data, this->support[i], featureVectorLength);
		res += this->alpha[i] * kernel->compute(data, this->support[i], featureVectorLength);
	}

	delete[] data;
	data = NULL;
	return res;
}

shared_ptr<SvmClassifier> SvmClassifier::loadMatlab(const string& classifierFilename, const string& thresholdsFilename)
{
	std::cout << "[SvmClassifier] Loading " << classifierFilename << std::endl;	// TODO replace with Logger
	shared_ptr<SvmClassifier> svm = make_shared<SvmClassifier>();

	MATFile *pmatfile;
	mxArray *pmxarray; // =mat
	double *matdata;
	pmatfile = matOpen(classifierFilename.c_str(), "r");
	if (pmatfile == NULL) {
		std::cout << "[SvmClassifier] Error opening file." << std::endl;
		exit(EXIT_FAILURE);	// TODO replace with error throwing!
		return shared_ptr<SvmClassifier>(); // FIXME just a dummy return until exceptions are thrown
	}

	pmxarray = matGetVariable(pmatfile, "param_nonlin1");
	if (pmxarray == 0) {
		std::cout << "[SvmClassifier] Error: There is a no param_nonlin1 in the file." << std::endl;
		exit(EXIT_FAILURE);
		return shared_ptr<SvmClassifier>(); // FIXME just a dummy return until exceptions are thrown
	}
	std::cout << "[SvmClassifier] Reading param_nonlin1" << std::endl;
	matdata = mxGetPr(pmxarray);
	// TODO we don't need all of those
	svm->nonlinThreshold = (float)matdata[0];
	int nonLinType       = (int)matdata[1];
	float basisParam       = (float)(matdata[2]/65025.0); // because the training image's graylevel values were divided by 255
	int polyPower        = (int)matdata[3];
	float divisor          = (float)matdata[4];
	mxDestroyArray(pmxarray);

	svm->kernel = std::make_shared<RbfKernel>(basisParam);	// TODO correct??

	pmxarray = matGetVariable(pmatfile, "support_nonlin1");
	if (pmxarray == 0) {
		std::cout << "[SvmClassifier] Error: There is a nonlinear SVM in the file, but the matrix support_nonlin1 is lacking!" << std::endl;
		exit(EXIT_FAILURE);
		return shared_ptr<SvmClassifier>(); // FIXME just a dummy return until exceptions are thrown
	} 
	if (mxGetNumberOfDimensions(pmxarray) != 3) {
		std::cout << "[SvmClassifier] Error: The matrix support_nonlin1 in the file should have 3 dimensions." << std::endl;
		exit(EXIT_FAILURE);
		return shared_ptr<SvmClassifier>(); // FIXME just a dummy return until exceptions are thrown
	}
	const mwSize *dim = mxGetDimensions(pmxarray);
	svm->numSV = (int)dim[2];
	matdata = mxGetPr(pmxarray);

	int is;
	int filter_size_x = (int)dim[1];
	int filter_size_y = (int)dim[0];

	// Alloc space for SV's and alphas (weights)
	svm->support = new unsigned char* [svm->numSV];
	int size = filter_size_x*filter_size_y;
	for (int i = 0; i < svm->numSV; ++i)
		svm->support[i] = new unsigned char[size];
	svm->alpha = new float [svm->numSV];

	int k = 0;
	for (is = 0; is < (int)dim[2]; ++is)
		for (int x = 0; x < filter_size_x; ++x)	// row-first (ML-convention)
			for (int y = 0; y < filter_size_y; ++y)
				svm->support[is][y*filter_size_x+x] = (unsigned char)(255.0*matdata[k++]);	 // because the training images gray level values were divided by 255;
	mxDestroyArray(pmxarray);

	pmxarray = matGetVariable(pmatfile, "weight_nonlin1");
	if (pmxarray == 0) {
		std::cout << "[SvmClassifier] Error: There is a nonlinear SVM in the file but the matrix threshold_nonlin is lacking." << std::endl;
		exit(EXIT_FAILURE);
		return shared_ptr<SvmClassifier>(); // FIXME just a dummy return until exceptions are thrown
	}
	matdata = mxGetPr(pmxarray);
	for (is = 0; is < svm->numSV; ++is)
		svm->alpha[is] = (float)matdata[is];
	mxDestroyArray(pmxarray);

	if (matClose(pmatfile) != 0) {
		std::cout << "[SvmClassifier] Error closing file." << std::endl;
	}

	std::cout << "[SvmClassifier] Done reading SVM!" << std::endl;

	return svm;
}

} /* namespace classification */