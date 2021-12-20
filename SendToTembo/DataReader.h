#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <time.h>
#include <codecvt>
#include <algorithm>
#include <tuple>
#include <sstream>

#include <chrono>

/*************************************************************************************************************************************************************************
* maintainer Xing Jin (IFAG ATV PS PD MUC CVSV)
* since v4.0.0
*
* date		27.01.2021
*
* author	Ali Ganbarov (IFAG ATV PSN D PD PRES)
*************************************************************************************************************************************************************************/

using namespace std;

#pragma once
class DataReader
{

protected:
	/*************************************************************************************************************************************************************************
	* This function converts common_meta_data and data_objects structures into JSON in chunks
	*
	* Input:
	*		header				map<wstring, wstring>									header struct <key, value> - redundant for now, only 1 item
	*		common_meta_data	map<wstring, wstring>									<key, value> mapping for common_meta_data
	*		data_objects		map<wstring, map<wstring, map<wstring, wstring>>>		all data_objects
	*		json_path			wstring												where to store JSON file
	*		recipe_payload		wstring												recipe for report generation
	* Output:
	*		res					bool												success or not
	*
	* This function converts all structures generated so far into JSON
	* The processing is done in chunks of 500 objects to keep json wstring small (to avoid slow huge wstring manipulations)
	* Wherever possible numeric values are written without "" marks
	* Recipe is written as last object
	*
	*************************************************************************************************************************************************************************/
	bool json_writer(map<wstring, wstring>, map<wstring, wstring>, vector<map<wstring, map<wstring, wstring>>>*, wstring, wstring);


	/*************************************************************************************************************************************************************************
	* This function splits wstring on the given delimiters
	*
	* Input:
	*		line					wstring				wstring to split
	*		delimiters				wstring				wstring containing chars to split on (e.g. ' \t' both ' ' and '\t')
	*		collapse_delimiters		bool				flag to add or not sequence of delimiters (e.g. true: 'a  b' -> ['a', 'b'], false: 'a  b' -> ['a', '', 'b'])
	* Output:
	*		tokens					vector<wstring>		resulting tokens after split
	*
	* This function splits given wstring (line) into tokens based on delimiters
	* collapse_delimiter controls whether to remove all delimiters in sequence or add as empty token
	*
	*************************************************************************************************************************************************************************/
	vector<wstring> strsplit(wstring, wstring, bool collapse_delimiters = true);

	
	map <wstring, wstring> construct_common_meta_data(wstring, wstring, wstring, wstring, wstring);
	map <wstring, wstring> construct_limit_meta_data(map<wstring, wstring>, wstring, wstring, wstring, wstring, wstring);
	wstring construct_recipe(wstring, wstring, wstring);
	wstring scale_value(int, wstring);
	wstring generate_limit_from_test_value(wstring, bool);


	/*************************************************************************************************************************************************************************
	* This function replaces characters in wstring
	*
	* Input:
	*		line	wstring		line to replace in
	*		from	char		to be replaced
	*		to		char		replaced by
	* Output:
	*		line	wstring		updated line
	*
	* Iterates over chars in line and replaces chars
	*
	*************************************************************************************************************************************************************************/
	wstring strrep(wstring, char, char);


	/*************************************************************************************************************************************************************************
	* This function removes characters from wstring
	*
	* Input:
	*		line	wstring		line to remove from
	*		rem		char		to be removed
	* Output:
	*		line	wstring		updated line
	*
	* Iterates over chars and copies all chars except the ones to be removed
	*
	*************************************************************************************************************************************************************************/
	wstring strremove(wstring, char);


	/*************************************************************************************************************************************************************************
	* This function trims ' ' from wstring
	*
	* Input:
	*		line	wstring		line to trim
	*		rem		char		to be removed
	* Output:
	*		line	wstring		updated line
	*
	* Iterates over chars and copies all chars except ' ' in the beginning or end
	*
	*************************************************************************************************************************************************************************/
	wstring strtrim(wstring);


	/*************************************************************************************************************************************************************************
	* This function converts raw_unit to scale and unit without measurement char
	*
	* Input:
	*		raw_unit		wstring					original unit value
	* Output:
	*		(scale, unit)	tuple(int, wstring)		corresponding scale and unit
	*
	* This function determines scale from first char of raw_unit and removes
	* fist character
	*
	*************************************************************************************************************************************************************************/
	tuple<int, wstring>get_unit_scale(const wstring&);


	/*************************************************************************************************************************************************************************
	* This function create simple progress bar
	*
	* Input:
	*		curr		int			current progress
	*		total		int			total progress
	*		step		int			number of steps represented by each bar char
	*
	* Output:
	*		progress	wstring		progress bar (e.g. |==>.....| (30%))
	*
	*************************************************************************************************************************************************************************/
	wstring progress_bar(int, int, int);


	/*************************************************************************************************************************************************************************
	* This function validates correct parameter by replacing special chars with _
	*
	* Input:
	*		raw_param_name		wstring			original parameter name
	* Output:
	*		param_name			wstring			validated parameter name
	*
	* This function replaces all special characters by _. If _ occurs as first char, remove it.
	*
	*************************************************************************************************************************************************************************/
	wstring validate_param_name(wstring);


	/*************************************************************************************************************************************************************************
	* This function converts integer to excel style column naming, e.g. 3 -> C
	*
	* Input:
	*		col					int					col number
	* Output:
	*		name				wstring				excel style naming
	*
	*
	*************************************************************************************************************************************************************************/
	wstring get_excel_col_name(int);

	/*************************************************************************************************************************************************************************
	* This function counts the number of occurences of character in string
	*
	* Input:
	*		s					wstring					string to search in
	*		c					char					char to search for
	* Output:
	*		count				int						number of occurences
	*
	*
	*************************************************************************************************************************************************************************/
	int count_char_occurence(wstring, char);

public:
	DataReader();
	~DataReader();


	/*************************************************************************************************************************************************************************
	* This function reads config file into configs_struct
	*
	* Input:
	*		config_file_path	wstring absolute			path to Config_Tembo.txt file
	* Output:
	*		configs_struct		map<wstring, wstring>		map container containing configurations from Config_Tembo.txt
	*
	* configs_struct structure holds data in format <key, value>, e.g. <Project, psn-general>
	*
	* Configs file is read line by line and split on ':' into key value pairs.
	* Project name value is converted to all lower case (Tembo requirement)
	*
	*************************************************************************************************************************************************************************/
	map <wstring, wstring> read_config_file(const wstring&);


	/*************************************************************************************************************************************************************************
	* This function sets up necessary configurations, using hardcoded or from file
	*
	* Input:
	*		configs_struct		map<wstring, wstring>		map container containing configurations, generated by read_config_file() from Config_Tembo.txt
	*		is_csv				bool					flag to check whether configurations are for csv or not (eff)
	* Output:
	*		final_configs		map<wstring, wstring>		map container containing final configurations
	*
	* configs_struct structure holds data in format <key, value>, e.g. <Project, psn-general>
	*
	* Reads configs_struct and sets up necessary configurations. If there is no given value for specific configuration, hardcoded value will be used
	* For CSV files (is_csv=true) ReportName is taken from configurations file, otherwise original file name is used
	*
	*************************************************************************************************************************************************************************/
	map<wstring, wstring> setup_configurations(map<wstring, wstring>, bool);


	/*************************************************************************************************************************************************************************
	* This function converts wstring to all lower case characters
	*
	* Input:
	*		data					wstring			original input
	* Output:
	*		data					wstring			lower case version
	*
	*
	*************************************************************************************************************************************************************************/
	wstring convert_to_lower(wstring);

};

