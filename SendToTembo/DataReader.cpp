#include "DataReader.h"

/*************************************************************************************************************************************************************************
* maintainer Xing Jin (IFAG ATV PS PD MUC CVSV)
* since v4.0.0
*
* date		27.01.2021
*
* author	Ali Ganbarov (IFAG ATV PSN D PD PRES)
*************************************************************************************************************************************************************************/

DataReader::DataReader()
{
}


DataReader::~DataReader()
{
}

map <wstring, wstring> DataReader::read_config_file(const wstring& config_file_path) {
	vector <wstring> config_arr;
	map <wstring, wstring> configs_struct;
	wifstream inf(config_file_path);
	if (!inf) {
		wcout << L"Couldn't read config file: " << config_file_path << endl;
		exit(1);
	}
	cout << "CONFIG READER: " << endl;
	while (inf) {
		wstring strInp;
		// read line
		getline(inf, strInp);
		// split on :
		config_arr = this->strsplit(strInp, L":");
		// skip if empty
		if (config_arr.size() < 1) {
			continue;
		}
		// convert project name to all lower case
		if (config_arr[0] == L"Project") {
			// convert project name to lower
			config_arr[1] = convert_to_lower(config_arr[1]);
		}
		// trim key and value
		config_arr[0] = this->strtrim(config_arr[0]);
		config_arr[1] = this->strtrim(config_arr[1]);
		// save key => value pair in configs_struct
		if (!config_arr[0].empty()) {
			configs_struct[config_arr[0]] = config_arr[1];
		}
	}

	return configs_struct;
}

map<wstring, wstring> DataReader::setup_configurations(map<wstring, wstring> configs_struct, bool is_csv) {
	map<wstring, wstring> final_configs;
	// Setup configurations
	// init recipe variables, try to use config, otherwise set to default
	wstring project_name = L"psn-general";
	wstring report_template = L"48292680-1751-43d9-beb3-e511e156641e";
	wstring report_name = L"Simple Report";
	wstring email = L"syed.ghaznavi@infineon.com"; // by default
	wstring api_id_perl = L"";
	wstring username = L"";
	bool default_email = true;
	for (map<wstring, wstring>::value_type& config : configs_struct) {
		wstring key = this->convert_to_lower(config.first);
		if (key == L"project") {
			project_name = config.second;
		}
		else if (key == L"report_template") {
			report_template = config.second;
		}
		else if (key == L"name_report") {
			report_name = config.second;
		}
		else if (key == L"email") {
			email = config.second;
			default_email = false;
		}
		else if (key == L"api_id_perl") {
			api_id_perl = config.second;
		}
		else if (key == L"username") {
			username = config.second;
		}
	}
	if (default_email) {
		wcout << endl << L"No configuration for email found in 'Config_Tembo.txt'" << endl;
		wcout << L"Default email: " << email << endl;
	}
	
	

	// set variables in structure
	final_configs[L"Project"] = project_name;
	final_configs[L"ReportTemplate"] = report_template;
	final_configs[L"Email"] = email;
	final_configs[L"api_id_perl"] = api_id_perl;
	final_configs[L"Username"] = username;
	if (is_csv) {
		final_configs[L"ReportName"] = report_name;
		wcout << endl << L"CSV Configurations" << endl;
		wcout << L"Report name: " << report_name << endl;
	}
	else {
		wcout << endl << L"EFF Configurations" << endl;
	}
	wcout << L"Project name: " << project_name << endl << L"Report template: " << report_template << endl;
	wcout << L"Email: " << email << endl;
	wcout << endl;

	return final_configs;
}

