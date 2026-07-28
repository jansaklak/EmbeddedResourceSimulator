let currentMode = 'file';
let currentSimData = null;
let currentFileList = [];

document.addEventListener('DOMContentLoaded', () => {
  loadFileList();

  document.getElementById('fileSelect').addEventListener('change', (e) => {
    if (e.target.value) {
      document.getElementById('currentFileBadge').textContent = `Plik: ${e.target.value}`;
      loadEditorContent(e.target.value);
    }
  });

  // Auto-run simulation on startup
  setTimeout(() => {
    runSimulation();
  }, 300);
});

function setSourceMode(mode) {
  currentMode = mode;
  document.getElementById('modeFileBtn').classList.toggle('active', mode === 'file');
  document.getElementById('modeRandBtn').classList.toggle('active', mode === 'random');
  document.getElementById('fileControlsGroup').classList.toggle('hidden', mode !== 'file');
  document.getElementById('randControlsGroup').classList.toggle('hidden', mode !== 'random');
}

async function loadFileList() {
  try {
    const res = await fetch('/api/files');
    const files = await res.json();
    currentFileList = files;
    const select = document.getElementById('fileSelect');
    select.innerHTML = '';
    if (files.length === 0) {
      select.innerHTML = '<option value="">Brak plików w data/</option>';
      return;
    }
    files.forEach(f => {
      const opt = document.createElement('option');
      opt.value = f.name;
      opt.textContent = `${f.name} (${Math.round(f.size / 1024 * 10) / 10} KB)`;
      if (f.name === 'graph20.dat') opt.selected = true;
      select.appendChild(opt);
    });
    if (select.value) {
      document.getElementById('currentFileBadge').textContent = `Plik: ${select.value}`;
      loadEditorContent(select.value);
    }
  } catch (err) {
    logConsole("Błąd ładowania listy plików: " + err.message);
  }
}

async function loadEditorContent(filename) {
  try {
    const res = await fetch(`/api/file-content?name=${encodeURIComponent(filename)}`);
    if (res.ok) {
      const text = await res.text();
      document.getElementById('fileEditorTextarea').value = text;
      document.getElementById('editorTitle').textContent = `Podgląd Pliku: ${filename}`;
    }
  } catch (err) {
    logConsole("Błąd odczytu pliku: " + err.message);
  }
}

async function saveEditorContent() {
  const filename = document.getElementById('fileSelect').value || 'new_graph.dat';
  const content = document.getElementById('fileEditorTextarea').value;
  try {
    const res = await fetch('/api/save-file', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ filename, content })
    });
    const data = await res.json();
    if (data.success) {
      logConsole(`Pomyślnie zapisano plik ${data.filename}`);
      loadFileList();
    }
  } catch (err) {
    logConsole("Błąd zapisu pliku: " + err.message);
  }
}

async function runSimulation() {
  const strategy = parseInt(document.getElementById('strategySelect').value, 10);
  let payload = { strategy };

  if (currentMode === 'random') {
    payload.random = true;
    payload.tasks = parseInt(document.getElementById('randTasks').value, 10);
    payload.hc = parseInt(document.getElementById('randHC').value, 10);
    payload.pe = parseInt(document.getElementById('randPE').value, 10);
    payload.channels = parseInt(document.getElementById('randChannels').value, 10);
    payload.withCost = document.getElementById('randWithCost').checked;
    payload.conditional = document.getElementById('randConditional').checked;
    logConsole(`Uruchamianie losowej symulacji: Zadań=${payload.tasks}, Strategy=${strategy}...`);
  } else {
    const filename = document.getElementById('fileSelect').value;
    if (!filename) {
      alert("Proszę wybrać plik danych!");
      return;
    }
    payload.random = false;
    payload.filename = filename;
    logConsole(`Uruchamianie symulacji dla pliku ${filename} ze strategią ${strategy}...`);
  }

  try {
    const res = await fetch('/api/run-simulation', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    });
    const data = await res.json();
    if (data.error) {
      logConsole("BŁĄD SYMULACJI: " + data.error + "\n" + (data.stderr || ''));
      return;
    }

    currentSimData = data;
    logConsole(data.log || "Symulacja zakończona pomyślnie!");

    const activeInstancesCount = data.activeInstancesCount !== undefined 
      ? data.activeInstancesCount 
      : (new Set((data.schedule || []).map(s => s.unit).filter(Boolean)).size);
    document.getElementById('quickInstances').textContent = `${activeInstancesCount} instancji`;

    // Keep live strategy selector in sync
    const liveSelect = document.getElementById('liveStrategySelect');
    if (liveSelect) liveSelect.value = strategy;

    // Init Live Simulator
    LiveSimulator.init(data);

    // Render active views
    renderGantt();
    renderDag();

  } catch (err) {
    logConsole("Błąd komunikacji z serwerem: " + err.message);
  }
}

