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

    // Update Quick Stats
    document.getElementById('quickCritTime').textContent = `${data.criticalTime} j.c.`;
    document.getElementById('quickTotalCost').textContent = `${data.totalCost} PLN`;
    document.getElementById('quickInstances').textContent = `${data.hardware ? data.hardware.length : 0} jednostek`;

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
  svg.innerHTML = '';

  const tasks = currentSimData.tasks;
  const numTasks = tasks.length;
  if (numTasks === 0) return;

  // Add marker defs for arrowheads
  const defs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');
  defs.innerHTML = `
    <marker id="arrow" viewBox="0 0 10 10" refX="28" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
      <path d="M 0 0 L 10 5 L 0 10 z" fill="rgba(148, 163, 184, 0.6)" />
    </marker>
    <marker id="arrow-active" viewBox="0 0 10 10" refX="28" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
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
  const layerWidth = 150;
  const nodeHeight = 65;

  let maxLayerSize = 0;
  layers.forEach(l => { if (l.length > maxLayerSize) maxLayerSize = l.length; });

  const totalWidth = Math.max(800, layers.length * layerWidth + 160);
  const totalHeight = Math.max(480, maxLayerSize * nodeHeight + 120);

  svg.setAttribute('width', totalWidth);
  svg.setAttribute('height', totalHeight);
  svg.setAttribute('viewBox', `0 0 ${totalWidth} ${totalHeight}`);

  layers.forEach((layer, layerIdx) => {
    const x = 80 + layerIdx * layerWidth;
    const layerH = layer.length * nodeHeight;
    const startY = (totalHeight - layerH) / 2 + 30;

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
    circle.setAttribute('r', '20');
    
    let cls = 'dag-node';
    if (t.isConditional) cls += ' conditional';
    if (t.isUnpredicted) cls += ' unpredicted';
    circle.setAttribute('class', cls);

    const title = document.createElementNS('http://www.w3.org/2000/svg', 'title');
    let nodeInfo = `Zadanie T${t.id}`;
    if (t.isConditional) nodeInfo += `\nWarunek: ${t.condition || 'TAK'}`;
    if (t.isUnpredicted) nodeInfo += `\nZadanie Nieprzewidziane`;
    title.textContent = nodeInfo;
    g.appendChild(title);

    const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    text.setAttribute('class', 'dag-node-text');
    text.setAttribute('dy', '1');
    text.setAttribute('text-anchor', 'middle');
    text.textContent = `T${t.id}`;

    g.appendChild(circle);
    g.appendChild(text);
    svg.appendChild(g);
  });
}

async function runBenchmark() {
  const filename = document.getElementById('fileSelect').value || 'graph20.dat';
  logConsole(`Uruchamianie benchmarku wszystkich strategii dla ${filename}...`);

  try {
    const res = await fetch('/api/benchmark', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ filename })
    });
    const data = await res.json();
    if (data.results) {
      renderBenchmarkResults(data.results);
      switchView('benchmark');
      logConsole("Benchmark wykonany pomyślnie na wszystkich rdzeniach!");
    }
  } catch (err) {
    logConsole("Błąd wykonywania benchmarku: " + err.message);
  }
}

function renderBenchmarkResults(results) {
  const cardsContainer = document.getElementById('benchmarkCards');
  const barChartContainer = document.getElementById('benchmarkBarChart');

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
      <div class="b-card-title">${stratNames[r.strategy] || 'Strategia ' + r.strategy} ${r.strategy === winnerStrat ? '🏆' : ''}</div>
      <div class="b-card-metric"><span>Czas Krytyczny:</span> <strong>${r.criticalTime || '-'}</strong></div>
      <div class="b-card-metric"><span>Koszt Total:</span> <strong>${r.totalCost || '-'}</strong></div>
      <div class="b-card-metric"><span>Instancje HW:</span> <strong>${r.hardwareCount || '-'}</strong></div>
    `;
    cardsContainer.appendChild(card);

    // Bar chart group
    const barGroup = document.createElement('div');
    barGroup.className = 'chart-bar-group';

    const heightPct = Math.round((r.criticalTime / maxTime) * 100);
    barGroup.innerHTML = `
      <div class="chart-bar" style="height: ${heightPct}%;" title="Czas: ${r.criticalTime}, Koszt: ${r.totalCost}"></div>
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
