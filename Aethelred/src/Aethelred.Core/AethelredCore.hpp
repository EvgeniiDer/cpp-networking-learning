#pragma once

#include<string>
#include<vector>
#include<deque>
#include<optional>
#include"zmq.hpp"
#include"nlohmann/json.hpp"
#include<thread>
#include<chrono>

struct Candle {
	double open;
	double high;
	double low;
	double close;
	double volume;
};

class AethelredCore{
private:
	static const size_t MAX_HISTORY_SIZE = 200;
	static const int MIN_DATA_FOR_CALC = 39;

	zmq::context_t context;
	zmq::socket_t subscriber;
	zmq::socket_t publisher;
	std::deque<Candle> candle_history;
	
	;
	std::optional<nlohmann::json> process_new_candle(const nlohmann::json& candle_date);
	nlohmann::json calculate_all_indicators();
	void calculate_rsi(const std::vector<double>& closes, nlohmann::json& indicators, int period = 14);
	void calculate_ema(const std::vector<double>& closes, nlohmann::json& indicators, int period = 38);
	void calculate_adx_di(const std::vector<double>& highs, const std::vector<double>& lows,
		const std::vector<double>& closes, nlohmann::json& indicators, int period = 14);
	void calculate_atr(const std::vector<double>& highs, const std::vector<double>& lows, const std::vector<double>& closes, nlohmann::json& indicators, int period = 14);
	void calculate_breakout(const std::vector<double>& closes, nlohmann::json& indicators, int period = 20);
public:
	AethelredCore(const std::string& sub_connect_address, const std::string& pub_bind_address);
	void run();
};