async function changeLiveStrategy(newStrat) {
  document.getElementById('strategySelect').value = newStrat;
  await runSimulation();
  switchView('liveSim');
  LiveSimulator.play();
}

/* =========================================================
   LIVE SIMULATOR ENGINE & PLAYBACK CONTROLLER
   ========================================================= */
const LiveSimulator = {
  currentTime: 0,
  maxTime: 100,
  isPlaying: false,
  timerId: null,
  speedMs: 200,
  speedRatio: 1.0,

  init(data) {
    if (!data || !data.schedule) return;
    this.pause();
    this.currentTime = 0;
    this.maxTime = data.criticalTime || 1;
    this.render();
  },

  togglePlay() {
    if (this.isPlaying) {
      this.pause();
    } else {
      this.play();
    }
  },

  play() {
    if (this.currentTime >= this.maxTime) {
      this.currentTime = 0;
    }
    this.isPlaying = true;
    this.updatePlayBtnUI();
    this.scheduleNextTick();
  },

  pause() {
    this.isPlaying = false;
    if (this.timerId) {
      clearTimeout(this.timerId);
      this.timerId = null;
    }
    this.updatePlayBtnUI();
  },

  step() {
    this.pause();
    if (this.currentTime < this.maxTime) {
      this.currentTime++;
      this.render();
    }
  },

  reset() {
    this.pause();
    this.currentTime = 0;
    this.render();
  },

  scheduleNextTick() {
    if (!this.isPlaying) return;
    this.timerId = setTimeout(() => {
      if (this.currentTime < this.maxTime) {
        this.currentTime++;
        this.render();
        this.scheduleNextTick();
      } else {
        this.pause();
      }
    }, Math.max(10, Math.round(this.speedMs / this.speedRatio)));
  },

  onSpeedChange(val) {
    this.speedMs = parseInt(val, 10);
    const badge = document.getElementById('simTempoVal');
    if (badge) badge.textContent = `${this.speedMs} ms`;
  },

  setSpeedRatio(ratio) {
    this.speedRatio = ratio;
    document.querySelectorAll('.btn-speed').forEach(b => b.classList.remove('active'));
    const target = Array.from(document.querySelectorAll('.btn-speed')).find(b => b.textContent === `${ratio}x`);
    if (target) target.classList.add('active');
  },

  updatePlayBtnUI() {
    const icon = document.getElementById('livePlayIcon');
    const text = document.getElementById('livePlayText');
    const btn = document.getElementById('btnLivePlay');
    if (this.isPlaying) {
      if (icon) icon.textContent = '⏸';
      if (text) text.textContent = 'Pauza';
      if (btn) btn.className = 'btn btn-secondary btn-sm';
    } else {
      if (icon) icon.textContent = '▶';
      if (text) text.textContent = 'Uruchom';
      if (btn) btn.className = 'btn btn-primary btn-sm';
    }
  },

  render() {
    if (!currentSimData || !currentSimData.schedule) return;

    const t = this.currentTime;
    const digitEl = document.getElementById('liveTimeDigit');
    const maxEl = document.getElementById('liveTimeMax');
    const barEl = document.getElementById('liveProgressBar');

    if (digitEl) digitEl.textContent = `t = ${t}`;
    if (maxEl) maxEl.textContent = `/ ${this.maxTime} ms`;
    const pct = Math.min(100, Math.round((t / (this.maxTime || 1)) * 100));
    if (barEl) barEl.style.width = `${pct}%`;

    // Map resource states
    const unitsMap = {};
    currentSimData.schedule.forEach(item => {
      const u = item.unit || `HW_${item.hwId}`;
      if (!unitsMap[u]) {
        unitsMap[u] = {
          unit: u,
          hwId: item.hwId,
          activeTask: null,
          tasks: []
        };
      }
      unitsMap[u].tasks.push(item);
    });

    Object.values(unitsMap).forEach(uObj => {
      uObj.tasks.forEach(task => {
        if (t >= task.startTime && t < task.endTime) {
          uObj.activeTask = task;
        }
      });
    });

    // Render Resource Cards
    const resCardsGrid = document.getElementById('liveResourceCards');
    if (resCardsGrid) {
      resCardsGrid.innerHTML = '';
      Object.values(unitsMap).sort((a,b) => a.unit.localeCompare(b.unit)).forEach(uObj => {
        const card = document.createElement('div');
        const isActive = !!uObj.activeTask;
        card.className = `res-card ${isActive ? 'active' : ''}`;

        let currentTaskHtml = `<div class="res-current-task" style="color: var(--text-muted);">Bezczynny</div>`;
        let progressPct = 0;
        if (uObj.activeTask) {
          const taskDuration = uObj.activeTask.endTime - uObj.activeTask.startTime;
          const taskElapsed = t - uObj.activeTask.startTime;
          progressPct = Math.min(100, Math.round((taskElapsed / (taskDuration || 1)) * 100));
          currentTaskHtml = `
            <div class="res-current-task">
              <strong style="color: var(--primary);">Zadanie T${uObj.activeTask.taskId}</strong> 
              (${uObj.activeTask.startTime} → ${uObj.activeTask.endTime}ms)
            </div>
          `;
        }

        card.innerHTML = `
          <div class="res-card-header">
            <span class="res-title">${uObj.unit}</span>
            <span class="res-badge ${isActive ? 'running' : 'idle'}">
              ${isActive ? '🟢 AKTYWNY' : '⚪ BEZCZYNNY'}
            </span>
          </div>
          ${currentTaskHtml}
          <div class="res-progress">
            <div class="res-progress-fill" style="width: ${progressPct}%;"></div>
          </div>
          <div class="res-metrics">
            <span>Zadań: <strong>${uObj.tasks.length}</strong></span>
            <span>Postęp: <strong>${progressPct}%</strong></span>
          </div>
        `;
        resCardsGrid.appendChild(card);
      });
    }

    // Categorize Tasks
    const running = [];
    const completed = [];
    const pending = [];

    const tasksCount = currentSimData.tasksCount || 0;
    const taskScheduleMap = {};
    currentSimData.schedule.forEach(item => {
      taskScheduleMap[item.taskId] = item;
    });

    for (let id = 0; id < tasksCount; id++) {
      const item = taskScheduleMap[id];
      if (!item) {
        pending.push(id);
        continue;
      }
      if (t >= item.endTime) {
        completed.push(id);
      } else if (t >= item.startTime && t < item.endTime) {
        running.push({ id, unit: item.unit });
      } else {
        pending.push(id);
      }
    }

    const cntRun = document.getElementById('countRunningTasks');
    const cntPen = document.getElementById('countPendingTasks');
    const cntCom = document.getElementById('countCompletedTasks');

    if (cntRun) cntRun.textContent = running.length;
    if (cntPen) cntPen.textContent = pending.length;
    if (cntCom) cntCom.textContent = completed.length;

    const listRunning = document.getElementById('listRunningTasks');
    if (listRunning) {
      listRunning.innerHTML = running.length
        ? running.map(r => `<span class="task-pill running">T${r.id} (${r.unit})</span>`).join('')
        : '<span style="font-size: 0.75rem; color: var(--text-muted);">Brak aktywnych</span>';
    }

    const listPending = document.getElementById('listPendingTasks');
    if (listPending) {
      listPending.innerHTML = pending.length
        ? pending.map(id => `<span class="task-pill pending">T${id}</span>`).join('')
        : '<span style="font-size: 0.75rem; color: var(--text-muted);">Brak oczekujących</span>';
    }

    const listCompleted = document.getElementById('listCompletedTasks');
    if (listCompleted) {
      listCompleted.innerHTML = completed.length
        ? completed.map(id => `<span class="task-pill completed">T${id}</span>`).join('')
        : '<span style="font-size: 0.75rem; color: var(--text-muted);">Brak ukończonych</span>';
    }
  }
};

