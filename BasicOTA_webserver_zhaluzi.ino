#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include "timer2Minim.h"

#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include<AccelStepper.h>
#define HALFSTEP 8
// Определение пинов для управления двигателем
#define motorPin1  D1 // IN1 на 1-м драйвере ULN2003
#define motorPin2  D5 // IN2 на 1-м драйвере ULN2003
#define motorPin3  D6 // IN3 на 1-м драйвере ULN2003
#define motorPin4  D7 // IN4 на 1-м драйвере ULN2003
AccelStepper stepper1(HALFSTEP, motorPin1, motorPin3, motorPin2, motorPin4);

#define INIT_ADDR 500
#define INIT_KEY 50
int OTKR = 150; //уставка света, при котором открываются жалюзи
int ZAKR = 400; //уставка света, при котором закрываются жалюзи
int ZAKR_vnutr = 800;  //уставка внутреннего освещения, при котором закрываются жалюзи
int ZAKR_vnesh = 80;   //уставка внешнего освещение, при котором блокируется закрытие по внутр. свету
int ZAKR_vnesh_hist = 5;   //гистерезис уставки внешнего освещение, при котором блокируется закрытие по внутр. свету
unsigned int rast = 42000, rast_calibrate;
unsigned long TIME_ZAKR = 60000; //время, за которое закрываются жалюзи

#ifndef STASSID
#define STASSID "DIR-620"
#define STAPSK  "09012014"
#endif
#define NTP_INTERVAL 60 * 1000    // обновление (1 минута)
//#define NTP_INTERVAL 10 * 1000    // обновление (10 секунд)
#define ESP_MODE 1
#define GMT 3              // смещение (москва 3)
#define NTP_ADDRESS  "europe.pool.ntp.org"    // сервер времени

WiFiUDP Udp;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, GMT * 3600, NTP_INTERVAL);
timerMinim timeTimer(1000);
timerMinim timeStrTimer(120);

const char* ssid = STASSID;
const char* password = STAPSK;
unsigned long t1, t2, t3, t_a0, t1_test, t_zakr, t_conn;
byte hrs, mins, secs, days, index_val;
String timeStr = "00:00";
byte time_init = 1;
String get_value;
int val0_1, val0_2;
//int time1_hrs=6, time1_mins=30, time2_hrs=22, time2_mins=0;
int time1_hrs[] = {6,6,6,6,6,6,6};
int time1_mins[] = {30,30,30,30,30,30,30};
int time2_hrs[] = {22,22,22,22,22,22,22};
int time2_mins[] = {0,0,0,0,0,0,0};
byte napr, napr_calibrate;
boolean f_a0, client_open, client_close, zhaluzi_open, zhaluzi_close, f_calibrate,
ust0, f_svet_vnutr, f_sost_open, f_sost_close, dvizh, open_time1, close_time2, f_ip=1;
int val_vnesh[5];
int val_vnutr[5];
long  svet, svet_vnutr, delay_v = 0, delay_n = 0, delay_ust = 100;
const int sclPin = D3;
const int sdaPin = D2;

ESP8266WebServer server;
char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8" name="viewport" content="width=devise-width, initial-scale=1">
<title>ESPServer</title>
<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">

<script>

function w3Open() {
  document.getElementById("mySidebar").style.display = "block";
}

function w3Close() {
  document.getElementById("mySidebar").style.display = "none";
}

function myFunction_open(){
  var xhr = new XMLHttpRequest();
  var url = "/curtainsOpen";

//  xhr.onreadystatechange = function() {
//    if (this.readyState == 4 && this.status == 200) {
//      document.getElementById("curtains-state").innerHTML = this.responseText;
//    }
//  };
  xhr.open("GET", url, true);
  xhr.send();
}

function myFunction_close(){
  var xhr = new XMLHttpRequest();
  var url = "/curtainsClose";

//  xhr.onreadystatechange = function() {
//    if (this.readyState == 4 && this.status == 200) {
//      document.getElementById("curtains-state").innerHTML = this.responseText;
//    }
//  };
  xhr.open("GET", url, true);
  xhr.send();
}

function myFunction(){
  console.log("button was clicked!");
  var xhr = new XMLHttpRequest();
  var url = "/ledstate";

  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("led-state").innerHTML = this.responseText;
    }
  };
  xhr.open("GET", url, true);
  xhr.send();
}

function myFunction2(){
  var xhr = new XMLHttpRequest();
  var url = "/a0state";
  xhr.responseType = "json";

  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var jsonResponse = this.response;
      document.getElementById("a0_1-state").innerHTML = jsonResponse["val0_1"];
      document.getElementById("a0_2-state").innerHTML = jsonResponse["val0_2"];
    }
  };
  xhr.open("GET", url, true);
  xhr.send();
}

function myFunctionZhaluziState(){
  var xhr = new XMLHttpRequest();
  var url = "/zhaluzistate";
  xhr.responseType = "json";

  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var jsonResponse = this.response;
      
      var sost_close = jsonResponse["zhaluzi_state"];
      if (sost_close == "1") {
        document.getElementById("curtains-state").innerHTML = "Закрыты";
      } else if (sost_close == "0") {
        document.getElementById("curtains-state").innerHTML = "Открыты";
      }
      var auto_state = jsonResponse["napr"];
      if (auto_state == "1") {
        document.getElementById("auto-state").innerHTML = "Ожидается открытие";
      } else if (auto_state == "2") {
        document.getElementById("auto-state").innerHTML = "Ожидается закрытие";
      }
    }
  };
  xhr.open("GET", url, true);
  xhr.send();
}

function myFunctionTime(){
  var xhr = new XMLHttpRequest();
  var url = "/timestate";
  
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var serverResponse = JSON.parse(xhr.responseText);
      document.getElementById("time-state").innerHTML = serverResponse["timeStr"];
      switch (serverResponse["days"]) {
      case 0:
        document.getElementById("days-state").innerHTML = "Воскресенье";
        break;
      case 1:
        document.getElementById("days-state").innerHTML = "Понедельник";
        break;
      case 2:
        document.getElementById("days-state").innerHTML = "Вторник";
        break;
      case 3:
        document.getElementById("days-state").innerHTML = "Среда";
        break;
      case 4:
        document.getElementById("days-state").innerHTML = "Четверг";
      break;
      case 5:
        document.getElementById("days-state").innerHTML = "Пятница";
      break;
      case 6:
        document.getElementById("days-state").innerHTML = "Суббота";
      break;
      default:
        break;
      }
    }
  };
  xhr.open("GET", url, true);
  xhr.send();
}

