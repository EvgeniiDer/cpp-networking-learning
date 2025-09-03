#include"AethelredCore.hpp"

#include<iostream>
#include"ta_libc.h"



AethelredCore::AethelredCore(const std::string& sub_connect_address, const std::string& pub_bind_address) : context(1)
{
	subscriber = zmq::socket_t(context, zmq::socket_type::sub);
	publisher = zmq::socket_t(context, zmq::socket_type::pub);
	std::cout << "[INFO] Aethelred.core connecting to Gateway at " << sub_connect_address << "..." << std::endl;
	subscriber.connect(sub_connect_address);
	const std::string topic = "CANDLE_DATA";
	subscriber.set(zmq::sockopt::subscribe, topic);
	std::cout << "[INFO] Subscribed to topic: '" << topic << "'" << std::endl;

	
	std::cout << "[INFO] Aethelred.core binding publisher to " << pub_bind_address << "..." << std::endl;
	publisher.bind(pub_bind_address);
}

void AethelredCore::run()
{
	std::cout << "[INFO] Core is running. Waiting for candle data from Gateway..." << std::endl;
	while (true)
	{
		try {
			zmq::message_t topic_msg;
			subscriber.recv(topic_msg, zmq::recv_flags::none);

			zmq::message_t data_msg;
			subscriber.recv(data_msg, zmq::recv_flags::none);

			nlohmann::json received_json = nlohmann::json::parse(static_cast<char*>(data_msg.data()), static_cast<char*>(data_msg.data()) + data_msg.size());
			if (received_json.value("event", "") == "ADD_CANDLE")
			{
				std::optional<nlohmann::json> indicators_opt = process_new_candle(received_json["data"]);
				if (indicators_opt)
				{
					/*
					* indicators_opt is not a JSON object itself. It is a "box" (std::optional) that can contain a JSON object. To access the contents of this box, you must use special "unpacking" operators: * or ->.
					*/
					nlohmann::json& indicators = *indicators_opt;
					indicators["close"] = candle_history.back().close;

					std::string indicators_str = indicators.dump();

					const std::string indicators_topic = "INDICATORS";
					publisher.send(zmq::buffer(indicators_topic), zmq::send_flags::sndmore);
					publisher.send(zmq::buffer(indicators_str), zmq::send_flags::none);

					std::cout << "[PUB] Published indicators. RSI: " << indicators.value("rsi", 0.0) << std::endl;
				}
				/*else{
					continue; Не рекомендовано плюс к ясности минус к избыточности кода
				}*/
			}
		}
		catch (const zmq::error_t& e) {
			std::cerr << "[ZMQ ERROR] " << e.what() << std::endl;
			
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		catch (const std::exception& e)
		{
			std::cerr << "[ERROR] " << e.what() << std::endl;
		}
	}
	
}

std::optional<nlohmann::json> AethelredCore::process_new_candle(const nlohmann::json& candle_data)
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
		std::cout << "[INFO] Not enough history data to calculate indicators. Need " << MIN_DATA_FOR_CALC << ", have " << candle_history.size() << "." << std::endl;
		return std::nullopt; 
	}

	return calculate_all_indicators();
}

nlohmann::json AethelredCore::calculate_all_indicators()
{
	std::vector<double> highs, lows, closes;
	highs.reserve(candle_history.size());
	lows.reserve(candle_history.size());
	closes.reserve(candle_history.size());
	for (const auto& c : candle_history) {
		highs.push_back(c.high);
		lows.push_back(c.low);
		closes.push_back(c.close);
	}

	nlohmann::json indicators;

	
	calculate_rsi(closes, indicators);
	calculate_ema(closes, indicators);
	calculate_adx_di(highs, lows, closes, indicators);
	calculate_atr(highs, lows, closes, indicators);
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
void AethelredCore::calculate_atr(const std::vector<double>& highs, const std::vector<double>& lows, const std::vector<double>& closes, nlohmann::json& indicators, int period)
{
	std::vector<double> out(closes.size());
	int out_begin, out_nb_element;
	if (TA_ATR(0, closes.size() - 1, highs.data(), lows.data(), closes.data(), period, &out_begin, &out_nb_element, out.data()) == TA_SUCCESS && out_nb_element > 0) {
		indicators["atr"] = out[out_nb_element - 1];
	}
}

void AethelredCore::calculate_breakout(const std::vector<double>& closes, nlohmann::json& indicators, int period /* = 20 */)
{
	if (closes.size() < period)
		return;
	double highest = 0.0;
	double lowest = std::numeric_limits<double>::max();
	for (size_t i = closes.size() - period - 1; i < closes.size() - 1; ++i)
	{
		if (closes[i] > highest) highest = closes[i];
		if (closes[i] < lowest) lowest = closes[i];
	}
	indicators["highest_break"] = highest;
	indicators["lowest_break"] = lowest;
}
void AethelredCore::calculate_adx_di(const std::vector<double>& highs, const std::vector<double>& lows,
	const std::vector<double>& closes, nlohmann::json& indicators, int period)
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


