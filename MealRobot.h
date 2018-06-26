#pragma once
#include "resource.h"
#include "ImageRenderer.h"
#include "FaceBasics.h"

class MealRobot : private CFaceBasics {
public:
	/// <summary>
	/// Move arm to given coordinates
	/// </summary>
	int MoveArm(float x, float y, float z);

	/// <summary>
	/// Override
	/// </summary>
	void ProcessFaces();

	/// <summary>
	/// Get the goal position
	/// </summary>
	HRESULT GetMouthPosition(IBody* pBody, CameraSpacePoint* mouthPosition);
};