function myFunctionTestvalue(){
  var xhr = new XMLHttpRequest();
  var url = "/sendvalues";

  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("test-value").innerHTML = this.responseText;
    }
  };
  xhr.open("GET", url, true);
  xhr.send();
}

function myFunctionTestvalues(){
  var xhr = new XMLHttpRequest();
  var url = "/sendvalues";
  xhr.responseType = "json";

  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var jsonResponse = this.response;
      param_json = jsonResponse;
      
      document.getElementById("test_val1").value = jsonResponse["test_val1"];
      document.getElementById("test_val2").value = jsonResponse["test_val2"];
      document.getElementById("test_val3").value = jsonResponse["test_val3"];
      document.getElementById("test_val4").value = jsonResponse["test_val4"];
      document.getElementById("test_rast").value = jsonResponse["test_rast"];
//      document.getElementById("test_val5").value = jsonResponse["test_val5"];
      document.getElementById("test_napr").value = jsonResponse["test_napr"];
      document.getElementById("test_time1_1").value = param_json["test_time1_1"];
      document.getElementById("test_time2_1").value = param_json["test_time2_1"];
      document.getElementById("test_time1_2").value = param_json["test_time1_2"];
      document.getElementById("test_time2_2").value = param_json["test_time2_2"];
      document.getElementById("test_time1_3").value = param_json["test_time1_3"];
      document.getElementById("test_time2_3").value = param_json["test_time2_3"];
      document.getElementById("test_time1_4").value = param_json["test_time1_4"];
      document.getElementById("test_time2_4").value = param_json["test_time2_4"];
      document.getElementById("test_time1_5").value = param_json["test_time1_5"];
      document.getElementById("test_time2_5").value = param_json["test_time2_5"];
      document.getElementById("test_time1_6").value = param_json["test_time1_6"];
      document.getElementById("test_time2_6").value = param_json["test_time2_6"];
      document.getElementById("test_time1_0").value = param_json["test_time1_0"];
      document.getElementById("test_time2_0").value = param_json["test_time2_0"];
//      if ( jsonResponse["test_napr"] == 1 ) {
//        document.getElementById("curtains-state").innerHTML = "Закрыты";
//      } else if ( jsonResponse["test_napr"] == 2 ) {
//        document.getElementById("curtains-state").innerHTML = "Открыты";
//      }
      
    }
  };
  
  xhr.open("GET", url, true);
  xhr.send();
}

function TimeSelectChange() {
  var TimeSelectVal = document.getElementById("select_days").value;
  switch (TimeSelectVal) {
  case "0":
    document.getElementById("test_time1").value = param_json["test_time1_0"];
    document.getElementById("test_time2").value = param_json["test_time2_0"];
    break;
  case "1":
    document.getElementById("test_time1").value = param_json["test_time1_1"];
    document.getElementById("test_time2").value = param_json["test_time2_1"];
    break;
  case "2":
    document.getElementById("test_time1").value = param_json["test_time1_2"];
    document.getElementById("test_time2").value = param_json["test_time2_2"];
    break;
  case "3":
    document.getElementById("test_time1").value = param_json["test_time1_3"];
    document.getElementById("test_time2").value = param_json["test_time2_3"];
    break;
  case "4":
    document.getElementById("test_time1").value = param_json["test_time1_4"];
    document.getElementById("test_time2").value = param_json["test_time2_4"];
  break;
  case "5":
    document.getElementById("test_time1").value = param_json["test_time1_5"];
    document.getElementById("test_time2").value = param_json["test_time2_5"];
  break;
  case "6":
    document.getElementById("test_time1").value = param_json["test_time1_6"];
    document.getElementById("test_time2").value = param_json["test_time2_6"];
  break;
  default:
    break;
  }
}

function myFuncAccord(id) {
  var x = document.getElementById(id);
  if (x.className.indexOf("w3-show") == -1) {
    x.className += " w3-show";
  } else { 
    x.className = x.className.replace(" w3-show", "");
  }
}

setInterval(myFunction2, 500);
setInterval(myFunctionTime, 1000);
setInterval(myFunctionZhaluziState, 1000);
var param_json;
document.addEventListener('DOMContentLoaded', myFunctionTestvalues);
</script>

</head>
<body>

<div class="w3-sidebar w3-bar-block w3-collapse w3-card w3-animate-left" style="width:200px;" id="mySidebar">
  <button class="w3-bar-item w3-button w3-large w3-hide-large" onclick="w3Close()">Закрыть &times;</button>
  <a href="/" class="w3-bar-item w3-button">Главная</a>
  <a href="/description" class="w3-bar-item w3-button">Описание</a>
  <a href="/calibration" class="w3-bar-item w3-button">Калибровка</a>
</div>

