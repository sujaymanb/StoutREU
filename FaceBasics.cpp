//------------------------------------------------------------------------------
// <copyright file="FaceBasics.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


#include "stdafx.h"
#include <strsafe.h>
#include "resource.h"
#include "FaceBasics.h"
#include "SpeechBasics-D2D/SpeechBasics.h"
#include "ArTracker.h"

// Kinova Includes
#include "CommunicationLayerWindows.h"
#include "CommandLayer.h"
#include <conio.h>
#include "KinovaTypes.h"
#include "JacoArm.h"

// other
#include <iostream>
#include <string>
#include <sstream>
#include <conio.h>
#include <cstdlib>

//test
#include "stdafx.h"
#include <strsafe.h>
#include <math.h>
#include <limits>
#include <Wincodec.h>
#include "resource.h"


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
/// Constructor
/// </summary>
CFaceBasics::CFaceBasics() :
    m_hWnd(NULL),
    m_nStartTime(0),
    m_nLastCounter(0),
    m_nFramesSinceUpdate(0),
    m_fFreq(0),
    m_nNextStatusTime(0),
    m_pCoordinateMapper(nullptr),
    m_pColorFrameReader(nullptr),
    m_pD2DFactory(nullptr),
    m_pDrawDataStreams(nullptr),
    m_pColorRGBX(nullptr),
    m_pBodyFrameReader(nullptr),	
	cameraSpacePoints(nullptr),
	m_pDepthCoordinates(nullptr)
{
	pDepthBuffer = nullptr;
    LARGE_INTEGER qpf = {0};
    if (QueryPerformanceFrequency(&qpf))
    {
        m_fFreq = double(qpf.QuadPart);
    }

    for (int i = 0; i < BODY_COUNT; i++)
    {
        m_pFaceFrameSources[i] = nullptr;
        m_pFaceFrameReaders[i] = nullptr;
    }

    // create heap storage for color pixel data in RGBX format
    m_pColorRGBX = new RGBQUAD[cColorWidth * cColorHeight];

	// create heap storage for the coorinate mapping from color to depth
	m_pDepthCoordinates = new DepthSpacePoint[cColorWidth * cColorHeight];

	cameraSpacePoints = new CameraSpacePoint[cColorWidth * cColorHeight];
	for (int i = 0; i < (cColorWidth * cColorHeight); i++) {
		cameraSpacePoints[i].X = 0;
		cameraSpacePoints[i].Y = 0;
		cameraSpacePoints[i].Z = 0;
	}
	// init vars
	armMovingFlag = false;
	int mouthOpenCounter[BODY_COUNT] = { 0 };
	int eyesClosedCounter[BODY_COUNT] = { 0 };

}


/// <summary>
/// Destructor
/// </summary>
CFaceBasics::~CFaceBasics()
{
    // clean up Direct2D renderer
    if (m_pDrawDataStreams)
    {
        delete m_pDrawDataStreams;
        m_pDrawDataStreams = nullptr;
    }

    if (m_pColorRGBX)
    {
        delete [] m_pColorRGBX;
        m_pColorRGBX = nullptr;
    }

    // clean up Direct2D
    SafeRelease(m_pD2DFactory);

    // done with face sources and readers
    for (int i = 0; i < BODY_COUNT; i++)
    {
        SafeRelease(m_pFaceFrameSources[i]);
        SafeRelease(m_pFaceFrameReaders[i]);		
    }

    // done with body frame reader
    SafeRelease(m_pBodyFrameReader);

    // done with color frame reader
    SafeRelease(m_pColorFrameReader);

    // done with coordinate mapper
    SafeRelease(m_pCoordinateMapper);

    // close the Kinect Sensor
    if (m_pKinectSensor)
    {
        m_pKinectSensor->Close();
    }

    SafeRelease(m_pKinectSensor);

	
}

