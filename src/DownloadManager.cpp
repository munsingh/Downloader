#include "DownloadManager.h"
#include "Downloader.h"
#include "Globals.h"

#include <sstream>
#include <iostream>
#include <process.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

namespace Helper {
	HANDLE DownloadManager::sm_hExitEvent = NULL;

	DownloadManager::DownloadManager( const std::vector< Helper::URL >& vecURLs,
									  const std::string& strDownloadDirectory ) 
					:m_vecURLs(vecURLs), 
					 m_strDownloadDirectory(strDownloadDirectory),
					 m_hStdOut( nullptr ),
					 m_hStdIn( nullptr ),
					 m_dwConsoleInputMode( 0 ),
					 m_phCURL( NULL ),
					 m_nCurrentPos( 0 ),
					 m_nRunningHandles( -1 )
	{
		if( INVALID_HANDLE_VALUE == ( m_hStdOut = ::GetStdHandle( STD_OUTPUT_HANDLE ) ) ) {
			//throw an exception out
			DWORD dwLastError = ::GetLastError();
			std::stringstream oSS;
			oSS << "Failed to retreive the STDOUT handle for the console. Last Error: 0x" << std::hex << dwLastError;
			throw new std::runtime_error( oSS.str() );
		}

		if( INVALID_HANDLE_VALUE == ( m_hStdIn = ::GetStdHandle( STD_INPUT_HANDLE ) ) ) {
			//throw an exception out
			DWORD dwLastError = ::GetLastError();
			std::stringstream oSS;
			oSS << "Failed to retreive the STDIN handle for the console. Last Error: 0x" << std::hex << dwLastError;
			throw new std::runtime_error( oSS.str() );
		}

		if( 0 == ::GetConsoleMode( m_hStdIn, &m_dwConsoleInputMode ) ) {
			//throw an exception out
			DWORD dwLastError = ::GetLastError();
			std::stringstream oSS;
			oSS << "Failed to retreive the current STDIN Console Mode. Last Error: 0x" << std::hex << dwLastError;
			throw new std::runtime_error( oSS.str() );
		}

		if( 0 == ::SetConsoleCtrlHandler( reinterpret_cast< PHANDLER_ROUTINE >( DownloadManager::ControlHandler ), TRUE ) ) {
			//throw an exception out
			DWORD dwLastError = ::GetLastError();
			std::stringstream oSS;
			oSS << "Failed to set CTRL-C/BREAK handler. Last Error: 0x" << std::hex << dwLastError;
			throw new std::runtime_error( oSS.str() );
		}

		if( 0 == ::SetConsoleMode( m_hStdIn, ENABLE_PROCESSED_INPUT | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT ) ) {
			//throw an exception out
			DWORD dwLastError = ::GetLastError();
			std::stringstream oSS;
			oSS << "Failed to retreive the current STDIN Console Mode. Last Error: 0x" << std::hex << dwLastError;
			throw new std::runtime_error( oSS.str() );
		}

		DownloadManager::sm_hExitEvent = ::CreateEvent( NULL, FALSE, FALSE, TEXT( "ExitEvent" ) );
		if( NULL == DownloadManager::sm_hExitEvent ) {
			//Create Event failed, throw an exception
			DWORD dwLastError = ::GetLastError();
			std::stringstream oSS;
			oSS << "Failed to create the exit event. The CreateEvent API failed. Last Error: 0x" << std::hex << dwLastError;
			throw new std::runtime_error( oSS.str() );
		}
		else {
			DWORD dwLastError = ::GetLastError();
			if( ERROR_ALREADY_EXISTS == dwLastError ) {
				std::stringstream oSS;
				oSS << "An ExitEvent already exists. The application may already be running. Last Error: 0x" << std::hex << dwLastError;
				throw new std::runtime_error( oSS.str() );
			}
		}

		//Clear Console
		Globals::CLS( m_hStdOut );
		curl_global_init( CURL_GLOBAL_ALL );
	}

	DownloadManager::~DownloadManager() {
		curl_global_cleanup();
		//close the exit event
		::CloseHandle( DownloadManager::sm_hExitEvent );
		DownloadManager::sm_hExitEvent = NULL;

		//unhook the control handler
		::SetConsoleCtrlHandler( reinterpret_cast< PHANDLER_ROUTINE >( DownloadManager::ControlHandler ), FALSE );
		
		//restore the console mode
		::SetConsoleMode( m_hStdIn, m_dwConsoleInputMode );
		
		m_hStdIn = nullptr;
		m_hStdOut = nullptr;
	}