<div class="w3-main" style="margin-left:200px">
  <div class="w3-row w3-khaki">
    <div class="w3-col s1">
      <button class="w3-button w3-xlarge w3-hide-large" style="margin-top:10px" onclick="w3Open()">&#9776;</button>
    </div>
    <div class="w3-container w3-rest" onclick="w3Close()">
      <h1>Автоматические жалюзи</h1>
    </div>
  </div>
  <br>
  
  <div class="w3-row-padding" onclick="w3Close()">
    <div class="w3-col m3">
      <div class="w3-container w3-khaki w3-card-4">
        <p> Жалюзи: <span id="curtains-state"> </span></p>
        <p> Автоматический режим: <span id="auto-state"> </span></p>
        <p> Свет снаружи: <span id="a0_1-state">___</span></p>
        <p> Свет внутри: <span id="a0_2-state">___</span></p>
        <p> Время: <span id="time-state">___</span> &nbsp; <span id="days-state">___</span></p>
        <p> Состояние LED: <span id="led-state"></span></p>
        <p> Тестовый JSON с параметрами: <span id="test-value"> </span></p>
      </div><br>
      <div class="w3-container">
        <button style="width:100px" onclick="myFunction_open()"> Открыть </button>&emsp;
        <button style="width:100px" onclick="myFunction_close()"> Закрыть </button>
      </div><br>
      <div class="w3-container">
        <button style="width:100px" onclick="myFunction()"> LED </button>&emsp;
        <button style="width:100px" onclick="myFunctionTestvalue()"> test_json </button>
      </div><br>
    </div>
    <div class="w3-col m6">
      <div class="w3-container w3-card-4 w3-padding-small">
        <button onclick="myFuncAccord('Accord1')" class="w3-btn w3-block" style="outline:none; box-shadow:0px 0px"><h3 class="w3-center">Запись времени</h3></button>
        <div id="Accord1" class="w3-hide">
          <form action="/gettimes" method="post">
            <table class="w3-table w3-striped w3-bordered" style="width:100%">
            <tr class="w3-row">
              <th class="w3-col s4 w3-padding-small"><label class="w3-text" style="padding-top:8px;display:inline-block">День недели</label></td>
              <th class="w3-col s4 w3-padding-small"><label class="w3-text" style="padding-top:8px;display:inline-block">Открытие</label></td>
              <th class="w3-col s4 w3-padding-small"><label class="w3-text" style="padding-top:8px;display:inline-block">Закрытие</label></td>
            </tr>
            <tr class="w3-row">
              <td class="w3-col s4 w3-padding-small"><label class="w3-text" style="padding-top:8px;display:inline-block"><b>Понедельник</b></label></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px" name="test_time1_1" id="test_time1_1" class="w3-input w3-border w3-small" type="time"></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px" name="test_time2_1" id="test_time2_1" class="w3-input w3-border w3-small" type="time"></td>
            </tr>
            <tr class="w3-row">
              <td class="w3-col s4 w3-padding-small"><label class="w3-text" style="padding-top:8px;display:inline-block"><b>Вторник</b></label></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px"  name="test_time1_2" id="test_time1_2" class="w3-input w3-border w3-small" type="time"></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px"  name="test_time2_2" id="test_time2_2" class="w3-input w3-border w3-small" type="time"></td>
            </tr>
            <tr class="w3-row">
              <td class="w3-col s4 w3-padding-small"><label class="w3-text" style="padding-top:8px;display:inline-block"><b>Среда</b></label></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px"  name="test_time1_3" id="test_time1_3" class="w3-input w3-border w3-small" type="time"></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px"  name="test_time2_3" id="test_time2_3" class="w3-input w3-border w3-small" type="time"></td>
            </tr>
            <tr class="w3-row">
              <td class="w3-col s4 w3-padding-small"><label class="w3-text" style="padding-top:8px;display:inline-block"><b>Четверг</b></label></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px"  name="test_time1_4" id="test_time1_4" class="w3-input w3-border w3-small" type="time"></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px"  name="test_time2_4" id="test_time2_4" class="w3-input w3-border w3-small" type="time"></td>
            </tr>
            <tr class="w3-row">
              <td class="w3-col s4 w3-padding-small"><label class="w3-text" style="padding-top:8px;display:inline-block"><b>Пятница</b></label></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px"  name="test_time1_5" id="test_time1_5" class="w3-input w3-border w3-small" type="time"></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px"  name="test_time2_5" id="test_time2_5" class="w3-input w3-border w3-small" type="time"></td>
            </tr>
            <tr class="w3-row">
              <td class="w3-col s4 w3-padding-small"><label class="w3-text" style="padding-top:8px;display:inline-block"><b>Суббота</b></label></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px"  name="test_time1_6" id="test_time1_6" class="w3-input w3-border w3-small" type="time"></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px"  name="test_time2_6" id="test_time2_6" class="w3-input w3-border w3-small" type="time"></td>
            </tr>
            <tr class="w3-row">
              <td class="w3-col s4 w3-padding-small"><label class="w3-text" style="padding-top:8px;display:inline-block"><b>Воскресенье</b></label></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px"  name="test_time1_0" id="test_time1_0" class="w3-input w3-border w3-small" type="time"></td>
              <td class="w3-col s4 w3-padding-small"><input style="padding-left:0px"  name="test_time2_0" id="test_time2_0" class="w3-input w3-border w3-small" type="time"></td>
            </tr>
            </table><br>
            <button class="w3-btn w3-orange" style="margin-left:12px">Записать</button><br><p></p>
          </form><br>
        </div>
      </div>
      <br>
      <div class="w3-container w3-card-4 w3-padding-small">
        <button onclick="myFuncAccord('Accord2')" class="w3-btn w3-block" style="outline:none; box-shadow:0px 0px"><h3 class="w3-center">Запись параметров</h3></button>
        <div id="Accord2" class="w3-hide">
          <form action="/getvalue" method="post">
            <table class="w3-table w3-striped w3-bordered">
            <tr>
              <td><label class="w3-text" style="padding-top:8px;display:inline-block"><b>OTKR</b>
              <br><span class="w3-text w3-small">Уставка внешнего света, при котором открываются жалюзи</span></label></td>
              <td><input name="test_val1" id="test_val1" class="w3-input w3-border" type="number" min="1" max="1022" style="width:80px; margin-left:5px"></td>
            </tr>
            <tr>
              <td><label class="w3-text" style="padding-top:8px;display:inline-block"><b>ZAKR</b>
              <br><span class="w3-text w3-small">Уставка внешнего света, при котором закрываются жалюзи</span></label></td>
              <td><input name="test_val2" id="test_val2" class="w3-input w3-border" type="number" min="1" max="1022" style="width:80px; margin-left:5px"></td>
            </tr>
            <tr>
              <td><label class="w3-text" style="padding-top:8px;display:inline-block"><b>ZAKR_vnutr</b>
              <br><span class="w3-text w3-small">Уставка внутреннего света, при котором закрываются жалюзи</span></label></td>
              <td><input name="test_val3" id="test_val3" class="w3-input w3-border" type="number" min="1" max="1022" style="width:80px; margin-left:5px"></td>
            </tr>
            <tr>
              <td><label class="w3-text" style="padding-top:8px;display:inline-block"><b>ZAKR_vnesh</b>
              <br><span class="w3-text w3-small">Уставка внешнего света, при котором блокируется закрытие по внутреннему свету</span></label></td>
              <td><input name="test_val4" id="test_val4" class="w3-input w3-border" type="number" min="1" max="1022" style="width:80px; margin-left:5px"></td>
            </tr>
            <!--<tr>
              <td><label class="w3-text" style="padding-top:8px;display:inline-block"><b>ZAKR_vnesh_hist</b>
              <br><span class="w3-text w3-small">Гистерезис уставки внешнего освещение, при котором блокируется закрытие по внутреннему свету</span></label></td>
              <td><input name="test_val5" id="test_val5" class="w3-input w3-border" type="number" min="1" max="50" style="width:80px; margin-left:5px"></td>
            </tr>-->
            <tr>
              <td><label class="w3-text" style="padding-top:8px;display:inline-block"><b>rast</b>
              <br><span class="w3-text w3-small">Количество шагов двигателя (по умол. 42000)</span></label></td>
              <td><input name="test_rast" id="test_rast" class="w3-input w3-border" type="number" min="1" max="65000" style="width:80px; margin-left:5px"></td>
            </tr>
            <tr>
              <td><label class="w3-text" style="padding-top:8px;display:inline-block"><b>napr</b>
              <br><span class="w3-text w3-small">1 - Ожидается открытие <br> 2 - Ожидается закрытие</span></label></td>
              <td><input name="test_napr" id="test_napr" class="w3-input w3-border" type="number" min="1" max="2" style="width:80px; margin-left:5px"></td>
            </tr>
            </table><br> 
            <button class="w3-btn w3-orange" style="margin-left:12px">Записать</button><br><p></p>
          </form>
        </div>
      </div><br>
    </div>
  </div>
