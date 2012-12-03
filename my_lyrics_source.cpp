#include "StdAfx.h"
#include "my_lyrics_source.h"

// {E1E58234-765C-4874-8BEF-40A04E63FB90}
const GUID custom_search_requirements::class_guid = 
{ 0xe1e58234, 0x765c, 0x4874, { 0x8b, 0xef, 0x40, 0xa0, 0x4e, 0x63, 0xfb, 0x90 } };

static const service_factory_t< custom_search_requirements > custom_search_requirements_factory;

// {B4B107A1-0470-4C0A-A51F-77EDA7E61706}
static const GUID guid = 
{ 0xb4b107a1, 0x470, 0x4c0a, { 0xa5, 0x1f, 0x77, 0xed, 0xa7, 0xe6, 0x17, 0x6 } };

static const source_factory<my_lyrics_source> my_lyrics_source_factory;

//Each different source must return it's own unique GUID
const GUID my_lyrics_source::GetGUID()
{
	return guid;
}

bool my_lyrics_source::PrepareSearch( const search_info* pQuery, lyric_result_client::ptr p_results, search_requirements::ptr& pRequirements )
{
	TRACK_CALL_TEXT( "my_lyrics_source::PrepareSearch" );
	
	/*//If you need to save some data, use the search requirements interface (see my_lyrics_source.h).
	//This creates the instance of your custom search requirements.
	custom_search_requirements::ptr requirements = static_api_ptr_t< custom_search_requirements >().get_ptr();

	//Set some data
	requirements->some_example_string = "This is an example string";

	//Copy the requirements so the lyrics plugin can provide them for the Search() function.
	pRequirements = requirements.get_ptr();*/

	//return true on success, false on failure
	return true;
}

std::string trim(std::string data)
{
	std::string spaces = " \t\r\n";
	std::string::size_type begin = data.find_first_not_of(spaces);
	std::string::size_type end = data.find_last_not_of(spaces) + 1;

	return (begin != std::string::npos) ? data.substr(begin, end - begin) : "";
}

void replace_all(std::string* data, std::string from, std::string to)
{
	std::string::size_type position = 0;

	while ((position = data->find(from, position)) != std::string::npos)
	{
		data->replace(position, from.size(), to);
		position++;
	}
}

bool my_lyrics_source::Search( const search_info* pQuery, search_requirements::ptr& pRequirements, lyric_result_client::ptr p_results )
{
	TRACK_CALL_TEXT( "my_lyrics_source::Search" );
	//do a search

	//custom_search_requirements::ptr requirements = reinterpret_cast<custom_search_requirements*>( pRequirements.get_ptr() );
	
	pfc::string8 url;
	url = "http://www.karaoketexty.cz/search?q=";
	pfc::urlEncodeAppend(url, pQuery->artist);
	//url += pQuery->artist;
	url += " ";
	pfc::urlEncodeAppend(url, pQuery->title);
	//url += pQuery->title

	pfc::string8 page;

	m_pHttpClient->download_page( page, "Mozilla Firefox", url );

	/*	
	page.truncate(300);

	//We insert the string we saved in PrepareSearch();
	page.insert_chars( 0, requirements->some_example_string );*/
		
//	std::string a = pfc::stringcvt::convert_wide_to_ansi(

	std::string data = page;
	std::string::size_type start = data.find("<ul class='title'>", 0);
	std::string::size_type end = data.find("</ul>", start);
	if (start == std::string::npos || end == std::string::npos)
		return false;

	data = data.substr(start, end-start);
	start = end = 0;
	while ((start = data.find("<li>", start)) != std::string::npos) {
		end = data.find("</li>", start);
		std::string line = data.substr(start, end - start);
		start = end;
		
		std::string::size_type pos = 0;

		// url
		pos = line.find("<a href=\"");
		if (pos == std::string::npos)
			continue;

		pos += 9;
		std::string url = line.substr(pos, line.find("\"", pos) - pos);

		// artist-title
		pos = line.find(">", pos);
		if (pos == std::string::npos)
			continue;

		pos += 1;
		std::string title = trim(line.substr(pos, line.find("</a>", pos) - pos));
				
		// contains artist?
		std::string artist;
		pos = title.find(" - ");
		if (pos != std::string::npos) {
			artist = title.substr(0, pos);
			title = title.substr(pos+3);
		}

		// ignore not searched results
		if (_stricmp(artist.c_str(), pQuery->artist) && _stricmp(title.c_str(), pQuery->title))
			continue;

		//then we use the results client to provide an interface which contains the new lyric 
		lyric_container_base* new_lyric = p_results->AddResult();

		//This is only for demonstration purposes, you should should set these to what you search source returns.
		new_lyric->SetFoundInfo( artist.c_str(), "", title.c_str()/*pQuery->artist, pQuery->album, pQuery->title*/ );
		
		//These tell the user about where the lyric has come from. This is where you should save a web address/file name to allow you to load the lyric
		new_lyric->SetSources( url.c_str(), url.c_str(), GetGUID(), ST_INTERNET );

		new_lyric->SetTimestamped(false);
		new_lyric->SetLoaded( false );
	}		

	return true;
}

bool my_lyrics_source::Load( lyric_container_base* lyric )
{
	//Load the lyric

	//This gets the source info you set in Search(), 
	pfc::string8 source, source_private;
	lyric->GetSources( source, source_private );

	
	// download and parse lyrics	
	pfc::string8 url;
	url = "http://www.karaoketexty.cz";
	url += source_private;

	pfc::string8 page;

	m_pHttpClient->download_page( page, "Mozilla Firefox", url );

	std::string data = page;

	std::string::size_type start = data.find("<p class=\"text\">", 0);
	std::string::size_type end = data.find("</p>", start);
	if (start == std::string::npos || end == std::string::npos)
		return false;

	start += 16;
	std::string lyrics = data.substr(start, end-start);
	replace_all(&lyrics, "<br />", "");

	// TODO: add authors!

	//Copy the lyric text into here
	lyric->SetLyric( lyrics.c_str() );

	//You should set this if the lyric loads properly.
	// lyric->SetLoaded(); // automatically in SetLyric

	//return true on success, false on failure
	return true;
}

void my_lyrics_source::ShowProperties( HWND parent )
{
	//This displays the standard internet source properties.
	static_api_ptr_t< generic_internet_source_properties >()->run( parent );

	return;
}
