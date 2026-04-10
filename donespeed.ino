#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>

/**
 * ROVER SLAM PRO - MOBILE OPTIMIZED WITH SPEED CONTROL
 * Pins Used:
 * Ultrasonic: Trig=5, Echo=18
 * Motors: ENA=32, IN1=25, IN2=26, ENB=33, IN3=27, IN4=14
 * I2C: MPU6050 (SDA=4, SCL=15), QMC5883L (SDA=21, SCL=22)
 */

// ========= I2C BUSES =========
TwoWire I2C_MPU = TwoWire(0);
TwoWire I2C_QMC = TwoWire(1);

#define MPU_ADDR 0x68
#define QMC_ADDR 0x0D

// ========= PINS =========
const int TRIG = 5;
const int ECHO = 18;
const int ENA = 32;
const int IN1 = 25;
const int IN2 = 26;
const int ENB = 33;
const int IN3 = 27;
const int IN4 = 14;

// ========= CORE VARIABLES =========
float x = 0, y = 0;
float headingDeg = 0;
float distanceCM = 0;
bool surveying = false;
bool obstacleDetected = false;
int speedVal = 190; // Default speed
unsigned long lastTime = 0;
unsigned long lastLogicTick = 0;
bool mpuStatus = false;

const char* ap_ssid = "ROVER_X_PRO";
const char* ap_password = "12345678";
WebServer server(80);

