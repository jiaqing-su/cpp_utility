#pragma once
#include <string>

class Command
{
public:
	Command();
	~Command(void);

    bool exec(const std::wstring& cmd_str, const wchar_t* cmd_path = nullptr);
	const std::string& get_std_out()const{return str_std_out;}
	const std::string& get_std_err()const{return str_std_err;}
	int get_exit_code()const{return exit_code;}

private:
	std::string str_std_err;
	std::string str_std_out;
	int         exit_code;
};

