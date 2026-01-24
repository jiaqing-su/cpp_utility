#pragma once
#ifndef _CMD_ARGS_HPP_
#define _CMD_ARGS_HPP_

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <assert.h>
#include <type_traits>

void println(const std::string& str) {
	std::cout << str << std::endl;
	::OutputDebugStringA((str + "\n").c_str());
}
void println(const std::wstring& str) {
	std::wcout << str << std::endl;
	::OutputDebugStringW((str + L"\n").c_str());
}

#define ARGS_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
			println(msg);\
            std::cerr << "Assertion failed: " << #expr <<",@"<<__FUNCTION__<<"("<<__LINE__<< "):"; \
            assert(expr); \
        } \
    } while(0)

template<typename CharT>
struct CmdArgBase {
	template<typename U> friend class CmdArgs;
protected:
	using StringT = std::basic_string<CharT>;
	StringT m_full_name;
	StringT m_simple_name;
	StringT m_help_string;
	bool m_have_value = false;//use defalut_value
	bool m_required = false;//must provided
	bool m_flag = false;
	bool m_is_scoped = false;
	bool m_is_set_default = false;

public:
	CmdArgBase() = default;
	CmdArgBase(const StringT& full_name, const StringT& simple_name = StringT()) :m_full_name(full_name), m_simple_name(simple_name) {}
	virtual ~CmdArgBase() { std::cout << __FUNCTION__ << std::endl; }

protected:
	virtual void set_value(const StringT& val) = 0;
};

template<typename T, typename CharT>
class CmdArg : public CmdArgBase<CharT> {
	template<typename U> friend class CmdArgs;
//public:
//protected:
	using CmdArgBase<CharT>::m_full_name;
	using CmdArgBase<CharT>::m_simple_name;
	using CmdArgBase<CharT>::m_help_string;
	using CmdArgBase<CharT>::m_have_value;//use defalut_value
	using CmdArgBase<CharT>::m_required;//must provided
	using CmdArgBase<CharT>::m_flag;
	using CmdArgBase<CharT>::m_is_scoped;
	using CmdArgBase<CharT>::m_is_set_default;
    using CmdArgBase<CharT>::CmdArgBase;//继承构造函数
	using StringT = std::basic_string<CharT>;

//public:
	T m_value;
	T m_default_value;
	T m_min_value;
	T m_max_value;
	std::vector<T> m_choices;
//public:

	bool check_in_choices() {
		if (m_choices.empty())
			return true;
		bool is_in_choices = false;
		std::basic_stringstream<CharT> ss;
		ss << m_full_name << ".value(" << m_value << ")must be in :";
		for (const auto& opt : m_choices) {
			if (opt == m_value) {
				is_in_choices = true;
				break;
			}
			ss << opt << ",";
		}
		ARGS_ASSERT(is_in_choices, ss.str());
		return is_in_choices;
	}

	bool check_in_scope() {
		bool is_scoped = true;
		if (m_is_scoped) {
			is_scoped = !(m_value<m_min_value || m_value>m_max_value);
			std::basic_stringstream<CharT> ss;
			ss << m_full_name << ".value(" << m_value << ")must be in :" << m_min_value << "<->" << m_max_value;
			ARGS_ASSERT(m_value<m_min_value || m_value>m_max_value, ss.str());
		}
		return is_scoped;
	}

	//CmdArgBase
	virtual void set_value(const StringT& val) override {
		std::basic_istringstream<CharT> iss(val);
		iss >> m_value;//注意流中不能有空格
		//是否在指定范围内
		check_in_choices();
		//检查范围
		check_in_scope();

		m_have_value = true;
	}

public:

	auto& help(const StringT& help_str) {
		m_help_string = help_str;
		return *this;
	}

	//必需的参数，不必default_value
	auto& require() {
		std::basic_stringstream<CharT> ss;
		ss << m_full_name << ",Cannot specify as required parameter when default value is provided";
		ARGS_ASSERT(!m_is_set_default, ss.str());
		m_required = true;
		return *this;
	}