function renderGantt() {
  if (!currentSimData || !currentSimData.schedule) return;

  const container = document.getElementById('ganttChart');
  container.innerHTML = '';

  const zoomFactor = parseFloat(document.getElementById('ganttZoom').value);
  document.getElementById('ganttZoomVal').textContent = `${zoomFactor}x`;

  const scale = 3 * zoomFactor; // pixels per time unit
  const schedule = currentSimData.schedule;
  const criticalTime = currentSimData.criticalTime || 100;

  // Group schedule by unit
  const unitsMap = {};
  schedule.forEach(item => {
    const u = item.unit || 'HW';
    if (!unitsMap[u]) unitsMap[u] = [];
    unitsMap[u].push(item);
  });

  // Create timeline header
  const headerRow = document.createElement('div');
  headerRow.className = 'gantt-row';
  headerRow.innerHTML = `<div class="gantt-label">Sprzęt / Czas</div><div class="gantt-track" style="width: ${criticalTime * scale}px"></div>`;
  container.appendChild(headerRow);

  // Render rows
  Object.keys(unitsMap).sort().forEach(unitName => {
    const row = document.createElement('div');
    row.className = 'gantt-row';

    const label = document.createElement('div');
    label.className = 'gantt-label';
    label.textContent = unitName;
    row.appendChild(label);

    const track = document.createElement('div');
    track.className = 'gantt-track';
    track.style.width = `${criticalTime * scale}px`;

    unitsMap[unitName].forEach(t => {
      const bar = document.createElement('div');
      bar.className = 'gantt-bar';
      
      const taskObj = (currentSimData.tasks || []).find(x => x.id === t.taskId);
      if (taskObj && taskObj.isConditional) bar.classList.add('conditional');
      if (taskObj && taskObj.isUnpredicted) bar.classList.add('unpredicted');

      const left = t.startTime * scale;
      const width = Math.max((t.endTime - t.startTime) * scale, 24);

      bar.style.left = `${left}px`;
      bar.style.width = `${width}px`;
      bar.textContent = `T${t.taskId}`;
      bar.title = `Zadanie T${t.taskId}\nCzas: ${t.startTime} -> ${t.endTime} (Czas trwania: ${t.endTime - t.startTime})\nJednostka: ${unitName}`;

      track.appendChild(bar);
    });

    row.appendChild(track);
    container.appendChild(row);
  });
}

