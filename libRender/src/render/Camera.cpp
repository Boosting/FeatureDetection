/*!
 * \file Camera.cpp
 *
 * \author Patrik Huber
 * \date December 5, 2012
 *
 * [comment here]
 */

#include "render/Camera.hpp"
#include "render/MatrixUtils.hpp"

namespace render {
	
Camera::Camera(void) : horizontalAngle(0.0f), verticalAngle(0.0f), distanceFromEyeToAt(1.0f)
{
}


Camera::~Camera(void)
{
}

void Camera::updateFixed(const cv::Vec3f& eye, const cv::Vec3f& at, const cv::Vec3f& up)
{
	this->forwardVector = at - eye;
	this->forwardVector /= cv::norm(forwardVector, cv::NORM_L2);

	this->eye = eye;
	this->at = at;
	this->up = up;
}



void Camera::updateFree(const cv::Vec3f& eye, const cv::Vec3f& up)
{
	cv::Mat transformMatrix = render::utils::MatrixUtils::createRotationMatrixX(verticalAngle) * render::utils::MatrixUtils::createRotationMatrixY(horizontalAngle);
	
	cv::Mat tmp = (cv::Mat_<float>(1, 4) << 0.0f, 0.0f, -1.0f, 0.0f);
	cv::Mat tmpRes = tmp * transformMatrix;
	forwardVector[0] = tmpRes.at<float>(0, 0);
	forwardVector[1] = tmpRes.at<float>(0, 1);
	forwardVector[2] = tmpRes.at<float>(0, 2);

	cv::Mat tmp2 = (cv::Mat_<float>(1, 4) << 1.0f, 0.0f, 0.0f, 0.0f);
	cv::Mat tmpRes2 = tmp2 * transformMatrix;
	rightVector[0] = tmpRes2.at<float>(0, 0);
	rightVector[1] = tmpRes2.at<float>(0, 1);
	rightVector[2] = tmpRes2.at<float>(0, 2);
	//rightVector = cv::Vec3f(1.0f, 0.0f, 0.0f) * transformMatrix;

	upVector = rightVector.cross(forwardVector);

	this->eye = eye;
	this->at = eye + forwardVector;
	this->up = up;
}



void Camera::updateFocused(const cv::Vec3f& at, const cv::Vec3f& up)
{
	cv::Mat transformMatrix = render::utils::MatrixUtils::createRotationMatrixX(verticalAngle) * render::utils::MatrixUtils::createRotationMatrixY(horizontalAngle);

	cv::Mat tmp = (cv::Mat_<float>(1, 4) << 0.0f, 0.0f, -1.0f, 0.0f);
	cv::Mat tmpRes = tmp * transformMatrix;
	forwardVector[0] = tmpRes.at<float>(0, 0);
	forwardVector[1] = tmpRes.at<float>(0, 1);
	forwardVector[2] = tmpRes.at<float>(0, 2);

	cv::Mat tmp2 = (cv::Mat_<float>(1, 4) << 1.0f, 0.0f, 0.0f, 0.0f);
	cv::Mat tmpRes2 = tmp2 * transformMatrix;
	rightVector[0] = tmpRes2.at<float>(0, 0);
	rightVector[1] = tmpRes2.at<float>(0, 1);
	rightVector[2] = tmpRes2.at<float>(0, 2);
	//rightVector = cv::Vec3f(1.0f, 0.0f, 0.0f) * transformMatrix;
	
	upVector = rightVector.cross(forwardVector);

	this->eye = at - forwardVector*distanceFromEyeToAt;
	this->at = at;
	this->up = up;
}

}