	//没有参数的value
	auto& flag() {
		std::basic_stringstream<CharT> ss;
		ss << m_full_name << ",choices isn't empty, Cannot specify as flag parameter";
		ARGS_ASSERT(m_choices.empty(), ss.str());
		m_flag = true;
		return *this;
	}

	auto& default_value(const T& def_value) {
		std::basic_stringstream<CharT> ss;
		ss << m_full_name << " is required paramter, it doesn't need default value";
		ARGS_ASSERT(!m_required, ss.str());
		m_default_value = def_value;
		m_is_set_default = true;
		return *this;
	}

	auto& choices(const std::initializer_list<T>& choice_list) {
		static_assert(!std::is_same_v<T, bool>);
		static_assert(!std::is_same_v<T, float>);
		static_assert(!std::is_same_v<T, double>);
		std::basic_stringstream<CharT> ss;
		ss << m_full_name << " has be specified as a flag, it doesn't need choice";
		ARGS_ASSERT(!m_flag, ss.str());
		m_choices = choice_list;
		return this;
	}

	auto& scope(const T& min_val, const T& max_val) {
		static_assert(!std::is_same_v<T, bool>);
		static_assert(!std::is_same_v<T, StringT>);
		std::basic_stringstream<CharT> ss;
		ss << m_full_name << " only be choices or scope one option";
		ARGS_ASSERT(!m_choices.empty(), ss.str());
		std::basic_stringstream<CharT> sss;
		sss << m_full_name << " has be specified as a flag, it doesn't need scope";
		ARGS_ASSERT(!m_flag, sss.str());
		m_min_value = min_val;
		m_max_value = max_val;
		return this;
	}
};

template<>
void CmdArg<bool, char>::set_value(const std::string& val) {
	if (val == "true" || val == "1" || m_flag)
		m_value = true;
	else if (val == "false" || val == "0")
		m_value = false;
	else {
		std::basic_stringstream<char> ss;
		ss << m_full_name << " is specified as bool, it't value only be true/false or 1/0";
		ARGS_ASSERT(false, ss.str());
	}
	m_have_value = true;
}

template<>
void CmdArg<bool, wchar_t>::set_value(const std::wstring& val) {
	if (val == L"true" || val == L"1" || m_flag)
		m_value = true;
	else if (val == L"false" || val == L"0")
		m_value = false;
	else {
		std::basic_stringstream<wchar_t> ss;
		ss << m_full_name << L" is specified as bool, it't value only be true/false or 1/0";
		ARGS_ASSERT(false, ss.str());
	}
	m_have_value = true;
}

template<typename CharT>
class CmdArgs {
	using StringT = std::basic_string<CharT>;
	std::map<StringT, StringT> m_map_simple_names;//arg_simple_name -> arg_full_name
	std::map<StringT, CmdArgBase<CharT>*> m_map_args;//arg_full_name
	CharT m_prefix_char = '-';// '/'...
	CharT m_assign_char = '=';//:
public:
	~CmdArgs() {
		for (const auto& a : m_map_args) {
			delete a.second;
		}
	}

	template<typename T>
	CmdArg<T, CharT>& add_arg(const StringT& arg_full_name, const StringT& arg_simple_name_default = StringT()) {
		ARGS_ASSERT(!arg_full_name.empty(), "must provide a valid full name");

		auto iter = m_map_args.find(arg_full_name);
		std::basic_stringstream<CharT> ss;
		ss << arg_full_name << "(full name) already exists";
		ARGS_ASSERT(iter == m_map_args.end(), ss.str());

		auto arg_simple_name = arg_simple_name_default;
		if (arg_simple_name.empty())
			arg_simple_name = arg_full_name;

		auto p = new CmdArg<T, CharT>(arg_full_name, arg_simple_name);
		m_map_args[arg_full_name] = p;

		auto simple_iter = m_map_simple_names.find(arg_simple_name);
		std::basic_stringstream<CharT> sss;
		sss << arg_full_name << "(full name).simple name (" << arg_simple_name << ") already exists";
		ARGS_ASSERT(simple_iter == m_map_simple_names.end(), sss.str());
		m_map_simple_names[arg_simple_name] = arg_full_name;

		return *p;
	}

