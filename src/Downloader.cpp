#include "Downloader.h"
#include "Globals.h"

#include <iostream>
#include <sstream>
#include <wchar.h>
#include <iomanip>

namespace Helper {
	bool Downloader::Init() {
		//Start a libCurl easy session for this download
		bool	 bRetval  = false;
		CURLcode curlCode = CURLE_OK;

		try {
			m_phCURL = curl_easy_init();
			if( !m_phCURL ) {
				std::string strMessage( "Something went wrong while starting up a download session for the URL: " );
				strMessage += GetURL();
				throw std::runtime_error( strMessage );
			}

			//Setup callback for writing data.
			curlCode = curl_easy_setopt( m_phCURL, CURLOPT_WRITEFUNCTION, Downloader::WriteData );
			if( 0 != curlCode ) {
				std::string strMessage( "Something went wrong while setting up Callback function for the URL: " );
				strMessage += GetURL();
				throw std::runtime_error( strMessage );
			}

			std::string strDownloadFileName = m_strDownloadFolder + "\\" + m_strDownloadFile;

			errno_t err = fopen_s( &m_pFile, strDownloadFileName.c_str(), "wb" );
			if( 0 != err ) {
				std::string strMessage( "Failed to open file for downloading the URL: " );
				strMessage += GetURL();
				throw std::runtime_error( strMessage );
			}
			//Setup User data to pass to callback.
			curlCode = curl_easy_setopt( m_phCURL, CURLOPT_WRITEDATA, m_pFile );
			if( 0 != curlCode ) {
				std::string strMessage( "Something went wrong while setting up UserData to pass to the callback for the URL: " );
				strMessage += GetURL();
				throw std::runtime_error( strMessage );
			}

			//Setup the URL to work on.
			curlCode = curl_easy_setopt( m_phCURL, CURLOPT_URL, GetURL().c_str() );
			if( 0 != curlCode ) {
				std::string strMessage( "Something went wrong while setting up the URL to download. URL: " );
				strMessage += GetURL();
				throw std::runtime_error( strMessage );
			}

			//specify to follow redirection
			curlCode = curl_easy_setopt( m_phCURL, CURLOPT_FOLLOWLOCATION, 1L );
			if( 0 != curlCode ) {
				std::string strMessage( "Something went wrong while setting up redirections. URL: " );
				strMessage += GetURL();
				throw std::runtime_error( strMessage );
			}

			//Setup a private data for the easy handle
			curlCode = curl_easy_setopt( m_phCURL, CURLOPT_PRIVATE, this );
			if( 0 != curlCode ) {
				std::string strMessage( "Something went wrong while setting up private information to the easy handle. URL: " );
				strMessage += GetURL();
				throw std::runtime_error( strMessage );
			}

			////Switch on custom progress callbacks
			curlCode = curl_easy_setopt( m_phCURL, CURLOPT_NOPROGRESS, 0L );
			if( 0 != curlCode ) {
				std::string strMessage( "Something went wrong while turning on the Progrss meter for the URL: " );
				strMessage += GetURL();
				throw std::runtime_error( strMessage );
			}

			//setup the progress callback progress function
			curlCode = curl_easy_setopt( m_phCURL, CURLOPT_XFERINFOFUNCTION, Downloader::ProgressCallback );
			if( 0 != curlCode ) {
				std::string strMessage( "Something went wrong while setting up the progress callback for the URL: " );
				strMessage += GetURL();
				throw std::runtime_error( strMessage );
			}

			//setup the progress callback progress function
			curlCode = curl_easy_setopt( m_phCURL, CURLOPT_XFERINFODATA, this );
			if( 0 != curlCode ) {
				std::string strMessage( "Something went wrong while specifiyg the user data for progress callback the URL: " );
				strMessage += GetURL();
				throw std::runtime_error( strMessage );
			}

			//Also get a location 
			bRetval = true;
		}
		catch( std::runtime_error& oError ) {
			std::cerr << oError.what() << std::endl;
			m_eStatus = Status::FAILED_TO_START;
		}
		catch( ... ) {
			std::string strMessage( "Something went wrong while starting up a download session for the URL: " );
			strMessage += GetURL();
			std::cerr << strMessage << std::endl;
			m_eStatus = Status::FAILED_TO_START;
		}

		return bRetval;
	}

	bool Downloader::Uninit() {
		curl_easy_cleanup( m_phCURL );
		m_phCURL = NULL;

		if( m_pFile ) {
			fclose( m_pFile );
			m_pFile = NULL;
		}

		if( m_eStatus != Status::COMPLETED ) {
			//Delete the file
			std::string strDownloadFileName = m_strDownloadFolder + "\\" + m_strDownloadFile;
			std::remove( strDownloadFileName.c_str() );
		}
		return true;
	}

	size_t Downloader::WriteData( char* pData, size_t nSize, size_t nMemb, FILE* pStream ) {
		return fwrite( pData, nSize, nMemb, pStream );
	}

	int Downloader::ProgressCallback( void* pClient, curl_off_t dlTotal, curl_off_t dlNow, curl_off_t /*ulTotal*/, curl_off_t /*ulNow*/ ) {
		int			nRetval = 0;
		Downloader* pDownloader = reinterpret_cast< Downloader* >( pClient );
		if( pDownloader ) {
			//From pDownloader we get the row on which data is being written.
			//Then we move to that row in the console and write data.

			if( dlTotal > 0 ) {
				//TODO::Check for disk space.
				
				double dVal = 0.0;
				CURLcode res = curl_easy_getinfo( pDownloader->GetCURL(), CURLINFO_SPEED_DOWNLOAD, &dVal );
				if( CURLE_OK == res && dVal > 0 ) {
					float flPercent = static_cast< float >( ( static_cast< double >( dlNow ) / static_cast< double >( dlTotal ) ) * 100 );
					std::ostringstream oSStream;
					oSStream << pDownloader->m_strDownloadFile << ": " << std::fixed << std::setprecision( 2 ) << flPercent << " % @ " << dVal / 1024 << "KPbs";
					pDownloader->m_eStatus = Status::INPROGRESS;
					Globals::WriteToConsole( pDownloader->m_hStdOut, pDownloader->m_coordStart, oSStream.str() );
				}
			}
		}
		return nRetval;
	}
}//namespace Helper