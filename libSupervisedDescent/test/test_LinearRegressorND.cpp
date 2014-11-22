#include "gtest/gtest.h"

#include "superviseddescent/superviseddescent.hpp"

#include "opencv2/core/core.hpp"

TEST(LinearRegressor, NDimOneExampleLearningNotInvertible) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	Mat data = Mat::ones(1, 2, CV_32FC1);
	Mat labels = Mat::ones(1, 1, CV_32FC1);
	// A simple case of a singular matrix. Yields infinitely many possible results.
	v2::LinearRegressor lr;
	bool isInvertible = lr.learn(data, labels);
	ASSERT_EQ(false, isInvertible);
}

TEST(LinearRegressor, NDimOneExampleLearningRegularisation) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	v2::Regulariser r(v2::Regulariser::RegularisationType::Manual, 1.0f, true); // no bias, so regularise every data-row
	Mat data = Mat::ones(1, 2, CV_32FC1);
	Mat labels = Mat::ones(1, 1, CV_32FC1);
	// This case becomes solvable with regularisation
	v2::LinearRegressor lr(r);
	bool isInvertible = lr.learn(data, labels);
	EXPECT_FLOAT_EQ(1.0f/3.0f, lr.x.at<float>(0)) << "Expected the learned x_0 to be 1.0f/3.0f";
	EXPECT_FLOAT_EQ(1.0f/3.0f, lr.x.at<float>(1)) << "Expected the learned x_1 to be 1.0f/3.0f";
	ASSERT_EQ(true, isInvertible);
}

// We can't construct an invertible 2D example with 1 training point
TEST(LinearRegressor, NDimTwoExamplesLearning) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	Mat data = Mat::ones(2, 2, CV_32FC1);
	data.at<float>(0) = 0.0f; // data = [0 1; 1 1]
	Mat labels = Mat::ones(2, 1, CV_32FC1); // The label can also be multi-dim. More test cases?
	labels.at<float>(0) = 0.0f; // labels = [0; 1]
	v2::LinearRegressor lr;
	bool isInvertible = lr.learn(data, labels);
	EXPECT_EQ(true, isInvertible);
	EXPECT_FLOAT_EQ(1.0f, lr.x.at<float>(0)) << "Expected the learned x_0 to be 1.0f";
	ASSERT_FLOAT_EQ(0.0f, lr.x.at<float>(1)) << "Expected the learned x_1 to be 0.0f";
}

TEST(LinearRegressor, NDimTwoExamplesPrediction) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	// Note/Todo: Load from filesystem, or from memory-bytes?
	Mat data = Mat::ones(2, 2, CV_32FC1);
	data.at<float>(0) = 0.0f; // data = [0 1; 1 1]
	Mat labels = Mat::ones(2, 1, CV_32FC1); // The label can also be multi-dim. More test cases?
	labels.at<float>(0) = 0.0f; // labels = [0; 1]
	v2::LinearRegressor lr;
	lr.learn(data, labels);

	// Test starts here:
	Mat test = 2.0f * Mat::ones(1, 2, CV_32FC1);
	Mat prediction = lr.predict(test);
	ASSERT_FLOAT_EQ(2.0f, prediction.at<float>(0)) << "Expected the prediction to be 2.0f";
}

