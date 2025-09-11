// Aethelred.OrderExecution/Program.cs
using System;
using System.Threading.Tasks;
using NetMQ;
using NetMQ.Sockets;
using Newtonsoft.Json;
using Bybit;
using Bybit.Net.Clients;
using Bybit.Net.Objects.Models.V5;
using CryptoExchange.Net.Authentication;
using Bybit.Net.Enums;
using CryptoExchange.Net.Objects; // Добавлен для использования WebCallResult
using System.Threading;

namespace Aethelred.OrderExecution
    {
    public class ExecutionReport
        {
        public string OriginalSignal {  get; set; }
        public string Symbol {  get; set; }
        public bool Success { get; set; }
        public string OrderId {  get; set; }
        public string ErrorMessage { get; set; }
        }
    internal class Program
        {
        // Адрес, где слушаем сигналы от Strategy
        private const string SignalsConnectAddress = "tcp://localhost:5557";
        private const string SignalsTopic = "TRADE_SIGNALS";

        //Адрес куда публикуем отчеты для Strategy
        private const string ReportsBindAddress = "tcp://*:5558";
        private const string ReportsTopic = "EXECUTION_REPORTS";

        // Клиент для работы с биржей
        private static BybitRestClient _bybitClient;

        static async Task Main(string[] args)
            {
            Console.WriteLine("[INFO] Aethelred.OrderExecution starting...");

            // --- Инициализация клиента ByBit ---
            // ВАЖНО: Храни ключи в переменных окружения, а не в коде.
            string apiKey = Environment.GetEnvironmentVariable("BYBIT_API_KEY");
            string apiSecret = Environment.GetEnvironmentVariable("BYBIT_API_SECRET");

            if (string.IsNullOrEmpty(apiKey) || string.IsNullOrEmpty(apiSecret))
                {
                Console.WriteLine("[FATAL] API keys are not set. Please set BYBIT_API_KEY and BYBIT_API_SECRET environment variables.");
                return;
                }

            _bybitClient = new BybitRestClient(options =>
            {
                options.ApiCredentials = new ApiCredentials(apiKey, apiSecret);
                options.AutoTimestamp = true;
                options.TimestampRecalculationInterval = TimeSpan.FromMinutes(30);
            });

            try
                {
                using (SubscriberSocket subscriberSocket = new SubscriberSocket())
                using (PublisherSocket publisherSocket = new PublisherSocket()) 
                    {
                    subscriberSocket.Connect(SignalsConnectAddress);
                    subscriberSocket.Subscribe(SignalsTopic);
                    Console.WriteLine($"[INFO] Subscribed to '{SignalsTopic}' at {SignalsConnectAddress}");

                    publisherSocket.Bind(ReportsBindAddress);
                    Console.WriteLine($"[INFO] Binding for reports at {ReportsBindAddress}");
                    Thread.Sleep(500);
                    while (true)
                        {
                        // Ждем сообщение. Первым приходит топик, вторым - данные.
                        string receivedTopic = subscriberSocket.ReceiveFrameString();
                        string jsonSignal = subscriberSocket.ReceiveFrameString();

                        Console.WriteLine($"[RECV] Received signal: {jsonSignal}");

                        Console.WriteLine($"[DEBUG] Received raw signal string: '>{jsonSignal}<'");
                        // Десериализуем JSON в наш объект TradeSignal
                        TradeSignal signal = JsonConvert.DeserializeObject<TradeSignal>(jsonSignal);

                        if (signal != null)
                            {
                            // Передаем сигнал на асинхронную обработку
                            await ProcessSignal(signal, publisherSocket);
                            }
                        }
                         
                    }
                }
            catch (Exception ex)
                {
                Console.WriteLine($"[FATAL] An error occurred: {ex.Message}");
                }
            }

        
        private static async Task ProcessSignal(TradeSignal signal, PublisherSocket publisher)
            {
            string symbol = signal.Symbol;
            ExecutionReport report = new ExecutionReport
                {
                OriginalSignal = signal.Signal,
                Symbol = signal.Symbol
                };

            WebCallResult<BybitOrderId> orderResult = null;

            switch (signal.Signal)
                {
                case "ENTRY_LONG":
                decimal amountInUsdt = 10m;             
                Console.WriteLine($" [{DateTime.Now.ToString("HH:mm:ss")}][EXECUTION] Placing MARKET BUY order for {amountInUsdt} USDT of {symbol}");

                orderResult = await _bybitClient.V5Api.Trading.PlaceOrderAsync(
                    Category.Spot,
                    symbol,
                    OrderSide.Buy,
                    NewOrderType.Market,
                    quantity: amountInUsdt,
                    marketUnit: MarketUnit.QuoteAsset
                );
                break;
                case "EXIT_LONG":
                Console.WriteLine($"{DateTime.Now.ToString("HH: mm:ss")}[EXECUTION] Attempting to close entire position. Fetching balance..."); ;
                var balanceResult = await _bybitClient.V5Api.Account.GetBalancesAsync(AccountType.Unified);

                if (!balanceResult.Success)
                    {
                    Console.WriteLine($"[ERROR] Failed to get wallet balance: {balanceResult.Error.Message}");
                    report.Success = false;
                    report.ErrorMessage = "Failed to get wallet balance: " + balanceResult.Error.Message;
                    // orderResult остается null, отчет будет отправлен в конце
                    }
                else
                    {
                    var accountInfo = balanceResult.Data.List.FirstOrDefault();
                    if (accountInfo != null)
                        {
                        var ethBalance = accountInfo.Assets.FirstOrDefault(c => c.Asset == "ETH");
                        if (ethBalance != null && ethBalance.WalletBalance > 0)
                            {
                            decimal quantityToSell = ethBalance.WalletBalance.Value;
                            decimal quantityStep = 0.00001m;
                            quantityToSell = Math.Floor(quantityToSell / quantityStep) * quantityStep;
                            Console.WriteLine($"[EXECUTION] Balance found: {quantityToSell} ETH. Placing MARKET SELL order...");

                            orderResult = await _bybitClient.V5Api.Trading.PlaceOrderAsync(
                                Category.Spot,
                                symbol: "ETHUSDT",
                                OrderSide.Sell,
                                NewOrderType.Market,
                                quantity: quantityToSell
                            );
                            }
                        else
                            {
                            // ИСПРАВЛЕНИЕ 2: Обработка случая, когда продавать нечего
                            Console.WriteLine("[WARN] No ETH available to sell. Position might be already closed.");
                            report.Success = true; // Считаем это успехом, т.к. цели (выйти из позиции) мы достигли
                            report.ErrorMessage = "No position to close.";
                            // orderResult остается null, отчет будет отправлен в конце
                            }
                        }
                    else
                        {
                        // Дополнительная обработка редкого, но возможного случая
                        Console.WriteLine("[ERROR] Could not find account info in balance response.");
                        report.Success = false;
                        report.ErrorMessage = "Could not find account info in balance response.";
                        }
                    }
                break;

                default:
                Console.WriteLine($"[WARN] Unknown signal received: {signal.Signal}");
                return; // Выходим, отчет не отправляем
                }

            // --- Улучшенная логика отправки отчета ---
            // Этот блок теперь выполняется всегда и обрабатывает все сценарии

            // Если был выполнен вызов ордера, обрабатываем его результат
            if (orderResult != null)
                {
                if (orderResult.Success)
                    {
                    Console.WriteLine($"[SUCCESS] Order placed successfully. Order ID: {orderResult.Data.OrderId}");
                    report.Success = true;
                    report.OrderId = orderResult.Data.OrderId;
                    }
                else
                    {
                    Console.WriteLine($"[ERROR] Failed to place order: {orderResult.Error.Message}");
                    report.Success = false;
                    report.ErrorMessage = orderResult.Error.Message;
                    }
                }
            // Если orderResult == null, значит, отчет уже был частично заполнен
            // в блоках обработки ошибок или в случае отсутствия баланса.

            // ИСПРАВЛЕНИЕ 3: Отправляем отчет в любом случае, чтобы Strategy не зависла
            string jsonReport = JsonConvert.SerializeObject(report);
            publisher.SendMoreFrame(ReportsTopic).SendFrame(jsonReport);
            Console.WriteLine($"[PUB] Published report: {jsonReport}");
            }
        }
    }