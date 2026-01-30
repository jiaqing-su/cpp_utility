// named_pipe.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <memory>
#include <thread>
//#include "../../NamedPipeClient.hpp"
#include "../../NamedPipe.hpp"

int main(int argc, const char* argv[])
{
	bool isSvr = argc > 1;
	std::cout << std::string(isSvr ? "server" : "client") << std::endl;

	std::shared_ptr<sjq::IODevice> pipe;
	if (isSvr) {
		auto svr = std::make_shared<sjq::ipc::NamedPipeServer>("test");
		pipe = svr;
		if (!svr->Start())
			return -1;
	}
	else {
		auto clt = std::make_shared<sjq::ipc::NamedPipeClient>("test");
		pipe = clt;
		if (!clt->Connect())
			return -1;
	}

	std::thread([&]() {
		std::string msg;
		pipe->ReadMessage(msg);
		std::cout << "recv:" << msg << std::endl;
		}).detach();

	while (1)
	{
		std::string msg;
		if (isSvr&&false) {			
			pipe->ReadMessage(msg);
			std::cout << "client:" << msg << std::endl;
			pipe->WriteMessage(msg);
		}
		else {
			std::cin >> msg;
			pipe->WriteMessage(msg);
			msg.clear();
			//pipe->ReadMessage(msg);
			//std::cout << msg << std::endl;
		}
	}

	return 0;
}