bool DataReader::json_writer(map<wstring, wstring> header, map<wstring, wstring> common_meta_data, 
	vector<map<wstring, map<wstring, wstring>>> *data_objects, wstring json_path, wstring recipe_payload) {
	typedef std::chrono::high_resolution_clock clock;
	typedef std::chrono::duration<float, std::milli> mil;
	int c{};
	chrono::time_point<chrono::steady_clock> t0, t1;

	// open file, write to file by chunks
	wstring json_chunk;
	wofstream out(json_path);

	wcout << L"Writing JSON.." << endl;

	// open json {
	json_chunk = L"{\n";
	// write header
	json_chunk += L"\"header\":\n\t{\n\t\t\"version\":\"1.0.1\"\n\t},\n";

	// write common_meta_data
	// open commonMetaData tag
	json_chunk += L"\"commonMetaData\":\n\t{";
	// write common_meta_data items
	for (map<wstring, wstring>::value_type& com_meta : common_meta_data) {
		// try to convert to integer wherever possible
		double second;
		wstring str_second;
		wistringstream iss(com_meta.second);
		iss >> dec >> second;
		json_chunk += L"\n\t\t";
		if (iss.fail() || com_meta.first == L"ts_data_created") {
			// couldn't convert, write as wstring
			json_chunk = json_chunk + L"\"" + com_meta.first + L"\":\"" + com_meta.second + L"\",";
		}
		else {
			// success write as int
			str_second = to_wstring(second);
			str_second = this->strrep(str_second, ',', '.');
			str_second = str_second.erase(str_second.find_last_not_of('0') + 1, wstring::npos);
			if (str_second[str_second.size() - 1] == '.') {
				str_second = this->strremove(str_second, '.');
			}
			json_chunk = json_chunk + L"\"" + com_meta.first + L"\":" + str_second + L",";
		}

	}
	// remove last ,
	json_chunk = json_chunk.substr(0, json_chunk.size() - 1);
	// close commonMetaData tag
	json_chunk += L"\n\t},\n";

	// write to file and reset
	out << json_chunk;
	json_chunk = L"";

	// calculate step size for progress bar
	int progress_step{};
	int initial_size = data_objects->size();
	if (data_objects->size() < 100) {
		progress_step = 1;
	}
	else {
		progress_step = ceil(data_objects->size() / 100.0);
	}
	// write data objects
	// open dataObjects tag
	json_chunk += L"\"dataObjects\":[";
	wcout << data_objects->size() << L" data objects" << endl;
	while(!data_objects->empty()) {
		map<wstring, map<wstring, wstring>> data_objects_element = data_objects->back();
		data_objects->pop_back();
		// open item tag {
		json_chunk += L"\n\t{";
		//json_chunk += "{";
		for (map<wstring, map<wstring, wstring>>::value_type& data_object : data_objects_element) {
			// open data_object tag (meta_data or payload)
			json_chunk += L"\n\t\t";
			json_chunk = json_chunk + L"\"" + data_object.first + L"\":\n\t\t\t{";
			//json_chunk = json_chunk + "\"" + data_object.first + "\":{";
			bool raw_data_link_opening_tag_created = false;
			bool comment_opening_tag_created = false;

			for (map<wstring, wstring>::value_type& field : data_object.second) {
				// convert to integer wherever possible
				// double second;
				// wstring str_second;
				// wistringstream iss(field.second);
				// iss >> dec >> second;
				json_chunk += L"\n\t\t\t\t";
				

				// check if png filename
				if (field.first.find(L"png_filename___") != wstring::npos) {
					// if raw_data_link opening tag was created, then it's not the first 
					// filename. Remove the last character \n\t\t\t\t], characters
					if (raw_data_link_opening_tag_created) {
						json_chunk = json_chunk.substr(0, json_chunk.size() - 7);
						json_chunk += L",";	// close previous one
					}
					// check if raw_data_link tag was already created, if not create
					if (!raw_data_link_opening_tag_created) {
						json_chunk += L"\"raw_data_link\":[";
						raw_data_link_opening_tag_created = true;
					}
					// populate raw_data_link
					json_chunk += L"\n\t\t\t\t\t{";
					json_chunk += L"\n\t\t\t\t\t\t\"type\":\"PNG\",";
					json_chunk += L"\n\t\t\t\t\t\t\"filename\":\"" + field.second + L"\"";
					json_chunk += L"\n\t\t\t\t\t}";
					// close raw_data_link tag
					json_chunk += L"\n\t\t\t\t],";
				}
				else if (field.first.find(L"mat_filename___") != wstring::npos) {
					// if raw_data_link opening tag was created, then it's not the first 
					// filename. Remove the last character \n\t\t\t\t], characters
					if (raw_data_link_opening_tag_created) {
						json_chunk = json_chunk.substr(0, json_chunk.size() - 7);
						json_chunk += L",";	// close previous one
					}
					// check if raw_data_link tag was already created, if not create
					if (!raw_data_link_opening_tag_created) {
						json_chunk += L"\"raw_data_link\":[";
						raw_data_link_opening_tag_created = true;
					}
					// populate raw_data_link
					json_chunk += L"\n\t\t\t\t\t{";
					json_chunk += L"\n\t\t\t\t\t\t\"type\":\"MAT\",";
					json_chunk += L"\n\t\t\t\t\t\t\"filename\":\"" + field.second + L"\"";
					json_chunk += L"\n\t\t\t\t\t}";
					// close raw_data_link tag
					json_chunk += L"\n\t\t\t\t],";
				}
				else if (field.first.find(L"comment___") != wstring::npos) {
					// if comment_opening_tag was created then it's not the first comment
					if (comment_opening_tag_created) {
						json_chunk = json_chunk.substr(0, json_chunk.size() - 7);
						json_chunk += L",";	// close previous one
					}
					// check if comment tag was already created, if not create
					if (!comment_opening_tag_created) {
						json_chunk += L"\"comments\":[";
						comment_opening_tag_created = true;
					}
					// add comment
					json_chunk += L"\n\t\t\t\t\t\"" + field.second + L"\"";
					// close comments tag
					json_chunk += L"\n\t\t\t\t],";
				}
				else {
					json_chunk = json_chunk + L"\"" + field.first + L"\":\"" + field.second + L"\",";
				}
				
				// Writing everything as wstring to save precision for big number conversion to and from scientific version
				// e.g. test_number = 12345678 as number becomes 1.23e6, which converts back to number as 1230000
				/*
				if (iss.fail() || field.first == "ts_data_created" || field.first == "dut_id" || field.first == "package" || field.first.find("cond_") != wstring::npos || 
					field.first == "rddf_tc_id") {
					// write inner object fields of meta or payload
					json_chunk = json_chunk + "\"" + field.first + "\":\"" + field.second + "\",";
				}
				else {
					if (second != 0) {
						// 0 value is converted as empty
						ostringstream strs;
						strs << second;
						str_second = strs.str();
						str_second = this->strrep(str_second, ',', '.');
					}
					else {
						// hardcode 0 wstring to avoid empty value
						str_second = "0";
					}
					json_chunk = json_chunk + "\"" + field.first + "\":" + str_second + ",";
				}
				*/

			}
			// remove last ,
			json_chunk = json_chunk.substr(0, json_chunk.size() - 1);
			// close data_object tag }
			json_chunk += L"\n\t\t\t},";
			// json_chunk += "},";
		}
		// remove last ,
		json_chunk = json_chunk.substr(0, json_chunk.size() - 1);
		// close item tag
		json_chunk += L"\n\t},";
		// json_chunk += "},";

		// write to file every 100 steps to prevent dealing with huge strings
		if (++c % 100 == 0) {
			out << json_chunk;
			json_chunk = L"";
		}
		// update progress bar every {progress_steps}
		if (c % progress_step == 0) {
			wcout << '\r' << this->progress_bar(c, initial_size, progress_step);
		}
	}
	// update progress bar for final chunk
	wcout << '\r' << this->progress_bar(c, initial_size, progress_step);

	// putting recipe
	json_chunk += L"\n\t{";
	json_chunk += L"\n\t\t\"metaData\":\n\t\t\t{\n\t\t\t\t\"data_object_type\":\"recipe\"\n\t\t\t},";
	json_chunk += L"\n\t\t\"payload\":\n\t\t\t{\n\t\t\t\t\"recipe\":";
	json_chunk += L"\"" + recipe_payload + L"\"\n\t\t\t}";
	json_chunk += L"\n\t}";

	// close dataObjects tag
	json_chunk += L"\n]";

	// close json }
	json_chunk += L"\n}";
	// wcout << json << endl;
	// write last chunk
	out << json_chunk;

	out.close();
	wcout << endl << endl << L"JSON is saved in " << endl << json_path << endl << endl;

	return true;
}

