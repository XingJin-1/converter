#include <vector>
#include <iostream>
#include <algorithm>
#include <string>
#include <experimental\filesystem>
#include <windows.h>
#include "DataReader.h"
#include "CSVReader.h"
#include "EFFReader.h"
#include <clocale>

/*************************************************************************************************************************************************************************
* maintainer Xing Jin (IFAG ATV PS PD MUC CVSV)
* since v4.0.0
*
* date		27.01.2021
*
* author	Ali Ganbarov (IFAG ATV PSN D PD PRES)
*************************************************************************************************************************************************************************/

namespace filesys = std::experimental::filesystem;
using namespace std;

vector<wstring> getAllFilesInDir(const wstring &dirPath, const wstring &fileExt)
{
	// Create a vector of wstring
	vector<wstring> listOfFiles;

	try {
		// Check if given path exists and points to a directory
		if (filesys::exists(dirPath) && filesys::is_directory(dirPath))
		{
			// Create a Recursive Directory Iterator object and points to the starting of directory
			filesys::recursive_directory_iterator iter(dirPath);

			// Create a Recursive Directory Iterator object pointing to end.
			filesys::recursive_directory_iterator end;

			// Iterate till end
			while (iter != end) {
				// Check if current entry is a directory and if exists in skip list
				if (filesys::is_directory(iter->path())) {
					// Skip the iteration of current directory pointed by iterator
				}
				else {
					// Add the name in vector
					try {
						if (iter->path().wstring().find(fileExt) != wstring::npos) {
							listOfFiles.push_back(iter->path().wstring());
						}
					}
					catch (invalid_argument &e) {
						error_code ec;
						iter.increment(ec);
					}
				}

				error_code ec;
				// Increment the iterator to point to next entry in recursive iteration
				iter.increment(ec);
				if (ec) {
					cerr << "Error While Accessing : " << iter->path().string() << " :: " << ec.message() << '\n';
				}
			}

		}
	}
	catch (system_error & e) {
		cerr << "Exception :: " << e.what();
	}
	return listOfFiles;
}

