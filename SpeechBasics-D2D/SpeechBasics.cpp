//------------------------------------------------------------------------------
// <copyright file="SpeechBasics.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "../stdafx.h"
#include "SpeechBasics.h"
#include "../resource.h"
Action ActionsForJaco = ActionNone;
//#include <guiddef.h>

// Static initializers
 LPCWSTR CSpeechBasics::GrammarFileName = L"SpeechBasics-D2D.grxml";

// This is the class ID we expect for the Microsoft Speech recognizer.
// Other values indicate that we're using a version of sapi.h that is
// incompatible with this sample.
//  DEFINE_GUID(CLSID_ExpectedRecognizer, 0x495648e7, 0xf7ab, 0x4267, 0x8e, 0x0f, 0xca, 0xfb, 0x7a, 0x33, 0xc1, 0x60);

/// <summary>
/// Constructor
/// </summary>
CSpeechBasics::CSpeechBasics() :
    m_pAudioBeam(NULL),
    m_pAudioStream(NULL),
    m_p16BitAudioStream(NULL),
    m_hSensorNotification(reinterpret_cast<WAITABLE_HANDLE>(INVALID_HANDLE_VALUE)),
    m_pSpeechStream(NULL),
    m_pSpeechRecognizer(NULL),
    m_pSpeechContext(NULL),
    m_pSpeechGrammar(NULL),
    m_hSpeechEvent(INVALID_HANDLE_VALUE)
{
}

/// <summary>
/// Destructor
/// </summary>
CSpeechBasics::~CSpeechBasics()
{
    if (m_pKinectSensor)
    {
        m_pKinectSensor->Close();
    }

    //16 bit Audio Stream
    if (NULL != m_p16BitAudioStream)
    {
        delete m_p16BitAudioStream;
        m_p16BitAudioStream = NULL;
    }
    SafeRelease(m_pAudioStream);
    SafeRelease(m_pAudioBeam);
    SafeRelease(m_pKinectSensor);
}