vector<wstring> DataReader::strsplit(wstring line, wstring delimiters, bool collapse_delimiters) {
	wstring temp;				// store temporarily built tokens
	vector <wstring> tokens;		// final vector of tokens
								// iteratre over each character in line
	for (auto i = 0; i < line.size(); i++) {
		// if current char is not in delimiters, save it to temp
		if (delimiters.find(line[i]) == wstring::npos) {
			temp += line[i];
		}
		// if current char is delimiting char
		else {
			// if temp is not empty and collapse_delimiters is chosen
			// save current token
			if (temp != L"" && collapse_delimiters) {
				tokens.push_back(temp);
			}
			// if collapse_delimiters is false, add empty token
			if (!collapse_delimiters) {
				tokens.push_back(temp);
			}
			temp = L"";
		}
	}
	// add remaining any non-empty token
	if (temp != L"") {
		tokens.push_back(temp);
	}

	return tokens;
}

map <wstring, wstring> DataReader::construct_common_meta_data(wstring basic_type, wstring product_design_step, wstring product_sales_code, wstring username, wstring email) {
	map <wstring, wstring> common_meta_data;
	// construct ts_data_created
	time_t theTime = time(NULL);
	struct tm *aTime = localtime(&theTime);
	int day = aTime->tm_mday;
	int month = aTime->tm_mon + 1;
	int year = aTime->tm_year + 1900;
	bool month_one_digit = false;
	bool day__one_digit = false;
	if (month < 10)
		month_one_digit = true;
	if (day < 10)
		day__one_digit = true;
	wstring ts_data_created = to_wstring(year) + ( (month_one_digit) ? to_wstring(0): + L"" ) + to_wstring(month) + ( (day__one_digit) ? to_wstring(0) : + L"") + to_wstring(day);
	//wstring ts_data_created = to_wstring(year) + to_wstring(month) + to_wstring(day);
	// construct common meta data
	common_meta_data[L"basic_type"] = basic_type;
	common_meta_data[L"product_design_step"] = product_design_step;
	common_meta_data[L"product_sales_code"] = product_sales_code;
	common_meta_data[L"ts_data_created"] = ts_data_created;
	common_meta_data[L"generator"] = L"C++";
	common_meta_data[L"generator_version"] = L"V11";
	common_meta_data[L"generator_domain"] = L"CV";
	common_meta_data[L"simulator_name"] = L"simulator";
	common_meta_data[L"simulation_type"] = L"type";
	common_meta_data[L"data_object_type"] = L"value";
	common_meta_data[L"data_object_type_version"] = L"1";
	common_meta_data[L"netlist_label"] = L"netlist_label";
	common_meta_data[L"user_name"] = username;
	common_meta_data[L"user_email_address"] = email;
	return common_meta_data;
}

