#ifndef UNICODE
#define UNICODE
#endif

#include "stdafx.h"
#include "Defines.h"
#include "RegistryModule.h"
#include <iostream>
#include <regex>
#include <limits.h>
#include <sstream>

struct keyAndName {
	HKEY hKey;
	std::wstring keyName;
};

void RegistryModule::checkRegistry(std::vector<REGISTRY_SEARCH_DATA> searchData, std::vector<FindData>* found, std::vector<FailInfo>* fails) {


	GetPrivileges();
	//return;
	std::vector<keyAndName> bases;
	keyAndName helper;
	helper.hKey = HKEY_CLASSES_ROOT;
	helper.keyName = L"HKEY_CLASSES_ROOT";
	bases.push_back(helper);
	helper.hKey = HKEY_CURRENT_USER;
	helper.keyName = L"HKEY_CURRENT_USER";
	bases.push_back(helper);
	helper.hKey = HKEY_LOCAL_MACHINE;
	helper.keyName = L"HKEY_LOCAL_MACHINE";
	bases.push_back(helper);
	helper.hKey = HKEY_USERS;
	helper.keyName = L"HKEY_USERS";
	bases.push_back(helper);
	helper.hKey = HKEY_PERFORMANCE_DATA;
	helper.keyName = L"HKEY_PERFORMANCE_DATA";
	bases.push_back(helper);
	helper.hKey = HKEY_PERFORMANCE_TEXT;
	helper.keyName = L"HKEY_PERFORMANCE_TEXT";
	bases.push_back(helper);
	helper.hKey = HKEY_PERFORMANCE_NLSTEXT;
	helper.keyName = L"HKEY_PERFORMANCE_NLSTEXT";
	bases.push_back(helper);
	helper.hKey = HKEY_CURRENT_CONFIG;
	helper.keyName = L"HKEY_CURRENT_CONFIG";
	bases.push_back(helper);
	helper.hKey = HKEY_DYN_DATA;
	helper.keyName = L"HKEY_DYN_DATA";
	bases.push_back(helper);
	helper.hKey = HKEY_CURRENT_USER_LOCAL_SETTINGS;
	helper.keyName = L"HKEY_CURRENT_USER_LOCAL_SETTINGS";
	bases.push_back(helper);

	for (int i = 0; i < bases.size(); ++i) {
		FindKeyByNameOrValue(searchData, bases[i].hKey, bases[i].keyName, L"", L"", found, fails);
	}

	DropPrivileges();

}

