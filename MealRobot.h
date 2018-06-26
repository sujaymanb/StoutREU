#pragma once
#include "resource.h"
#include "ImageRenderer.h"
#include "FaceBasics.h"

class MealRobot : public CFaceBasics {
private:
	/// <summary>
	/// Move arm to given coordinates
	/// </summary>
	int MoveArm(float x, float y, float z);
};