TEST(LinearRegressor, NDimTwoExamplesTestingResidual) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	// Note/Todo: Load from filesystem, or from memory-bytes?
	Mat data = Mat::ones(2, 2, CV_32FC1);
	data.at<float>(0) = 0.0f; // data = [0 1; 1 1]
	Mat labels = Mat::ones(2, 1, CV_32FC1); // The label can also be multi-dim. More test cases?
	labels.at<float>(0) = 0.0f; // labels = [0; 1]
	v2::LinearRegressor lr;
	lr.learn(data, labels);

	// Test starts here:
	Mat test(3, 2, CV_32FC1);
	test.at<float>(0, 0) = 0.0f;
	test.at<float>(0, 1) = 2.0f;
	test.at<float>(1, 0) = 2.0f;
	test.at<float>(1, 1) = 1.0f;
	test.at<float>(2, 0) = 2.0f;
	test.at<float>(2, 1) = 1.0f; // test = [ 0 2; 2 1; 2 1]
	Mat groundtruth(3, 1, CV_32FC1);
	groundtruth.at<float>(0) = 0.0f;
	groundtruth.at<float>(1) = 2.0f;
	groundtruth.at<float>(2) = -1.0f; // gt = [ 0 (error 0), 2 (error 0), -1 (error -3)]
	double residual = lr.test(test, groundtruth);
	ASSERT_DOUBLE_EQ(1.3416407864998738, residual) << "Expected the residual to be 1.34164...";
}

TEST(LinearRegressor, NDimTwoExamplesNDimYLearning) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	Mat data = Mat::ones(2, 2, CV_32FC1);
	data.at<float>(0) = 0.0f; // data = [0 1; 1 1]
	Mat labels = Mat::ones(2, 2, CV_32FC1); // The label can also be multi-dim. More test cases?
	labels.at<float>(0) = 0.0f; // labels = [0 1; 1 1]
	v2::LinearRegressor lr;
	bool isInvertible = lr.learn(data, labels);
	EXPECT_EQ(true, isInvertible);
	EXPECT_FLOAT_EQ(1.0f, lr.x.at<float>(0, 0)) << "Expected the learned x_0_0 to be 1.0f"; // Every col is a learned regressor for a label
	EXPECT_FLOAT_EQ(0.0f, lr.x.at<float>(1, 0)) << "Expected the learned x_1_0 to be 0.0f";
	EXPECT_FLOAT_EQ(0.0f, lr.x.at<float>(0, 1)) << "Expected the learned x_0_1 to be 0.0f";
	ASSERT_FLOAT_EQ(1.0f, lr.x.at<float>(1, 1)) << "Expected the learned x_1_1 to be 1.0f";
}

TEST(LinearRegressor, NDimTwoExamplesNDimYPrediction) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	Mat data = Mat::ones(2, 2, CV_32FC1);
	data.at<float>(0) = 0.0f; // data = [0 1; 1 1]
	Mat labels = Mat::ones(2, 2, CV_32FC1); // The label can also be multi-dim. More test cases?
	labels.at<float>(0) = 0.0f; // labels = [0 1; 1 1]
	v2::LinearRegressor lr;
	bool isInvertible = lr.learn(data, labels);

	// Test starts here:
	Mat test = Mat::ones(1, 2, CV_32FC1);
	test.at<float>(1) = 2.0f;
	Mat prediction = lr.predict(test);
	EXPECT_FLOAT_EQ(1.0f, prediction.at<float>(0)) << "Expected the predicted y_0 to be 1.0f";
	ASSERT_FLOAT_EQ(2.0f, prediction.at<float>(1)) << "Expected the predicted y_1 to be 2.0f";
}

TEST(LinearRegressor, NDimTwoExamplesNDimYTestingResidual) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	Mat data = Mat::ones(2, 2, CV_32FC1);
	data.at<float>(0) = 0.0f; // data = [0 1; 1 1]
	Mat labels = Mat::ones(2, 2, CV_32FC1); // The label can also be multi-dim. More test cases?
	labels.at<float>(0) = 0.0f; // labels = [0 1; 1 1]
	v2::LinearRegressor lr;
	bool isInvertible = lr.learn(data, labels);

	// Test starts here:
	Mat test(3, 2, CV_32FC1);
	test.at<float>(0, 0) = 0.0f;
	test.at<float>(0, 1) = 2.0f;
	test.at<float>(1, 0) = 2.0f;
	test.at<float>(1, 1) = 1.0f;
	test.at<float>(2, 0) = 2.0f;
	test.at<float>(2, 1) = 1.0f; // test = [ 0 2; 2 1; 2 1]
	Mat groundtruth(3, 2, CV_32FC1);
	groundtruth.at<float>(0, 0) = 0.0f;
	groundtruth.at<float>(1, 0) = 2.0f;
	groundtruth.at<float>(2, 0) = -1.0f;
	groundtruth.at<float>(0, 1) = 0.0f;
	groundtruth.at<float>(1, 1) = 4.0f;
	groundtruth.at<float>(2, 1) = -2.0f;
	double residual = lr.test(test, groundtruth);
	ASSERT_DOUBLE_EQ(1.1135528725660042, residual) << "Expected the residual to be 1.11355...";
}

