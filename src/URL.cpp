#include "URL.h"
#include "Globals.h"

//C++ headers
#include "algorithm"
#include "functional"

namespace Helper{
	bool URL::GetDownloadFileName( std::string& strFilename ) const {
		//This function creates a unique file name based on the URL
		//As of now handles simple URLs, can be extended to handle encoded URLs
		//as well.

		//Example URLs could be: ftp://ftp.gnu.org/README -> Filename: ftp_gnu_rg_README
		//http://www.microsoft.com -> No Filename. So filename could be www_microsoft_com
		
		//So this function needs to do the following:
		//1. After the Protocol:// convert all non number or alpbateic character to _
		//2. This becomes our Unique file name.

		//find first occurence of the character // in m_strURL
		//std::string::size_type nPos = m_strURL.find( "//" );
		//if( std::string::npos == nPos ) {
		//	//not found, somethings wrong with the URL
		//	return false;
		//}

		////found //, so now get the portion after the Protocol://
		//strFilename = m_strURL.substr( nPos + 2, m_strURL.length() - nPos );
		////Now convert all non alphanumberic chars to underscore
		//std::replace_if( strFilename.begin(), strFilename.end(), std::not1( std::ptr_fun( isalnum ) ), '_' );

		return Globals::GetFilenameFromURL( m_strURL, strFilename );
	}

	bool URL::GetURLW( std::wstring& strURLW ) const {
		return Globals::UTF8ToUTF16( m_strURL, strURLW );
	}

	std::ostream& operator << ( std::ostream& out, const URL& oURL ) {
		out << oURL.m_strURL;
		return out;
	}

	std::istream& operator >> ( std::istream& in, URL& oURL ) {
		in >> oURL.m_strURL;
		return in;
	}
}