</div> 


</body>
</html>
)=====";


char webpage_desc[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<title>ESPServer</title>
<meta charset="utf-8" name="viewport" content="width=devise-width, initial-scale=1">
<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">
<head>
</head>
<body>
<script>
function w3Open() {
  document.getElementById("mySidebar").style.display = "block";
}
function w3Close() {
  document.getElementById("mySidebar").style.display = "none";
}
</script>

<div class="w3-sidebar w3-bar-block w3-collapse w3-card w3-animate-left" style="width:200px;" id="mySidebar">
  <button class="w3-bar-item w3-button w3-large w3-hide-large" onclick="w3Close()">Закрыть &times;</button>
  <a href="/" class="w3-bar-item w3-button">Главная</a>
  <a href="/description" class="w3-bar-item w3-button">Описание</a>
  <a href="/calibration" class="w3-bar-item w3-button">Калибровка</a>
</div>

<div class="w3-main" style="margin-left:200px">
  <div class="w3-row w3-khaki">
    <div class="w3-col s1">
      <button class="w3-button w3-xlarge w3-hide-large" style="margin-top:10px" onclick="w3Open()">&#9776;</button>
    </div>
    <div class="w3-container w3-rest" onclick="w3Close()">
      <h1>Описание</h1>
    </div>
  </div>
  <br>
  <div class="w3-row" onclick="w3Close()">
    <div class="w3-col m6">
      <div class="w3-container w3-padding-small">
        <!--<img src="https://i.ibb.co/FmRx1bh/Schematic-Wemos-zhaluzi-2021-04-03.jpg" alt="Schematic-Wemos-zhaluzi-2021-04-03"
        class="w3-image">-->
        <iframe src="https://drive.google.com/file/d/192uWkRCO53Lf4804DUi20RPX_Kd22GGl/preview"
        width="100%" height="230" frameborder="0"></iframe>
        <br>
        <img src="http://i2.wp.com/randomnerdtutorials.com/wp-content/uploads/2019/05/ESP8266-WeMos-D1-Mini-pinout-gpio-pin.png?quality=100&strip=all&ssl=1" 
        class="w3-image w3-padding" alt="Wemos D1 mini pinout">
        <br>
        <table class="w3-table w3-bordered">
          <tr>
            <th>Label</th>
            <th>GPIO</th>
            <th>Input</th>
            <th>Output</th>
            <th>Notes</th>
          </tr>
          <tr>
            <td>D0</td>
            <td>GPIO16</td>
            <td>no interrupt</td>
            <td>no PWM or I2C support</td>
            <td>HIGH at boot used to wake up from deep sleep</td>
          </tr>
          <tr>
            <td>D1</td>
            <td>GPIO5</td>
            <td>OK</td>
            <td>OK</td>
            <td>often used as SCL (I2C)</td>
          </tr>
        </table>
      </div>
    </div>
    <div class="w3-col m6">
      <div class="w3-container w3-padding">
        WeMos D1 mini — это плата, позволяющая управлять различными модулями вместо Arduino, 
        но в отличии от большинства плат Arduino, у платы WeMos D1 mini больший объем памяти программ и памяти ОЗУ, 
        она построена на базе 32 разрядного микроконтроллера с большей тактовой частотой и оснащена встроенным WiFi модулем, 
        который можно настроить как клиент (STA), точка доступа (AP), или клиент+точка доступа (STA+AP).
        <ul class="w3-ul" style="list-style-type:circle; margin-left:5px">
          <li>Микроконтроллер: ESP8266.</li>
          <li>Разрядность: 32 бит.</li>
          <li>Напряжение питания платы: 3,3 / 5,0 В.</li>
          <li>Беспроводной интерфейс: Wi-Fi 802.11 b/g/n 2,4 ГГц (STA/AP/STA+AP, WEP/TKIP/AES, WPA/WPA2).</li>
          <li>Поддерживаемые шины: SPI, I2C, I2S, 1-wire, UART, UART1, IR Remote Control.</li>
          <li>Цифровые выводы I/O: 11 (RX, TX, D0...D8) все выводы кроме D0 поддерживают INT (внешнее прерывание), ШИМ, I2C, 1-wire.</li>
          <li>Аналоговые входы: 1 (A0) 10-битный АЦП.</li>
          <li>Логические уровни выводов I/O: 3,3 В</li>
          <li>Максимальный ток на выводе I/O: 12 мА (для каждого вывода).</li>
          <li>Максимальное напряжение на входе A0: 3,2 В (между выводом A0 и GND)</li>
          <li>Flash-память: 4 МБ.</li>
          <li>RAM-память данных: 80 КБ.</li>
          <li>RAM-память инструкций: 32 КБ.</li>
          <li>Тактовая частота микроконтроллера: 80 МГц.</li>
          <li>Чип USB-UART преобразователя: CH340G.</li>
          <li>Рабочая температура: -40 ... +85 C.</li>
          <li>Габариты: 34,2x25,6 мм.</li>
          <li>Вес: 10 г.</li>
        </ul>
      </div>
    </div>
  </div><br><p></p>
</div>

</body>
</html>
)=====";

