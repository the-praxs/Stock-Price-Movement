#include <stdio.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <locale>
#include <iomanip>
#include <fstream>
#include <map>
#include <algorithm>
#include <cmath>
#include "Stock.hpp"
#include "Bootstrap.hpp"
#include "Vector.hpp"

using namespace std;
const char* cStockTickersFile = "Russell_1000_component_stocks.csv";
const char* cIWVFile = "Russell_3000_Earnings_Announcements.csv";

void Bootstrap::populateTickerVector(vector<string>& tickers) {
    ifstream fstream;
    fstream.open(cStockTickersFile, ios::in);

    string line, ticker, name;
    int count = 0;

    while (!fstream.eof()) {
        getline(fstream, line);
        stringstream sin(line);

        getline(sin, ticker, ',');
        getline(sin, name);
        tickers.push_back(ticker);

        count++;
    }

    cout << "Number of stocks successfully populated: " << count << endl << endl;
}

void Bootstrap::populateIWVVector(map<string, Stock>& stock_map) {
    ifstream fstream;
    fstream.open(cIWVFile, ios::in);

    string line, ticker, announce_date, end_date, estimated_eps, reported_eps, surprise, surprise_pct;

    while (!fstream.eof()) {
        getline(fstream, line);
        stringstream sin(line);

        getline(sin, ticker, ',');
        getline(sin, announce_date, ',');
        getline(sin, end_date, ',');
        getline(sin, estimated_eps, ',');
        getline(sin, reported_eps, ',');
        getline(sin, surprise, ',');
        getline(sin, surprise_pct);

        stock_map[ticker].SetTicker(ticker);
        stock_map[ticker].SetAnnounceDate(announce_date);
        stock_map[ticker].SetEndDate(end_date);
        stock_map[ticker].SetEstimatedEPS(stod(estimated_eps));
        stock_map[ticker].SetReportedEPS(stod(reported_eps));
        stock_map[ticker].SetSurprise(stod(surprise));
        stock_map[ticker].SetSurprisePercent(stod(surprise_pct));
    }

    cout << "IWV data populated successfully." << endl << endl;
}

void Bootstrap::SplitToGroups(vector<string>& title, vector<string>& beat, vector<string>& miss, vector<string>& meet) {
    int msize = title.size() / 3;

    copy(title.begin(), title.begin() + msize, back_inserter(miss));
    copy(title.begin() + msize, title.begin() + 2 * msize, back_inserter(meet));
    copy(title.begin() + 2 * msize, title.begin() + title.size(), back_inserter(beat));

    cout << "Stocks have been seperated to different groups." << endl << endl;
}

vector<Vector> Bootstrap::GetAR(int n, vector<vector<string>> vec, map<string, Vector> ar_table) {
    map<string, Vector>::iterator itr;
    vector<vector<double>> AR;
    int number_of_samples = Bootstrap::GetSamples();

    for (int i = 0; i < number_of_samples; i++) {
        itr = ar_table.find(vec[n][i]);

        if (itr != ar_table.end())
            AR.push_back(itr->second);
        else
            cout << vec[n][i] << "could not be found." << endl << endl;
    }

    return AR;
}

Vector Bootstrap::GetAAR(vector<Vector> vec) {
    vector<double> AAR;
    int number_of_dates = Bootstrap::GetDates();
    int number_of_samples = Bootstrap::GetSamples();

    for (int i = 0; i < number_of_dates; i++) {
        double sum = 0;

        for (int n = 0; n < number_of_samples; n++)
            sum += vec[n][i];

        double avg = sum / number_of_samples;
        AAR.push_back(avg);
    }

    return AAR;
}

Vector Bootstrap::GetCAAR(Vector vec) {
    Vector CAAR;
    double caar = 0.0;
    int number_of_dates = Bootstrap::GetDates();

    for (int i = 0; i < number_of_dates; i++) {
        caar += vec[i];
        CAAR.push_back(caar);
    }

    return CAAR;
}

Vector Bootstrap::CalculateAvg(vector<Vector> vec) {
    Vector AVG;
    double res = 0.0;
    int resample_times = Bootstrap::GetResamples();
    int number_of_dates = Bootstrap::GetDates();

    for (int i = 0; i < number_of_dates; i++) {
        for (int j = 0; j < resample_times; j++) {
            res += vec[j][i];
        }
        res = res / resample_times;
        AVG.push_back(res);
    }

    return AVG;
}

Vector Bootstrap::CalculateStd(vector<Vector> vec, Vector avg) {
    Vector STD;
    double res = 0.0;
    double mean = 0.0;
    int resample_times = Bootstrap::GetResamples();
    int number_of_dates = Bootstrap::GetDates();

    for (int i = 0; i < number_of_dates; i++) {
        mean = avg[i];

        for (int j = 0; j < resample_times; j++) {
            res += pow((vec[j][i] - mean), 2);
        }

        res = sqrt(res / resample_times);
        STD.push_back(res);
    }

    return STD;
}

