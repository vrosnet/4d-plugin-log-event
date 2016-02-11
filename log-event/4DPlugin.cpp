/* --------------------------------------------------------------------------------
 #
 #	4DPlugin.cpp
 #	source generated by 4D Plugin Wizard
 #	Project : Log Event
 #	author : miyako
 #	2016/02/11
 #
 # --------------------------------------------------------------------------------*/


#include "4DPluginAPI.h"
#include "4DPlugin.h"

namespace LOG
{
	CUTF16String serverName;
	CUTF16String sourceName;
#if VERSIONWIN
	HANDLE hEventLog = NULL;
	void setDWORD(HKEY hkey, LPCWSTR path, LPCWSTR key, C_LONGINT &v)
	{
		HKEY hk = NULL;
		DWORD value = v.getIntValue();
		if (ERROR_SUCCESS == RegCreateKeyEx(hkey, (LPCWSTR)path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))
		{
			RegSetValueEx(hk, (LPCWSTR)key, 0, REG_DWORD, (const BYTE *)&value, sizeof(DWORD));
			RegCloseKey(hk);
		}
	}
	void setSZ(HKEY hkey, LPCWSTR path, LPCWSTR key, C_TEXT &v)
	{
		HKEY hk = NULL;
		const BYTE *str = (const BYTE *)v.getUTF16StringPtr();
		uint32_t size = (v.getUTF16Length()*sizeof(PA_Unichar)) + sizeof(PA_Unichar);
		if (ERROR_SUCCESS == RegCreateKeyEx(hkey, (LPCWSTR)path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hk, NULL))
		{
			RegSetValueEx(hk, (LPCWSTR)key, 0, REG_SZ, str, size);
			RegCloseKey(hk);
		}
	}
	void registerSource(C_TEXT &SourceName, C_LONGINT &CategoryCount, C_TEXT &MessageFilePath, C_LONGINT &TypesSupported)
	{
		if (SourceName.getUTF16Length())
		{
			CUTF16String s = CUTF16String((const PA_Unichar*)L"SYSTEM\\CurrentControlSet\\services\\eventlog\\Application\\");
			s += SourceName.getUTF16StringPtr();
			LPCWSTR path  =(LPCWSTR)s.c_str();
			LOG::setDWORD(HKEY_LOCAL_MACHINE, path, L"CategoryCount", CategoryCount);
			LOG::setSZ(HKEY_LOCAL_MACHINE, path, L"CategoryMessageFile", MessageFilePath);
			LOG::setSZ(HKEY_LOCAL_MACHINE, path, L"EventMessageFile", MessageFilePath);
			LOG::setSZ(HKEY_LOCAL_MACHINE, path, L"ParameterMessageFile", MessageFilePath);
			LOG::setDWORD(HKEY_LOCAL_MACHINE, path, L"TypesSupported", TypesSupported);
		}
	}
	int setSource(C_TEXT &ServerName, C_TEXT &SourceName)
	{
		int errorCode = 0;
		LPCTSTR lpUNCServerName = (LPCTSTR)(ServerName.getUTF16Length() ? ServerName.getUTF16StringPtr() : NULL);
		LPCTSTR lpSourceName = (LPCTSTR)SourceName.getUTF16StringPtr();		
		HANDLE h = RegisterEventSource(lpUNCServerName, lpSourceName);
	
		if (h)
		{
			if (LOG::hEventLog)
			{
				DeregisterEventSource(LOG::hEventLog);
				LOG::hEventLog = NULL;
			}
			LOG::hEventLog = h;
			LOG::serverName = CUTF16String(ServerName.getUTF16StringPtr());
			LOG::sourceName = CUTF16String(SourceName.getUTF16StringPtr());
		}else
		{
			errorCode = GetLastError();
		}
		
		return errorCode;
	}
#endif
}

#pragma mark -

bool IsProcessOnExit()
{
    C_TEXT name;
    PA_long32 state, time;
    PA_GetProcessInfo(PA_GetCurrentProcessNumber(), name, &state, &time);
    CUTF16String procName(name.getUTF16StringPtr());
    CUTF16String exitProcName((PA_Unichar *)"$\0x\0x\0\0\0");
    return (!procName.compare(exitProcName));
}

void OnStartup()
{
#if VERSIONWIN
	C_TEXT ServerName;
	C_TEXT SourceName;
	ServerName.setUTF16String((const PA_Unichar *)L"", 0);
	SourceName.setUTF16String((const PA_Unichar *)L"4D Application", wcslen(L"4D Application"));
	LOG::setSource(ServerName, SourceName);
#endif
}

