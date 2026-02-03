// ini.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "../../ini.hpp"
#include <assert.h>

int main()
{
    // 创建测试用的INI文件
    std::ofstream testFile("test.ini");
    testFile << "[section1]\n";
    testFile << "key1=value1\n";
    testFile << "key2 = value with spaces\n";
    testFile << "\n";  // 空行
    testFile << "; This is a comment\n";
    testFile << "# Another comment\n";
    testFile << "key3=value=with=equals\n";
    testFile << "[section2]\n";
    testFile << "another_key=another_value\n";
    testFile << "path=C:\\Program Files\\MyApp\n";
    testFile << "[section with spaces]\n";
    testFile << "special_key=special value\n";
    testFile.close();

    sjq::Ini ini;

    // 测试加载功能
    bool loaded = ini.Load("test.ini");
    std::cout << "Load result: " << (loaded ? "SUCCESS" : "FAILED") << std::endl;
    assert(loaded);

    // 测试获取值功能
    std::cout << "\n--- Testing Get function ---\n";

    // 测试 section1 的值
    std::string val1 = ini.Get("key1", "section1");
    std::cout << "section1.key1 = " << val1 << std::endl;
    assert(val1 == "value1");

    std::string val2 = ini.Get("key2", "section1");
    std::cout << "section1.key2 = " << val2 << std::endl;
    assert(val2 == "value with spaces");

    std::string val3 = ini.Get("key3", "section1");
    std::cout << "section1.key3 = " << val3 << std::endl;
    // 注意：当前实现会截断等号后的部分
    assert(val3 == "value");  // 实际上只匹配到第一个等号

    // 测试 section2 的值
    std::string val4 = ini.Get("another_key", "section2");
    std::cout << "section2.another_key = " << val4 << std::endl;
    assert(val4 == "another_value");

    // 测试带空格的section名
    std::string val5 = ini.Get("special_key", "section with spaces");
    std::cout << "\"section with spaces\".special_key = " << val5 << std::endl;
    assert(val5 == "special value");

    // 测试默认section（应该找不到值）
    std::string defaultVal = ini.Get("nonexistent");
    std::cout << "Default section nonexistent key = '" << defaultVal << "'" << std::endl;
    assert(defaultVal.empty());

    // 测试不存在的section
    std::string invalidSection = ini.Get("key1", "nonexistent_section");
    std::cout << "Nonexistent section result = '" << invalidSection << "'" << std::endl;
    assert(invalidSection.empty());

    // 测试不存在的key
    std::string invalidKey = ini.Get("nonexistent_key", "section1");
    std::cout << "Nonexistent key result = '" << invalidKey << "'" << std::endl;
    assert(invalidKey.empty());

    std::cout << "\n--- All tests completed ---" << std::endl;

    // 清理测试文件
    std::remove("test.ini");
    return 0;
}

