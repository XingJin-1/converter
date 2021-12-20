#include "CSVReader.h"

/*************************************************************************************************************************************************************************
* maintainer Xing Jin (IFAG ATV PS PD MUC CVSV)
* since v4.0.0
*
* date		27.01.2021
*
* author	Ali Ganbarov (IFAG ATV PSN D PD PRES)
*************************************************************************************************************************************************************************/


CSVReader::CSVReader() {
}

CSVReader::~CSVReader() {
}

map <wstring, map <wstring, wstring>> CSVReader::read_limits_file(const wstring& limits_file_path) {
	vector <wstring> headers_arr;	// to store array of headers
	vector <wstring> limits_arr;		// to store array of limit values for each line
	map <wstring, map <wstring, wstring>> limits_struct;	// final structure to store all limits with corresponding key => value pairs

	wifstream inf(limits_file_path);
	if (!inf) {
		wcout << "Couldn't read limits file: " << limits_file_path << endl;
		exit(1);
	}
	locale loc("");
	inf.imbue(loc);
	while (inf) {
		// use wstring for wider char encodings (e.g. micro symbol)
		wstring strInp;
		// read line
		getline(inf, strInp);
		// lines containing 'key' are header lines
		if (strInp.find(L"key") != wstring::npos) {
			// get array of keys
			headers_arr = this->strsplit(strInp, L" \t");
			// remove # element
			headers_arr.erase(headers_arr.begin());
		}
		// skip irrelevant lines
		else if (strInp.find(L"#") != wstring::npos || strInp.find(L"standby") != wstring::npos) {
			continue;
		}
		// any other non-empty line
		else if (strInp.size() > 1) {
			// replace all " with ' to avoid json crush
			strInp = this->strrep(strInp, '"', '\'');
			// split on ' ' or '\t'
			limits_arr = this->strsplit(strInp, L" \t");
			// skip lines which have number of items < 3 (random irrelevant lines)
			if (limits_arr.size() < 3) {
				continue;
			}
			// construct limit object
			map <wstring, wstring> limit_struct;
			// iterate through header_arr for key => val mapping
			for (auto i = 0; i < headers_arr.size(); i++) {
				// adding corresponding limit value if it exists
				if (i < limits_arr.size()) {
					limit_struct[headers_arr[i]] = limits_arr[i];
				}
				// if limit value doesn't exist is empty add ""
				else {
					limit_struct[headers_arr[i]] = L"";
				}
			}
			// read the rest of limits_arr into description
			for (int i = headers_arr.size(); i < limits_arr.size(); i++) {
				if (limits_arr[i].find(L"href") != wstring::npos) {
					// stop once "href" is encountered, since weird symbols break tembo report generation
					// add current piece containing href
					limit_struct[L"Description"] = limit_struct[L"Description"] + L" " + limits_arr[i];
					break;
				}
				limit_struct[L"Description"] = limit_struct[L"Description"] + L" " + limits_arr[i];
			}
			// add current limit_struct to final limits_struct give parameter_name as key
			limits_struct[limits_arr[0]] = limit_struct;
		}
	}

	return limits_struct;
}