char webpage_cal[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<title>ESPServer</title>
<meta charset="utf-8" name="viewport" content="width=devise-width, initial-scale=1">
<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">
<head>
</head>
<body>
<script>
function w3Open() {
  document.getElementById("mySidebar").style.display = "block";
}
function w3Close() {
  document.getElementById("mySidebar").style.display = "none";
}
</script>

<div class="w3-sidebar w3-bar-block w3-collapse w3-card w3-animate-left" style="width:200px;" id="mySidebar">
  <button class="w3-bar-item w3-button w3-large w3-hide-large" onclick="w3Close()">Закрыть &times;</button>
  <a href="/" class="w3-bar-item w3-button">Главная</a>
  <a href="/description" class="w3-bar-item w3-button">Описание</a>
  <a href="/calibration" class="w3-bar-item w3-button">Калибровка</a>
</div>

<div class="w3-main" style="margin-left:200px">
  <div class="w3-row w3-khaki">
    <div class="w3-col s1">
      <button class="w3-button w3-xlarge w3-hide-large" style="margin-top:10px" onclick="w3Open()">&#9776;</button>
    </div>
    <div class="w3-container w3-rest" onclick="w3Close()">
      <h1>Калибровка</h1>
    </div>
  </div>
  <br>
  <div class="w3-row" onclick="w3Close()">
    <div class="w3-col m6">
      <div class="w3-container w3-padding-small">
        <div class="w3-container w3-card-4 w3-padding-small">
          <form action="/getcalibrate" method="post">
            <table class="w3-table">
            <tr>
              <td>
              <input class="w3-radio" type="radio" name="dvizh" value="zakr" checked>
              <label>Закрыть</label></p>
              <p>
              <input class="w3-radio" type="radio" name="dvizh" value="otkr">
              <label>Открыть</label></p>
              </td>
              <td><input name="test_rast1" id="test_rast1" class="w3-input w3-border" type="number" min="1" max="60000" style="width:80px; margin-left:5px"></td>
              <td><button class="w3-btn w3-orange">Записать</button><br></td>
            </tr>
            </table><br>
          </form>
        </div><br>
      </div>
    </div>
  </div><br><p></p>
</div>

</body>
</html>
)=====";

void eeWriteInt(int pos, int val) {
    byte* p = (byte*) &val;
    EEPROM.write(pos, *p);
    EEPROM.write(pos + 1, *(p + 1));
    EEPROM.write(pos + 2, *(p + 2));
    EEPROM.write(pos + 3, *(p + 3));
    EEPROM.commit();
}

int eeGetInt(int pos) {
  int val;
  byte* p = (byte*) &val;
  *p        = EEPROM.read(pos);
  *(p + 1)  = EEPROM.read(pos + 1);
  *(p + 2)  = EEPROM.read(pos + 2);
  *(p + 3)  = EEPROM.read(pos + 3);
  return val;
}

void eeWriteUint(int pos, unsigned int val) {
    byte* p = (byte*) &val;
    EEPROM.write(pos, *p);
    EEPROM.write(pos + 1, *(p + 1));
    EEPROM.write(pos + 2, *(p + 2));
    EEPROM.write(pos + 3, *(p + 3));
    EEPROM.commit();
}

unsigned int eeGetUint(int pos) {
  unsigned int val;
  byte* p = (byte*) &val;
  *p        = EEPROM.read(pos);
  *(p + 1)  = EEPROM.read(pos + 1);
  *(p + 2)  = EEPROM.read(pos + 2);
  *(p + 3)  = EEPROM.read(pos + 3);
  return val;
}

void otobr(void) {
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0, 0);            // Start at top-left corner
  display.print(delay_v);
  display.print(F(" "));
  display.print(delay_n);
  display.print(F("  "));
  display.print(svet);
  display.setCursor(60, 15);
  display.print(svet_vnutr);
  display.display();
}

int middle_of_5(int a, int b, int c, int d, int e) {
  int middle;
  //a----------------------------------------------------------
  if ((a <= b) && (a <= c) && (a <= d) && (a <= e)) {
    if ((b <= c) && (b <= d) && (b <= e)) {
      if ((c <= d) && (c <= e)) {
        middle = c;
      }
      if ((d <= c) && (d <= e)) {
        middle = d;
      }
      if ((e <= c) && (e <= d)) {
        middle = e;
      }
    } else if ((c <= b) && (c <= d) && (c <= e)) {
      if ((b <= d) && (b <= e)) {
        middle = b;
      }
      if ((d <= b) && (d <= e)) {
        middle = d;
      }
      if ((e <= b) && (e <= d)) {
        middle = e;
      }
    } else if ((d <= b) && (d <= c) && (d <= e)) {
      if ((b <= c) && (b <= e)) {
        middle = b;
      }
      if ((c <= b) && (c <= e)) {
        middle = c;
      }
      if ((e <= b) && (e <= c)) {
        middle = e;
      }
    } else {
      if ((b <= d) && (b <= c)) {
        middle = b;
      }
      if ((c <= d) && (c <= b)) {
        middle = c;
      }
      if ((d <= b) && (d <= c)) {
        middle = d;
      }
    }
  }
  //b----------------------------------------------------------
  if ((b <= a) && (b <= c) && (b <= d) && (b <= e)) {
    if ((a <= c) && (a <= d) && (a <= e)) {
      if ((c <= d) && (c <= e)) {
        middle = c;
      }
      if ((d <= c) && (d <= e)) {
        middle = d;
      }
      if ((e <= c) && (e <= d)) {
        middle = e;
      }
    } else if ((c <= a) && (c <= d) && (c <= e)) {
      if ((a <= d) && (a <= e)) {
        middle = a;
      }
      if ((d <= a) && (d <= e)) {
        middle = d;
      }
      if ((e <= a) && (e <= d)) {
        middle = e;
      }
    } else if ((d <= a) && (d <= c) && (d <= e)) {
      if ((a <= c) && (a <= e)) {
        middle = a;
      }
      if ((c <= a) && (c <= e)) {
        middle = c;
      }
      if ((e <= a) && (e <= c)) {
        middle = e;
      }
    } else {
      if ((a <= d) && (a <= c)) {
        middle = a;
      }
      if ((c <= d) && (c <= a)) {
        middle = c;
      }
      if ((d <= a) && (d <= c)) {
        middle = d;
      }
    }
  }
  //c----------------------------------------------------------
  if ((c <= b) && (c <= a) && (c <= d) && (c <= e)) {
    if ((b <= a) && (b <= d) && (b <= e)) {
      if ((a <= d) && (a <= e)) {
        middle = a;
      }
      if ((d <= a) && (d <= e)) {
        middle = d;
      }
      if ((e <= a) && (e <= d)) {
        middle = e;
      }
    } else if ((a <= b) && (a <= d) && (a <= e)) {
      if ((b <= d) && (b <= e)) {
        middle = b;
      }
      if ((d <= b) && (d <= e)) {
        middle = d;
      }
      if ((e <= b) && (e <= d)) {
        middle = e;
      }
    } else if ((d <= b) && (d <= a) && (d <= e)) {
      if ((b <= a) && (b <= e)) {
        middle = b;
      }
      if ((a <= b) && (a <= e)) {
        middle = a;
      }
      if ((e <= b) && (e <= a)) {
        middle = e;
      }
    } else {
      if ((b <= d) && (b <= a)) {
        middle = b;
      }
      if ((a <= d) && (a <= b)) {
        middle = a;
      }
      if ((d <= b) && (d <= a)) {
        middle = d;
      }
    }
  }
  //d----------------------------------------------------------
  if ((d <= b) && (d <= c) && (d <= a) && (d <= e)) {
    if ((b <= c) && (b <= a) && (b <= e)) {
      if ((c <= a) && (c <= e)) {
        middle = c;
      }
      if ((a <= c) && (a <= e)) {
        middle = a;
      }
      if ((e <= c) && (e <= a)) {
        middle = e;
      }
    } else if ((c <= b) && (c <= a) && (c <= e)) {
      if ((b <= a) && (b <= e)) {
        middle = b;
      }
      if ((a <= b) && (a <= e)) {
        middle = a;
      }
      if ((e <= b) && (e <= a)) {
        middle = e;
      }
    } else if ((a <= b) && (a <= c) && (a <= e)) {
      if ((b <= c) && (b <= e)) {
        middle = b;
      }
      if ((c <= b) && (c <= e)) {
        middle = c;
      }
      if ((e <= b) && (e <= c)) {
        middle = e;
      }
    } else {
      if ((b <= a) && (b <= c)) {
        middle = b;
      }
      if ((c <= a) && (c <= b)) {
        middle = c;
      }
      if ((a <= b) && (a <= c)) {
        middle = a;
      }
    }
  }
  //e----------------------------------------------------------
  if ((e <= b) && (e <= c) && (e <= d) && (e <= a)) {
    if ((b <= c) && (b <= d) && (b <= a)) {
      if ((c <= d) && (c <= a)) {
        middle = c;
      }
      if ((d <= c) && (d <= a)) {
        middle = d;
      }
      if ((a <= c) && (a <= d)) {
        middle = a;
      }
    } else if ((c <= b) && (c <= d) && (c <= a)) {
      if ((b <= d) && (b <= a)) {
        middle = b;
      }
      if ((d <= b) && (d <= a)) {
        middle = d;
      }
      if ((a <= b) && (a <= d)) {
        middle = a;
      }
    } else if ((d <= b) && (d <= c) && (d <= a)) {
      if ((b <= c) && (b <= a)) {
        middle = b;
      }
      if ((c <= b) && (c <= a)) {
        middle = c;
      }
      if ((a <= b) && (a <= c)) {
        middle = a;
      }
    } else {
      if ((b <= d) && (b <= c)) {
        middle = b;
      }
      if ((c <= d) && (c <= b)) {
        middle = c;
      }
      if ((d <= b) && (d <= c)) {
        middle = d;
      }
    }
  }

  return middle;
}