TEST(LinearRegressor, NDimManyExamplesNDimY) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	// An invertible example, constructed from Matlab
	Mat data = (cv::Mat_<float>(5, 3) << 1.0f, 4.0f, 2.0f, 4.0f, 9.0f, 1.0f, 6.0f, 5.0f, 2.0f, 0.0f, 6.0f, 2.0f, 6.0f, 1.0f, 9.0f);
	Mat labels = (cv::Mat_<float>(5, 2) << 1.0f, 1.0f, 2.0f, 5.0f, 3.0f, -2.0f, 0.0f, 5.0f, 6.0f, 3.0f);
	v2::LinearRegressor lr;
	bool isInvertible = lr.learn(data, labels);
	EXPECT_EQ(true, isInvertible);
	EXPECT_FLOAT_EQ(0.489539176f, lr.x.at<float>(0, 0)) << "Expected the learned x_0_0 to be different"; // Every col is a learned regressor for a label
	EXPECT_FLOAT_EQ(-0.0660829917f, lr.x.at<float>(1, 0)) << "Expected the learned x_1_0 to be different";
	EXPECT_FLOAT_EQ(0.339629412f, lr.x.at<float>(2, 0)) << "Expected the learned x_2_0 to be different";
	EXPECT_FLOAT_EQ(-0.833899379f, lr.x.at<float>(0, 1)) << "Expected the learned x_0_1 to be different";
	EXPECT_FLOAT_EQ(0.626753688f, lr.x.at<float>(1, 1)) << "Expected the learned x_1_1 to be different";
	EXPECT_FLOAT_EQ(0.744218946f, lr.x.at<float>(2, 1)) << "Expected the learned x_2_1 to be different";

	// Testing:
	Mat test = (cv::Mat_<float>(3, 3) << 2.0f, 6.0f, 5.0f, 2.9f, -11.3, 6.0f, -2.0f, -8.438f, 3.3f);
	Mat groundtruth = (cv::Mat_<float>(3, 2) << 2.2807f, 5.8138f, 4.2042f, -5.0353f, 0.6993f, -1.1648f);
	double residual = lr.test(test, groundtruth);
	ASSERT_LE(residual, 0.000006) << "Expected the residual to be smaller than "; // we could relax that to 0.00001 or even larger, but for now, I want to be pedantic
}

