#include <iostream>
#include <thread>
#include "FaceBasics.h"
#include "SpeechBasics-D2D/SpeechBasics.h"
#include "stdafx.h"
#define TESTING 0


#define INITGUID
#include <guiddef.h>

// Static initializers
//LPCWSTR CSpeechBasics::GrammarFileName = L"SpeechBasics-D2D/SpeechBasics-D2D.grxml";

// This is the class ID we expect for the Microsoft Speech recognizer.
// Other values indicate that we're using a version of sapi.h that is
// incompatible with this sample.
DEFINE_GUID(CLSID_ExpectedRecognizer, 0x495648e7, 0xf7ab, 0x4267, 0x8e, 0x0f, 0xca, 0xfb, 0x7a, 0x33, 0xc1, 0x60);


using namespace std;


void FaceBasicsThread(HINSTANCE hInstance, int nCmdShow)
{
#if TESTING
	while (1)
	{
		OutputDebugString(L"Hello World\n");
		Sleep(10000);
	}
#else
	CFaceBasics application;
	application.Run(hInstance, nCmdShow);
#endif
}

int SpeechRecognizerThread(HINSTANCE hInstance, int nCmdShow)
{

	if (CLSID_ExpectedRecognizer != CLSID_SpInprocRecognizer)
	{
		MessageBoxW(NULL, L"This sample was compiled against an incompatible version of sapi.h.\nPlease ensure that Microsoft Speech SDK and other sample requirements are installed and then rebuild application.", L"Missing requirements", MB_OK | MB_ICONERROR);

		return EXIT_FAILURE;
	}

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (SUCCEEDED(hr))
	{
		{
			CSpeechBasics application1;
			application1.Run(hInstance, nCmdShow);
		}

		CoUninitialize();
	}

	return EXIT_FAILURE;


}
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
	thread th1(FaceBasicsThread, hInstance, nCmdShow);
	thread th2(SpeechRecognizerThread, hInstance, nCmdShow);

	th1.join();
	th2.join();

	return 0;

}