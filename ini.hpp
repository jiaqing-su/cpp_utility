#ifndef _INI_HPP_
#define _INI_HPP_
#include <fstream>
#include <string>
#include <map>
#include <regex>
#include <iostream>

namespace sjq
{
	class Ini
	{
		std::map<std::string, std::map<std::string, std::string>> m_kvs;

	public:
		static void PrintError(const std::string &msg)
		{
			std::cerr << "Error: " << msg << std::endl;
		}

	public:
		bool Load(const std::string &file)
		{
			std::ifstream ifs(file);
			if (!ifs.is_open())
				return false;

			std::string current_section = "default";
			std::map<std::string, std::string> current_kvs;
			std::string line;
			while (std::getline(ifs, line))
			{
				std::string section_name;
				static const std::regex section_pattern(R"(^\s*\[([^\[\]\s]+)\]\s*$)");
				std::smatch matches;
				if (std::regex_match(line, matches, section_pattern))
				{
					if (matches.size() > 1)
					{
						section_name = matches[1].str();
					}
				}
				if (!section_name.empty())
				{
					if (section_name != current_section)
					{
						m_kvs[current_section] = current_kvs;
						current_kvs.clear();
						current_section = section_name;
					}
				}
				else
				{
					static const std::regex kv_pattern(R"(^\s*([^=\s]+)\s*=\s*(.*?)\s*$)");
					std::smatch kv_matches;
					if (std::regex_match(line, kv_matches, kv_pattern))
					{
						if (kv_matches.size() > 2)
						{
							auto key = kv_matches[1].str();
							auto value = kv_matches[2].str();
							current_kvs[key] = value;
						}
						else
						{
							PrintError("Invalid line: " + line);
						}
					}
					else
					{
						PrintError("Invalid line: " + line);
					}
				}
			}

			m_kvs[current_section] = current_kvs;
			return true;
		}

		std::string Get(const std::string &key, const std::string &section = "default")
		{
			auto iterSec = m_kvs.find(section);
			if (iterSec != m_kvs.end())
			{
				auto iterItem = iterSec->second.find(key);
				if (iterItem != iterSec->second.end())
				{
					return iterItem->second;
				}
			}
			return std::string();
		}
	};
}
#endif //_INI_HPP_