vector<Vector> Bootstrap::CalculateAll(vector<vector<string>> vec, map<string, Vector> ar_table) {
    vector<Vector> result;
    vector<Vector> AAR_pop;
    vector<Vector>CAAR_pop;
    Vector AVG_AAR;
    Vector AVG_CAAR;
    Vector STD_AAR;
    Vector STD_CAAR;

    int resample_times = Bootstrap::GetResamples();

    for (int n = 0; n < resample_times; n++) {
        vector<Vector> AR = Bootstrap::GetAR(n, vec, ar_table);
        Vector AAR = Bootstrap::GetAAR(AR);
        Vector CAAR = Bootstrap::GetCAAR(AAR);

        AAR_pop.push_back(AAR);
        CAAR_pop.push_back(CAAR);
    }

    AVG_AAR = Bootstrap::CalculateAvg(AAR_pop);
    AVG_CAAR = Bootstrap::CalculateAvg(CAAR_pop);
    STD_AAR = Bootstrap::CalculateStd(AAR_pop, AVG_AAR);
    STD_CAAR = Bootstrap::CalculateStd(CAAR_pop, AVG_CAAR);

    result.push_back(AVG_AAR);
    result.push_back(AVG_CAAR);
    result.push_back(STD_AAR);
    result.push_back(STD_CAAR);

    return result;
}

vector<string> Bootstrap::Resample(vector<string> vec) {
    int len = vec.size();
    int index = 0;
    int count = 0;
    int number_of_samples = Bootstrap::GetSamples();
    vector<string> result;

    while (count < number_of_samples) {

        count += 1;
        index = rand() % len;

        result.push_back(vec[index]);
    }

    return result;
}

void Bootstrap::ResampleVector(vector<vector<string>>& vec, vector<string>& vec2) {
    int resample_times = Bootstrap::GetResamples();

    for (int i = 0; i < resample_times; i++) {
        vector<string> sample = Resample(vec2);
        vec.push_back(sample);
    }

    cout << "Resampling group successfull." << endl << endl;
}

void Bootstrap::GetHistoricalPrices(map<string, string> ticker_date_map, map<string, Vector>& price_map, map<string, Vector>& benchmark_map, map<string, map<string, double>>& date_price_map, map<string, double>& iwv_date_map) {
    //map<string, string>::iterator ticker_itr = ticker_date_map.begin();

    for (auto ticker_itr = ticker_date_map.begin(); ticker_itr != ticker_date_map.end(); ticker_itr++) {
        string ticker = ticker_itr->first;
        string date = ticker_itr->second;

        Vector adj_close;
        Vector benchmark;

        //map<string, double>::iterator itr = date_price_map[ticker].find(date);
        auto itr = date_price_map[ticker].find(date);
        // if the date zero is not found, return empty
        if (itr == date_price_map[ticker].end()) {
            cout << ticker + " day zero is not found!" << endl;
            continue;
        }

        for (int i = 0; i < N_; i++) {
            //if the ticker doesn't have +N days, return end
            if (itr == date_price_map[ticker].end())
                break;
            itr++;
        }

        // map<string, double>::iterator end_itr = itr;
        auto end_itr = itr;
        if (itr != date_price_map[ticker].end())
            end_itr++;

        //reset itr
        itr = date_price_map[ticker].find(date);
        for (int i = 0; i < N_; i++) {
            //if the ticker doesn't have -N days, return begin
            if (itr == date_price_map[ticker].begin())
                break;
            itr--;
        }

        while (itr != end_itr) {
            adj_close.push_back(itr->second);
            benchmark.push_back(iwv_date_map[itr->first]);
            itr++;
        }

        price_map[ticker] = adj_close;
        benchmark_map[ticker] = benchmark;
    }
}

Vector Bootstrap::CalculateReturn(Vector V) {
    int size = (int)V.size();
    Vector result;

    for (int i = 0; i < size - 1; i++)
        result.push_back((V[i + 1] - V[i]) / V[i]);

    return result;
}

Vector Bootstrap::CalculateCumReturn(Vector V) {
    int size = (int)V.size();
    Vector result;
    double cumsum = 0.0;

    for (int i = 0; i < size; i++) {
        cumsum += V[i];
        result.push_back(cumsum);
    }

    return result;
}

int Bootstrap::GetN() const {
    return number_of_samples_;
}

int Bootstrap::GetDates() const {
    return number_of_dates_;
}

int Bootstrap::GetSamples() const {
    return number_of_samples_;
}

int Bootstrap::GetResamples() const {
    return resample_times_;
}

void Bootstrap::SetN(int N) {
    N_ = N;
}

void Bootstrap::SetDates(int number_of_dates) {
    number_of_dates_ = number_of_dates;
}

void Bootstrap::SetSamples(int number_of_samples) {
    number_of_samples_ = number_of_samples;
}

void Bootstrap::SetResamples(int resample_times) {
    resample_times_ = resample_times;
}