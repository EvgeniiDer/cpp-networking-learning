// Aethelred.Strategy/Program.cs
using NetMQ;
using NetMQ.Sockets;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Threading;

namespace Aethelred.Strategy
    {
    // Класс для отчетов об исполнении
    public class ExecutionReport
        {
        public string OriginalSignal { get; set; }
        public string Symbol { get; set; }
        public bool Success { get; set; }
        public string OrderId { get; set; }
        public string ErrorMessage { get; set; }
        }

    // --- ИЗМЕНЕНО ---
    // Класс для хранения состояния нашей стратегии
    public static class StrategyState
        {
        public enum PositionStatus { None, PendingEntry, InPosition, PendingExit }

        public static PositionStatus CurrentStatus { get; set; } = PositionStatus.None;
        public static string LastTradeType { get; set; } = "None";

        // --- ДОБАВЛЕНО ---
        // Храним цену нашего плавающего трейлинг-стопа. 0 = стоп не установлен.
        public static double TrailingStopPrice { get; set; } = 0;
        }

    internal class Program
        {
        private const string CoreConnectAddress = "tcp://localhost:5556";
        private const string CoreTopic = "INDICATORS";

        private const string SignalsBindAddress = "tcp://*:5557";
        private const string SignalsTopic = "TRADE_SIGNALS";

        private const string ReportsConnectAddress = "tcp://localhost:5558";
        private const string ReportsTopic = "EXECUTION_REPORTS";

        // --- Параметры стратегии из Pine Script ---
        private const double AdxThreshold = 20.0;
        private const int RsiBuyThreshold = 40;
        private const int RsiExitThreshold = 50;

        static void Main(string[] args)
            {
            Console.WriteLine("[INFO] Aethelred.Strategy starting...");
            using (var poller = new NetMQPoller())
            using (var coreSubscriber = new SubscriberSocket())
            using (var reportSubscriber = new SubscriberSocket())
            using (var publisher = new PublisherSocket())
                {
                // Подписка на индикаторы от C++ ядра
                coreSubscriber.Connect(CoreConnectAddress);
                coreSubscriber.Subscribe(CoreTopic);
                coreSubscriber.ReceiveReady += (s, a) =>
                {
                    a.Socket.ReceiveFrameString(); // Topic
                    string jsonData = a.Socket.ReceiveFrameString();
                    JObject indicators = JObject.Parse(jsonData);
                    ProcessStrategyLogic(indicators, publisher);
                };
                poller.Add(coreSubscriber);

                // Подписка на отчеты от исполнителя ордеров
                reportSubscriber.Connect(ReportsConnectAddress);
                reportSubscriber.Subscribe(ReportsTopic);
                reportSubscriber.ReceiveReady += (s, a) =>
                {
                    a.Socket.ReceiveFrameString(); // Topic
                    string jsonReport = a.Socket.ReceiveFrameString();
                    var report = JsonConvert.DeserializeObject<ExecutionReport>(jsonReport);
                    ProcessExecutionReport(report);
                };
                poller.Add(reportSubscriber);

                // Публикация торговых сигналов
                publisher.Bind(SignalsBindAddress);
                Console.WriteLine($"[INFO] Binding for signals at {SignalsBindAddress}");
                Thread.Sleep(500);

                Console.WriteLine("[INFO] Starting poller...");
                poller.Run();
                }
            }

        // --- ИЗМЕНЕНО ---
        // Обработка отчетов и обновление состояния
        private static void ProcessExecutionReport(ExecutionReport report)
            {
            Console.WriteLine($"[RECV REPORT] Received report: Success={report.Success}, Signal={report.OriginalSignal}, Msg='{report.ErrorMessage}'");

            if (report.Success)
                {
                if (report.OriginalSignal == "ENTRY_LONG" && StrategyState.CurrentStatus == StrategyState.PositionStatus.PendingEntry)
                    {
                    Console.WriteLine("[STATE] Confirmed: We are IN POSITION.");
                    StrategyState.CurrentStatus = StrategyState.PositionStatus.InPosition;
                    }
                else if (report.OriginalSignal == "EXIT_LONG" && StrategyState.CurrentStatus == StrategyState.PositionStatus.PendingExit)
                    {
                    Console.WriteLine("[STATE] Confirmed: Position is CLOSED.");
                    StrategyState.CurrentStatus = StrategyState.PositionStatus.None;
                    StrategyState.LastTradeType = "None";

                    // --- ДОБАВЛЕНО ---
                    // Сбрасываем трейлинг-стоп, чтобы он был готов для следующей сделки
                    StrategyState.TrailingStopPrice = 0;
                    }
                }
            else
                {
                Console.WriteLine($"[STATE] Order FAILED: {report.ErrorMessage}. Reverting state.");
                if (report.OriginalSignal == "ENTRY_LONG" && StrategyState.CurrentStatus == StrategyState.PositionStatus.PendingEntry)
                    {
                    StrategyState.CurrentStatus = StrategyState.PositionStatus.None;
                    StrategyState.LastTradeType = "None";
                    }
                else if (report.OriginalSignal == "EXIT_LONG" && StrategyState.CurrentStatus == StrategyState.PositionStatus.PendingExit)
                    {
                    Console.WriteLine("[CRITICAL] FAILED TO EXIT POSITION! WE ARE STILL IN THE MARKET.");
                    StrategyState.CurrentStatus = StrategyState.PositionStatus.InPosition;
                    }
                }
            }

        // --- ИЗМЕНЕНО ---
        // Главная логика стратегии
        private static void ProcessStrategyLogic(JObject indicators, PublisherSocket publisher)
            {
            // Извлекаем нужные значения из JSON
            double close = indicators.Value<double>("close");
            double rsi = indicators.Value<double>("rsi");
            double ema = indicators.Value<double>("ema");
            double adx = indicators.Value<double>("adx");
            double highestBreak = indicators.Value<double>("highest_break");
            double atr = indicators.Value<double>("atr"); // ATR нужен для трейлинг-стопа

            const double atrMult = 2.0;

            // --- Логика из Pine Script ---
            bool isTrending = adx > AdxThreshold;
            bool isRanging = !isTrending;
            bool isBullish = close > ema;

            // Условия для входа
            bool rsiLongCondition = isRanging && rsi < RsiBuyThreshold && isBullish;
            bool breakoutLongCondition = isTrending && isBullish && close > highestBreak;

            // --- Принятие решений ---

            // 1. Если у нас нет открытой позиции, ищем точку входа
            if (StrategyState.CurrentStatus == StrategyState.PositionStatus.None)
                {
                if (rsiLongCondition)
                    {
                    Console.WriteLine($"[SIGNAL] RSI Long Entry triggered! RSI: {rsi}, Close: {close}");
                    SendSignal(publisher, "ENTRY_LONG", "RSI", close);
                    StrategyState.CurrentStatus = StrategyState.PositionStatus.PendingEntry;
                    StrategyState.LastTradeType = "RSI";
                    }
                else if (breakoutLongCondition)
                    {
                    Console.WriteLine($"[SIGNAL] Breakout Long Entry triggered! Close: {close} > Highest: {highestBreak}");
                    SendSignal(publisher, "ENTRY_LONG", "Breakout", close);
                    StrategyState.CurrentStatus = StrategyState.PositionStatus.PendingEntry;
                    StrategyState.LastTradeType = "Breakout";
                    }
                }
            // 2. Если мы в позиции, ищем выход, основываясь на типе входа
            else if (StrategyState.CurrentStatus == StrategyState.PositionStatus.InPosition)
                {
                // ВЫХОД ДЛЯ СДЕЛКИ RSI
                if (StrategyState.LastTradeType == "RSI")
                    {
                    bool rsiLongExitCondition = rsi > RsiExitThreshold;
                    if (rsiLongExitCondition)
                        {
                        Console.WriteLine($"[SIGNAL] RSI Long Exit triggered! RSI: {rsi}");
                        SendSignal(publisher, "EXIT_LONG", "RSI", close);
                        StrategyState.CurrentStatus = StrategyState.PositionStatus.PendingExit;
                        }
                    }
                // ВЫХОД ДЛЯ СДЕЛКИ BREAKOUT (ТРЕЙЛИНГ-СТОП)
                else if (StrategyState.LastTradeType == "Breakout")
                    {
                    // Рассчитываем потенциальный новый уровень стопа
                    double newStopPrice = close - (atr * atrMult);

                    // Если это первая свеча после входа, мы просто устанавливаем начальный стоп
                    if (StrategyState.TrailingStopPrice == 0)
                        {
                        StrategyState.TrailingStopPrice = newStopPrice;
                        Console.WriteLine($"[TRAIL] Initial Trailing Stop set at: {StrategyState.TrailingStopPrice:F4}");
                        }
                    else
                        {
                        // Передвигаем стоп ВВЕРХ, если newStopPrice > старого стопа.
                        // Стоп никогда не должен двигаться вниз!
                        if (newStopPrice > StrategyState.TrailingStopPrice)
                            {
                            StrategyState.TrailingStopPrice = newStopPrice;
                            Console.WriteLine($"[TRAIL] Trailing Stop moved up to: {StrategyState.TrailingStopPrice:F4}");
                            }
                        }

                    // ПРОВЕРЯЕМ, НЕ ПРОБИЛА ЛИ ЦЕНА НАШ СТОП
                    if (close < StrategyState.TrailingStopPrice)
                        {
                        Console.WriteLine($"[SIGNAL] Breakout Trailing Stop triggered! Close {close:F4} < Stop {StrategyState.TrailingStopPrice:F4}");
                        SendSignal(publisher, "EXIT_LONG", "Breakout", close);
                        StrategyState.CurrentStatus = StrategyState.PositionStatus.PendingExit;
                        }
                    }
                }
            }

        private static void SendSignal(PublisherSocket publisher, string signal, string type, double price)
            {
            var signalJson = new JObject
                {
                ["symbol"] = "ETHUSDT",
                ["signal"] = signal, // "ENTRY_LONG", "EXIT_LONG"
                ["type"] = type,     // "RSI", "Breakout"
                ["price"] = price
                };

            string messageToSend = signalJson.ToString(Formatting.None);

            publisher.SendMoreFrame(SignalsTopic).SendFrame(messageToSend);
            Console.WriteLine($"[PUB] Published signal: {messageToSend}");
            }
        }
    }