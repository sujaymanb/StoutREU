#pragma once
#include <Kinect.h>
#include "opencv2/core.hpp"

#define cColorWidth 1920
#define cColorHeight 1080

#define armMarkerId 0
#define bowlMarkerId 1

class ArTracker
{
	const float arucoSquareDimensions = 0.066f; // meters	
	IColorFrameReader* reader;
	cv::Mat cameraMatrix, distCoefficients;

public:
	ArTracker();

	~ArTracker();

	void GetARPosition(cv::Vec3d& armTranslation, cv::Vec3d& bowlTranslation);

private:
	bool getKinectData(cv::Mat& colorMat);

	bool loadCameraCalibration(string name);
};