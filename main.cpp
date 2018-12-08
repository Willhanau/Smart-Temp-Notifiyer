#include "mbed.h"
#include "https_request.h"
#include "http_request.h"
#include "rtos.h"

DigitalOut myled(LED1);
WiFiInterface *wifi;

//Threads
Thread temp_Thread;
Thread buttons_Thread;
Thread schedule_twilio_Thread;

//Variables for temperature sensor DS1631
I2C temp_sensor(I2C_SDA, I2C_SCL);
const int temp_addr = 0x90;
//0x51 -> Start Convert Temp
//0xAA -> Read Temp
const char cmd[] = {0x51, 0xAA};
char read_temp[2];
float temp = 0;

//Input Buttons
DigitalIn hour_Button(PB_4);
DigitalIn minute_Button(PB_5);
DigitalIn set_Button(PB_3);
DigitalIn query_Button(PA_10);
DigitalIn onboard_Button(USER_BUTTON);

//Variables
int current_b = 0;
int previous_b = 0;
int notify_time[] = {0, 0};
int set_notify_time[2];
int classify = 0;

const char body[] = "{\"requests\": [{\"image\": {\"source\": {\"imageUri\": \"https://cloud.google.com/vision/images/rushmore.jpg\"}}, \"features\": [{\"type\": \"LABEL_DETECTION\", \"maxResults\": 5}]}]}";
const char body2[] = "{\"requests\": [{\"image\": {\"source\": {\"imageUri\": \"https://storage.googleapis.com/wh-images/Person.jpg\"}}, \"features\": [{\"type\": \"LABEL_DETECTION\", \"maxResults\": 5}]}]}";

const char SSL_CA_PEM[] = "-----BEGIN CERTIFICATE-----\n"
                        "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n"
                        "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
                        "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"
                        "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n"
                        "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
                        "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n"
                        "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n"
                        "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n"
                        "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n"
                        "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n"
                        "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n"
                        "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n"
                        "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n"
                        "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n"
                        "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n"
                        "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n"
                        "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n"
                        "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n"
                        "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n"
                        "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"
                        "-----END CERTIFICATE-----\n"
                        "-----BEGIN CERTIFICATE-----\n"
                        "MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\n"
                        "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\n"
                        "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\n"
                        "MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\n"
                        "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n"
                        "hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\n"
                        "v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\n"
                        "eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\n"
                        "tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd\n"
                        "C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\n"
                        "zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\n"
                        "mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\n"
                        "V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\n"
                        "bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\n"
                        "3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\n"
                        "J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\n"
                        "291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\n"
                        "ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\n"
                        "AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\n"
                        "TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\n"
                        "-----END CERTIFICATE-----\n";



void Blink_LED(){
    while(1) {
        myled = 1; // LED is ON
        wait(0.2); // 200 ms
        myled = 0; // LED is OFF
        wait(1.0); // 1 sec
    } 
}

int Connect_to_Wifi(WiFiInterface *wifi){
    printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        printf("\nConnection error: %d\n", ret);
        return 0;
    }

    printf("Success\n\n");
    printf("MAC: %s\n", wifi->get_mac_address());
    printf("IP: %s\n", wifi->get_ip_address());
    printf("Netmask: %s\n", wifi->get_netmask());
    printf("Gateway: %s\n", wifi->get_gateway());
    printf("RSSI: %d\n\n", wifi->get_rssi());
    
    return 1;
}

void Google_Cloud_Vision_https_Request(NetworkInterface *network){
    printf("\n\nSending Google Cloud Vision a https request!\n\n");
    
    HttpsRequest* request = new HttpsRequest(network, SSL_CA_PEM, HTTP_POST, MBED_CONF_APP_GC_VISION_API_URL);
    request->set_header("Content-Type", "application/json");
    HttpResponse* response;
    if(classify == 0) response = request->send(body, strlen(body));
    if(classify == 1) response = request->send(body2, strlen(body2));
    
    //Print Http Response
    printf("Status: %d - %s\n", response->get_status_code(), response->get_status_message().c_str());
    printf("Headers:\n");
    for (size_t ix = 0; ix < response->get_headers_length(); ix++) {
        printf("\t%s: %s\n", response->get_headers_fields()[ix]->c_str(), response->get_headers_values()[ix]->c_str());
    }
    printf("\nBody (%lu bytes):\n\n%s\n", response->get_body_length(), response->get_body_as_string().c_str());
    
    classify = !classify;
    delete request;
}

void Twilio_https_Request(NetworkInterface *network){
    printf("\n\nSending Twilio a https request!\n\n");
    
    HttpsRequest* request = new HttpsRequest(network, SSL_CA_PEM, HTTP_POST, MBED_CONF_APP_TWILIO_API_URL);
    request->set_header("Content-Type", "application/x-www-form-urlencoded");
    request->set_header("Authorization", MBED_CONF_APP_TWILIO_BASIC_AUTH);
    
    //Please enter your phone number(country code, area code, number) and twilio phone number where the Xs are
    char body[200] = "To=%2BXXXXXXXXXXX&From=%2BXXXXXXXXXXX&MediaUrl=https%3A%2F%2Fdemo.twilio.com%2Fowl.png&Body=";
    char tmp[50];
    sprintf(tmp, "Current temperature:+%.2f+Celsius", temp);
    strcat(body, tmp);
    HttpResponse* response = request->send(body, strlen(body));
    
    //Print Http Response
    printf("Status: %d - %s\n", response->get_status_code(), response->get_status_message().c_str());
    printf("Headers:\n");
    for (size_t ix = 0; ix < response->get_headers_length(); ix++) {
        printf("\t%s: %s\n", response->get_headers_fields()[ix]->c_str(), response->get_headers_values()[ix]->c_str());
    }
    printf("\nBody (%lu bytes):\n\n%s\n", response->get_body_length(), response->get_body_as_string().c_str());
    
    delete request;
}