	bool DownloadManager::Start() {
		bool bRetval = false;
		
		//We are using the CURL Multiple Interface, wherein a single thread
		//can handle multiple connections
		CURLMcode	curlCode = CURLM_BAD_HANDLE;	//intialize to an erro value,
		try {
			m_phCURL = curl_multi_init();
			if( NULL == m_phCURL )
				throw std::runtime_error( "Failed to initialize CURL multi-handle." );

		
			//First we tell the multi-session how many concurrent downloads should occur
			//sets the size of connection cache 
			curlCode = curl_multi_setopt( m_phCURL, CURLMOPT_MAXCONNECTS, GetNumberOfURLsToDownload() );
			if( CURLM_UNKNOWN_OPTION == curlCode )
				throw std::runtime_error( "Unsupported option, while setting CURLMOPT_MAXCONNECTS using curl_multi_setopt" );

			//Now all curl multi options are set
			//So now initilizae the individual Downloaders pDLManager->m_usMaxConcurentDownloads at a time
			//So basically we need to schedule the number of items to be downloaded
			InitDownloaders();

			//Keep downloading unless complete or aborted by a user intervention (abort/cancel)
			Download();

			//Uninitialize
			UninitDownloaders( Status::COMPLETED );
		}
		catch( std::runtime_error& /*error*/ ){

		}
		catch( ... ) {

		}

		if( m_phCURL ) {
			//Close the multi-session.
			curlCode = curl_multi_cleanup( m_phCURL );
			m_phCURL = NULL;
		}

		return bRetval;
	}

	bool DownloadManager::InitDownloaders() {
		bool bRetval = false;

		//Here we initialize the Downloaders based on the max concurrent downloads 
		//and the number of URLs to Download
		//If the Number of URLs to be downloaded are more than the concurrent downloads
		//Then we first initialize downloaders equal to the concurrent downloads
		//and remember how many more we have done.
		try {
			//URLs to be downloaded are in the m_vecURLs vector
			//We iterate over this vector depending upon 
			//what is greater max concurrent connections and number of URLs to download
			std::vector< URL >::size_type nNumOfURLs = GetNumberOfURLsToDownload();
			
			Downloader *pDownloader = nullptr;
			for( m_nCurrentPos = 0; m_nCurrentPos < nNumOfURLs; ++m_nCurrentPos ) {
				pDownloader = new Downloader( m_vecURLs[ m_nCurrentPos ], m_strDownloadDirectory, m_hStdOut, static_cast< short >( m_nCurrentPos ) );
				pDownloader->Init();
				m_vecDownloaders.push_back( pDownloader );

				curl_multi_add_handle( m_phCURL, pDownloader->GetCURL() );
			}
			bRetval = true;
		}
		catch( std::bad_alloc& oException ) {
			std::cerr << "Out of memory. " << oException.what() << std::endl;
		}
		catch(...) {
			std::cerr << "Unknown exception" << std::endl;
		}

		return false;
	}

	bool DownloadManager::UninitDownloaders( Status eStatus ) {
		bool bRetval = false;

		//here we iterate over the sm_vecDownloaders
		//and cleanup all the 
		Downloader *pDownloader = nullptr;
		for( std::vector< Downloader* >::iterator it = m_vecDownloaders.begin(); it != m_vecDownloaders.end(); ++it ) {
			pDownloader = *it;
			pDownloader->SetStatus( eStatus );
			curl_multi_remove_handle( m_phCURL, pDownloader->GetCURL() );
			delete pDownloader;
			pDownloader = nullptr;
		}

		m_vecDownloaders.clear();
		return bRetval;
	}

	bool DownloadManager::RemoveDownloader( CURL* pEasyHandle, Status eStatus ){
		bool bRetval = false;

		//here we iterate over the sm_vecDownloaders
		//and cleanup all the 
		Downloader *pDownloader = nullptr;
		for( std::vector< Downloader* >::iterator it = m_vecDownloaders.begin(); it != m_vecDownloaders.end(); ++it ) {
			pDownloader = *it;
			if( pDownloader->GetCURL() == pEasyHandle ) {
				pDownloader->SetStatus( eStatus );
				curl_multi_remove_handle( m_phCURL, pDownloader->GetCURL() );
				delete pDownloader;
				pDownloader = nullptr;
				m_vecDownloaders.erase( it );
				bRetval = true;
				break;
			}
		}

		return bRetval;
	}

