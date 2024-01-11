#include <M5StickCPlus.h>
#include "UNIT_8ENCODER.h"
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

UNIT_8ENCODER sensor;

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

const char* ssid = "Brainwave Generator";
const char* password = NULL;  // Open network (no password)

bool isPlaying = false;
float lastFreq1 = 0.0;
float lastFreq2 = 0.0;
float lastPulseRate = 0.0;

// Store the current frequencies and pulse rate
float currentFreq1 = 0.0;
float currentFreq2 = 0.0;
float currentPulseRate = 0.0;

// Define a variable to store the encoder zero values
int encoderZeroValue1 = 0;
int encoderZeroValue2 = 0;

// Function prototypes
float calculateFrequency(int baseValue, int fineTuneValue, int multiplierValue);
float calculatePulseRate(int baseValue, int fineAdjustment);
void updateDisplay(float f1, float f2, float pulse);
void startSound(float frequency1, float frequency2, float pulseRate);
void stopSound();
void zeroEncoders();

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    // Handle WebSocket event
}

void setup() {
    M5.begin();
    Wire.begin(32, 33);
    sensor.begin();

    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setTextSize(2);

    WiFi.softAP(ssid, password);
    server.on("/", HTTP_GET, handleRoot);
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    server.handleClient();
    webSocket.loop();

    // Read values from the 8Encoder
    int rawEncoderValue1_0 = sensor.getEncoderValue(0);
    int rawEncoderValue1_1 = sensor.getEncoderValue(1);
    int rawEncoderValue1_2 = sensor.getEncoderValue(2);

    int rawEncoderValue2_4 = sensor.getEncoderValue(4);
    int rawEncoderValue2_5 = sensor.getEncoderValue(5);
    int rawEncoderValue2_6 = sensor.getEncoderValue(6);

    float freq1 = calculateFrequency(adjustedEncoderValue1(rawEncoderValue1_0), 
                                      adjustedEncoderValue1(rawEncoderValue1_1), 
                                      adjustedEncoderValue1(rawEncoderValue1_2));
    float freq2 = calculateFrequency(adjustedEncoderValue2(rawEncoderValue2_4), 
                                      adjustedEncoderValue2(rawEncoderValue2_5), 
                                      adjustedEncoderValue2(rawEncoderValue2_6));

    float pulseRate = calculatePulseRate(sensor.getEncoderValue(3), sensor.getEncoderValue(7));

    // Check if the values have changed
    if (freq1 != lastFreq1 || freq2 != lastFreq2 || pulseRate != lastPulseRate) {
        // Update the display only when there's a change
        updateDisplay(freq1, freq2, pulseRate);

        // Check if isPlaying and send WebSocket message
        if (isPlaying) {
            String message = "play," + String(freq1, 2) + "," + String(freq2, 2) + "," + String(pulseRate, 2);
            webSocket.broadcastTXT(message);
        }

        // Update last values for the next comparison
        lastFreq1 = freq1;
        lastFreq2 = freq2;
        lastPulseRate = pulseRate;
    }

    // Check if button A is pressed to toggle the playing state
    if (M5.BtnA.wasPressed()) {
        isPlaying = !isPlaying;
        String message = String(isPlaying ? "play" : "stop") + "," + String(freq1, 2) + "," + String(freq2, 2) + "," + String(pulseRate, 2);
        webSocket.broadcastTXT(message);
    }

    M5.update();
}

float calculateFrequency(int baseValue, int fineTuneValue, int multiplierValue) {
    // Ensure non-negative values
    if (baseValue < 0) baseValue = 0;
    if (fineTuneValue < 0) fineTuneValue = 0;
    if (multiplierValue < 0) multiplierValue = 0;

    float frequency = 20.0 + baseValue * 2.0;
    frequency += fineTuneValue * 0.1;
    frequency *= (1 + multiplierValue);
    return frequency;
}

float calculatePulseRate(int baseValue, int fineAdjustment) {
    // Ensure non-negative values
    if (baseValue < 0) baseValue = 0;
    if (fineAdjustment < 0) fineAdjustment = 0;

    float pulseRate = baseValue * 1.0;
    pulseRate += fineAdjustment * 0.1;
    return pulseRate;
}

void updateDisplay(float f1, float f2, float pulse) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(1, 10);
    M5.Lcd.printf("Freq 1: %.2f Hz\n", f1);
    M5.Lcd.printf("Freq 2: %.2f Hz\n", f2);
    M5.Lcd.printf("Pulse: %.2f Hz", pulse);
}

void startSound(float frequency1, float frequency2, float pulseRate) {
    stopSound(); // Stop any existing sound
    // Rest of your sound generation code here...
    // Update current values
    currentFreq1 = frequency1;
    currentFreq2 = frequency2;
    currentPulseRate = pulseRate;
}

void stopSound() {
    // Stop the sound generation and update current values
    // Rest of your stop sound code here...
    currentFreq1 = 0.0;
    currentFreq2 = 0.0;
    currentPulseRate = 0.0;
}

void zeroEncoders() {
    encoderZeroValue1 = sensor.getEncoderValue(0);
    encoderZeroValue2 = sensor.getEncoderValue(4);
}

int adjustedEncoderValue1(int rawValue) {
    return rawValue - encoderZeroValue1;
}

int adjustedEncoderValue2(int rawValue) {
    return rawValue - encoderZeroValue2;
}

