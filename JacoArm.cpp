#include "JacoArm.h"
#include "FaceBasics.h"
#include "stdafx.h"
#include <strsafe.h>
#include "resource.h"

// other
#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>

// Kinova API init
#include "SpeechBasics-D2D/SpeechBasics.h"

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
int(*MyGetGlobalTrajectoryInfo)(TrajectoryFIFO &);

JacoArm::JacoArm() 
{
	UpdateArPositions();

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
	MyGetGlobalTrajectoryInfo = (int(*)(TrajectoryFIFO &)) GetProcAddress(commandLayer_handle, "GetGlobalTrajectoryInfo");

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
			//MyMoveHome();
			MoveToNeutralPosition();
		}
	}
}

JacoArm::~JacoArm()
{
	// close arm api
	int result = (*MyCloseAPI)();
	FreeLibrary(commandLayer_handle);
}

int JacoArm::MoveToNeutralPosition()
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
	float y_offset_bowl = .3;

	KinectToArm(bowl_xpos+ BOWL_OFFSET_X, (bowl_ypos + BOWL_OFFSET_Y +  y_offset_bowl), bowl_zpos + BOWL_OFFSET_Z, &neutral_x, &neutral_y, &neutral_z);

	pointToSend.Position.CartesianPosition.X = neutral_x;
	pointToSend.Position.CartesianPosition.Y = neutral_y;
	pointToSend.Position.CartesianPosition.Z = neutral_z;
	pointToSend.Position.CartesianPosition.ThetaX = 2.1797;//1.8796;
	pointToSend.Position.CartesianPosition.ThetaY = -0.5404;//0.4309;
	pointToSend.Position.CartesianPosition.ThetaZ = -1.1281;//-1.5505;
	pointToSend.Position.Fingers.Finger1 = currentCommand.Fingers.Finger1;
	pointToSend.Position.Fingers.Finger2 = currentCommand.Fingers.Finger2;
	pointToSend.Position.Fingers.Finger3 = currentCommand.Fingers.Finger3;
	int rc = SendPoint(pointToSend);

	MyEraseAllTrajectories();

	return rc;
}


/// <summary>
/// Move arm to given position
/// </summary>
int JacoArm::MoveArm(float x, float y, float z)
{
	MoveToNeutralPosition();

 	MyEraseAllTrajectories();
	CartesianPosition currentCommand;
	TrajectoryPoint pointToSend;
	pointToSend.InitStruct();

	//We specify that this point will be a Cartesian Position.
	pointToSend.Position.Type = CARTESIAN_POSITION;
	pointToSend.LimitationsActive = 0;

	MyGetCartesianCommand(currentCommand);

	pointToSend.Position.CartesianPosition.X = x;
	pointToSend.Position.CartesianPosition.Y = y;
	pointToSend.Position.CartesianPosition.Z = z;
	pointToSend.Position.CartesianPosition.ThetaX = 2.1797;//1.8796;
	pointToSend.Position.CartesianPosition.ThetaY = -0.5404;//0.4309;
	pointToSend.Position.CartesianPosition.ThetaZ = -1.1281;//-1.5505;
	pointToSend.Position.Fingers.Finger1 = currentCommand.Fingers.Finger1;
	pointToSend.Position.Fingers.Finger2 = currentCommand.Fingers.Finger2;
	pointToSend.Position.Fingers.Finger3 = currentCommand.Fingers.Finger3;
	
	OutputDebugString(L"Sending the point to the robot.\n");
	int rc = SendPoint(pointToSend);

	MyEraseAllTrajectories();

	return rc;
}


/// <summary>
/// Scoop up food
/// </summary>

