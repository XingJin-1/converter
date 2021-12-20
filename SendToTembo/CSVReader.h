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
#include "DataReader.h"
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
class CSVReader: public DataReader
{

private:
	wstring file_path;
	wstring waveform_explorer_path = L"\\\\mucsdn31\\ATV_Power_Dev\\PSN_ProductEngineering\\CV\\Projects\\CV_framework_PSN\\Group3_CV_Reporting\\Tembo\\PSN_Development\\WaveformExplorer\\WaveformExplorer\\for_redistribution_files_only\\WaveformExplorer.exe";

public:
	CSVReader();
	~CSVReader();


	/*************************************************************************************************************************************************************************
	* This function reads limit file into limits_struct
	*
	* Input:
	*		limits_file_path	wstring									absolute path to testlimits.txt file
	* Output:
	*		limits_struct		map<wstring, map<wstring, wstring>>		map container containing limits from testlimits.txt
	*
	* limits_struct structure holds data in format <parameter_name, <limit_key, limit_value>>, e.g. <ibat_rom, <LSL, 1>>, <ibat_rom, <USL, 3>>
	*
	* Limits file is read line by line and split on ' ' or '\t' into headers_arr and limits_arr.
	* headers_arr and limits_arr are mapped to each other based on array index (e.g. if LSL's position in headers_array is 4, the
	* value for LSL in limits_arr will have position 4 too). Empty limit values are implicitly NOT allowed, because it will ruin ordering of header to
	* value mapping (values will be shifted to left).
	* Description value should come last and it can contain ' ' or '\t' (multiple words in sentence). All words in description will also be splitted into limits_arr items
	* and will occur at the end of array. So, description will be constructed with leftover limits_arr items after every previous
	* header was filled with corresponding limit_value
	*
	*************************************************************************************************************************************************************************/
	map <wstring, map <wstring, wstring>> read_limits_file(const wstring&);


	/*************************************************************************************************************************************************************************
	* This function constructs JSON representation of test values and writes to file
	*
	* Input:
	*		csv_files		vector<wstring>						vector of strings containing paths to csv files to be converted
	*		limits_struct	map<wstring, map<wstring, wstring>>	structure containing limits for test values
	*		configs_struct	map<wstring, wstring>					structure containing configurations
	*		json_path		wstring								path to store final JSON file
	* Output:
	*		res				bool								whether JSON construction was successful or not
	*
	* Internal variables explanation
	*	cond_str			wstring												combination of conditions for given test value
	*   key_cond_str		wstring												parameter name + cond_str to keep conditions for each param separately to avoid condition repetition
	*	common_meta_data	map<wstring, wstring>									stores common_meta_data as <key, value>, e.g. <username, Ali Ganbarov>
	*	unique_params		map <wstring, int>									mapping for each unique param and test number, e.g. <ibat_rom, 123>
	***	meta_data			map <wstring, wstring>								stores meta_data of each data object as <key, value>, e.g. <cond_vio, 5>
	***	payload				map <wstring, wstring>								stores payload of each data object as <key, value>, e.g. <ibat_rom, 1.2345>
	*	internal_json		map <wstring, map<wstring, map<wstring, wstring>>>		stores data in format map<key_cond_str, map<type, map_of_type<key, value>>> where type = payload or meta_data
	*	data_objects		vector<map<wstring, map<wstring, wstring>>>			final version of all data_objects, similar to internal_json
	*
	* #meta lines are used to get username, basic_type, product_design_step and product_sales_code for meta and common_meta data
	*
	* Lines containing test values are once iterated to construct meta_data structure of current row,
	* then iterated once again to construct data object for parameter values with payload + metaData
	*
	* If limits_struct contains limit for the given parameter, the limit_object and test_number are read from limit
	* for parameters which don't have limit data, hardcoded limits are applied
	* test_number is generated based on test_number_counter increment and by avoiding overlap with used test numbers from limits (params that have limits)
	* each unique parameter encountered is saved in unique_params as <param_name, test_number>
	* limit object is added for any parameter only if unique_params doesn't contain that parameter (in order to add one limit object per unique parameter)
	* test_number for already encountered parameters are taken from unique_params value to keep number consistent among all parameter's values
	*
	* internal_json is used to store test values uniquely based on combination of all conditions
	* combination of all conditions for a given parameter is stored in key_cond_str, so the first key of internal_json is key_cond_str
	* Whenever condition combination is repetated for parameter the key_cond_str will be same for the previous occurences, so adding
	* data object to internal_json with same key_cond_str will overwrite previous item.
	* data_objects has same structure as internal_json, but used for storing final version of data_objects
	* internal_json is copied to data_objects in two cases:
	*		1) new #meta tag occured. For current version it means that temperature has changed so there definitely will not be repetition of condition combination
	*		2) end of CSV file. Other CSVs will not have same parameter name, so repetitions are not possible
	* After copying internal_json to data_objects, it is set to empty map and filled again
	*
	* In case of absence of required configuration item, default is used
	* last item written to json is recipe
	*
	* Test or limit values are scaled based on units
	*
	*************************************************************************************************************************************************************************/
	bool csvs_to_json(vector<wstring>, map<wstring, map<wstring, wstring>>, map<wstring, wstring>, wstring, vector<wstring>, vector<wstring>);


	vector<wstring>get_corresponding_files(vector<wstring>, vector<wstring>);
};