function renderDag() {
  if (!currentSimData || !currentSimData.tasks) return;

  const svg = document.getElementById('dagSvg');
  if (!svg) return;
  svg.innerHTML = '';

  const tasks = currentSimData.tasks;
  const numTasks = tasks.length;
  if (numTasks === 0) return;

  // Add marker defs for arrowheads
  const defs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');
  defs.innerHTML = `
    <marker id="arrow" viewBox="0 0 10 10" refX="26" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">
      <path d="M 0 0 L 10 5 L 0 10 z" fill="#38bdf8" />
    </marker>
    <marker id="arrow-active" viewBox="0 0 10 10" refX="26" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">
      <path d="M 0 0 L 10 5 L 0 10 z" fill="#06b6d4" />
    </marker>
  `;
  svg.appendChild(defs);

  // Layered topological layout algorithm
  const layers = [];
  const visited = new Set();
  
  const inDegree = new Array(numTasks).fill(0);
  tasks.forEach(t => {
    (t.outEdges || []).forEach(e => {
      if (e.target < numTasks) inDegree[e.target]++;
    });
  });

  let currentLayer = [];
  for (let i = 0; i < numTasks; i++) {
    if (inDegree[i] === 0) currentLayer.push(i);
  }
  if (currentLayer.length === 0) currentLayer.push(0);

  layers.push(currentLayer);
  currentLayer.forEach(id => visited.add(id));

  while (visited.size < numTasks) {
    const nextLayer = [];
    currentLayer.forEach(nodeId => {
      const node = tasks.find(x => x.id === nodeId);
      if (node && node.outEdges) {
        node.outEdges.forEach(e => {
          if (!visited.has(e.target) && !nextLayer.includes(e.target)) {
            nextLayer.push(e.target);
          }
        });
      }
    });

    if (nextLayer.length === 0) {
      for (let i = 0; i < numTasks; i++) {
        if (!visited.has(i)) {
          nextLayer.push(i);
          break;
        }
      }
    }

    layers.push(nextLayer);
    nextLayer.forEach(id => visited.add(id));
    currentLayer = nextLayer;
  }

  // Calculate coordinates & dimensions
  const nodeCoords = {};
  const layerWidth = 160;
  const nodeHeight = 70;

  let maxLayerSize = 0;
  layers.forEach(l => { if (l.length > maxLayerSize) maxLayerSize = l.length; });

  const totalWidth = Math.max(900, layers.length * layerWidth + 180);
  const totalHeight = Math.max(500, maxLayerSize * nodeHeight + 140);

  svg.setAttribute('width', totalWidth);
  svg.setAttribute('height', totalHeight);
  svg.setAttribute('viewBox', `0 0 ${totalWidth} ${totalHeight}`);

  layers.forEach((layer, layerIdx) => {
    const x = 90 + layerIdx * layerWidth;
    const layerH = layer.length * nodeHeight;
    const startY = (totalHeight - layerH) / 2 + 35;

    layer.forEach((nodeId, idx) => {
      const y = startY + idx * nodeHeight;
      nodeCoords[nodeId] = { x, y };
    });
  });

  // Render Edges
  tasks.forEach(t => {
    const source = nodeCoords[t.id];
    if (!source) return;

    (t.outEdges || []).forEach(e => {
      const target = nodeCoords[e.target];
      if (target) {
        const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
        line.setAttribute('x1', source.x);
        line.setAttribute('y1', source.y);
        line.setAttribute('x2', target.x);
        line.setAttribute('y2', target.y);
        line.setAttribute('stroke', '#38bdf8');
        line.setAttribute('stroke-width', '3');
        line.setAttribute('stroke-dasharray', '6 3');
        line.setAttribute('marker-end', 'url(#arrow)');
        line.setAttribute('class', 'dag-edge');

        const title = document.createElementNS('http://www.w3.org/2000/svg', 'title');
        title.textContent = `T${t.id} → T${e.target} (Koszt comm: ${e.weight})`;
        line.appendChild(title);

        svg.appendChild(line);
      }
    });
  });

  // Render Nodes
  tasks.forEach(t => {
    const coord = nodeCoords[t.id];
    if (!coord) return;

    const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    g.setAttribute('transform', `translate(${coord.x}, ${coord.y})`);

    const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    circle.setAttribute('cx', '0');
    circle.setAttribute('cy', '0');
    circle.setAttribute('r', '22');
    
    let fillColor = '#0284c7';   // Vibrant Blue
    let strokeColor = '#38bdf8'; // Light cyan border
    if (t.isConditional) {
      fillColor = '#db2777';   // Vibrant Pink
      strokeColor = '#f472b6'; // Light pink border
    } else if (t.isUnpredicted) {
      fillColor = '#d97706';   // Vibrant Amber
      strokeColor = '#fbbf24'; // Light yellow border
    }

    circle.setAttribute('fill', fillColor);
    circle.setAttribute('stroke', strokeColor);
    circle.setAttribute('stroke-width', '3.5');
    circle.setAttribute('class', 'dag-node');

    const title = document.createElementNS('http://www.w3.org/2000/svg', 'title');
    let nodeInfo = `Zadanie T${t.id}`;
    if (t.isConditional) nodeInfo += `\nWarunek: ${t.condition || 'TAK'}`;
    if (t.isUnpredicted) nodeInfo += `\nNieprzewidziane`;
    title.textContent = nodeInfo;
    g.appendChild(title);

    const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    text.setAttribute('x', '0');
    text.setAttribute('y', '0');
    text.setAttribute('fill', '#ffffff');
    text.setAttribute('font-size', '14px');
    text.setAttribute('font-weight', '900');
    text.setAttribute('font-family', 'monospace, sans-serif');
    text.setAttribute('text-anchor', 'middle');
    text.setAttribute('dominant-baseline', 'central');
    text.setAttribute('alignment-baseline', 'middle');
    text.setAttribute('dy', '0.3em');
    text.textContent = `T${t.id}`;

    g.appendChild(circle);
    g.appendChild(text);
    svg.appendChild(g);
  });
}

