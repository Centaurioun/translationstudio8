// win_x86_Series.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "net_heartsome_license_Series.h"
#include <comutil.h>
#include <Wbemidl.h>
#include <tchar.h>
#include <strsafe.h>
#include <algorithm>
#include <atlconv.h>
#include <ntddndis.h>
#include <winioctl.h>
#include <windows.h>

#pragma comment (lib, "comsuppw.lib")
#pragma comment (lib, "wbemuuid.lib")

#ifndef MACRO_T_DEVICE_PROPERTY
	#define MACRO_T_DEVICE_PROPERTY

	#define PROPERTY_MAX_LEN	128	// �����ֶ���󳤶�
	typedef struct _T_DEVICE_PROPERTY
	{
		TCHAR szProperty[PROPERTY_MAX_LEN];
	} T_DEVICE_PROPERTY;
#endif

#define WMI_QUERY_TYPENUM	3	// WMI��ѯ֧�ֵ�������

typedef struct _T_WQL_QUERY
{
	CHAR*	szSelect;		// SELECT���
	WCHAR*	szProperty;		// �����ֶ�
} T_WQL_QUERY;

// WQL��ѯ���
const T_WQL_QUERY szWQLQuery[] = {
	// ������ǰMAC��ַ
	"SELECT * FROM Win32_NetworkAdapter WHERE (MACAddress IS NOT NULL) AND (NOT (PNPDeviceID LIKE 'ROOT%'))",
	L"PNPDeviceID",

	// �������к�
	"SELECT * FROM Win32_BaseBoard WHERE (SerialNumber IS NOT NULL)",
	L"SerialNumber",

	// BIOS���к�
	"SELECT * FROM Win32_BIOS WHERE (SerialNumber IS NOT NULL)",
	L"SerialNumber"
};

static BOOL WMI_DoWithPNPDeviceID( const TCHAR *PNPDeviceID, TCHAR *MacAddress, UINT uSize )
{
	TCHAR	DevicePath[MAX_PATH];
	HANDLE	hDeviceFile;	
	BOOL	isOK = FALSE;

	// �����豸·����
	StringCchCopy( DevicePath, MAX_PATH, TEXT("\\\\.\\") );
	StringCchCat( DevicePath, MAX_PATH, PNPDeviceID );
	StringCchCat( DevicePath, MAX_PATH, TEXT("#{ad498944-762f-11d0-8dcb-00c04fc3358c}") );

	// ����PNPDeviceID���еġ�\���滻�ɡ�#�����Ի���������豸·����
	std::replace( DevicePath + 4, DevicePath + 4 + _tcslen(PNPDeviceID), TEXT('\\'), TEXT('#') ); 

	// ��ȡ�豸���
	hDeviceFile = CreateFile( DevicePath,
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if( hDeviceFile != INVALID_HANDLE_VALUE )
	{	
		ULONG	dwID;
		BYTE	ucData[8];
		DWORD	dwByteRet;		

		// ��ȡ����ԭ��MAC��ַ
		dwID = OID_802_3_PERMANENT_ADDRESS;
		isOK = DeviceIoControl( hDeviceFile, IOCTL_NDIS_QUERY_GLOBAL_STATS, &dwID, sizeof(dwID), ucData, sizeof(ucData), &dwByteRet, NULL );
		if( isOK )
		{	// ���ֽ�����ת����16�����ַ���
			for( DWORD i = 0; i < dwByteRet; i++ )
			{
				StringCchPrintf( MacAddress + (i << 1), uSize - (i << 1), TEXT("%02X"), ucData[i] );
			}
		}

		CloseHandle( hDeviceFile );
	}

	return isOK;
}

static BOOL WMI_DoWithProperty(INT iQueryType, TCHAR *szProperty, UINT uSize )
{
	BOOL isOK = TRUE;

	if (iQueryType == 0) {
		isOK = WMI_DoWithPNPDeviceID( szProperty, szProperty, uSize );
	}

	// ȥ��ð��
	std::remove( szProperty, szProperty + _tcslen(szProperty) + 1, L':' );
	// ȥ���ո�
	std::remove( szProperty, szProperty + _tcslen(szProperty) + 1, L' ' );

	return isOK;
}

// ����Windows Management Instrumentation��Windows����淶��
INT WMI_DeviceQuery( INT iQueryType, T_DEVICE_PROPERTY *properties, INT iSize )
{
	HRESULT hres;
	INT	iTotal = 0;
	
	// �жϲ�ѯ�����Ƿ�֧��
	if( (iQueryType < 0) || (iQueryType >= sizeof(szWQLQuery)/sizeof(T_WQL_QUERY)) )
	{
		return -1;	// ��ѯ���Ͳ�֧��
	}

    // ��ʼ��COM
    hres = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED ); 
    if( FAILED(hres) )
    {
        return -2;
    }

    // ����COM�İ�ȫ��֤����
	hres = CoInitializeSecurity( 
		NULL, 
		-1, 
		NULL, 
		NULL, 
		RPC_C_AUTHN_LEVEL_DEFAULT, 
		RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
		);
	if(hres != S_OK && hres != RPC_E_TOO_LATE)
    {
        CoUninitialize();
        return -2;
    }
    
	// ���WMI����COM�ӿ�
    IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance( 
		CLSID_WbemLocator,             
        NULL, 
        CLSCTX_INPROC_SERVER, 
        IID_IWbemLocator,
		reinterpret_cast<LPVOID*>(&pLoc)
		); 
    if( FAILED(hres) )
    {
		CoUninitialize();
        return -2;
    }

    // ͨ�����ӽӿ�����WMI���ں˶�����"ROOT\\CIMV2"
	IWbemServices *pSvc = NULL;
	hres = pLoc->ConnectServer(
         _bstr_t( L"ROOT\\CIMV2" ),
         NULL,
         NULL,
         NULL,
         0,
         NULL,
         NULL,
         &pSvc
		 );    
    if( FAILED(hres) )
    {
		pLoc->Release(); 
        CoUninitialize();
        return -2;
    }

	// �����������İ�ȫ����
    hres = CoSetProxyBlanket(
		pSvc,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
		);
	if( FAILED(hres) )
    {
        pSvc->Release();
        pLoc->Release();     
        CoUninitialize();
        return -2;
    }

    // ͨ�������������WMI��������
    IEnumWbemClassObject *pEnumerator = NULL;
    hres = pSvc->ExecQuery(
		bstr_t("WQL"), 
		bstr_t( szWQLQuery[iQueryType].szSelect ),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
        NULL,
        &pEnumerator
		);
	if( FAILED(hres) )
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return -3;
    }

    // ѭ��ö�����еĽ������  
    while( pEnumerator )
    {
		IWbemClassObject *pclsObj = NULL;
		ULONG uReturn = 0;

		if( (properties != NULL) && (iTotal >= iSize) )
		{
			break;
		}

        pEnumerator->Next(
			WBEM_INFINITE,
			1, 
            &pclsObj,
			&uReturn
			);

        if( uReturn == 0 )
        {
			StringCchCopy( properties[iTotal].szProperty, PROPERTY_MAX_LEN, W2T(L"None") );
            break;
        }

		if( properties != NULL )
		{	// ��ȡ����ֵ
			VARIANT vtProperty;
			
			VariantInit( &vtProperty );	
			pclsObj->Get( szWQLQuery[iQueryType].szProperty, 0, &vtProperty, NULL, NULL );
			StringCchCopy( properties[iTotal].szProperty, PROPERTY_MAX_LEN, W2T(vtProperty.bstrVal) );
			VariantClear( &vtProperty );
			
			// ������ֵ����һ���Ĵ���
			if( WMI_DoWithProperty(iQueryType, properties[iTotal].szProperty, PROPERTY_MAX_LEN ) )
			{
				iTotal++;
			}
		}
		else
		{
			iTotal++;
		}

		pclsObj->Release();
    } // End While

    // �ͷ���Դ
	pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();    
    CoUninitialize();

    return iTotal;
}

