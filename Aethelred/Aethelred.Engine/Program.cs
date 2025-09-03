using System;
using NetMQ;
using NetMQ.Sockets;
using Newtonsoft.Json;


namespace Aethelred.StrategyEngine
    {
    public class IndicatorData
        {
        public double close { get; set; }
        public double rsi { get; set; }
        public double ema { get; set; }
        public double adx { get; set; }
        public double atr { get; set; }
        public double plus_di { get; set; }
        public double minus_di { get; set; }
        public double highest_break { get; set; }
        public double lowest_break { get; set; }
        }
    class Program
        {
        private const string CoreAddress = "tcp://localhost:5556";
        private const string Topic = "INDICATORS";

        private static bool _isPositionOpen = false;
        private static string _entryType = "None";


        static void Main(string[] args)
            {
            Console.WriteLine("[INFO] Aethelred.StrategyEngine starting...");
            try
                {
                using(SubscriberSocket subscriber = new SubscriberSocket())
                    {
                    subscriber.Connect(CoreAddress);
                    Console.WriteLine($"[INFO] Connected to Aethelred.Core at {CoreAddress}");

                    subscriber.Subscribe(Topic);
                    Console.WriteLine($"[INFO] Subscribed to topic: '{Topic}'");

                    Console.WriteLine("[INFO] Waiting for indicator data from Core...");

                    
                    while (true)
                        {
                        
                        subscriber.ReceiveFrameString();         
                        string receivedJson = subscriber.ReceiveFrameString();
                        IndicatorData indicators = JsonConvert.DeserializeObject<IndicatorData>(receivedJson);
                        ProcessStrategy(indicators);
                        }
                    }
                }
                    catch (Exception ex)
                {
                Console.WriteLine($"[FATAL] An error occurred: {ex.Message}");
                Console.WriteLine("Press any key to exit.");
                Console.ReadKey();
                }
            }
        private static void ProcessStrategy(IndicatorData indicators)
            {
            const double adxThreshold = 20.0;
            bool isTrending = indicators.adx > adxThreshold;
            bool isRanging = !isTrending;

            bool isBullish = indicators.close > indicators.ema;
            bool isBearish = indicators.close < indicators.ema;

            if(_isPositionOpen)
                {
                if(_entryType == "RSI")
                    {
                    const double rsiExitThreshold = 50.0;
                    bool rsiLongExit = indicators.rsi > rsiExitThreshold;
                    if(rsiLongExit)
                        {
                        _isPositionOpen = false;
                        _entryType = "None";
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] [ACTION] --- ЗАКРЫТЬ ДЛИННУЮ ПОЗИЦИЮ (RSI EXIT) ---");
                        Console.ResetColor();
                        }
                    }
                }
            if(!_isPositionOpen)
                {
                const double rsiBuyThreshold = 40.0;
                bool rsiLongCondition = isRanging && indicators.rsi < rsiBuyThreshold && isBullish;
                if (rsiLongCondition)
                    {
                    _isPositionOpen = true;
                    _entryType = "RSI";
                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] [A" +
                        $"CTION] +++ ОТКРЫТЬ ДЛИННУЮ ПОЗИЦИЮ (RSI) +++ | ADX: {indicators.adx:F2}, RSI: {indicators.rsi:F2}");
                    Console.ResetColor();
                    return; // Выходим из метода, чтобы не проверить другие условия входа на этой же свече
                    }
                }
            bool longBreakCondition = isTrending && isBullish && indicators.close > indicators.highest_break;
            if (longBreakCondition)
                {
                _isPositionOpen = true;
                _entryType = "Breakout";
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] [ACTION] +++ ОТКРЫТЬ ДЛИННУЮ ПОЗИЦИЮ (BREAKOUT) +++ | ADX: {indicators.adx:F2}, Close: {indicators.close}, Break: {indicators.highest_break}");
                Console.ResetColor();
                }
            }
        }

    }