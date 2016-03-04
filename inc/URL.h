#pragma once

#include <string>
#include <ostream>
#include <istream>

namespace Helper {
	class URL {
		public:
			URL() : m_strURL( "" ) {};
			URL( const std::string& strURL ) : m_strURL( strURL ) {};
			URL( const URL& oURL ) { m_strURL = oURL.m_strURL; }
			URL& operator = ( const URL& oURL ) {
				if( this != &oURL ) {
					m_strURL = oURL.m_strURL;
				}

				return *this;
			}
			~URL() {};

			bool	GetDownloadFileName( std::string& strFilename ) const;
			const std::string& 
					GetURL() const	{ return m_strURL; };
			bool	GetURLW( std::wstring& strURLW ) const;

			friend std::ostream& operator << ( std::ostream& out, const URL& oURL );
			friend std::istream& operator >> ( std::istream& in, URL& oURL );
		private:
			std::string m_strURL;
	};//class URL
}//namepsace Helper