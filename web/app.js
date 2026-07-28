let currentMode = 'file';
let currentSimData = null;
let currentFileList = [];
let benchmarkRunning = false;
let lastBenchmarkResults = null;

let dagZoomScale = 1.0;
let dagCustomNodeCoords = {};
let isDraggingDagNode = false;
let draggedNodeId = null;

function zoomDag(delta) {
  dagZoomScale = Math.min(3.0, Math.max(0.3, dagZoomScale + delta));
  const zoomVal = document.getElementById('dagZoomVal');
  if (zoomVal) zoomVal.textContent = Math.round(dagZoomScale * 100) + '%';
  const containerGroup = document.getElementById('dagContainerGroup');
  if (containerGroup) {
    containerGroup.setAttribute('transform', `scale(${dagZoomScale})`);
  }
}

function resetDagZoom() {
  dagZoomScale = 1.0;
  dagCustomNodeCoords = {};
  const zoomVal = document.getElementById('dagZoomVal');
  if (zoomVal) zoomVal.textContent = '100%';
  renderDag();
}

document.addEventListener('DOMContentLoaded', async () => {
  await loadFileList();

  const fileSel = document.getElementById('fileSelect');
  if (fileSel) {
    fileSel.addEventListener('change', (e) => {
      if (e.target.value) {
        document.getElementById('currentFileBadge').textContent = `Plik: ${e.target.value}`;
        loadEditorContent(e.target.value);
      }
    });
  }

  // Run initial simulation safely after files are loaded
  runSimulation();
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
    if (!select) return;
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
      await loadEditorContent(select.value);
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
  const strategySelect = document.getElementById('strategySelect');
  const strategy = strategySelect ? parseInt(strategySelect.value, 10) : 8;
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
    const select = document.getElementById('fileSelect');
    const filename = select ? select.value : '';
    if (!filename) {
      logConsole("Oczekiwanie na wybór pliku danych...");
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
      
    if (document.getElementById('quickCritTime')) {
      document.getElementById('quickCritTime').textContent = `${data.criticalTime !== undefined ? data.criticalTime : 0} ms`;
    }
    if (document.getElementById('quickTotalCost')) {
      document.getElementById('quickTotalCost').textContent = `${data.totalCost !== undefined ? data.totalCost : 0} PLN`;
    }
    if (document.getElementById('quickInstances')) {
      document.getElementById('quickInstances').textContent = `${activeInstancesCount} instancji`;
    }

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

  // Create defs for arrow markers (blue normal & red critical)
  const defs = document.createElementNS('http://www.w3.org/2000/svg', 'defs');
  defs.innerHTML = `
    <marker id="arrow" viewBox="0 0 10 10" refX="28" refY="5" markerWidth="6" markerHeight="6" orient="auto-start-reverse">
      <path d="M 0 0 L 10 5 L 0 10 z" fill="#38bdf8" />
    </marker>
    <marker id="arrow-red" viewBox="0 0 10 10" refX="28" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">
      <path d="M 0 0 L 10 5 L 0 10 z" fill="#ef4444" />
    </marker>
  `;
  svg.appendChild(defs);

  // Main container group for zooming
  const containerGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  containerGroup.setAttribute('id', 'dagContainerGroup');
  containerGroup.setAttribute('transform', `scale(${dagZoomScale})`);
  containerGroup.style.transformOrigin = '0 0';
  svg.appendChild(containerGroup);

  // Mouse wheel zoom event
  svg.onwheel = (e) => {
    e.preventDefault();
    const delta = e.deltaY < 0 ? 0.15 : -0.15;
    zoomDag(delta);
  };

  const tasks = currentSimData.tasks;
  const numTasks = tasks.length;
  if (numTasks === 0) return;

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

  svg.setAttribute('width', totalWidth * Math.max(1, dagZoomScale));
  svg.setAttribute('height', totalHeight * Math.max(1, dagZoomScale));
  svg.setAttribute('viewBox', `0 0 ${totalWidth} ${totalHeight}`);

  layers.forEach((layer, layerIdx) => {
    const x = 90 + layerIdx * layerWidth;
    const layerH = layer.length * nodeHeight;
    const startY = (totalHeight - layerH) / 2 + 35;

    layer.forEach((nodeId, idx) => {
      const defaultY = startY + idx * nodeHeight;
      if (dagCustomNodeCoords[nodeId]) {
        nodeCoords[nodeId] = { ...dagCustomNodeCoords[nodeId] };
      } else {
        nodeCoords[nodeId] = { x, y: defaultY };
      }
    });
  });

  // Calculate Critical Path before rendering
  const isCritPathChecked = document.getElementById('chkCriticalPath')?.checked;
  const critPathTasks = new Set();
  const critPathEdges = new Set();

  if (isCritPathChecked && currentSimData && currentSimData.schedule && currentSimData.schedule.length > 0) {
    const schedMap = {};
    currentSimData.schedule.forEach(s => {
      const tid = s.taskId !== undefined ? s.taskId : s.task_id;
      schedMap[tid] = s;
    });

    const maxEnd = currentSimData.criticalTime || Math.max(...currentSimData.schedule.map(s => s.endTime));
    
    // Find all sink tasks ending at or near the max critical time
    let frontier = currentSimData.schedule
      .filter(s => s.endTime >= maxEnd - 1)
      .map(s => (s.taskId !== undefined ? s.taskId : s.task_id));
    
    if (frontier.length === 0) {
      let bestTid = -1, maxT = -1;
      currentSimData.schedule.forEach(s => {
        if (s.endTime > maxT) {
          maxT = s.endTime;
          bestTid = s.taskId !== undefined ? s.taskId : s.task_id;
        }
      });
      if (bestTid !== -1) frontier.push(bestTid);
    }

    frontier.forEach(id => critPathTasks.add(id));

    const visitedFrontier = new Set(frontier);

    while (frontier.length > 0) {
      const nextFrontier = [];
      frontier.forEach(vId => {
        const vSched = schedMap[vId];
        if (!vSched) return;

        // Find the predecessor u (with edge u -> v) that finishes latest among all predecessors of v
        let bestPreds = [];
        let maxPredEndTime = -1;

        tasks.forEach(uTask => {
          const uId = uTask.id;
          const uSched = schedMap[uId];
          if (!uSched || uId === vId) return;

          const hasEdge = (uTask.outEdges || []).some(e => e.target === vId);
          if (hasEdge) {
            if (uSched.endTime > maxPredEndTime) {
              maxPredEndTime = uSched.endTime;
              bestPreds = [uId];
            } else if (uSched.endTime === maxPredEndTime) {
              bestPreds.push(uId);
            }
          }
        });

        // Add latest-finishing predecessors to critical path
        bestPreds.forEach(uId => {
          critPathTasks.add(uId);
          critPathEdges.add(`${uId}->${vId}`);
          if (!visitedFrontier.has(uId)) {
            visitedFrontier.add(uId);
            nextFrontier.push(uId);
          }
        });
      });

      frontier = nextFrontier;
    }
  }

  // Update Critical Path Info Banner
  const critChainEl = document.getElementById('critPathChain');
  const critBannerEl = document.getElementById('critPathBanner');

  if (isCritPathChecked && critPathTasks.size > 0 && currentSimData && currentSimData.schedule) {
    if (critBannerEl) critBannerEl.style.display = 'flex';
    const sortedCritTasks = Array.from(critPathTasks).sort((a, b) => {
      const sA = (currentSimData.schedule.find(x => (x.taskId !== undefined ? x.taskId : x.task_id) === a)?.startTime || 0);
      const sB = (currentSimData.schedule.find(x => (x.taskId !== undefined ? x.taskId : x.task_id) === b)?.startTime || 0);
      return sA - sB;
    });

    if (critChainEl) {
      critChainEl.innerHTML = sortedCritTasks
        .map(id => `<span class="badge" style="background:#ef4444; color:#fff; padding:0.25rem 0.6rem; border-radius:6px; font-size:0.85rem; box-shadow:0 0 6px rgba(239,68,68,0.5);">T${id}</span>`)
        .join('<span style="color:#ef4444; margin:0 0.2rem; font-weight:bold;">➔</span>') + 
        `<span style="margin-left:0.75rem; color:#fca5a5; font-size:0.8rem; font-weight:normal;">(Czas wykonania: ${currentSimData.criticalTime || 0} ms)</span>`;
    }
  } else {
    if (critBannerEl) critBannerEl.style.display = 'none';
  }

  // Render Edges
  tasks.forEach(t => {
    const source = nodeCoords[t.id];
    if (!source) return;

    (t.outEdges || []).forEach(e => {
      const target = nodeCoords[e.target];
      if (target) {
        const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
        line.setAttribute('id', `dag-edge-${t.id}-${e.target}`);
        line.setAttribute('x1', source.x);
        line.setAttribute('y1', source.y);
        line.setAttribute('x2', target.x);
        line.setAttribute('y2', target.y);

        const isCritEdge = isCritPathChecked && critPathEdges.has(`${t.id}->${e.target}`);
        if (isCritEdge) {
          line.setAttribute('stroke', '#ef4444');
          line.setAttribute('stroke-width', '5');
          line.setAttribute('marker-end', 'url(#arrow-red)');
          line.setAttribute('style', 'filter: drop-shadow(0 0 6px #ef4444);');
        } else {
          line.setAttribute('stroke', '#38bdf8');
          line.setAttribute('stroke-width', '3');
          line.setAttribute('stroke-dasharray', '6 3');
          line.setAttribute('marker-end', 'url(#arrow)');
        }
        line.setAttribute('class', 'dag-edge');

        const title = document.createElementNS('http://www.w3.org/2000/svg', 'title');
        title.textContent = `T${t.id} → T${e.target} (Koszt comm: ${e.weight})${isCritEdge ? ' 🔴 ŚCIEŻKA KRYTYCZNA' : ''}`;
        line.appendChild(title);

        containerGroup.appendChild(line);
      }
    });
  });

  // Render Nodes with Drag & Drop
  tasks.forEach(t => {
    const coord = nodeCoords[t.id];
    if (!coord) return;

    const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    g.setAttribute('id', `dag-node-g-${t.id}`);
    g.setAttribute('transform', `translate(${coord.x}, ${coord.y})`);
    g.style.cursor = 'grab';

    const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    circle.setAttribute('cx', '0');
    circle.setAttribute('cy', '0');
    circle.setAttribute('r', '22');
    
    let fillColor = '#0284c7';
    let strokeColor = '#38bdf8';
    let strokeWidth = '3.5';

    if (isCritPathChecked && critPathTasks.has(t.id)) {
      fillColor = '#dc2626';   // Vibrant Crimson Red fill!
      strokeColor = '#f87171'; // Bright Red border!
      strokeWidth = '6';
      circle.setAttribute('style', 'filter: drop-shadow(0 0 14px rgba(239, 68, 68, 0.9));');
    } else if (t.isConditional) {
      fillColor = '#db2777';
      strokeColor = '#f472b6';
    } else if (t.isUnpredicted) {
      fillColor = '#d97706';
      strokeColor = '#fbbf24';
    }

    circle.setAttribute('fill', fillColor);
    circle.setAttribute('stroke', strokeColor);
    circle.setAttribute('stroke-width', strokeWidth);
    circle.setAttribute('class', 'dag-node');

    const title = document.createElementNS('http://www.w3.org/2000/svg', 'title');
    let nodeInfo = `Zadanie T${t.id}`;
    if (t.isConditional) nodeInfo += `\nWarunek: ${t.condition || 'TAK'}`;
    if (t.isUnpredicted) nodeInfo += `\nNieprzewidziane`;
    if (isCritPathChecked && critPathTasks.has(t.id)) nodeInfo += `\n🔴 NALEŻY DO ŚCIEŻKI KRYTYCZNEJ`;
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
    text.style.pointerEvents = 'none';
    text.textContent = `T${t.id}`;

    g.appendChild(circle);
    g.appendChild(text);

    // Drag mousedown listener
    g.onmousedown = (evt) => {
      evt.preventDefault();
      isDraggingDagNode = true;
      draggedNodeId = t.id;
      g.style.cursor = 'grabbing';
    };

    containerGroup.appendChild(g);
  });

  // Global SVG Drag Move and Up listeners
  svg.onmousemove = (evt) => {
    if (!isDraggingDagNode || draggedNodeId === null) return;
    const rect = svg.getBoundingClientRect();
    const mouseX = (evt.clientX - rect.left) / dagZoomScale;
    const mouseY = (evt.clientY - rect.top) / dagZoomScale;

    // Update node coords
    dagCustomNodeCoords[draggedNodeId] = { x: mouseX, y: mouseY };

    // Move node g element
    const gEl = document.getElementById(`dag-node-g-${draggedNodeId}`);
    if (gEl) gEl.setAttribute('transform', `translate(${mouseX}, ${mouseY})`);

    // Update outgoing edges from draggedNodeId
    const draggedTask = tasks.find(x => x.id === draggedNodeId);
    if (draggedTask && draggedTask.outEdges) {
      draggedTask.outEdges.forEach(e => {
        const line = document.getElementById(`dag-edge-${draggedNodeId}-${e.target}`);
        if (line) {
          line.setAttribute('x1', mouseX);
          line.setAttribute('y1', mouseY);
        }
      });
    }

    // Update incoming edges to draggedNodeId
    tasks.forEach(uTask => {
      (uTask.outEdges || []).forEach(e => {
        if (e.target === draggedNodeId) {
          const line = document.getElementById(`dag-edge-${uTask.id}-${draggedNodeId}`);
          if (line) {
            line.setAttribute('x2', mouseX);
            line.setAttribute('y2', mouseY);
          }
        }
      });
    });
  };

  svg.onmouseup = () => {
    if (isDraggingDagNode) {
      if (draggedNodeId !== null) {
        const gEl = document.getElementById(`dag-node-g-${draggedNodeId}`);
        if (gEl) gEl.style.cursor = 'grab';
      }
      isDraggingDagNode = false;
      draggedNodeId = null;
    }
  };
}

async function runBenchmark(force = false) {
  if (benchmarkRunning) return;
  if (!force && lastBenchmarkResults && lastBenchmarkResults.length > 0) {
    renderBenchmarkResults(lastBenchmarkResults);
    return;
  }

  benchmarkRunning = true;
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

  try {
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
        if (!data.error) {
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

    lastBenchmarkResults = results;
    if (statusText) statusText.textContent = `✅ Benchmark ukończony! Przeanalizowano wszystkie ${results.length} strategii.`;
    logConsole(`=======================================================`);
    logConsole(`🏆 BENCHMARK ZAKOŃCZONY SUKCESEM dla ${results.length} strategii.`);
    logConsole(`=======================================================`);
  } finally {
    benchmarkRunning = false;
    setTimeout(() => {
      if (statusBox) statusBox.classList.add('hidden');
    }, 4000);
  }
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

async function downloadBenchmarkCSV() {
  if (!lastBenchmarkResults || lastBenchmarkResults.length === 0) {
    logConsole("Brak wyników benchmarku do wyeksportowania. Uruchom najpierw benchmark.");
    return;
  }
  try {
    const res = await fetch('/api/benchmark-csv', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ results: lastBenchmarkResults })
    });
    const blob = await res.blob();
    const url = window.URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'benchmark_results.csv';
    document.body.appendChild(a);
    a.click();
    a.remove();
    logConsole("Pobrano plik benchmark_results.csv");
  } catch (err) {
    logConsole("Błąd pobierania pliku CSV: " + err.message);
  }
}

function exportGanttPNG() {
  const chart = document.getElementById('ganttChart');
  if (!chart) return;

  // Use HTML Canvas to render SVG/HTML Gantt chart
  const canvas = document.createElement('canvas');
  const ctx = canvas.getContext('2d');
  const width = Math.max(800, chart.scrollWidth + 40);
  const height = Math.max(300, chart.scrollHeight + 40);

  canvas.width = width;
  canvas.height = height;

  ctx.fillStyle = '#0b0f19';
  ctx.fillRect(0, 0, width, height);

  ctx.fillStyle = '#38bdf8';
  ctx.font = '16px sans-serif';
  ctx.fillText('Embedded Resource Simulator — Harmonogram Gantta', 20, 30);

  // Render Gantt rows onto canvas
  const rows = chart.querySelectorAll('.gantt-row');
  let y = 60;
  rows.forEach((row, i) => {
    const label = row.querySelector('.gantt-label')?.textContent || '';
    ctx.fillStyle = '#94a3b8';
    ctx.font = '12px sans-serif';
    ctx.fillText(label, 20, y + 20);

    const bars = row.querySelectorAll('.gantt-bar');
    bars.forEach(bar => {
      const left = parseFloat(bar.style.left) || 0;
      const w = parseFloat(bar.style.width) || 40;
      const text = bar.textContent || '';

      ctx.fillStyle = bar.classList.contains('conditional') ? '#db2777' :
                      bar.classList.contains('unpredicted') ? '#d97706' : '#0284c7';
      ctx.fillRect(160 + left, y, w, 28);
      ctx.strokeStyle = '#38bdf8';
      ctx.lineWidth = 1;
      ctx.strokeRect(160 + left, y, w, 28);

      ctx.fillStyle = '#ffffff';
      ctx.font = 'bold 11px sans-serif';
      ctx.fillText(text, 160 + left + 4, y + 18);
    });

    y += 40;
  });

  const a = document.createElement('a');
  a.href = canvas.toDataURL('image/png');
  a.download = 'gantt_chart.png';
  a.click();
  logConsole("Wyeksportowano wykres Gantta do pliku gantt_chart.png");
}

function renderMatrices() {
  const fileText = document.getElementById('fileEditorTextarea').value || '';
  const timesCont = document.getElementById('matrixTimesContainer');
  const costsCont = document.getElementById('matrixCostsContainer');

  if (!timesCont || !costsCont) return;

  const parseSection = (tag) => {
    const start = fileText.indexOf(tag);
    if (start === -1) return [];
    const lines = fileText.substring(start).split('\n');
    const matrix = [];
    for (let i = 1; i < lines.length; i++) {
      const line = lines[i].trim();
      if (line.startsWith('@') || line === '') break;
      const nums = line.split(/\s+/).map(Number).filter(n => !isNaN(n));
      if (nums.length > 0) matrix.push(nums);
    }
    return matrix;
  };

  const timesMatrix = parseSection('@times');
  const costsMatrix = parseSection('@cost');

  const buildHeatmapTable = (matrix, unitLabel) => {
    if (!matrix || matrix.length === 0) return '<div style="color: var(--text-muted);">Brak danych macierzy</div>';
    let allVals = matrix.flat();
    let minVal = Math.min(...allVals);
    let maxVal = Math.max(...allVals);
    let range = maxVal - minVal || 1;

    let html = '<table style="width: 100%; border-collapse: collapse; font-family: var(--font-mono); font-size: 0.8rem; margin-top: 0.5rem;">';
    html += '<thead><tr style="border-bottom: 1px solid var(--border-color);"><th style="padding: 6px;">T \\ HW</th>';
    const colsCount = matrix[0].length;
    for (let j = 0; j < colsCount; j++) {
      html += `<th style="padding: 6px; color: var(--primary);">HW${j}</th>`;
    }
    html += '</tr></thead><tbody>';

    matrix.forEach((row, i) => {
      html += `<tr><td style="padding: 6px; font-weight: bold; color: var(--accent-purple);">T${i}</td>`;
      row.forEach(val => {
        let norm = (val - minVal) / range;
        let bg = `rgba(6, 182, 212, ${0.1 + norm * 0.45})`;
        html += `<td style="padding: 6px; text-align: center; background: ${bg}; border: 1px solid rgba(255,255,255,0.05);">${val}</td>`;
      });
      html += '</tr>';
    });
    html += '</tbody></table>';
    return html;
  };

  timesCont.innerHTML = buildHeatmapTable(timesMatrix, 'ms');
  costsCont.innerHTML = buildHeatmapTable(costsMatrix, 'PLN');
}

async function renderCompareView() {
  const stratA = parseInt(document.getElementById('compareStratA').value, 10);
  const stratB = parseInt(document.getElementById('compareStratB').value, 10);
  const filename = document.getElementById('fileSelect').value || 'graph20.dat';

  const stratNames = {
    1: 'S1: Najszybsza Dedykowana', 2: 'S2: Najtańsza Dedykowana', 3: 'S3: Upakowywanie Instancji',
    5: 'S5: BFS', 6: 'S6: Ścieżka Krytyczna', 7: 'S7: Dwuetapowa Rafinacja',
    8: 'S8: Optymalizacja z Kary', 9: 'S9: Single Core Baseline'
  };

  document.getElementById('compareTitleA').textContent = `${stratNames[stratA]} (S${stratA})`;
  document.getElementById('compareTitleB').textContent = `${stratNames[stratB]} (S${stratB})`;

  try {
    const [resA, resB] = await Promise.all([
      fetch('/api/run-simulation', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ strategy: stratA, filename }) }).then(r => r.json()),
      fetch('/api/run-simulation', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ strategy: stratB, filename }) }).then(r => r.json())
    ]);

    const renderMiniStats = (data, elId) => {
      const container = document.getElementById(elId);
      const activeCount = data.activeInstancesCount !== undefined ? data.activeInstancesCount : (new Set((data.schedule || []).map(s => s.unit).filter(Boolean)).size);
      container.innerHTML = `
        <div class="stat-box"><span class="stat-label">Czas Krytyczny</span><span class="stat-value">${data.criticalTime || 0} ms</span></div>
        <div class="stat-box"><span class="stat-label">Koszt Total</span><span class="stat-value highlight">${data.totalCost || 0} PLN</span></div>
        <div class="stat-box"><span class="stat-label">Instancje HW</span><span class="stat-value">${activeCount}</span></div>
      `;
    };

    renderMiniStats(resA, 'compareStatsA');
    renderMiniStats(resB, 'compareStatsB');

    const renderMiniGantt = (data, containerId) => {
      const container = document.getElementById(containerId);
      container.innerHTML = '';
      const scale = 2;
      const criticalTime = data.criticalTime || 100;
      const unitsMap = {};
      (data.schedule || []).forEach(item => {
        const u = item.unit || 'HW';
        if (!unitsMap[u]) unitsMap[u] = [];
        unitsMap[u].push(item);
      });

      Object.keys(unitsMap).sort().forEach(unitName => {
        const row = document.createElement('div');
        row.className = 'gantt-row';
        row.innerHTML = `<div class="gantt-label" style="width: 70px; font-size: 0.75rem;">${unitName}</div>`;
        const track = document.createElement('div');
        track.className = 'gantt-track';
        track.style.width = `${criticalTime * scale}px`;

        unitsMap[unitName].forEach(t => {
          const bar = document.createElement('div');
          bar.className = 'gantt-bar';
          bar.style.left = `${t.startTime * scale}px`;
          bar.style.width = `${Math.max((t.endTime - t.startTime) * scale, 18)}px`;
          bar.textContent = `T${t.taskId}`;
          track.appendChild(bar);
        });

        row.appendChild(track);
        container.appendChild(row);
      });
    };

    renderMiniGantt(resA, 'compareGanttA');
    renderMiniGantt(resB, 'compareGanttB');

  } catch (err) {
    logConsole("Błąd porównywania strategii: " + err.message);
  }
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
  } else if (viewName === 'matrices') {
    document.querySelector("button[onclick=\"switchView('matrices')\"]").classList.add('active');
    document.getElementById('viewMatrices').classList.add('active');
    renderMatrices();
  } else if (viewName === 'benchmark') {
    document.querySelector("button[onclick=\"switchView('benchmark')\"]").classList.add('active');
    document.getElementById('viewBenchmark').classList.add('active');
    runBenchmark(false);
  } else if (viewName === 'compare') {
    document.querySelector("button[onclick=\"switchView('compare')\"]").classList.add('active');
    document.getElementById('viewCompare').classList.add('active');
    renderCompareView();
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