void OnCloseProcess()
{
#if VERSIONWIN
	if(IsProcessOnExit())
	{
		if (LOG::hEventLog)
		{
			DeregisterEventSource(LOG::hEventLog);
			LOG::hEventLog = NULL;
		}
	}
#endif
}

#pragma mark -

void PluginMain(PA_long32 selector, PA_PluginParameters params)
{
	try
	{
		PA_long32 pProcNum = selector;
		sLONG_PTR *pResult = (sLONG_PTR *)params->fResult;
		PackagePtr pParams = (PackagePtr)params->fParameters;

		CommandDispatcher(pProcNum, pResult, pParams); 
	}
	catch(...)
	{

	}
}

void CommandDispatcher (PA_long32 pProcNum, sLONG_PTR *pResult, PackagePtr pParams)
{
	switch(pProcNum)
	{
        case kInitPlugin :
        case kServerInitPlugin :            
            OnStartup();
            break;    

        case kCloseProcess :            
            OnCloseProcess();
            break;
// --- Log

		case 1 :
			LOG_WRITE_ENTRY(pResult, pParams);
			break;

		case 2 :
			LOG_REGISTER_SOURCE(pResult, pParams);
			break;

		case 3 :
			LOG_SET_SOURCE(pResult, pParams);
			break;

		case 4 :
			LOG_GET_SOURCE(pResult, pParams);
			break;

	}
}

// -------------------------------------- Log -------------------------------------


void LOG_WRITE_ENTRY(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_LONGINT Type;
	C_LONGINT Category;
	C_LONGINT EventID;
	C_BLOB RawData;
	C_LONGINT ErrorCode;

	Type.fromParamAtIndex(pParams, 1);
	Category.fromParamAtIndex(pParams, 2);
	EventID.fromParamAtIndex(pParams, 3);
	RawData.fromParamAtIndex(pParams, 5);

#if VERSIONWIN

	if (LOG::hEventLog)
	{
		WORD wType = (WORD)Type.getIntValue();
		WORD wCategory = (WORD)Category.getIntValue();
		DWORD dwEventID = (DWORD)EventID.getIntValue();
		PSID lpUserSid = NULL;
		
		DWORD dwDataSize = (DWORD)RawData.getBytesLength();
		LPVOID lpRawData = (LPVOID)RawData.getBytesPtr();
		
		std::vector<LPCTSTR> params;
		PA_Variable arr = *((PA_Variable*) pParams[3]);
		if(arr.fType == eVK_ArrayUnicode)
		{
			for(uint32_t i = 1; i <= (uint32_t)arr.uValue.fArray.fNbElements; ++i)
			{
				PA_Unistring str = (*(PA_Unistring **) (arr.uValue.fArray.fData))[i];
				params.push_back((LPCTSTR)str.fString);
			}
		}
		WORD wNumStrings = params.size();
		LPCTSTR *lpStrings = params.size() ? &params[0] : NULL;
		
		if (!ReportEvent(
					LOG::hEventLog,
					wType,
					wCategory,
					dwEventID,
					lpUserSid,
					wNumStrings,
					dwDataSize,
					lpStrings,
					lpRawData)
			)
    {
			ErrorCode.setIntValue(GetLastError());
		}
		
	}
#endif

	ErrorCode.toParamAtIndex(pParams, 6);
}

void LOG_REGISTER_SOURCE(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT SourceName;
	C_LONGINT CategoryCount;
	C_TEXT MessageFilePath;
	C_LONGINT TypesSupported;

	SourceName.fromParamAtIndex(pParams, 1);
	CategoryCount.fromParamAtIndex(pParams, 2);
	MessageFilePath.fromParamAtIndex(pParams, 3);
	TypesSupported.fromParamAtIndex(pParams, 4);

#if VERSIONWIN
	LOG::registerSource(SourceName, CategoryCount, MessageFilePath, TypesSupported);
#endif

}

void LOG_SET_SOURCE(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT ServerPath;
	C_TEXT SourceName;
	C_LONGINT ErrorCode;

	ServerPath.fromParamAtIndex(pParams, 1);
	SourceName.fromParamAtIndex(pParams, 2);

#if VERSIONWIN
	ErrorCode.setIntValue(LOG::setSource(ServerPath, SourceName));
#endif

	ErrorCode.toParamAtIndex(pParams, 3);
}

void LOG_GET_SOURCE(sLONG_PTR *pResult, PackagePtr pParams)
{
	C_TEXT Param1;
	C_TEXT Param2;

	Param1.setUTF16String(&LOG::serverName);
	Param2.setUTF16String(&LOG::sourceName);

	Param1.toParamAtIndex(pParams, 1);
	Param2.toParamAtIndex(pParams, 2);
}