TEST(LinearRegressor, NDimManyExamplesNDimYRegularisation) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	// An invertible example, constructed from Matlab
	Mat data = (cv::Mat_<float>(5, 3) << 1.0f, 4.0f, 2.0f, 4.0f, 9.0f, 1.0f, 6.0f, 5.0f, 2.0f, 0.0f, 6.0f, 2.0f, 6.0f, 1.0f, 9.0f);
	Mat labels = (cv::Mat_<float>(5, 2) << 1.0f, 1.0f, 2.0f, 5.0f, 3.0f, -2.0f, 0.0f, 5.0f, 6.0f, 3.0f);
	v2::Regulariser r(v2::Regulariser::RegularisationType::Manual, 50.0f, true); // no bias, so regularise every data-row
	v2::LinearRegressor lr(r);
	bool isInvertible = lr.learn(data, labels);
	EXPECT_EQ(true, isInvertible);
	EXPECT_FLOAT_EQ(0.282755911f, lr.x.at<float>(0, 0)) << "Expected the learned x_0_0 to be different"; // Every col is a learned regressor for a label
	EXPECT_FLOAT_EQ(0.0360795595f, lr.x.at<float>(1, 0)) << "Expected the learned x_1_0 to be different";
	EXPECT_FLOAT_EQ(0.291039944f, lr.x.at<float>(2, 0)) << "Expected the learned x_2_0 to be different";
	EXPECT_FLOAT_EQ(-0.0989616737f, lr.x.at<float>(0, 1)) << "Expected the learned x_0_1 to be different";
	EXPECT_FLOAT_EQ(0.330635577f, lr.x.at<float>(1, 1)) << "Expected the learned x_1_1 to be different";
	EXPECT_FLOAT_EQ(0.217046738f, lr.x.at<float>(2, 1)) << "Expected the learned x_2_1 to be different";

	// Testing:
	Mat test = (cv::Mat_<float>(3, 3) << 2.0f, 6.0f, 5.0f, 2.9f, -11.3, 6.0f, -2.0f, -8.438f, 3.3f);
	Mat groundtruth = (cv::Mat_<float>(3, 2) << 2.2372f, 2.8711f, 2.1585f, -2.7209f, 0.0905f, -1.8757f);
	double residual = lr.test(test, groundtruth);
	ASSERT_LE(residual, 0.000011) << "Expected the residual to be smaller than "; // we could relax that a bit
}

TEST(LinearRegressor, NDimManyExamplesNDimYBias) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	// An invertible example, constructed from Matlab
	Mat data = (cv::Mat_<float>(5, 3) << 1.0f, 4.0f, 2.0f, 4.0f, 9.0f, 1.0f, 6.0f, 5.0f, 2.0f, 0.0f, 6.0f, 2.0f, 6.0f, 1.0f, 9.0f);
	Mat labels = (cv::Mat_<float>(5, 2) << 1.0f, 1.0f, 2.0f, 5.0f, 3.0f, -2.0f, 0.0f, 5.0f, 6.0f, 3.0f);
	v2::LinearRegressor lr;
	Mat biasColumn = Mat::ones(data.rows, 1, CV_32FC1);
	cv::hconcat(data, biasColumn, data);
	bool isInvertible = lr.learn(data, labels);
	EXPECT_EQ(true, isInvertible);
	auto a1 = lr.x.at<float>(0, 0);
	auto a2 = lr.x.at<float>(1, 0);
	auto a3 = lr.x.at<float>(2, 0);
	auto ann = lr.x.at<float>(3, 0);
	auto a4 = lr.x.at<float>(0, 1);
	auto a5 = lr.x.at<float>(1, 1);
	auto a6 = lr.x.at<float>(2, 1);
	auto annn = lr.x.at<float>(3, 1);
	EXPECT_FLOAT_EQ(0.485008746f, lr.x.at<float>(0, 0)) << "Expected the learned x_0_0 to be different"; // Every col is a learned regressor for a label
	EXPECT_FLOAT_EQ(0.0122186244f, lr.x.at<float>(1, 0)) << "Expected the learned x_1_0 to be different";
	EXPECT_FLOAT_EQ(0.407824278f, lr.x.at<float>(2, 0)) << "Expected the learned x_2_0 to be different";
	EXPECT_FLOAT_EQ(-0.615155935f, lr.x.at<float>(3, 0)) << "Expected the learned x_3_0 to be different";
	EXPECT_FLOAT_EQ(-0.894791782f, lr.x.at<float>(0, 1)) << "Expected the learned x_0_1 to be different";
	EXPECT_FLOAT_EQ(1.67920494f, lr.x.at<float>(1, 1)) << "Expected the learned x_1_1 to be different";
	EXPECT_FLOAT_EQ(1.66081595f, lr.x.at<float>(2, 1)) << "Expected the learned x_2_1 to be different";
	EXPECT_FLOAT_EQ(-8.26834011f, lr.x.at<float>(3, 1)) << "Expected the learned x_3_1 to be different";

	// Testing:
	Mat test = (cv::Mat_<float>(3, 3) << 2.0f, 6.0f, 5.0f, 2.9f, -11.3, 6.0f, -2.0f, -8.438f, 3.3f);
	Mat biasColumnTest = Mat::ones(test.rows, 1, CV_32FC1);
	cv::hconcat(test, biasColumnTest, test);
	Mat groundtruth = (cv::Mat_<float>(3, 2) << 2.4673f, 8.3214f, 3.1002f, -19.8734f, -0.3425f, -15.1672f);
	double residual = lr.test(test, groundtruth);
	ASSERT_LE(residual, 0.000003) << "Expected the residual to be smaller than "; // we could relax that a bit
}

