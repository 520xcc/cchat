#pragma once
#include "const.h"
#include <map>

//�ڲ��ļ�ֵ
/*
�ⲿ-->������[GateServer]   ��ֵ���ڲ�-->������Port = ��ֵ��8080    
[VarifyServer]   Port = 50051   
*/
struct SectionInfo {
	SectionInfo() {}
	~SectionInfo() {
		_section_datas.clear();
	}

	//��������
	SectionInfo(const SectionInfo& src) {
		_section_datas = src._section_datas;
	}
	//������ֵ�����
	//�޸�û�з���ֵ������
	SectionInfo& operator = (const SectionInfo& src) {
		if (&src != this) {
			this->_section_datas = src._section_datas;
		}
		return *this;
	}

	std::map<std::string, std::string> _section_datas;
	//����[]�������ϣ��ͨ��section[key] = value(ֱ��ȡ��value)
	std::string  operator[](const std::string& key) {
		//û�ҵ����ء���
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}
		//�ҵ������ع�ϣ���Ӧ��ֵ
		// ����������һЩ�߽���  
		return _section_datas[key];
	}
};

class ConfigMgr
{
public:
	~ConfigMgr() {
		_config_map.clear();
	}
	//����[]�����  
	SectionInfo operator[](const std::string& section) {
		if (_config_map.find(section) == _config_map.end()) {
			return SectionInfo();
		}
		return _config_map[section];
	}
	
	//������ֵ�����
	ConfigMgr& operator=(const ConfigMgr& src) = delete;

	//��������
	ConfigMgr(const ConfigMgr& src) = delete;

	static ConfigMgr& Inst() {
		static ConfigMgr cfr_mgr;//�ֲ���̬����ֻ����ֲ�������
		return cfr_mgr;
	}
private:
	//���캯��
	ConfigMgr();
	// �洢section��key-value�Ե�map  
	std::map<std::string, SectionInfo> _config_map;
};
