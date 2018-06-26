/*
// Kinect Includes

#include "stdafx.h"
#include <strsafe.h>
#include "resource.h"
#include "MealRobot.h"

// Kinova Includes
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

	MealRobot application;
	application.Run(hInstance, nCmdShow);
}


/// <summary>
/// Get the goal position
/// </summary>
HRESULT MealRobot::GetMouthPosition(IBody* pBody, CameraSpacePoint* mouthPosition)
{
	HRESULT hr = E_FAIL;

	if (pBody != nullptr)
	{
		BOOLEAN bTracked = false;
		hr = pBody->get_IsTracked(&bTracked);

		if (SUCCEEDED(hr) && bTracked)
		{
			Joint joints[JointType_Count];
			hr = pBody->GetJoints(_countof(joints), joints);
			if (SUCCEEDED(hr))
			{
				CameraSpacePoint neckJoint = joints[JointType_Neck].Position;
				
				// set offsets here if needed
				// for now just returns neck position
				mouthPosition->X = neckJoint.X;
				mouthPosition->Y = neckJoint.Y;
				mouthPosition->Z = neckJoint.Z;
			}
		}
	}

	return hr;
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

			TrajectoryPoint pointToSend;
			pointToSend.InitStruct();

			//We specify that this point will be a Cartesian Position.
			pointToSend.Position.Type = CARTESIAN_POSITION;

			//We get the actual cartesian command of the robot. (Use this for relative position)
			MyGetCartesianCommand(currentCommand);

			//pointToSend.Position.CartesianPosition.X = currentCommand.Coordinates.X - 0.1f;
			//pointToSend.Position.CartesianPosition.Y = currentCommand.Coordinates.Y - 0.1f;
			//pointToSend.Position.CartesianPosition.Z = currentCommand.Coordinates.Z - 0.1f;
			pointToSend.Position.CartesianPosition.X = x;
			pointToSend.Position.CartesianPosition.Y = y;
			pointToSend.Position.CartesianPosition.Z = z;
			pointToSend.Position.CartesianPosition.ThetaX = currentCommand.Coordinates.ThetaX;
			pointToSend.Position.CartesianPosition.ThetaY = currentCommand.Coordinates.ThetaY;
			pointToSend.Position.CartesianPosition.ThetaZ = currentCommand.Coordinates.ThetaZ;
			pointToSend.Position.Fingers.Finger1 = currentCommand.Fingers.Finger1;
			pointToSend.Position.Fingers.Finger2 = currentCommand.Fingers.Finger2;
			pointToSend.Position.Fingers.Finger3 = currentCommand.Fingers.Finger3;

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

/// <summary>
/// Processes new face frames (Override)
/// </summary>
void MealRobot::ProcessFaces()
{
	HRESULT hr;
	IBody* ppBodies[BODY_COUNT] = { 0 };
	CameraSpacePoint* mouthPoints[BODY_COUNT];
	for (int i = 0; i < BODY_COUNT; ++i)
	{
		mouthPoints[i]->X = 999.0;
		mouthPoints[i]->Y = 999.0;
		mouthPoints[i]->Z = 999.0;
	}
	int iFaceMin = 0;	// current min face index
	bool bHaveBodyData = SUCCEEDED(UpdateBodyData(ppBodies));
	int mouthOpenCounter = 0;

	// iterate through each face reader
	for (int iFace = 0; iFace < BODY_COUNT; ++iFace)
	{
		// retrieve the latest face frame from this reader
		IFaceFrame* pFaceFrame = nullptr;
		hr = m_pFaceFrameReaders[iFace]->AcquireLatestFrame(&pFaceFrame);
		float min = 999.0;

		BOOLEAN bFaceTracked = false;
		if (SUCCEEDED(hr) && nullptr != pFaceFrame)
		{
			// check if a valid face is tracked in this face frame
			hr = pFaceFrame->get_IsTrackingIdValid(&bFaceTracked);
		}

		if (SUCCEEDED(hr))
		{
			if (bFaceTracked)
			{
				IFaceFrameResult* pFaceFrameResult = nullptr;
				RectI faceBox = { 0 };
				PointF facePoints[FacePointType::FacePointType_Count];
				Vector4 faceRotation;
				DetectionResult faceProperties[FaceProperty::FaceProperty_Count];
				D2D1_POINT_2F faceTextLayout;

				hr = pFaceFrame->get_FaceFrameResult(&pFaceFrameResult);

				// need to verify if pFaceFrameResult contains data before trying to access it
				if (SUCCEEDED(hr) && pFaceFrameResult != nullptr)
				{
					hr = pFaceFrameResult->get_FaceBoundingBoxInColorSpace(&faceBox);

					if (SUCCEEDED(hr))
					{
						hr = pFaceFrameResult->GetFacePointsInColorSpace(FacePointType::FacePointType_Count, facePoints);
					}

					if (SUCCEEDED(hr))
					{
						hr = pFaceFrameResult->get_FaceRotationQuaternion(&faceRotation);
					}

					if (SUCCEEDED(hr))
					{
						hr = pFaceFrameResult->GetFaceProperties(FaceProperty::FaceProperty_Count, faceProperties);
					}

					if (SUCCEEDED(hr))
					{
						hr = GetFaceTextPositionInColorSpace(ppBodies[iFace], &faceTextLayout);
					}

					if (SUCCEEDED(hr))
					{
						// draw face frame results
						m_pDrawDataStreams->DrawFaceFrameResults(iFace, &faceBox, facePoints, &faceRotation, faceProperties, &faceTextLayout);

						// update the distances
						hr = MealRobot::GetMouthPosition(ppBodies[iFace], mouthPoints[iFace]);

						for (int i = 0; i < BODY_COUNT; ++i)
						{
							if (mouthPoints[i]->Z < min)
							{
								iFaceMin = i;
								min = mouthPoints[i]->Z;
							}
						}

						std::wstringstream s;
						s << L"Goal Coords: " << mouthPoints[iFace]->X << ", " << mouthPoints[iFace]->Y << ", " << mouthPoints[iFace]->Z << "\n" << mouthOpenCounter << "\n";
						std::wstring ws = s.str();
						LPCWSTR l = ws.c_str();
						OutputDebugString(l);
						OutputDebugString(L"WHY");

						// check if mouth is open
						if (faceProperties[FaceProperty_MouthOpen] == DetectionResult_Yes && mouthPoints[iFace]->Z <= mouthPoints[iFaceMin]->Z)
						{
							mouthOpenCounter++;
							if (mouthOpenCounter == 30)
							{
								// send move command
								// using dummy coordinates for now
								int armResult;
								armResult = MoveArm(0.3248, 0.45, 0.1672);
								if (armResult == 1)
								{
									OutputDebugString(L"Arm Moved Successfully");
								}
							}
						}
						else
						{
							mouthOpenCounter = 0;
						}
					}
				}

				SafeRelease(pFaceFrameResult);
			}
			else
			{
				// face tracking is not valid - attempt to fix the issue
				// a valid body is required to perform this step
				if (bHaveBodyData)
				{
					// check if the corresponding body is tracked 
					// if this is true then update the face frame source to track this body
					IBody* pBody = ppBodies[iFace];
					if (pBody != nullptr)
					{
						BOOLEAN bTracked = false;
						hr = pBody->get_IsTracked(&bTracked);

						UINT64 bodyTId;
						if (SUCCEEDED(hr) && bTracked)
						{
							// get the tracking ID of this body
							hr = pBody->get_TrackingId(&bodyTId);
							if (SUCCEEDED(hr))
							{
								// update the face frame source with the tracking ID
								m_pFaceFrameSources[iFace]->put_TrackingId(bodyTId);
							}
						}
					}
				}
			}
		}

		SafeRelease(pFaceFrame);
	}

	if (bHaveBodyData)
	{
		for (int i = 0; i < _countof(ppBodies); ++i)
		{
			SafeRelease(ppBodies[i]);
		}
	}
}

*/