#ifndef _NAMED_PIPE_HPP_
#define _NAMED_PIPE_HPP_
#include <windows.h>
#include <string>
#include "IODevice.hpp"

namespace sjq
{
    namespace ipc
    {
        class NamedPipeClient : public IODevice
        {
            std::string m_wstrPipeName;
            bool m_bMessageMode = true;

        public:
            NamedPipeClient(const std::string &name, bool messageMode = true)
                : m_bMessageMode(messageMode)
            {
                m_wstrPipeName = "\\\\.\\pipe\\" + name;
            }
            ~NamedPipeClient() = default;
            bool Connect(uint32_t timeout = 5000)
            {
                int retryCount = 0;
                do
                {
                    m_hDevice = CreateFileA(
                        m_wstrPipeName.c_str(),
                        GENERIC_READ | GENERIC_WRITE | FILE_WRITE_ATTRIBUTES,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);

                    if (m_hDevice != INVALID_HANDLE_VALUE)
                        break; // ok

                    DWORD dwError = GetLastError();
                    PrintError("CreateFile", dwError);
                    if (dwError == ERROR_PIPE_BUSY)
                    {
                        if (!WaitNamedPipeA(m_wstrPipeName.c_str(), timeout))
                        {
                            PrintError("WaitNamedPipe", dwError);
                            return false;
                        }
                    }
                    else
                    {
                        return false;
                    }

                } while (retryCount++ < 10);

                DWORD mode = m_bMessageMode ? PIPE_READMODE_MESSAGE : PIPE_READMODE_BYTE;
                mode |= PIPE_WAIT;
                // SetNamedPipeHandleState need FILE_WRITE_ATTRIBUTES
                if (!SetNamedPipeHandleState(m_hDevice, &mode, NULL, NULL))
                {
                    PrintError("SetNamedPipeHandleState", GetLastError());
                    Close();
                    return false;
                }

                return true;
            }
        };

        class NamedPipeServer : public IODevice
        {
            std::string m_strPipeName;
            bool m_bConnected = false;
            DWORD m_dwPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE;

        public:
            NamedPipeServer(const std::string &name, bool messageMode = true, bool blocked = false)
            {
                m_strPipeName = "\\\\.\\pipe\\" + name;

                if (messageMode)
                    m_dwPipeMode = PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE;
                else
                    m_dwPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE;
                m_dwPipeMode |= blocked ? PIPE_WAIT : PIPE_NOWAIT;
            }
            ~NamedPipeServer() = default;

            bool WaitConnect(uint32_t timeout = -1)
            {
                m_hDevice = CreateNamedPipeA(
                    m_strPipeName.c_str(),
                    PIPE_ACCESS_DUPLEX,
                    m_dwPipeMode,
                    PIPE_UNLIMITED_INSTANCES,
                    4096,
                    4096,
                    0,
                    NULL);

                if (m_hDevice == INVALID_HANDLE_VALUE)
                {
                    PrintError("CreateNamedPipe", GetLastError());
                    return false;
                }

                uint32_t timeOfConnect = 0;
                do
                {
                    BOOL fConnected = ConnectNamedPipe(m_hDevice, NULL);
                    if (fConnected)
                    {
                        m_bConnected = true;
                        break;
                    }

                    DWORD dwError = GetLastError();
                    if (dwError == ERROR_PIPE_CONNECTED)
                    {
                        m_bConnected = true;
                        break;
                    }

                    if (ERROR_PIPE_LISTENING == dwError)
                    {
                        if (timeOfConnect > timeout)
                        {
                            break;
                        }
                        timeOfConnect += 100;
                        Sleep(100);
                    }
                    else
                    {
                        PrintError("ConnectNamedPipe", dwError);
                        Close();
                        break;
                    }
                } while (1);

                return m_bConnected;
            }

            void Disconnect()
            {
                if (!m_bConnected || !IsOpen())
                {
                    return;
                }

                if (!DisconnectNamedPipe(m_hDevice))
                {
                    PrintError("DisconnectNamedPipe", GetLastError());
                }
                Close();
                m_bConnected = false;
            }
            bool IsConnected() const { return m_bConnected; }
        };
    }
}
#endif