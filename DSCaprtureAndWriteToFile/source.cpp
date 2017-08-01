#include <dshow.h>
#include <atlbase.h>
#include <initguid.h>
#include <dvdmedia.h>
#include <wmsdkidl.h>
#include "SampleGrabber.h"

#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "Quartz.lib")


// {860BB310-5D01-11D0-BD3B-00A0C911CE86}
DEFINE_GUID(CLSID_VideoCaptureSource,
	0x860BB310, 0x5D01, 0x11D0, 0xBD, 0x3B, 0x00, 0xA0, 0xC9, 0x11, 0xCE, 0x86);

// {C1F400A0-3F08-11D3-9F0B-006008039E37}
DEFINE_GUID(CLSID_SampleGrabber, 
	0xC1F400A0, 0x3F08, 0x11D3, 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37); //qedit.dll

// {B87BEB7B-8D29-423F-AE4D-6582C10175AC}
DEFINE_GUID(CLSID_VideoRenderer,
	0xB87BEB7B, 0x8D29, 0x423F, 0xAE, 0x4D, 0x65, 0x82, 0xC1, 0x01, 0x75, 0xAC); //quartz.dll


bool hrcheck(HRESULT hr, char *errText)
{
	DWORD dwRes = 0;
	TCHAR szErr[MAX_ERROR_TEXT_LEN] = { 0 };
	if (hr >= S_OK)
	{
		return false;
	}
	
	dwRes = AMGetErrorText(hr, szErr, MAX_ERROR_TEXT_LEN);
	if (dwRes > 0)
	{
		printf("Error %x: %s\n%s\n", hr, errText, szErr);
	}
	else
	{
		printf("Error %x: %s\n", hr, errText);
	}
	return true;
}

#define  CHECK_HR(hr, msg) if (hrcheck(hr, msg)) return hr;

CComPtr<IBaseFilter> CreateFilterByName(const WCHAR *filterName, const GUID& category)
{
	HRESULT hr = S_OK;
	CComPtr<ICreateDevEnum> pSysDevEnum;
	hr = pSysDevEnum.CoCreateInstance(CLSID_SystemDeviceEnum);
	if (hrcheck(hr, ("Can't create System Device Enumerator")))
	{
		return NULL;
	}

	CComPtr<IEnumMoniker> pEnumCat;
	hr = pSysDevEnum->CreateClassEnumerator(category, &pEnumCat, 0);

	if (hr == S_OK)
	{
		CComPtr<IMoniker> pMoniker;
		ULONG cFetched;
		while (pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK)
		{
			CComPtr<IPropertyBag> pBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
			if (SUCCEEDED(hr))
			{
				VARIANT varName;
				VariantInit(&varName);
				hr = pBag->Read(L"FriendlyName", &varName, 0);
				if (SUCCEEDED(hr))
				{
					if (wcscmp(filterName, varName.bstrVal) == 0)
					{
						CComPtr<IBaseFilter> pFilter;
						hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pFilter);
						if (hrcheck(hr, "Can't bind moniker to filter object"))
						{
							return NULL;
						}
						return pFilter;
					}
				}
				VariantClear(&varName);
			}
			pMoniker.Release();
		}
	}
	return NULL;
}

CComPtr<IPin> GetPin(IBaseFilter *pFilter, LPCOLESTR pinName)
{
	CComPtr<IEnumPins> pEnum;
	CComPtr<IPin> pPin;

	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (hrcheck(hr, "Can't enumerate pins."))
		return NULL;

	while (pEnum->Next(1, &pPin, 0) == S_OK)
	{
		PIN_INFO pInfo;
		pPin->QueryPinInfo(&pInfo);
		bool bFound = !wcsicmp(pinName, pInfo.achName);
		if (pInfo.pFilter != NULL)
		{
			pInfo.pFilter->Release();
		}
		if (bFound == true)
		{
			return pPin;
		}
		pPin.Release();
	}
	printf("Pin not found!\n");
	return NULL;
}