/// <summary>
/// Creates the main window and begins processing
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
int CFaceBasics::Run(HINSTANCE hInstance, int nCmdShow, JacoArm& arm)
{
    MSG       msg = {0};
    WNDCLASS  wc;

    // Dialog custom window class
    ZeroMemory(&wc, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.cbWndExtra    = DLGWINDOWEXTRA;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP));
    wc.lpfnWndProc   = DefDlgProcW;
    wc.lpszClassName = L"FaceBasicsAppDlgWndClass";

    if (!RegisterClassW(&wc))
    {
        return FAILURE;
    }

    // Create main application window
    HWND hWndApp = CreateDialogParamW(
        NULL,
        MAKEINTRESOURCE(IDD_APP),
        NULL,
        (DLGPROC)CFaceBasics::MessageRouter, 
        reinterpret_cast<LPARAM>(this));

    // Show window
    ShowWindow(hWndApp, nCmdShow);

    // Main message loop
    while (WM_QUIT != msg.message)
    {
        Update(arm);
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            // If a dialog message will be taken care of by the dialog proc
            if (hWndApp && IsDialogMessageW(hWndApp, &msg))
            {
                continue;
            }

            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}

/// <summary>
/// Handles window messages, passes most to the class instance to handle
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CFaceBasics::MessageRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CFaceBasics* pThis = nullptr;

    if (WM_INITDIALOG == uMsg)
    {
        pThis = reinterpret_cast<CFaceBasics*>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<CFaceBasics*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis)
    {
        return pThis->DlgProc(hWnd, uMsg, wParam, lParam);
    }

    return FAILURE;
}

/// <summary>
/// Handle windows messages for the class instance
/// </summary>
/// <param name="hWnd">window message is for</param>
/// <param name="uMsg">message</param>
/// <param name="wParam">message data</param>
/// <param name="lParam">additional message data</param>
/// <returns>result of message processing</returns>
LRESULT CALLBACK CFaceBasics::DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Bind application window handle
            m_hWnd = hWnd;

            // Init Direct2D
            D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

            // Create and initialize a new Direct2D image renderer (take a look at ImageRenderer.h)
            // We'll use this to draw the data we receive from the Kinect to the screen
            m_pDrawDataStreams = new ImageRenderer();
            HRESULT hr = m_pDrawDataStreams->Initialize(GetDlgItem(m_hWnd, IDC_VIDEOVIEW), m_pD2DFactory, cColorWidth, cColorHeight, cColorWidth * sizeof(RGBQUAD)); 
            if (FAILED(hr))
            {
                SetStatusMessage(L"Failed to initialize the Direct2D draw device.", 10000, true);
            }

            // Get and initialize the default Kinect sensor
            InitializeDefaultSensor();
        }
        break;

        // If the titlebar X is clicked, destroy app
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;

    case WM_DESTROY:
        // Quit the main message pump
        PostQuitMessage(0);
        break;        
    }

    return FALSE;
}


/// <summary>
/// Initializes the default Kinect sensor
/// </summary>
/// <returns>indicates success or failure</returns>
HRESULT CFaceBasics::InitializeDefaultSensor()
{
	HRESULT hr;

	hr = GetDefaultKinectSensor(&m_pKinectSensor);
	if (FAILED(hr))
	{
		return hr;
	}
	IBodyFrameSource* pBodyFrameSource = nullptr;

	if (m_pKinectSensor)
	{
		// Initialize the Kinect and get coordinate mapper and the frame reader

		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_CoordinateMapper(&m_pCoordinateMapper);
		}

		hr = m_pKinectSensor->Open();

		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->get_BodyFrameSource(&pBodyFrameSource);
		}

		if (SUCCEEDED(hr))
		{
			hr = pBodyFrameSource->OpenReader(&m_pBodyFrameReader);
		}

		if (SUCCEEDED(hr))
		{
			hr = m_pKinectSensor->OpenMultiSourceFrameReader(
				FrameSourceTypes::FrameSourceTypes_Depth | FrameSourceTypes::FrameSourceTypes_Color | FrameSourceTypes::FrameSourceTypes_BodyIndex,
				&m_pMultiSourceFrameReader);
		}

		if (SUCCEEDED(hr))
		{
			// create a face frame source + reader to track each body in the fov
			for (int i = 0; i < BODY_COUNT; i++)
			{
				if (SUCCEEDED(hr))
				{
					// create the face frame source by specifying the required face frame features
					hr = CreateFaceFrameSource(m_pKinectSensor, 0, c_FaceFrameFeatures, &m_pFaceFrameSources[i]);
				}
				if (SUCCEEDED(hr))
				{
					// open the corresponding reader
					hr = m_pFaceFrameSources[i]->OpenReader(&m_pFaceFrameReaders[i]);
				}
			}
		}
	}

	if (!m_pKinectSensor || FAILED(hr))
	{
		SetStatusMessage(L"No ready Kinect found!", 10000, true);
		return E_FAIL;
	}

	return hr;
}

