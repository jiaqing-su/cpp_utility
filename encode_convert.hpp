#ifndef _ENCODE_CONVERT_HPP_
#define _ENCODE_CONVERT_HPP_
#pragma once

#include <codecvt>
#include <string>
#include <mbctype.h>//_getmbcp

namespace sj
{
	namespace encode
	{
#if defined(WIN32) || defined(_WIN32)
#define GBK_LOCALE_NAME ".437"
#else
#define GBK_LOCALE_NAME "zh_CN.GBK"//linux
#endif
		using WCHAR_GBK = std::codecvt_byname<wchar_t, char, mbstate_t>;
		using WCHAR_UTF8 = std::codecvt_utf8<wchar_t>;
		using wchar_utf8_convert = std::wstring_convert<WCHAR_UTF8>;

		class wchar_gbk_convert : public std::wstring_convert<WCHAR_GBK>
		{
		public:
			wchar_gbk_convert() : std::wstring_convert<WCHAR_GBK>(new WCHAR_GBK(GetCurCodePage()))
			{
			}

			static std::string GetCurCodePage(void)
			{
                static std::string curCodePage;
                if (curCodePage.empty())
                    curCodePage = "." + std::to_string(_getmbcp());
                return curCodePage;
			}
		};

		class gbk_utf8_convert : public wchar_gbk_convert, public wchar_utf8_convert
		{
		public:
			std::string to_bytes(const std::string& gbk)
			{
				return wchar_utf8_convert::to_bytes(wchar_gbk_convert::from_bytes(gbk));
			}

			std::string to_bytes(const char* gbk)
			{
				return wchar_utf8_convert::to_bytes(wchar_gbk_convert::from_bytes(gbk));
			}

			std::string from_bytes(const std::string& utf8)
			{
				return wchar_gbk_convert::to_bytes(wchar_utf8_convert::from_bytes(utf8));
			}
			std::string from_bytes(const char* utf8)
			{
				return wchar_gbk_convert::to_bytes(wchar_utf8_convert::from_bytes(utf8));
			}
		};

		inline std::string gbk(const std::wstring& wstr)
		{
			std::string str;
            if (!wstr.empty())
            {
                try
                {
                    wchar_gbk_convert cvt;
                    str = cvt.to_bytes(wstr);
                }
                catch (...)
                {
                    //XLOG_WARNING(L"exception: unicode -> gbk");
                }
            }
			return str;
		}

		inline std::string gbk(const wchar_t* wstr)
		{
			std::wstring ws;
			if (wstr)
				ws = wstr;
			return gbk(ws);
		}

		inline std::string gbk(const std::string& utf8)
		{
			std::string str;
            if (!utf8.empty())
            {
                try
                {
                    gbk_utf8_convert cvt;
                    str = cvt.from_bytes(utf8);
                }
                catch (...)
                {
                    //XLOG_WARNING(L"exception: utf8 -> gbk");
                }
            }
			return str;
		}

		//inline std::string gbk(const char* utf8)
		//{
		//	std::string strutf8;
		//	if (utf8)
		//		strutf8 = utf8;
		//	return gbk(strutf8);
		//}
		inline std::string utf8(const std::wstring& wstr)
		{
			std::string str;
            if (!wstr.empty())
            {
                try
                {
                    wchar_utf8_convert cvt;
                    str = cvt.to_bytes(wstr);
                }
                catch (...)
                {
                    //XLOG_WARNING(L"exception: unicode -> utf8");
                }
            }
			return str;
		}

		//inline std::string utf8(const wchar_t* wstr)
		//{
		//	std::wstring ws;
		//	if (wstr)
		//		ws = wstr;
		//	return utf8(ws);
		//}
		
		inline std::string utf8(const std::string& gbk)
		{
			std::string str;
            if (!gbk.empty())
            {
                try
                {
                    gbk_utf8_convert cvt;
                    str = cvt.to_bytes(gbk);
                }
                catch (...)
                {
                    //XLOG_WARNING(L"exception: gbk -> utf8");
                }
            }
			return str;
		}

		inline std::string utf8(const char* gbk)
		{
			std::string strgbk;
			if (gbk)
				strgbk = gbk;
			return utf8(strgbk);
		}

		inline std::wstring unicode(const std::string& str, bool is_utf8 = false)
		{
			std::wstring wstr;
            if (!str.empty())
            {
                for (int i = 0; i < 2; i++)
                {
                    try
                    {
                        if (is_utf8)
                        {
                            wchar_utf8_convert cvt;
                            wstr = cvt.from_bytes(str);
                        }
                        else
                        {
                            wchar_gbk_convert cvt;
                            wstr = cvt.from_bytes(str);
                        }
                        break;
                    }
                    catch (...)
                    {
                        //XLOG_ERROR(L"exception: %s -> unicode", is_utf8 ? L"utf8" : L"gbk");
                        is_utf8 = !is_utf8;
                    }
                }
            }
			return wstr;
		}

		inline std::wstring unicode(const char* str, bool is_utf8 = false)
		{
			std::string s;
			if (str)
				s = str;
			return unicode(s, is_utf8);
		}

		

	}//encode
}//sj

#endif//_ENCODE_CONVERT_HPP_