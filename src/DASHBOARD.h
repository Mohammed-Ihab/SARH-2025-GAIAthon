const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Agriculture Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); 
            min-height: 100vh; 
            color: #333; 
            transition: all 0.3s ease; 
        }
        body.dark-mode { 
            background: #121212;
            color: #f0f0f0;
        }
        
        .container { max-width: 1400px; margin: 0 auto; padding: 20px; }
        
        /* Header styles with connection indicator */
        .header { 
            text-align: center; 
            margin-bottom: 30px; 
            color: white;
            position: relative;
            padding-top: 40px;
        }
        .dark-mode .header { 
            color: white;
        }
        .header h1 { 
            font-size: 2.5rem; 
            margin-bottom: 10px; 
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3); 
        }
        .dark-mode .header h1 {
            text-shadow: 2px 2px 4px rgba(0,0,0,0.5);
        }
        
        /* Connection indicator */
        .connection-status {
            position: absolute;
            top: 10px;
            left: 10px;
            display: flex;
            align-items: center;
            font-size: 0.9rem;
            padding: 5px 10px;
            border-radius: 20px;
            background: rgba(255,255,255,0.2);
        }
        .dark-mode .connection-status {
            background: rgba(255,255,255,0.1);
        }
        .connection-dot {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            margin-right: 8px;
        }
        .connected {
            background-color: #4CAF50;
            box-shadow: 0 0 10px #4CAF50;
        }
        .disconnected {
            background-color: #F44336;
            box-shadow: 0 0 10px #F44336;
        }
        
        /* Dark/Light mode toggle */
        .theme-switch {
            position: absolute;
            top: 10px;
            right: 10px;
            display: flex;
            align-items: center;
        }
        .switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
            margin-left: 10px;
        }
        .switch input { 
            opacity: 0;
            width: 0;
            height: 0;
        }
        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #764ba2;
            transition: .4s;
            border-radius: 34px;
        }
        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }
        input:checked + .slider {
            background-color: #333;
        }
        input:checked + .slider:before {
            transform: translateX(26px);
        }
        
        .monitoring-section { margin-bottom: 40px; position: relative; }
        .section-title { 
            font-size: 1.8rem; 
            color: white;
            text-align: center; 
            margin-bottom: 20px; 
            text-shadow: 1px 1px 3px rgba(0,0,0,0.3); 
        }
        .dark-mode .section-title { 
            color: white;
            text-shadow: 1px 1px 3px rgba(0,0,0,0.5);
        }
        
        .dashboard-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 20px; }
        .sensor-card { 
            background: rgba(255,255,255,0.95);
            border-radius: 15px; 
            padding: 25px; 
            box-shadow: 0 8px 32px rgba(0,0,0,0.1); 
            backdrop-filter: blur(10px); 
            border: 1px solid rgba(255,255,255,0.2); 
            transition: all 0.3s ease; 
        }
        .dark-mode .sensor-card {
            background: rgba(30,30,30,0.95);
            border: 1px solid rgba(255,255,255,0.1);
        }
        .sensor-card:hover { 
            transform: translateY(-5px); 
            box-shadow: 0 12px 40px rgba(0,0,0,0.15); 
        }
        .sensor-header { display: flex; align-items: center; margin-bottom: 20px; }
        .sensor-icon { 
            width: 40px; 
            height: 40px; 
            border-radius: 50%; 
            display: flex; 
            align-items: center; 
            justify-content: center; 
            margin-right: 15px; 
            font-size: 20px; 
            color: white;
        }
        .sensor-title { 
            font-size: 1.2rem; 
            font-weight: 600; 
            color: #333;
        }
        .dark-mode .sensor-title {
            color: white;
        }
        .sensor-value { 
            font-size: 2.5rem; 
            font-weight: bold; 
            margin: 15px 0; 
            color: #333;
        }
        .dark-mode .sensor-value {
            color: white;
        }
        .sensor-unit { 
            font-size: 1rem; 
            color: #666; 
            margin-left: 10px; 
        }
        .dark-mode .sensor-unit {
            color: #aaa;
        }
        .status-indicator { 
            display: inline-block; 
            padding: 5px 15px; 
            border-radius: 20px; 
            font-size: 0.9rem; 
            font-weight: 500; 
            margin-top: 10px; 
        }
        .good { background: #4CAF50; color: white; }
        .warning { background: #FF9800; color: white; }
        .danger { background: #F44336; color: white; }
        
        .controls { 
            background: rgba(255,255,255,0.95);
            border-radius: 15px; 
            padding: 25px; 
            margin: 20px auto 0 auto; 
            border: 1px solid rgba(255,255,255,0.2);
            position: relative;
            max-width: 800px;
        }
        .dark-mode .controls {
            background: rgba(30,30,30,0.95);
            border: 1px solid rgba(255,255,255,0.1);
        }
        
        /* Mode switch in corner */
        .mode-switch {
            position: absolute;
            top: 15px;
            right: 15px;
            display: flex;
            align-items: center;
            font-size: 0.9rem;
            z-index: 10;
        }
        .mode-switch label {
            margin-right: 10px;
            font-weight: 500;
            color: #333;
        }
        .dark-mode .mode-switch label {
            color: white;
        }
        
        /* Control buttons */
        .control-button { 
            padding: 12px 24px; 
            border: none; 
            border-radius: 25px; 
            cursor: pointer; 
            font-weight: 500; 
            margin: 5px 10px; 
            transition: all 0.3s ease; 
            font-size: 1rem;
            min-width: 150px;
        }
        
        .control-button, .btn {
            -webkit-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            user-select: none;
            -webkit-touch-callout: none;
        }

        /* Dynamic button colors */
        .control-button.inactive {
            background: white;
            color: #333;
            border: 2px solid #ddd;
        }
        .dark-mode .control-button.inactive {
            background: #333;
            color: white;
            border: 2px solid #555;
        }
        .control-button.active {
            background: #4CAF50;
            color: white;
            border: 2px solid #4CAF50;
        }
        .control-button:hover:not(:disabled) { 
            transform: translateY(-2px); 
            box-shadow: 0 5px 15px rgba(0,0,0,0.2); 
        }
        
        /* Fan speed slider */
        .slider-container {
            display: block;
            margin: 15px auto;
            text-align: center;
        }
        .slider-label {
            display: block;
            margin-bottom: 8px;
            font-weight: 500;
            color: #333;
            font-size: 0.9rem;
        }
        .dark-mode .slider-label {
            color: white;
        }
        .speed-slider {
            width: 300px;
            height: 8px;
            border-radius: 5px;
            background: #ddd;
            outline: none;
            opacity: 0.7;
            transition: opacity 0.2s;
            appearance: none;
        }
        .dark-mode .speed-slider {
            background: #555;
        }
        .speed-slider:hover {
            opacity: 1;
        }
        .speed-slider::-webkit-slider-thumb {
            appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #4CAF50;
            cursor: pointer;
        }
        .speed-slider::-moz-range-thumb {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: #4CAF50;
            cursor: pointer;
            border: none;
        }
        .speed-value {
            display: inline-block;
            margin-left: 10px;
            font-weight: bold;
            color: #4CAF50;
            min-width: 40px;
        }
        
        /* Automatic mode overlay */
        .auto-mode-overlay {
            position: absolute;
            top: 60px;
            left: 25px;
            right: 25px;
            bottom: 25px;
            background: rgba(255,255,255,0.8);
            border-radius: 10px;
            display: none;
            align-items: center;
            justify-content: center;
            z-index: 5;
            backdrop-filter: blur(3px);
        }
        .dark-mode .auto-mode-overlay {
            background: rgba(30,30,30,0.8);
        }
        .auto-mode-text {
            font-size: 1.5rem;
            font-weight: bold;
            color: #667eea;
            text-shadow: 1px 1px 2px rgba(0,0,0,0.1);
        }
        .dark-mode .auto-mode-text {
            color: #8a9fff;
        }
        
        /* Controls content blur when in auto mode */
        .controls.auto-mode .controls-content {
            filter: blur(2px);
            pointer-events: none;
        }
        .controls.auto-mode .auto-mode-overlay {
            display: flex;
        }
        
        .btn { 
            padding: 12px 24px; 
            border: none; 
            border-radius: 25px; 
            background: linear-gradient(45deg, #667eea, #764ba2); 
            color: white; 
            cursor: pointer; 
            font-weight: 500; 
            margin: 5px; 
            transition: all 0.3s ease; 
        }
        .btn:hover { 
            transform: translateY(-2px); 
            box-shadow: 0 5px 15px rgba(0,0,0,0.2); 
        }
        .btn.active { 
            background: linear-gradient(45deg, #4CAF50, #45a049); 
        }
        
        /* Sensor icon gradients (unchanged in both modes) */
        .ph-level { background: linear-gradient(45deg, #667eea, #764ba2); }
        .turbidity { background: linear-gradient(45deg, #FFA726, #FF7043); }
        .tds { background: linear-gradient(45deg, #26C6DA, #00ACC1); }
        .co2 { background: linear-gradient(45deg, #66BB6A, #43A047); }
        .co { background: linear-gradient(45deg, #EF5350, #E53935); }
        .soil-moisture { background: linear-gradient(45deg, #8B4513, #D2691E); }
        .water-usage { background: linear-gradient(45deg, #36D1DC, #5B86E5); }
        .air-moisture { background: linear-gradient(45deg, #87CEEB, #4682B4); }
        
        /* Empty tank overlay styles */
        .empty-tank-overlay { 
            position: absolute; 
            top: 60px; 
            left: 0; 
            right: 0; 
            bottom: 0; 
            background: rgba(244, 67, 54, 0.9); 
            border-radius: 15px; 
            display: none; 
            align-items: center; 
            justify-content: center; 
            z-index: 10;
            backdrop-filter: blur(5px);
        }
        .empty-tank-message { 
            background: white; 
            padding: 30px 40px; 
            border-radius: 15px; 
            text-align: center; 
            font-size: 1.3rem; 
            font-weight: bold; 
            color: #F44336; 
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
            animation: pulse 2s infinite;
            line-height: 1.6;
        }
        @keyframes pulse {
            0% { transform: scale(1); }
            50% { transform: scale(1.05); }
            100% { transform: scale(1); }
        }
        .water-monitoring.tank-empty .dashboard-grid { 
            filter: blur(8px); 
            pointer-events: none; 
            opacity: 0.3;
        }
        .water-monitoring.tank-empty .empty-tank-overlay { 
            display: flex; 
        }
        
        /* Section divider */
        .section-divider {
            height: 2px;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.5), transparent);
            margin: 30px 0;
        }
        .dark-mode .section-divider {
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <!-- 
            <div class="connection-status">
                <div class="connection-dot disconnected" id="connectionDot"></div>
                <span id="connectionText">ESP Disconnected</span>
            </div>
            -->
            
            <!-- Theme toggle switch -->
            <div class="theme-switch">
                <span>Dark Mode</span>
                <label class="switch">
                    <input type="checkbox" id="themeToggle">
                    <span class="slider"></span>
                </label>
            </div>
            
            <h1>🌱 SARH Monitoring & Controlling Dashboard</h1>
            <div>Last Update: <span id="lastUpdate">--:--:--</span></div>
        </div>
        
        <!-- Water Quality & Usage Monitoring Section -->
        <div class="monitoring-section water-monitoring" id="waterMonitoring">
            <h2 class="section-title">💧 Water Quality & Usage Monitoring</h2>
            <div class="empty-tank-overlay">
                <div class="empty-tank-message">
                    🚨 Water tank is empty 🚨<br>🚨 Fill the Tank 🚨
                </div>
            </div>
            <div class="dashboard-grid">
                <div class="sensor-card">
                    <div class="sensor-header">
                        <div class="sensor-icon ph-level">🧪</div>
                        <div class="sensor-title">Water pH Level</div>
                    </div>
                    <div class="sensor-value"><span id="phValue">--</span><span class="sensor-unit">pH</span></div>
                    <div class="status-indicator" id="phStatus">Reading...</div>
                </div>
                
                <div class="sensor-card">
                    <div class="sensor-header">
                        <div class="sensor-icon turbidity">🌊</div>
                        <div class="sensor-title">Water Turbidity</div>
                    </div>
                    <div class="sensor-value"><span id="turbidityValue">--</span><span class="sensor-unit">NTU</span></div>
                    <div class="status-indicator" id="turbidityStatus">Reading...</div>
                </div>
                
                <div class="sensor-card">
                    <div class="sensor-header">
                        <div class="sensor-icon tds">💧</div>
                        <div class="sensor-title">TDS Level</div>
                    </div>
                    <div class="sensor-value"><span id="tdsValue">--</span><span class="sensor-unit">ppm</span></div>
                    <div class="status-indicator" id="tdsStatus">Reading...</div>
                </div>
                
                <div class="sensor-card">
                    <div class="sensor-header">
                        <div class="sensor-icon water-usage">🚰</div>
                        <div class="sensor-title">Water Usage</div>
                    </div>
                    <div class="sensor-value"><span id="waterUsed">--</span><span class="sensor-unit">L</span></div>
                    <div style="font-size: 0.9rem; color: #666; margin-top: 10px;">
                        Pump Flow Rate: ~<span id="flowRate">--</span> L/min
                    </div>
                    <div class="status-indicator" id="waterStatus">Monitoring...</div>
                </div>
            </div>
        </div>
        
        <div class="section-divider"></div>
        
        <!-- Air Quality Monitoring Section -->
        <div class="monitoring-section air-monitoring" id="airMonitoring">
            <h2 class="section-title">🌬️ Air Quality Monitoring</h2>
            <div class="dashboard-grid">
                <div class="sensor-card">
                    <div class="sensor-header">
                        <div class="sensor-icon co2">🌿</div>
                        <div class="sensor-title">Carbon Dioxide</div>
                    </div>
                    <div class="sensor-value"><span id="co2Value">--</span><span class="sensor-unit">ppm</span></div>
                    <div class="status-indicator" id="co2Status">Reading...</div>
                </div>
                
                <div class="sensor-card">
                    <div class="sensor-header">
                        <div class="sensor-icon co">⚠️</div>
                        <div class="sensor-title">Carbon Monoxide</div>
                    </div>
                    <div class="sensor-value"><span id="coValue">--</span><span class="sensor-unit">ppm</span></div>
                    <div class="status-indicator" id="coStatus">Reading...</div>
                </div>
            </div>
        </div>
        
        <div class="section-divider"></div>
        
        <!-- Plant Monitoring Section -->
        <div class="monitoring-section plant-monitoring" id="plantMonitoring">
            <h2 class="section-title">🌱 Plant Monitoring</h2>
            <div class="dashboard-grid">
                <div class="sensor-card">
                    <div class="sensor-header">
                        <div class="sensor-icon soil-moisture">🌱</div>
                        <div class="sensor-title">Soil Moisture</div>
                    </div>
                    <div class="sensor-value"><span id="soilMoisture">--</span><span class="sensor-unit">%</span></div>
                    <div class="status-indicator" id="soilStatus">Reading...</div>
                </div>
                
                <div class="sensor-card">
                    <div class="sensor-header">
                        <div class="sensor-icon air-moisture">💨</div>
                        <div class="sensor-title">Air Moisture</div>
                    </div>
                    <div class="sensor-value"><span id="airMoisture">--</span><span class="sensor-unit">%</span></div>
                    <div class="status-indicator" id="airMoistureStatus">Reading...</div>
                </div>
            </div>
        </div>
        
        <div class="controls" id="systemControls">
            <!-- Mode switch in corner -->
            <div class="mode-switch">
                <label for="modeToggle">Manual</label>
                <label class="switch">
                    <input type="checkbox" id="modeToggle" checked>
                    <span class="slider"></span>
                </label>
                <label>Auto</label>
            </div>
            
            <!-- Automatic mode overlay -->
            <div class="auto-mode-overlay">
                <div class="auto-mode-text">Automatic Mode</div>
            </div>
            
            <div class="controls-content">
                <h3 style="margin-bottom: 20px; text-align: center;">🎛️ System Controls</h3>
                
                <!-- Pump Controls -->
                <div style="margin-bottom: 20px; text-align: center;">
                    <!-- Replace the existing pump button lines with these: -->
                    <button class="control-button inactive" id="filterPumpBtn" 
                            onmousedown="activateFilterPump()" onmouseup="deactivateFilterPump()" onmouseleave="deactivateFilterPump()"
                            ontouchstart="activateFilterPump()" ontouchend="deactivateFilterPump()">
                        Filter Pump
                    </button>
                    <button class="control-button inactive" id="plantPumpBtn" 
                            onmousedown="activatePlantPump()" onmouseup="deactivatePlantPump()" onmouseleave="deactivatePlantPump()"
                            ontouchstart="activatePlantPump()" ontouchend="deactivatePlantPump()">
                        Plant Pump
                    </button>
                    <button class="control-button inactive" id="refilterPumpBtn" 
                            onmousedown="activateRefilterPump()" onmouseup="deactivateRefilterPump()" onmouseleave="deactivateRefilterPump()"
                            ontouchstart="activateRefilterPump()" ontouchend="deactivateRefilterPump()">
                        Refilter Pump
                    </button>
                    <button class="control-button inactive" id="ledStripBtn" 
                            onclick="toggleLEDStrip()" 
                            ontouchstart="handleLEDTouch(event)" ontouchend="handleLEDTouchEnd(event)">
                        LED Strip
                    </button>
                </div>
                
                <!-- Fan Speed Slider -->
                <div class="slider-container" style="text-align: center;">
                    <label class="slider-label">Fan Speed</label>
                    <div style="display: inline-block;">
                        <input type="range" min="30" max="100" value="30" class="speed-slider" id="fanSpeedSlider" oninput="updateFanSpeed(this.value)">
                        <span class="speed-value" id="fanSpeedValue">30%</span>
                    </div>
                </div>
                
                <!-- Other controls -->
                <div style="margin-top: 20px; text-align: center;">
                    <button class="btn" onclick="resetWaterCounter()">Reset Water Counter</button>
                    <!--
                    <button class="btn" onclick="toggleEmptyTank()" style="background: linear-gradient(45deg, #FF6B6B, #EE5A52);">Test Empty Tank Alert</button>
                    -->
                </div>
            </div>
        </div>
    </div>

    <script>
        let tankEmptyState = false;
        let espConnected = false;
        let lastUpdateTime = 0;
        const connectionTimeout = 5000; // 5 seconds
        let filterPumpState = false;
        let plantPumpState = false;
        let refilterPumpState = false;
        let ledStripState = false;
        let ledTouchHandled = false;
        let fanSpeed = 30;
        let autoMode = true;
        
        // Theme toggle functionality
        const themeToggle = document.getElementById('themeToggle');
        themeToggle.addEventListener('change', function() {
            if(this.checked) {
                document.body.classList.add('dark-mode');
                localStorage.setItem('theme', 'dark');
            } else {
                document.body.classList.remove('dark-mode');
                localStorage.setItem('theme', 'light');
            }
        });
        
        // Check for saved theme preference
        if(localStorage.getItem('theme') === 'dark') {
            document.body.classList.add('dark-mode');
            themeToggle.checked = true;
        }
        
        function preventTouchSelection(e) {
            e.preventDefault();
        }

        // Initialize automatic mode on page load
        document.addEventListener('DOMContentLoaded', function() {
            const systemControls = document.getElementById('systemControls');
            const modeToggle = document.getElementById('modeToggle');
            
            // Set initial automatic mode
            if(autoMode) {
                systemControls.classList.add('auto-mode');
                modeToggle.checked = true;
            }
            
            // Initialize connection status as disconnected
            updateConnectionStatus(false);

            const controlButtons = document.querySelectorAll('.control-button, .btn');
            controlButtons.forEach(button => {
                button.addEventListener('touchstart', preventTouchSelection);
                button.addEventListener('selectstart', preventTouchSelection);
            });
        });
        
        // Mode toggle functionality
        const modeToggle = document.getElementById('modeToggle');
        const systemControls = document.getElementById('systemControls');
        
        modeToggle.addEventListener('change', function() {
            autoMode = this.checked;
            if(autoMode) {
                systemControls.classList.add('auto-mode');
            } else {
                systemControls.classList.remove('auto-mode');
            }
            
            // Send mode change to server (which proxies to ESP32)
            fetch('/api/mode', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ auto: autoMode })
            })
            .then(response => response.json())
            .then(data => {
                console.log('Mode updated:', data);
            })
            .catch(error => {
                console.log('Server not available for mode control');
            });
        });
        
        function updateConnectionStatus(isConnected) {
            const dot = document.getElementById('connectionDot');
            const text = document.getElementById('connectionText');
            
            console.log('Updating connection status:', isConnected, 'Dot found:', !!dot, 'Text found:', !!text);
            
            if(dot && text) {
                if(isConnected) {
                    dot.classList.remove('disconnected');
                    dot.classList.add('connected');
                    text.textContent = 'ESP Connected';
                    espConnected = true;
                    console.log('Set to connected');
                } else {
                    dot.classList.remove('connected');
                    dot.classList.add('disconnected');
                    text.textContent = 'ESP Disconnected';
                    espConnected = false;
                    console.log('Set to disconnected');
                }
            } else {
                console.error('Connection status elements not found!');
            }
        }
        
        function activateFilterPump() {
            if(autoMode) return;
            
            const btn = document.getElementById('filterPumpBtn');
            btn.classList.remove('inactive');
            btn.classList.add('active');
            
            // Send press command to server (which proxies to ESP32)
            fetch('/api/filter-pump/press', {method: 'POST'})
                .then(response => response.json())
                .then(data => {
                    console.log('Filter pump pressed:', data);
                })
                .catch(error => {
                    console.log('Server not available for pump control');
                });
        }
        
        function deactivateFilterPump() {
            if(autoMode) return;
            
            const btn = document.getElementById('filterPumpBtn');
            btn.classList.remove('active');
            btn.classList.add('inactive');
            
            // Send release command to server (which proxies to ESP32)
            fetch('/api/filter-pump/release', {method: 'POST'})
                .then(response => response.json())
                .then(data => {
                    console.log('Filter pump released:', data);
                })
                .catch(error => {
                    console.log('Server not available for pump control');
                });
        }
        
        function activatePlantPump() {
            if(autoMode) return;
            
            const btn = document.getElementById('plantPumpBtn');
            btn.classList.remove('inactive');
            btn.classList.add('active');
            
            // Send command to server (which proxies to ESP32)
            fetch('/api/plant-pump/press', {method: 'POST'})
                .then(response => response.json())
                .then(data => {
                    console.log('Plant pump pressed:', data);
                })
                .catch(error => {
                    console.log('Server not available for pump control');
                });
        }
        
        function deactivatePlantPump() {
            if(autoMode) return;
            
            const btn = document.getElementById('plantPumpBtn');
            btn.classList.remove('active');
            btn.classList.add('inactive');
            
            // Send command to server (which proxies to ESP32)
            fetch('/api/plant-pump/release', {method: 'POST'})
                .then(response => response.json())
                .then(data => {
                    console.log('Plant pump released:', data);
                })
                .catch(error => {
                    console.log('Server not available for pump control');
                });
        }
        
        function activateRefilterPump() {
            if(autoMode) return;
            
            const btn = document.getElementById('refilterPumpBtn');
            btn.classList.remove('inactive');
            btn.classList.add('active');
            
            fetch('/api/refilter-pump/press', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ state: true })
            })
            .then(response => response.json())
            .then(data => {
                console.log('Refilter pump activated:', data);
            })
            .catch(error => {
                console.log('Server not available for refilter pump control');
            });
        }

        function deactivateRefilterPump() {
            if(autoMode) return;
            
            const btn = document.getElementById('refilterPumpBtn');
            btn.classList.remove('active');
            btn.classList.add('inactive');
            
            fetch('/api/refilter-pump/release', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ state: false })
            })
            .then(response => response.json())
            .then(data => {
                console.log('Refilter pump deactivated:', data);
            })
            .catch(error => {
                console.log('Server not available for refilter pump control');
            });
        }

        function handleLEDTouch(e) {
            e.preventDefault();
            ledTouchHandled = true;
            toggleLEDStrip();
        }

        function handleLEDTouchEnd(e) {
            e.preventDefault();
            // Reset the flag after a short delay
            setTimeout(() => {
                ledTouchHandled = false;
            }, 100);
        }

        // Modify the existing toggleLEDStrip function to prevent double execution
        function toggleLEDStrip() {
            if(autoMode) return;
            
            // Prevent double execution on touch devices
            if(ledTouchHandled && event && event.type === 'click') {
                return;
            }
            
            ledStripState = !ledStripState;
            const btn = document.getElementById('ledStripBtn');
            
            if(ledStripState) {
                btn.classList.remove('inactive');
                btn.classList.add('active');
            } else {
                btn.classList.remove('active');
                btn.classList.add('inactive');
            }
            
            // Send command to server
            fetch('/api/led-strip/toggle', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ state: ledStripState })
            })
            .then(response => response.json())
            .then(data => {
                console.log('LED strip toggled:', data);
            })
            .catch(error => {
                console.log('Server not available for LED strip control');
            });
        }

        function updateFanSpeed(value) {
            if(autoMode) return;
            
            fanSpeed = value;
            document.getElementById('fanSpeedValue').textContent = value + '%';
            
            // Send command to server (which proxies to ESP32)
            fetch('/api/fan-speed', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ speed: parseInt(value) })
            })
            .then(response => response.json())
            .then(data => {
                console.log('Fan speed updated:', data);
            })
            .catch(error => {
                console.log('Server not available for fan control');
            });
        }
        
        function updateDashboard() {
            fetch('/api/sensors')
                .then(response => {
                    if(!response.ok) {
                        updateConnectionStatus(false);
                        throw new Error('Network response was not ok');
                    }
                    lastUpdateTime = Date.now();
                    return response.json();
                })
                .then(data => {
                    console.log('Received sensor data:', data);
                    
                    // Update connection status based on ESP32 connectivity
                    const isConnected = data.esp32Connected === true;
                    console.log('ESP32 connected status from data:', isConnected);
                    updateConnectionStatus(isConnected);
                    
                    // Update all sensor values
                    document.getElementById('phValue').textContent = data.ph ? data.ph.toFixed(1) : '--';
                    document.getElementById('turbidityValue').textContent = data.turbidity ? data.turbidity.toFixed(1) : '--';
                    document.getElementById('tdsValue').textContent = data.tds ? data.tds.toFixed(0) : '--';
                    document.getElementById('co2Value').textContent = data.co2 ? data.co2.toFixed(0) : '--';
                    document.getElementById('coValue').textContent = data.co ? data.co.toFixed(1) : '--';
                    document.getElementById('soilMoisture').textContent = data.soilMoisture ? data.soilMoisture.toFixed(1) : '--';
                    document.getElementById('airMoisture').textContent = data.airMoisture ? data.airMoisture.toFixed(1) : '--';
                    document.getElementById('waterUsed').textContent = data.waterUsed ? data.waterUsed.toFixed(1) : '--';
                    document.getElementById('flowRate').textContent = data.flowRate ? data.flowRate.toFixed(2) : '--';
                    
                    // Pump buttons are momentary, so they don't need state updates from ESP
                    // They will always return to inactive state after being pressed
                    
                    if(data.fanSpeed !== undefined) {
                        fanSpeed = data.fanSpeed;
                        document.getElementById('fanSpeedSlider').value = fanSpeed;
                        document.getElementById('fanSpeedValue').textContent = fanSpeed + '%';
                    }
                    
                    // Check for empty tank status from ESP
                    if (data.tankEmpty !== undefined) {
                        handleTankEmptyStatus(data.tankEmpty);
                    }
                    
                    // Update status indicators
                    if(data.ph) updateStatus('phStatus', data.ph, 6.0, 8.5);
                    if(data.turbidity) updateStatus('turbidityStatus', data.turbidity, 0, 35);
                    if(data.tds) updateStatus('tdsStatus', data.tds, 0, 1600);
                    if(data.co2) updateStatus('co2Status', data.co2, 350, 1200);
                    if(data.co) updateStatus('coStatus', data.co, 0, 5);
                    if(data.soilMoisture) updateStatus('soilStatus', data.soilMoisture, 30, 70);
                    if(data.airMoisture) updateStatus('airMoistureStatus', data.airMoisture, 40, 80);
                    
                    // Water usage status
                    const waterElement = document.getElementById('waterStatus');
                    if (data.waterUsed) {
                        if (data.waterUsed < 500) {
                            waterElement.className = 'status-indicator good';
                            waterElement.textContent = 'Normal Usage';
                        } else if (data.waterUsed < 800) {
                            waterElement.className = 'status-indicator warning';
                            waterElement.textContent = 'High Usage';
                        } else {
                            waterElement.className = 'status-indicator danger';
                            waterElement.textContent = 'Very High Usage';
                        }
                    }
                    
                    document.getElementById('lastUpdate').textContent = new Date().toLocaleTimeString();
                })
                .catch(error => {
                    console.error('Error:', error);
                    // Check if we haven't received an update in the timeout period
                    if(Date.now() - lastUpdateTime > connectionTimeout) {
                        updateConnectionStatus(false);
                    }
                });
        }
        
        function handleTankEmptyStatus(isEmpty) {
            const waterSection = document.getElementById('waterMonitoring');
            if (isEmpty && !tankEmptyState) {
                waterSection.classList.add('tank-empty');
                tankEmptyState = true;
            } else if (!isEmpty && tankEmptyState) {
                waterSection.classList.remove('tank-empty');
                tankEmptyState = false;
            }
        }
        
        function updateStatus(elementId, value, minGood, maxGood) {
            const element = document.getElementById(elementId);
            if (value >= minGood && value <= maxGood) {
                element.className = 'status-indicator good';
                element.textContent = 'Optimal';
            } else if (elementId === 'coStatus' && value > maxGood) {
                element.className = 'status-indicator danger';
                element.textContent = 'DANGER!';
            } else {
                element.className = 'status-indicator warning';
                element.textContent = 'Check Level';
            }
        }
        
        function resetWaterCounter() {
            fetch('/api/water/reset', {method: 'POST'})
                .then(response => response.json())
                .then(data => {
                    console.log('Water counter reset:', data);
                });
        }
        
        // Test function to simulate empty tank (remove this in production)
        function toggleEmptyTank() {
            handleTankEmptyStatus(!tankEmptyState);
        }
        
        // Update dashboard every 2 seconds
        setInterval(updateDashboard, 2000);
        updateDashboard(); // Initial load
    </script>
</body>
</html>
)rawliteral";