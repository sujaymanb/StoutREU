#include <Windows.h>
#include <Ole2.h>
#include <Kinect.h>
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/aruco.hpp"
#include "opencv2/calib3d.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include "ArTracker.h"
#include "SpeechBasics-D2D/SpeechBasics.h"

using namespace std;
using namespace cv;

ArTracker::ArTracker()
{
	// init kinect color reader
	IColorFrameSource* framesource = NULL;
	m_pKinectSensor->get_ColorFrameSource(&framesource);

	if (framesource)
	{
		framesource->OpenReader(&reader);
		OutputDebugString(L"Reader opened successfully");
		framesource->Release();
		framesource = NULL;
	}

	Sleep(6000);

	// init camera
	cameraMatrix = Mat::eye(3, 3, CV_64F);
	loadCameraCalibration("TestKinectCalib.txt");
}

ArTracker::~ArTracker()
{
	if (reader)
	{
		reader->Release();
		reader = NULL;
	}
}

void ArTracker::GetARPosition(Vec3d& armTranslation, Vec3d& bowlTranslation)
{
	Mat frame(cColorHeight, cColorWidth, CV_8UC3);
	Mat flipped(cColorHeight, cColorWidth, CV_8UC3);
	vector<int> markerIds;
	vector<vector<Point2f>> markerCorners, rejectedCandidates;
	aruco::DetectorParameters parameters;

	Ptr<aruco::Dictionary> markerDictionary = aruco::getPredefinedDictionary(aruco::PREDEFINED_DICTIONARY_NAME::DICT_4X4_50);
	bool armMarkerFound = false, bowlMarkerFound = false;
	vector<Vec3d> rotationVectors, translationVectors;

	while (!armMarkerFound || !bowlMarkerFound)
	{
		if (!getKinectData(frame))
		{
			OutputDebugString(L"failed to read\n");
		}

		flip(frame, flipped, 1);
		aruco::detectMarkers(flipped, markerDictionary, markerCorners, markerIds);
		aruco::estimatePoseSingleMarkers(markerCorners, arucoSquareDimensions, cameraMatrix, distCoefficients, rotationVectors, translationVectors);
		//aruco::drawDetectedMarkers(flipped, markerCorners, markerIds);
		for (int i = 0; i < markerIds.size(); i++)
		{
			if (markerIds[i] == armMarkerId)
			{
				armMarkerFound = true;
				armTranslation = translationVectors[i];
			}

			if (markerIds[i] == bowlMarkerId)
			{
				bowlMarkerFound = true;
				bowlTranslation = translationVectors[i];
			}
		}
	}
}

bool ArTracker::getKinectData(cv::Mat& colorMat)
{
	IColorFrame* frame = NULL;
	ColorImageFormat imageFormat = ColorImageFormat_None;
	HRESULT hr;
	UINT bufferSize = 0;
	BYTE *pBuffer = nullptr;
	hr = reader->AcquireLatestFrame(&frame);

	if (SUCCEEDED(hr))
	{
		hr = frame->AccessRawUnderlyingBuffer(&bufferSize, &pBuffer); // YUY2
	}

	if (SUCCEEDED(hr))
	{
		//cout << "Frame read\n";
		Mat bufferMat(cColorHeight, cColorWidth, CV_8UC2, pBuffer);
		cvtColor(bufferMat, colorMat, COLOR_YUV2BGR_YUYV);
		if (frame) frame->Release();
		return true;
	}

	if (frame) frame->Release();
	OutputDebugString(L"Failed to get frame\n");
	return false;
}

bool ArTracker::loadCameraCalibration(string name)
{
	ifstream inStream(name);
	if (inStream)
	{
		uint16_t rows;
		uint16_t columns;

		inStream >> rows;
		inStream >> columns;

		cameraMatrix = Mat(Size(columns, rows), CV_64F);

		// cam matrix
		for (int r = 0; r < rows; r++)
		{
			for (int c = 0; c < columns; c++)
			{
				double read = 0.0f;
				inStream >> read;
				cameraMatrix.at<double>(r, c) = read;
				cout << cameraMatrix.at<double>(r, c) << "\n";
			}
		}

		// distortion coeff
		inStream >> rows;
		inStream >> columns;

		distCoefficients = Mat::zeros(rows, columns, CV_64F);

		for (int r = 0; r < rows; r++)
		{
			for (int c = 0; c < columns; c++)
			{
				double read = 0.0f;
				inStream >> read;
				distCoefficients.at<double>(r, c) = read;
				cout << distCoefficients.at<double>(r, c) << "\n";
			}
		}

		inStream.close();
		return true;
	}

	return false;
}