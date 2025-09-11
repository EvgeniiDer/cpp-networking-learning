using NetMQ;
using NetMQ.Sockets;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Threading;
using Websocket.Client;


namespace Aethelred.Gateway
    {
    class Program
        {
        private const string ZmqAddress = "tcp://*:5555";
        private const string ByBitUrl = "wss://stream.bybit.com/v5/public/spot";
        private const string ByBitTopic = "kline.1.ETHUSDT";

        private static PublisherSocket _publisher;
        private static WebsocketClient _client;
        public static void Main(string[] args)
            {
            Console.WriteLine("[INFO] Aethelred.Gateway starting...");
            try
                {
                using (_publisher = new PublisherSocket())
                    {
                    _publisher.Bind(ZmqAddress);
                    Console.WriteLine($"[INFO] ZMQ Publisher bound to {_publisher.Options.LastEndpoint}");
                    using(_client = new WebsocketClient(new Uri(ByBitUrl)))
                        {
                        _client.MessageReceived.Subscribe(OnMessageReceived);
                        _client.ReconnectionHappened.Subscribe(OnReconnectionHappened);

                        _client.Start();
                        Console.WriteLine("[INFO] Websocket client started. Waiting for messages....");
                        Console.ReadLine();
                        }
                    }
                }catch (Exception ex)
                {
                Console.WriteLine($"[FATAl] An error occured: {ex.Message}");
                Console.WriteLine("Press any key to exit");
                Console.ReadKey();
                }
            }
        private static void OnMessageReceived(ResponseMessage msg)
            {
            JObject json = new JObject();
            json = JObject.Parse(msg.Text);

            if (json["topic"] != null && json["topic"].ToString().StartsWith("kline"))
                {
                JArray candlArray = (JArray)json["data"];
                JObject candleInfo = (JObject)candlArray[0];
                if (!(bool)candleInfo["confirm"])
                    return;
                Console.WriteLine($"[RECV] Received final candle data: {candleInfo["close"]}");
                JObject aethleredCandl = new JObject();
                aethleredCandl["event"] = "ADD_CANDLE";
                JObject innerData = new JObject();
                innerData["Open"] = (double)candleInfo["open"];
                innerData["High"] = (double)candleInfo["high"];
                innerData["Low"] = (double)candleInfo["low"];
                innerData["Close"] = (double)candleInfo["close"];
                innerData["Volume"] = (double)candleInfo["volume"];
                

                aethleredCandl["data"] = innerData;
                string messageToSent = aethleredCandl.ToString(Formatting.None);
                _publisher.SendMoreFrame("CANDLE_DATA").SendFrame(messageToSent);
                Console.WriteLine($"[PUB] Pulbished candle to ZMQ. Close price: {candleInfo["close"]}");
                }
            
            }
        private static void OnReconnectionHappened(ReconnectionInfo info)
            {
            Console.WriteLine($"[INFO] Recconect to ByBit WebSocket. Type: {info.Type}");
            Dictionary<string, object> subscribeMessage = new Dictionary<string, object>
                {
                ["op"] = "subscribe",          // операция "подписаться"
                ["args"] = new[] { ByBitTopic } // на какие данные подписываемся
                };

            // 3. Преобразуем сообщение в JSON и отправляем
            string jsonMessage = JsonConvert.SerializeObject(subscribeMessage);
            _client.Send(jsonMessage);

            // 4. Подтверждаем отправку
            Console.WriteLine($"[INFO] Отправили запрос на подписку для: {ByBitTopic}");
            }
       
        }

    }






