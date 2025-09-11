using Bybit.Net.Clients;
using Bybit.Net.Enums;
using Bybit.Net.Objects.Models.V5;
using CryptoExchange.Net.Authentication;
using CryptoExchange.Net.Objects;
using System;
namespace CheckAccount
    {
    internal class Program
        {
 
        static async Task Main(string[] args)
            {
            string apiKey = Environment.GetEnvironmentVariable("BYBIT_API_KEY");
            string apiSecrt = Environment.GetEnvironmentVariable("BYBIT_API_SECRET");
            if(string.IsNullOrEmpty(apiKey) || string.IsNullOrEmpty(apiSecrt))
                {
                Console.WriteLine("[FATAL] API keys are not set. Please set BYBIT_API_KEY and BYBIT_API_SECRET environment variables.");
                return;
                }
            else
                {
                Bybit.Net.Clients.BybitRestClient bybitClient = new Bybit.Net.Clients.BybitRestClient();
                ApiCredentials _apiCred = new ApiCredentials(apiKey, apiSecrt);
                bybitClient.SetApiCredentials(_apiCred);
                try
                    {
                    WebCallResult<BybitOrderId> orderResult = null;
                    WebCallResult<BybitResponse<BybitBalance>> balance = await bybitClient.V5Api.Account.GetBalancesAsync(AccountType.Unified);
                     if(balance.Success)
                        {
                        Bybit.Net.Objects.Models.V5.BybitBalance bybitAssetAccountBalance = null;
                        foreach(Bybit.Net.Objects.Models.V5.BybitBalance account in balance.Data.List)
                            {
                            if(account.AccountType == AccountType.Unified)
                                {
                                bybitAssetAccountBalance = account;
                                break;
                                }
                            }
                        if(bybitAssetAccountBalance != null)
                            {

                            decimal quantityToSell = 0;
                            Console.WriteLine("Balance is: ");
                            Bybit.Net.Objects.Models.V5.BybitAssetBalance assetBalance = null;
                            foreach(var asset in bybitAssetAccountBalance.Assets)
                                {
                                Console.WriteLine(asset.WalletBalance);
                                if(asset.Asset == "ETH")
                                    {
                                    assetBalance = asset;
                                    quantityToSell = assetBalance.WalletBalance.Value;
                                    break;
                                    }
                                }
                            decimal quantityStep = 0.00001m;
                            decimal result;
                            quantityToSell = Math.Floor(quantityToSell / quantityStep) * quantityStep;
                            Console.WriteLine($"quantityToSell: {quantityToSell} ");

                            //if (quantityToSell > 0)
                            //    {
                            //    orderResult = await bybitClient.V5Api.Trading.PlaceOrderAsync(
                            //       Category.Spot,
                            //       symbol: "ETHUSDT",
                            //       OrderSide.Sell,
                            //       NewOrderType.Market,
                            //       quantity: quantityToSell
                            //        );
                            //    }
                            //if (orderResult.Success)
                            //    {
                            //    Console.WriteLine($"Ордер успешно размещен! ID ордера: {orderResult.Data.OrderId}");
                            //    }
                            //else
                            //    {
                            //    // Эта часть кода покажет вам точную причину ошибки от биржи
                            //    Console.WriteLine($"Ошибка размещения ордера: {orderResult.Error.Code} - {orderResult.Error.Message}");
                            //    }
                            }
                        else
                            {
                            Console.WriteLine("Cant access to Balance");
                            }
                        }
                    else
                        {
                        Console.WriteLine("Error to get balance access: " + balance.Error.Message);
                        }
                    }
                catch (Exception ex)
                    {
                    Console.WriteLine("Произошла критическая ошибка: " + ex.Message);
                    }

                }
            
            }
        }
    }