// ========= DASHBOARD HTML (Auto-Scaling & Speed Control) =========
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ROVER SLAM MOBILE</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <style>
        body { font-family: 'Segoe UI', sans-serif; background: #080808; color: #00ff41; text-align: center; margin: 0; padding: 0; overflow: hidden; }
        .hud-grid { display: grid; grid-template-columns: repeat(3, 1fr); background: #111; padding: 8px; font-size: 11px; border-bottom: 1px solid #222; gap: 4px; }
        .hud-item { background: #181818; padding: 5px; border-radius: 4px; border: 1px solid #222; }
        .val { display: block; font-size: 14px; font-weight: bold; color: #fff; }
        canvas { background: #000; display: block; margin: 5px auto; width: 98vw; height: 48vh; border-radius: 8px; border: 1px solid #333; }
        .speed-cont { padding: 10px; background: #111; margin: 5px 12px; border-radius: 8px; border: 1px solid #222; }
        input[type=range] { width: 100%; height: 15px; border-radius: 5px; background: #333; outline: none; accent-color: #00ff41; }
        .controls { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; padding: 8px 12px; }
        button { background: #111; color: #00ff41; border: 1px solid #00ff41; padding: 15px; font-size: 13px; font-weight: bold; border-radius: 8px; }
        button:active { background: #00ff41; color: #000; }
        .stop { border-color: #ff4141; color: #ff4141; }
        #status-bar { font-size: 10px; padding: 4px; color: #666; background: #000; }
        .ok { color: #00ff41; } .err { color: #ff4141; }
    </style>
</head>
<body>
    <div class="hud-grid">
        <div class="hud-item">POS X<span class="val" id="vx">0</span></div>
        <div class="hud-item">POS Y<span class="val" id="vy">0</span></div>
        <div class="hud-item">HEADING<span class="val" id="vh">0°</span></div>
        <div class="hud-item">SONAR<span class="val" id="vd">0</span></div>
        <div class="hud-item">MPU<span class="val" id="sm">--</span></div>
        <div class="hud-item">SPEED<span class="val" id="vs">190</span></div>
    </div>
    
    <div id="status-bar">SYSTEM ACTIVE | READY</div>
    <canvas id="map"></canvas>

    <div class="speed-cont">
        <div style="display:flex; justify-content: space-between; font-size: 10px; margin-bottom: 5px;">
            <span>MOTOR THRUST</span>
            <span id="spd_val">190</span>
        </div>
        <input type="range" min="100" max="255" value="190" oninput="updateSpeed(this.value)">
    </div>
    
    <div class="controls">
        <button id="mainBtn" onclick="toggle()">START MISSION</button>
        <button onclick="resetMap()">RESET MAP</button>
        <button class="stop" onclick="cmd('/stop')" style="grid-column: span 2; padding: 12px;">EMERGENCY STOP</button>
    </div>

    <script>
        let path = [{x:0, y:0}];
        let obstacles = [];
        let minX = -100, maxX = 100, minY = -100, maxY = 100;
        const canvas = document.getElementById('map');
        const ctx = canvas.getContext('2d');

        async function update() {
            try {
                const res = await fetch('/data');
                const d = await res.json();
                
                document.getElementById('vx').innerText = Math.round(d.x);
                document.getElementById('vy').innerText = Math.round(d.y);
                document.getElementById('vh').innerText = Math.round(d.heading) + '°';
                document.getElementById('vd').innerText = Math.round(d.distance);
                document.getElementById('vs').innerText = d.speed;
                document.getElementById('sm').innerHTML = d.mpu ? '<span class="ok">OK</span>' : '<span class="err">FAIL</span>';
                document.getElementById('mainBtn').innerText = d.surveying ? 'STOP MISSION' : 'START MISSION';
                
                if(d.surveying) {
                    path.push({x: d.x, y: d.y});
                    if(d.isObs) obstacles.push({x: d.ox, y: d.oy});
                    minX = Math.min(minX, d.x - 20, d.ox - 20);
                    maxX = Math.max(maxX, d.x + 20, d.ox + 20);
                    minY = Math.min(minY, d.y - 20, d.oy - 20);
                    maxY = Math.max(maxY, d.y + 20, d.oy + 20);
                }
                draw();
            } catch(e) {}
        }

        function draw() {
            canvas.width = canvas.clientWidth; canvas.height = canvas.clientHeight;
            ctx.clearRect(0,0,canvas.width, canvas.height);
            const padding = 20;
            const mapW = maxX - minX;
            const mapH = maxY - minY;
            const scale = Math.min((canvas.width - padding)/mapW, (canvas.height - padding)/mapH);
            ctx.save();
            ctx.translate(canvas.width/2, canvas.height/2);
            ctx.scale(scale, -scale);
            ctx.translate(-(minX + maxX)/2, -(minY + maxY)/2);
            ctx.strokeStyle = '#111'; ctx.lineWidth = 1/scale;
            for(let i=-1000; i<=1000; i+=50){
                ctx.beginPath(); ctx.moveTo(i, -1000); ctx.lineTo(i, 1000); ctx.stroke();
                ctx.beginPath(); ctx.moveTo(-1000, i); ctx.lineTo(1000, i); ctx.stroke();
            }
            ctx.strokeStyle = '#ff4141'; ctx.lineWidth = 3/scale;
            ctx.beginPath();
            path.forEach((p, i) => { if(i==0) ctx.moveTo(p.x, p.y); else ctx.lineTo(p.x, p.y); });
            ctx.stroke();
            ctx.fillStyle = '#00ff41';
            obstacles.forEach(o => { ctx.beginPath(); ctx.arc(o.x, o.y, 3/scale, 0, 7); ctx.fill(); });
            const last = path[path.length-1];
            ctx.fillStyle = '#fff'; ctx.beginPath(); ctx.arc(last.x, last.y, 6/scale, 0, 7); ctx.fill();
            ctx.restore();
        }

        function updateSpeed(v) {
            document.getElementById('spd_val').innerText = v;
            fetch('/setSpeed?val=' + v);
        }

        function toggle() {
            const btn = document.getElementById('mainBtn');
            const target = (btn.innerText === 'START MISSION') ? '/start' : '/stop';
            cmd(target);
        }
        function cmd(e) { fetch(e); }
        function resetMap() { path = [{x:0,y:0}]; obstacles = []; minX=-100; maxX=100; minY=-100; maxY=100; }
        setInterval(update, 250);
    </script>
</body>
</html>
)rawliteral";

// ========= SENSOR HELPERS =========

float getDistance() {
    digitalWrite(TRIG, LOW); delayMicroseconds(2);
    digitalWrite(TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    long duration = pulseIn(ECHO, HIGH, 25000); 
    if (duration == 0) return 400;
    return duration * 0.034 / 2;
}

void applyMotors(int s1, int s2, int s3, int s4) {
    digitalWrite(IN1, s1); digitalWrite(IN2, s2);
    digitalWrite(IN3, s3); digitalWrite(IN4, s4);
    analogWrite(ENA, speedVal); analogWrite(ENB, speedVal);
}

void stopMotors() {
    digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
    analogWrite(ENA, 0); analogWrite(ENB, 0);
}

// ========= SETUP & SERVER =========

void setup() {
    Serial.begin(115200);
    I2C_MPU.begin(4, 15);
    I2C_QMC.begin(21, 22);

    pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
    pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
    pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

    I2C_MPU.beginTransmission(MPU_ADDR);
    mpuStatus = (I2C_MPU.endTransmission() == 0);
    if(mpuStatus) {
        I2C_MPU.beginTransmission(MPU_ADDR); I2C_MPU.write(0x6B); I2C_MPU.write(0); I2C_MPU.endTransmission();
    }

    WiFi.softAP(ap_ssid, ap_password);
    
    server.on("/", []() { server.send(200, "text/html", index_html); });
    server.on("/start", []() { surveying = true; server.send(200); });
    server.on("/stop", []() { surveying = false; stopMotors(); server.send(200); });
    
    server.on("/setSpeed", []() {
        if(server.hasArg("val")) {
            speedVal = server.arg("val").toInt();
            speedVal = constrain(speedVal, 0, 255);
        }
        server.send(200);
    });
    
    server.on("/data", []() {
        float rad = headingDeg * PI / 180.0;
        float ox = x + (distanceCM * cos(rad));
        float oy = y + (distanceCM * sin(rad));
        char json[300];
        snprintf(json, sizeof(json), 
            "{\"x\":%.1f,\"y\":%.1f,\"ox\":%.1f,\"oy\":%.1f,\"heading\":%.1f,\"distance\":%.1f,\"isObs\":%s,\"surveying\":%s,\"mpu\":%s,\"speed\":%d}",
            x, y, ox, oy, headingDeg, distanceCM, 
            obstacleDetected?"true":"false", surveying?"true":"false", mpuStatus?"true":"false", speedVal);
        server.send(200, "application/json", json);
    });

    server.begin();
    lastTime = millis();
}

void loop() {
    server.handleClient();

    if (surveying && (millis() - lastLogicTick > 50)) {
        lastLogicTick = millis();

        I2C_MPU.beginTransmission(MPU_ADDR);
        I2C_MPU.write(0x47);
        I2C_MPU.endTransmission(false);
        I2C_MPU.requestFrom(MPU_ADDR, 2);
        if (I2C_MPU.available() >= 2) {
            int16_t gz = I2C_MPU.read() << 8 | I2C_MPU.read();
            float dt = (millis() - lastTime) / 1000.0;
            lastTime = millis();
            float gyroZ = gz / 131.0; 
            if(abs(gyroZ) > 0.8) headingDeg += gyroZ * dt;
        }

        distanceCM = getDistance();
        obstacleDetected = (distanceCM < 25);
        float rad = headingDeg * PI / 180.0;

        if (obstacleDetected) {
            stopMotors();
            applyMotors(LOW, HIGH, HIGH, LOW); 
            delay(150); 
        } else {
            applyMotors(HIGH, LOW, HIGH, LOW); 
            x += 1.1 * cos(rad);
            y += 1.1 * sin(rad);
        }
    } else if (!surveying) {
        lastTime = millis();
    }
}
