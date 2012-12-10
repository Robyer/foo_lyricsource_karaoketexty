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

std::string source_get_value(std::string* data, std::string::size_type start, unsigned int argument_count, ...)
{
	va_list arg;
	std::string ret;
	std::string::size_type end = 0;
	
	va_start(arg, argument_count);
	
	for (unsigned int i = argument_count; i > 0; i--)
	{
		if (i == 1)
		{
			end = data->find(va_arg(arg, char*), start);
			if (start == std::string::npos || end == std::string::npos)
				break;
			ret = data->substr(start, end - start);
		} else {
			std::string term = va_arg(arg, char*);
			start = data->find(term, start);
			if (start == std::string::npos)
				break;
			start += term.length();
		}
	}
	
	va_end(arg);	
	return ret;
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
	/*url += " ";
	pfc::urlEncodeAppend(url, pQuery->title);*/
	//url += pQuery->title

	pfc::string8 page;

	m_pHttpClient->download_page( page, "Mozilla Firefox", url );

	/*	
	page.truncate(300);

	//We insert the string we saved in PrepareSearch();
	page.insert_chars( 0, requirements->some_example_string );*/
		
//	std::string a = pfc::stringcvt::convert_wide_to_ansi(

	std::string data = page;
	std::string::size_type start = data.find("<ul class='artist'>", 0);
	std::string::size_type end = data.find("</ul>", start);
	if (start == std::string::npos || end == std::string::npos)
		return false;

	data = data.substr(start, end-start);
	start = end = 0;
	while ((start = data.find("<li>", start)) != std::string::npos) {
		end = data.find("</li>", start);
		std::string line = data.substr(start, end - start);
		start = end;

		std::string href = source_get_value(&line, 0, 2, "<a href=\"", "\"");
		std::string artist = trim(source_get_value(&line, 0, 3, "<a href=\"", ">", "</a>"));

		// search for same artist only
		if (!_stricmp(artist.c_str(), pQuery->artist)) {

			pfc::string8 url;
			url = "http://www.karaoketexty.cz";
			url += href.c_str();
			
			pfc::string8 page;
			m_pHttpClient->download_page( page, "Mozilla Firefox", url );

			std::string data = page;

			data = data.substr(data.find("<div id=\"left_column"));
			data = data.substr(0, data.find("<div id=\"right_column"));
						
			std::string::size_type pos = 0;
			std::string::size_type pos2 = 0;
						
			while ((pos = data.find("</table>", pos2)) != std::string::npos) {
				std::string blok = data.substr(pos2, pos-pos2);
				pos2 = pos+8;

				std::string album;
				if ((pos = blok.find("<div class=\"album_cover_box")) != std::string::npos) {
					// album with cover
					album = source_get_value(&blok, pos, 2, "<strong>", "</strong>");
				} else {
					// album without cover
					album = source_get_value(&blok, 0, 3,"<th class=\"left", ">", "</th>");
					if ((pos = album.find(">")) != std::string::npos)
						album = source_get_value(&album, pos, 2, ">", "</");
				}

				blok = blok.substr(blok.find("<table"));

				std::string::size_type pos3 = 0;
				while ((pos = blok.find("</tr>", pos3)) != std::string::npos) {
					std::string row = blok.substr(pos3, pos-pos3);
					pos3 = pos+5;

					std::string title = source_get_value(&row, 0, 3, "<a href=\"/texty-pisni/", ">", "</a>");

					// searching for all?
					if (strcmp(pQuery->title, "*")) {
						// ignore not similar titles
						if (title.find(pQuery->title) == std::string::npos && strstr(pQuery->title, title.c_str()) == NULL)
							continue;
						
						// ignore not precisely same titles
						/*if (_stricmp(title.c_str(), pQuery->title))
							continue;*/
					}						
					
					href = source_get_value(&row, 0, 2, "<a href=\"/texty-pisni/", "\"");
					if (!href.empty()) {
						std::string lyrics_url = "http://www.karaoketexty.cz/texty-pisni/";
						lyrics_url += href;

						lyric_container_base* new_lyric = p_results->AddResult();

						new_lyric->SetTimestamped(false);
						new_lyric->SetFoundInfo(artist.c_str(), album.c_str(), title.c_str());
						new_lyric->SetSources(lyrics_url.c_str(), lyrics_url.c_str(), GetGUID(), ST_INTERNET);
						new_lyric->SetLoaded(false);
					}

					href = source_get_value(&row, 0, 2, "<a href=\"/karaoke/", "\"");
					if (!href.empty()) {
						/*std::string karaoke_url = "http://www.karaoketexty.cz/karaoke/";
						karaoke_url += href;
											
						lyric_container_base* new_lyric = p_results->AddResult();

						new_lyric->SetTimestamped(true);
						new_lyric->SetFoundInfo(artist.c_str(), album.c_str(), title.c_str());
						new_lyric->SetSources(karaoke_url.c_str(), karaoke_url.c_str(), GetGUID(), ST_INTERNET);
						new_lyric->SetLoaded(false);*/
					}

				}

			}

		}		
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

/*	if (lyric->IsTimestamped()) {
		url = "http://www.karaoketexty.cz/karaoke/";
	} else {
		url = "http://www.karaoketexty.cz/texty-pisni/";
	}
	url += source_private;*/
	url = source_private;

	pfc::string8 page;

	m_pHttpClient->download_page( page, "Mozilla Firefox", url );

	std::string data = page;

	if (lyric->IsTimestamped()) {

		//lyric->SetLyric("TODO: BUDE CASOVANE!");

	} else {

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

	}

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