// Actually we could test the Regulariser separately, no need to have separate unit tests.
TEST(LinearRegressor, NDimManyExamplesNDimYBiasRegularisation) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	// An invertible example, constructed from Matlab
	Mat data = (cv::Mat_<float>(5, 3) << 1.0f, 4.0f, 2.0f, 4.0f, 9.0f, 1.0f, 6.0f, 5.0f, 2.0f, 0.0f, 6.0f, 2.0f, 6.0f, 1.0f, 9.0f);
	Mat labels = (cv::Mat_<float>(5, 2) << 1.0f, 1.0f, 2.0f, 5.0f, 3.0f, -2.0f, 0.0f, 5.0f, 6.0f, 3.0f);
	v2::Regulariser r(v2::Regulariser::RegularisationType::Manual, 50.0f, true); // regularise the bias as well
	v2::LinearRegressor lr(r);
	Mat biasColumn = Mat::ones(data.rows, 1, CV_32FC1);
	cv::hconcat(data, biasColumn, data);
	bool isInvertible = lr.learn(data, labels);
	EXPECT_EQ(true, isInvertible);
	auto a1 = lr.x.at<float>(0, 0);
	auto a2 = lr.x.at<float>(1, 0);
	auto a3 = lr.x.at<float>(2, 0);
	auto ann = lr.x.at<float>(3, 0);
	auto a4 = lr.x.at<float>(0, 1);
	auto a5 = lr.x.at<float>(1, 1);
	auto a6 = lr.x.at<float>(2, 1);
	auto annn = lr.x.at<float>(3, 1);
	EXPECT_FLOAT_EQ(0.281424582f, lr.x.at<float>(0, 0)) << "Expected the learned x_0_0 to be different"; // Every col is a learned regressor for a label
	EXPECT_FLOAT_EQ(0.0331765190f, lr.x.at<float>(1, 0)) << "Expected the learned x_1_0 to be different";
	EXPECT_FLOAT_EQ(0.289116770f, lr.x.at<float>(2, 0)) << "Expected the learned x_2_0 to be different";
	EXPECT_FLOAT_EQ(0.0320090912f, lr.x.at<float>(3, 0)) << "Expected the learned x_3_0 to be different";
	EXPECT_FLOAT_EQ(-0.100544833f, lr.x.at<float>(0, 1)) << "Expected the learned x_0_1 to be different";
	EXPECT_FLOAT_EQ(0.327183396f, lr.x.at<float>(1, 1)) << "Expected the learned x_1_1 to be different";
	EXPECT_FLOAT_EQ(0.214759737f, lr.x.at<float>(2, 1)) << "Expected the learned x_2_1 to be different";
	EXPECT_FLOAT_EQ(0.0380640067f, lr.x.at<float>(3, 1)) << "Expected the learned x_3_1 to be different";

	// Testing:
	Mat test = (cv::Mat_<float>(3, 3) << 2.0f, 6.0f, 5.0f, 2.9f, -11.3, 6.0f, -2.0f, -8.438f, 3.3f);
	Mat biasColumnTest = Mat::ones(test.rows, 1, CV_32FC1);
	cv::hconcat(test, biasColumnTest, test);
	Mat groundtruth = (cv::Mat_<float>(3, 2) << 2.2395f, 2.8739f, 2.2079f, -2.6621f, 0.1433f, -1.8129f);
	double residual = lr.test(test, groundtruth);
	ASSERT_LE(residual, 0.000012) << "Expected the residual to be smaller than "; // we could relax that a bit
}