void updTime() {
  timeStr = String(hrs);
  timeStr += ":";
  timeStr += (mins < 10) ? "0" : "";
  timeStr += String(mins);
  timeStr += ":";
  timeStr += (secs < 10) ? "0" : "";
  timeStr += String(secs);
}

void getLEDState() {
  digitalWrite(D4,!digitalRead(D4));
  String led_state = digitalRead(D4) ? "OFF" : "ON";
  server.send(200,"text/plain",led_state);
}

void curtainsOpen() {
  client_open=1;
//  napr = 2;
  server.send(200,"text/plain","Открыты");
}

void curtainsClose() {
  client_close=1;
//  napr = 1;
  server.send(200,"text/plain","Закрыты");
}

void a0State_handle() {
  if ((millis() - t2) > 100) {
    t2 = millis();
    int val0 = analogRead(0);
    if (f_a0) {
        val0_1 = val0;
        f_a0=0;
      digitalWrite(D8, 1);
    } else {
        val0_2 = val0;
        f_a0=1;
      digitalWrite(D8, 0);
    }
    if (++index_val > 4) index_val = 0; // переключаем индекс с 0 до 4
    val_vnesh[index_val] = val0_1;
    val_vnutr[index_val] = val0_2;
    svet = middle_of_5(val_vnesh[0], val_vnesh[1], val_vnesh[2], val_vnesh[3], val_vnesh[4]);
    svet_vnutr = middle_of_5(val_vnutr[0], val_vnutr[1], val_vnutr[2], val_vnutr[3], val_vnutr[4]);
    otobr();
  }
}

void a0State() {
  String message = "{\n";
  message += "  \"val0_1\": "; message += val0_1; message += ",\n";
  message += "  \"val0_2\": "; message += val0_2; message += "\n";
  message += "}";
  server.send(200, "jsonText", message);
}

void timestate() {
//  server.send(200,"text/plain", timeStr);
  String message = "{\n";
  message += "  \"timeStr\": "; message += "\""; message += timeStr; message += "\""; message += ",\n";
  message += "  \"days\": "; message += days; message += "\n";
  message += "}";
  server.send(200, "jsonText", message);
}

void zhaluzistate() {
  String message = "{\n";
  message += "  \"zhaluzi_state\": "; message += "\""; message += f_sost_close; message += "\""; message += ",\n";
  message += "  \"napr\": "; message += "\""; message += napr; message += "\""; message += "\n";
  message += "}";
  server.send(200, "jsonText", message);
}

void gettimes() {
  for (int i = 0; i <= 6; i++) {
//    String str_option_days = server.arg("option_days");
//    int i = str_option_days.toInt();
//    Serial.println(str_option_days);
//    test_time1_1
//    test_time1_1
    char test_time1_serv[12];
    char test_time2_serv[12];
    
    sprintf(test_time1_serv,"test_time1_%d",i);
    String str_test_time1 = server.arg(test_time1_serv);
    
    sprintf(test_time2_serv,"test_time2_%d",i);
    String str_test_time2 = server.arg(test_time2_serv);
    
    time1_hrs[i] = str_test_time1.substring(0, 2).toInt();
    time1_mins[i] = str_test_time1.substring(3, 5).toInt();
    time2_hrs[i] = str_test_time2.substring(0, 2).toInt();
    time2_mins[i] = str_test_time2.substring(3, 5).toInt();
  
    int j = 21 + i*4;
    eeWriteInt(j, time1_hrs[i]);
    
    j = 49 + i*4;
    eeWriteInt(j, time1_mins[i]);
    
    j = 77 + i*4;
    eeWriteInt(j, time2_hrs[i]);
  
    j = 105 + i*4;
    eeWriteInt(j, time2_mins[i]);
//    int j = 21 + i*4;
//    eeWriteInt(j, time1_hrs[i]);
  }
  
//  eeWriteInt(21, time1_hrs);
//  eeWriteInt(25, time1_mins);
//  eeWriteInt(29, time2_hrs);
//  eeWriteInt(33, time2_mins);
  EEPROM.commit();

  server.sendHeader("Location", String("/"), true);
  server.send(302,"text/plain", "");
}