bool RegistryModule::FindKeyByNameOrValue(std::vector<REGISTRY_SEARCH_DATA> searchData, HKEY baseKey, std::wstring baseKeyName, std::wstring name, std::wstring path, std::vector<FindData>* found, std::vector<FailInfo>* fails) {
	HKEY hKey;
	
	
	std::wstring realPath = L"";
	if (path.size() > 0) {
		realPath = path.substr(1);
	}
	LONG result = RegOpenKeyExW(baseKey, realPath.c_str(), 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
	if (result == ERROR_SUCCESS) {
		wchar_t* buffer = new wchar_t[KEY_MAX_LENGTH]; // max size je 255

		DWORD size = KEY_MAX_LENGTH;
		LONG value;
		std::vector<std::wstring> subkeys;
		subkeys.clear();

		// pozri ci ho nehladas
		int i = 0;
		DWORD numofsubkeys = 0;

		for (int k = 0; k < searchData.size(); ++k) {

			if (searchData[k].found)continue;
			if (searchData[k].dataId == REGISTRY_EXACT_DATA) {
				std::wstring p(baseKeyName);
				p.append(path);

				// pozri path/meno
				if ((searchData[k].name.compare(p) == 0) ||
					(searchData[k].name.compare(name) == 0) ||
					(searchData[k].name.compare(L"") == 0)) {


					// pozri value
					// pozeram valueValue a nie value name lebo 
					// v pripade (Default) value v kluci je valueName == ""
					// ale value moze byt setnuty
					if ((searchData[k].valueValue.compare(L"") == 0) &&
						(searchData[k].valueName.compare(L"") == 0)) {
						// no value
						FindData fd;
						fd.id = k;
						fd.data.push_back(p);
						std::wstring ws;
						ws = L"";
						fd.data.push_back(ws);
						fd.data.push_back(ws);
						found->push_back(fd);

					}
					else {
						std::wstring valName;
						valName.clear();
						if (checkValue(hKey, searchData[k], false, &valName)) {

							FindData fd;
							fd.id = k;
							fd.data.push_back(p);
							fd.data.push_back(valName);
							fd.data.push_back(searchData[k].valueValue);
							found->push_back(fd);




						};
					}



				}

			}

			if (searchData[k].dataId == REGISTRY_REGEX_DATA) {
				std::wstring p(baseKeyName);
				p.append(path);
				std::wregex nameRegex;
				nameRegex.assign(searchData[k].name.c_str());

				// pozri path
				if ((std::regex_match(p, nameRegex)) ||
					(std::regex_match(name, nameRegex)) ||
					(searchData[k].name.compare(L"") == 0)) {
					// pozri meno

					// pozri value
					// pozeram valueValue a nie value name lebo 
					// v pripade (Default) value v kluci je valueName == ""
					// ale value moze byt setnuty
					if ((searchData[k].valueValue.compare(L"") == 0) &&
						(searchData[k].valueName.compare(L"") == 0)) {
						// no value
						FindData fd;
						fd.id = k;
						fd.data.push_back(p);
						fd.data.push_back(searchData[k].valueName);
						fd.data.push_back(searchData[k].valueValue);
						found->push_back(fd);

					}
					else {
						
						fflush(stdout);
						std::wstring valName;
						valName.clear();
						if (checkValue(hKey, searchData[k], true, &valName)) {
							FindData fd;
							fd.id = k;
							fd.data.push_back(p);
							fd.data.push_back(valName);
							fd.data.push_back(searchData[k].valueValue);
							found->push_back(fd);

						};
					}



				}

			}

		}

		DWORD retvalinfo = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, &numofsubkeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		while (((value = RegEnumKeyExW(hKey, i, buffer, &size, NULL, NULL, NULL, NULL)) != ERROR_NO_MORE_ITEMS) && (i <= numofsubkeys)) {
			if (value != ERROR_SUCCESS) {
				++i;
				continue;
			}         
			size = KEY_MAX_LENGTH;
			// RegEnumKeyEx size je In_Out pozor vracia sa tam velkost stringu ktory som dostal
			// treba reinicializovat
			++i;
			std::wstring wss = path + L"\\" + buffer;
			FindKeyByNameOrValue(searchData, baseKey, baseKeyName, buffer, wss, found, fails);
		}
		RegCloseKey(hKey);
		delete buffer; 
	}
	else {
		FailInfo fi;
		fi.type = L"Registry";
		fi.data = baseKeyName + path;
		fails->push_back(fi);
	}
	return true;
}

bool RegistryModule::checkValue(HKEY hKey, REGISTRY_SEARCH_DATA data, bool regexp, std::wstring* valName) {
	bool noValue = false;
	bool noName = false;
	
	if (data.valueValue.compare(L"") == 0) noValue = true;
	if (data.valueName.compare(L"") == 0) noName = true;

	DWORD numOfValues;
	RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL, &numOfValues, NULL, NULL, NULL, NULL);

	for (int i = 0; i < numOfValues; ++i) {
		DWORD bufferSize = 16383;
		DWORD dataSize = 0;
		wchar_t* valueName = new wchar_t[bufferSize];

		DWORD valueType;
		RegEnumValueW(hKey, i, valueName, &bufferSize, NULL, &valueType, NULL, &dataSize);
		LPBYTE valueData = new BYTE[dataSize];
		bufferSize = 16383;
		RegEnumValueW(hKey, i, valueName, &bufferSize, NULL, NULL, valueData, &dataSize);


		if (noName == false) {
			if (regexp == false) {
				if (data.valueName.compare(valueName) != 0) {
					delete valueData;
					delete valueName;
					continue;
				}
			}
			else {
				std::wregex valueNameRegex;
				valueNameRegex.assign(data.valueName);
				if (std::regex_match(valueName, valueNameRegex) == false) {
					delete valueData;
					delete valueName;
					continue;
				}

			}
		}
		if (noValue) {
			valName->append(valueName);
			delete valueData;
			delete valueName;
			return true;
		}

		if (compareData(data.valueValue, valueData, dataSize, valueType)) {
			valName->append(valueName);
			delete valueData;
			delete valueName;
			return true;
		};
		delete valueData;
		delete valueName;

	}

	return false;
}

