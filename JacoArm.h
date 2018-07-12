#pragma once

#include "opencv2/core.hpp"
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
	int WaitForArmMove(float goalX, float goalY, float goalZ);

	/// <summary>
	/// Move arm to given coordinates
	/// </summary>
	int MoveArm(float x, float y, float z);

	/// <summary>
	/// Pick up food in scoop style
	/// </summary>
	int Scoop();

	/// <summary>
	/// Pick up food in soup style
	/// </summary>
	int Soup();

	/// <summary>
	/// Check if arm is moving
	/// </summary>
	bool ArmMoving(float newX, float newY, float newZ, float oldX, float oldY, float oldZ);

	/// <summary>
	/// map kinect coords to arm command
	/// </summary>
	void KinectToArm(float kx, float ky, float kz, float* x, float* y, float* z);
private:
	// offsets
	float				   x_offset;
	float				   y_offset;
	float				   z_offset;
};