void getvalue() {
  String str_test_val1 = server.arg("test_val1");
  String str_test_val2 = server.arg("test_val2");
  String str_test_val3 = server.arg("test_val3");
  String str_test_val4 = server.arg("test_val4");
//  String str_test_val5 = server.arg("test_val5");
  String str_test_napr = server.arg("test_napr");
  String str_test_rast = server.arg("test_rast");
  OTKR = str_test_val1.toInt();
  ZAKR = str_test_val2.toInt();
  ZAKR_vnutr = str_test_val3.toInt();
  ZAKR_vnesh = str_test_val4.toInt();
  rast = str_test_rast.toInt();
  TIME_ZAKR = (rast/2) * 3;
//  ZAKR_vnesh_hist = str_test_val5.toInt();
//  napr = str_test_napr.toInt();
//  EEPROM.write(0, napr);
  eeWriteInt(1, OTKR);
  eeWriteInt(5, ZAKR);
  eeWriteInt(9, ZAKR_vnutr);
  eeWriteInt(13, ZAKR_vnesh);
  eeWriteUint(154, rast);
//  eeWriteInt(17, ZAKR_vnesh_hist);
  EEPROM.commit();

  server.sendHeader("Location", String("/"), true);
  server.send(302,"text/plain", "");
}

void getcalibrate() {
  f_calibrate=1;
  String str_dvizh = server.arg("dvizh");
  String str_test_rast1 = server.arg("test_rast1");
  if (str_dvizh == "zakr") {
    napr_calibrate = 2;
  } else if (str_dvizh == "otkr") {
    napr_calibrate = 1;
  }
  
  rast_calibrate = str_test_rast1.toInt();
  
  server.sendHeader("Location", String("/calibration"), true);
  server.send(302,"text/plain", "");
}

void sendvalues() {
  char test_time1_json1[20];
  char test_time2_json2[20];
  
  String message = "{\n";
  message += "  \"test_val1\": "; message += OTKR; message += ",\n";
  message += "  \"test_val2\": "; message += ZAKR; message += ",\n";
  message += "  \"test_val3\": "; message += ZAKR_vnutr; message += ",\n";
  message += "  \"test_val4\": "; message += ZAKR_vnesh; message += ",\n";
  message += "  \"test_val5\": "; message += ZAKR_vnesh_hist; message += ",\n";
  message += "  \"test_rast\": "; message += rast; message += ",\n";
  message += "  \"test_napr\": "; message += napr; message += ",\n";

  for (int i = 0; i <= 6; i++) {
    String time1_str = "";
    if (time1_hrs[i]<10) time1_str += "0";
    time1_str += time1_hrs[i];
    time1_str += ":";
    if (time1_mins[i]<10) time1_str += "0";
    time1_str += time1_mins[i];
    sprintf(test_time1_json1,"  \"test_time1_%d\": ",i);
    
    message += test_time1_json1;
    message += "\""; message += time1_str; message += "\""; message += ",\n";

    String time2_str = "";
    if (time2_hrs[i]<10) time2_str += "0";
    time2_str += time2_hrs[i];
    time2_str += ":";
    if (time2_mins[i]<10) time2_str += "0";
    time2_str += time2_mins[i];
    sprintf(test_time2_json2,"  \"test_time2_%d\": ",i);
    
    message += test_time2_json2;
    message += "\""; message += time2_str; message += "\""; message += ",\n";
  }
  message += "  \"test_last\": "; message += "\""; message += "empty"; message += "\""; message += "\n";
  message += "}";
  server.send(200, "jsonText", message);
}

byte minuteCounter = 0;

void timeTick() {
  if (ESP_MODE == 1) {
    if (timeTimer.isReady()) {  // каждую секунду
      secs++;
      if (secs == 60) {
        secs = 0;
        mins++;
        minuteCounter++;
        updTime();
      }
      if (mins == 60) {
        mins = 0;
        hrs++;
        if (hrs == 24) {
          hrs = 0;
          days++;
          if (days > 6) days = 0;
        }
        updTime();
      }

      if ( ((minuteCounter > 30) || (time_init && (secs==20))) && (WiFi.status() == WL_CONNECTED) )  {    // синхронизация каждые 30 минут
        minuteCounter = 0;
        if (timeClient.update()) {
          if (time_init) updTime();
          time_init = 0;
          hrs = timeClient.getHours();
          mins = timeClient.getMinutes();
          secs = timeClient.getSeconds();
          days = timeClient.getDay();
        }
      }
    }
  }
}

void zhaluzi_handle() {
  zhaluzi_open=0;
  zhaluzi_close=0;
  
  if ((svet_vnutr < ZAKR_vnutr) && (svet > ZAKR_vnesh)) {
    f_svet_vnutr = 1;
  } else {
    f_svet_vnutr = 0;
  }

  if ( (((hrs == time1_hrs[days]) && (mins >= time1_mins[days])) || (hrs > time1_hrs[days])) 
  && ( ((hrs == time2_hrs[days]) && (mins <= time2_mins[days])) || (hrs < time2_hrs[days]) ) ) {
    open_time1=1;
  } else {
    open_time1=0;
  }
  if (time_init) open_time1=1;
  
  if ( (hrs == time2_hrs[days]) && (mins >= time2_mins[days]) || (hrs > time2_hrs[days]) ) {
    if (!time_init) close_time2=1;
  } else {
    close_time2=0;
  }

  if ((((svet < OTKR) && !f_svet_vnutr) && open_time1) && (!close_time2) && (!dvizh) && (napr == 1) && ((millis() - t_zakr) > TIME_ZAKR)) {
    delay_v ++;
    if (delay_v > delay_ust) {
      dvizh = 1;
      delay_v = 0;
    }
  } else {
    delay_v = 0;
  }
  if (((svet < OTKR) || open_time1) && dvizh) {
    t_zakr = millis();
    napr = 2;
    EEPROM.write(0, napr);
    EEPROM.commit();
    zhaluzi_open=1;
    dvizh = 0;
  }
  if ((((svet > ZAKR) || f_svet_vnutr) || close_time2) && (!dvizh) && (napr == 2) && ((millis() - t_zakr) > TIME_ZAKR)) {
    delay_n ++;
    if (delay_n > delay_ust) {
      dvizh = 1;
      delay_n = 0;
    }
  } else {
    delay_n = 0;
  }
  if ((((svet > ZAKR) || f_svet_vnutr) || close_time2) && dvizh) {
    t_zakr = millis();
    napr = 1;
    EEPROM.write(0, napr);
    EEPROM.commit();
    zhaluzi_close=1;
    dvizh = 0;
  }
  
  if (client_open) {
    zhaluzi_open=1;
    client_open=0;
  }
  if (client_close) {
    zhaluzi_close=1;
    client_close=0;
  }
  
  if (f_calibrate) {
    if (napr_calibrate == 1) {
      stepper1.enableOutputs();
      stepper1.move(((-1)*rast_calibrate));
    }
    if (napr_calibrate == 2) {
      stepper1.enableOutputs();
      stepper1.move(rast_calibrate);
    }
    f_calibrate=0;
  }

  if (zhaluzi_open && f_sost_close) {
//    digitalWrite(D5,HIGH);
    stepper1.enableOutputs();
    stepper1.move(((-1)*rast));
    f_sost_open=1;
    f_sost_close=0;
    EEPROM.write(150, f_sost_close);
    EEPROM.write(152, f_sost_open);
    EEPROM.commit();

    zhaluzi_open=0;
  }

  if (zhaluzi_close && f_sost_open) {
//    digitalWrite(D6,HIGH);
    stepper1.enableOutputs();
    stepper1.move(rast);
    f_sost_close=1;
    f_sost_open=0;
    EEPROM.write(150, f_sost_close);
    EEPROM.write(152, f_sost_open);
    EEPROM.commit();

    zhaluzi_close=0;
  }

}