void handleRoot() {
    String html = 
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "<title>Brainwave Generator</title>"
    "<style>"
    "  body { text-align: center; font-family: Arial, sans-serif; margin-top: 50px; }"
    "  canvas {"
    "    border:1px solid #000;"
    "    margin-top: 10px;"
    "  }"
    "</style>"
    "<script>"
    "var audioCtx = new (window.AudioContext || window.webkitAudioContext)();"
    "var oscillator1, oscillator2, gainNode, lfo;"
    "var analyser = audioCtx.createAnalyser();"
    "var canvas, ctx;"
    "var originalFreq1, originalFreq2, maxVariance = 10, changeInterval = 5000;"
    "var isPlaying = false;"

    "function startSound(frequency1, frequency2, pulseRate) {"
    "  stopSound();"
    "  oscillator1 = audioCtx.createOscillator();"
    "  oscillator2 = audioCtx.createOscillator();"
    "  gainNode = audioCtx.createGain();"
    "  lfo = audioCtx.createOscillator();"
    "  oscillator1.type = 'sine';"
    "  oscillator2.type = 'sine';"
    "  originalFreq1 = frequency1;"
    "  originalFreq2 = frequency2;"
    "  oscillator1.frequency.setValueAtTime(frequency1, audioCtx.currentTime);"
    "  oscillator2.frequency.setValueAtTime(frequency2, audioCtx.currentTime);"
    "  lfo.frequency.setValueAtTime(pulseRate, audioCtx.currentTime);"
    "  lfo.connect(gainNode.gain);"
    "  oscillator1.connect(gainNode);"
    "  oscillator2.connect(gainNode);"
    "  gainNode.connect(analyser);"
    "  analyser.connect(audioCtx.destination);"
    "  oscillator1.start();"
    "  oscillator2.start();"
    "  lfo.start();"
    "  isPlaying = true;"

    // Periodically modify the frequencies slightly
    "  setInterval(function() {"
    "    if(isPlaying) {"
    "      oscillator1.frequency.setValueAtTime("
    "        originalFreq1 + (Math.random() * 2 * maxVariance - maxVariance),"
    "        audioCtx.currentTime"
    "      );"
    "      oscillator2.frequency.setValueAtTime("
    "        originalFreq2 + (Math.random() * 2 * maxVariance - maxVariance),"
    "        audioCtx.currentTime"
    "      );"
    "    }"
    "  }, changeInterval);"

    "  draw();"
    "}"

    "function draw() {"
    "  if(!isPlaying) return;"
    "  requestAnimationFrame(draw);"
    "  var bufferLength = analyser.frequencyBinCount;"
    "  var dataArray = new Uint8Array(bufferLength);"
    "  analyser.getByteTimeDomainData(dataArray);"
    "  ctx.fillStyle = 'rgb(200, 200, 200)';"
    "  ctx.fillRect(0, 0, canvas.width, canvas.height);"
    "  ctx.lineWidth = 2;"
    "  ctx.strokeStyle = 'rgb(0, 0, 0)';"
    "  ctx.beginPath();"
    "  var sliceWidth = canvas.width * 1.0 / bufferLength;"
    "  var x = 0;"
    "  for(var i = 0; i < bufferLength; i++) {"
    "    var v = dataArray[i] / 128.0;"
    "    var y = v * canvas.height/2;"
    "    if(i === 0) {"
    "      ctx.moveTo(x, y);"
    "    } else {"
    "      ctx.lineTo(x, y);"
    "    }"
    "    x += sliceWidth;"
    "  }"
    "  ctx.lineTo(canvas.width, canvas.height/2);"
    "  ctx.stroke();"
    "}"

    "function stopSound() {"
    "  if (oscillator1) {"
    "    oscillator1.stop();"
    "    oscillator1.disconnect();"
    "  }"
    "  if (oscillator2) {"
    "    oscillator2.stop();"
    "    oscillator2.disconnect();"
    "  }"
    "  if (lfo) {"
    "    lfo.stop();"
    "    lfo.disconnect();"
    "  }"
    "  if (gainNode) {"
    "    gainNode.disconnect();"
    "  }"
    "  isPlaying = false;"
    "}"

    "function togglePlay() {"
    "  if(isPlaying) {"
    "    stopSound();"
    "    isPlaying = false;"
    "  } else {"
    "    startSound(parseFloat(values[0]), parseFloat(values[1]), parseFloat(values[2]));"
    "    isPlaying = true;"
    "  }"
    "}"

    "document.addEventListener('DOMContentLoaded', function() {"
    "  canvas = document.getElementById('oscilloscope');"
    "  ctx = canvas.getContext('2d');"
    "  document.getElementById('playButton').click();"
    "});"

    "var ws = new WebSocket('ws://' + window.location.hostname + ':81/');"
    "ws.onmessage = function(event) {"
    "  var parts = event.data.split(',');"
    "  if(parts[0] === 'play') {"
    "    startSound(parseFloat(parts[1]), parseFloat(parts[2]), parseFloat(parts[3]));"
    "  } else if(parts[0] === 'stop') {"
    "    stopSound();"
    "  }"
    "};"
    "</script>"
    "</head>"
    "<body>"
    "<h1>Brainwave Generator</h1>"
    "<canvas id='oscilloscope' width='640' height='480'></canvas>"
    "<hr/>"
    "<button id='playButton' onclick='togglePlay()'>Play</button>"
    "<p>Adjust the frequencies and pulse on the device.</p>"
    "</body>"
    "</html>";

    server.send(200, "text/html", html);
}
