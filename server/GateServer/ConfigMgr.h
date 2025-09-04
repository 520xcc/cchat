#pragma once
#include "const.h"
#include <map>

//内部的键值
/*
外部-->（键）[GateServer]   （值）内部-->（键）Port = （值）8080    
[VarifyServer]   Port = 50051   
*/
struct SectionInfo {
	SectionInfo() {}
	~SectionInfo() {
		_section_datas.clear();
	}

	//拷贝构造
	SectionInfo(const SectionInfo& src) {
		_section_datas = src._section_datas;
	}
	//拷贝赋值运算符
	//修改没有返回值的问题
	SectionInfo& operator = (const SectionInfo& src) {
		if (&src != this) {
			this->_section_datas = src._section_datas;
		}
		return *this;
	}

	std::map<std::string, std::string> _section_datas;
	//重载[]运算符，希望通过section[key] = value(直接取到value)
	std::string  operator[](const std::string& key) {
		//没找到返回“”
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}
		//找到，返回哈希表对应的值
		// 这里可以添加一些边界检查  
		return _section_datas[key];
	}
};

class ConfigMgr
{
public:
	~ConfigMgr() {
		_config_map.clear();
	}
	//重载[]运算符  
	SectionInfo operator[](const std::string& section) {
		if (_config_map.find(section) == _config_map.end()) {
			return SectionInfo();
		}
		return _config_map[section];
	}
	
	//拷贝赋值运算符
	ConfigMgr& operator=(const ConfigMgr& src) = delete;

	//拷贝构造
	ConfigMgr(const ConfigMgr& src) = delete;

	static ConfigMgr& Inst() {
		static ConfigMgr cfr_mgr;//局部静态变量只见与局部作用域
		return cfr_mgr;
	}
private:
	//构造函数
	ConfigMgr();
	// 存储section和key-value对的map  
	std::map<std::string, SectionInfo> _config_map;
};
