#pragma once

#include<string>
#include<vector>
#include<deque>

#include"zmq.hpp"
#include"nlohmann/json.hpp"


struct Candle {
	double open;
	double hight;
	double low;
	double close;
	double volume;
};

class AethelredCore{
private:
	static const size_t MAX_HISTORY_SIZE = 200;
	static const int MIN_DATA_FOR_CALC = 39;

	zmq::context_t context;
	zmq::socket_t socket;
	std::deque<Candle> candle_history;

	void handle_request();
	nlohmann::json process_add_candle(const nlohmann::json& candle_date);
	nlohmann::json calculate_all_indicators();
	void calculate_rsi(const std::vector<double>& closes, nlohmann::json& indicators, int period = 14);
	void calculate_ema(const std::vector<double>& closes, nlohmann::json& indicators, int period = 38);
	void calculate_adx_di(const std::vector<double>& hights, const std::vector<double>& lows,
		const std::vector<double>& closes, nlohmann::json& inicators, int period = 14);
	void culculate_atr(const std::vector<double>& highs, const std::vector<double>& lows, const std::vector<double>& closes, nlohmann::json& indicators, int period = 14);
	void calculate_breakout(const std::vector<double>& closes, nlohmann::json& indicators, int period = 20);
public:
	AethelredCore(const std::string& bind_address);
	void run();
};