/// <summary>
/// Get the goal position
/// </summary>
HRESULT CFaceBasics::GetMouthPosition(IBody* pBody, CameraSpacePoint* mouthPosition, PointF facePoints[])
{
	CameraSpacePoint headPos;
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
				CameraSpacePoint headJoint = joints[JointType_Head].Position;

				// set offsets here if needed
				// for now just returns head position
				headPos.X = headJoint.X + MOUTH_OFFSET_X;
				headPos.Y = headJoint.Y + MOUTH_OFFSET_Y;
				headPos.Z = headJoint.Z + MOUTH_OFFSET_Z;
			}
		} 
	}

	CameraSpacePoint mouthPos;

	ColorSpacePoint leftMouthCorner{ facePoints[FacePointType_MouthCornerRight].X, facePoints[FacePointType_MouthCornerRight].Y };
	
	hr = MapColorToCameraCoordinates(leftMouthCorner, mouthPos);

	if (SUCCEEDED(hr))
	{
		mouthPosition->X = mouthPos.X + MOUTH_OFFSET_X;
		mouthPosition->Y = mouthPos.Y + MOUTH_OFFSET_Y;
		mouthPosition->Z = mouthPos.Z + MOUTH_OFFSET_Z;
	}

	return hr;
}

/// <summary>
/// Main processing function
/// </summary>
void CFaceBasics::Update(JacoArm& arm)
{
	if (!m_pMultiSourceFrameReader || !m_pBodyFrameReader)
	{
		return;
	}

	static IMultiSourceFrame* pMultiSourceFrame = nullptr;
	IDepthFrame* pDepthFrame = nullptr;
	IColorFrame* pColorFrame = nullptr;
	IBodyIndexFrame* pBodyIndexFrame = nullptr;

	// Get color frame

	HRESULT hr = m_pMultiSourceFrameReader->AcquireLatestFrame(&pMultiSourceFrame);
	if (SUCCEEDED(hr))
	{
		IColorFrameReference* pColorFrameReference = nullptr;

		hr = pMultiSourceFrame->get_ColorFrameReference(&pColorFrameReference);
		if (SUCCEEDED(hr))
		{
			hr = pColorFrameReference->AcquireFrame(&pColorFrame);
		}

		SafeRelease(pColorFrameReference);
		//IColorFrame* pColorFrame = nullptr;

		IFrameDescription* pColorFrameDescription = NULL;
		int nColorWidth = 0;
		int nColorHeight = 0;
		ColorImageFormat imageFormat = ColorImageFormat_None;
		UINT nColorBufferSize = 0;
		RGBQUAD *pColorBuffer = nullptr;
		INT64 nTime = 0;

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_RelativeTime(&nTime);
		}

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_FrameDescription(&pColorFrameDescription);
		}

		if (SUCCEEDED(hr))
		{
			hr = pColorFrameDescription->get_Width(&nColorWidth);
		}

		if (SUCCEEDED(hr))
		{
			hr = pColorFrameDescription->get_Height(&nColorHeight);
		}

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
		}

		if (SUCCEEDED(hr))
		{
			if (imageFormat == ColorImageFormat_Bgra)
			{
				hr = pColorFrame->AccessRawUnderlyingBuffer(&nColorBufferSize, reinterpret_cast<BYTE**>(&pColorBuffer));
			}
			else if (m_pColorRGBX)
			{
				pColorBuffer = m_pColorRGBX;
				nColorBufferSize = cColorWidth * cColorHeight * sizeof(RGBQUAD);
				hr = pColorFrame->CopyConvertedFrameDataToArray(nColorBufferSize, reinterpret_cast<BYTE*>(pColorBuffer), ColorImageFormat_Bgra);
			}
			else
			{
				hr = E_FAIL;
			}
		}

		// Get Depth data

		if (SUCCEEDED(hr))
		{
			IDepthFrameReference* pDepthFrameReference = NULL;

			hr = pMultiSourceFrame->get_DepthFrameReference(&pDepthFrameReference);
			if (SUCCEEDED(hr))
			{
				hr = pDepthFrameReference->AcquireFrame(&pDepthFrame);
			}

			SafeRelease(pDepthFrameReference);
		}

		INT64 nDepthTime = 0;
		IFrameDescription* pDepthFrameDescription = NULL;
		UINT nDepthBufferSize = 0;
		// depth frame data
		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->get_FrameDescription(&pDepthFrameDescription);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameDescription->get_Width(&nDepthWidth);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrameDescription->get_Height(&nDepthHeight);
		}

		if (SUCCEEDED(hr))
		{
			hr = pDepthFrame->AccessUnderlyingBuffer(&nDepthBufferSize, &pDepthBuffer);
		}

		// Draw image on window
		if (SUCCEEDED(hr))
		{
			DrawStreams(nTime, pColorBuffer, nColorWidth, nColorHeight, arm);
		}

		//SafeRelease(pBodyIndexFrameDescription);
		SafeRelease(pColorFrameDescription);
		SafeRelease(pDepthFrameDescription);
	}
	
	SafeRelease(pDepthFrame);
    SafeRelease(pColorFrame);
	SafeRelease(pBodyIndexFrame);
	SafeRelease(pMultiSourceFrame);
}