	bool DownloadManager::Download() {
		bool		bRetval		= false;
		CURLMcode	curlCode	= CURLM_BAD_HANDLE;
		CURLMsg		*pCurlMSG	= NULL;

		fd_set		structReadFDSet, structWriteFDSet, structExecuteFDSet;
		int			nMaxFD = 0;
		int			nMsgInQueue = 0;
		long		lTimeOut = 0;
		timeval		structTime;

		while( m_nRunningHandles ) {

			DWORD dwStatus = ::WaitForSingleObject( DownloadManager::sm_hExitEvent, 0 );
			switch( dwStatus ) {
				case WAIT_OBJECT_0://signalled aborted
					//Here we remove the downloaders
					UninitDownloaders( Status::ABORTED );
					break;
				case WAIT_TIMEOUT: {
					//Read available data from each easy handle
					curl_multi_perform( m_phCURL, &m_nRunningHandles );

					if( m_nRunningHandles ) {
						FD_ZERO( &structReadFDSet );
						FD_ZERO( &structWriteFDSet );
						FD_ZERO( &structExecuteFDSet );

						//Extract file descriptions from multi-handle
						curlCode = curl_multi_fdset( m_phCURL, &structReadFDSet, &structWriteFDSet, &structExecuteFDSet, &nMaxFD );
						if( CURLM_OK != curlCode ) {
							std::cerr << "Error while retrieveing the libcurl file descriptors for select or poll." << std::endl;
							break;
						}

						//check how long to wait
						curlCode = curl_multi_timeout( m_phCURL, &lTimeOut );
						if( CURLM_OK != curlCode ) {
							std::cerr << "Error while retrieveing the needed time out value." << std::endl;
							break;
						}

						if( lTimeOut == -1 ) {
							lTimeOut = 100;
						}

						if( nMaxFD == -1 ){
							//no file descriptors set, wait for timeout time
							::Sleep( lTimeOut );
						}
						else {
							//file descriptors set
							structTime.tv_sec = lTimeOut / 1000;
							structTime.tv_usec = ( lTimeOut % 1000 ) * 1000;

							if( 0 > select( nMaxFD + 1, &structReadFDSet, &structWriteFDSet, &structExecuteFDSet, &structTime ) ) {
								//error
								std::cerr << "Error in select." << std::endl;
								break;
							}
						}
					}//if( m_nRunningHandles )

					while( NULL != ( pCurlMSG = curl_multi_info_read( m_phCURL, &nMsgInQueue ) ) ) {
						if( pCurlMSG->msg == CURLMSG_DONE ) {
							//This Downloader is complete
							RemoveDownloader( pCurlMSG->easy_handle, Status::COMPLETED );
						}
						else {
							continue;
						}

						//TODO::
					}
				}
			}
			

		}//while( m_nRunningHandles ) 
		return bRetval;
	}

	BOOL DownloadManager::ControlHandler( DWORD dwCtrlType ){
		//After handling it should Set an Event,
		//once the main app sees this event it will exit.
		SetEvent( DownloadManager::sm_hExitEvent );

		switch( dwCtrlType ) {
			// Handle the CTRL-C signal. 
			case CTRL_C_EVENT:
				std::cout <<  "Ctrl-C event\n\n" << std::endl;
				return( TRUE );

				// CTRL-CLOSE: confirm that the user wants to exit. 
			case CTRL_CLOSE_EVENT:
				std::cout << "Ctrl-Close event\n\n" << std::endl;
				return( TRUE );

				// Pass other signals to the next handler. 
			case CTRL_BREAK_EVENT:
				std::cout << "Ctrl-Break event\n\n" << std::endl;
				return FALSE;

			case CTRL_LOGOFF_EVENT:
				std::cout << "Ctrl-Logoff event\n\n" << std::endl;
				return FALSE;

			case CTRL_SHUTDOWN_EVENT:
				std::cout << "Ctrl-Shutdown event\n\n" << std::endl;
				return FALSE;

			default:
				return FALSE;
		}
	}
}
