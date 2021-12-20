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
class EFFReader : public DataReader
{

private:
	wstring file_path;

public:
	EFFReader();
	~EFFReader();


	/*************************************************************************************************************************************************************************
	* This function constructs JSON representation of test values and writes to file
	*
	* Input:
	*		eff_path			wstring								wstring containing paths to csv file to be converted
	*		configs_struct		map<wstring, wstring>					structure containing configurations
	*		out_folder_path		wstring								path to folder to store JSON
	* Output:
	*		res				bool								whether JSON construction was successful or not
	*
	* Internal variables explanation
	*	cond_str			wstring												combination of conditions for given test value
	*   key_cond_str		wstring												parameter name + cond_str to keep conditions for each param separately to avoid condition repetition
	*	common_meta_data	map<wstring, wstring>									stores common_meta_data as <key, value>, e.g. <username, Ali Ganbarov>
	*	unique_params		map <wstring, int>									mapping for each unique param and test number, e.g. <ibat_rom, 123>
	*	meta_data			map <wstring, wstring>								stores meta_data of each data object as <key, value>, e.g. <cond_vio, 5>
	*	payload				map <wstring, wstring>								stores payload of each data object as <key, value>, e.g. <ibat_rom, 1.2345>
	*	internal_json		map <wstring, map<wstring, map<wstring, wstring>>>		stores data in format map<key_cond_str, map<type, map_of_type<key, value>>> where type = payload or meta_data
	*	data_objects		vector<map<wstring, map<wstring, wstring>>>			final version of all data_objects, similar to internal_json
	*	test_col_ind		integer												stores the column number where the test data starts
	*
	* <<EFF:1.00>> lines are used to get username, basic_type, product_design_step and product_sales_code for meta and common_meta data
	* <+EFF:1.00> is used to get condition names. It iterates over each word and if it's convertible to number, then it means test data column has started (test_numbers), since
	* condition name can't be numeric.
	* <+PName> line is used to get parameter names
	* <Unit> line is used to get units
	* <USL> line is used to get upper limits
	* <LSL> line is used to get lower limits
	* 05_Die lines are used to get test data
	*
	* Test data is read row by row. In each row it iterates over columns to construct meta from conditions (till test_col_ind) and payload from test value (after test_col_ind).
	* Meta is constructed using conds names and values. 
	*
	* Test or limit values are scaled based on units
	*
	*************************************************************************************************************************************************************************/
	bool eff_to_json(wstring, map<wstring, wstring>, wstring);
};

