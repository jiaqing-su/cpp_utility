#ifndef _IODEVICE_HPP_
#define _IODEVICE_HPP_

#include <windows.h>
#include <stdint.h>
#include <string>
#include <iostream>

namespace sjq
{
    class IODevice
    {
    protected:
        HANDLE m_hDevice = INVALID_HANDLE_VALUE;

    public:
        IODevice() = default;
        void Close()
        {
            if (IsOpen())
            {
                CloseHandle(m_hDevice);
                m_hDevice = INVALID_HANDLE_VALUE;
            }
        }

        ~IODevice()
        {
            Close();
        }

        bool IsOpen() const
        {
            return m_hDevice != INVALID_HANDLE_VALUE && m_hDevice != NULL;
        }

        static std::string GetLastErrorString(DWORD errorCode)
        {
            // DWORD errorCode = GetLastError();
            LPSTR buffer = nullptr;

            FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                errorCode,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_CHINESE_SIMPLIFIED),
                (LPSTR)&buffer,
                0,
                NULL);

            LocalFree(buffer);
            return buffer ? std::string(buffer) : "Unknown error";
        }
        static void PrintError(const std::string &msg, DWORD error)
        {
            std::string errorMsg = GetLastErrorString(error);
            std::cerr << "ERROR: " << msg.c_str() << ", GetLastError=" << error << ":" << errorMsg << std::endl;
        }

    public:
        uint32_t Write(const char *buf, uint32_t len)
        {
            if (!IsOpen() || !buf || len == 0)
                return -1;

            DWORD bytesWritten = 0;
            BOOL result = WriteFile(m_hDevice, buf, len, &bytesWritten, NULL);
            if (!result)
            {
                PrintError("WriteFile", GetLastError());
                return -1;
            }
            return bytesWritten;
        }

        uint32_t Read(char *buf, uint32_t len)
        {
            if (!IsOpen() || !buf || len == 0)
                return -1;

            DWORD bytesRead = 0;
            BOOL result = ReadFile(m_hDevice, buf, len, &bytesRead, NULL);
            if (!result)
            {
                PrintError("ReadFile", GetLastError());
                return -1;
            }
            return bytesRead;
        }

        bool WriteMessage(const std::string &msg)
        {
            if (!IsOpen() || msg.empty())
            {
                return msg.empty();
            }

            int tryCount = 0;
            uint32_t total = static_cast<uint32_t>(msg.length());
            uint32_t written = 0;
            do
            {
                DWORD bytesWritten = 0;
                BOOL result = WriteFile(m_hDevice, msg.c_str() + written, total - written, &bytesWritten, NULL);
                written += bytesWritten;
                if (!result)
                {
                    DWORD error = GetLastError();
                    if (error == ERROR_NO_DATA && tryCount++ < 10)
                    {
                        Sleep(10);
                    }
                    else
                    {
                        PrintError("WriteFile", error);
                        break;
                    }
                }

            } while (written < total);

            return total == written;
        }
        bool ReadMessage(std::string &msg)
        {
            if (!IsOpen())
            {
                return false;
            }

            constexpr int buffer_size = 1024;
            char buffer[buffer_size] = {0};

            do
            {
                DWORD dwBytesRead = 0;
                BOOL result = ReadFile(m_hDevice, buffer, buffer_size, &dwBytesRead, NULL);
                if (dwBytesRead > 0)
                {
                    msg.append(buffer, dwBytesRead);
                }

                if (result)
                {
                    if (dwBytesRead < buffer_size)
                        break;
                }
                else
                {
                    DWORD error = GetLastError();
                    if (error == ERROR_MORE_DATA)
                    {
                        // Sleep(10);
                    }
                    else
                    {
                        PrintError("ReadFile", error);
                        return false;
                    }
                }

            } while (1);

            return true;
        }
    };

}

#endif //_IODEVICE_HPP_