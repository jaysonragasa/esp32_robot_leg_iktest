#ifndef INDEX_HTML_H
#define INDEX_HTML_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
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
      --danger: #ef4444;
      --success: #10b981;
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
    .main-wrapper {
      display: flex;
      flex-wrap: wrap;
      gap: 20px;
      width: 95%;
      max-width: 900px;
      margin: 20px auto;
    }
    .container {
      background: var(--card-bg);
      padding: 2rem;
      border-radius: 20px;
      box-shadow: 0 10px 25px rgba(0,0,0,0.5);
      flex: 1;
      min-width: 300px;
    }
    h1 {
      font-size: 1.8rem;
      margin-bottom: 0.5rem;
      background: linear-gradient(90deg, #60a5fa, #a78bfa);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      text-align: center;
    }
    p.subtitle {
      color: var(--text-muted);
      margin-bottom: 2rem;
      font-size: 0.9rem;
      text-align: center;
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
    button {
      padding: 12px 24px;
      border-radius: 8px;
      border: none;
      background: var(--primary);
      color: white;
      font-weight: bold;
      font-size: 1rem;
      cursor: pointer;
      transition: background 0.2s, opacity 0.2s;
    }
    button:disabled {
      opacity: 0.5;
      cursor: not-allowed;
    }
    .btn-small {
      padding: 6px 12px;
      font-size: 0.8rem;
    }
    .btn-danger { background: var(--danger); }
    .btn-success { background: var(--success); }
    .btn-secondary { background: #475569; }
    
    .sequence-item {
      background: #0f172a;
      padding: 12px;
      border-radius: 8px;
      margin-bottom: 10px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-size: 0.9rem;
      cursor: pointer;
      border: 1px solid transparent;
      transition: background 0.2s, border 0.2s;
    }
    .sequence-item:hover {
      background: #1e293b;
    }
    .sequence-item.editing {
      border: 1px solid var(--primary);
      background: #1e293b;
    }
    .sequence-item.dragging {
      opacity: 0.5;
      background: #334155;
      border: 1px dashed var(--primary);
    }
    .sequence-coords {
      color: #cbd5e1;
      font-family: monospace;
      font-size: 0.8rem;
    }
    .sequence-actions {
      display: flex;
      gap: 8px;
    }
    .seq-list-container {
      max-height: 480px;
      overflow-y: auto;
      margin-bottom: 20px;
      padding-right: 5px;
    }
    .seq-list-container::-webkit-scrollbar {
      width: 6px;
    }
    .seq-list-container::-webkit-scrollbar-thumb {
      background: #334155;
      border-radius: 3px;
    }
    
    .mini-canvas-container {
      display: flex;
      gap: 5px;
      margin: 0 10px;
    }
    .mini-canvas {
      width: 40px;
      height: 40px;
      background: #000;
      border-radius: 4px;
    }
  </style>
</head>
<body>
  <div class="main-wrapper">
    <!-- LEFT PANEL: Controls -->
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
          <span class="value-display"><span id="yVal">17</span> mm</span>
        </div>
        <input type="range" min="-50" max="150" value="17" id="ySlider" oninput="updateValues()">
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

      <div style="display: flex; gap: 10px; margin-top: 15px;">
        <button id="addBtn" onclick="handleAddOrUpdate()" style="flex: 1;">Add To Sequence</button>
        <button id="cancelBtn" onclick="setEditing(-1)" class="btn-secondary" style="display: none;">Cancel</button>
      </div>
      <div style="margin-top: 10px;">
        <button onclick="resetToDefault()" class="btn-secondary" style="width: 100%;">Back to Default</button>
      </div>
    </div>

    <!-- RIGHT PANEL: Sequence Editor -->
    <div class="container" style="display: flex; flex-direction: column;">
      <h2 style="font-size: 1.4rem; margin-top: 0; text-align: center;">Gait Sequence</h2>
      
      <div class="seq-list-container" id="sequenceList">
        <!-- Sequence items will be injected here -->
      </div>
      
      <div style="margin-top: auto; text-align: center; padding-top: 20px; border-top: 1px solid #334155;">
        <button id="playGaitBtn" onclick="toggleGait()" style="width: 100%;" class="btn-success">Play Gait</button>
      </div>
    </div>
  </div>

<script>
  const L_COXA = 17.6;
  const L_FEMUR = 49.3;
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

  function getPoints(x, y, z) {
    x = -x; // Flip X axis so positive means forward
    let L_yz = Math.sqrt(y*y + z*z);
    if (L_COXA > L_yz) return null; 
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
    
    // Also save angles to UI if this is the main visualizer
    return { pts: calculateFK(coxa_a, femur_a, tibia_a), coxa_a, femur_a, tibia_a };
  }

  function drawLeg(x, y, z) {
    let data = getPoints(x, y, z);
    if (!data) return;
    
    let elCoxa = document.getElementById("coxaAngleVal");
    if(elCoxa) elCoxa.innerText = (data.coxa_a * 180 / Math.PI).toFixed(2);
    let elFemur = document.getElementById("femurAngleVal");
    if(elFemur) elFemur.innerText = (data.femur_a * 180 / Math.PI).toFixed(2);
    let elTibia = document.getElementById("tibiaAngleVal");
    if(elTibia) elTibia.innerText = (data.tibia_a * 180 / Math.PI).toFixed(2);

    let drawView = (id, tx, ty, axisX, color) => {
      let canvas = document.getElementById(id);
      if(!canvas) return;
      let ctx = canvas.getContext("2d");
      ctx.clearRect(0, 0, 150, 150);
      ctx.save();
      ctx.translate(tx, ty);
      
      ctx.fillStyle = "#334155";
      ctx.fillRect(-4, -4, 8, 8);

      ctx.beginPath();
      ctx.moveTo(data.pts[0][axisX], -data.pts[0].z);
      for(let i=1; i<4; i++) ctx.lineTo(data.pts[i][axisX], -data.pts[i].z);
      ctx.strokeStyle = color;
      ctx.lineWidth = 4;
      ctx.lineJoin = "round";
      ctx.lineCap = "round";
      ctx.stroke();
      
      ctx.fillStyle = "#f8fafc";
      for(let p of data.pts) {
        ctx.beginPath();
        ctx.arc(p[axisX], -p.z, 3.5, 0, Math.PI*2);
        ctx.fill();
      }
      ctx.restore();
    };

    drawView("sideCanvas", 75, 20, "x", "#3b82f6");
    drawView("frontCanvas", 40, 20, "y", "#a78bfa");
  }

  function drawMiniView(id, pts, tx, ty, axisX, color) {
    let canvas = document.getElementById(id);
    if(!canvas) return;
    let ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, 40, 40);
    ctx.save();
    ctx.scale(0.26, 0.26); // scale down 150px space to 40px
    ctx.translate(tx, ty);
    
    ctx.beginPath();
    ctx.moveTo(pts[0][axisX], -pts[0].z);
    for(let i=1; i<4; i++) ctx.lineTo(pts[i][axisX], -pts[i].z);
    ctx.strokeStyle = color;
    ctx.lineWidth = 15;
    ctx.lineJoin = "round";
    ctx.lineCap = "round";
    ctx.stroke();
    ctx.restore();
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

  // --- Sequence Editor Logic ---
  let sequence = [
    {x: 20, y: 17, z: -90},
    {x: -20, y: 17, z: -90},
    {x: 0, y: 17, z: -70}
  ];
  let isGait = false;
  let editingIndex = -1;

  function setEditing(index) {
    if (isGait) return;
    editingIndex = index;
    if (index !== -1) {
      let step = sequence[index];
      document.getElementById("xSlider").value = step.x;
      document.getElementById("ySlider").value = step.y;
      document.getElementById("zSlider").value = step.z;
      document.getElementById("addBtn").innerText = "Update Step " + (index + 1);
      document.getElementById("cancelBtn").style.display = "inline-block";
    } else {
      document.getElementById("addBtn").innerText = "Add To Sequence";
      document.getElementById("cancelBtn").style.display = "none";
    }
    updateValues();
    renderSequence();
  }

  function handleAddOrUpdate() {
    var x = parseFloat(document.getElementById("xSlider").value);
    var y = parseFloat(document.getElementById("ySlider").value);
    var z = parseFloat(document.getElementById("zSlider").value);
    if (editingIndex !== -1) {
      sequence[editingIndex] = {x, y, z};
      setEditing(-1); // reset editing state
    } else {
      sequence.push({x, y, z});
      renderSequence();
    }
  }

  function deleteStep(index, e) {
    e.stopPropagation(); // prevent row click
    sequence.splice(index, 1);
    if (editingIndex === index) setEditing(-1);
    else if (editingIndex > index) setEditing(editingIndex - 1);
    else renderSequence();
  }

  function playStep(index, e) {
    e.stopPropagation(); // prevent row click
    if (isGait) return;
    let step = sequence[index];
    document.getElementById("xSlider").value = step.x;
    document.getElementById("ySlider").value = step.y;
    document.getElementById("zSlider").value = step.z;
    updateValues();
  }

  function resetToDefault() {
    if (isGait) return;
    document.getElementById("xSlider").value = 0;
    document.getElementById("ySlider").value = 17;
    document.getElementById("zSlider").value = -90;
    updateValues();
  }

  function renderSequence() {
    const list = document.getElementById("sequenceList");
    list.innerHTML = "";
    
    if (sequence.length === 0) {
      list.innerHTML = "<div style='text-align: center; color: var(--text-muted);'>No steps in sequence</div>";
      return;
    }

    sequence.forEach((step, index) => {
      let div = document.createElement("div");
      div.className = "sequence-item" + (editingIndex === index ? " editing" : "");
      div.dataset.index = index;
      if (!isGait) div.draggable = true;
      
      div.onclick = () => setEditing(index);
      
      div.addEventListener('dragstart', (e) => {
        if (isGait) { e.preventDefault(); return; }
        e.dataTransfer.effectAllowed = 'move';
        setTimeout(() => div.classList.add('dragging'), 0);
      });
      
      div.addEventListener('dragend', () => {
        div.classList.remove('dragging');
        
        let newSequence = [];
        let newEditingIndex = -1;
        const listItems = document.getElementById("sequenceList").querySelectorAll('.sequence-item');
        listItems.forEach((item, i) => {
          let oldIndex = parseInt(item.dataset.index);
          newSequence.push(sequence[oldIndex]);
          if (oldIndex === editingIndex) {
            newEditingIndex = i;
          }
        });
        sequence = newSequence;
        editingIndex = newEditingIndex;
        renderSequence(); 
      });
      
      div.addEventListener('dragover', (e) => {
        e.preventDefault();
        const draggingEl = document.querySelector('.dragging');
        if (!draggingEl || draggingEl === div) return;
        
        const bounding = div.getBoundingClientRect();
        const offset = e.clientY - bounding.top;
        if (offset > bounding.height / 2) {
          div.parentNode.insertBefore(draggingEl, div.nextSibling);
        } else {
          div.parentNode.insertBefore(draggingEl, div);
        }
      });
      
      div.innerHTML = `
        <div style="display:flex; align-items:center; gap: 8px;">
          <span style="display:inline-block; color:var(--text-muted); font-weight:bold; cursor:grab; margin-right: 5px;">&#9776;</span>
          <span style="display:inline-block; width:15px; color:var(--text-muted); font-weight:bold;">${index+1}.</span> 
          <div class="mini-canvas-container">
            <canvas id="miniSide_${index}" class="mini-canvas" width="40" height="40"></canvas>
            <canvas id="miniFront_${index}" class="mini-canvas" width="40" height="40"></canvas>
          </div>
          <div class="sequence-coords">
            X: ${step.x}<br>Y: ${step.y}<br>Z: ${step.z}
          </div>
        </div>
        <div class="sequence-actions">
          <button class="btn-small" onclick="playStep(${index}, event)" ${isGait ? 'disabled' : ''}>Play</button>
          <button class="btn-small btn-danger" onclick="deleteStep(${index}, event)">X</button>
        </div>
      `;
      list.appendChild(div);
      
      // Draw the mini canvases right after appending
      let data = getPoints(step.x, step.y, step.z);
      if (data) {
        drawMiniView("miniSide_"+index, data.pts, 75, 20, "x", "#3b82f6");
        drawMiniView("miniFront_"+index, data.pts, 40, 20, "y", "#a78bfa");
      }
    });
  }

  function toggleGait() {
    let btn = document.getElementById("playGaitBtn");
    isGait = !isGait;
    
    if (!isGait) {
      btn.innerText = "Play Gait";
      btn.className = "btn-success";
      fetch(`/gait?enable=false`).catch(err => console.error(err));
      renderSequence(); 
    } else {
      if (sequence.length === 0) {
        alert("Add some steps first!");
        isGait = false;
        return;
      }
      setEditing(-1); // stop editing if we start gait
      btn.innerText = "Stop Gait";
      btn.className = "btn-danger";
      renderSequence(); 
      
      let seqString = sequence.map(s => `${s.x},${s.y},${s.z}`).join(';');
      
      fetch('/upload_gait', {
        method: 'POST',
        body: seqString
      }).catch(err => {
        console.error(err);
        toggleGait(); 
      });
    }
  }

  window.onload = () => {
    updateValues();
    renderSequence();
  };
</script>
</body>
</html>
)rawliteral";

#endif
