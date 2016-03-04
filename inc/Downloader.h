#pragma once

#include "URL.h"

#include <curl/multi.h>
namespace Helper {
	enum class Status { YET_TO_START, FAILED_TO_START, INPROGRESS, COMPLETED, ABORTED };
	class Downloader {
		public:
			Downloader( const URL& oURL, std::string& strDownloadFolder, HANDLE hStdOut, short nRow ) 
					: m_oURL( oURL ), 
					  m_phCURL( NULL ), 
					  m_eStatus( Status::YET_TO_START ), 
					  m_pFile( NULL ), 
					  m_strDownloadFile("" ),
					  m_strDownloadFolder( strDownloadFolder ),
					  m_hStdOut( hStdOut )  {
				m_coordStart = { 0, nRow };
				m_oURL.GetDownloadFileName( m_strDownloadFile );
			};

			~Downloader(){ Uninit(); };

			bool	Init();
			bool	Uninit();

			void				SetStatus( Status eStatus ) { m_eStatus = eStatus; };
			Status				GetStatus() const			{ return m_eStatus; };
			const std::string&	GetURL() const				{ return m_oURL.GetURL(); };
			CURL*				GetCURL() const				{ return m_phCURL; };
		private:
			Downloader();
			Downloader( const Downloader& );
			Downloader& operator=( const Downloader& );

			static size_t	WriteData( char* pData, size_t nSize, size_t nMemb, FILE* pUserData );
			static int		ProgressCallback( void* pClient, curl_off_t dlTotal, curl_off_t dlNow, curl_off_t ulTotal, curl_off_t ulNow );

			URL			m_oURL;
			CURL*		m_phCURL;
			Status		m_eStatus;
			FILE*		m_pFile;
			std::string	m_strDownloadFile;
			std::string	m_strDownloadFolder;
			HANDLE		m_hStdOut;
			COORD		m_coordStart;
	};//class Downloader
}//namespace Helper