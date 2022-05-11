# 1 "d:\\firmware\\arduino\\easykey\\easykey.ino"
//引入依赖
# 3 "d:\\firmware\\arduino\\easykey\\easykey.ino" 2
//定时任务
# 5 "d:\\firmware\\arduino\\easykey\\easykey.ino" 2
# 6 "d:\\firmware\\arduino\\easykey\\easykey.ino" 2
# 7 "d:\\firmware\\arduino\\easykey\\easykey.ino" 2

# 9 "d:\\firmware\\arduino\\easykey\\easykey.ino" 2
# 10 "d:\\firmware\\arduino\\easykey\\easykey.ino" 2

// WIFI热点信息





// 设置产品和设备的信息，从阿里云设备信息里查看






// 定义硬件gpio序号



//创建对象
WiFiClient wifiClient;
Ticker ticker;
PubSubClient mqttClient(wifiClient);

//定义变量
uint8_t ledOffValue = 1;
bool currentLedStatus = true;
bool canSendHeartBeat = false;

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

    digitalWrite(13, ch1);
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
            String queueBeat = "/" + String("a1GMNxoK2vM") + "/" + String("ddkzq2") + "/user/heartbeat";
            mqttClient.publish(queueBeat.c_str(), ("{\"deviceId\":" + String("ddkzq2") + "}").c_str(), 1);
            //订阅
            String queueControl = "/" + String("a1GMNxoK2vM") + "/" + String("ddkzq2") + "/user/control";
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
        String queueBeat = "/" + String("a1GMNxoK2vM") + "/" + String("ddkzq2") + "/user/heartbeat";
        mqttClient.publish(queueBeat.c_str(), ("{\"deviceId\":" + String("ddkzq2") + "}").c_str(), 1);
        Serial.println("heartbear send!");
    }
}

void wifiInit()
{
    Serial.println("start connect wifi");
    currentLedStatus = ledOffValue;
    WiFi.begin("canting", "minth@888");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        currentLedStatus = !currentLedStatus;
        digitalWrite(2, currentLedStatus);
    }
    //连接上WiFi后led常亮
    digitalWrite(2, !ledOffValue);
}
static String hmac256(const String &signcontent, const String &ds)
{
    byte hashCode[32];
    SHA256 sha256;

    const char *key = ds.c_str();
    size_t keySize = ds.length();

    sha256.resetHMAC(key, keySize);
    sha256.update((const byte *)signcontent.c_str(), signcontent.length());
    sha256.finalizeHMAC(key, keySize, hashCode, sizeof(hashCode));

    String sign = "";
    for (byte i = 0; i < 32; ++i)
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
    pinMode(2, 0x01);
    pinMode(13, 0x01);
    //基础输出
    wifiInit();
    mqttClient.setServer("a1GMNxoK2vM.iot-as-mqtt.cn-shanghai.aliyuncs.com", 1883);
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
        String mqttClientId = String("ddkzq2") + "|securemode=3,signmethod=hmacsha256,timestamp=" + String(timestamp) + "|";
        String signcontent = "clientId";
        signcontent += "ddkzq2";
        signcontent += "deviceName";
        signcontent += "ddkzq2";
        signcontent += "productKey";
        signcontent += "a1GMNxoK2vM";
        signcontent += "timestamp";
        signcontent += timestamp;
        String mqttPassword = hmac256(signcontent, "9cec41c4b14fe3a8b67f465bee7512c4");

        // String mqttClientId = "a1GMNxoK2vM.ddkzq1|securemode=2,signmethod=hmacsha256,timestamp=1652075938999|";
        String mqttUserName = "ddkzq2" + String("&") + "a1GMNxoK2vM";
        // String mqttUserName = "ddkzq1&a1GMNxoK2vM";
        // String mqttPassword = "6528d5a707a993c13be7be362bdb7f9cc3bcb223f2bf72bf5ab7e89901f1db29";

        reconnect(mqttClientId, mqttUserName, mqttPassword);
    }
    mqttClient.loop();
}