async function runBenchmark() {
  const filename = document.getElementById('fileSelect').value || 'graph20.dat';

  const statusBox = document.getElementById('benchmarkStatusBox');
  const statusText = document.getElementById('benchmarkStatusText');
  const progressBar = document.getElementById('benchmarkProgressBar');
  const cardsContainer = document.getElementById('benchmarkCards');
  const barChartContainer = document.getElementById('benchmarkBarChart');

  if (statusBox) statusBox.classList.remove('hidden');
  if (cardsContainer) cardsContainer.innerHTML = '';
  if (barChartContainer) barChartContainer.innerHTML = '';

  const stratList = [
    { id: 1, name: 'S1: Najszybsza Dedykowana' },
    { id: 2, name: 'S2: Najtańsza Dedykowana' },
    { id: 3, name: 'S3: Najszybsza z Upakowywaniem' },
    { id: 5, name: 'S5: Poziomowa BFS' },
    { id: 6, name: 'S6: Zachłanna Ścieżki Krytycznej' },
    { id: 7, name: 'S7: Hybrydowa z Rafinacją' },
    { id: 8, name: 'S8: Optymalizacja z Funkcją Kary' },
    { id: 9, name: 'S9: Monolityczna Baseline' }
  ];

  logConsole(`=======================================================`);
  logConsole(`📊 ROZPOCZYNANIE BENCHMARKU STRATEGII DLA PLIKU: ${filename}`);
  logConsole(`=======================================================`);

  const results = [];
  const totalStrats = stratList.length;

  for (let i = 0; i < totalStrats; i++) {
    const s = stratList[i];
    const pct = Math.round(((i + 1) / totalStrats) * 100);
    if (progressBar) progressBar.style.width = `${pct}%`;
    if (statusText) statusText.textContent = `⏳ [${i + 1}/${totalStrats}] Testowanie: ${s.name}...`;
    logConsole(`⏳ [${i + 1}/${totalStrats}] Wykonywanie symulacji dla ${s.name} (ID: ${s.id})...`);

    try {
      const payload = { strategy: s.id, random: false, filename };
      const res = await fetch('/api/run-simulation', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });
      const data = await res.json();
        const activeCount = data.activeInstancesCount !== undefined 
          ? data.activeInstancesCount 
          : (new Set((data.schedule || []).map(x => x.unit).filter(Boolean)).size);
        const resObj = {
          strategy: s.id,
          name: s.name,
          criticalTime: data.criticalTime,
          totalCost: data.totalCost,
          hardwareCount: activeCount
        };
        results.push(resObj);
        logConsole(`  ✅ ${s.name} -> Czas Krytyczny: ${data.criticalTime} ms | Koszt Total: ${data.totalCost} PLN | Instancji HW: ${resObj.hardwareCount}`);
        renderBenchmarkResults(results);
      } else {
        logConsole(`  ❌ Błąd dla ${s.name}: ${data.error}`);
      }
    } catch (err) {
      logConsole(`  ❌ Błąd komunikacji dla ${s.name}: ${err.message}`);
    }
  }

  if (statusText) statusText.textContent = `✅ Benchmark ukończony! Przeanalizowano wszystkie ${results.length} strategii.`;
  logConsole(`=======================================================`);
  logConsole(`🏆 BENCHMARK ZAKOŃCZONY SUKCESEM dla ${results.length} strategii.`);
  logConsole(`=======================================================`);

  setTimeout(() => {
    if (statusBox) statusBox.classList.add('hidden');
  }, 4000);
}

