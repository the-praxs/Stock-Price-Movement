#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <locale>
#include <iomanip>
#include <fstream>
#include <map>
#include "curl/curl.h"
#include "Stock.hpp"

using namespace std;

struct MemoryStruct {
	char* memory;
	size_t size;
	size_t total_size;
};

void* myrealloc(void* ptr, size_t size) {
	if (ptr)
		return realloc(ptr, size);
	else
		return malloc(size);
}

int write_data2(void* ptr, size_t size, size_t nmemb, void* data) {
	size_t realsize = size * nmemb;
	struct MemoryStruct* mem = (struct MemoryStruct*)data;
	if ((mem->size + realsize) >= mem->total_size)
		mem->memory = (char*)myrealloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
		if (mem->size > mem->total_size)
			mem->total_size = mem->size;
	}
	return realsize;
}

int ExtractIWVData(map<string, double>& iwv_date_map) {
	string start_date = "2021-04-01";
	string end_date = "2022-03-31";

	// declaration of an object CURL
	CURL* handle;

	// result of the whole process
	CURLcode result;

	// set up the program environment that libcurl needs
	curl_global_init(CURL_GLOBAL_ALL);

	// curl_easy_init() returns a CURL easy handle
	handle = curl_easy_init();

	// if everything's all right with the easy handle...
	if (handle)
	{
		string url_common = "https://eodhistoricaldata.com/api/eod/";
		string api_token = "62686d23ea5b52.97340064";  // You must replace this API token with yours

		struct MemoryStruct data;
		data.memory = NULL;
		data.size = 0;
		data.total_size = 0;

		string url_request = url_common + "IWV" + ".US?" + "from=" + start_date + "&to=" + end_date + "&api_token=" + api_token + "&period=d";
		curl_easy_setopt(handle, CURLOPT_URL, url_request.c_str());

		//adding a user agent
		curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:74.0) Gecko/20100101 Firefox/74.0");
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);

		curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data2); 
		curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&data);

		// perform, then store the expected code in result
		result = curl_easy_perform(handle);

		if (result != CURLE_OK)
		{
			// if errors have occured, tell us what is wrong with result
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
			return 1;
		}

		stringstream sData;
		sData.str(data.memory);
		string sValue, sDate;
		double dValue = 0;
		string line;
		
		while (getline(sData, line)) {
			size_t found = line.find('-');

			if (found != std::string::npos) {
				sDate = line.substr(0, line.find_first_of(','));
				line.erase(line.find_last_of(','));
				sValue = line.substr(line.find_last_of(',') + 1);
				dValue = strtod(sValue.c_str(), NULL);
				iwv_date_map[sDate] = dValue;
			}
		}

		free(data.memory);
		data.size = 0;
	}
	else {
		fprintf(stderr, "Curl init failed!\n");
		return -1;
	}

	// cleanup since you've used curl_easy_init
	curl_easy_cleanup(handle);

	// release resources acquired by curl_global_init()
	curl_global_cleanup();

	cout << "IWV data extraction complete." << endl << endl;

	return 0;
}

int ExtractStockData(map<string, string> symbol_date_map, map<string, map<string, double>>& date_price_map) {
	string start_date = "2021-04-01";
	string end_date = "2021-06-30";

	int count = 0;
	int length = symbol_date_map.size();
	symbol_date_map["IWB"] = start_date;

	// declaration of an object CURL
	CURL* handle;

	// result of the whole process
	CURLcode result;

	// set up the program environment that libcurl needs
	curl_global_init(CURL_GLOBAL_ALL);

	// curl_easy_init() returns a CURL easy handle
	handle = curl_easy_init();

	// if everything's all right with the easy handle...
	if (handle)
	{
		string url_common = "https://eodhistoricaldata.com/api/eod/";
		string api_token = "62686d23ea5b52.97340064";  // You must replace this API token with yours

		struct MemoryStruct data;
		data.memory = NULL;
		data.size = 0;
		data.total_size = 0;

		map<string, string>::iterator itr = symbol_date_map.begin();
		for (; itr != symbol_date_map.end(); itr++) {
			data.size = 0;
			memset(data.memory, '\0', data.total_size);

			string symbol = itr->first;
			string date = itr->second;
			string url_request = url_common + symbol + ".US?" + "from=" + start_date + "&to=" + end_date + "&api_token=" + api_token + "&period=d";
			curl_easy_setopt(handle, CURLOPT_URL, url_request.c_str());

			//adding a user agent
			curl_easy_setopt(handle, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:74.0) Gecko/20100101 Firefox/74.0");
			curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
			curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0);

			curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data2);
			curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&data);

			// perform, then store the expected code in result
			result = curl_easy_perform(handle);

			if (result != CURLE_OK) {
				// if errors have occured, tell us what is wrong with result
				fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
				return 1;
			}

			stringstream sData;
			sData.str(data.memory);
			string sValue, sDate;
			double dValue = 0;
			string line;

			while (getline(sData, line)) {
				size_t found = line.find('-');

				if (found != std::string::npos) {
					sDate = line.substr(0, line.find_first_of(','));
					line.erase(line.find_last_of(','));
					sValue = line.substr(line.find_last_of(',') + 1);
					dValue = strtod(sValue.c_str(), NULL);
					date_price_map[symbol][sDate] = dValue;
				}
			}

			count += 1;

			if (count % (length / 10) == 0)
				cout << "- - - - - Downloading " << (count * 1.0 / length) * 100 << "% - - - - -" << endl;
		}

		free(data.memory);
		data.size = 0;

	}
	else {
		fprintf(stderr, "Curl init failed!\n");
		return -1;
	}

	// cleanup since you've used curl_easy_init
	curl_easy_cleanup(handle);

	// release resources acquired by curl_global_init()
	curl_global_cleanup();

	cout << "Stock data extraction complete." << endl << endl;

	return 0;
}