bool RegistryModule::isDWORD(std::wstring s) {
	if (s.size() > 10)return false; // 4294967295 max dword
	if (s.size() == 10)
		if (s.compare(L"4294967295") == -1)return false;
	// realne nemusim kontorlovat
	// ci to je skutocne int staci sa len pozriet ci ma vsetky cifry cisla
	// a ci je pred 4,294,967,295 a ci je kratsi
	// ak je kratsi resp rovny tak bud je to int a vtedy compare vrati 1, resp 0 ak sa rovna
	// ak nie vrati -1
	for (int i = 1; i < s.size(); ++i) {

		if (isdigit(s[i]) == false)return false;
	}
	return true;
}

bool RegistryModule::isQWORD(std::wstring s) {
	if (s.size() > 20)return false; // 18446744073709551615 max qword
	if (s.size() == 20)
		if (s.compare(L"18446744073709551615") == -1)return false;
	// taky isty reasoning ako pri isDword
	for (int i = 1; i < s.size(); ++i) {

		if (isdigit(s[i]) == false)return false;
	}
	return true;
}

int RegistryModule::getInt(wchar_t c) {
	return c - '0';
}
DWORD RegistryModule::toDWORD(std::wstring s) {
	DWORD ret = 0;
	if (isdigit(s[0])) ret = getInt(s[0]);

	for (int i = 1; i < s.length(); ++i) {
		ret *= 10;
		ret += getInt(s[i]);
	}

	return ret;
}

int64_t RegistryModule::toQWORD(std::wstring s) {
	int64_t ret = 0;


	if (isdigit(s[0])) ret = getInt(s[0]);

	for (int i = 0; i < s.length(); ++i) {
		ret *= 10;
		ret += getInt(s[i]);
	}

	return ret;
}

