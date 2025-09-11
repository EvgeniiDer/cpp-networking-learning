using Newtonsoft.Json;

namespace Aethelred.OrderExecution
    {
    public class TradeSignal
        {
        [JsonProperty("symbol")]
        public string Symbol { get; set; }

        [JsonProperty("signal")]
        public string Signal { get; set; }

        [JsonProperty("type")]
        public string Type { get; set; }

        [JsonProperty("price")]
        public double Price { get; set; }
        }
    }