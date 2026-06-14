#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "FS.h"
#include "SD_MMC.h"
#include "time.h"
#include "secrets.h"

// ==========================================
// 1. Wi-Fi 및 NTP 설정
// ==========================================
// credentials moved to secrets.h

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 32400; // KST (UTC+9)
const int   daylightOffset_sec = 0;

WebServer server(80);

// ==========================================
// 2. 핀맵 설정 (Freenove ESP32-S3 Cam N16R8 기준)
// ==========================================
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y9_GPIO_NUM      16
#define Y8_GPIO_NUM      17
#define Y7_GPIO_NUM      18
#define Y6_GPIO_NUM      12
#define Y5_GPIO_NUM      10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM      11
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM    13

// SD Card Pins
#define SD_MMC_CMD  38
#define SD_MMC_CLK  39
#define SD_MMC_D0   40

// MJPEG 스트리밍을 위한 HTTP Header 구조 정의
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// ==========================================
// 3. 시간 및 파일 관리 함수
// ==========================================
String getLocalTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "unknown_time";
  }
  char timeString[20];
  strftime(timeString, sizeof(timeString), "%Y%m%d_%H%M%S", &timeinfo);
  return String(timeString);
}

// ==========================================
// 4. 웹서버 핸들러
// ==========================================
void handleStream() {
  char tempBuf[64];
  WiFiClient client = server.client();

  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: ");
  client.print(_STREAM_CONTENT_TYPE);
  client.print("\r\n\r\n");

  Serial.println("[WEB] 스트리밍 클라이언트가 연결되었습니다.");

  while (client.connected()) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("[ERROR] 카메라 프레임 버퍼를 가져오지 못했습니다.");
      delay(100);
      continue;
    }

    if (client.print(_STREAM_BOUNDARY) == 0) { esp_camera_fb_return(fb); break; }
    sprintf(tempBuf, _STREAM_PART, fb->len);
    if (client.print(tempBuf) == 0) { esp_camera_fb_return(fb); break; }
    if (client.write(fb->buf, fb->len) == 0) { esp_camera_fb_return(fb); break; }

    esp_camera_fb_return(fb);
    delay(1); 
  }
  Serial.println("[WEB] 스트리밍 클라이언트 연결이 종료되었습니다.");
}


// ... (existing code: 1. Wi-Fi 및 NTP 설정)

// ... (existing code: 2. 핀맵 설정)

// ... (existing code: 3. 시간 및 파일 관리 함수)

// ... (existing code: 4. 웹서버 핸들러)


// 신규: 이미지 파일 보기
void handleViewFile() {
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }
  String path = server.arg("path");
  File file = SD_MMC.open(path);
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  
  server.streamFile(file, "image/jpeg");
  file.close();
}

// SD 카드에 사진 저장 로직 (공통)
bool takePhoto() {
  // 1. 버퍼 플러시: 카메라 내부의 오래된 프레임을 제거하기 위해 1회 캡처 후 반환
  camera_fb_t * fb = esp_camera_fb_get();
  if (fb) {
    esp_camera_fb_return(fb);
  }
  delay(300); // 100ms에서 300ms로 증가시켜 센서 안정화 및 다음 프레임 확보 시간 충분히 확보

  // 2. 실제 사진 촬영
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[ERROR] 카메라 캡처 실패");
    return false;
  }

  String fileName = "/photo_" + getLocalTimestamp() + ".jpg";
  File file = SD_MMC.open(fileName, FILE_WRITE);
  
  if (!file) {
    Serial.println("[ERROR] SD 카드 쓰기 실패");
    esp_camera_fb_return(fb);
    return false;
  }

  file.write(fb->buf, fb->len);
  file.close();
  esp_camera_fb_return(fb);
  
  Serial.println("[SD] 사진 저장 완료: " + fileName);
  return true;
}

void handleTakePhoto() {
  if (takePhoto()) {
    server.sendHeader("Location", "/gallery");
    server.send(303); // 갤러리 페이지로 리다이렉트
  } else {
    server.send(500, "text/plain", "사진 촬영 실패");
  }
}


void handleDeleteFile() {
  if (!server.hasArg("path")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }
  String path = server.arg("path");
  if (SD_MMC.remove(path)) {
    Serial.println("[SD] 파일 삭제 성공: " + path);
  } else {
    Serial.println("[ERROR] 파일 삭제 실패: " + path);
  }
  server.sendHeader("Location", "/gallery");
  server.send(303);
}

void handleDeleteAllFiles() {
  File root = SD_MMC.open("/");
  if (root) {
    File file = root.openNextFile();
    while (file) {
      String fileName = String(file.name());
      if (fileName.startsWith("photo_") && fileName.endsWith(".jpg")) {
        SD_MMC.remove("/" + fileName);
        Serial.println("[SD] 파일 삭제: " + fileName);
      }
      file = root.openNextFile();
    }
    file.close();
  }
  root.close();
  server.sendHeader("Location", "/gallery");
  server.send(303);
}

String getMenu() {
  return "<nav style='background:#333; padding:10px; text-align:center;'>"
         "<a href='/' style='color:white; margin:0 15px; text-decoration:none;'>[라이브 스트림]</a>"
         "<a href='/gallery' style='color:white; margin:0 15px; text-decoration:none;'>[사진 갤러리]</a>"
         "</nav>";
}