HRESULT BuildGraph(IGraphBuilder *pGraph)
{
	HRESULT hr = S_OK;
	CComPtr<ICaptureGraphBuilder2> pBuilder;
	hr = pBuilder.CoCreateInstance(CLSID_CaptureGraphBuilder2);
	CHECK_HR(hr, "Can't create Capture Graph Builder");
	hr = pBuilder->SetFiltergraph(pGraph);
	CHECK_HR(hr, "Can't SetFilterGraph ");
	CComPtr<IBaseFilter> pCam = CreateFilterByName(L"Logitech QuickCam Pro 5000", CLSID_VideoCaptureSource);
	hr = pGraph->AddFilter(pCam, L"Logitech QuickCam Pro 5000");
	CHECK_HR(hr, "Can't add Logitech QuickCam Pro 5000 to graph");

	CComPtr<IBaseFilter> pSGrabber;
	hr = pSGrabber.CoCreateInstance(CLSID_SampleGrabber);
	CHECK_HR(hr, "Can't create SampleGrabber");
	hr = pGraph->AddFilter(pSGrabber, L"SampleGrabber");
	CHECK_HR(hr, "Can't add SampleGrabber to graph");
	AM_MEDIA_TYPE pSGrabber_pmt;
	ZeroMemory(&pSGrabber_pmt, sizeof(AM_MEDIA_TYPE));
	pSGrabber_pmt.majortype = MEDIATYPE_Video;
	pSGrabber_pmt.subtype = WMMEDIASUBTYPE_I420;
	pSGrabber_pmt.formattype = FORMAT_VideoInfo;
	pSGrabber_pmt.bFixedSizeSamples = TRUE;
	pSGrabber_pmt.cbFormat = 88;
	pSGrabber_pmt.lSampleSize = 115200;
	pSGrabber_pmt.bTemporalCompression = FALSE;

	VIDEOINFOHEADER pSGrabber_format;
	ZeroMemory(&pSGrabber_format, sizeof(VIDEOINFOHEADER));
	pSGrabber_format.bmiHeader.biSize = 40;
	pSGrabber_format.bmiHeader.biWidth = 320;
	pSGrabber_format.bmiHeader.biHeight = 240;
	pSGrabber_format.bmiHeader.biPlanes = 1;
	pSGrabber_format.bmiHeader.biBitCount = 12;
	pSGrabber_format.bmiHeader.biCompression = 808596553;
	pSGrabber_format.bmiHeader.biSizeImage = 115200;
	pSGrabber_pmt.pbFormat = (BYTE *)&pSGrabber_format;

	CComQIPtr<ISampleGrabber, &IID_ISampleGrabber> pSGrabber_isg(pSGrabber);
	hr = pSGrabber_isg->SetMediaType(&pSGrabber_pmt);
	CHECK_HR(hr, "Can't set media type to sample grabber");

	// Соединение камеры с SampleGrabber
	hr = pGraph->ConnectDirect(GetPin(pCam, L"Запись"), GetPin(pSGrabber, L"Input"), NULL);
	CHECK_HR(hr, "Can't connect Logitech QuickCam Pro 5000 and SampleGrabber");

	// и AVI декомпрессор
	CComPtr<IBaseFilter> pAVIDecompressor;
	hr = pAVIDecompressor.CoCreateInstance(CLSID_AVIDec);
	CHECK_HR(hr, "Can't create AVI Decompressor");
	hr = pGraph->AddFilter(pAVIDecompressor, L"AVI Decompressor");
	CHECK_HR(hr, "Can't add AVI Decompressor to graph");

	// соединение AVIDec и SampleGrabber
	hr = pGraph->ConnectDirect(GetPin(pSGrabber, L"Output"), GetPin(pAVIDecompressor, L"XForm In"), NULL);
	CHECK_HR(hr, "Can't connect SampleGrabber and AVI Decompressor");

	// подключение Color Space Converter
	CComPtr<IBaseFilter> pColorSpaceConverter;
	hr = pColorSpaceConverter.CoCreateInstance(CLSID_Colour);
	CHECK_HR(hr, "Can't creater Color Space Converter");
	hr = pGraph->AddFilter(pColorSpaceConverter, L"Color Space Converter");
	CHECK_HR(hr, "Can't add CSC to graph");

	// соединение AVIDec и CSC
	hr = pGraph->ConnectDirect(GetPin(pAVIDecompressor, L"XForm Out"), GetPin(pColorSpaceConverter, L"Input"), NULL);
	CHECK_HR(hr, "Can't connect AVIDec and CSC");

	// Подключение VideoRenderer
	CComPtr<IBaseFilter> pVideoRend;
	hr = pVideoRend.CoCreateInstance(CLSID_VideoRenderer);
	CHECK_HR(hr, "Can't create Video Renderer");
	hr = pGraph->AddFilter(pVideoRend, L"Video Renderer");
	CHECK_HR(hr, "Can't add Video Renderer to graph");

	hr = pGraph->ConnectDirect(GetPin(pColorSpaceConverter, L"XForm Out"), GetPin(pVideoRend, L"VMR Input0"), NULL);
	CHECK_HR(hr, "Can't connect CSC and Video Renderer");

	return S_OK;
}



int main()
{
	CoInitialize(NULL);
	CComPtr<IGraphBuilder> graph;
	graph.CoCreateInstance(CLSID_FilterGraph);

	printf("Building graph...\n");
	HRESULT hr = BuildGraph(graph);
	if (hr == S_OK)
	{
		printf("Running");
		CComQIPtr<IMediaControl, &IID_IMediaControl> mediaControl(graph);
		hr = mediaControl->Run();
		CHECK_HR(hr, "Can't run");
		CComQIPtr<IMediaEvent, &IID_IMediaEvent> mediaEvent(graph);
		bool bStop = false;
		MSG msg;
		while (!bStop)
		{
			long ev = 0;
			long p1 = 0;
			long p2 = 0;
			printf(".");
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				DispatchMessage(&msg);
			}
			while (mediaEvent->GetEvent(&ev, &p1, &p2, 0) == S_OK)
			{
				if (ev == EC_COMPLETE || ev == EC_USERABORT)
				{
					printf("Done!\n");
					bStop = true;
				}
				else if (ev == EC_ERRORABORT)
				{
					printf("An error occured: HRESULT=%x\n", p1);
					mediaControl->Stop();
					bStop = true;
				}
				mediaEvent->FreeEventParams(ev, p1, p2);
			}
		}
	}
	CoUninitialize();
	return 0;
}