wchar_t RegistryModule::hexToChar(std::wstring byte) {
	int ret = 0;
	for (int i = 0; i < 2; ++i) {
		if (isdigit(byte[i])) {
			if(i == 0){
				ret += getInt(byte[i]) * 16;
			}
			if (i == 1) {
				ret += getInt(byte[i]);
			}
		}
		else {
			if (byte[i] == 'a') {
				if (i == 0) {
					ret += 10 * 16;
				}
				if (i == 1) {
					ret += 10;
				}
			}
			if (byte[i] == 'b') {
				if (i == 0) {
					ret += 11 * 16;
				}
				if (i == 1) {
					ret += 11;
				}
			}
			if (byte[i] == 'c') {
				if (i == 0) {
					ret += 12 * 16;
				}
				if (i == 1) {
					ret += 12;
				}
			}
			if (byte[i] == 'd') {
				if (i == 0) {
					ret += 13 * 16;
				}
				if (i == 1) {
					ret += 13;
				}
			}
			if (byte[i] == 'e') {
				if (i == 0) {
					ret += 14 * 16;
				}
				if (i == 1) {
					ret += 14;
				}
			}
			if (byte[i] == 'f') {
				if (i == 0) {
					ret += 15 * 16;
				}
				if (i == 1) {
					ret += 15;
				}
			}

				
		}
	}
	return (wchar_t)ret;
}
bool RegistryModule::compareData(std::wstring s, unsigned char* data, int dataSize, DWORD valueType) {
	int searchLength = s.size();

	if (valueType == REG_BINARY || REG_NONE) {
		if (s.size() % 2)return false;
		if (dataSize != (searchLength / 2))return false;
		int len = s.length();
		std::wstring compareData;
		for (int i = 0; i< len; i += 2)
		{
			std::wstring byte = s.substr(i, 2);
			 
			wchar_t chr = hexToChar(byte);
			compareData.push_back(chr);
		}
		for (int i = 0; i < dataSize; ++i) {
			if (data[i] != compareData[i])return false;
		}
		return true;
		
	}
	DWORD dwValue;
	if ((valueType == REG_DWORD) || (valueType == REG_DWORD_LITTLE_ENDIAN)) {
		if (isDWORD(s) == false)return false;
		dwValue = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | (data[0]);
		DWORD val = toDWORD(s);
		if (val == dwValue) return true;
		return false;
	}

	if (valueType == REG_DWORD_BIG_ENDIAN) {
		if (isDWORD(s) == false)return false;
		dwValue = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
		DWORD val = toDWORD(s);
		if (val == dwValue) return true;
		return false;
	}

	unsigned long long qwValue; // QWORD nemam header ale def je takyto
	// Windows systemy su littleENdian tkaze je to jedno
	if ((valueType == REG_QWORD) || (valueType == REG_QWORD_LITTLE_ENDIAN)) {
		if (isQWORD(s) == false) return false;
		qwValue = (data[7] << 56) | (data[6] << 48) | (data[5] << 40) | (data[4] << 32) |
			(data[3] << 24) | (data[2] << 16) | (data[1] << 8) | (data[0]);
		unsigned long long val = toQWORD(s);
		if (val == qwValue) return true;
		return false;
	}

	if ((valueType == REG_SZ) || (valueType == REG_LINK) || (valueType == REG_EXPAND_SZ)) {
		
		
		std::wstring vData = std::wstring((const wchar_t*)data);
		if (wcscmp(s.c_str(), vData.c_str()) == 0) return true;
		return false;
	}

	if (valueType == REG_MULTI_SZ) {
		std::vector<std::wstring>multiValData;

		wchar_t* all;
		all = (wchar_t*)data;
		int currSize = 0;
		int realSize = (dataSize / 2) - 1; // /2 -> wchar_t = 2 byte , -1 -> posledny null character
		while (currSize < realSize) {
			int size = wcslen(all);
			multiValData.push_back(all);
			all = &all[size + 1];
			currSize += size + 1;
		}


		std::wstring wsData;
		for (int i = 0; i < multiValData.size(); ++i) {
			wsData.append(multiValData[i]);
			wsData.append(L" ");
		}
		
		s.append(L" ");


		if (wcscmp(s.c_str(), wsData.c_str()) != 0) {
			return false;
		}

		return true;

	}

	return false;

}


int RegistryModule::GetPrivileges() {
	HANDLE hToken;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		return 1;
	}

	if (!SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, TRUE))
	{
		std::wcout << L"Cannot aquire privilege" << std::endl;
		CloseHandle(hToken);
		return 1;
	}
	//std::cout << "done" << std::endl;
	CloseHandle(hToken);
	
	return 0;
}

int RegistryModule::DropPrivileges() {
	HANDLE hProcess;
	HANDLE hToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		return 1;
	}

	if (!SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, FALSE))
	{
		std::wcout << L"Cannot drop privilege" << std::endl;
		CloseHandle(hToken);
		return 1;
	}
	CloseHandle(hToken);
	return 0;
}

BOOL RegistryModule::SetPrivilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege) {
	TOKEN_PRIVILEGES tp;
	LUID luid;
	TOKEN_PRIVILEGES tpPrevious;
	DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);

	if (!LookupPrivilegeValue(NULL, Privilege, &luid)) return FALSE;

	// 
	// first pass.  get current privilege setting
	// 
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = 0;

	AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tp,
		sizeof(TOKEN_PRIVILEGES),
		&tpPrevious,
		&cbPrevious
	);

	if (GetLastError() != ERROR_SUCCESS) return FALSE;

	// 
	// second pass.  set privilege based on previous setting
	// 
	tpPrevious.PrivilegeCount = 1;
	tpPrevious.Privileges[0].Luid = luid;

	if (bEnablePrivilege) {
		tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
	}
	else {
		tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
			tpPrevious.Privileges[0].Attributes);
	}

	AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tpPrevious,
		cbPrevious,
		NULL,
		NULL
	);

	if (GetLastError() != ERROR_SUCCESS) return FALSE;

	return TRUE;
}