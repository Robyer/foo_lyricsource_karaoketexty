#include "StdAfx.h"
#include "my_lyrics_source.h"

//You must use your own GUID, this one is for example purposes only (in VS2010 Tools->Create GUID).
// {E0FCD77D-9A52-4D70-9225-9E960FABBF2E}
const GUID custom_search_requirements::class_guid = 
{ 0xe0fcd77d, 0x9a52, 0x4d70, { 0x92, 0x25, 0x9e, 0x96, 0xf, 0xab, 0xbf, 0x2e } };

static const service_factory_t< custom_search_requirements > custom_search_requirements_factory;

//You must use your own GUID, this one is for example purposes only.
// {B9BF008E-F28B-4B4E-A784-D07AC238BA3E}
static const GUID guid = 
{ 0xb9bf008e, 0xf28b, 0x4b4e, { 0xa7, 0x84, 0xd0, 0x7a, 0xc2, 0x38, 0xba, 0x3e } };

static const source_factory<my_lyrics_source> my_lyrics_source_factory;

//Each different source must return it's own unique GUID
const GUID my_lyrics_source::GetGUID()
{
	return guid;
}

bool my_lyrics_source::PrepareSearch( const search_info* pQuery, lyric_result_client::ptr p_results, search_requirements::ptr& pRequirements )
{
	TRACK_CALL_TEXT( "my_lyrics_source::PrepareSearch" );
	
	//If you need to save some data, use the search requirements interface (see my_lyrics_source.h).
	//This creates the instance of your custom search requirements.
	custom_search_requirements::ptr requirements = static_api_ptr_t< custom_search_requirements >().get_ptr();

	//Set some data
	requirements->some_example_string = "This is an example string";

	//Copy the requirements so the lyrics plugin can provide them for the Search() function.
	pRequirements = requirements.get_ptr();

	//return true on success, false on failure
	return true;
}

bool my_lyrics_source::Search( const search_info* pQuery, search_requirements::ptr& pRequirements, lyric_result_client::ptr p_results )
{
	TRACK_CALL_TEXT( "my_lyrics_source::Search" );
	//do a search

	custom_search_requirements::ptr requirements = reinterpret_cast<custom_search_requirements*>( pRequirements.get_ptr() );

	//For demonstration, we just download the page of a google search for lyrics of these lyrics.
	pfc::string8 url;
	url = "http://www.google.co.uksearch?q=";
	url += pQuery->artist;
	url += " ";
	url += pQuery->title;
	url += " lyrics";

	pfc::string8 page;

	m_pHttpClient->download_page( page, "Mozilla Firefox", url );
	page.truncate( 600 );

	//We insert the string we saved in PrepareSearch();
	page.insert_chars( 0, requirements->some_example_string );

	//then we use the results client to provide an interface which contains the new lyric 
	lyric_container_base* new_lyric = p_results->AddResult();

	//This is only for demonstration purposes, you should should set these to what you search source returns.
	new_lyric->SetFoundInfo( pQuery->artist, pQuery->album, pQuery->title );

	//These tell the user about where the lyric has come from. This is where you should save a web address/file name to allow you to load the lyric
	new_lyric->SetSources( "Custom Source", "Custom Private Source", GetGUID(), ST_INTERNET );

	//Copy the lyric text into here
	new_lyric->SetLyric( page.get_ptr() );

	return true;
}

bool my_lyrics_source::Load( lyric_container_base* lyric )
{
	//Load the lyric

	//This gets the source info you set in Search(), 
	pfc::string8 source, source_private;
	lyric->GetSources( source, source_private );

	//You should set this if the lyric loads properly.
	lyric->SetLoaded();

	//return true on success, false on failure
	return true;
}

void my_lyrics_source::ShowProperties( HWND parent )
{
	//This displays the standard internet source properties.
	static_api_ptr_t< generic_internet_source_properties >()->run( parent );

	return;
}
