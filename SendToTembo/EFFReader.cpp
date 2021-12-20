#include "EFFReader.h"

/*************************************************************************************************************************************************************************
* maintainer Xing Jin (IFAG ATV PS PD MUC CVSV)
* since v4.0.0
*
* date		27.01.2021
*
* author	Ali Ganbarov (IFAG ATV PSN D PD PRES)
*************************************************************************************************************************************************************************/


EFFReader::EFFReader() {
}

EFFReader::~EFFReader() {
}

bool EFFReader::eff_to_json(wstring eff_path, map<wstring, wstring> configs_struct, wstring out_folder_path) {
	// define header struct
	map<wstring, wstring> header_struct;
	header_struct[L"version"] = L"1.0.1";

	// define common_meta_data
	map <wstring, wstring> common_meta_data;
	bool common_meta_was_created = false;
	vector<map<wstring, map<wstring, wstring>>> data_objects;
	// define internal_json as array of maps
	map<wstring, map<wstring, map<wstring, wstring>>> internal_json;
	// define unique params to keep track of already appeared params
	// used to control adding limit only once for each param
	map <wstring, int> unique_params;

	// define meta data variables
	wstring username = L"";
	wstring product_sales_code = L"";
	wstring basic_type = L"";
	wstring product_design_step = L"";
	wstring package = L"";
	wstring dut_id = L"";
	vector <wstring> conds;
	vector <wstring> params;
	vector <wstring> units;
	vector <wstring> usl;
	vector <wstring> lsl;
	vector <wstring> test_data;

	// keep count of lines in file
	int line_count = 0;
	// define structure to keep repeated condition data for output
	map<wstring, map<wstring, vector<int>>> repeated_conds;
	bool cond_repetition = false;
	int test_col_ind{};

	// get report name from file name
	wstring base_filename = eff_path.substr(eff_path.find_last_of(L"/\\") + 1);
	wstring::size_type const p(base_filename.find_last_of('.'));
	wstring file_without_extension = base_filename.substr(0, p);
	wstring report_name = file_without_extension;

	// start reading file
	wifstream inf(eff_path);
	if (!inf) {
		wcout << L"Couldn't read eff file: " << eff_path << endl;
		return false;
	}
	locale loc("");
	inf.imbue(loc);
	while (inf) {
		wstring strInp;
		// read line
		getline(inf, strInp);
		line_count++;
		// remove " from line to avoid JSON crash
		strInp = this->strremove(strInp, '"');
		// remove ' from line to avoid Tembo crash
		strInp = this->strremove(strInp, '\'');
		// split line on ;
		vector <wstring> line_data = this->strsplit(strInp, L";", false);
		// get username
		if (strInp.find(L"<<EFF:1.00>>") != wstring::npos) {
			// if username is not in configs
			if (configs_struct[L"Username"].empty()) {
				// iterate through line_data till Ref is contained
				for (auto i = 0; i < line_data.size(); i++) {
					if (line_data[i].find(L"Ref") != wstring::npos) {
						vector <wstring> key_val;
						key_val = this->strsplit(line_data[i], L"=");
						username = key_val[1];
					}
				}
			}
			else {
				username = configs_struct[L"Username"];
			}
		}
		// get condition names
		else if (strInp.find(L"<+EFF:1.00>") != wstring::npos) {
			conds = line_data;
			// get the col index of where test data starts
			for (auto i = 0; i < conds.size(); i++) {
				// convert to numeric wherever possible
				double second;
				wistringstream iss(conds[i]);
				iss >> dec >> second;
				if (!iss.fail()) {
					// if conversion was successful, then test data cols started (conds can't be numeric value, 
					// so it's a test number)
					test_col_ind = i;
					break;
				}
			}
		}
		// get param names
		else if (strInp.find(L"<+PName>") != wstring::npos) {
			params = line_data;
		}
		// get units
		else if (strInp.find(L"<Unit>") != wstring::npos) {
			units = line_data;
		}
		// get upper limits
		else if (strInp.find(L"<USL>") != wstring::npos) {
			usl = line_data;
		}
		// get lower limits
		else if (strInp.find(L"<LSL>") != wstring::npos) {
			lsl = line_data;
		}
		// get test data
		else if (strInp.find(L"05_Die") != wstring::npos) {
			// row containing test values
			test_data = line_data;
			// init meta data struct to construct meta_data
			map<wstring, wstring> meta_data;
			// wstring containing combination of conditions
			wstring cond_str = L"";
			wstring key_cond_str = L"";
			// iterate through each piece of line_data for meta data, start from 1 
			// (skip 05_Die) till beginning of actual test values
			for (int col = 1; col < test_col_ind; col++) {
				test_data[col] = this->strremove(test_data[col], ',');
				// meta names are in conds
				if (!conds[col].compare(L"design")) {
					// split on _
					vector<wstring> tokens = this->strsplit(test_data[col], L"_");
					basic_type = tokens[0];
					product_design_step = tokens[1];
					if (tokens.size() > 2) {
						product_sales_code = tokens[2];
					}
					else {
						product_sales_code = basic_type;
					}
				}
				else if (!conds[col].compare(L"dut")) {
					dut_id = test_data[col];
				}
				else {
					// construct conds
					wstring key_name = L"cond_" + conds[col];
					if (this->convert_to_lower(key_name).compare(L"cond_temp") == 0) {
						key_name = L"cond_tambient";
						// if temperature is empty, make it 0
						if (test_data[col].empty()) {
							test_data[col] = L"0";
						}
					}
					else if (key_name.compare(L"cond_vio") == 0) {
						key_name = L"cond_VIO";
					}
					else if (test_data[col].empty()) {
						// skip empty condition (other than temp)
						// cond_str += L"_";
						// if (key_name.compare(L"cond_VIO") == 0) {
						// 	test_data[col] = L"0";
						// } else {
						// 	continue;
						// }
						// saving 0 value for missing conditions to avoid Tembo undefine column error
						test_data[col] = L"0";
					}
					// combine conditions
					cond_str = cond_str + L"_" + test_data[col];
					// get value from current cell of the table
					meta_data[key_name] = test_data[col];
				}

			}
			//cond_str = cond_str + username + L"_" + basic_type + L"_" + product_sales_code + L"_" + product_design_step + L"_" +
			//	package + L"_" + dut_id;
			cond_str = cond_str + username;
			repeated_conds[cond_str][eff_path].push_back(line_count);
			if (!common_meta_was_created && basic_type != L"" && product_sales_code != L"" && product_design_step != L"") {
				common_meta_data = this->construct_common_meta_data(basic_type, product_design_step, product_sales_code, username, configs_struct[L"Email"]);
				common_meta_was_created = true;
			}
			// Once meta is ready, read the rest of row for test objects
			for (auto col = test_col_ind; col < test_data.size(); col++) {
				// skip if empty
				if (test_data[col].empty()) {
					continue;
				}
				// init structre to keep payload
				map <wstring, wstring> payload;
				// construct key_name from variables row, e.g. ibat_stb
				wstring key_name = params[col];
				// validate parameter name
				key_name = this->validate_param_name(key_name);
				if (key_name.empty()) {
					continue;
				}
				// add out param name to keep param conds str separately
				key_cond_str = key_name + cond_str;
				// scale according to unit
				int scale{};
				wstring unit{};
				wstring scaled_value{};
				// get the scale for the payload
				tie(scale, unit) = this->get_unit_scale(units[col]);
				payload[key_name] = this->scale_value(scale, test_data[col]);
				// add other meta fields
				meta_data[L"test_name"] = key_name;
				meta_data[L"data_object_type"] = L"value";
				meta_data[L"dut_id"] = dut_id;
				// meta_data["package"] = package;
				meta_data[L"user_name"] = username;
				// set test number depending on if it's api or actual test number
				if (configs_struct[L"api_id_perl"].empty()) {
					if (conds[col].empty()) {
						// if empty, take column number
						meta_data[L"test_number"] = to_wstring(col);
					}
					else {
						// take test number as it is
						meta_data[L"test_number"] = conds[col];
					}
				}
				else {
					// if api key is present take test number as column number
					meta_data[L"test_number"] = to_wstring(col);
					// construct rddf_tc_id
					meta_data[L"rddf_tc_id"] = configs_struct[L"api_id_perl"] + L":" + L"GID-" + conds[col];
					// Fill in dummy values (Christian request)
					meta_data[L"testunit_name"] = L"dummy";
					meta_data[L"testunit_version"] = L"dummy";
				}

				// create dataObject for current out value with payload and meta_data
				map <wstring, map<wstring, wstring>> data_object;
				data_object[L"payload"] = payload;
				data_object[L"metaData"] = meta_data;
				// if key_cond_str is already in internal_json
				// mark flag true to inform user
				if (internal_json.find(key_cond_str) != internal_json.end()) {
					// cout << "Repeated condition on line: " << line_count<< endl;
					cond_repetition = true;
				}

				// story current metaData and payload in internal_json
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

					// construct limit payload
					// hardcode scale to 0, because tembo does auto conversion
					// New: setting scale to 0 leads to the scaling issue in tembo report, leave it empty and tembo will do auto conversion 
					//limit_payload[L"scale"] = L"0";
					
					// get scale and unit
					tie(scale, unit) = this->get_unit_scale(units[col]);
					limit_payload[L"unit"] = unit;
					//limit_payload[L"scale"] = scale;

					// get lower limit
					wstring lower_limit{};
					if (lsl[col].empty()) {
						// hardcode
						// limit_payload["lower_limit"] = generate_limit_from_test_value(payload[key_name], false);
						// back to empty limit
						limit_payload[L"lower_limit"] = L"";
					}
					else {
						// scale limit acc to unit
						limit_payload[L"lower_limit"] = this->scale_value(scale, lsl[col]);
					}

					// get upper limit
					if (usl[col].empty()) {
						// hardcode
						// limit_payload["upper_limit"] = generate_limit_from_test_value(payload[key_name], true);
						// back to empty limit
						limit_payload[L"upper_limit"] = L"";
					}
					else {
						limit_payload[L"upper_limit"] = this->scale_value(scale, usl[col]);
					}

					// get limit meta data
					limit_meta_data = this->construct_limit_meta_data(common_meta_data, L"", L"", L"", meta_data[L"test_number"], key_name);

					// create a data object for current limit
					map <wstring, map<wstring, wstring>> limit_data_object;
					limit_data_object[L"payload"] = limit_payload;
					limit_data_object[L"metaData"] = limit_meta_data;

					// add limit_data_object to internal_json
					internal_json[L"limit_for_" + key_name] = limit_data_object;
					// store unique out params to prevent readding limit again
					unique_params[key_name] = 1;
				}
			}
		}
	}

	// since current csv is done, copy remaining internal json objects into
	// data_objects, because new file will have different params
	for (map <wstring, map<wstring, map<wstring, wstring>>>::value_type& data_object : internal_json) {
		data_objects.push_back(data_object.second);
		internal_json.erase(data_object.first);
	}

	if (cond_repetition) {
		wcout << L"Repeated condition occured (saved only last occurence).. For more details please check 50_Report/" + report_name + L"_repeated_conditions.csv"
			<< endl << endl << endl;
		wofstream out(out_folder_path + L"\\" + report_name + L"_repeated_conditions.csv");
		out << L"Lines\n";
		for (map<wstring, map<wstring, vector<int>>>::value_type& repeated_cond_str : repeated_conds) {
			for (map<wstring, vector<int>>::value_type& repeated_cond : repeated_cond_str.second) {
				if (repeated_cond.second.size() > 1) {
					//out << repeated_cond_str.first << ";";
					for (auto data : repeated_cond.second) {
						out << data << L";";
					}
					out << L"\n";
				}
			}
		}
	}

	// notify user about api_id_perl, if present
	if (!configs_struct[L"api_id_perl"].empty()) {
		wcout << endl << L"WARNING: api_id_perl is detected in Config_Tembo file. Test numbers are filled with dummy values!" << endl << endl;
	}

	// create recipe payload
	wstring recipe_payload = this->construct_recipe(configs_struct[L"ReportTemplate"], report_name, configs_struct[L"Project"]);

	bool res = this->json_writer(header_struct, common_meta_data, &data_objects, out_folder_path + L"\\" + report_name + L".json", recipe_payload);
	
	return res;
}