/// <summary>
/// Renders the color and face streams
/// </summary>
/// <param name="nTime">timestamp of frame</param>
/// <param name="pBuffer">pointer to frame data</param>
/// <param name="nWidth">width (in pixels) of input image data</param>
/// <param name="nHeight">height (in pixels) of input image data</param>
void CFaceBasics::DrawStreams(INT64 nTime, RGBQUAD* pBuffer, int nWidth, int nHeight, JacoArm& arm)
{
    if (m_hWnd)
    {
        HRESULT hr;
        hr = m_pDrawDataStreams->BeginDrawing();

        if (SUCCEEDED(hr))
        {
            // Make sure we've received valid color data
            if (pBuffer && (nWidth == cColorWidth) && (nHeight == cColorHeight))
            {
                // Draw the data with Direct2D
                hr = m_pDrawDataStreams->DrawBackground(reinterpret_cast<BYTE*>(pBuffer), cColorWidth * cColorHeight * sizeof(RGBQUAD));        
            }
            else
            {
                // Recieved invalid data, stop drawing
                hr = E_INVALIDARG;
            }

            if (SUCCEEDED(hr))
            {
                // begin processing the face frames
                ProcessFaces(arm);				
            }

            m_pDrawDataStreams->EndDrawing();
        }

        if (!m_nStartTime)
        {
            m_nStartTime = nTime;
        }

        double fps = 0.0;

        LARGE_INTEGER qpcNow = {0};
        if (m_fFreq)
        {
            if (QueryPerformanceCounter(&qpcNow))
            {
                if (m_nLastCounter)
                {
                    m_nFramesSinceUpdate++;
                    fps = m_fFreq * m_nFramesSinceUpdate / double(qpcNow.QuadPart - m_nLastCounter);
                }
            }
        }

        WCHAR szStatusMessage[64];
        StringCchPrintf(szStatusMessage, _countof(szStatusMessage), L" FPS = %0.2f    Time = %I64d", fps, (nTime - m_nStartTime));

        if (SetStatusMessage(szStatusMessage, 1000, false))
        {
            m_nLastCounter = qpcNow.QuadPart;
            m_nFramesSinceUpdate = 0;
        }
    }    
}

