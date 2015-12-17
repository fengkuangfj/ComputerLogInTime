// ComputerLogInTime.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#define EVENT_ARRAY_SIZE 1

BOOL
	PrintTime(
	__in LPTSTR pRenderedContent
	)
{
	BOOL	bRet				= FALSE;

	LPTSTR	lpPosition			= NULL;
	TCHAR	tchTime[MAX_PATH]	= {0};
	TCHAR	tchTemp[MAX_PATH]	= {0};
	int		nHour				= 0;


	__try
	{
		if (!pRenderedContent)
			__leave;

		lpPosition = StrRStrIW(pRenderedContent, NULL, _T("<TimeCreated SystemTime='"));
		if (!lpPosition)
			__leave;

		CopyMemory(tchTime, lpPosition + _tcslen(_T("<TimeCreated SystemTime=\'")), 10 * sizeof(TCHAR));
		CopyMemory(tchTime + 10, _T(" "), 1 * sizeof(TCHAR));

		CopyMemory(tchTemp, lpPosition + _tcslen(_T("<TimeCreated SystemTime=\'")) + 11, 2 * sizeof(TCHAR));
		nHour = _wtoi(tchTemp) + 8;
		if (nHour > 24)
			nHour -= 24;

		ZeroMemory(tchTemp, sizeof(tchTemp));
		_itow_s(nHour, tchTemp, _countof(tchTemp), 10);

		if (nHour < 10)
		{
			CopyMemory(tchTime + 11, _T("0"), 1 * sizeof(TCHAR));
			CopyMemory(tchTime + 12, tchTemp, 1 * sizeof(TCHAR));
		}
		else
			CopyMemory(tchTime + 11, tchTemp, 2 * sizeof(TCHAR));

		CopyMemory(tchTime + 13, lpPosition + _tcslen(_T("<TimeCreated SystemTime=\'")) + 13, 6 * sizeof(TCHAR));

		printf("%S \n", tchTime);

		bRet = TRUE;
	}
	__finally
	{
		;
	}

	return bRet;
}

DWORD
	PrintEvent(
	__in EVT_HANDLE hEvent
	)
{
	DWORD status = ERROR_SUCCESS;
	DWORD dwBufferSize = 0;
	DWORD dwBufferUsed = 0;
	DWORD dwPropertyCount = 0;
	LPWSTR pRenderedContent = NULL;


	if (!EvtRender(
		NULL,
		hEvent,
		EvtRenderEventXml,
		dwBufferSize,
		pRenderedContent,
		&dwBufferUsed,
		&dwPropertyCount
		))
	{
		if (ERROR_INSUFFICIENT_BUFFER == (status = GetLastError()))
		{
			dwBufferSize = dwBufferUsed;
			pRenderedContent = (LPWSTR)malloc(dwBufferSize);
			if (pRenderedContent)
			{
				EvtRender(
					NULL,
					hEvent,
					EvtRenderEventXml,
					dwBufferSize,
					pRenderedContent,
					&dwBufferUsed,
					&dwPropertyCount
					);
			}
			else
			{
				wprintf(L"malloc failed\n");
				status = ERROR_OUTOFMEMORY;
				goto cleanup;
			}
		}

		if (ERROR_SUCCESS != (status = GetLastError()))
		{
			wprintf(L"EvtRender failed with %d\n", GetLastError());
			goto cleanup;
		}
	}

	// wprintf(L"\n\n%s", pRenderedContent);
	PrintTime(pRenderedContent);

cleanup:

	if (pRenderedContent)
		free(pRenderedContent);

	return status;
}

