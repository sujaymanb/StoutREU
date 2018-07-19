#pragma once

#include "opencv2/core.hpp"

// Kinova Includes
#include "CommunicationLayerWindows.h"
#include "CommandLayer.h"
#include <conio.h>
#include "KinovaTypes.h"


extern cv::Vec3d armVec, bowlVec; 
class JacoArm
{
public:
	// Constructor
	JacoArm(cv::Vec3d armVec);

	// Destructor
	~JacoArm();

	/// <summary>
	/// Wait for arm to stop moving
	/// </summary>
	int WaitForArmMove();

	/// <summary>
	/// Move arm to given coordinates
	/// </summary>
	int MoveArm(float x, float y, float z);
	
	/// <summary>
	/// move to the designated neutral position
	/// </summary>
	int MoveToNeutralPosition();

	/// <summary>
	/// Pick up food in scoop style
	/// </summary>
	int Scoop();

	/// <summary>
	/// Pick up food in soup style
	/// </summary>
	int Soup();

	/// <summary>
	/// map kinect coords to arm command
	/// </summary>
	void KinectToArm(float kx, float ky, float kz, float* x, float* y, float* z);

	/// <summary>
	/// sends a point returns false if it doesn't send or is stopped
	/// </summary>
	int SendPoint(TrajectoryPoint pointToSend);

	float				   bowl_xpos;
	float				   bowl_ypos;
	float				   bowl_zpos;

private:
	// offsets
	float				   x_offset;
	float				   y_offset;
	float				   z_offset;

	
};