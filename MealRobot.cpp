// Kinect Includes

#include "stdafx.h"
#include <strsafe.h>
#include "resource.h"
#include "MealRobot.h"

// Kinova Includes
#include <Windows.h>
#include "CommunicationLayerWindows.h"
#include "CommandLayer.h"
#include <conio.h>
#include "KinovaTypes.h"
#include <iostream>
#include <string>
#include <sstream>

// Kinova API init

// A handle to the API.
HINSTANCE commandLayer_handle;

// Function pointers to the functions we need
int(*MyInitAPI)();
int(*MyCloseAPI)();
int(*MySendBasicTrajectory)(TrajectoryPoint command);
int(*MyGetDevices)(KinovaDevice devices[MAX_KINOVA_DEVICE], int &result);
int(*MySetActiveDevice)(KinovaDevice device);
int(*MyMoveHome)();
int(*MyInitFingers)();
int(*MyGetCartesianCommand)(CartesianPosition &);

// Kinect consts, etc

// face property text layout offset in X axis
static const float c_FaceTextLayoutOffsetX = -0.1f;

// face property text layout offset in Y axis
static const float c_FaceTextLayoutOffsetY = -0.125f;

// define the face frame features required to be computed by this application
static const DWORD c_FaceFrameFeatures =
FaceFrameFeatures::FaceFrameFeatures_BoundingBoxInColorSpace
| FaceFrameFeatures::FaceFrameFeatures_PointsInColorSpace
| FaceFrameFeatures::FaceFrameFeatures_RotationOrientation
| FaceFrameFeatures::FaceFrameFeatures_Happy
| FaceFrameFeatures::FaceFrameFeatures_RightEyeClosed
| FaceFrameFeatures::FaceFrameFeatures_LeftEyeClosed
| FaceFrameFeatures::FaceFrameFeatures_MouthOpen
| FaceFrameFeatures::FaceFrameFeatures_MouthMoved
| FaceFrameFeatures::FaceFrameFeatures_LookingAway
| FaceFrameFeatures::FaceFrameFeatures_Glasses
| FaceFrameFeatures::FaceFrameFeatures_FaceEngagement;

/// <summary>
/// Entry point for the application
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="hPrevInstance">always 0</param>
/// <param name="lpCmdLine">command line arguments</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
/// <returns>status</returns>
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	CFaceBasics application;
	application.Run(hInstance, nCmdShow);
}

/// <summary>
/// Move arm to given position
/// </summary>
int MealRobot::MoveArm(float x, float y, float z)
{
	//We load the API.
	commandLayer_handle = LoadLibrary(L"CommandLayerWindows.dll");

	CartesianPosition currentCommand;

	int programResult = 0;

	//We load the functions from the library (Under Windows, use GetProcAddress)
	MyInitAPI = (int(*)()) GetProcAddress(commandLayer_handle, "InitAPI");
	MyCloseAPI = (int(*)()) GetProcAddress(commandLayer_handle, "CloseAPI");
	MyMoveHome = (int(*)()) GetProcAddress(commandLayer_handle, "MoveHome");
	MyInitFingers = (int(*)()) GetProcAddress(commandLayer_handle, "InitFingers");
	MyGetDevices = (int(*)(KinovaDevice devices[MAX_KINOVA_DEVICE], int &result)) GetProcAddress(commandLayer_handle, "GetDevices");
	MySetActiveDevice = (int(*)(KinovaDevice devices)) GetProcAddress(commandLayer_handle, "SetActiveDevice");
	MySendBasicTrajectory = (int(*)(TrajectoryPoint)) GetProcAddress(commandLayer_handle, "SendBasicTrajectory");
	MyGetCartesianCommand = (int(*)(CartesianPosition &)) GetProcAddress(commandLayer_handle, "GetCartesianCommand");

	//Verify that all functions has been loaded correctly
	if ((MyInitAPI == NULL) || (MyCloseAPI == NULL) || (MySendBasicTrajectory == NULL) ||
		(MyGetDevices == NULL) || (MySetActiveDevice == NULL) || (MyGetCartesianCommand == NULL) ||
		(MyMoveHome == NULL) || (MyInitFingers == NULL))

	{
		OutputDebugString(L"* * *  Error During Arm Initialization  * * *\n");
		programResult = 0;
	}
	else
	{
		OutputDebugString(L"Arm Initialization Complete\n\n");

		int result = (*MyInitAPI)();

		OutputDebugString(L"Initialization's result :");

		KinovaDevice list[MAX_KINOVA_DEVICE];

		int devicesCount = MyGetDevices(list, result);

		for (int i = 0; i < devicesCount; i++)
		{
			std::wstringstream s;
			s << L"Found a robot on the USB bus (" << list[i].SerialNumber << ")\n";
			std::wstring ws = s.str();
			LPCWSTR l = ws.c_str();
			OutputDebugString(l);

			//Setting the current device as the active device.
			MySetActiveDevice(list[i]);

			OutputDebugString(L"Send the robot to HOME position\n");
			MyMoveHome();

			OutputDebugString(L"Initializing the fingers\n");
			MyInitFingers();

			TrajectoryPoint pointToSend;
			pointToSend.InitStruct();

			//We specify that this point will be a Cartesian Position.
			pointToSend.Position.Type = CARTESIAN_POSITION;

			//We get the actual cartesian command of the robot. (Use this for relative position)
			//MyGetCartesianCommand(currentCommand);

			//pointToSend.Position.CartesianPosition.X = currentCommand.Coordinates.X - 0.1f;
			//pointToSend.Position.CartesianPosition.Y = currentCommand.Coordinates.Y - 0.1f;
			//pointToSend.Position.CartesianPosition.Z = currentCommand.Coordinates.Z - 0.1f;
			pointToSend.Position.CartesianPosition.X = x;
			pointToSend.Position.CartesianPosition.Y = y;
			pointToSend.Position.CartesianPosition.Z = z;
			pointToSend.Position.CartesianPosition.ThetaX = currentCommand.Coordinates.ThetaX;
			pointToSend.Position.CartesianPosition.ThetaY = currentCommand.Coordinates.ThetaY;
			pointToSend.Position.CartesianPosition.ThetaZ = currentCommand.Coordinates.ThetaZ;

			OutputDebugString(L"Sending the point to the robot.\n");
			MySendBasicTrajectory(pointToSend);
		}

		OutputDebugString(L"Closing Arm API\n");
		result = (*MyCloseAPI)();
		programResult = 1;
	}

	FreeLibrary(commandLayer_handle);

	return programResult;
}