/// <summary>
/// Creates the main window and begins processing
/// </summary>
/// <param name="hInstance">handle to the application instance</param>
/// <param name="nCmdShow">whether to display minimized, maximized, or normally</param>
int CSpeechBasics::Run(HINSTANCE hInstance, int nCmdShow)
{
   
    const int maxEventCount = 2;
    int eventCount = 1;
    HANDLE hEvents[maxEventCount];

	StartKinect();
    // Main message loop
    while (1)
    {
        if (m_hSpeechEvent != INVALID_HANDLE_VALUE)
        {
            hEvents[1] = m_hSpeechEvent;
            eventCount = 2;
        }

        hEvents[0] = reinterpret_cast<HANDLE>(m_hSensorNotification);

        // Check to see if we have either a message (by passing in QS_ALLINPUT)
        // Or sensor notification (hEvents[0])
        // Or a speech event (hEvents[1])
        DWORD waitResult = MsgWaitForMultipleObjectsEx(eventCount, hEvents, 50, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
		
        switch (waitResult)
        {
        case WAIT_OBJECT_0:

            {
                BOOLEAN sensorState = FALSE;

                // Getting the event data will reset the event.
                IIsAvailableChangedEventArgs* pEventData = nullptr;
                if (FAILED(m_pKinectSensor->GetIsAvailableChangedEventData(m_hSensorNotification, &pEventData)))
                {
					OutputDebugString(L"Failed to get sensor availability.\n");
                    break;
                }

                pEventData->get_IsAvailable(&sensorState);
                SafeRelease(pEventData);

                if (sensorState == FALSE)
                {
					OutputDebugString(L"Sensor has been disconnected - attach Sensor\n");
                }
                else
                {
                    HRESULT hr = S_OK;

                    if (m_pSpeechRecognizer == NULL)
                    {
                        hr =InitializeSpeech();
						OutputDebugString(L"Initialized Speech\n");
                    }
                    if (SUCCEEDED(hr))
                    {
						OutputDebugString(L"Ready to recieve commands\n");
                    }
                    else
                    {
						OutputDebugString(L"Speech Initialization Failed\n");
                    }
                }
            }
            break;
        case WAIT_OBJECT_0 + 1:
            if(eventCount == 2)
            {
			#if TESTING
				OutputDebugString(L"Entering Process\n");
			#endif
                ProcessSpeech();
            }
            break;
        }
    }
	return 0;
}



/// <summary>
/// Open the KinectSensor and its Audio Stream
/// </summary>
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT CSpeechBasics::StartKinect()
{
    HRESULT hr = S_OK;
    IAudioSource* pAudioSource = NULL;
    IAudioBeamList* pAudioBeamList = NULL;
    BOOLEAN sensorState = TRUE;

    hr = m_pKinectSensor->SubscribeIsAvailableChanged(&m_hSensorNotification);

    if (SUCCEEDED(hr))
    {
        hr = m_pKinectSensor->Open();
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pKinectSensor->get_AudioSource(&pAudioSource);
    }

    if (SUCCEEDED(hr))
    {
        hr = pAudioSource->get_AudioBeams(&pAudioBeamList);
    }

    if (SUCCEEDED(hr))
    {        
        hr = pAudioBeamList->OpenAudioBeam(0, &m_pAudioBeam);
    }

    if (SUCCEEDED(hr))
    {        
        hr = m_pAudioBeam->OpenInputStream(&m_pAudioStream);
        m_p16BitAudioStream = new KinectAudioStream(m_pAudioStream);
    }

    if (FAILED(hr))
    {
        OutputDebugString(L"Failed opening an audio stream!");
    }

    SafeRelease(pAudioBeamList);
    SafeRelease(pAudioSource);
    return hr;
}

/// <summary>
/// Open the KinectSensor and its Audio Stream
/// </summary>
/// <returns>S_OK on success, otherwise failure code.</returns>
HRESULT CSpeechBasics::InitializeSpeech()
{

    // Audio Format for Speech Processing
    WORD AudioFormat = WAVE_FORMAT_PCM;
    WORD AudioChannels = 1;
    DWORD AudioSamplesPerSecond = 16000;
    DWORD AudioAverageBytesPerSecond = 32000;
    WORD AudioBlockAlign = 2;
    WORD AudioBitsPerSample = 16;

    WAVEFORMATEX wfxOut = {AudioFormat, AudioChannels, AudioSamplesPerSecond, AudioAverageBytesPerSecond, AudioBlockAlign, AudioBitsPerSample, 0};

    HRESULT hr = CoCreateInstance(CLSID_SpStream, NULL, CLSCTX_INPROC_SERVER, __uuidof(ISpStream), (void**)&m_pSpeechStream);

    if (SUCCEEDED(hr))
    {

        m_p16BitAudioStream->SetSpeechState(true);        
        hr = m_pSpeechStream->SetBaseStream(m_p16BitAudioStream, SPDFID_WaveFormatEx, &wfxOut);
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateSpeechRecognizer();
    }

    if (FAILED(hr))
    {
		OutputDebugString(L"Could not create speech recognizer. Please ensure that Microsoft Speech SDK and other sample requirements are installed.");
        return hr;
    }

    hr = LoadSpeechGrammar();

    if (FAILED(hr))
    {
		OutputDebugString(L"Could not load speech grammar. Please ensure that grammar configuration file was properly deployed.");
        return hr;
    }

    hr = StartSpeechRecognition();

    if (FAILED(hr))
    {
        OutputDebugString(L"Could not start recognizing speech.");
        return hr;
    }

    return hr;
}


/// <summary>
/// Create speech recognizer that will read Kinect audio stream data.
/// </summary>
/// <returns>
/// <para>S_OK on success, otherwise failure code.</para>
/// </returns>
HRESULT CSpeechBasics::CreateSpeechRecognizer()
{
    ISpObjectToken *pEngineToken = NULL;

    HRESULT hr = CoCreateInstance(CLSID_SpInprocRecognizer, NULL, CLSCTX_INPROC_SERVER, __uuidof(ISpRecognizer), (void**)&m_pSpeechRecognizer);

    if (SUCCEEDED(hr))
    {
        m_pSpeechRecognizer->SetInput(m_pSpeechStream, TRUE);

        // If this fails here, you have not installed the acoustic models for Kinect
        hr = SpFindBestToken(SPCAT_RECOGNIZERS, L"Language=409;Kinect=True", NULL, &pEngineToken);

        if (SUCCEEDED(hr))
        {
            m_pSpeechRecognizer->SetRecognizer(pEngineToken);
            hr = m_pSpeechRecognizer->CreateRecoContext(&m_pSpeechContext);

            // For long recognition sessions (a few hours or more), it may be beneficial to turn off adaptation of the acoustic model. 
            // This will prevent recognition accuracy from degrading over time.
            if (SUCCEEDED(hr))
            {
                hr = m_pSpeechRecognizer->SetPropertyNum(L"AdaptationOn", 0);                
            }
        }
    }
    SafeRelease(pEngineToken);
    return hr;
}

/// <summary>
/// Load speech recognition grammar into recognizer.
/// </summary>
/// <returns>
/// <para>S_OK on success, otherwise failure code.</para>
/// </returns>
HRESULT CSpeechBasics::LoadSpeechGrammar()
{
    HRESULT hr = m_pSpeechContext->CreateGrammar(1, &m_pSpeechGrammar);

    if (SUCCEEDED(hr))
    {
        // Populate recognition grammar from file
        hr = m_pSpeechGrammar->LoadCmdFromFile(GrammarFileName, SPLO_STATIC);
    }

    return hr;
}

/// <summary>
/// Start recognizing speech asynchronously.
/// </summary>
/// <returns>
/// <para>S_OK on success, otherwise failure code.</para>
/// </returns>
HRESULT CSpeechBasics::StartSpeechRecognition()
{
    HRESULT hr = S_OK;

    // Specify that all top level rules in grammar are now active
    hr = m_pSpeechGrammar->SetRuleState(NULL, NULL, SPRS_ACTIVE);
    if (FAILED(hr))
    {
        return hr;
    }

    // Specify that engine should always be reading audio
    hr = m_pSpeechRecognizer->SetRecoState(SPRST_ACTIVE_ALWAYS);
    if (FAILED(hr))
    {
        return hr;
    }

    // Specify that we're only interested in receiving recognition events
    hr = m_pSpeechContext->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION));
    if (FAILED(hr))
    {
        return hr;
    }

    // Ensure that engine is recognizing speech and not in paused state
    hr = m_pSpeechContext->Resume(0);
    if (FAILED(hr))
    {
        return hr;
    }

    m_hSpeechEvent = m_pSpeechContext->GetNotifyEventHandle();
    return hr;
}