void get_Temperature(){
    while(1){
        myled = 1; // LED is ON
        //Send command to Start convert temp
        temp_sensor.write(temp_addr, &cmd[0], 1);
        
        //Send command to read temp
        temp_sensor.write(temp_addr, &cmd[1], 1);
        
        //read temp sensor
        temp_sensor.read(temp_addr, read_temp, 2);
        
        //Convert temperature to Celsius
        temp = (float((read_temp[0] << 8) | read_temp[1]) / 256);
        myled = 0; // LED is OFF
        wait(2);
    }
}

void scheduled_Twilio_Request(){
    while(1){
        wait((3600 * set_notify_time[0]) + (60 * set_notify_time[1]));
        Twilio_https_Request(wifi);
    }
}

void increment_hour(){
    notify_time[0]++;
    notify_time[0] %= 24;
    
    if(notify_time[1] < 10){
        printf("\f\n24-hour-Time (hours:minutes):\n%d:0%d\n", notify_time[0], notify_time[1]);
    } else {
        printf("\f\n24-hour-Time (hours:minutes):\n%d:%d\n", notify_time[0], notify_time[1]);
    }
}

void increment_minute(){
    notify_time[1]++;
    notify_time[1] %= 60;
    
    if(notify_time[1] < 10){
        printf("\f\n24-hour-Time (hours:minutes):\n%d:0%d\n", notify_time[0], notify_time[1]);
    } else {
        printf("\f\n24-hour-Time (hours:minutes):\n%d:%d\n", notify_time[0], notify_time[1]);
    }
}

void set_notification_time(){
    //Prompt user to set a time if hours and minutes is not at least greater than 0
    if((notify_time[0] != 0) || (notify_time[1] != 0)){
        //set scheduled time
        set_notify_time[0] = notify_time[0];
        set_notify_time[1] = notify_time[1];
        
        if(set_notify_time[1] < 10){
            printf("\f\nNotification time has been set for (hours:minutes):\n%d:0%d\n", set_notify_time[0], set_notify_time[1]);
        } else {
            printf("\f\nNotification time has been set for (hours:minutes):\n%d:%d\n", set_notify_time[0], set_notify_time[1]);
        }
        
        //Start event
        schedule_twilio_Thread.start(scheduled_Twilio_Request);
    } else {
        printf("\f\nPlease set a time that is not 0 hours and 0 mins.\n");
    }
}

void print_current_temp_and_notify_time(){
    printf("\f\nCurrent temperature:\n%.2f Celsius\n", temp);
    if(set_notify_time[1] < 10){
        printf("\nCurrent notification time is set for (hours:minutes):\n%d:0%d\n", set_notify_time[0], set_notify_time[1]);
    } else {
        printf("\nCurrent notification time is set for (hours:minutes):\n%d:%d\n", set_notify_time[0], set_notify_time[1]);
    }
}

void input_Buttons_Loop(){
    while(1){
        
        if(!hour_Button){
            previous_b = current_b;
            current_b = 1;
            if(previous_b != current_b){
                increment_hour();
                wait(0.1);
            }
        }
        else if(!minute_Button){
            previous_b = current_b;
            current_b = 2;
            if(previous_b != current_b){
                increment_minute();
                wait(0.1);
            }
        }
        else if(!set_Button){
            previous_b = current_b;
            current_b = 3;
            if(previous_b != current_b){
                set_notification_time();
                wait(0.1);
            }
        }
        else if(!query_Button){
            previous_b = current_b;
            current_b = 4;
            if(previous_b != current_b){
                print_current_temp_and_notify_time();
                wait(0.1);
            }
        }
        else if(!onboard_Button){
            previous_b = current_b;
            current_b = 5;
            if(previous_b != current_b){
                Google_Cloud_Vision_https_Request(wifi);
                wait(0.1);
            }
        }
        else{
            previous_b = current_b;
            current_b = 0;
        }
    }   
}


int main() {
    printf("\fSmart Temp Notifyer V2.0\n");

#ifdef MBED_MAJOR_VERSION
    printf("Mbed OS version %d.%d.%d\n\n", MBED_MAJOR_VERSION, MBED_MINOR_VERSION, MBED_PATCH_VERSION);
#endif
    
    //Intialize wifi interface
    wifi = WiFiInterface::get_default_instance();
    if (!wifi) {
        printf("ERROR: No WiFiInterface found.\n");
        return -1;
    }
    
    //Connect to wifi access point specified in mbed_app.json
    if(Connect_to_Wifi(wifi) == 1){
        printf("\nConnection to Wifi was SUCCESSFUL!\n");    
    } else{
        printf("\nConnection to wifi FAILED...\n");        
    }
    
    //Disconnect from wifi access point
    //wifi->disconnect();
    //printf("\nDone: Wifi has successfully disconnected.\n");
    
    temp_Thread.start(get_Temperature);
    buttons_Thread.start(input_Buttons_Loop);
    
}