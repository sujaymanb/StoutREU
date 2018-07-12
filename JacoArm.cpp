#include "JacoArm.h"
#include "FaceBasics.h"
#include "stdafx.h"
#include <strsafe.h>
#include "resource.h"
// Kinova Includes
#include "CommunicationLayerWindows.h"
#include "CommandLayer.h"
#include <conio.h>
#include "KinovaTypes.h"

// other
#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>

// Kinova API init

// A handle to the API.
HINSTANCE commandLayer_handle;

// Function pointers to the functions we need
int(*MyInitAPI)();
int(*MyCloseAPI)();
int(*MySendBasicTrajectory)(TrajectoryPoint command);
int(*MySendAdvanceTrajectory)(TrajectoryPoint command);
int(*MyGetDevices)(KinovaDevice devices[MAX_KINOVA_DEVICE], int &result);
int(*MySetActiveDevice)(KinovaDevice device);
int(*MyMoveHome)();
int(*MyInitFingers)();
int(*MyGetCartesianCommand)(CartesianPosition &);
int(*MyEraseAllTrajectories)();

JacoArm::JacoArm(cv::Vec3d armVec) 
{
	x_offset = -armVec[0];
	y_offset = -armVec[1];
	z_offset = armVec[2];

	std::wstringstream s;
	s << L"\noffset x: " << x_offset << "\n" << "offset y: " << y_offset << "\n" << "offset z: " << z_offset << "\n";
	std::wstring ws = s.str();
	LPCWSTR l = ws.c_str();
	OutputDebugString(l);

	//We load the API.
	commandLayer_handle = LoadLibrary(L"CommandLayerWindows.dll");
	int programResult = 0;

	//We load the functions from the library (Under Windows, use GetProcAddress)
	MyInitAPI = (int(*)()) GetProcAddress(commandLayer_handle, "InitAPI");
	MyCloseAPI = (int(*)()) GetProcAddress(commandLayer_handle, "CloseAPI");
	MyMoveHome = (int(*)()) GetProcAddress(commandLayer_handle, "MoveHome");
	MyInitFingers = (int(*)()) GetProcAddress(commandLayer_handle, "InitFingers");
	MyGetDevices = (int(*)(KinovaDevice devices[MAX_KINOVA_DEVICE], int &result)) GetProcAddress(commandLayer_handle, "GetDevices");
	MySetActiveDevice = (int(*)(KinovaDevice devices)) GetProcAddress(commandLayer_handle, "SetActiveDevice");
	MySendBasicTrajectory = (int(*)(TrajectoryPoint)) GetProcAddress(commandLayer_handle, "SendBasicTrajectory");
	MySendAdvanceTrajectory = (int(*)(TrajectoryPoint)) GetProcAddress(commandLayer_handle, "SendAdvanceTrajectory");
	MyGetCartesianCommand = (int(*)(CartesianPosition &)) GetProcAddress(commandLayer_handle, "GetCartesianCommand");
	MyEraseAllTrajectories = (int(*)()) GetProcAddress(commandLayer_handle, "EraseAllTrajectories");

	//Verify that all functions has been loaded correctly
	if ((MyInitAPI == NULL) || (MyCloseAPI == NULL) || (MySendBasicTrajectory == NULL) ||
		(MyGetDevices == NULL) || (MySetActiveDevice == NULL) || (MyGetCartesianCommand == NULL) ||
		(MyMoveHome == NULL) || (MyInitFingers == NULL))

	{
		OutputDebugString(L"FaceBasics.cpp: Error During Arm Initialization\n");
		programResult = 0;
	}
	else
	{
		int result = (*MyInitAPI)();

		KinovaDevice list[MAX_KINOVA_DEVICE];

		int devicesCount = MyGetDevices(list, result);

		if (devicesCount < 1)
		{
			OutputDebugString(L"JacoArm.cpp: Robot not found \n");
		}
		else
		{

			std::wstringstream s;
			s << L"JacoArm.cpp: Found a robot on the USB bus (" << list[0].SerialNumber << ")\n";
			std::wstring ws = s.str();
			LPCWSTR l = ws.c_str();
			OutputDebugString(l);

			//Setting the current device as the active device.
			MySetActiveDevice(list[0]);

			Sleep(1000);
			// Move home
			MoveArm(.5273, -.4949, .0674);
		}
	}
}