TEST(LinearRegressor, NDimManyExamplesNDimYBiasRegularisationButNotBias) {
	using cv::Mat;
	namespace v2 = superviseddescent::v2;
	// An invertible example, constructed from Matlab
	Mat data = (cv::Mat_<float>(5, 3) << 1.0f, 4.0f, 2.0f, 4.0f, 9.0f, 1.0f, 6.0f, 5.0f, 2.0f, 0.0f, 6.0f, 2.0f, 6.0f, 1.0f, 9.0f);
	Mat labels = (cv::Mat_<float>(5, 2) << 1.0f, 1.0f, 2.0f, 5.0f, 3.0f, -2.0f, 0.0f, 5.0f, 6.0f, 3.0f);
	v2::Regulariser r(v2::Regulariser::RegularisationType::Manual, 50.0f, false); // don't regularise the bias
	v2::LinearRegressor lr(r);
	Mat biasColumn = Mat::ones(data.rows, 1, CV_32FC1);
	cv::hconcat(data, biasColumn, data);
	bool isInvertible = lr.learn(data, labels);
	EXPECT_EQ(true, isInvertible);
	auto a1 = lr.x.at<float>(0, 0);
	auto a2 = lr.x.at<float>(1, 0);
	auto a3 = lr.x.at<float>(2, 0);
	auto ann = lr.x.at<float>(3, 0);
	auto a4 = lr.x.at<float>(0, 1);
	auto a5 = lr.x.at<float>(1, 1);
	auto a6 = lr.x.at<float>(2, 1);
	auto annn = lr.x.at<float>(3, 1);
	EXPECT_FLOAT_EQ(0.218878254f, lr.x.at<float>(0, 0)) << "Expected the learned x_0_0 to be different"; // Every col is a learned regressor for a label
	EXPECT_FLOAT_EQ(-0.103211358f, lr.x.at<float>(1, 0)) << "Expected the learned x_1_0 to be different";
	EXPECT_FLOAT_EQ(0.198760599f, lr.x.at<float>(2, 0)) << "Expected the learned x_2_0 to be different";
	EXPECT_FLOAT_EQ(1.53583705f, lr.x.at<float>(3, 0)) << "Expected the learned x_3_0 to be different";
	EXPECT_FLOAT_EQ(-0.174922630f, lr.x.at<float>(0, 1)) << "Expected the learned x_0_1 to be different";
	EXPECT_FLOAT_EQ(0.164996058f, lr.x.at<float>(1, 1)) << "Expected the learned x_1_1 to be different";
	EXPECT_FLOAT_EQ(0.107311621f, lr.x.at<float>(2, 1)) << "Expected the learned x_2_1 to be different";
	EXPECT_FLOAT_EQ(1.82635951f, lr.x.at<float>(3, 1)) << "Expected the learned x_3_1 to be different";

	// Testing:
	Mat test = (cv::Mat_<float>(3, 3) << 2.0f, 6.0f, 5.0f, 2.9f, -11.3, 6.0f, -2.0f, -8.438f, 3.3f);
	Mat biasColumnTest = Mat::ones(test.rows, 1, CV_32FC1);
	cv::hconcat(test, biasColumnTest, test);
	Mat groundtruth = (cv::Mat_<float>(3, 2) << 2.3481f, 3.0030f, 4.5294f, 0.0985f, 2.6249f, 1.1381f);
	double residual = lr.test(test, groundtruth);
	ASSERT_LE(residual, 0.000011) << "Expected the residual to be smaller than "; // we could relax that a bit
}