function renderBenchmarkResults(results) {
  const cardsContainer = document.getElementById('benchmarkCards');
  const barChartContainer = document.getElementById('benchmarkBarChart');

  if (!cardsContainer || !barChartContainer) return;
  cardsContainer.innerHTML = '';
  barChartContainer.innerHTML = '';

  let minTime = Infinity;
  let winnerStrat = -1;
  results.forEach(r => {
    if (r.criticalTime > 0 && r.criticalTime < minTime) {
      minTime = r.criticalTime;
      winnerStrat = r.strategy;
    }
  });

  const stratNames = {
    1: 'S1: Najszybsza Dedykowana',
    2: 'S2: Najtańsza Dedykowana',
    3: 'S3: Najszybsza z Upakowywaniem',
    5: 'S5: Poziomowa BFS',
    6: 'S6: Zachłanna Ścieżki Krytycznej',
    7: 'S7: Hybrydowa z Rafinacją',
    8: 'S8: Optymalizacja z Funkcją Kary',
    9: 'S9: Monolityczna Baseline'
  };

  const maxTime = Math.max(...results.map(r => r.criticalTime || 1), 1);

  results.forEach(r => {
    const card = document.createElement('div');
    card.className = `b-card ${r.strategy === winnerStrat ? 'winner' : ''}`;
    card.innerHTML = `
      <div class="b-card-title">${stratNames[r.strategy] || 'Strategia ' + r.strategy} ${r.strategy === winnerStrat ? '🏆 WINNER' : ''}</div>
      <div class="b-card-metric"><span>Czas Krytyczny:</span> <strong>${r.criticalTime || '-'} ms</strong></div>
      <div class="b-card-metric"><span>Koszt Total:</span> <strong>${r.totalCost || '-'} PLN</strong></div>
      <div class="b-card-metric"><span>Instancje HW:</span> <strong>${r.hardwareCount || '-'}</strong></div>
    `;
    cardsContainer.appendChild(card);

    // Bar chart group
    const barGroup = document.createElement('div');
    barGroup.className = 'chart-bar-group';

    const heightPct = Math.round((r.criticalTime / maxTime) * 100);
    barGroup.innerHTML = `
      <div class="chart-bar" style="height: ${heightPct}%;" title="${stratNames[r.strategy]}: Czas=${r.criticalTime}ms, Koszt=${r.totalCost} PLN"></div>
      <span class="chart-bar-label">S${r.strategy}</span>
    `;
    barChartContainer.appendChild(barGroup);
  });
}

