// process.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <fstream>
#include "Win32Process.hpp"

int main(int argc ,const char*argv[])
{
    if (argc > 1) {
        //child
        while (1) {
            std::ofstream ofs("test");
            std::string msg;
            std::cin >> msg;
            //ofs << msg;
			std::string echo = "child.recv:" + msg +": PID:" + std::to_string(sjq::Process::GetThisPid());
			std::cout << sjq::Process::GetThisPid() << ":" << msg;
            //break;
            //Sleep(1000);
        }
    }
    else {
        auto child = sjq::Process::Create("process.exe -a=child", true, "", true);
        while (1) {
            std::string msg;
            std::cin >> msg;
            //msg.push_back('\n');
            auto wt = child->PipeWrite(msg.c_str(), msg.length());
            //std::cout << "parent.write " << wt << " bytes" << std::endl;

            char buf[1024] = { 0 };
            auto len = child->PipeRead(buf, 1024);
            buf[len] = 0;
			std::cout << buf << std::endl;
        }
    }

    //std::cout << "Hello World!\n";
}