int main(int argc, char *argv[]) {
	typedef std::chrono::high_resolution_clock clock;
	typedef std::chrono::duration<float, std::milli> mil;
	auto t3 = clock::now();

	std::setlocale(LC_ALL, "en_US.utf8");
	//std::locale::global(std::locale("en_US.utf8"));

	vector<wstring> csv_files;
	vector<wstring> png_files;
	vector<wstring> mat_files;
	vector<wstring> eff_files;
	vector<wstring> test_limits_file;
	map<wstring, map<wstring, wstring>> limits_struct;
	vector<wstring> configs_file;
	map<wstring, wstring> raw_configs_struct;
	map<wstring, wstring> configs_struct;
	wstring test_flow_folder{};
	string path{};
	wstring wpath{};
	//used for right click on a single folder
	wstring searchpath{};
	bool use_sys_pause = true;
	bool is_manual_measurement_data = false;
	DataReader dr;
	for (int i = 1; i < argc; i++) {
		path = argv[i];
		wstring searchPathTmp(path.begin(), path.end());
		searchpath = searchPathTmp;
		wcout << "SearchPath: " << searchpath << endl;
		// check whether upload whole 30_RawData or just a single folder within it 
		int sign = -1;
		sign = path.find("30_RawData\\");
		// right click on a single file 
		if (sign != -1) {
			path = path.replace(path.find_last_of("\\"), path.size() - 1, "");
		}
		cout << "Path: " << path << endl;
		wstring wsTmp(path.begin(), path.end());
		wpath = wsTmp;
		// check if input contains number. If it does remove system pause (another program is calling)
		double doub;
		wistringstream iss(wpath);
		iss >> dec >> doub;
		if (!iss.fail()) {
			use_sys_pause = false;
			continue;
		}
		// get test_flow folder
		test_flow_folder = wpath;
		test_flow_folder = test_flow_folder.replace(test_flow_folder.find_last_of(L"\\") + 1, test_flow_folder.size() - 1, L"20_TestFlow");

		// read configs
		configs_file = getAllFilesInDir(test_flow_folder, L"Config_Tembo.txt");
		if (configs_file.size() > 0) {
			raw_configs_struct = dr.read_config_file(configs_file[0]);
		}
		else {
			cout << "Couldn't read Config_Tembo.txt file" << endl;
		}
		// if basic_type is in raw_configs_struct then it is excel data (manual measurement)
		for (map<wstring, wstring>::value_type& config : raw_configs_struct) {
			if (L"basic_type" == config.first) {
				is_manual_measurement_data = true;
			}
		}

		time_t theTime = time(NULL);
		struct tm *aTime = localtime(&theTime);
		int day = aTime->tm_mday;
		int month = aTime->tm_mon + 1;
		int year = aTime->tm_year + 1900;
		int hour = aTime->tm_hour;
		int min = aTime->tm_min;
		int sec = aTime->tm_sec;
		string out_folder_name = "50_Report\\" + to_string(year) + to_string(month) + to_string(day) + "T" + to_string(hour) + to_string(min) + to_string(sec);

		// get the output folder path
		string out_folder_path = path.replace(path.find_last_of("\\") + 1, path.size() - 1, out_folder_name);
		cout << "Out folder: " << out_folder_path << endl;
		wstring wsTmp2(out_folder_path.begin(), out_folder_path.end());
		wstring w_out_folder_path = wsTmp2;

		if (CreateDirectory(out_folder_path.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError()) {
			// read EFF files
			eff_files = getAllFilesInDir(searchpath, L".eff");
			if (eff_files.size() > 0) {
				// Input folder contains eff files
				EFFReader er;
				// setup configurations
				configs_struct = dr.setup_configurations(raw_configs_struct, false);
				bool res;
				// write each file separately
				for (auto eff_file : eff_files) {
					wcout << L"Reading EFF file: " << eff_file << endl;
					res = er.eff_to_json(eff_file, configs_struct, w_out_folder_path);
					if (res) {
						wstring prj_name = configs_struct[L"Project"];
						transform(prj_name.begin(), prj_name.end(), prj_name.begin(), ::toupper);
						wstring staging_area = wstring(L"\\\\VIHSDV002.infineon.com\\tembo_staging_prod\\") + prj_name + L"\\job";
						wcout << L"Staging area location" << endl << staging_area << endl << endl;

						// get report name from EFF file name
						wstring base_filename = wstring(eff_file).substr(wstring(eff_file).find_last_of(L"/\\") + 1);
						wstring::size_type const p(base_filename.find_last_of('.'));
						wstring file_without_extension = base_filename.substr(0, p);
						wstring report_name = file_without_extension;
						
						// move file to Tembo
						try {
							filesys::copy(w_out_folder_path + L"\\" + report_name + L".json", staging_area, filesys::copy_options::overwrite_existing);
						}
						catch (filesys::filesystem_error &e) {
							cout << "Couldn't copy file to staging area: " << e.what() << endl;
						}
						
					}
				}
			}
			// read csv files
			csv_files = getAllFilesInDir(searchpath, L".csv");
			// read all png files
			png_files = getAllFilesInDir(searchpath, L".png");
			// read all mat files
			mat_files = getAllFilesInDir(searchpath, L".mat");
			bool res;
			if (csv_files.size() > 0) {
				// Input folder contains csv files
				CSVReader cr;
				// use raw_configs_struct for manual measurement data, since it deals with default values
				// for normal CSV file use configs_struct
				if (!is_manual_measurement_data) {
					configs_struct = dr.setup_configurations(raw_configs_struct, true);
				}
				else {
					configs_struct = raw_configs_struct;
				}
				
				// read limits
				test_limits_file = getAllFilesInDir(test_flow_folder, L"testlimits.txt");
				if (test_limits_file.size() > 0) {
					limits_struct = cr.read_limits_file(test_limits_file[0]);
				}
				else {
					cout << "Couldn't read testlimits.txt file" << endl;
				}
				res = cr.csvs_to_json(csv_files, limits_struct, configs_struct, w_out_folder_path, png_files, mat_files);
				if (res) {
					wstring prj_name = configs_struct[L"Project"];
					transform(prj_name.begin(), prj_name.end(), prj_name.begin(), ::toupper);
					wstring staging_area = wstring(L"\\\\VIHSDV002.infineon.com\\tembo_staging_prod\\") + prj_name + L"\\job";
					wcout << L"Staging area location" << endl << staging_area << endl << endl;

					wstring report_name = configs_struct[L"ReportName"];
					// move file to Tembo
					try {
						// move png files
						for (auto png_file : png_files) {
							if (dr.convert_to_lower(png_file).find(L"report-picture") != wstring::npos) {
								filesys::copy(png_file, staging_area, filesys::copy_options::overwrite_existing);
							}
						}
						// move mat files
						for (auto mat_file : mat_files) {
							if (dr.convert_to_lower(mat_file).find(L"report-waveform") != wstring::npos) {
								filesys::copy(mat_file, staging_area, filesys::copy_options::overwrite_existing);
							}
						}
						cout << "png and mat done" << endl;
						filesys::copy(w_out_folder_path + L"\\" + report_name + L".json", staging_area, filesys::copy_options::overwrite_existing);
					}
					catch (filesys::filesystem_error &e) {
						cout << "Couldn't copy file to staging area: " << e.what() << endl;
						wcout << staging_area << endl;
						wcout << w_out_folder_path + L"\\" + report_name + L".json" << endl;
					}
					
				}
			}
			else {
				wcout << L"Couldn't locate csv files" << endl;
			}

		}
		else {
			wcout << L"Failed to create directory!" << endl;
		}
	}

	auto t4 = clock::now();
	cout << "Total time: " << mil(t4 - t3).count() << " ms" << endl;

	if (use_sys_pause) {
		system("pause");
	}

	return 0;
}