JacoArm::~JacoArm()
{
	// close arm api
	int result = (*MyCloseAPI)();
	FreeLibrary(commandLayer_handle);
}
/// <summary>
/// Move arm to given position
/// </summary>
int JacoArm::MoveArm(float x, float y, float z)
{
 	MyEraseAllTrajectories();
	CartesianPosition currentCommand;
	TrajectoryPoint pointToSend;
	pointToSend.InitStruct();

	//We specify that this point will be a Cartesian Position.
	pointToSend.Position.Type = CARTESIAN_POSITION;
	pointToSend.LimitationsActive = 0;

	MyGetCartesianCommand(currentCommand);
	
	float neutral_x, neutral_y, neutral_z;
	
	KinectToArm(0.0, .15, 0.1, &neutral_x, &neutral_y, &neutral_z);

	pointToSend.Position.CartesianPosition.X = neutral_x;
	pointToSend.Position.CartesianPosition.Y = neutral_y;
	pointToSend.Position.CartesianPosition.Z = neutral_z;
	pointToSend.Position.CartesianPosition.ThetaX = 1.8796;
	pointToSend.Position.CartesianPosition.ThetaY = 0.4309;
	pointToSend.Position.CartesianPosition.ThetaZ = -1.5505;
	pointToSend.Position.Fingers.Finger1 = currentCommand.Fingers.Finger1;
	pointToSend.Position.Fingers.Finger2 = currentCommand.Fingers.Finger2;
	pointToSend.Position.Fingers.Finger3 = currentCommand.Fingers.Finger3;

	int result = MySendAdvanceTrajectory(pointToSend);
	if (result != NO_ERROR_KINOVA)
	{
		OutputDebugString(L"Could not send advanced trajectory");
	}
	
	pointToSend.Position.CartesianPosition.X = x;
	pointToSend.Position.CartesianPosition.Y = y;
	pointToSend.Position.CartesianPosition.Z = z;

	OutputDebugString(L"Sending the point to the robot.\n");
	result = MySendAdvanceTrajectory(pointToSend);
	if (result != NO_ERROR_KINOVA)
	{
		OutputDebugString(L"Could not send advanced trajectory");
	}

	WaitForArmMove(x, y, z);
	MyEraseAllTrajectories();

	return 1;
}


/// <summary>
/// Scoop up food
/// </summary>

int JacoArm::Scoop()
{
	MyEraseAllTrajectories();
	CartesianPosition currentCommand;
	TrajectoryPoint pointToSend;
	pointToSend.InitStruct();

	//We specify that this point will be a Cartesian Position.
	pointToSend.Position.Type = CARTESIAN_POSITION;
	pointToSend.LimitationsActive = 0;

	MyGetCartesianCommand(currentCommand);

	// dip down and back up for now
	pointToSend.Position.CartesianPosition.X = currentCommand.Coordinates.X;
	pointToSend.Position.CartesianPosition.Y = currentCommand.Coordinates.Y;
	pointToSend.Position.CartesianPosition.Z = currentCommand.Coordinates.Z - 0.05f;
	pointToSend.Position.CartesianPosition.ThetaX = 2.4858;
	pointToSend.Position.CartesianPosition.ThetaY = 0.3713;
	pointToSend.Position.CartesianPosition.ThetaZ = -1.5505;
	pointToSend.Position.Fingers.Finger1 = currentCommand.Fingers.Finger1;
	pointToSend.Position.Fingers.Finger2 = currentCommand.Fingers.Finger2;
	pointToSend.Position.Fingers.Finger3 = currentCommand.Fingers.Finger3;

	int result = MySendAdvanceTrajectory(pointToSend);
	if (result != NO_ERROR_KINOVA)
	{
		OutputDebugString(L"Could not send advanced trajectory");
	}

	// scrape
	pointToSend.Position.CartesianPosition.Y = currentCommand.Coordinates.Y - 0.06f;
	result = MySendAdvanceTrajectory(pointToSend);
	if (result != NO_ERROR_KINOVA)
	{
		OutputDebugString(L"Could not send advanced trajectory");
	}

	// back up
	pointToSend.Position.CartesianPosition.Z = currentCommand.Coordinates.Z;
	pointToSend.Position.CartesianPosition.ThetaX = 1.8796;
	pointToSend.Position.CartesianPosition.ThetaY = 0.4309;
	pointToSend.Position.CartesianPosition.ThetaZ = -1.5505;
	result = MySendAdvanceTrajectory(pointToSend);
	if (result != NO_ERROR_KINOVA)
	{
		OutputDebugString(L"Could not send advanced trajectory");
	}

	Sleep(3000);
	MyEraseAllTrajectories();

	return 1;
}