void handleStreamView() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Live Stream</title>";
  html += "<style>body{font-family:sans-serif; text-align:center; margin:0; padding:0; background:#f4f6f9;} ";
  html += "#stream-container{width:640px; height:480px; margin:20px auto; border:8px solid #fff; border-radius:4px; box-shadow:0 4px 8px rgba(0,0,0,0.1); display:flex; align-items:center; justify-content:center; background:#eee;} ";
  html += "#stream{max-width:100%; height:auto; display:none;}</style>";
  html += "<script>"
          "function startStream(){document.getElementById('stream').src='/stream'; document.getElementById('stream').style.display='block'; document.getElementById('placeholder').style.display='none';}"
          "function stopStream(){document.getElementById('stream').src=''; document.getElementById('stream').style.display='none'; document.getElementById('placeholder').style.display='block';}"
          "</script></head><body>";
  html += getMenu();
  html += "<h1>실시간 스트리밍</h1>";
  html += "<div id='stream-container'><img id='stream' src='' /><div id='placeholder'>스트리밍이 대기 중입니다.</div></div><br>";
  html += "<button onclick='startStream()' style='padding:10px 20px; background:#007bff; color:white; border:none; border-radius:5px; cursor:pointer; margin:5px;'>스트리밍 시작</button>";
  html += "<button onclick='stopStream()' style='padding:10px 20px; background:#dc3545; color:white; border:none; border-radius:5px; cursor:pointer; margin:5px;'>스트리밍 종료</button><br><br>";
  html += "</body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

void handleGalleryView() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Gallery</title>";
  html += "<style>body{font-family:sans-serif; text-align:center; margin:0; padding:0; background:#f4f6f9;} ";
  html += ".gallery{margin-top:20px; text-align:left; display:inline-block;} ";
  html += "ul{list-style-type:none; padding:0;} li{margin:5px;} ";
  html += ".del-btn{color:red; margin-left:10px; text-decoration:none;} ";
  html += ".del-all-btn{color:white; background:red; padding:5px 10px; text-decoration:none; border-radius:3px;}</style>";
  html += "<script>function openPopup(url){window.open(url,'_blank','width=640,height=480');}</script>";
  html += "</head><body>";
  html += getMenu();
  html += "<h1>저장된 사진 목록</h1>";
  html += "<p><a href='/take_photo' style='padding:15px 30px; background:#28a745; color:white; text-decoration:none; border-radius:5px; font-size:20px; font-weight:bold;'>사진찍기</a></p>";
  html += " <a href='/delete_all' class='del-all-btn' onclick='return confirm(\"정말 모든 사진을 삭제하시겠습니까?\")'>전체 삭제</a>";
  html += "<div class='gallery'><ul>";
  
  File root = SD_MMC.open("/");
  if (root) {
    File file = root.openNextFile();
    while (file) {
      String fileName = String(file.name());
      if (fileName.startsWith("photo_") && fileName.endsWith(".jpg")) {
        String filePath = "/" + fileName;
        html += "<li><a href='#' onclick='openPopup(\"/view?path=" + filePath + "\")'>" + fileName + "</a>";
        html += "<a href='/delete?path=" + filePath + "' class='del-btn' onclick='return confirm(\"정말 삭제하시겠습니까?\")'>[삭제]</a></li>";
      }
      file = root.openNextFile();
    }
    file.close();
  }
  root.close();
  html += "</ul></div></body></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

// ... (existing initCamera, setup, loop)
// setup() 함수 내에 server.on 추가
//   server.on("/list_files", handleListFiles);
//   server.on("/view", handleViewFile);

// ==========================================
// 5. 초기화 함수들
// ==========================================
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 1; // 1로 변경하여 버퍼 중복 방지
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 14;
    config.fb_count = 1;
  }


  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[ERROR] 카메라 초기화 실패: 0x%x\n", err);
    return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) { delay(10); }

  Serial.println("\n==========================================");
  Serial.println("  Freenove ESP32-S3 CAM WebServer Start   ");
  Serial.println("==========================================");

  // 1. 카메라 초기화
  if (initCamera()) {
    Serial.println("[OK] 카메라 초기화 완료");
  } else {
    Serial.println("[FAIL] 카메라 초기화 실패.");
    while (true) { delay(1000); }
  }

  // 2. Wi-Fi 연결 및 NTP 초기화
  Serial.printf("[WIFI] %s 에 연결 중...", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[OK] Wi-Fi 연결 성공!");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("[INFO] NTP 시간 동기화 완료");

  // 3. SD 카드 초기화 및 최초 사진 촬영
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!SD_MMC.begin("/sdcard", true)) {
    Serial.println("[ERROR] SD 카드 마운트 실패");
  } else {
    Serial.println("[OK] SD 카드 마운트 성공");
    takePhoto(); // 최초 사진 촬영
  }

  // 4. 웹서버 라우팅 매핑
  server.on("/", handleStreamView);
  server.on("/gallery", handleGalleryView);
  server.on("/stream", handleStream);
  server.on("/take_photo", handleTakePhoto);
  server.on("/view", handleViewFile);
  server.on("/delete", handleDeleteFile);
  server.on("/delete_all", handleDeleteAllFiles);

  // 5. 웹서버 기동
  server.begin();
  Serial.println("[OK] HTTP 웹서버가 가동되었습니다. 포트: 80");
  Serial.println("==========================================");
}

void loop() {
  server.handleClient();
  delay(1); 
}