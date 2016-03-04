
//boost includes
#include "boost/any.hpp"
#include "boost/program_options.hpp"

//Project Includes
#include "Globals.h"
#include "DownloadManager.h"
#include "URL.h"

//C++ includes
#include <vector>
#include <iostream>

namespace po = boost::program_options;

struct Directory {
	public:
		Directory( const std::string& strDirectory ) : m_strDirectory( strDirectory ) {}
		std::string m_strDirectory;
};

void validate( boost::any& v, std::vector< std::string > const& values, Directory*, int ) {
	// Make sure no previous assignment to 'v' was made.
	po::validators::check_first_occurrence( v );

	//Extract the first string from 'values'. If there is more than one string,
	//it's an error, and exception will be thrown.
	std::string const& s = po::validators::get_single_string( values );

	//check the validity of the
	//TODO::
	v = boost::any( Directory( s ) );
}

int wmain( int argc, const wchar_t** argv ) {

	int nRetval = 0;

	try {
		//Setup boost program options, to handle command line switches passed
		//

		std::string strDownloadFolderA;
		bool bRetval = Globals::GetDownloadsFolder( strDownloadFolderA );
		if( !bRetval ) {
			return 1;
		}

		po::options_description desc( "This application can be used to download one ore more files from the internet.\nIt supports downloading from various protocols http, ftp, sftp and so on.\nUsage: Downloaders.exe --dir <DirectoryName> URL1 URL2" );
		desc.add_options()
			( "help,h", "Displays this help" )
			( "dir,d", po::value< Directory >(), "Directory where the files will be downloaded and kept.\nIf not specified, it defaults to the Systems Download directory.\nIf exact path is not specified, then this path will be created under the default folder." )
			( "url,u", po::value< std::vector< Helper::URL > >(), "URLs to download from space separated" );

		//all positional arguments will be treated as URLs
		po::positional_options_description p;
		p.add( "url", -1 );

		po::variables_map vm;
		po::store( po::wcommand_line_parser( argc, argv ).options( desc ).positional( p ).run(), vm );
		po::notify( vm );

		if( vm.count( "help" ) || vm.empty() ) {
			std::cout << desc << std::endl;
			nRetval = 0;
		}
		else {
			//Max Concurrent Downloads is automatically stored in the variable uMaxConcurentDownloads
			//Retrive the download directory
			if( vm.count( "dir" ) ) {
				//directory specified,
				strDownloadFolderA = vm[ "dir" ].as< Directory >().m_strDirectory;
			}

			if( vm.count( "url" ) ) {
				Helper::DownloadManager oDLManager( vm[ "url" ].as< std::vector<Helper::URL>>(),
													strDownloadFolderA );
				oDLManager.Start();
				
			}
			else {
				std::cout << "You need to specify the URLs to download.";
				nRetval = 1;
			}
		}
	}
	catch( std::runtime_error& ) {
		//TODO::
	}
	catch( ... ) {
		//TODO::
	}

	return nRetval;
}