#include "CallbackObject.h"

CCallbackObject::CCallbackObject()
{
	m_Ref = 0;
	fOut = fopen("d:\\tmp\\video.out", "w");
	fOutMod = fopen("d:\\tmp\\video_mod.out", "w");
}


CCallbackObject::~CCallbackObject()
{
}

STDMETHODIMP CCallbackObject::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) return E_POINTER;
	if (riid == __uuidof(IUnknown)) {
		*ppv = static_cast<IUnknown*>(this);
		return S_OK;
	}
	if (riid == __uuidof(ISampleGrabberCB)) {
		*ppv = static_cast<ISampleGrabberCB*>(this);
		return S_OK;
	}
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCallbackObject::CCallbackObject::AddRef()
{
	InterlockedIncrement(&m_Ref);
	return m_Ref;
}

STDMETHODIMP_(ULONG) CCallbackObject::CCallbackObject::Release()
{
	InterlockedDecrement(&m_Ref);
	if (m_Ref == 0)
	{
		delete this;
		fcloseall();
		return 0;
	}
	else
		return m_Ref;
}

STDMETHODIMP CCallbackObject::SampleCB(double SampleTime, IMediaSample *pSample)
{
	if (!pSample)
		return E_POINTER;
	long sz = pSample->GetActualDataLength();
	BYTE *pBuf = NULL;
	pSample->GetPointer(&pBuf);
	if (sz <= 0 || pBuf == NULL) 
		return E_UNEXPECTED;
//	fprintf(fOut, "%s", pBuf);
//	fflush(fOut);
	for (int i = 0; i < sz; i++)
	{
		pBuf[i] = 1-pBuf[i];
	}
// 	fprintf(fOutMod, "%s", pBuf);
// 	fflush(fOutMod);

	return S_OK;
}

STDMETHODIMP CCallbackObject::BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen)
{
	return S_OK;
}