map <wstring, wstring> DataReader::construct_limit_meta_data(map<wstring, wstring> common_meta_data, wstring req_id, wstring description, wstring typical, wstring test_number, wstring key_name) {
	map <wstring, wstring> limit_meta_data;
	// copy common meta_data into limit_meta_data, skip user_name
	for (map<wstring, wstring>::value_type& com_meta : common_meta_data) {
		if (com_meta.first.compare(L"user_name") != 0) {
			limit_meta_data[com_meta.first] = com_meta.second;
		}
	}
	limit_meta_data[L"reqID"] = req_id;
	limit_meta_data[L"description"] = description;
	limit_meta_data[L"typical"] = typical;
	limit_meta_data[L"test_number"] = test_number;
	limit_meta_data[L"p_number"] = L"";
	limit_meta_data[L"parameter_name"] = key_name;
	limit_meta_data[L"data_object_type"] = L"limit";
	limit_meta_data[L"limit_type"] = L"spec";
	return limit_meta_data;
}

wstring DataReader::construct_recipe(wstring report_template, wstring report_name, wstring project_name) {
	return wstring(L"<Recipe><Name>Report Starter</Name>") +
		L"<ApplyTemplate href=&quot;rcodes/apply_report_template_to_data." +
		L"xsl&quot; enabled=&quot;true&quot;>" +
		L"<param><name>generatePDF</name><value>true</value></param>" +
		L"<param><name>reportTemplate</name><value>" + report_template + L"</value></param>" +
		L"<param><name>reportName</name><value>" + report_name + L"</value></param>" +
		L"<param><name>reportID</name><value /></param>" +
		L"<param><name>d</name><value /></param>" +
		L"<param><name>mapping</name><value /></param>" +
		L"<param><name>index</name><value>" + project_name + L"</value></param>"+
		L"<param><name>field</name><value>payload.*</value></param>" +
		L"<param><name>filter</name><value><filters>" +
		L"<filter><field>metaData.dataset_id</field><name>Dataset Id</name><filterType>MultiSelect</filterType><filterOperator /><filterUnit />" +
		L"<filterValues><filterValue>$dataset_id$</filterValue></filterValues></filter>" +
		L"<filter><field>metaData.data_object_type</field><name>Data Object Type</name><filterType>MultiSelect</filterType><filterOperator /><filterUnit />" +
		L"<filterValues><filterValue>value</filterValue></filterValues></filter></filters></value></param>" +
		L"<param><name>grouping</name><value /></param>" +
		L"<param><name>query</name><value>{      &quot;query&quot" +
		L";:{      &quot;bool&quot;: {      &quot;must&quot; :" +
		L"[            ]      }      }      }</value></param></ApplyTemplate></Recipe>";
}

