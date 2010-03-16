// AutoCrop.cpp : Implementation of CAutoCrop

#include "stdafx.h"
#include <gdiplus.h>
#include "avisynth.h"
#include "AutoCrop.h"

using namespace Gdiplus;

#define CHK(x) \
	do { hr = (x); if (FAILED(hr)) return hr; } while (0)
#define CHK_BOOL(x) \
	do { if (!(x)) return E_FAIL; } while (0)

inline DWORD *getPixel(BYTE *pixel, int stride, int x, int y)
{
	return (DWORD *)(pixel + stride * y) + x;
}

bool scan(DWORD *pStart, int delta, int count)
{
	int blackPixels = 0;
	for (int i = 0; i < count; i++)
	{
		DWORD pixel = *pStart & 0xE0E0E0;
		if (pixel == 0)
			blackPixels++;
		pStart += delta;
	}
	return blackPixels == count;
}

CAutoCrop::CAutoCrop()
: m_pEnv(NULL)
{
}

HRESULT CAutoCrop::FinalConstruct()
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	HMODULE hLib = LoadLibrary(L"avisynth.dll");
	CHK_BOOL(hLib != 0);

	IScriptEnvironment* (__stdcall *CreateScriptEnvironment)(int version) = (IScriptEnvironment* (__stdcall *)(int)) GetProcAddress(hLib, "CreateScriptEnvironment");
	CHK_BOOL(CreateScriptEnvironment != 0);

	m_pEnv = CreateScriptEnvironment(AVISYNTH_INTERFACE_VERSION);
	CHK_BOOL(m_pEnv != NULL);

	return S_OK;
}

inline int roundUp(int x, int align)
{
	return ((x + align - 1) / align) * align;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if(size == 0)
	  return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if(pImageCodecInfo == NULL)
	  return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for(UINT j = 0; j < num; ++j)
	{
	  if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
	  {
		 *pClsid = pImageCodecInfo[j].Clsid;
		 free(pImageCodecInfo);
		 return j;  // Success
	  }    
	}

	free(pImageCodecInfo);
	return -1;  // Failure
}

HRESULT CAutoCrop::AutoCropFrame(int frameNo, int *pLeft, int *pTop, int *pRight, int *pBottom)
{
	int width = m_inf.width;
	int height = m_inf.height;

	PVideoFrame frame = m_clp->GetFrame(frameNo, m_pEnv);

	Bitmap *pBitmap = new Bitmap(width, height);
	BitmapData* pBitmapData = new BitmapData;
	Rect rect(0, 0, width, height);
	pBitmap->LockBits(
		&rect,
		ImageLockModeWrite,
		PixelFormat24bppRGB,
		pBitmapData);

	int stride = pBitmapData->Stride;
	BYTE *pixels = (BYTE *)pBitmapData->Scan0;

	m_pEnv->BitBlt(pixels, stride, frame->GetReadPtr(), frame->GetPitch(), frame->GetRowSize(), frame->GetHeight());

	pBitmap->UnlockBits(pBitmapData);

	CLSID pngClsid;
	GetEncoderClsid(L"image/png", &pngClsid);
   
	wchar_t imgPath[MAX_PATH];
	wsprintf(imgPath, L"%ls\\cap-%d.png", m_path, frameNo);
	pBitmap->Save(imgPath, &pngClsid);

	pBitmap->LockBits(
		&rect,
		ImageLockModeRead,
		PixelFormat32bppARGB,
		pBitmapData);

	stride = pBitmapData->Stride;
	pixels = (BYTE *)pBitmapData->Scan0;

	int y;
	for (y = 0; y < height; y++)
	{
		DWORD *p = getPixel(pixels, stride, 0, y);
		if (!scan(p, 1, width))
			break;
	}
	
	*pBottom = roundUp(y, 2); // Image is reversed

	for (y = height - 1; y >= 0; y--)
	{
		DWORD *p = getPixel(pixels, stride, 0, y);
		if (!scan(p, 1, width))
			break;
	}

	*pTop = roundUp(height - 1 - y, 2);

	int x;
	for (x = 0; x < width; x++)
	{
		DWORD *p = getPixel(pixels, stride, x, 0);
		if (!scan(p, stride / sizeof(DWORD), height))
			break;
	}

	*pLeft = roundUp(x, 4);

	for (x = width - 1; x >= 0; x--)
	{
		DWORD *p = getPixel(pixels, stride, x, 0);
		if (!scan(p, stride / sizeof(DWORD), height))
			break;
	}

	*pRight = roundUp(width - 1 - x, 4);

	pBitmap->UnlockBits(pBitmapData);

	return S_OK;
}

int fCmpint(const void *a, const void *b)
{
	return *(int *)a - *(int *)b;
}

HRESULT CAutoCrop::Median(int *pValues, int *pMedian)
{
	qsort(pValues, nSamples, sizeof(int), fCmpint);
	*pMedian = pValues[nSamples / 2];
	return S_OK;
}

STDMETHODIMP CAutoCrop::GetAutoCropValues(BSTR fileName)
{
	HRESULT hr = S_OK;
	char afileName[MAX_PATH];
	int sz = WideCharToMultiByte(CP_ACP, 0, fileName, SysStringLen(fileName), afileName, sizeof(afileName), NULL, NULL);
	afileName[sz] = 0;

	m_path[0] = 0;
	int len = SysStringLen(fileName);
	for (int i = len - 1; i >= 0; i--)
	{
		if (fileName[i] == L'\\')
		{
			wcsncpy_s(m_path, MAX_PATH, fileName, i);
			break;
		}
	}

	AVSValue arg(afileName);
	AVSValue res = m_pEnv->Invoke("Import", AVSValue(&arg, 1));
	CHK_BOOL(res.IsClip());

	m_clp = res.AsClip();
	m_inf = m_clp->GetVideoInfo();

	if (!m_inf.IsRGB24())
	{
		res = m_pEnv->Invoke("ConvertToRGB24", AVSValue(&res, 1));
		m_clp = res.AsClip();
		m_inf = m_clp->GetVideoInfo();
	}
	CHK_BOOL(m_inf.IsRGB24());

	int nFrames = m_inf.num_frames;
	for (int iSample = 0; iSample < nSamples; iSample++)
	{
		int sampleNo = (iSample + 1) * nFrames / (nSamples + 1);
		CHK(AutoCropFrame(sampleNo, &m_left[iSample], &m_top[iSample], &m_right[iSample], &m_bottom[iSample]));
	}

	CHK(Median(m_left, &m_mLeft));
	CHK(Median(m_top, &m_mTop));
	CHK(Median(m_right, &m_mRight));
	CHK(Median(m_bottom, &m_mBottom));

	return S_OK;
}

STDMETHODIMP CAutoCrop::get_Left(USHORT* pVal)
{
	*pVal = (USHORT)m_mLeft;
	return S_OK;
}

STDMETHODIMP CAutoCrop::get_Top(USHORT* pVal)
{
	*pVal = (USHORT)m_mTop;
	return S_OK;
}

STDMETHODIMP CAutoCrop::get_Right(USHORT* pVal)
{
	*pVal = (USHORT)m_mRight;
	return S_OK;
}

STDMETHODIMP CAutoCrop::get_Bottom(USHORT* pVal)
{
	*pVal = (USHORT)m_mBottom;
	return S_OK;
}
