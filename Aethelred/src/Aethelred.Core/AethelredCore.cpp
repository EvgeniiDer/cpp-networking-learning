#include"AethelredCore.hpp"

#include<iostream>
#include"ta_libc.h"



AethelredCore::AethelredCore(const std::string& bind_address) : context(1)
{
	socket = zmq::socket_t(context, zmq::socket_type::rep);
	std::cout << "[INFO] Aethelred.core connecting to " << bind_address << "..." << std::endl;
	socket.bind(bind_address);
}

void AethelredCore::run()
{
	std::cout << "[INFO] Core is running. Waiting for events..." << std::endl;
	while (true)
	{
		handle_request();
	}
}
void AethelredCore::handle_request()
{
	zmq::message_t request;
	socket.recv(request, zmq::recv_flags::none);
	nlohmann::json response;
	try {
		nlohmann::json received_json = nlohmann::json::parse(static_cast<char*>(request.data()), static_cast<char*>(request.data()) + request.size());
		if (received_json.value("event", "") == "ADD_CANDLE")
		{
			response = process_add_candle(received_json["data"]);
		}
		else
		{
			throw std::runtime_error("Unknown event type: " + received_json.value("event", "N/A"));
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "[ERROR] " << e.what() << std::endl;
		response["status"] = "error";
		response["message"] = e.what();
	}
	socket.send(zmq::buffer(response.dump()), zmq::send_flags::none);
}
nlohmann::json AethelredCore::process_add_candle(const nlohmann::json& candle_data)
{
	candle_history.push_back({
		candle_data.value("Open", 0.0),
		candle_data.value("High", 0.0),
		candle_data.value("Low", 0.0),
		candle_data.value("Close", 0.0),
		candle_data.value("Volume", 0.0)
		});
		if (candle_history.size() > MAX_HISTORY_SIZE)
		{
			candle_history.pop_front();
		}
	std::cout << "[DEBUG] Added candle. History size: " << candle_history.size() << std::endl;
	if (candle_history.size() < MIN_DATA_FOR_CALC)
	{
		throw std::runtime_error("Not enough history data.");
	}
	nlohmann::json response;
	response["status"] = "success";
	response["indicators"] = calculate_all_indicators();
	return response;
}

nlohmann::json AethelredCore::calculate_all_indicators()
{
	std::vector<double> highs, lows, closes;
	highs.reserve(candle_history.size());
	lows.reserve(candle_history.size());
	closes.reserve(candle_history.size());
	for (const auto& c : candle_history) {
		highs.push_back(c.hight);
		lows.push_back(c.low);
		closes.push_back(c.close);
	}

	nlohmann::json indicators;

	
	calculate_rsi(closes, indicators);
	calculate_ema(closes, indicators);
	calculate_adx_di(highs, lows, closes, indicators);
	culculate_atr(highs, lows, closes, indicators);
	calculate_breakout(closes, indicators);

	return indicators;
}
void AethelredCore::calculate_rsi(const std::vector<double>& closes, nlohmann::json& indicators, int period /* = 14 */)
{
	std::vector<double>out(closes.size());
	int out_begin, out_nb_element;
	TA_RetCode retCode = TA_RSI(0, closes.size() - 1, closes.data(), period, &out_begin, &out_nb_element, out.data());
	if (retCode == TA_SUCCESS && out_nb_element > 0)
	{
		indicators["rsi"] = out[out_nb_element - 1];
	}
}
void AethelredCore::calculate_ema(const std::vector<double>& closes, nlohmann::json& indicators, int period /* = 38 */)
{
	std::vector<double>out(closes.size());
	int out_begin, out_nb_element;
	TA_RetCode retCode = TA_EMA(0, closes.size() - 1, closes.data(), period, &out_begin, &out_nb_element, out.data());
	if (retCode == TA_SUCCESS && out_nb_element > 0) {
		indicators["ema"] = out[out_nb_element - 1];
	}
}
void AethelredCore::culculate_atr(const std::vector<double>& highs, const std::vector<double>& lows, const std::vector<double>& closes, nlohmann::json& indicators, int period)
{
	std::vector<double> out(closes.size());
	int out_begin, out_nb_element;
	if (TA_ATR(0, closes.size() - 1, highs.data(), lows.data(), closes.data(), period, &out_begin, &out_nb_element, out.data()) == TA_SUCCESS && out_nb_element > 0) {
		indicators["atr"] = out[out_nb_element - 1];
	}
}

void AethelredCore::calculate_breakout(const std::vector<double>& closes, nlohmann::json& indicators, int period /* = 20 */)
{
	if (closes.size() < period - 1)
		return;
	double highest = 0.0;
	double lowest = std::numeric_limits<double>::max();
	for (size_t i = closes.size() - 2; i >= closes.size() - 1 - period; --i)
	{
		if (closes[i] > highest) highest = closes[i];
		if (closes[i] < lowest) lowest = closes[i];
	}
	indicators["highest_break"] = highest;
	indicators["lowest_break"] = lowest;
}
void AethelredCore::calculate_adx_di(const std::vector<double>& highs, const std::vector<double>& lows, const std::vector<double>& closes, nlohmann::json& indicators, int period)
{
	std::vector<double>out(closes.size());
	int out_begin, out_nb_element;
	if (TA_ADX(0, closes.size() - 1, highs.data(), lows.data(), closes.data(), period, &out_begin, &out_nb_element, out.data()) == TA_SUCCESS && out_nb_element > 0) {
		indicators["adx"] = out[out_nb_element - 1];
	}
	if (TA_PLUS_DI(0, closes.size() - 1, highs.data(), lows.data(), closes.data(), period, &out_begin, &out_nb_element, out.data()) == TA_SUCCESS && out_nb_element > 0) {
		indicators["plus_di"] = out[out_nb_element - 1];
	}
	if (TA_MINUS_DI(0, closes.size() - 1, highs.data(), lows.data(), closes.data(), period, &out_begin, &out_nb_element, out.data()) == TA_SUCCESS && out_nb_element > 0) {
		indicators["minus_di"] = out[out_nb_element - 1];
	}
}