/// <summary>
/// Processes new face frames
/// </summary>
void CFaceBasics::ProcessFaces(JacoArm& arm)
{
	HRESULT hr;
	IBody* ppBodies[BODY_COUNT] = { 0 };
	CameraSpacePoint mouthPoints[BODY_COUNT];
	for (int i = 0; i < BODY_COUNT; ++i)
	{
		mouthPoints[i].X = 999.0;
		mouthPoints[i].Y = 999.0;
		mouthPoints[i].Z = 999.0;
	}

	bool bHaveBodyData = SUCCEEDED(UpdateBodyData(ppBodies));

	// iterate through each face reader
	for (int iFace = 0; iFace < BODY_COUNT; ++iFace)
	{
		// retrieve the latest face frame from this reader
		IFaceFrame* pFaceFrame = nullptr;
		hr = m_pFaceFrameReaders[iFace]->AcquireLatestFrame(&pFaceFrame);

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

						// update the distance

						InterpretSpeechAndGestures(faceProperties, mouthPoints, iFace, arm, ppBodies[iFace], facePoints);
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

/// <summary>
/// Computes the face result text position by adding an offset to the corresponding 
/// body's head joint in camera space and then by projecting it to screen space
/// </summary>
/// <param name="pBody">pointer to the body data</param>
/// <param name="pFaceTextLayout">pointer to the text layout position in screen space</param>
/// <returns>indicates success or failure</returns>
HRESULT CFaceBasics::GetFaceTextPositionInColorSpace(IBody* pBody, D2D1_POINT_2F* pFaceTextLayout)
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
                CameraSpacePoint headJoint = joints[JointType_Head].Position;
                CameraSpacePoint textPoint = 
                {
                    headJoint.X + c_FaceTextLayoutOffsetX,
                    headJoint.Y + c_FaceTextLayoutOffsetY,
                    headJoint.Z
                };

                ColorSpacePoint colorPoint = {0};
                hr = m_pCoordinateMapper->MapCameraPointToColorSpace(textPoint, &colorPoint);
			

                if (SUCCEEDED(hr))
                {
                    pFaceTextLayout->x = colorPoint.X;
                    pFaceTextLayout->y = colorPoint.Y;
                }
            }
        }
    }

    return hr;
}

/// <summary>
/// Updates body data
/// </summary>
/// <param name="ppBodies">pointer to the body data storage</param>
/// <returns>indicates success or failure</returns>
HRESULT CFaceBasics::UpdateBodyData(IBody** ppBodies)
{
    HRESULT hr = E_FAIL;

    if (m_pBodyFrameReader != nullptr)
    {
        IBodyFrame* pBodyFrame = nullptr;
        hr = m_pBodyFrameReader->AcquireLatestFrame(&pBodyFrame);
        if (SUCCEEDED(hr))
        {
            hr = pBodyFrame->GetAndRefreshBodyData(BODY_COUNT, ppBodies);
        }
        SafeRelease(pBodyFrame);    
    }

    return hr;
}

/// <summary>
/// Set the status bar message
/// </summary>
/// <param name="szMessage">message to display</param>
/// <param name="showTimeMsec">time in milliseconds to ignore future status messages</param>
/// <param name="bForce">force status update</param>
/// <returns>success or failure</returns>
bool CFaceBasics::SetStatusMessage(_In_z_ WCHAR* szMessage, ULONGLONG nShowTimeMsec, bool bForce)
{
    ULONGLONG now = GetTickCount64();

    if (m_hWnd && (bForce || (m_nNextStatusTime <= now)))
    {
        SetDlgItemText(m_hWnd, IDC_STATUS, szMessage);
        m_nNextStatusTime = now + nShowTimeMsec;

        return true;
    }

    return false;
}