	void set_split_flag(CharT prefix_char, CharT assign_char) {
		m_prefix_char = prefix_char;
		m_assign_char = assign_char;
	}

	void show_help() {
		std::cout << "Usage:" << std::endl;
		for (const auto& kv : m_map_args) {
			auto arg = kv.second;
			const char* requred_str = arg->m_required ? "[required]" : "[optional]";
			std::basic_ostringstream<CharT> ss;
			ss << "\t" << arg->m_full_name << "\t" << arg->m_simple_name << "\t" << requred_str << arg->m_help_string;
			println(ss.str());
		}
	}

	bool parse(const std::vector<StringT>& argv) {
		std::string error_info;
		size_t argc = argv.size();
		size_t i = 0;
		for (; i < argc; i++) {
			StringT arg_string = argv[i];
			StringT arg_name;
			StringT arg_value;
			bool is_flag = false;
			bool is_simple_name = false;

			if (arg_string[0] != m_prefix_char) {
				std::basic_ostringstream<CharT> ss;
				ss <<"parse error: " << "paramter[" << i << "]" << "has not valid prefix";
				ARGS_ASSERT(false, ss.str());
				break;
			}
			if (arg_string[1] == m_prefix_char) {
				arg_string = arg_string.substr(2);//--完整参数名
			}
			else {
				arg_string = arg_string.substr(1);//-简写
				is_simple_name = true;
			}

			auto pos = arg_string.find(m_assign_char);
			if (pos != StringT::npos) {
				//-a=b
				arg_name = arg_string.substr(0, pos);
				arg_value = arg_string.substr(pos + 1);
			}
			else {
				//-a ,flag
				arg_name = arg_string;
				is_flag = true;
			}

			if (is_simple_name) {
				auto iter = m_map_simple_names.find(arg_name);
				if (iter == m_map_simple_names.end()) {
					std::basic_ostringstream<CharT> ss;
					ss << "parse error: " << "paramter[" << i << "]" << "is a invalid simple name [" << arg_name << "]";
					ARGS_ASSERT(false, ss.str());
					break;
				}
				arg_name = iter->second;
			}

			auto iter_arg = m_map_args.find(arg_name);
			if (iter_arg == m_map_args.end()) {
				std::basic_ostringstream<CharT> ss;
				ss << "parse error: " << "paramter[" << i << "]" << arg_name << " isn't be specified";
				ARGS_ASSERT(false, ss.str());
				break;
			}

			if (!is_flag && arg_value.empty()) {
				std::basic_ostringstream<CharT> ss;
				ss << "parse error: " << "paramter[" << i << "]" << arg_name << " isn't a flag ,but value is empty";
				ARGS_ASSERT(false, ss.str());
				break;
			}

			auto cur_arg = iter_arg->second;

			if (is_flag != cur_arg->m_flag) {
				std::basic_ostringstream<CharT> ss;
				ss << "parse error: " << "paramter[" << i << "]" << arg_name << " is a flag or not ???";
				ARGS_ASSERT(false, ss.str());
				break;
			}

			cur_arg->set_value(arg_value);
		}

		//check required agrs
		for (const auto& kv : m_map_args) {
			auto arg_name = kv.first;
			auto arg = kv.second;
			if (arg->m_required && !arg->m_have_value) {
				std::basic_ostringstream<CharT> ss;
				ss << "parse error: " << arg_name << " is required, but it didn't be provided";
				ARGS_ASSERT(false, ss.str());
				return false;
			}
		}

		return i == argc;
	}
	bool parse(int argc, const CharT** argv) {
		std::vector<StringT> vArgs;
		for (auto i = 1; i < argc; i++) {
			vArgs.push_back(argv[i]);
		}
		return parse(vArgs);
	}

