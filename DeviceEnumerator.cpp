#include "stdafx.h"
#include "DeviceEnumerator.h"
#include <string>
#include <algorithm>
#include <assert.h>
#include <atlbase.h>
//core api
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
//dshow
#include <uuids.h>
#include <Strmif.h>
//#include <comutil.h>

#pragma comment(lib, "strmiids.lib")

namespace sj
{
    DeviceEnumerator::DeviceEnumerator()
    {
    }


    DeviceEnumerator::~DeviceEnumerator()
    {
    }

    template<>
    std::vector<DeviceEnumerator::DeviceInfo> DeviceEnumerator::GetDevices<DeviceEnumerator::DirectShow>(DeviceEnumerator::DeviceType type)
    {
        std::vector<DeviceInfo> vDeviceInfo;

        GUID guidDeviceCategory = CLSID_VideoInputDeviceCategory;
        switch (type)
        {
        case DEVICE_TYPE_VIDEO_INPUT:
            guidDeviceCategory = CLSID_VideoInputDeviceCategory;
            break;
        case DEVICE_TYPE_AUDIO_INPUT:
            guidDeviceCategory = CLSID_AudioInputDeviceCategory;
            break;
        case DEVICE_TYPE_AUDIO_OUTPUT:
            guidDeviceCategory = CLSID_AudioRendererCategory;
            break;
        default:
            break;
        }

        do
        {
            HRESULT hr = ::CoInitialize(nullptr);
            if (FAILED(hr))
                break;

            ATL::CComPtr<ICreateDevEnum> pSysDevEnum;
            hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void **)&pSysDevEnum);
            if (hr != S_OK)
            {
                XLOG_ERROR(L"CoCreateInstance CLSID_SystemDeviceEnum.error = %d", hr);
                break;
            }

            ATL::CComPtr<IEnumMoniker> pEnumCat;
            hr = pSysDevEnum->CreateClassEnumerator(guidDeviceCategory/*CLSID_VideoInputDeviceCategory*/, &pEnumCat, 0);
            if (hr != S_OK)
            {
                XLOG_ERROR(L"CoCreateInstance CreateClassEnumerator.error = %d", hr);
                break;
            }

            std::wstring strLastFriendlyName;
            int nLastIndex = 0;//maybe exists same FriendlyNames

            while (1)
            {
                ATL::CComPtr<IMoniker> pMoniker;
                ULONG cFetched = 0;
                hr = pEnumCat->Next(1, &pMoniker, &cFetched);
                if (hr != S_OK || cFetched == 0)
                    break;

                DeviceInfo dev;
                memset(&dev, 0, sizeof(dev));

                //display name
                LPOLESTR pszDisplayName;
                pMoniker->GetDisplayName(nullptr, nullptr, &pszDisplayName);
                if (pszDisplayName)
                    wcscpy_s(dev.display_name, pszDisplayName);
                ::CoTaskMemFree(pszDisplayName);

                dev.is_virtual = true;

                ATL::CComPtr<IPropertyBag> pPropBag;
                hr = pMoniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void **)&pPropBag);
                if (hr == S_OK)
                {
                    CComVariant varDevicePath;
                    hr = pPropBag->Read(L"DevicePath", &varDevicePath, nullptr);
                    if (hr == S_OK)                    {
                        const wchar_t* wzDevicePath = (wchar_t*)varDevicePath.bstrVal;
                        if (wzDevicePath)
                        {
                            wcscpy_s(dev.device_path, wzDevicePath);
                            dev.is_virtual = false;
                        }
                    }

                    CComVariant varFriendlyName;
                    hr = pPropBag->Read(L"FriendlyName", &varFriendlyName, nullptr);
                    if (hr == S_OK)                    {
                        wchar_t* wzFriendlyName = (wchar_t*)varFriendlyName.bstrVal;
                        if (wzFriendlyName)                        {
                            if (strLastFriendlyName == wzFriendlyName && strLastFriendlyName.empty() == false)
                                nLastIndex++;
                            else
                                nLastIndex = 0;

                            strLastFriendlyName = wzFriendlyName;
                            wcscpy_s(dev.friendly_name, wzFriendlyName);
                            dev.index_same_name = nLastIndex;
                        }
                        else                        {
                            strLastFriendlyName.clear();
                            nLastIndex = 0;
                        }
                    }

                    vDeviceInfo.push_back(dev);
                }
            }

        } while (0);

        ::CoUninitialize();

