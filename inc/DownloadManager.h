#pragma once

//STD C++ headers
#include <vector>
#include <string>

//Windows headers
#include <Windows.h>

//Curl headers
#include <curl/multi.h>

//Project includes
#include "URL.h"
#include "Downloader.h"

namespace Helper {
	class DownloadManager {
		public:
			DownloadManager( const std::vector< URL >& vecURLs,
							 const std::string& strDownloadDirectory);
			~DownloadManager();

			bool	Start();
			std::vector< URL >::size_type
					GetNumberOfURLsToDownload() const { return m_vecURLs.size(); };
		private:

			static BOOL			ControlHandler( DWORD dwCtrlType );

			bool	InitDownloaders();
			bool	UninitDownloaders( Status eStatus );
			bool	Download();

			bool	RemoveDownloader( CURL* pEasyHandle, Status eStatus );

			//Hidden constructor, copy constructor and assignment operator
			DownloadManager();
			DownloadManager( const DownloadManager& );
			DownloadManager& operator = ( const DownloadManager& );

			std::vector< URL >	m_vecURLs;
			std::string			m_strDownloadDirectory;

			HANDLE				m_hStdOut;
			HANDLE				m_hStdIn;
			DWORD				m_dwConsoleInputMode;
			CURLM*				m_phCURL;
			std::vector< URL >::size_type m_nCurrentPos;
			int					m_nRunningHandles;

			static HANDLE		sm_hExitEvent;
			std::vector< Downloader* > m_vecDownloaders;
	};//class DownloadManager
}//namespace Helper