#pragma once
#include <vector>

namespace sj 
{
	class DeviceEnumerator {
	public:
		//device category
		enum DeviceCategory {
			DirectShow,
			CoreApi
			//static constexpr int DirectShow = 1000;
			//static constexpr int CoreApi = 2000;
		};

		enum DeviceType {
			DEVICE_TYPE_VIDEO_INPUT = 0,
			DEVICE_TYPE_AUDIO_INPUT,
			DEVICE_TYPE_AUDIO_OUTPUT,
		};
		struct DeviceInfo {
			wchar_t display_name[_MAX_PATH] = { 0 };
			wchar_t friendly_name[_MAX_PATH] = { 0 };
			wchar_t device_path[_MAX_PATH] = { 0 };
			int index_same_name = 0;
			int is_virtual = 0;
		};

	public:
		DeviceEnumerator();
		~DeviceEnumerator();

		template<DeviceCategory category>
		static std::vector<DeviceInfo> GetDevices(DeviceType type);

		template<DeviceCategory category>
		static DeviceInfo GetDefaultDevice(DeviceType type);//core api

		static bool CoreApiToDirectShow(DeviceInfo& mmDevice, const std::vector<DeviceInfo>& dshowDevices);
	};
}