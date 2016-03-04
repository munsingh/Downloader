#pragma once

#include <string>
#include <Windows.h>

namespace Globals {
	/*
	 * TBD
	 */
	bool	GetDownloadsFolder( std::wstring& strFolder );
	bool	GetDownloadsFolder( std::string& strFolderA );
	bool	UTF16ToUTF8( const std::wstring& strTextW, std::string& strTextA );
	bool	UTF8ToUTF16( const std::string& strTextA, std::wstring& strTextW );
	bool	GetFilenameFromURL( const std::string& strURL, std::string& strFileName );

	//Console functions
	bool	CLS( HANDLE hConsole );
	bool	WriteToConsole( HANDLE hConsole, COORD coordOutput, const std::string& strText );
}