void CFaceBasics::InterpretSpeechAndGestures(DetectionResult faceProperties[], CameraSpacePoint mouthPoints[], int iFace, JacoArm& arm, IBody* pBody, PointF facePoints[])
{
	static EatingMode mode = SoupMode;
	static State armState = WaitForEyesClosed;
	static float min = 999.0;
	static int iFaceMin;

	for (int i = 0; i < BODY_COUNT; ++i)
	{
		if (mouthPoints[i].Z < min)
		{
			iFaceMin = i;
			min = mouthPoints[i].Z;
		}
	}

	// mode switches
	if (ActionsForJaco == ActionScoop)
	{
		mode = ScoopMode;
	}
	else if (ActionsForJaco == ActionSoup)
	{
		mode = SoupMode;
	}

	if (ActionsForJaco == ActionStop)
	{
		armState = StopAllMovement;
	}

	if (armState == WaitForEyesClosed)
	{
		// If min face and if one of the following is true:
		// both eyes closed
		// 'Bowl' is said by user
		if (((faceProperties[FaceProperty_LeftEyeClosed] == DetectionResult_Yes
			|| faceProperties[FaceProperty_RightEyeClosed] == DetectionResult_Yes) || ActionsForJaco == ActionBowl)
			&& mouthPoints[iFace].Z <= mouthPoints[iFaceMin].Z)
		{
			eyesClosedCounter[iFace]++;
			if (eyesClosedCounter[iFace] >= 10 || ActionsForJaco == ActionBowl)
			{
				eyesClosedCounter[iFace] = 0;

				armState = ArmMovingTowardBowl;
			}
			if (ActionsForJaco != ActionNone && ActionsForJaco != ActionStop)
			{
				ActionsForJaco = ActionNone;
			}
		}
		else
		{
			eyesClosedCounter[iFace] = 0;
		}
	}
	else if (armState == ArmMovingTowardBowl)
	{
		float x, y, z;
		// go to plate, position hard coded for now
		arm.UpdateArPositions();
		arm.KinectToArm(arm.bowl_xpos + BOWL_OFFSET_X, arm.bowl_ypos + BOWL_OFFSET_Y, arm.bowl_zpos + BOWL_OFFSET_Z, &x, &y, &z);
		arm.MoveArm(x, y, z);

		// pick up food
#if TESTING
		if (mode == ScoopMode)
		{
			arm.Scoop();
			OutputDebugString(L"\nIn Scoop Mode\n");
		}
		else if (mode == SoupMode)
		{
			arm.Soup();
			OutputDebugString(L"\nIn Soup Mode\n");
		}
#endif	
		arm.MoveToNeutralPosition();
		armState = WaitForMouthOpen;
	}
	else if (armState == WaitForMouthOpen)
	{
		// check if mouth is open
		if ((faceProperties[FaceProperty_MouthOpen] == DetectionResult_Yes || ActionsForJaco == ActionFood) && mouthPoints[iFace].Z <= mouthPoints[iFaceMin].Z)
		{
			mouthOpenCounter[iFace]++;
			if (mouthOpenCounter[iFace] >= 10 || ActionsForJaco == ActionFood)
			{
				mouthOpenCounter[iFace] = 0;
				armState = ArmMovingTowardMouth;
			}
			if (ActionsForJaco != ActionNone && ActionsForJaco != ActionStop)
			{
				ActionsForJaco = ActionNone;
			}
		}
		else
		{
			mouthOpenCounter[iFace] = 0;
		}
	}
	else if (armState == ArmMovingTowardMouth)
	{
		// send move command
		// using dummy coordinates for now					
		int armResult;
		float x, y, z;

		arm.UpdateArPositions();
		GetMouthPosition(pBody, &mouthPoints[iFace], facePoints);
		arm.KinectToArm(mouthPoints[iFace].X, mouthPoints[iFace].Y, mouthPoints[iFace].Z, &x, &y, &z);
		//armResult = MoveArm(0.3248, 0.45, 0.1672);

		armResult = arm.MoveArm(x, y, z);

		armState = WaitForEyesClosed;
	}
	else // if(armState == StopAllMovement
	{
		if (ActionsForJaco == ActionReset)
		{
			arm.UpdateArPositions();
			arm.MoveToNeutralPosition();
			armState = WaitForEyesClosed;
		}
	}

#if TESTING
	//state stuff end
	std::wstringstream s;
	s << L"Goal Coords: " << mouthPoints[iFace].X << ", " << mouthPoints[iFace].Y << ", " << mouthPoints[iFace].Z << "\n"
		<< mouthOpenCounter[iFace] << " Min: " << mouthPoints[iFaceMin].Z << "\n";
	std::wstring ws = s.str();
	LPCWSTR l = ws.c_str();
	//OutputDebugString(l);
#endif
}


HRESULT CFaceBasics::MapColorToCameraCoordinates(const ColorSpacePoint& colorsps, CameraSpacePoint& camerasps)
{
	UINT16* depthImageBuffer = nullptr;
	//Access frame

	HRESULT hr = 1;

	if (SUCCEEDED(hr))
	{
		if (pDepthBuffer &&  cameraSpacePoints && (nDepthWidth == cDepthWidth) && (nDepthHeight == cDepthHeight))
		{	
			hr = m_pCoordinateMapper->MapColorFrameToCameraSpace((nDepthWidth * nDepthHeight), (const UINT16*)pDepthBuffer, (cColorWidth * cColorHeight), cameraSpacePoints);
			if (SUCCEEDED(hr))
			{
				int colorX = static_cast<int>(colorsps.X + 0.5f);
				int colorY = static_cast<int>(colorsps.Y + 0.5f);
				long colorIndex = (long)(colorY * cColorWidth + colorX);
				CameraSpacePoint csp = cameraSpacePoints[colorIndex];
				camerasps = CameraSpacePoint{ csp.X, csp.Y, csp.Z };
			}
			//delete[] cameraSpacePoints;
		}
		else
		{
			hr = E_FAIL;
		}
		
	}
	return hr;
}