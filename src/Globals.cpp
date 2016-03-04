//Project Includes
#include "Globals.h"

//System Includes
#include <Shlobj.h>
#include <Shlwapi.h>

//C++ includes
#include <memory>
#include <vector>

namespace Globals {
	bool GetDownloadsFolder( std::string& strFolderA ) {
		std::wstring strFolderW;
		if( GetDownloadsFolder( strFolderW ) ) {
			return UTF16ToUTF8( strFolderW, strFolderA );
		}

		return false;
	}

	bool GetDownloadsFolder( std::wstring& strFolder ) {
		PWSTR lpszFolder = nullptr;
		HRESULT hr = ::SHGetKnownFolderPath( FOLDERID_Downloads,
											 KF_FLAG_DEFAULT_PATH,
											 NULL,	//for the current user
											 &lpszFolder );
		if( SUCCEEDED( hr ) ) {
			strFolder = lpszFolder;
			::CoTaskMemFree( lpszFolder );
			lpszFolder = nullptr;
		}
		return SUCCEEDED( hr ) ? true : false;
	}

	bool UTF16ToUTF8( const std::wstring& strTextW, std::string& strTextA ) {
		HRESULT hr = E_FAIL;

		try {
			int nNumberOfBytesWritten = ::WideCharToMultiByte( CP_UTF8,
															   WC_ERR_INVALID_CHARS,
															   strTextW.c_str(),
															   -1,
															   NULL,
															   0,
															   NULL,
															   NULL );
			if( 0 == nNumberOfBytesWritten ) {
				//the function failed
				hr = HRESULT_FROM_WIN32( ::GetLastError() );
				strTextA = "";
			}
			else {
				//the function succeeded, i.e. we now know the size of the buffer required

				//pbyBuffer unique_ptr
				std::unique_ptr < BYTE[] > pbyBuffer( new BYTE[ nNumberOfBytesWritten + 1 ] );
				nNumberOfBytesWritten = ::WideCharToMultiByte( CP_UTF8,
															   WC_ERR_INVALID_CHARS,
															   strTextW.c_str(),
															   -1,
															   reinterpret_cast< LPSTR >( pbyBuffer.get() ),
															   nNumberOfBytesWritten + 1,
															   NULL,
															   NULL );

				strTextA = reinterpret_cast< char* >( pbyBuffer.get() );
				hr = S_OK;
			}
		}
		catch( std::bad_alloc& /*oException*/ ) {
			hr = E_OUTOFMEMORY;
		}
		catch( ... ){
			hr = E_UNEXPECTED;
		}

		return SUCCEEDED( hr ) ? true : false;
	}

	bool UTF8ToUTF16( const std::string& strTextA, std::wstring& strTextW ) {
		HRESULT hr = E_FAIL;
		try {

			int nNumberOfBytesWritten = ::MultiByteToWideChar( CP_UTF8,					//Code page to use in performing the conversion
															   MB_ERR_INVALID_CHARS,	//Flags indicating the conversion type, set to 0 for UTF-8
															   strTextA.c_str(),		//Pointer to the character string to convert
															   -1,						//Size, in bytes, of the string indicated by the lpMultiByteStr parameter. Alternatively, this parameter can be set to -1 if the string is null-terminated
															   NULL,					//Pointer to a buffer that receives the converted string, NULL to get the size needed
															   0 );					//Size, in WCHAR values, of the buffer indicated by lpWideCharStr, 0 to get the size needed
			if( 0 == nNumberOfBytesWritten ) {
				//the function failed
				hr = HRESULT_FROM_WIN32( ::GetLastError() );
			}
			else {
				//the function succeeded, i.e. we now know the size of the buffer required

				//pbyBuffer unique_ptr
				std::unique_ptr < WCHAR[] > pbyBuffer( new WCHAR[ nNumberOfBytesWritten + 1 ] );
				nNumberOfBytesWritten = ::MultiByteToWideChar( CP_UTF8,
															   MB_ERR_INVALID_CHARS,
															   strTextA.c_str(),
															   -1,
															   reinterpret_cast< LPWSTR >( pbyBuffer.get() ),
															   nNumberOfBytesWritten + 1 );

				strTextW = reinterpret_cast< WCHAR* >( pbyBuffer.get() );
				hr = S_OK;
			}
		}
		catch( std::bad_alloc& /*oException*/ ) {
			hr = E_OUTOFMEMORY;
		}
		catch( ... ) {
			hr = E_UNEXPECTED;
		}

		return SUCCEEDED( hr ) ? true : false;
	}

	bool GetFilenameFromURL( const std::string& strURL, std::string& strFileName ){
		strFileName = "";
		PARSEDURLA pu;
		pu.cbSize = sizeof( pu );
		HRESULT hr = ::ParseURLA( strURL.c_str(), &pu );
		if( SUCCEEDED( hr ) ) {
			char* pCurrent = const_cast< char* >( pu.pszSuffix );
			std::vector< std::string > oResults;

			do {
				char* pBegin = pCurrent;

				while( *pCurrent != '/' && *pCurrent ) {
					++pCurrent;
				}

				oResults.push_back( std::string( pBegin, pCurrent ) );
			} while( NULL != *pCurrent++ );

			strFileName = oResults.at( oResults.size() - 1 );
			strFileName.assign( strFileName.begin(), strFileName.end() );

			return true;
		}
		else
			return false;
	}

	bool CLS( HANDLE hConsole ) {
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if( 0 == ::GetConsoleScreenBufferInfo( hConsole, &csbi ) ) {
			//failure
			return false;
		}

		DWORD dwConsoleSize = csbi.dwSize.X * csbi.dwSize.Y;
		COORD coordScreen{ 0, 0 };
		DWORD chCharsWritten = 0;
		//Fill the entire screen with blanks
		if( 0 == ::FillConsoleOutputCharacter( hConsole, TEXT( ' ' ), dwConsoleSize, coordScreen, &chCharsWritten ) ) {
			//error;
			return false;
		}

		//Set the buffers' attributes accordinly
		if( 0 == ::FillConsoleOutputAttribute( hConsole, csbi.wAttributes, dwConsoleSize, coordScreen, &chCharsWritten ) ) {
			//error;
			return false;
		}

		//Put the cursor at its home co-ordinates
		::SetConsoleCursorPosition( hConsole, coordScreen );

		return true;
	}

	bool WriteToConsole( HANDLE hConsole, COORD coordOutput, const std::string& strText ) {
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if( 0 == ::GetConsoleScreenBufferInfo( hConsole, &csbi ) ) {
			//failure
			return false;
		}
		
		//Fist set the Console Cursor position
		if( 0 == ::SetConsoleCursorPosition( hConsole, coordOutput ) ) {
			return false;
		}

		//Now write the text at this locaton.
		DWORD dwNumberOfCharsWritten = 0;
		if( 0 == ::WriteConsoleA( hConsole, strText.c_str(), strText.length(), &dwNumberOfCharsWritten, NULL ) ) {
			return false;
		}

		return true;
	}
}