wstring DataReader::scale_value(int scale, wstring value) {
	wistringstream to_double2(value);
	double current_value2;
	to_double2 >> current_value2;
	current_value2 = current_value2 * pow(10, -scale);
	// convert back to wstring
	wostringstream scaled_value;
	scaled_value << current_value2;
	wstring str_val = scaled_value.str();
	str_val = this->strremove(str_val, ',');
	return str_val;
}

wstring DataReader::generate_limit_from_test_value(wstring value, bool is_upper_limit) {
	// get scale of the first test value and scale acc to that
	wistringstream to_double(value);
	double first_test_val;
	to_double >> first_test_val;
	double scaled_val;
	double scale_from_test_value{};
	if (first_test_val < 10) {
		scale_from_test_value = ceil(log10(abs(first_test_val)));
	}
	else {
		scale_from_test_value = floor(log10(abs(first_test_val)));
	}
	// scale according to unit from first test value
	if (is_upper_limit) {
		scaled_val = first_test_val + 0.5 * pow(10, scale_from_test_value);
	}
	else {
		scaled_val = first_test_val - 0.5 * pow(10, scale_from_test_value);
	}

	// convert back to wstring
	wostringstream strs;
	strs << scaled_val;
	return strs.str();
}


wstring DataReader::strrep(wstring line, char from, char to) {
	for (auto i = 0; i < line.size(); i++) {
		if (char(line[i]) == char(from)) {
			line[i] = to;
		}
	}
	return line;
}

wstring DataReader::strremove(wstring line, char rem) {
	wstring new_line{};
	for (auto i = 0; i < line.size(); i++) {
		if (line[i] != rem) {
			new_line += line[i];
		}
	}
	return new_line;
}

