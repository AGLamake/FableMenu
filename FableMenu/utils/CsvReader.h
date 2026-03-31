#pragma once

#include <fstream>
#include <string>
#include <vector>

class CCsvReader {
private:
	char m_szFileName[255];
	std::ifstream m_csvFile;

public:
	CCsvReader(const char* szFileName, bool usePath);
	bool OpenCsv();
	std::vector<std::string> ReadLine();
};