/// <summary>
/// Process recently triggered speech recognition events.
/// </summary>
void CSpeechBasics::ProcessSpeech()
{
    const float ConfidenceThreshold = 0.3f;

    SPEVENT curEvent = {SPEI_UNDEFINED, SPET_LPARAM_IS_UNDEFINED, 0, 0, 0, 0};
    ULONG fetched = 0;
    HRESULT hr = S_OK;

    m_pSpeechContext->GetEvents(1, &curEvent, &fetched);
    while (fetched > 0)
    {
        switch (curEvent.eEventId)
        {
        case SPEI_RECOGNITION:
            if (SPET_LPARAM_IS_OBJECT == curEvent.elParamType)
            {
                // this is an ISpRecoResult
                ISpRecoResult* result = reinterpret_cast<ISpRecoResult*>(curEvent.lParam);
                SPPHRASE* pPhrase = NULL;

                hr = result->GetPhrase(&pPhrase);
                if (SUCCEEDED(hr))
                {
                    if ((pPhrase->pProperties != NULL) && (pPhrase->pProperties->pFirstChild != NULL))
                    {
                        const SPPHRASEPROPERTY* pSemanticTag = pPhrase->pProperties->pFirstChild;
                        if (pSemanticTag->SREngineConfidence > ConfidenceThreshold)
                        {
                            Action tempAction = MapSpeechTagToAction(pSemanticTag->pszValue);
							if (tempAction != ActionNone)
							{
								ActionsForJaco = tempAction;
							}
                        }
                    }
                    ::CoTaskMemFree(pPhrase);
                }
            }
            break;
        }

        m_pSpeechContext->GetEvents(1, &curEvent, &fetched);
    }

    return;
}

/// <summary>
/// Maps a specified speech semantic tag to the corresponding action to be performed on turtle.
/// </summary>
/// <returns>
/// Action that matches <paramref name="pszSpeechTag"/>, or TurtleActionNone if no matches were found.
/// </returns>
Action CSpeechBasics::MapSpeechTagToAction(LPCWSTR pszSpeechTag)
{
    struct SpeechTagToAction
    {
        LPCWSTR pszSpeechTag;
        Action action;
    };
    const SpeechTagToAction Map[] =
    {
        {L"DRINK", ActionDrink},
        {L"FOOD", ActionFood},
        {L"BOWL", ActionBowl},
		{L"SOUP", ActionSoup},
		{L"SCOOP", ActionScoop},
		{L"STOP", ActionStop},
		{L"RESET", ActionReset}
    };

	
    Action action = ActionNone;

    for (int i = 0; i < _countof(Map); ++i)
    {
        if ( (Map[i].pszSpeechTag != NULL) && (0 == wcscmp(Map[i].pszSpeechTag, pszSpeechTag)))
        {
            action = Map[i].action;
            break;
        }
    }

    return action;
}