JNIEXPORT jstring JNICALL Java_net_heartsome_license_Series_getSeries
  (JNIEnv *env, jobject obj) {
	T_DEVICE_PROPERTY properties1;
	WMI_DeviceQuery(1, &properties1, 1);

	T_DEVICE_PROPERTY properties2;
	WMI_DeviceQuery(2, &properties2, 1);

	//��ȡ�ֽڳ���  
	int iLength1 = WideCharToMultiByte(CP_ACP, 0, properties1.szProperty, -1, NULL, 0, NULL, NULL);
	int iLength2 = WideCharToMultiByte(CP_ACP, 0, properties2.szProperty, -1, NULL, 0, NULL, NULL);
	int length = 0;

	char * _char1 = new char[iLength1];
	char * _char2 = new char[iLength2];

	WideCharToMultiByte(CP_ACP, 0, properties1.szProperty, -1, _char1, iLength1, NULL, NULL); 
	if (strcmp(_char1, "None") == 0) {
		length += 10;
	} else {
		length += iLength1;
	}

	length += 1;

	WideCharToMultiByte(CP_ACP, 0, properties2.szProperty, -1, _char2, iLength2, NULL, NULL); 
	if (strcmp(_char2, "None") == 0) {
		length += 10;
	} else {
		length += iLength2;
	}
	length += 1;

	T_DEVICE_PROPERTY *properties0 = new T_DEVICE_PROPERTY[10];
	int size = WMI_DeviceQuery(0, properties0, 10);
	length += size * 13;

	char * temp = new char[length];

	if (strcmp(_char1, "None") == 0) {
		strcpy(temp, "s4j3U78Iiq");
	} else {
		strcpy(temp, _char1);
	}
	strcat(temp, "%");

	if (strcmp(_char2, "None") == 0) {
		strcat(temp, "kl9UNLu81b");
	} else {
		strcat(temp, _char2);
	}
	strcat(temp, "%");

	for (int i = 0 ; i < size ; i++) {
		int iLength = WideCharToMultiByte(CP_ACP, 0, properties0[i].szProperty, -1, NULL, 0, NULL, NULL);
		char * _char = new char[iLength];
		
		//��tcharֵ����_char   
		WideCharToMultiByte(CP_ACP, 0, properties0[i].szProperty, -1, _char, iLength, NULL, NULL); 
		if (strcmp(_char, "None") == 0) {
			strcat(temp, "d4r6i84oe067");
		} else {
			strcat(temp, _char);
		}
		if (i < size - 1) {
			strcat(temp, "+");
		}
	}

	return env->NewStringUTF(temp);
}