function switchView(viewName) {
  document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
  document.querySelectorAll('.view-panel').forEach(panel => panel.classList.remove('active'));

  if (viewName === 'gantt') {
    document.querySelector("button[onclick=\"switchView('gantt')\"]").classList.add('active');
    document.getElementById('viewGantt').classList.add('active');
    renderGantt();
  } else if (viewName === 'liveSim') {
    document.querySelector("button[onclick=\"switchView('liveSim')\"]").classList.add('active');
    document.getElementById('viewLiveSim').classList.add('active');
    if (currentSimData) LiveSimulator.init(currentSimData);
  } else if (viewName === 'dag') {
    document.querySelector("button[onclick=\"switchView('dag')\"]").classList.add('active');
    document.getElementById('viewDag').classList.add('active');
    renderDag();
  } else if (viewName === 'benchmark') {
    document.querySelector("button[onclick=\"switchView('benchmark')\"]").classList.add('active');
    document.getElementById('viewBenchmark').classList.add('active');
    runBenchmark();
  } else if (viewName === 'editor') {
    document.querySelector("button[onclick=\"switchView('editor')\"]").classList.add('active');
    document.getElementById('viewEditor').classList.add('active');
  }
}

function logConsole(msg) {
  const consoleEl = document.getElementById('consoleOutput');
  consoleEl.textContent += '\n' + msg;
  consoleEl.scrollTop = consoleEl.scrollHeight;
}

function clearConsole() {
  document.getElementById('consoleOutput').textContent = '';
}
