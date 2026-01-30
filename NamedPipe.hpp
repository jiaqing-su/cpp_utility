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
            NamedPipeClient(const std::string &name)
            {
                m_wstrPipeName = "\\\\.\\pipe\\" + name;
            }
            NamedPipeClient(const std::string &name, std::string host, bool messageMode)
                : m_bMessageMode(messageMode)
            {
                if (host.empty())
                    host = ".";
                m_wstrPipeName = "\\\\" + host + "\\pipe\\" + name;
            }
            ~NamedPipeClient() = default;
            bool Connect(uint32_t timeout = 5000)
            {
                int retryCount = 0;
                do
                {
                    m_hDevice = CreateFileA(
                        m_wstrPipeName.c_str(),
                        GENERIC_READ | GENERIC_WRITE| FILE_WRITE_ATTRIBUTES,
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
                 //SetNamedPipeHandleState need FILE_WRITE_ATTRIBUTES
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
            bool m_bMessageMode = true;
            //DWORD m_dwMaxInstances = PIPE_UNLIMITED_INSTANCES;
            //DWORD m_dwOutBufSize = 4096;
            //DWORD m_dwInBufSize = 4096;
            bool m_bConnected = false;

        public:
            NamedPipeServer(const std::string &name)
            {
                m_strPipeName = "\\\\.\\pipe\\" + name;
            }
            NamedPipeServer(const std::string &name, std::string host, bool messageMode)
                : m_bMessageMode(messageMode)
            {
                if (host.empty())
                    host = ".";
                m_strPipeName = "\\\\" + host + "\\pipe\\" + name;
            }
            ~NamedPipeServer() = default;

			bool Start(DWORD dwMaxInstances = PIPE_UNLIMITED_INSTANCES, DWORD dwBufSize = 0)
            {
                DWORD dwPipeMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
                if (m_bMessageMode)
                    dwPipeMode = PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT;
                m_hDevice = CreateNamedPipeA(
                    m_strPipeName.c_str(),
                    PIPE_ACCESS_DUPLEX,
                    dwPipeMode,
                    dwMaxInstances,
                    dwBufSize,
                    dwBufSize,
                    0,
                    NULL);

                if (m_hDevice == INVALID_HANDLE_VALUE)
                {
                    PrintError("CreateNamedPipe", GetLastError());
                    return false;
                }

                BOOL fConnected = ConnectNamedPipe(m_hDevice, NULL);
                if (fConnected)
                {
                    m_bConnected = true;
                }
                else
                {
                    DWORD dwError = GetLastError();
                    if (dwError == ERROR_PIPE_CONNECTED)
                    {
                        m_bConnected = true;
                    }
                    else
                    {
                        PrintError("ConnectNamedPipe", dwError);
                        Close();
                    }
                }

                return m_bConnected;
            }

            void Stop()
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