BOOL
	GetLogInTime()
{
	BOOL			bRet							= FALSE;

	EVT_HANDLE		EvtHandle						= NULL;
	TCHAR			tchPath[MAX_PATH]				= {0};
	TCHAR			tchQuery[MAX_PATH]				= {0};
	EVT_HANDLE		EventArray[EVENT_ARRAY_SIZE]	= {0};
	DWORD			dwReturned						= 0;
	DWORD			i								= 0;
	PEVT_VARIANT	pEvtVariant						= NULL;
	DWORD			dwPropertyValueBufferUsed		= 0;
	int				nCount							= 0;


	__try
	{
		// HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\eventlog\Application
		// File
		// %SystemRoot%\system32\winevt\Logs\Application.evtx

		_tcscat_s(tchPath, _countof(tchPath), _T("%SystemRoot%\\system32\\winevt\\Logs\\Application.evtx"));
		_tcscat_s(tchQuery, _countof(tchQuery), _T("Event/System[EventID=4101]"));

		EvtHandle = EvtQuery(
			NULL,
			tchPath,
			tchQuery,
			EvtQueryFilePath | EvtQueryReverseDirection | EvtQueryTolerateQueryErrors
			);
		if (!EvtHandle)
		{
			printf("EvtQuery failed. (%d) \n", GetLastError());
			__leave;
		}

		do 
		{
			if (!EvtNext(
				EvtHandle,
				EVENT_ARRAY_SIZE,
				EventArray,
				INFINITE,
				0,
				&dwReturned
				))
			{
				if (ERROR_NO_MORE_ITEMS != GetLastError())
				{
					printf("EvtNext failed. (%d) \n", GetLastError());
					__leave;
				}

				break;
			}

			for (i = 0; i < dwReturned; i++)
			{
				if (pEvtVariant)
				{
					free(pEvtVariant);
					pEvtVariant = NULL;
				}

				if (!EvtGetEventInfo(
					EventArray[i],
					EvtEventPath,
					0,
					pEvtVariant,
					&dwPropertyValueBufferUsed
					))
				{
					if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
					{
						printf("EvtGetLogInfo first failed. (%d) \n", GetLastError());
						__leave;
					}

					pEvtVariant = (PEVT_VARIANT)calloc(1, dwPropertyValueBufferUsed);
					if (!pEvtVariant)
					{
						printf("calloc failed. (%d) \n", GetLastError());
						__leave;
					}

					if (!EvtGetEventInfo(
						EventArray[i],
						EvtEventPath,
						dwPropertyValueBufferUsed,
						pEvtVariant,
						&dwPropertyValueBufferUsed
						))
					{
						printf("EvtGetLogInfo second failed. (%d) \n", GetLastError());
						__leave;
					}

					PrintEvent(EventArray[i]);

					nCount++;
					if (1 == nCount)
						__leave;

					if (EventArray[i])
					{
						EvtClose(EventArray[i]);
						EventArray[i] = NULL;
					}
				}
			}
		} while (TRUE);

		bRet = TRUE;
	}
	__finally
	{
		for (i = 0; i < dwReturned; i++)
		{
			if (EventArray[i])
			{
				EvtClose(EventArray[i]);
				EventArray[i] = NULL;
			}
		}

		if (pEvtVariant)
		{
			free(pEvtVariant);
			pEvtVariant = NULL;
		}
	}

	return bRet;
}

BOOL
	UseIADsUser()
{
	BOOL		bRet			= FALSE;

	HRESULT		hResult			= S_FALSE;
	IID			IID_IADsUser	= {0x3E37E320, 0x17E2, 0x11CF, {0xAB, 0xC4, 0x02, 0x60, 0x8C, 0x9E, 0x75, 0x53}};
	IADsUser *	pIADsUser		= NULL;
	VARIANT		vLoginHours		= {0};
	DATE		Date			= 0;


	__try
	{
		CoInitialize(NULL);

		hResult = ADsGetObject(_T("WinNT://irtouch.local/YueXiang"), IID_IADsUser, (VOID**)&pIADsUser);
		// hResult = ADsGetObject(_T("WinNT://./Administrator"), IID_IADsUser, (VOID**)&pIADsUser);
		if (FAILED(hResult)) 
			__leave;

		if (!pIADsUser)
			__leave;

		// hResult = pIADsUser->get_LoginHours(&vLoginHours);
		hResult = pIADsUser->get_LastLogin(&Date);
		if (FAILED(hResult)) 
			__leave;

		bRet = TRUE;
	}
	__finally
	{
		CoUninitialize();
	}

	return bRet;
}

int _tmain(int argc, _TCHAR* argv[])
{
	GetLogInTime();
	// UseIADsUser();

	_getch();

	return 0;
}