int JacoArm::Scoop()
{
	if (ActionsForJaco == ActionStop)
	{
		return false;
	}

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
	pointToSend.Position.CartesianPosition.Z = currentCommand.Coordinates.Z - .03f;
	pointToSend.Position.CartesianPosition.ThetaX = 2.4540;
	pointToSend.Position.CartesianPosition.ThetaY = -0.3986;
	pointToSend.Position.CartesianPosition.ThetaZ = -1.4348;
	pointToSend.Position.Fingers.Finger1 = currentCommand.Fingers.Finger1;
	pointToSend.Position.Fingers.Finger2 = currentCommand.Fingers.Finger2;
	pointToSend.Position.Fingers.Finger3 = currentCommand.Fingers.Finger3;
	SendPoint(pointToSend);

	// scrape
	pointToSend.Position.CartesianPosition.Y = currentCommand.Coordinates.Y - .07f;
	int rc = SendPoint(pointToSend);

	// back up
	pointToSend.Position.CartesianPosition.Z = currentCommand.Coordinates.Z;
	pointToSend.Position.CartesianPosition.ThetaX = 2.4540;
	pointToSend.Position.CartesianPosition.ThetaY = -0.3986;
	pointToSend.Position.CartesianPosition.ThetaZ = -1.4348;
	rc = SendPoint(pointToSend);
	MyEraseAllTrajectories();

	return rc;
}

int JacoArm::Soup()
{
	if (ActionsForJaco == ActionStop)
	{
		return false;
	}
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
	int rc = SendPoint(pointToSend);

	pointToSend.Position.CartesianPosition.Z = currentCommand.Coordinates.Z;
	rc = SendPoint(pointToSend);

	pointToSend.Position.CartesianPosition.ThetaZ = currentCommand.Coordinates.ThetaZ + .25;
	rc = SendPoint(pointToSend);

	pointToSend.Position.CartesianPosition.ThetaZ = currentCommand.Coordinates.ThetaZ;
	rc = SendPoint(pointToSend);
	MyEraseAllTrajectories();

	return rc;
}


int JacoArm::WaitForArmMove()
{
	Sleep(250);
	int timeout = 0;
	TrajectoryFIFO poses_buffer;
	MyGetGlobalTrajectoryInfo(poses_buffer);
	while (poses_buffer.TrajectoryCount > 0)
	{
		if (ActionsForJaco == ActionStop)
		{
			OutputDebugString(L"We stoppin");
			MyEraseAllTrajectories();
			return false;
		}

		if (timeout >= 40)
		{
			return false;
		}

		OutputDebugString(L"JacoArm.cpp: Arm is moving\n");
		MyGetGlobalTrajectoryInfo(poses_buffer);
		Sleep(250);
		timeout++;
	}
	OutputDebugString(L"JacoArm.cpp: Arm is stopped\n");
	return true;
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

int JacoArm::SendPoint(TrajectoryPoint pointToSend)
{
	int result = MySendAdvanceTrajectory(pointToSend);
	if (result != NO_ERROR_KINOVA)
	{
		OutputDebugString(L"Could not send advanced trajectory");
		return false;
	}

	int rc = WaitForArmMove();
	
	return rc;
}


void JacoArm::UpdateArPositions()
{
	tracker.GetARPosition(armVec, bowlVec);
	x_offset = -armVec[0];
	y_offset = -armVec[1];
	z_offset = armVec[2];

	std::wstringstream s;
	s << L"\noffset x: " << x_offset << "\n" << "offset y: " << y_offset << "\n" << "offset z: " << z_offset << "\n";
	std::wstring ws = s.str();
	LPCWSTR l = ws.c_str();
	OutputDebugString(l);

	bowl_xpos = -bowlVec[0];
	bowl_ypos = -bowlVec[1];
	bowl_zpos = bowlVec[2];

	std::wstringstream s1;
	s1 << L"\nbowl offset x: " << bowl_xpos << "\n" << "bowl offset y: " << bowl_ypos << "\n" << "bowl offset z: " << bowl_zpos << "\n";
	std::wstring ws1 = s1.str();
	LPCWSTR m = ws1.c_str();
	OutputDebugString(m);
}