int JacoArm::Soup()
{
	MyEraseAllTrajectories();
	CartesianPosition currentCommand;
	TrajectoryPoint pointToSend;
	pointToSend.InitStruct();

	//We specify that this point will be a Cartesian Position.
	pointToSend.Position.Type = CARTESIAN_POSITION;
	pointToSend.LimitationsActive = 0;

	MyGetCartesianCommand(currentCommand);


	pointToSend.Position.CartesianPosition.X = currentCommand.Coordinates.X;
	pointToSend.Position.CartesianPosition.Y = currentCommand.Coordinates.Y;
	pointToSend.Position.CartesianPosition.Z = currentCommand.Coordinates.Z - 0.08f;
	pointToSend.Position.CartesianPosition.ThetaX = currentCommand.Coordinates.ThetaX;
	pointToSend.Position.CartesianPosition.ThetaY = currentCommand.Coordinates.ThetaY;
	pointToSend.Position.CartesianPosition.ThetaZ = currentCommand.Coordinates.ThetaZ;
	pointToSend.Position.Fingers.Finger1 = currentCommand.Fingers.Finger1;
	pointToSend.Position.Fingers.Finger2 = currentCommand.Fingers.Finger2;
	pointToSend.Position.Fingers.Finger3 = currentCommand.Fingers.Finger3;

	int result = MySendAdvanceTrajectory(pointToSend);

	Sleep(1000);
	if (result != NO_ERROR_KINOVA)
	{
		OutputDebugString(L"Could not send advanced trajectory");
	}

	pointToSend.Position.CartesianPosition.Z = currentCommand.Coordinates.Z;

	result = MySendAdvanceTrajectory(pointToSend);
	if (result != NO_ERROR_KINOVA)
	{
		OutputDebugString(L"Could not send advanced trajectory");
	}

	Sleep(1000);
	pointToSend.Position.CartesianPosition.ThetaZ = -1.2264;

	result = MySendAdvanceTrajectory(pointToSend);
	if (result != NO_ERROR_KINOVA)
	{
		OutputDebugString(L"Could not send advanced trajectory");
	}

	Sleep(1000);
	pointToSend.Position.CartesianPosition.ThetaZ = currentCommand.Coordinates.ThetaZ;

	result = MySendAdvanceTrajectory(pointToSend);
	if (result != NO_ERROR_KINOVA)
	{
		OutputDebugString(L"Could not send advanced trajectory");
	}

	Sleep(3000);
	MyEraseAllTrajectories();

	return 1;
}


int JacoArm::WaitForArmMove(float goalX, float goalY, float goalZ)
{
	int timeout = 0;
	CartesianPosition current;
	MyGetCartesianCommand(current);
	while (ArmMoving(current.Coordinates.X, current.Coordinates.Y, current.Coordinates.Z, goalX, goalY, goalZ))
	{
		if (timeout >= 10)
			break;
		OutputDebugString(L"FaceBasics.cpp: Arm is moving\n");
		MyGetCartesianCommand(current);
		Sleep(1000);
		timeout++;
	}
	OutputDebugString(L"FaceBasics.cpp: Arm is stopped\n");
	return 0;
}

bool JacoArm::ArmMoving(float newX, float newY, float newZ, float goalX, float goalY, float goalZ)
{
	// 0.2 is the goal accuracy tolerance
	if ((abs(newX - goalX) < 0.1) && (abs(newY - goalY) < 0.1) && (abs(newZ - goalZ) < 0.1))
	{
		Sleep(1000);
		return false;
	}
	else
	{
#if TESTING 
		std::wstringstream s;
		s << L"Current Coords: " << newX << ", " << newY << ", " << newZ << "\n"
			<< L"Goal Coords: " << goalX << ", " << goalY << ", " << goalZ << "\n";
		std::wstring ws = s.str();
		LPCWSTR l = ws.c_str();
		OutputDebugString(l);
#endif
		return true;
	}
}

/// <summary>
/// Kinect coords to arm coords
/// </summary>
void JacoArm::KinectToArm(float kx, float ky, float kz, float* x, float* y, float* z)
{
	float ax, ay, az;
	ax = kx - x_offset;
	ay = ky - y_offset;
	az = kz - z_offset;

	*x = -az;
	*y = -ax;
	*z = ay;
}