void setup() {
  stepper1.setMaxSpeed(1000.0);
  stepper1.setAcceleration(1000.0);
  stepper1.setSpeed(400);
  
  Wire.begin(sdaPin, sclPin);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
//  WiFi.waitForConnectResult();
//  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
//    Serial.println("Connection Failed! Rebooting...");
//    delay(1000);
//    ESP.restart();
//  }
  
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Start the server
  server.on("/",[](){
    if (!server.authenticate("timur","01102018")) { //авторизация
      return server.requestAuthentication();
    }
    server.send_P(200,"text/html",webpage);
  });
  server.on("/description",[](){
    if (!server.authenticate("timur","01102018")) { //авторизация
      return server.requestAuthentication();
    }
    server.send_P(200,"text/html",webpage_desc);
  });
  server.on("/calibration",[](){
    if (!server.authenticate("timur","01102018")) { //авторизация
      return server.requestAuthentication();
    }
    server.send_P(200,"text/html",webpage_cal);
  });
  server.on("/ledstate", getLEDState);
  server.on("/curtainsOpen", curtainsOpen);
  server.on("/curtainsClose", curtainsClose);
  server.on("/a0state", a0State);
  server.on("/timestate", timestate);
  server.on("/getvalue", getvalue);
  server.on("/gettimes", gettimes);
  server.on("/sendvalues", sendvalues);
  server.on("/zhaluzistate", zhaluzistate);
  server.on("/getcalibrate", getcalibrate);
//  server.begin();
  server.begin(10210);
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH);
  pinMode(D8, OUTPUT);
//  pinMode(D5, OUTPUT); //тест зеленый
//  pinMode(D6, OUTPUT); //тест красный

  EEPROM.begin(512);
  if (eeGetInt(INIT_ADDR) != INIT_KEY) {
    eeWriteInt(INIT_ADDR, INIT_KEY);
    EEPROM.write(0, 1); //napr
    EEPROM.write(150, 1); //f_sost_close
    EEPROM.write(152, 0); //f_sost_open
    eeWriteUint(154, rast); //rast
    eeWriteInt(1, OTKR);
    eeWriteInt(5, ZAKR);
    eeWriteInt(9, ZAKR_vnutr);
    eeWriteInt(13, ZAKR_vnesh);
    eeWriteInt(17, ZAKR_vnesh_hist);
    for (int i = 0; i <= 6; i++) {
      int j = 21 + i*4;
      eeWriteInt(j, time1_hrs[i]);
    }
    for (int i = 0; i <= 6; i++) {
      int j = 49 + i*4;
      eeWriteInt(j, time1_mins[i]);
    }
    for (int i = 0; i <= 6; i++) {
      int j = 77 + i*4;
      eeWriteInt(j, time2_hrs[i]);
    }
    for (int i = 0; i <= 6; i++) {
      int j = 105 + i*4;
      eeWriteInt(j, time2_mins[i]);
    }
//    eeWriteInt(21, time1_hrs);
//    eeWriteInt(25, time1_mins);
//    eeWriteInt(29, time2_hrs);
//    eeWriteInt(33, time2_mins);
    EEPROM.commit();
  }
  napr = EEPROM.read(0);
  f_sost_close = EEPROM.read(150);
  f_sost_open = EEPROM.read(152);
  rast = eeGetUint(154);
  TIME_ZAKR = (rast/2) * 3;
  OTKR = eeGetInt(1);
  ZAKR = eeGetInt(5);
  ZAKR_vnutr = eeGetInt(9);
  ZAKR_vnesh = eeGetInt(13);
  ZAKR_vnesh_hist = eeGetInt(17);

  for (int i = 0; i <= 6; i++) {
    int j = 21 + i*4;
    time1_hrs[i] = eeGetInt(j);
  }
  for (int i = 0; i <= 6; i++) {
    int j = 49 + i*4;
    time1_mins[i] = eeGetInt(j);
  }
  for (int i = 0; i <= 6; i++) {
    int j = 77 + i*4;
    time2_hrs[i] = eeGetInt(j);
  }
  for (int i = 0; i <= 6; i++) {
    int j = 105 + i*4;
    time2_mins[i] = eeGetInt(j);
  }
//  time1_hrs = eeGetInt(21);
//  time1_mins = eeGetInt(25);
//  time2_hrs = eeGetInt(29);
//  time2_mins = eeGetInt(33);

  timeClient.begin();
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  timeTick();
  a0State_handle();
  if ((millis()-t3)>100) {
    t3=millis();
    zhaluzi_handle();
  }
  if ((millis()-t_conn)>5000) {
    t_conn=millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Connection Failed! Reconnecting...");
      delay(1000);
      WiFi.reconnect();
      f_ip=1;
    } else if ( (WiFi.status() == WL_CONNECTED) && f_ip ) {
      Serial.println("Ready");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      f_ip=0;
    }
  }
  stepper1.run();
  if (stepper1.currentPosition() == stepper1.targetPosition()) {
  
    stepper1.disableOutputs();
  }
}
