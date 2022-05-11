#include <Arduino.h>
#line 1 "d:\\firmware\\arduino\\easykey\\easykey.ino"
//引入依赖
#include <ESP8266WiFi.h>
//定时任务
#include <Ticker.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include <Crypto.h>
#include <SHA256.h>

// WIFI热点信息
#define SSID "canting"
#define PASS "minth@888"
#define SHA256HMAC_SIZE 32


// 设置产品和设备的信息，从阿里云设备信息里查看
#define PRODUCT_KEY "a1GMNxoK2vM"
#define DEVICE_NAME "ddkzq2"
#define DEVICE_SECRET "9cec41c4b14fe3a8b67f465bee7512c4"
#define REGION_ID "cn-shanghai"
#define MQTT_SERVER "a1GMNxoK2vM.iot-as-mqtt.cn-shanghai.aliyuncs.com"

// 定义硬件gpio序号
#define statusLed 2
#define relay 13

//创建对象
WiFiClient wifiClient;
Ticker ticker;
PubSubClient mqttClient(wifiClient);

//定义变量
uint8_t ledOffValue = 1;
bool currentLedStatus = true;
bool canSendHeartBeat = false;

#line 38 "d:\\firmware\\arduino\\easykey\\easykey.ino"
void callback(char *topic, byte *payload, unsigned int length);
#line 75 "d:\\firmware\\arduino\\easykey\\easykey.ino"
void reconnect(String clientId, String userName, String password);
#line 99 "d:\\firmware\\arduino\\easykey\\easykey.ino"
void sendHeartBeat();
#line 109 "d:\\firmware\\arduino\\easykey\\easykey.ino"
void wifiInit();
#line 123 "d:\\firmware\\arduino\\easykey\\easykey.ino"
static String hmac256(const String &signcontent, const String &ds);
#line 145 "d:\\firmware\\arduino\\easykey\\easykey.ino"
void setup();
#line 159 "d:\\firmware\\arduino\\easykey\\easykey.ino"
void loop();
#line 38 "d:\\firmware\\arduino\\easykey\\easykey.ino"
void callback(char *topic, byte *payload, unsigned int length)
{

    String controlJson = "";
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    // JSONVar myObject = JSON.parse(payload);
    for (int i = 0; i < length; i++)
    {
        controlJson += (char)payload[i];
    }
    Serial.println(controlJson);
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, controlJson);
    JsonObject obj = doc.as<JsonObject>();

    bool ch1 = obj[String("ch1")];
    bool ch2 = obj[String("ch2")];
    bool ch3 = obj[String("ch3")];

    digitalWrite(relay, ch1);
    // digitalWrite(statusLed, !ledOffValue);
    // digitalWrite(statusLed, !ledOffValue);

    // JSONVar myObject = JSON.parse(configJson);
    // bool openStatus = (bool)myObject["on"];
    // if (openStatus)
    // {
    //     digitalWrite(relay, HIGH);
    // }
    // else
    // {
    //     digitalWrite(relay, LOW);
    // }
}
void reconnect(String clientId, String userName, String password)
{
    while (!mqttClient.connected())
    {
        if (mqttClient.connect(clientId.c_str(), userName.c_str(), password.c_str()))
        {
            Serial.println("connected");
            String queueBeat = "/" + String(PRODUCT_KEY) + "/" + String(DEVICE_NAME) + "/user/heartbeat";
            mqttClient.publish(queueBeat.c_str(), ("{\"deviceId\":" + String(DEVICE_NAME) + "}").c_str(), 1);
            //订阅
            String queueControl = "/" + String(PRODUCT_KEY) + "/" + String(DEVICE_NAME) + "/user/control";
            mqttClient.subscribe(queueControl.c_str());
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
    Serial.println("MQTT Connected!");
}
void sendHeartBeat()
{
    if (mqttClient.connected())
    {
        String queueBeat = "/" + String(PRODUCT_KEY) + "/" + String(DEVICE_NAME) + "/user/heartbeat";
        mqttClient.publish(queueBeat.c_str(), ("{\"deviceId\":" + String(DEVICE_NAME) + "}").c_str(), 1);
        Serial.println("heartbear send!");
    }
}

void wifiInit()
{
    Serial.println("start connect wifi");
    currentLedStatus = ledOffValue;
    WiFi.begin(SSID, PASS);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        currentLedStatus = !currentLedStatus;
        digitalWrite(statusLed, currentLedStatus);
    }
    //连接上WiFi后led常亮
    digitalWrite(statusLed, !ledOffValue);
}
static String hmac256(const String &signcontent, const String &ds)
{
    byte hashCode[SHA256HMAC_SIZE];
    SHA256 sha256;

    const char *key = ds.c_str();
    size_t keySize = ds.length();

    sha256.resetHMAC(key, keySize);
    sha256.update((const byte *)signcontent.c_str(), signcontent.length());
    sha256.finalizeHMAC(key, keySize, hashCode, sizeof(hashCode));

    String sign = "";
    for (byte i = 0; i < SHA256HMAC_SIZE; ++i)
    {
        sign += "0123456789ABCDEF"[hashCode[i] >> 4];
        sign += "0123456789ABCDEF"[hashCode[i] & 0xf];
    }

    return sign;
}

void setup()
{
    Serial.begin(115200);
    Serial.println("system boot!");
    pinMode(statusLed, OUTPUT);
    pinMode(relay, OUTPUT);
    //基础输出
    wifiInit();
    mqttClient.setServer(MQTT_SERVER, 1883);
    mqttClient.setCallback(callback);
    // 60秒一次心跳上报
    ticker.attach(60, sendHeartBeat);
}

void loop()
{
    //保持mqtt连接
    if (!mqttClient.connected())
    {
        long times = millis();
        String timestamp = String(times);
        //阿里云链接参数
        String mqttClientId = String(DEVICE_NAME) + "|securemode=3,signmethod=hmacsha256,timestamp=" + String(timestamp) + "|";
        String signcontent = "clientId";
        signcontent += DEVICE_NAME;
        signcontent += "deviceName";
        signcontent += DEVICE_NAME;
        signcontent += "productKey";
        signcontent += PRODUCT_KEY;
        signcontent += "timestamp";
        signcontent += timestamp;
        String mqttPassword = hmac256(signcontent, DEVICE_SECRET);

        // String mqttClientId = "a1GMNxoK2vM.ddkzq1|securemode=2,signmethod=hmacsha256,timestamp=1652075938999|";
        String mqttUserName = DEVICE_NAME + String("&") + PRODUCT_KEY;
        // String mqttUserName = "ddkzq1&a1GMNxoK2vM";
        // String mqttPassword = "6528d5a707a993c13be7be362bdb7f9cc3bcb223f2bf72bf5ab7e89901f1db29";

        reconnect(mqttClientId, mqttUserName, mqttPassword);
    }
    mqttClient.loop();
}