#ifdef _WIN64        
        if (DEVICE_TYPE_VIDEO_INPUT == type) {
            XLOG_INFO(L"酷狗直播伴侣vcam不支持x64，cef是x64，但助手是32的");
            sj::DeviceEnumerator::DeviceInfo kugouDev;
            kugouDev.is_virtual = 1;
            wcscpy_s(kugouDev.display_name, L"酷狗直播伴侣");
            wcscpy_s(kugouDev.friendly_name, L"酷狗直播伴侣");
            vDeviceInfo.push_back(kugouDev);
        }
#endif
        return vDeviceInfo;
    }

    template<>
    std::vector<DeviceEnumerator::DeviceInfo> DeviceEnumerator::GetDevices<DeviceEnumerator::CoreApi>(DeviceEnumerator::DeviceType type)
    {
        EDataFlow mmDevType = eCapture;
        std::vector<DeviceInfo> vDeviceInfo;

        switch (type)
        {
        case DEVICE_TYPE_VIDEO_INPUT:
            assert(false && "core api only for audio"); 
            return vDeviceInfo;
            break;
        case DEVICE_TYPE_AUDIO_INPUT:
            mmDevType = eCapture;
            break;
        case DEVICE_TYPE_AUDIO_OUTPUT:
            mmDevType = eRender;
            break;
        default:
            break;
        }

		do
		{
            HRESULT hr = ::CoInitialize(nullptr);
			if (FAILED(hr)) {
				XLOG_ERROR(L"CoInitialize failed, hr = %d", hr);
				break;
			}

            CComPtr<IMMDeviceEnumerator> pEnumerator;
            hr = pEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
            if (FAILED(hr)) {
                XLOG_ERROR(L"CoCreateInstance failed, hr = %d", hr);
                break;
            }

            if (pEnumerator == nullptr) {
                XLOG_ERROR(L"pMMDeviceEnumerator is null");
                break;
            }

            // 获取所有活动设备
            CComPtr<IMMDeviceCollection> pCollection;
            hr = pEnumerator->EnumAudioEndpoints(mmDevType, DEVICE_STATE_ACTIVE, &pCollection);
            if (FAILED(hr)) {
                XLOG_ERROR(L"EnumAudioEndpoints failed, hr = %d", hr);
                break;
            }

            if (pCollection == nullptr) {
                XLOG_ERROR(L"pCollection is null");
                break;
            }

			UINT count = 0;
			pCollection->GetCount(&count);

			for (UINT i = 0; i < count; i++) {
				CComPtr<IMMDevice> pDevice;
				if (SUCCEEDED(pCollection->Item(i, &pDevice))) {
					CComPtr<IPropertyStore> pProps;
					if (SUCCEEDED(pDevice->OpenPropertyStore(STGM_READ, &pProps))) {
                        DeviceInfo dev;
                        memset(&dev, 0, sizeof(dev));

						PROPVARIANT varName;
						PropVariantInit(&varName);
						if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
							if (varName.vt == VT_LPWSTR && varName.pwszVal) {
								wcscpy_s(dev.friendly_name, varName.pwszVal);
							}
							PropVariantClear(&varName);
						}

						if (dev.friendly_name[0])
							vDeviceInfo.push_back(dev);
					}
				}
			}
        } while (0);

        ::CoUninitialize();
        return vDeviceInfo;
    }

    template<>
    DeviceEnumerator::DeviceInfo DeviceEnumerator::GetDefaultDevice<DeviceEnumerator::CoreApi>(DeviceEnumerator::DeviceType type)
    {
        EDataFlow mmDevType = eCapture;
        //EDataFlow type = eCapture;
        DeviceEnumerator::DeviceInfo deviceInfo;
        memset(&deviceInfo, 0, sizeof(deviceInfo));

        switch (type)
        {
        case DEVICE_TYPE_VIDEO_INPUT:
            assert(false && "core api only for audio"); return deviceInfo;
            break;
        case DEVICE_TYPE_AUDIO_INPUT:
            mmDevType = eCapture;
            break;
        case DEVICE_TYPE_AUDIO_OUTPUT:
            mmDevType = eRender;
            break;
        default:
            assert(false && "core api dosen't suported device type"); return deviceInfo;
            break;
        }

		do
		{
			HRESULT hr = ::CoInitialize(nullptr);
			if (FAILED(hr)) {
				XLOG_ERROR(L"CoInitialize failed, hr = %d", hr);
				break;
			}

			CComPtr<IMMDeviceEnumerator> pEnumerator;
			hr = pEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
			if (FAILED(hr)) {
				XLOG_ERROR(L"CoCreateInstance failed, hr = %d", hr);
				break;
			}

			CComPtr<IMMDevice> pDevice;
			hr = pEnumerator->GetDefaultAudioEndpoint(mmDevType, eConsole, &pDevice);
			if (FAILED(hr)) {
				XLOG_ERROR(L"GetDefaultAudioEndpoint failed, hr = %d", hr);
				break;
			}
			if (pDevice == nullptr) {
				XLOG_ERROR(L"GetDefaultAudioEndpoint is success, but IMMDevice == null");
				break;
			}

			CComPtr<IPropertyStore> pProps;
			hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
			if (FAILED(hr)) {
				XLOG_ERROR(L"OpenPropertyStore failed, hr = %d", hr);
				break;
			}
			if (pProps == nullptr) {
				XLOG_ERROR(L"OpenPropertyStore is success, but IPropertyStore == null");
				break;
			}

			PROPVARIANT varName;
			PropVariantInit(&varName);
			hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
			if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
				if (varName.vt == VT_LPWSTR && varName.pwszVal) {
					wcscpy_s(deviceInfo.friendly_name, varName.pwszVal);
				}
				else {
					XLOG_ERROR(L"IPropertyStore success, but var(from GetValue) is wrong");
				}
			}
			else {
				XLOG_ERROR(L"failed to call IPropertyStore:GetValue(PKEY_Device_FriendlyName), hr = %d", hr);
			}
			PropVariantClear(&varName);

		} while (0);
        
        ::CoUninitialize();

        return deviceInfo;
    }

    template<>
    DeviceEnumerator::DeviceInfo DeviceEnumerator::GetDefaultDevice<DeviceEnumerator::DirectShow>(DeviceEnumerator::DeviceType type)
    {
        DeviceEnumerator::DeviceInfo deviceInfo;
        memset(&deviceInfo, 0, sizeof(deviceInfo));

        GUID guidDeviceCategory = CLSID_VideoInputDeviceCategory;
        switch (type)
        {
        case DEVICE_TYPE_VIDEO_INPUT:
            guidDeviceCategory = CLSID_VideoInputDeviceCategory;
            break;
        case DEVICE_TYPE_AUDIO_INPUT:
            guidDeviceCategory = CLSID_AudioInputDeviceCategory;
            break;
        case DEVICE_TYPE_AUDIO_OUTPUT:
            guidDeviceCategory = CLSID_AudioRendererCategory;
            break;
        default:
            break;
        }

		do
		{
			HRESULT hr = ::CoInitialize(nullptr);
			if (FAILED(hr))
				break;

			ATL::CComPtr<ICreateDevEnum> pSysDevEnum;
			hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pSysDevEnum);
			if (FAILED(hr)) {
				XLOG_ERROR(L"CoCreateInstance CLSID_SystemDeviceEnum.error = %d", hr);
				break;
			}

			ATL::CComPtr<IEnumMoniker> pEnumCat;
			hr = pSysDevEnum->CreateClassEnumerator(guidDeviceCategory/*CLSID_VideoInputDeviceCategory*/, &pEnumCat, 0);
			if (FAILED(hr)) {
				XLOG_ERROR(L"CoCreateInstance CreateClassEnumerator.error = %d", hr);
				break;
			}

			ATL::CComPtr<IMoniker> pMoniker;
			ULONG cFetched = 0;
			hr = pEnumCat->Next(1, &pMoniker, &cFetched);
			if (FAILED(hr) || cFetched == 0)
				break;

			//display name
			LPOLESTR pszDisplayName;
			pMoniker->GetDisplayName(nullptr, nullptr, &pszDisplayName);
			if (pszDisplayName)
				wcscpy_s(deviceInfo.display_name, pszDisplayName);
			::CoTaskMemFree(pszDisplayName);

			deviceInfo.is_virtual = true;

			ATL::CComPtr<IPropertyBag> pPropBag;
			hr = pMoniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void**)&pPropBag);
            if (FAILED(hr)) {
                XLOG_ERROR(L"OpenPropertyStore failed, hr = %d", hr);
                break;
            }
			if (pPropBag == nullptr) {
				XLOG_ERROR(L"OpenPropertyStore is success, but IPropertyStore == null");
				break;
			}

			CComVariant varDevicePath;
			hr = pPropBag->Read(L"DevicePath", &varDevicePath, nullptr);
			if (hr == S_OK) {
				const wchar_t* wzDevicePath = (wchar_t*)varDevicePath.bstrVal;
				if (wzDevicePath)
				{
					wcscpy_s(deviceInfo.device_path, wzDevicePath);
					deviceInfo.is_virtual = false;
				}
			}

			CComVariant varFriendlyName;
			hr = pPropBag->Read(L"FriendlyName", &varFriendlyName, nullptr);
			if (hr == S_OK) {
				wchar_t* wzFriendlyName = (wchar_t*)varFriendlyName.bstrVal;
				if (wzFriendlyName) {
					wcscpy_s(deviceInfo.friendly_name, wzFriendlyName);
				}
			}

		} while (0);

        ::CoUninitialize();

        return deviceInfo;
    }

    bool DeviceEnumerator::CoreApiToDirectShow(DeviceEnumerator::DeviceInfo& mmDevice, const std::vector<DeviceEnumerator::DeviceInfo>& dshowDevices)
    {
        std::wstring srcMmDevName = mmDevice.friendly_name;
        std::wstring dstDshowName;

		auto it = std::find_if(dshowDevices.begin(), dshowDevices.end(), [&](const DeviceEnumerator::DeviceInfo& devInfo) {
			return (devInfo.friendly_name == srcMmDevName);
			});

        //完全匹配
        if (it != dshowDevices.end())
            return true;

		XLOG_DEBUG(L"mm.device.name:%s", srcMmDevName.c_str());
		for (const DeviceEnumerator::DeviceInfo& dshowDev : dshowDevices) {
			XLOG_DEBUG(L"dshow.device.name:%s", dshowDev.friendly_name);
		}

		//部分匹配，后面带有修饰,去掉括号，前面的基本名字是否一样
		for (const DeviceEnumerator::DeviceInfo& dshowDev : dshowDevices) {
			std::wstring dshowDevName = dshowDev.friendly_name;

			auto baseDshowName = dshowDevName;
			auto pos1 = dshowDevName.find(L"(");
			if (pos1 != std::wstring::npos) {
				baseDshowName = dshowDevName.substr(0, pos1);
			}

			auto baseMmName = srcMmDevName;
			auto pos2 = srcMmDevName.find(L"(");
			if (pos2 != std::wstring::npos) {
				baseMmName = srcMmDevName.substr(0, pos2);
			}

			if (baseDshowName == baseMmName) {
				wcscpy_s(mmDevice.friendly_name, MAX_PATH, dshowDevName.c_str());
				XLOG_WARNING(L"(mm->dshow)base device name:%s , %s --> %s", baseMmName.c_str(), srcMmDevName.c_str(), dshowDevName.c_str());
				return true;
			}
		}

		//名字的前部分一致
		for (const DeviceEnumerator::DeviceInfo& dshowDev : dshowDevices) {
			std::wstring dshowDevName = dshowDev.friendly_name;
            size_t min_len = dshowDevName.length();
            if (min_len > srcMmDevName.length())
                min_len = srcMmDevName.length();
			//size_t min_len = std::min(dshowDevName.length(), srcMmDevName.length());
			if (srcMmDevName.compare(0, min_len, dshowDevName) == 0) {
				wcscpy_s(mmDevice.friendly_name, MAX_PATH, dshowDevName.c_str());
				auto shortName = srcMmDevName.substr(0, min_len);
				XLOG_WARNING(L"(mm->dshow)short device name:%s , %s --> %s", shortName.c_str(), srcMmDevName.c_str(), dshowDevName.c_str());
				return true;
			}
		}

		XLOG_ERROR(L"can't find mm.device : %s in dshow device list", srcMmDevName.c_str());
		for (const DeviceEnumerator::DeviceInfo& dshowDev : dshowDevices) {
			XLOG_INFO(L"dshow device : %s", dshowDev.friendly_name);
		}
       
        return false;
    }
}