bool CSVReader::csvs_to_json(vector<wstring> csv_files, map<wstring, map<wstring, wstring>> limits_struct, map<wstring, wstring> configs_struct, \
							wstring out_folder_path, vector<wstring> png_files, vector<wstring> mat_files) {
	// define header struct
	map<wstring, wstring> header_struct;
	header_struct[L"version"] = L"1.0.1";

	// define common_meta_data
	map <wstring, wstring> common_meta_data;
	// flag to keep create common meta data only once from first #meta lines
	bool common_meta_was_created = false;
	// define data_objects as array of maps
	// map<wstring, map<wstring, map<wstring, wstring>>> data_objects;
	vector<map<wstring, map<wstring, wstring>>> data_objects;

	// define structure to keep repeated condition data for output
	map<wstring, map<wstring, vector<int>>> repeated_conds;
	bool cond_repetition = false;

	// define structure to keep lines which have data not corresponding to any column header
	map<wstring, vector<int>> no_col_match_lines;

	// define meta data variables
	wstring username = L"";
	wstring product_sales_code = L"";
	wstring basic_type = L"";
	wstring product_design_step = L"";
	wstring package = L"";
	wstring dut_id = L"";
	wstring req_id = L"";
	wstring description = L"";
	wstring typical = L"";
	wstring test_number = L"";
	wstring api_id = L"";
	wstring global_id = L"";
	wstring test_program_name = L"";
	wstring testunit_version = L"";
	vector <wstring> column_types;
	vector <wstring> variables;
	vector <wstring> units;
	vector <wstring> usl;
	vector <wstring> lsl;
	vector <wstring> test_data;
	vector <wstring> no_limit_match;

	// define struct to store unique out paramters(e.g.uniq('ibat_stb') = dummy_test_number)
	// if there is no limit specified then test number will be added in increasing order for each
	// unique parameter.
	map <wstring, int> unique_params;
	int test_number_counter = 1;

	/*
	cout << "PNG files: " << endl;
	for (wstring png_file : png_files) {
		wcout << png_file << endl;
	}
	*/

	for (int i = 0; i < csv_files.size(); i++) {
		// iteratre through each csv file
		// represents temp structure, where each fieldname is wstring combining
		// unique conditions(e.g. "{cond_vio}{cond_vbat}")
		// internal_json = struct();
		map <wstring, map<wstring, map<wstring, wstring>>> internal_json;

		// keep count of lines in file
		int line_count = 0;

		// get parent folder name for png match
		wstring curr_file = csv_files[i];
		wstring parent_folder = curr_file.substr(0, curr_file.find_last_of(L"\\") + 1);
		// wcout << "Parent folder: " << parent_folder << endl;
		// conditions that will help to match corresponding png and .mat files for raw_data_link and waveform links
		vector<wstring> file_match_conditions;
		// add first png file match condition to png_file_match_conditions
		file_match_conditions.push_back(parent_folder);

		// start reading file
		wifstream inf(csv_files[i]);
		if (!inf) {
			wcout << L"Couldn't read csv file: " << csv_files[i] << endl;
			break;
		}
		locale loc("");
		inf.imbue(loc);

		// get name of the folder containing csv file -> test_program_name
		test_program_name = csv_files[i].substr(0, csv_files[i].find_last_of(L"\\"));
		test_program_name = test_program_name.substr(test_program_name.find_last_of(L"\\") + 1, test_program_name.size() - 1);

		wcout << L"Reading csv: " << csv_files[i] << endl << endl;
		// for manual measurement meta data is coming from configs_struct
		if (!configs_struct[L"user"].empty()) {
			username = configs_struct[L"user"];
		}
		if (!configs_struct[L"product_sales_code"].empty()) {
			product_sales_code = configs_struct[L"product_sales_code"];
		}
		if (!configs_struct[L"basic_type"].empty()) {
			basic_type = configs_struct[L"basic_type"];
		}
		if (!configs_struct[L"product_design_step"].empty()) {
			product_design_step = configs_struct[L"product_design_step"];
		}
		if (!configs_struct[L"package"].empty()) {
			package = configs_struct[L"package"];
		}
		if (!configs_struct[L"dut_id"].empty()) {
			dut_id = configs_struct[L"dut_id"];
		}
		if (!configs_struct[L"api_id"].empty()) {
			api_id = configs_struct[L"api_id"];
		}
		if (!configs_struct[L"global_id"].empty()) {
			global_id = configs_struct[L"global_id"];
		}
		if (!configs_struct[L"testunit_version"].empty()) {
			testunit_version = configs_struct[L"testunit_version"];
		}

		if (!common_meta_was_created && basic_type != L"" && product_sales_code != L"" && product_design_step != L"") {
			common_meta_data = this->construct_common_meta_data(basic_type, product_design_step, product_sales_code, username, configs_struct[L"Email"]);
			common_meta_was_created = true;
		}
		
		while (inf) {
			wstring strInp;
			// read line
			getline(inf, strInp);
			line_count++;
			if (strInp.find(L"#meta") != wstring::npos) {
				// read meta for following data lines
				// some csvs have format basic_type : S1234, some have format
				// basic_type, S1234.Converting all formats to latter one
				strInp = this->strrep(strInp, ':', ',');
				// replace ; with , (Bucharest data)
				strInp = this->strrep(strInp, ';', ',');

				// separate #meta lines on ,
				vector <wstring> line_data = this->strsplit(strInp, L",");

				// iterate through each meta word chunk(key, value)
				// if the keyword is found in chunk, then value is in next chunk, so
				// setting index to next counter
				for (int counter = 0; counter < line_data.size(); counter++) {
					if (line_data[counter].find(L"user") != wstring::npos && line_data[counter].find(L"email") == wstring::npos) {
						// if username is not set in configs read from CSV
						if (configs_struct[L"Username"].empty()) {
							username = line_data[counter + 1];
							username = this->strtrim(username);
						}
						else {
							username = configs_struct[L"Username"];
						}
					}
					else if (line_data[counter].find(L"product_sales_code") != wstring::npos) {
						product_sales_code = line_data[counter + 1];
						product_sales_code = this->strtrim(product_sales_code);
					}
					else if (line_data[counter].find(L"basic_type") != wstring::npos) {
						basic_type = line_data[counter + 1];
						basic_type = this->strtrim(basic_type);
					}
					else if (line_data[counter].find(L"product_design_step") != wstring::npos) {
						product_design_step = line_data[counter + 1];
						product_design_step = this->strtrim(product_design_step);
					}
					else if (line_data[counter].find(L"package") != wstring::npos) {
						package = line_data[counter + 1];
						package = this->strtrim(package);
					}
					else if (line_data[counter].find(L"dut_id") != wstring::npos) {					
						dut_id = line_data[counter + 1];
						dut_id = this->strtrim(dut_id);
						// clear previous dut_id condition if there was any
						while (file_match_conditions.size() > 1) {
							file_match_conditions.pop_back();
						}
						// add as sample={dut_id} to png conditions match list
						file_match_conditions.push_back(L"sample=" + dut_id);
					}
					else if (line_data[counter].find(L"api_id") != wstring::npos) {
						api_id = line_data[counter + 1];
						api_id = this->strtrim(api_id);
					}
					else if (line_data[counter].find(L"global_id") != wstring::npos) {
						global_id = line_data[counter + 1];
						global_id = this->strtrim(global_id);
					}
					else if (line_data[counter].find(L"testunit_version") != wstring::npos) {
						testunit_version = line_data[counter + 1];
						testunit_version = this->strtrim(testunit_version);
					}
				}
				if (!common_meta_was_created && basic_type != L"" && product_sales_code != L"" && product_design_step != L"") {
					common_meta_data = this->construct_common_meta_data(basic_type, product_design_step, product_sales_code, username, configs_struct[L"Email"]);
					common_meta_was_created = true;
				}
			}
			else {
				// reading other than #meta lines
				// replace , with ; (Buch data)
				strInp = this->strrep(strInp, ',', ';');
				// separate test data line on ;
				vector <wstring> line_data = this->strsplit(strInp, L";", false);
				// to check for empty csv files, if columns are less than 3 skip them
				// less than 3, because sometimes it can contains dummy values
				if (line_data.size() < 3) {
					continue;
				}
				// check type of line (col types, var names, units or test data)
				if (strInp.find(L"Columns type") != wstring::npos) {
					column_types = line_data;
					// iterate through column types, if any is empty report to user
					for (int i = 0; i < column_types.size(); i++) {
						if (column_types[i].empty()) {
							wstring col_name = this->get_excel_col_name(i+1);
							wcout << L"WARNING: Empty entry for Column Types at " << to_wstring(line_count) + col_name << L". Column "
								<< col_name << L" will be skipped!" << endl;
						}
					}
				}
				else if (strInp.find(L"Variables") != wstring::npos) {
					variables = line_data;
					// iterate through param names, if any is empty report to user 
					for (int i = 0; i < variables.size(); i++) {
						if (variables[i].empty()) {
							wstring col_name = this->get_excel_col_name(i+1);
							wcout << L"WARNING: Empty entry for Variables at " << to_wstring(line_count) + col_name << L". Column "
								<< col_name << L" will be skipped!" << endl;
						}
					}
				}
				else if (strInp.find(L"Units") != wstring::npos) {
					units = line_data;
				}
				else if (strInp.find(L"LSL") != wstring::npos) {
					lsl = line_data;
				}
				else if (strInp.find(L"USL") != wstring::npos) {
					usl = line_data;
				}
				else if (strInp[0] == '#' || all_of(line_data.begin(), line_data.end(), [](wstring elem) { return elem.compare(L"") == 0; })) {
					// skip all other rows starting with # or having empty vals
				}
				else {
					// if none of the above, then it's a test data
					test_data = line_data;
					// key_name wstring (e.g. conv_VIO)
					wstring key_name = L"";
					// init meta data struct to construct meta_data
					map <wstring, wstring> meta_data;
					// wstring containing combination of conditions
					wstring cond_str = L"";
					wstring key_cond_str = L"";
					int scale{};
					wstring unit{};
					wstring scaled_value{};
					vector<wstring> comments{};
					// iterate through each column in current row to build meta_data
					for (int current_col = 0; current_col < test_data.size(); current_col++) {
						// check if current data value doesn't correspond to any column header
						if (current_col >= column_types.size()) {
							// save line number to report the error
							no_col_match_lines[csv_files[i]].push_back(line_count);
							continue;
						}
						// check if param name is not present skip column
						if (variables[current_col].empty()) {
							continue;
						}
						// check if current column corresponds to parameter
						if (column_types[current_col].compare(L"param") == 0) {
							// construct meta_data key name (e.g. conv_VIO)
							key_name = L"cond_" + variables[current_col];
							// handle special cases
							if (this->convert_to_lower(key_name).compare(L"cond_temp") == 0) {
								key_name = L"cond_tambient";
								// if temperature is empty, make it 0
								if (test_data[current_col].empty()) {
									wcout << L"TEMP IS EMPTY AT " << line_count << endl;
									test_data[current_col] = L"0";
								}
							}
							else if (key_name.compare(L"cond_vio") == 0) {
								key_name = L"cond_VIO";
							}
							// combine conditions
							cond_str = cond_str + L"_" + test_data[current_col];
							cond_str = cond_str + username + L"_" + basic_type + L"_" + product_sales_code + L"_" + product_design_step + L"_" +
								package + L"_" + dut_id;
							meta_data[key_name] = test_data[current_col];
							// add each condition to the png_file_match_conditions with values. add [ as end of condition (e.g. vio=3[V])
							file_match_conditions.push_back(variables[current_col] + L"=" + test_data[current_col] + L"[");
						}
						// check if current column corresponds to comment, except if variable is picture path
						if (this->convert_to_lower(column_types[current_col]).find(L"comment") != wstring::npos && variables[current_col] != L"PicturePath" &&
							!test_data[current_col].empty()) {
							comments.push_back(test_data[current_col]);
						}
					}
					// get cond_link as path to the folder containing current CSV file
					meta_data[L"cond_link_screenshots"] = L"file:///" + this->strrep(csv_files[i].substr(0, csv_files[i].find_last_of(L"\\")), '\\', '/');
					meta_data[L"cond_link_raw_data"] = L"file:///" + this->strrep(csv_files[i].substr(0, csv_files[i].find_last_of(L"\\")), '\\', '/');
					// get cond_link_waveforms
					int matching_mat_files_count = 0;
					wstring matching_mat_filename{};
					for (auto mat_file: mat_files) {
						// if there are multiple .mat files for the current parent folder name 
						// get them based on conditions match
						if (mat_file.find(parent_folder) != wstring::npos) {
							matching_mat_files_count++;
						}
					}
					/*
					// FOR CONSTRUCTING BASED ON THE SPECIFIC FILENAME FOR MAT EXPLORER TOOL
					vector<wstring> matching_mat_files{};
					if (matching_mat_files_count > 1) {
						matching_mat_files = get_corresponding_files(file_match_conditions, mat_files);
						// there is only one mat file per condition combination
						if (matching_mat_files.size() > 0) {
							matching_mat_filename = matching_mat_files[0];
						}
					}
					else if (matching_mat_files_count == 1) {
						// only one mat file, matching is done based on parent folder name
						for (auto mat_file : mat_files) {
							if (mat_file.find(parent_folder) != wstring::npos) {
								matching_mat_filename = mat_file;
							}
						}
					}
					*/
					// since for now we use only folder name, it doesn't matter how many files matched. All of them are in the same folder
					for (auto mat_file : mat_files) {
						if (mat_file.find(parent_folder) != wstring::npos) {
							matching_mat_filename = mat_file;
							break;
						}
					}
					// constuct proper cond_link_waveforms if matching mat file was found
					if (!matching_mat_filename.empty()) {
						// TODO: uncomment this for cond_link_waveforms
						// meta_data[L"cond_link_waveforms"] = L"file:///" + waveform_explorer_path + L" /k " + matching_mat_filename;
						// meta_data[L"cond_link_waveforms"] = strrep(meta_data[L"cond_link_waveforms"], '\\', '/');
						meta_data[L"cond_link_waveforms"] = L"file:///" + this->strrep(matching_mat_filename.substr(0, matching_mat_filename.find_last_of(L"\\")), '\\', '/');
					}
					// wcout << meta_data[L"cond_link_waveforms"] << endl;

					repeated_conds[cond_str][csv_files[i]].push_back(line_count);
					// iterate through each col again and for each out param
					// construct dataObject with payload + meta_data
					for (int current_col = 0; current_col < test_data.size(); current_col++) {
						// check if current data value doesn't correspond to any column header
						if (current_col >= column_types.size()) {
							continue;
						}
						// check if param name is not present skip column
						if (variables[current_col].empty()) {
							continue;
						}
						if (column_types[current_col].compare(L"out") == 0) {
							// skip if empty
							if (test_data[current_col].empty()) {
								continue;
							}
							// init structre to keep payload
							map <wstring, wstring> payload;
							// construct key_name from variables row, e.g. ibat_stb
							key_name = variables[current_col];
							// validate key_name
							key_name = this->validate_param_name(key_name);
							// add out param name to keep param conds str separately
							key_cond_str = key_name + cond_str;
							
							// add current key_cond_str to repeated cond (to save first occurrence)
							// scale according to unit
							tie(scale, unit) = this->get_unit_scale(units[current_col]);
							scaled_value = this->scale_value(scale, test_data[current_col]);
							payload[key_name] = scaled_value;
							
							// if there are matching png files save them to payload
							file_match_conditions.push_back(L"Report-Picture");
							vector<wstring> matching_png_files = get_corresponding_files(file_match_conditions, png_files);
							file_match_conditions.pop_back();
							/* DEBUG
							if (scaled_value == L"0.277049") {
								vector<wstring> matching_png_files = get_corresponding_files(file_match_conditions, png_files);
								cout << "Conditions AFTER" << endl;
								for (auto condition : file_match_conditions) {
									wcout << condition << endl;
								}
								cout << "Matching files" << endl;
								for (auto matching_png_file : matching_png_files) {
									wcout << matching_png_file << endl;
								}
							}
							else {
								continue;
							}
							*/
							
							// save related png files to current payload
							for (auto i = 0; i < matching_png_files.size(); i++) {
								payload[L"png_filename___" + to_wstring(i)] = strrep(matching_png_files[i], '\\', '/');
							}
							// save related comments
							for (auto i = 0; i < comments.size(); i++) {
								payload[L"comment___" + to_wstring(i)] = comments[i];
							}
							// get corresponding .mat files
							file_match_conditions.push_back(L"Report-waveform");
							vector<wstring> matching_mat_files = get_corresponding_files(file_match_conditions, mat_files);
							file_match_conditions.pop_back();
							// save related .mat files
							for (auto i = 0; i < matching_mat_files.size(); i++) {
								payload[L"mat_filename___" + to_wstring(i)] = strrep(matching_mat_files[i], '\\', '/');
							}
							// add other meta fields
							meta_data[L"test_name"] = key_name;
							meta_data[L"data_object_type"] = L"value";
							meta_data[L"dut_id"] = dut_id;
							meta_data[L"package"] = package;
							meta_data[L"user_name"] = username;
							// add rddf_tc_id only if api_id and global_id are set
							if (!api_id.empty() && !global_id.empty()) {
								meta_data[L"rddf_tc_id"] = api_id + L":" + global_id;
							}
							meta_data[L"test_program_name"] = test_program_name;
							meta_data[L"test_program_revision"] = testunit_version;

							// add test number from limits if it exists, otherwise hardcode
							if (limits_struct.find(key_name) != limits_struct.end()) {
								// get test number from limits
								meta_data[L"test_number"] = limits_struct[key_name][L"TestNr"];
							}
							else {
								// if limit doesn't exist, check if hardcoded test number already exists
								if (unique_params.find(key_name) != unique_params.end()) {
									// use already assigned test number
									meta_data[L"test_number"] = to_wstring(unique_params[key_name]);
								}
								else if (unique_params.empty()) {
									// first unique parameter. Add test number manually and increment test_number_counter
									meta_data[L"test_number"] = to_wstring(test_number_counter);
								}
								else {
									// otherwise assign a new unique test number
									// by incrementing test_counter while uniqueness is achieved
									// to avoid overlap with test numbers from limits file
									bool found_unique = false;
									while (!found_unique) {
										for (map <wstring, int>::value_type& unique_param : unique_params) {
											if (unique_param.second == test_number_counter) {
												found_unique = false;
												break;
											}
											else {
												found_unique = true;
											}
										}
										test_number_counter++;
									}
									meta_data[L"test_number"] = to_wstring(test_number_counter);
								}
							}

							// create dataObject for current out value with payload and meta_data
							map <wstring, map<wstring, wstring>> data_object;
							data_object[L"payload"] = payload;
							data_object[L"metaData"] = meta_data;
							
							// if key_cond_str is already in internal_json, condition repetition occurred
							// mark flag true to inform user
							if (internal_json.find(key_cond_str) != internal_json.end()) {
								cond_repetition = true;
							}
							
							// store current metaData and payload in internal_json
							internal_json[key_cond_str] = data_object;
							// check if current parameter is not in unique_params,
							// add a limit for it
							if (unique_params.find(key_name) == unique_params.end()) {
								// create a payload for current limit
								map <wstring, wstring> limit_payload;
								// create a meta_data for current limit
								map <wstring, wstring> limit_meta_data;
								// define limit_struct to store single limit structure
								map<wstring, wstring> limit_struct;
								if (!lsl.empty() && !usl.empty() && !lsl[current_col].empty() && !usl[current_col].empty() 
											&& current_col < lsl.size() && current_col < usl.size()) {
									// get scale, unit
									tie(scale, unit) = this->get_unit_scale(units[current_col]);
									// hardcode scale 0, because tembo does auto conversion
									limit_payload[L"scale"] = L"NA";
									limit_payload[L"unit"] = unit;

									// deal with no limits: NaN
									// get lower limit
									if (lsl.at(current_col).find(L"NaN")==0) {
										limit_payload[L"lower_limit"] = L"";
									}
									else {
										scaled_value = this->scale_value(scale, lsl[current_col]);
										limit_payload[L"lower_limit"] = scaled_value;
									}
									// get upper limit scaled value
									if (usl.at(current_col).find(L"NaN")==0) {
										limit_payload[L"upper_limit"] = L"";
									}
									else {
										scaled_value = this->scale_value(scale, usl[current_col]);
										limit_payload[L"upper_limit"] = scaled_value;
									}

									req_id = L"";
									description = L"";
									typical = L"";
									test_number = to_wstring(test_number_counter);
									// wcout << L"Getting from USL: " << usl[current_col] << endl;
								}
								else if (limits_struct.find(key_name) != limits_struct.end()) {
									// get the current limit structure
									for (map<wstring, map<wstring, wstring>>::value_type& iter : limits_struct) {
										if (iter.first == key_name) {
											for (map<wstring, wstring>::value_type& iter_obj : iter.second) {
												limit_struct[iter_obj.first] = iter_obj.second;
											}
										}
									}
									
									// get scale, unit
									tie(scale, unit) = this->get_unit_scale(limit_struct[L"Unit"]);
									// hardcode scale 0, because tembo does auto conversion
									limit_payload[L"scale"] = L"NA";
									limit_payload[L"unit"] = unit;

									// get lower limit
									scaled_value = this->scale_value(scale, limit_struct[L"LSL"]);
									limit_payload[L"lower_limit"] = scaled_value;

									// get upper limit scaled value
									scaled_value = this->scale_value(scale, limit_struct[L"USL"]);
									limit_payload[L"upper_limit"] = scaled_value;

									// add meta data from limit struct
									req_id = limit_struct[L"ReqID"];
									description = limit_struct[L"Description"];
									typical = limit_struct[L"Typ"];
									test_number = limit_struct[L"TestNr"];
								}
								else {
									// use hardcoded limits
									// get scale and unit
									tie(scale, unit) = this->get_unit_scale(units[current_col]);
									limit_payload[L"unit"] = unit;
									// hardcode scale to 0, because tembo does auto conversion
									limit_payload[L"scale"] = L"NA";

									// get upper limit
									// limit_payload["upper_limit"] = generate_limit_from_test_value(payload[key_name], true);

									// get lower limit
									// limit_payload["lower_limit"] = generate_limit_from_test_value(payload[key_name], false);

									// Back to empty limits
									limit_payload[L"upper_limit"] = L"";
									limit_payload[L"lower_limit"] = L"";

									req_id = L"";
									description = L"";
									typical = L"";
									test_number = to_wstring(test_number_counter);
									// save no matches in txt
									no_limit_match.push_back(key_name);
								}
								// construct limit meta data
								limit_meta_data = this->construct_limit_meta_data(common_meta_data, req_id, description, typical, test_number, key_name);

								// create a data object for current limit
								map <wstring, map<wstring, wstring>> limit_data_object;
								limit_data_object[L"payload"] = limit_payload;
								limit_data_object[L"metaData"] = limit_meta_data;

								// add limit_data_object to data_objects
								data_objects.push_back(limit_data_object);
								// store unique out params to add limits
								// check if it has defined limits or hard coded
								if (limits_struct.find(key_name) != limits_struct.end() && usl.empty() && lsl.empty()) {
									unique_params[key_name] = stoi(limit_struct[L"TestNr"]);
								}
								else {
									unique_params[key_name] = test_number_counter++;
								}
							}
						}
					}
					// clear png file match conditions (skip first two for parent folder and dut it)
					while (file_match_conditions.size() > 2) {
						file_match_conditions.pop_back();
					}
				}
			}
		} // finished reading current csv -> while(inf)

		// since current csv is done, copy remaining internal json objects into
		// data_objects, because new file will have different params
		for (map <wstring, map<wstring, map<wstring, wstring>>>::value_type& data_object : internal_json) {
			data_objects.push_back(data_object.second);
			
		}
	}

	if (no_limit_match.size() > 0) {
		// wcout << endl << L"WARNING: Detected parameters without limits (applied hardcoded limits).. For more details please check 50_Report/No_Limits.csv" << endl << endl << endl;
		wofstream out(out_folder_path + L"\\No_Limit_Match.csv");
		for (auto no_limit : no_limit_match) {
			out << no_limit + L"\n";
		}
		out.close();
	}

	if (no_col_match_lines.size() > 0) {
		wcout << endl << L"WARNING: Detected values that are not correponding to any columns (values omitted)... For more details please check " << 
			L"50_Report/No_Col_Match.csv" << endl << endl << endl;
		wofstream out(out_folder_path + L"\\No_Col_Match.csv");
		out << L"File;Lines";
		for (map<wstring, vector<int>>::value_type& no_col_match_line : no_col_match_lines) {
			out << L"\n" << no_col_match_line.first << L";";
			for (auto line_number : no_col_match_line.second) {
				out << line_number << L"\n;";
			}
		}
	}

	if (cond_repetition) {
		cout << L"WARNING: Repeated condition occured (saved only last occurence).. For more details please check" 
			<< L"50_Report/CSVs_repeated_conditions.csv" << endl << endl << endl;
		wofstream out(out_folder_path + L"\\CSVs_repeated_conditions.csv");
		out << L"File;Lines\n";
		for (map<wstring, map<wstring, vector<int>>>::value_type& repeated_cond_str : repeated_conds) {
			for (map<wstring, vector<int>>::value_type& repeated_cond : repeated_cond_str.second) {
				if (repeated_cond.second.size() > 1) {
					out << repeated_cond.first << L";";
					for (auto data : repeated_cond.second) {
						out << data << L";";
					}
					out << L"\n";
				}
			}
		}
	}

	// create recipe payload
	wstring recipe_payload = this->construct_recipe(configs_struct[L"ReportTemplate"], configs_struct[L"ReportName"], configs_struct[L"Project"]);

	bool res = this->json_writer(header_struct, common_meta_data, &data_objects, out_folder_path + L"\\" + configs_struct[L"ReportName"] + L".json", recipe_payload);

	return res;
}

vector<wstring>CSVReader::get_corresponding_files(vector<wstring> file_match_conditions, vector<wstring> files) {
	vector<wstring> matching_files{};
	for (auto file : files) {
		// check how many conditions are given in the current filename based on the number of '=' chars
		// there is always should be at least 1 occurence for dut_id / sample, the rest are for conditions
		int num_of_conds_in_filename = this->count_char_occurence(file, '=');
		// add one more condition for matching parent folder name
		num_of_conds_in_filename++;
		// add one more condition for matching Report-Picture or Report-waveform names
		num_of_conds_in_filename++;
		int num_of_conds_matched = 0;
		for (auto file_match_cond : file_match_conditions) {
			// count number of conditions that match with conditions in the filename
			if (convert_to_lower(file).find(convert_to_lower(file_match_cond)) != wstring::npos) {
				num_of_conds_matched++;
			}
		}
		// png file is matched if the number of total matched conditions are same as the number of 
		// conditions in the filename
		if (num_of_conds_matched == num_of_conds_in_filename) {
			wstring base_filename = wstring(file).substr(wstring(file).find_last_of(L"/\\") + 1);
			matching_files.push_back(base_filename);
		}
	}
	return matching_files;
}