	std::vector<std::basic_string<CharT>> ConvertCmdLineToArgv(const CharT* lpCmdLine) {
		std::vector<std::basic_string<CharT>> args;

		// 处理空指针或空字符串
		if (!lpCmdLine || !*lpCmdLine) {
			return args;
		}

		std::basic_string<CharT> currentArg;
		bool inQuotes = false;
		bool escapeNext = false;
		int backslashCount = 0;

		// 遍历命令行字符串
		while (*lpCmdLine) {
			CharT c = *lpCmdLine++;

			// 处理转义序列
			if (escapeNext) {
				currentArg += c;
				escapeNext = false;
				continue;
			}

			// 处理反斜杠计数（用于精确的引号转义处理）
			if (c == static_cast<CharT>('\\')) {
				backslashCount++;
				continue;
			}

			// 处理引号
			if (c == static_cast<CharT>('\"')) {
				// 添加反斜杠的一半（整数除法）
				for (int i = 0; i < backslashCount / 2; i++) {
					currentArg += static_cast<CharT>('\\');
				}

				if (backslashCount % 2 == 1) {
					// 奇数个反斜杠：转义引号
					currentArg += static_cast<CharT>('\"');
				}
				else {
					// 偶数个反斜杠：切换引号状态
					inQuotes = !inQuotes;
				}

				backslashCount = 0;
				continue;
			}

			// 处理累积的反斜杠
			if (backslashCount > 0) {
				for (int i = 0; i < backslashCount; i++) {
					currentArg += static_cast<CharT>('\\');
				}
				backslashCount = 0;
			}

			// 处理空格和制表符（作为参数分隔符）
			if (!inQuotes && (c == static_cast<CharT>(' ') || c == static_cast<CharT>('\t'))) {
				if (!currentArg.empty()) {
					args.push_back(std::move(currentArg));
					currentArg.clear();
				}
			}
			else {
				currentArg += c;
			}
		}

		// 处理末尾可能剩余的反斜杠
		if (backslashCount > 0) {
			for (int i = 0; i < backslashCount; i++) {
				currentArg += static_cast<CharT>('\\');
			}
		}

		// 添加最后一个参数
		if (!currentArg.empty()) {
			args.push_back(std::move(currentArg));
		}

		return args;
	}

	bool parse(const CharT* lpCmdLine) {
		auto vArgs = ConvertCmdLineToArgv(lpCmdLine);
		return parse(vArgs);
	}

	template<typename T>
	const CmdArg<T, CharT>& at(const StringT& arg_name) {
		auto iter_arg = m_map_args.find(arg_name);
		if (iter_arg == m_map_args.end()) {
			//guess it's a simple name
			auto iter_name = m_map_simple_names.find(arg_name);
			if (iter_name == m_map_simple_names.end()) {
				std::basic_ostringstream<CharT> ss;
				ss << "paramter.access error: " << arg_name << " isn't full_name ,nor is simple name";
				ARGS_ASSERT(false, ss.str());
			}
			else {
				iter_arg = m_map_args.find(iter_name->second);
			}
		}

		std::basic_ostringstream<CharT> ss;
		ss << "paramter.access error: " << arg_name << " isn't exists";
		ARGS_ASSERT(iter_arg != m_map_args.end(), ss.str());

		auto arg = dynamic_cast<CmdArg<T, CharT>*>(iter_arg->second);
		std::basic_ostringstream<CharT> ss2;
		ss2 << "paramter.access error: " << arg_name << " declared type and actual type do not match";
		ARGS_ASSERT(arg != nullptr, ss2.str());

		return *arg;
	}

	template<typename T>
	T get(const StringT& arg_name) {
		auto arg = at<T>(arg_name);
		if (!arg.m_have_value)
			return arg.m_default_value;
		return arg.m_value;
	}
};

using CmdArgsW = CmdArgs<wchar_t>;
using CmdArgsA = CmdArgs<char>;

#endif//_CMD_ARGS_HPP_