#include "CsvReader.h"
#include <cstring>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#else
#error "This code is platform dependent and requires Windows."
#endif

CCsvReader::CCsvReader(const char* szFileName, bool usePath) {
	if (!usePath) {
		// Constructing full path from executable directory
		char moduleName[MAX_PATH];
		char csvPath[MAX_PATH];
		char* tempPointer;

		GetModuleFileNameA(NULL, moduleName, MAX_PATH);
		tempPointer = strrchr(moduleName, '\\');
		*tempPointer = '\0';
		strcpy_s(csvPath, MAX_PATH, moduleName);
		strcat_s(csvPath, MAX_PATH, "\\");
		strcat_s(csvPath, MAX_PATH, szFileName);

		// Copying the constructed path to m_szFileName
		strcpy_s(m_szFileName, MAX_PATH, csvPath);
	}
	else {
		// Using provided path directly
		strcpy_s(m_szFileName, MAX_PATH, szFileName);
	}
}

bool CCsvReader::OpenCsv() {
	m_csvFile.open(m_szFileName);
	return m_csvFile.is_open();
}

std::vector<std::string> CCsvReader::ReadLine() {
	std::vector<std::string> columns;
	std::string line;
	if (std::getline(m_csvFile, line)) {
		std::stringstream ss(line);
		std::string column;
		while (std::getline(ss, column, ',')) { // ַהוסü ',' - נאחהוכטעוכü סעמכבצמג ג CSV
			columns.push_back(column);
		}
	}
	return columns;
}