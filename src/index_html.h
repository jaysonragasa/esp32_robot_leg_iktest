#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>AskalBot Leg Controller</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    :root {
      --bg-color: #0f172a;
      --card-bg: #1e293b;
      --primary: #3b82f6;
      --primary-glow: rgba(59, 130, 246, 0.5);
      --text-main: #f8fafc;
      --text-muted: #94a3b8;
    }
    body {
      margin: 0;
      padding: 0;
      font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
      background-color: var(--bg-color);
      color: var(--text-main);
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
    }
    .container {
      background: var(--card-bg);
      padding: 2rem;
      border-radius: 20px;
      box-shadow: 0 10px 25px rgba(0,0,0,0.5);
      width: 90%;
      max-width: 400px;
      text-align: center;
    }
    h1 {
      font-size: 1.8rem;
      margin-bottom: 0.5rem;
      background: linear-gradient(90deg, #60a5fa, #a78bfa);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }
    p.subtitle {
      color: var(--text-muted);
      margin-bottom: 2rem;
      font-size: 0.9rem;
    }
    .slider-container {
      margin-bottom: 2rem;
      text-align: left;
    }
    .label-row {
      display: flex;
      justify-content: space-between;
      margin-bottom: 0.5rem;
      font-weight: 600;
    }
    .value-display {
      color: var(--primary);
    }
    input[type=range] {
      -webkit-appearance: none;
      width: 100%;
      background: transparent;
    }
    input[type=range]:focus {
      outline: none;
    }
    input[type=range]::-webkit-slider-runnable-track {
      width: 100%;
      height: 8px;
      cursor: pointer;
      background: #334155;
      border-radius: 4px;
    }
    input[type=range]::-webkit-slider-thumb {
      height: 24px;
      width: 24px;
      border-radius: 50%;
      background: var(--primary);
      cursor: pointer;
      -webkit-appearance: none;
      margin-top: -8px;
      box-shadow: 0 0 10px var(--primary-glow);
      transition: transform 0.1s;
    }
    input[type=range]::-webkit-slider-thumb:hover {
      transform: scale(1.2);
    }
    .footer {
      margin-top: 2rem;
      font-size: 0.8rem;
      color: var(--text-muted);
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>AskalBot</h1>
    <p class="subtitle">3DOF Leg IK Controller</p>

    <div class="slider-container">
      <div class="label-row">
        <span>X-Axis (Forward)</span>
        <span class="value-display"><span id="xVal">0</span> mm</span>
      </div>
      <input type="range" min="-100" max="100" value="0" id="xSlider" oninput="updateValues()">
    </div>

    <div class="slider-container">
      <div class="label-row">
        <span>Y-Axis (Lateral)</span>
        <span class="value-display"><span id="yVal">47</span> mm</span>
      </div>
      <input type="range" min="-50" max="150" value="47" id="ySlider" oninput="updateValues()">
    </div>

    <div class="slider-container">
      <div class="label-row">
        <span>Z-Axis (Vertical)</span>
        <span class="value-display"><span id="zVal">-90</span> mm</span>
      </div>
      <input type="range" min="-120" max="-20" value="-90" id="zSlider" oninput="updateValues()">
    </div>
    
    <div class="canvas-container" style="display: flex; justify-content: space-between; margin-bottom: 1.5rem;">
      <div style="text-align: center; width: 48%;">
        <div style="font-size: 0.8rem; color: var(--text-muted); margin-bottom: 4px;">Side View (X-Z)</div>
        <canvas id="sideCanvas" width="150" height="150" style="background: #0f172a; border-radius: 8px; width: 100%; max-width: 150px;"></canvas>
      </div>
      <div style="text-align: center; width: 48%;">
        <div style="font-size: 0.8rem; color: var(--text-muted); margin-bottom: 4px;">Front View (Y-Z)</div>
        <canvas id="frontCanvas" width="150" height="150" style="background: #0f172a; border-radius: 8px; width: 100%; max-width: 150px;"></canvas>
      </div>
    </div>
    
    <div class="angles-container" style="display: flex; justify-content: space-between; margin-bottom: 1.5rem; background: #0f172a; padding: 10px; border-radius: 8px; font-size: 0.85rem; color: #cbd5e1;">
      <div>Coxa: <span id="coxaAngleVal" style="color: var(--primary); font-weight: bold;">0.00</span>&deg;</div>
      <div>Femur: <span id="femurAngleVal" style="color: var(--primary); font-weight: bold;">0.00</span>&deg;</div>
      <div>Tibia: <span id="tibiaAngleVal" style="color: var(--primary); font-weight: bold;">0.00</span>&deg;</div>
    </div>

    <div class="footer">Real-time kinematic adjustments</div>
  </div>

<script>
  const L_COXA = 47.0;
  const L_FEMUR = 50.0;
  const L_TIBIA = 58.0;

  function calculateFK(coxa_a, femur_a, tibia_a) {
    let p0 = {x: 0, y: 0, z: 0};
    let p1 = {x: 0, y: L_COXA, z: 0};
    let p2 = {
      x: L_FEMUR * Math.sin(femur_a),
      y: L_COXA,
      z: -L_FEMUR * Math.cos(femur_a)
    };
    let p3 = {
      x: p2.x + L_TIBIA * Math.sin(femur_a + tibia_a),
      y: L_COXA,
      z: p2.z - L_TIBIA * Math.cos(femur_a + tibia_a)
    };

    let rotateX = (p, a) => ({
      x: p.x,
      y: p.y * Math.cos(a) - p.z * Math.sin(a),
      z: p.y * Math.sin(a) + p.z * Math.cos(a)
    });

    return [rotateX(p0, coxa_a), rotateX(p1, coxa_a), rotateX(p2, coxa_a), rotateX(p3, coxa_a)];
  }

  function drawLeg(x, y, z) {
    let L_yz = Math.sqrt(y*y + z*z);
    if (L_COXA > L_yz) return; 
    let L_p = Math.sqrt(L_yz*L_yz - L_COXA*L_COXA);
    
    let coxa_a = Math.atan2(y, -z) - Math.atan2(L_COXA, L_p);
    let D_sq = x*x + L_p*L_p;
    let D = Math.sqrt(D_sq);

    let cos_tibia = (D_sq - L_FEMUR*L_FEMUR - L_TIBIA*L_TIBIA) / (2.0 * L_FEMUR * L_TIBIA);
    cos_tibia = Math.max(-1.0, Math.min(1.0, cos_tibia));
    let tibia_a = -Math.acos(cos_tibia);

    let cos_femur = (L_FEMUR*L_FEMUR + D_sq - L_TIBIA*L_TIBIA) / (2.0 * L_FEMUR * D);
    cos_femur = Math.max(-1.0, Math.min(1.0, cos_femur));
    let femur_a = Math.atan2(x, L_p) + Math.acos(cos_femur);

    // Update angle text displays
    let elCoxa = document.getElementById("coxaAngleVal");
    if(elCoxa) elCoxa.innerText = (coxa_a * 180 / Math.PI).toFixed(2);
    let elFemur = document.getElementById("femurAngleVal");
    if(elFemur) elFemur.innerText = (femur_a * 180 / Math.PI).toFixed(2);
    let elTibia = document.getElementById("tibiaAngleVal");
    if(elTibia) elTibia.innerText = (tibia_a * 180 / Math.PI).toFixed(2);

    let pts = calculateFK(coxa_a, femur_a, tibia_a);

    let drawView = (id, tx, ty, axisX, color) => {
      let canvas = document.getElementById(id);
      if(!canvas) return;
      let ctx = canvas.getContext("2d");
      ctx.clearRect(0, 0, 150, 150);
      ctx.save();
      ctx.translate(tx, ty);
      
      // Draw origin marker
      ctx.fillStyle = "#334155";
      ctx.fillRect(-4, -4, 8, 8);

      // Draw bone segments
      ctx.beginPath();
      ctx.moveTo(pts[0][axisX], -pts[0].z);
      for(let i=1; i<4; i++) ctx.lineTo(pts[i][axisX], -pts[i].z);
      ctx.strokeStyle = color;
      ctx.lineWidth = 4;
      ctx.lineJoin = "round";
      ctx.lineCap = "round";
      ctx.stroke();
      
      // Draw joints
      ctx.fillStyle = "#f8fafc";
      for(let p of pts) {
        ctx.beginPath();
        ctx.arc(p[axisX], -p.z, 3.5, 0, Math.PI*2);
        ctx.fill();
      }
      ctx.restore();
    };

    // Draw Side (X) and Front (Y) views
    drawView("sideCanvas", 75, 20, "x", "#3b82f6");
    drawView("frontCanvas", 40, 20, "y", "#a78bfa");
  }

  let timeout = null;
  function updateValues() {
    var x = parseFloat(document.getElementById("xSlider").value);
    var y = parseFloat(document.getElementById("ySlider").value);
    var z = parseFloat(document.getElementById("zSlider").value);
    
    document.getElementById("xVal").innerText = x;
    document.getElementById("yVal").innerText = y;
    document.getElementById("zVal").innerText = z;
    
    drawLeg(x, y, z);
    
    if(timeout !== null) {
      clearTimeout(timeout);
    }
    timeout = setTimeout(() => {
      fetch(`/set?x=${x}&y=${y}&z=${z}`)
        .then(response => {
          if (!response.ok) console.error("Failed to update position");
        })
        .catch(err => console.error(err));
    }, 50);
  }

  window.onload = () => {
    updateValues();
  };
</script>
</body>
</html>
)rawliteral";

#endif