wstring DataReader::strtrim(wstring line) {
	wstring new_line{};
	for (auto i = 0; i < line.size(); i++) {
		if ((i == 0 || i == (line.size() - 1)) && line[i] == ' ') {
			continue;
		}
		else {
			new_line += line[i];
		}
	}
	return new_line;
}

tuple<int, wstring> DataReader::get_unit_scale(const wstring& raw_unit) {
	int scale{};
	wstring unit;
	if (raw_unit[0] == 'p') {
		scale = 12;
		unit = this->strremove(raw_unit, 'p');
	}
	else if (raw_unit[0] == 'n') {
		scale = 9;
		unit = this->strremove(raw_unit, 'n');
	}
	else if (raw_unit[0] == 'u') {
		scale = 6;
		unit = this->strremove(raw_unit, 'u');
	}
	else if (raw_unit[0] == 'm') {
		scale = 3;
		unit = this->strremove(raw_unit, 'm');
	}
	else if (raw_unit[0] == 'k') {
		scale = -3;
		unit = this->strremove(raw_unit, 'k');
	}
	/*
	else if (raw_unit[0] == 'K') {
		scale = -3;
		unit = this->strremove(raw_unit, 'K');
	}
	*/
	else if (raw_unit[0] == 'M') {
		scale = -6;
		unit = this->strremove(raw_unit, 'M');
	}
	else if (raw_unit[0] == 'G') {
		scale = -9;
		unit = this->strremove(raw_unit, 'G');
	}
	else if (raw_unit[0] == 'T') {
		scale = -12;
		unit = this->strremove(raw_unit, 'T');
	}
	else if (raw_unit[0] == ']' || raw_unit[0] == '[') {
		// invalid unit scale occured
		wcout << L"INVALID UNIT OCCURED IN LIMITS (testlimits.txt): " << unit << endl;
		system("PAUSE");
		exit(1);
		// mark as invalid raw_unit occured
		return make_tuple(42, raw_unit);
	}
	else {
		scale = 0;
		unit = raw_unit;
	}
	return make_tuple(scale, unit);
}

wstring DataReader::progress_bar(int curr, int total, int step) {
	wstring progress = L"|";
	for (int i = 0; i < total; i++) {
		if (i % step == 0) {
			if (i < curr) {
				progress += L"=";
			}
			else if (i == curr) {
				progress += L">";
			}
			else {
				progress += L".";
			}
		}
	}
	progress += L"| ";
	progress += to_wstring((int)(ceil((double)curr / (double)total * 100)));
	progress += L"%";
	return progress;
}

wstring DataReader::validate_param_name(wstring raw_param_name) {
	wstring param_name = raw_param_name;
	// replace all special characters with _ to avoid Tembo crash
	param_name = this->strrep(param_name, '-', '_');
	param_name = this->strrep(param_name, '(', '_');
	param_name = this->strrep(param_name, ')', '_');
	param_name = this->strrep(param_name, '!', '_');
	param_name = this->strrep(param_name, '#', '_');
	param_name = this->strrep(param_name, ',', '_');
	param_name = this->strrep(param_name, '.', '_');
	// if first char is _, remove it
	if (param_name[0] == '_') {
		param_name.erase(0, 1);
	}
	return param_name;
}

wstring DataReader::convert_to_lower(wstring data) {
	transform(data.begin(), data.end(), data.begin(),
		[](unsigned char c) { return tolower(c); });
	return data;
}

wstring DataReader::get_excel_col_name(int col) {
	// convert col to char name
	wstring res{};

	while (col > 0) {
		// find index and concat to res
		// 0 corresponds to A, 25 to Z
		int index = (col - 1) % 26;
		res = wchar_t(index + 'A') + res;
		col = (col - 1) / 26;
	}

	return res;
}

int DataReader::count_char_occurence(wstring s, char c) {
	int count = 0;
	for (auto l : s) {
		if (c == l) {
			count++;
		}
	}
	return count;
}