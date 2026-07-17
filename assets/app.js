const CHANNEL_NAME = 'flex-trainer-realtime';
const STORAGE_KEY = 'flex-trainer-payload';
const DEVICE_KEY = 'flex-trainer-mdns';
const SETTINGS_KEY = 'flex-trainer-settings';
const MODULE_KEY = 'flex-trainer-modules';

const defaults = {
  flexA: 2048,
  flexB: 0,
  pan: 0,
  servo: 90,
  grip: 0,
  phrase: 'Halo',
  mdns: '',
  sentAt: Date.now(),
};

const defaultSettings = {
  servo: { source: 'flexA', inMin: 3040, inMax: 2800, outMin: 0, outMax: 180, points: [], zones: [] },
  gripper: {
    armSource: 'flexA',
    armInMin: 3040,
    armInMax: 2800,
    armPoints: [],
    armZones: [],
    gripSource: 'flexB',
    gripInMin: 3040,
    gripInMax: 2800,
    gripPoints: [],
    gripZones: [],
  },
  audio: {
    graphMinA: 2800,
    graphMaxA: 3040,
    graphMinB: 2270,
    graphMaxB: 2800,
    flexARules: [
      { min: 2800, max: 2879, text: 'Halo' },
      { min: 2880, max: 2959, text: 'Apa kabar' },
      { min: 2960, max: 3040, text: 'Semangat' },
    ],
    flexBRules: [
      { min: 2800, max: 2879, text: 'Aulia' },
      { min: 2880, max: 2959, text: 'Siap' },
      { min: 2960, max: 3040, text: 'Mantap' },
    ],
  },
};

const clamp = (value, min, max) => Math.min(Math.max(value, min), max);
const round = (value, digits = 0) => {
  const factor = 10 ** digits;
  return Math.round(value * factor) / factor;
};
const numeric = (value, fallback = 0) => {
  const next = Number(value);
  return Number.isFinite(next) ? next : fallback;
};

function mapCalibrated(value, inMin, inMax, outMin, outMax) {
  if (inMax === inMin) return outMin;
  const ratio = clamp((value - inMin) / (inMax - inMin), 0, 1);
  return outMin + ratio * (outMax - outMin);
}

function mapCalibrationPoints(value, points, inMin, inMax, outMin, outMax, fallback) {
  const bounds = [
    { adc: numeric(inMin, NaN), output: numeric(outMin, NaN) },
    { adc: numeric(inMax, NaN), output: numeric(outMax, NaN) }
  ];

  const normalized = [...points, ...bounds]
    .map((point) => ({ adc: numeric(point.adc, NaN), output: numeric(point.output, NaN) }))
    .filter((point) => Number.isFinite(point.adc) && Number.isFinite(point.output))
    .sort((a, b) => a.adc - b.adc);
  const unique = normalized.filter((point, index) => index === 0 || point.adc !== normalized[index - 1].adc);
  if (unique.length < 2) return fallback();
  if (value <= unique[0].adc) return unique[0].output;
  if (value >= unique.at(-1).adc) return unique.at(-1).output;
  const upperIndex = unique.findIndex((point) => point.adc >= value);
  const lower = unique[upperIndex - 1];
  const upper = unique[upperIndex];
  return mapCalibrated(value, lower.adc, upper.adc, lower.output, upper.output);
}

function mapCalibrationZones(value, zones, fallback) {
  const normalized = zones
    .map((zone) => ({ min: numeric(zone.min, NaN), max: numeric(zone.max, NaN), output: numeric(zone.output, NaN) }))
    .filter((zone) => Number.isFinite(zone.min) && Number.isFinite(zone.max) && Number.isFinite(zone.output))
    .sort((a, b) => a.min - b.min);
  const valid = normalized.length === zones.length
    && normalized.every((zone, index) => zone.min <= zone.max && (index === 0 || normalized[index - 1].max < zone.min));
  if (!valid) return fallback();
  const match = normalized.find((zone) => value >= zone.min && value <= zone.max);
  return match ? match.output : fallback();
}

function normalizeMdns(value) {
  const raw = String(value || '').trim().toLowerCase().replace(/^https?:\/\//, '').replace(/\/.*$/, '');
  if (!raw) return '';
  if (/^\d{1,3}(\.\d{1,3}){3}$/.test(raw)) return raw;
  return raw.endsWith('.local') ? raw : `${raw}.local`;
}

function loadSettings() {
  try {
    const saved = JSON.parse(localStorage.getItem(SETTINGS_KEY)) || {};

    // Auto-migrate old defaults to new hardware defaults (3040 to 2800)
    if (saved.servo && (saved.servo.inMin === 1320 || saved.servo.inMin === 0)) {
      saved.servo.inMin = 3040;
      saved.servo.inMax = 2800;
    }
    let migrated = false;
    if (saved.gripper) {
      if (saved.gripper.armInMin === 1320 || saved.gripper.armInMin === 0) {
        saved.gripper.armInMin = 3040;
        saved.gripper.armInMax = 2800;
        migrated = true;
      }

      if (saved.gripper.gripInMin === 1320 || saved.gripper.gripInMin === 0) {
        saved.gripper.gripInMin = 3040;
        saved.gripper.gripInMax = 2800;
      }
    }
    if (saved.audio) {
      if (saved.audio.graphMinA === undefined) { saved.audio.graphMinA = saved.audio.graphMin !== undefined ? saved.audio.graphMin : 2800; migrated = true; }
      if (saved.audio.graphMaxA === undefined) { saved.audio.graphMaxA = saved.audio.graphMax !== undefined ? saved.audio.graphMax : 3040; migrated = true; }
      if (saved.audio.graphMinB === undefined) { saved.audio.graphMinB = 2270; migrated = true; }
      if (saved.audio.graphMaxB === undefined) { saved.audio.graphMaxB = 2800; migrated = true; }
    }

    const legacyRules = saved.audio?.rules;
    const audio = {
      ...defaultSettings.audio,
      ...saved.audio,
      flexARules: saved.audio?.flexARules || legacyRules || defaultSettings.audio.flexARules,
      flexBRules: saved.audio?.flexBRules || defaultSettings.audio.flexBRules,
    };
    delete audio.source;
    delete audio.rules;

    const fullSettings = {
      servo: { ...defaultSettings.servo, ...saved.servo, points: saved.servo?.points || [], zones: saved.servo?.zones || [] },
      gripper: {
        ...defaultSettings.gripper,
        ...saved.gripper,
        armPoints: saved.gripper?.armPoints || [],
        armZones: saved.gripper?.armZones || [],
        gripPoints: saved.gripper?.gripPoints || [],
        gripZones: saved.gripper?.gripZones || [],
      },
      audio,
    };

    // Jika terjadi migrasi data baru, simpan ulang ke LocalStorage agar permanen
    if (migrated) {
      localStorage.setItem(SETTINGS_KEY, JSON.stringify(fullSettings));
    }

    return fullSettings;
  } catch {
    return structuredClone(defaultSettings);
  }
}

let syncTimeout = null;
function syncSettingsToHardware(settings) {
  // Debounce 600ms agar tidak membanjiri ESP32 saat user sedang mengetik angka di web
  window.clearTimeout(syncTimeout);
  syncTimeout = window.setTimeout(async () => {
    const minA = Math.round(settings.servo.inMin);
    const maxA = Math.round(settings.servo.inMax);
    const minB = Math.round(settings.gripper.gripInMin);
    const maxB = Math.round(settings.gripper.gripInMax);

    // 1. Sinkronisasi via Web Serial USB jika tersambung
    if (serialActive && serialPort && serialWriter) {
      try {
        const cmd = `SET:{"minA":${minA},"maxA":${maxA},"minB":${minB},"maxB":${maxB}}\n`;
        await serialWriter.write(encoder.encode(cmd));
        console.log("Hardware: Berhasil kalibrasi via Serial USB!");
      } catch (e) {
        console.error("Gagal sinkronisasi kalibrasi via Serial", e);
      }
    }

    // 2. Sinkronisasi via Wi-Fi Web Server / WebSocket jika terhubung mDNS/IP
    if (ws && ws.readyState === WebSocket.OPEN) {
      try {
        ws.send(`SET:{"minA":${minA},"maxA":${maxA},"minB":${minB},"maxB":${maxB}}\n`);
        console.log("Hardware: Berhasil kalibrasi via WebSocket!");
      } catch (e) {
        console.error("Gagal sinkronisasi kalibrasi via WebSocket", e);
      }
    } else if (selectedMdns) {
      try {
        await fetch(`http://${selectedMdns}/config?minA=${minA}&maxA=${maxA}&minB=${minB}&maxB=${maxB}`, { mode: 'no-cors' });
        console.log("Hardware: Berhasil kalibrasi via Wi-Fi HTTP!");
      } catch (e) {
        console.error("Gagal sinkronisasi kalibrasi via Wi-Fi HTTP", e);
      }
    }
  }, 600);
}

function saveSettings(settings) {
  try {
    localStorage.setItem(SETTINGS_KEY, JSON.stringify(settings));
  } catch (e) {
    if (e.name === 'QuotaExceededError' || e.name === 'NS_ERROR_DOM_QUOTA_REACHED') {
      alert('Memori penyimpanan browser penuh! File audio kustom yang Anda upload terlalu besar. Harap gunakan file audio yang lebih kecil (disarankan di bawah 500KB).');
    } else {
      console.error('Gagal menyimpan pengaturan', e);
    }
  }
  syncSettingsToHardware(settings);
}

function loadModuleStates() {
  try {
    return JSON.parse(localStorage.getItem(MODULE_KEY)) || {};
  } catch {
    return {};
  }
}

function saveModuleStates(states) {
  localStorage.setItem(MODULE_KEY, JSON.stringify(states));
}

function phraseFromRules(value, rules) {
  const rule = rules.find((item) => value >= numeric(item.min) && value <= numeric(item.max));
  return rule?.text || '';
}

function phraseFromFlexes(flexA, flexB, settings) {
  return [
    phraseFromRules(flexA, settings.audio.flexARules),
    phraseFromRules(flexB, settings.audio.flexBRules),
  ].filter(Boolean).join(' ');
}

function payloadFromFlex(flexA, flexB, mdns = '', settings = loadSettings()) {
  const flex = { flexA: numeric(flexA), flexB: numeric(flexB) };
  const servoInput = flex[settings.servo.source] ?? flex.flexA;
  const armInput = flex[settings.gripper.armSource] ?? flex.flexA;
  const gripInput = flex[settings.gripper.gripSource] ?? flex.flexB;

  return {
    flexA: Math.round(flex.flexA),
    flexB: Math.round(flex.flexB),
    pan: round(mapCalibrated(
      mapCalibrationZones(armInput, settings.gripper.armZones, () => mapCalibrationPoints(armInput, settings.gripper.armPoints, settings.gripper.armInMin, settings.gripper.armInMax, 0, 100, () => mapCalibrated(armInput, settings.gripper.armInMin, settings.gripper.armInMax, 0, 100))),
      0, 100, 100, -100,  // Reversed: flex bengkok → rack ke arah yang benar
    ), 1),
    servo: round(mapCalibrationZones(servoInput, settings.servo.zones, () => mapCalibrationPoints(servoInput, settings.servo.points, settings.servo.inMin, settings.servo.inMax, settings.servo.outMin, settings.servo.outMax, () => mapCalibrated(servoInput, settings.servo.inMin, settings.servo.inMax, settings.servo.outMin, settings.servo.outMax))), 1),
    grip: round(mapCalibrationZones(gripInput, settings.gripper.gripZones, () => mapCalibrationPoints(gripInput, settings.gripper.gripPoints, settings.gripper.gripInMin, settings.gripper.gripInMax, 100, 0, () => mapCalibrated(gripInput, settings.gripper.gripInMin, settings.gripper.gripInMax, 100, 0))), 1),  // Reversed
    phrase: phraseFromFlexes(flex.flexA, flex.flexB, settings),
    mdns: normalizeMdns(mdns),
    sentAt: Date.now(),
  };
}

function readPayload() {
  try {
    return { ...defaults, ...JSON.parse(localStorage.getItem(STORAGE_KEY)) };
  } catch {
    return defaults;
  }
}

function createChannel(onPayload) {
  let channel = null;
  if ('BroadcastChannel' in window) {
    channel = new BroadcastChannel(CHANNEL_NAME);
    channel.onmessage = (event) => {
      if (event.data?.type === 'payload') onPayload(event.data.payload);
    };
  }

  window.addEventListener('storage', (event) => {
    if (event.key !== STORAGE_KEY || !event.newValue) return;
    try {
      onPayload(JSON.parse(event.newValue));
    } catch {
      onPayload(defaults);
    }
  });

  return {
    send(payload) {
      localStorage.setItem(STORAGE_KEY, JSON.stringify(payload));
      channel?.postMessage({ type: 'payload', payload });
    },
  };
}

function initSimulator() {
  try {
    const $ = (id) => document.getElementById(id);
    const refs = {

    arm: $('armGroup'),
    leftJaw: $('leftJaw'),
    rightJaw: $('rightJaw'),
    needle: $('needle'),
    flexA: $('flexARead'),
    flexB: $('flexBRead'),
    headerFlex: $('headerFlexRead'),
    pan: $('panRead'),
    grip: $('gripRead'),
    servo: $('servoRead'),
    servoFlex: $('servoFlexRead'),
    servoOutDisplay: $('servoOutDisplay'),
    activeModules: $('activeModulesRead'),
    status: $('connectionStatus'),
    led: $('connectionLed'),
    chart: $('flexChart'),
    voiceStatus: $('voiceStatus'),
    enableVoice: $('enableVoice'),
    speakNow: $('speakNow'),
    servoToggle: $('servoToggle'),
    gripperToggle: $('gripperToggle'),
    graphAudioToggle: $('graphAudioToggle'),
    flasherToggle: $('flasherToggle'),
    mdnsInput: $('mdnsInput'),
    connectEsp: $('connectEsp'),
    connectSerial: $('connectSerial'),
    disconnectEsp: $('disconnectEsp'),
    mdnsStatus: $('mdnsStatus'),
    mdnsLed: $('mdnsLed'),
  };

  const settings = loadSettings();

  // Synchronize settings between tabs (e.g. from virtual-esp32.html)
  window.addEventListener('storage', (event) => {
    if (event.key === SETTINGS_KEY) {
      try {
        Object.assign(settings, loadSettings());
        if (typeof renderCalibrationEditors === 'function') renderCalibrationEditors();
        if (typeof renderAudioRules === 'function') renderAudioRules();
      } catch (e) {
        console.error("Gagal sinkronisasi settings di tab simulator", e);
      }
    }
  });

  // Restore saved toggle states before reading .checked
  const savedModules = loadModuleStates();
  if ('servo' in savedModules) refs.servoToggle.checked = savedModules.servo;
  if ('gripper' in savedModules) refs.gripperToggle.checked = savedModules.gripper;
  if ('graphAudio' in savedModules) refs.graphAudioToggle.checked = savedModules.graphAudio;
  if ('flasher' in savedModules) refs.flasherToggle.checked = savedModules.flasher;

  const urlParams = new URLSearchParams(window.location.search);
  const showFlasher = urlParams.has('admin') || urlParams.has('flasher');

  // Hide flasher module card physically by default if not admin/flasher parameter
  const flasherCard = document.querySelector('[data-module-card="flasher"]');
  if (flasherCard && !showFlasher) {
    flasherCard.style.setProperty('display', 'none', 'important');
  }

  const modules = {
    servo: refs.servoToggle.checked,
    gripper: refs.gripperToggle.checked,
    graphAudio: refs.graphAudioToggle.checked,
    flasher: showFlasher ? refs.flasherToggle.checked : false,
  };

  let voiceEnabled = false;
  let lastPhrase = '';
  let lastVoiceAt = 0;
  let lastGraphAt = 0;
  let target = readPayload();
  let current = { ...target };
  let selectedMdns = normalizeMdns(localStorage.getItem(DEVICE_KEY));
  let espPolling = false;
  let pollTimer = null;
  let pollFailures = 0;
  let serialPort = null;
  let serialReader = null;
  let serialWriter = null;
  let serialActive = false;
  let lastSentServoAngle = -1; // Tracking last angle sent to ESP32
  const encoder = new TextEncoder(); // Global encoder for serial transmissions
  let ws = null; // WebSocket connection object
  let wsUserClose = false; // Flag to prevent automatic reconnection when user manually disconnects

  const graph = Array.from({ length: 160 }, () => ({ flexA: current.flexA, flexB: current.flexB }));
  const ctx = refs.chart.getContext('2d');

  refs.mdnsInput.value = selectedMdns.replace('.local', '');

  function setLed(element, state) {
    element.className = `led led-${state}`;
  }

  function bindSetting(id, path, isNumber = false) {
    const element = $(id);
    if (!element) return; // Cegah crash jika HTML lawas ter-cache dan element tidak ditemukan!
    const [section, key] = path;
    element.value = settings[section][key];
    element.addEventListener('input', () => {
      settings[section][key] = isNumber ? numeric(element.value) : element.value;
      saveSettings(settings);
      target = payloadFromFlex(current.flexA, current.flexB, target.mdns, settings);
      renderAudioRules();
    });
  }


  [
    ['servoSource', ['servo', 'source']],
    ['servoInMin', ['servo', 'inMin'], true],
    ['servoInMax', ['servo', 'inMax'], true],
    ['servoOutMin', ['servo', 'outMin'], true],
    ['servoOutMax', ['servo', 'outMax'], true],
    ['armSource', ['gripper', 'armSource']],
    ['armInMin', ['gripper', 'armInMin'], true],
    ['armInMax', ['gripper', 'armInMax'], true],
    ['gripSource', ['gripper', 'gripSource']],
    ['gripInMin', ['gripper', 'gripInMin'], true],
    ['gripInMax', ['gripper', 'gripInMax'], true],
    ['graphMinA', ['audio', 'graphMinA'], true],
    ['graphMaxA', ['audio', 'graphMaxA'], true],
    ['graphMinB', ['audio', 'graphMinB'], true],
    ['graphMaxB', ['audio', 'graphMaxB'], true],
  ].forEach(([id, path, isNumber]) => bindSetting(id, path, isNumber));

  function renderCalibrationPoints(containerId, points, sourceKey, outputLabel) {
    const container = $(containerId);
    container.innerHTML = '';
    points.forEach((point, index) => {
      const row = document.createElement('div');
      row.className = 'calibration-row';
      row.innerHTML = `
        <input type="number" value="${point.adc}" aria-label="ADC ${outputLabel}" placeholder="ADC" />
        <input type="number" value="${point.output}" aria-label="${outputLabel}" placeholder="${outputLabel}" />
        <button class="mini-action" type="button">Capture</button>
        <button class="mini-action muted" type="button">Hapus</button>
      `;
      const [adcInput, outputInput, captureButton, deleteButton] = row.children;
      const update = () => {
        point.adc = numeric(adcInput.value);
        point.output = numeric(outputInput.value);
        saveSettings(settings);
        target = payloadFromFlex(current.flexA, current.flexB, target.mdns, settings);
      };
      adcInput.addEventListener('input', update);
      outputInput.addEventListener('input', update);
      captureButton.addEventListener('click', () => {
        adcInput.value = Math.round(current[settings[sourceKey.section][sourceKey.key]] ?? current.flexA);
        update();
      });
      deleteButton.addEventListener('click', () => {
        points.splice(index, 1);
        saveSettings(settings);
        renderCalibrationEditors();
      });
      container.appendChild(row);
    });
  }

  function renderCalibrationZones(containerId, zones, sourceKey, outputLabel) {
    const container = $(containerId);
    container.innerHTML = '';
    zones.forEach((zone, index) => {
      const row = document.createElement('div');
      row.className = 'zone-row';
      row.innerHTML = `
        <input type="number" value="${zone.min}" aria-label="ADC Min ${outputLabel}" placeholder="Min" />
        <button class="mini-action" type="button">Cap Min</button>
        <input type="number" value="${zone.max}" aria-label="ADC Max ${outputLabel}" placeholder="Max" />
        <button class="mini-action" type="button">Cap Max</button>
        <input type="number" value="${zone.output}" aria-label="${outputLabel}" placeholder="${outputLabel}" />
        <button class="mini-action muted" type="button">Hapus</button>
      `;
      const [minInput, captureMin, maxInput, captureMax, outputInput, deleteButton] = row.children;
      const update = () => {
        zone.min = numeric(minInput.value);
        zone.max = numeric(maxInput.value);
        zone.output = numeric(outputInput.value);
        saveSettings(settings);
        target = payloadFromFlex(current.flexA, current.flexB, target.mdns, settings);
      };
      const capture = (input) => {
        input.value = Math.round(current[settings[sourceKey.section][sourceKey.key]] ?? current.flexA);
        update();
      };
      minInput.addEventListener('input', update);
      maxInput.addEventListener('input', update);
      outputInput.addEventListener('input', update);
      captureMin.addEventListener('click', () => capture(minInput));
      captureMax.addEventListener('click', () => capture(maxInput));
      deleteButton.addEventListener('click', () => {
        zones.splice(index, 1);
        saveSettings(settings);
        renderCalibrationEditors();
      });
      container.appendChild(row);
    });
  }

  function renderCalibrationEditors() {
    renderCalibrationPoints('servoPoints', settings.servo.points, { section: 'servo', key: 'source' }, 'Derajat');
    renderCalibrationPoints('armPoints', settings.gripper.armPoints, { section: 'gripper', key: 'armSource' }, 'Posisi %');
    renderCalibrationPoints('gripPoints', settings.gripper.gripPoints, { section: 'gripper', key: 'gripSource' }, 'Bukaan %');
    renderCalibrationZones('servoZones', settings.servo.zones, { section: 'servo', key: 'source' }, 'Derajat');
    renderCalibrationZones('armZones', settings.gripper.armZones, { section: 'gripper', key: 'armSource' }, 'Posisi %');
    renderCalibrationZones('gripZones', settings.gripper.gripZones, { section: 'gripper', key: 'gripSource' }, 'Bukaan %');
    target = payloadFromFlex(current.flexA, current.flexB, target.mdns, settings);
  }

  function bindCalibrationAdd(buttonId, points, output = 0) {
    $(buttonId).addEventListener('click', () => {
      points.push({ adc: 0, output });
      saveSettings(settings);
      renderCalibrationEditors();
    });
  }

  function bindZoneAdd(buttonId, zones, output = 0) {
    $(buttonId).addEventListener('click', () => {
      zones.push({ min: 0, max: 0, output });
      saveSettings(settings);
      renderCalibrationEditors();
    });
  }

  bindCalibrationAdd('addServoPoint', settings.servo.points);
  bindCalibrationAdd('addArmPoint', settings.gripper.armPoints);
  bindCalibrationAdd('addGripPoint', settings.gripper.gripPoints);
  bindZoneAdd('addServoZone', settings.servo.zones);
  bindZoneAdd('addArmZone', settings.gripper.armZones);
  bindZoneAdd('addGripZone', settings.gripper.gripZones);
  renderCalibrationEditors();

  function setHeaderStatus(text, state = 'yellow') {
    refs.status.innerHTML = `<i id="connectionLed" class="led led-${state}"></i> ${text}`;
    refs.status.classList.toggle('live', state === 'green');
    refs.led = document.getElementById('connectionLed');
  }

  function acceptPayload(payload, source = 'Virtual') {
    const incomingMdns = normalizeMdns(payload?.mdns);
    if (selectedMdns && incomingMdns && incomingMdns !== selectedMdns) {
      setHeaderStatus('Disconnected - mDNS mismatch', 'red');
      setConnectionStatus(`mDNS mismatch: ${incomingMdns}`, 'red');
      return;
    }
    const rawFlexA = numeric(payload.flexA, defaults.flexA);
    const rawFlexB = numeric(payload.flexB, defaults.flexB);
    target = payloadFromFlex(rawFlexA, rawFlexB, incomingMdns, settings);
    setHeaderStatus(source, 'green');
  }

  createChannel((payload) => {
    // Accept virtual data when: not using serial AND (not polling real ESP32, OR real ESP32 isn't responding)
    if (!serialActive && (!espPolling || pollFailures > 0)) acceptPayload(payload, 'Virtual ESP32');
  });

  function setConnectionStatus(text, state = 'red') {
    refs.mdnsStatus.innerHTML = `<i id="mdnsLed" class="led led-${state}"></i> ${text}`;
    refs.mdnsLed = document.getElementById('mdnsLed');
  }

  async function disconnectSerial() {
    serialActive = false;
    if (serialWriter) {
      try {
        serialWriter.releaseLock();
      } catch { }
      serialWriter = null;
    }
    if (serialReader) {
      try {
        await serialReader.cancel();
      } catch { }
    }
    if (serialPort) {
      try {
        while (serialReader) {
          await new Promise((resolve) => window.setTimeout(resolve, 0));
        }
        await serialPort.close();
      } catch { }
      serialPort = null;
    }
  }

  function acceptSerialLine(line) {
    // Format baru: DATA:{...} — JSON lengkap dari firmware (pan/grip sudah dihitung)
    const dataMatch = line.match(/^DATA:(\{.+\})$/);
    if (dataMatch) {
      try {
        const data = JSON.parse(dataMatch[1]);
        if (typeof data.flexA === 'number' && typeof data.flexB === 'number') {
          acceptPayload({ flexA: data.flexA, flexB: data.flexB }, 'Serial ESP32');
          setConnectionStatus('Connected: Serial ESP32', 'green');
        }
      } catch { }
      return;
    }
    // Format lama: "FlexA:xxx FlexB:xxx" — tetap didukung untuk kompatibilitas
    const match = line.match(/FlexA:\s*(\d+)\s+FlexB:\s*(\d+)/i)
      || line.match(/Flex A:\s*(\d+)\s*\|\s*Flex B:\s*(\d+)/i);
    if (!match) return;
    acceptPayload({ flexA: Number(match[1]), flexB: Number(match[2]) }, 'Serial ESP32');
    setConnectionStatus('Connected: Serial ESP32', 'green');
  }

  async function readSerialLoop() {
    const decoder = new TextDecoder();
    let buffer = '';
    while (serialActive && serialPort?.readable) {
      serialReader = serialPort.readable.getReader();
      try {
        while (serialActive) {
          const { value, done } = await serialReader.read();
          if (done) break;
          buffer += decoder.decode(value, { stream: true });
          const lines = buffer.split(/\r?\n/);
          buffer = lines.pop() || '';
          lines.forEach(acceptSerialLine);
        }
      } catch {
        if (serialActive) {
          setHeaderStatus('Reconnecting - Serial unavailable', 'yellow');
          setConnectionStatus('Reconnect Serial ESP32', 'yellow');
        }
      } finally {
        serialReader.releaseLock();
        serialReader = null;
      }
    }
  }

  function disconnectWifi() {
    wsUserClose = true;
    if (ws) {
      try {
        ws.close();
      } catch (e) {}
      ws = null;
    }
  }

  function connectWebSocket() {
    if (wsUserClose || !selectedMdns) return;
    const wsUrl = `ws://${selectedMdns}:81`;
    console.log(`Connecting to WebSocket: ${wsUrl}`);
    setHeaderStatus(`Connecting: ${selectedMdns}`, 'yellow');
    setConnectionStatus(`Connecting: ${selectedMdns}`, 'yellow');
    
    ws = new WebSocket(wsUrl);
    
    ws.onopen = () => {
      console.log('WebSocket Connected!');
      setHeaderStatus('ESP32 WiFi', 'green');
      setConnectionStatus(`Connected: ${selectedMdns}`, 'green');
      // Kirim kalibrasi saat pertama kali tersambung
      syncSettingsToHardware(settings);
    };
    
    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        if (typeof data.flexA === 'number' && typeof data.flexB === 'number') {
          acceptPayload({ flexA: data.flexA, flexB: data.flexB, mdns: selectedMdns }, `ESP32 ${selectedMdns}`);
        }
      } catch (e) {
        console.error('Failed to parse WebSocket message', e);
      }
    };
    
    ws.onerror = (error) => {
      console.error('WebSocket Error:', error);
    };
    
    ws.onclose = () => {
      console.log('WebSocket Closed');
      if (!wsUserClose) {
        setHeaderStatus('Reconnecting - ESP32 unavailable', 'yellow');
        setConnectionStatus(`Reconnecting: ${selectedMdns}`, 'yellow');
        setTimeout(connectWebSocket, 1500); // Auto reconnect
      }
    };
  }

  refs.connectEsp.addEventListener('click', async () => {
    await disconnectSerial();
    disconnectWifi();
    selectedMdns = normalizeMdns(refs.mdnsInput.value);
    refs.mdnsInput.value = selectedMdns.replace('.local', '');
    localStorage.setItem(DEVICE_KEY, selectedMdns);
    if (!selectedMdns) {
      setConnectionStatus('Virtual/manual mode', 'yellow');
      return;
    }
    wsUserClose = false;
    connectWebSocket();
  });

  refs.connectSerial.addEventListener('click', async () => {
    if (!('serial' in navigator)) {
      setHeaderStatus('Web Serial unsupported', 'red');
      setConnectionStatus('Gunakan Chrome atau Edge via HTTPS/localhost', 'red');
      return;
    }
    try {
      espPolling = false;
      window.clearTimeout(pollTimer);
      await disconnectSerial();
      setHeaderStatus('Connecting Serial ESP32', 'yellow');
      setConnectionStatus('Pilih COM ESP32...', 'yellow');
      serialPort = await navigator.serial.requestPort();
      await serialPort.open({ baudRate: 115200 });
      serialActive = true;
      serialWriter = serialPort.writable.getWriter(); // Dapatkan writer untuk kirim data ke ESP32
      setHeaderStatus('Serial ESP32', 'green');
      setConnectionStatus('Connected: Serial ESP32', 'green');
      readSerialLoop();
      // Kirim kalibrasi saat pertama kali tersambung
      syncSettingsToHardware(settings);
    } catch (error) {
      await disconnectSerial();
      const errorName = error?.name || '';
      const errorMessage = String(error?.message || '').toLowerCase();
      if (errorName === 'NotFoundError') {
        setHeaderStatus('Disconnected - Serial cancelled', 'red');
        setConnectionStatus('Pemilihan COM dibatalkan', 'red');
      } else if (errorName === 'NetworkError' || errorMessage.includes('failed to open') || errorMessage.includes('access denied')) {
        setHeaderStatus('Disconnected - COM busy', 'red');
        setConnectionStatus('Tutup Serial Monitor/Plotter lalu coba lagi', 'red');
      } else {
        setHeaderStatus('Disconnected - Serial unavailable', 'red');
        setConnectionStatus(`Serial error: ${errorName || 'unknown'}`, 'red');
      }
    }
  });

  refs.disconnectEsp.addEventListener('click', async () => {
    espPolling = false;
    pollFailures = 0;
    window.clearTimeout(pollTimer);
    disconnectWifi();
    await disconnectSerial();
    setHeaderStatus('Disconnected', 'red');
    setConnectionStatus('Disconnected', 'red');
  });

  if ('serial' in navigator) {
    navigator.serial.addEventListener('disconnect', async (event) => {
      if (event.target !== serialPort) return;
      await disconnectSerial();
      setHeaderStatus('Disconnected - Serial removed', 'red');
      setConnectionStatus('Serial ESP32 disconnected', 'red');
    });
  }

  function syncModuleState() {
    modules.servo = refs.servoToggle.checked;
    modules.gripper = refs.gripperToggle.checked;
    modules.graphAudio = refs.graphAudioToggle.checked;
    modules.flasher = showFlasher ? refs.flasherToggle.checked : false;
    document.querySelector('[data-module-card="servo"]').classList.toggle('module-off', !modules.servo);
    document.querySelector('[data-module-card="gripper"]').classList.toggle('module-off', !modules.gripper);
    document.querySelector('[data-module-card="graphAudio"]').classList.toggle('module-off', !modules.graphAudio);
    document.querySelector('[data-module-card="flasher"]').classList.toggle('module-off', !modules.flasher);

    // Count active modules, excluding flasher if hidden
    const visibleActiveCount = Object.keys(modules)
      .filter(key => key !== 'flasher' || showFlasher)
      .map(key => modules[key])
      .filter(Boolean).length;

    refs.activeModules.textContent = visibleActiveCount;
  }

  [refs.servoToggle, refs.gripperToggle, refs.graphAudioToggle, refs.flasherToggle].forEach((toggle) => {
    toggle.addEventListener('change', () => {
      syncModuleState();
      saveModuleStates({
        servo: refs.servoToggle.checked,
        gripper: refs.gripperToggle.checked,
        graphAudio: refs.graphAudioToggle.checked,
        flasher: refs.flasherToggle.checked,
      });
    });
  });
  syncModuleState();

  function renderRuleSet(containerId, rules, sensorLabel) {
    const container = $(containerId);
    container.innerHTML = '';
    rules.forEach((rule, index) => {
      if (!rule.type) rule.type = 'tts';

      const row = document.createElement('div');
      row.className = 'rule-row';
      row.innerHTML = `
        <input type="number" value="${rule.min}" aria-label="${sensorLabel} min suara" />
        <input type="number" value="${rule.max}" aria-label="${sensorLabel} max suara" />
        <select aria-label="${sensorLabel} tipe feedback" style="background:#08090a; color:#fff; border:1px solid var(--line); border-radius:7px; min-height:28px;">
          <option value="tts" ${rule.type === 'tts' ? 'selected' : ''}>TTS</option>
          <option value="audio" ${rule.type === 'audio' ? 'selected' : ''}>Audio</option>
        </select>
        <div class="feedback-container" style="display: flex; gap: 4px; align-items: center; min-width: 0; width: 100%;">
          <input type="text" class="tts-input" value="${rule.text || ''}" placeholder="Teks TTS" style="display: ${rule.type === 'tts' ? 'block' : 'none'}; width: 100%; min-height: 28px; padding: 5px 7px; border: 1px solid var(--line); border-radius: 7px; color: var(--text); background: #08090a;" />
          <div class="audio-input-wrapper" style="display: ${rule.type === 'audio' ? 'flex' : 'none'}; gap: 4px; align-items: center; min-width: 0; width: 100%;">
            <button class="mini-action upload-btn" type="button" style="padding: 2px 6px; font-size: 11px; flex-shrink: 0; background: rgba(255,255,255,0.08); border: 1px solid var(--line); border-radius: 5px; color: #fff; cursor: pointer;">Upload</button>
            <input type="file" class="audio-file-input" accept="audio/*" style="display: none;" />
            <span class="audio-name" style="font-size: 10px; color: var(--muted); overflow: hidden; text-overflow: ellipsis; white-space: nowrap; max-width: 80px;" title="${rule.audioName || 'Belum ada file'}">${rule.audioName || 'Belum ada file'}</span>
            <button class="mini-action play-preview-btn" type="button" style="display: ${rule.audioData ? 'inline-block' : 'none'}; padding: 2px 4px; font-size: 10px; flex-shrink: 0; background: rgba(59, 130, 246, 0.2); border: 1px solid rgb(59, 130, 246); border-radius: 5px; color: rgb(59, 130, 246); cursor: pointer;">▶</button>
          </div>
        </div>
        <button class="mini-action" type="button" style="background: rgba(255, 51, 51, 0.2); border: 1px solid rgb(255, 51, 51); border-radius: 5px; color: rgb(255, 51, 51); padding: 4px 8px; cursor: pointer;">Hapus</button>
      `;

      const [minInput, maxInput, typeSelect, feedbackContainer, deleteButton] = row.children;
      const ttsInput = feedbackContainer.querySelector('.tts-input');
      const audioWrapper = feedbackContainer.querySelector('.audio-input-wrapper');
      const uploadBtn = audioWrapper.querySelector('.upload-btn');
      const fileInput = audioWrapper.querySelector('.audio-file-input');
      const audioName = audioWrapper.querySelector('.audio-name');
      const playPreviewBtn = audioWrapper.querySelector('.play-preview-btn');

      minInput.addEventListener('input', () => {
        rule.min = numeric(minInput.value);
        saveSettings(settings);
        target = payloadFromFlex(current.flexA, current.flexB, target.mdns, settings);
      });

      maxInput.addEventListener('input', () => {
        rule.max = numeric(maxInput.value);
        saveSettings(settings);
        target = payloadFromFlex(current.flexA, current.flexB, target.mdns, settings);
      });

      typeSelect.addEventListener('change', () => {
        rule.type = typeSelect.value;
        saveSettings(settings);
        ttsInput.style.display = rule.type === 'tts' ? 'block' : 'none';
        audioWrapper.style.display = rule.type === 'audio' ? 'flex' : 'none';
        target = payloadFromFlex(current.flexA, current.flexB, target.mdns, settings);
      });

      ttsInput.addEventListener('input', () => {
        rule.text = ttsInput.value;
        saveSettings(settings);
        target = payloadFromFlex(current.flexA, current.flexB, target.mdns, settings);
      });

      uploadBtn.addEventListener('click', () => {
        fileInput.click();
      });

      fileInput.addEventListener('change', (e) => {
        const file = e.target.files[0];
        if (!file) return;

        // Batasi ukuran file hingga 1MB (1.048.576 bytes) untuk mencegah melebihi kuota LocalStorage
        if (file.size > 1024 * 1024) {
          alert('Ukuran file suara terlalu besar! Harap upload file audio berukuran kurang dari 1MB (disarankan potongan suara pendek / efek alarm).');
          fileInput.value = '';
          return;
        }

        const reader = new FileReader();
        reader.onload = (evt) => {
          rule.audioData = evt.target.result;
          rule.audioName = file.name;
          saveSettings(settings);
          audioName.textContent = file.name;
          audioName.title = file.name;
          playPreviewBtn.style.display = 'inline-block';
          target = payloadFromFlex(current.flexA, current.flexB, target.mdns, settings);
        };
        reader.readAsDataURL(file);
      });

      playPreviewBtn.addEventListener('click', () => {
        if (rule.audioData) {
          try {
            const preview = new Audio(rule.audioData);
            preview.play();
          } catch (err) {
            console.error("Gagal putar preview", err);
          }
        }
      });

      deleteButton.addEventListener('click', () => {
        rules.splice(index, 1);
        saveSettings(settings);
        renderAudioRules();
      });

      container.appendChild(row);
    });
  }

  function renderAudioRules() {
    renderRuleSet('audioRulesA', settings.audio.flexARules, 'Flex A');
    renderRuleSet('audioRulesB', settings.audio.flexBRules, 'Flex B');
    target = payloadFromFlex(current.flexA, current.flexB, target.mdns, settings);
  }

  $('addAudioRuleA').addEventListener('click', () => {
    settings.audio.flexARules.push({ min: 0, max: 4095, text: 'Flex A baru' });
    saveSettings(settings);
    renderAudioRules();
  });

  $('addAudioRuleB').addEventListener('click', () => {
    settings.audio.flexBRules.push({ min: 0, max: 4095, text: 'Flex B baru' });
    saveSettings(settings);
    renderAudioRules();
  });
  renderAudioRules();

  let lastRuleId = null;
  let lastAudioPlayAt = 0;
  let activeRulePlayCount = 0; // Melacak jumlah loop pemutaran suara aturan aktif

  function handleAudioFeedback(flexA, flexB, settings) {
    if (!voiceEnabled || !modules.graphAudio) return;
    const now = performance.now();

    // Find active rules
    const activeRuleA = settings.audio.flexARules.find(r => flexA >= numeric(r.min) && flexA <= numeric(r.max));
    const activeRuleB = settings.audio.flexBRules.find(r => flexB >= numeric(r.min) && flexB <= numeric(r.max));

    const activeRules = [activeRuleA, activeRuleB].filter(Boolean);
    if (activeRules.length === 0) {
      lastRuleId = null; // Reset agar saat masuk ke threshold lagi suara bisa bunyi
      activeRulePlayCount = 0;
      return;
    }

    // Process first active rule
    const activeRule = activeRules[0];
    const ruleId = `${activeRule.min}_${activeRule.max}_${activeRule.type || 'tts'}_${activeRule.text || ''}_${activeRule.audioName || ''}`;

    if (ruleId === lastRuleId) {
      if (activeRulePlayCount >= 3) return; // Batasi maksimal 3 kali putar (loop) jika stay
      if (now - lastAudioPlayAt < 1500) return;
      activeRulePlayCount++;
    } else {
      // Aturan baru terpicu, reset counter ke 1
      lastRuleId = ruleId;
      activeRulePlayCount = 1;
    }

    lastAudioPlayAt = now;


    if (activeRule.type === 'audio' && activeRule.audioData) {
      try {
        if ('speechSynthesis' in window) speechSynthesis.cancel();
        const snd = new Audio(activeRule.audioData);
        snd.play();
        refs.voiceStatus.textContent = `Playing: ${activeRule.audioName}`;
      } catch (err) {
        console.error("Gagal memutar audio", err);
      }
    } else {
      const text = activeRule.text;
      if (text && 'speechSynthesis' in window) {
        speechSynthesis.cancel();
        const utterance = new SpeechSynthesisUtterance(text);
        utterance.lang = 'id-ID';
        utterance.rate = 1;
        speechSynthesis.speak(utterance);
        refs.voiceStatus.textContent = text;
      }
    }
  }

  function speak(text, force = false) {
    if (!text || !voiceEnabled || !modules.graphAudio || !('speechSynthesis' in window)) return;
    const now = performance.now();
    if (!force && (text === lastPhrase || now - lastVoiceAt < 1500)) return;
    lastPhrase = text;
    lastVoiceAt = now;
    speechSynthesis.cancel();
    const utterance = new SpeechSynthesisUtterance(text);
    utterance.lang = 'id-ID';
    utterance.rate = 1;
    speechSynthesis.speak(utterance);
    refs.voiceStatus.textContent = text;
  }

  refs.enableVoice.addEventListener('click', () => {
    voiceEnabled = !voiceEnabled;
    refs.enableVoice.textContent = voiceEnabled ? 'Disable Voice' : 'Enable Voice';
    refs.voiceStatus.textContent = voiceEnabled ? 'Audio ready' : 'Audio off';
  });

  refs.speakNow.addEventListener('click', () => {
    voiceEnabled = true;
    refs.enableVoice.textContent = 'Disable Voice';
    speak(target.phrase, true);
  });

  function drawChart() {
    const width = refs.chart.width;
    const height = refs.chart.height;
    ctx.clearRect(0, 0, width, height);
    ctx.fillStyle = '#08090a';
    ctx.fillRect(0, 0, width, height);

    ctx.strokeStyle = 'rgba(255,255,255,0.12)';
    ctx.lineWidth = 1;
    for (let i = 1; i < 4; i += 1) {
      const y = (height / 4) * i;
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(width, y);
      ctx.stroke();
    }

    const drawLine = (key, color, minVal, maxVal) => {
      ctx.strokeStyle = color;
      ctx.lineWidth = 3;
      ctx.beginPath();
      graph.forEach((point, index) => {
        const x = (index / (graph.length - 1)) * width;
        const normalized = clamp((point[key] - minVal) / (maxVal - minVal || 1), 0, 1);
        const y = height - normalized * height;
        if (index === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      ctx.stroke();
    };

    const minA = numeric(settings.audio.graphMinA, 2800);
    const maxA = numeric(settings.audio.graphMaxA, 3040);
    const minB = numeric(settings.audio.graphMinB, 2270);
    const maxB = numeric(settings.audio.graphMaxB, 2800);

    drawLine('flexA', '#3b82f6', minA, maxA);
    drawLine('flexB', '#79e39f', minB, maxB);
    ctx.fillStyle = '#8e959d';
    ctx.font = '12px Inter, system-ui, sans-serif';
    ctx.fillText('Flex A', 12, 20);
    ctx.fillText('Flex B', 72, 20);
  }

  function draw(time) {
    current.flexA = target.flexA;
    current.flexB = target.flexB;
    refs.flexA.textContent = current.flexA;
    refs.flexB.textContent = current.flexB;
    refs.headerFlex.textContent = `Flex A ${current.flexA} - Flex B ${current.flexB}`;
    refs.servoFlex.textContent = current[settings.servo.source] ?? current.flexA;

    current.servo = target.servo;

    if (modules.servo) {
      refs.needle.style.transform = `rotate(${-90 + current.servo}deg)`;
      refs.servo.textContent = Math.round(current.servo);
      if (refs.servoOutDisplay) refs.servoOutDisplay.textContent = Math.round(current.servo);
    }

    // Kirim sudut servo ke ESP32 via Serial/WebSocket jika modul servo ATAU scara/gripper aktif
    if (modules.servo || modules.gripper) {
      const servoAngle = Math.round(current.servo);
      if (servoAngle !== lastSentServoAngle) {
        if (serialActive && serialWriter) {
          lastSentServoAngle = servoAngle;
          serialWriter.write(encoder.encode(`SERVO:${servoAngle}\n`)).catch(() => {});
        } else if (ws && ws.readyState === WebSocket.OPEN) {
          lastSentServoAngle = servoAngle;
          ws.send(`SERVO:${servoAngle}\n`);
        }
      }
    }


    if (modules.gripper) {
      current.pan = target.pan;
      current.grip = target.grip;
      const panPx = current.pan * 2.05;
      const jawAngle = mapCalibrated(current.grip, 0, 100, 0, 25);
      refs.arm.style.transform = `translate3d(${panPx}px, 0, 0)`;
      refs.leftJaw.style.transform = `rotate(${-jawAngle}deg)`;
      refs.rightJaw.style.transform = `rotate(${jawAngle}deg)`;
      window.scaraViewer?.setPose(current.pan, current.grip);
      refs.pan.textContent = `${Math.round(current.pan)}%`;
      refs.grip.textContent = `${Math.round(current.grip)}%`;
    }

    if (modules.graphAudio && time - lastGraphAt > 65) {
      graph.push({ flexA: current.flexA, flexB: current.flexB });
      graph.shift();
      drawChart();
      handleAudioFeedback(current.flexA, current.flexB, settings);
      lastGraphAt = time;
    }

    requestAnimationFrame(draw);
  }

  // ── Firmware dropdown select event listener ─────────────────────────────
  const firmwareSelect = $('firmwareSelect');
  const espInstallBtn = $('espInstallBtn');
  if (firmwareSelect && espInstallBtn) {
    firmwareSelect.addEventListener('change', () => {
      const selected = firmwareSelect.value;
      espInstallBtn.setAttribute('manifest', `manifests/${selected}.json`);
      console.log(`Flasher: Manifest updated to manifests/${selected}.json`);
    });
  }

  setConnectionStatus('Disconnected', 'red');
  refs.leftJaw.style.transformOrigin = '324px 306px';
  refs.rightJaw.style.transformOrigin = '436px 306px';
  drawChart();
  requestAnimationFrame(draw);
  } catch (err) {
    console.error("FATAL ERROR in initSimulator:", err);
    alert("FATAL ERROR: " + err.message + "\nStack: " + err.stack);
  }
}

const sketch = `#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

const char* ssid = "NAMA_WIFI";
const char* password = "PASSWORD_WIFI";
const char* mdnsName = "flex-kelompok1";

const int FLEX_A_PIN = 34;
const int FLEX_B_PIN = 35;
const int SAMPLE_COUNT = 20;
const unsigned long SENSOR_INTERVAL_MS = 5;
const unsigned long SERIAL_INTERVAL_MS = 50;
const unsigned long WIFI_RETRY_INTERVAL_MS = 5000;

WebServer server(80);
int readingsA[SAMPLE_COUNT];
int readingsB[SAMPLE_COUNT];
int readIndex = 0;
long totalA = 0;
long totalB = 0;
int flexA = 0;
int flexB = 0;
unsigned long lastSensorRead = 0;
unsigned long lastSerialPrint = 0;
unsigned long lastWifiRetry = 0;
bool serverStarted = false;
bool mdnsStarted = false;

const char MONITOR_PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
<html>
  <head>
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>ESP32 Flex Monitor</title>
    <style>
      body{margin:0;padding:24px;font-family:Arial,sans-serif;color:#f4f7fb;background:#08090a}
      main{max-width:520px;margin:auto}h1{font-size:22px}section{display:grid;grid-template-columns:1fr 1fr;gap:12px}
      article{padding:18px;border:1px solid #2a2d31;border-radius:8px;background:#15171a}
      span{display:block;color:#8e959d;font-size:12px}strong{display:block;margin-top:8px;color:#79e39f;font-size:36px}
    </style>
  </head>
  <body>
    <main>
      <h1>ESP32 Flex Monitor</h1>
      <section>
        <article><span>Flex A</span><strong id="a">-</strong></article>
        <article><span>Flex B</span><strong id="b">-</strong></article>
      </section>
    </main>
    <script>
      async function update(){
        try{
          const response=await fetch('/data',{cache:'no-store'});
          const data=await response.json();
          document.getElementById('a').textContent=data.flexA;
          document.getElementById('b').textContent=data.flexB;
        }catch(error){}
      }
      async function refresh(){
        await update();
        setTimeout(refresh,25);
      }
      refresh();
    </script>
  </body>
</html>
)rawliteral";

void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void initializeFlexReadings() {
  int initialA = analogRead(FLEX_A_PIN);
  int initialB = analogRead(FLEX_B_PIN);
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    readingsA[i] = initialA;
    readingsB[i] = initialB;
    totalA += initialA;
    totalB += initialB;
  }
  flexA = initialA;
  flexB = initialB;
}

void updateFlexReadings() {
  totalA -= readingsA[readIndex];
  totalB -= readingsB[readIndex];

  readingsA[readIndex] = analogRead(FLEX_A_PIN);
  readingsB[readIndex] = analogRead(FLEX_B_PIN);

  totalA += readingsA[readIndex];
  totalB += readingsB[readIndex];

  readIndex++;
  if (readIndex >= SAMPLE_COUNT) {
    readIndex = 0;
  }

  flexA = totalA / SAMPLE_COUNT;
  flexB = totalB / SAMPLE_COUNT;

}

void handleData() {
  String json = "{\\\"flexA\\\":" + String(flexA) + ",\\\"flexB\\\":" + String(flexB) + "}";
  sendCors();
  server.send(200, "application/json", json);
}

void startMdns() {
  if (MDNS.begin(mdnsName)) {
    mdnsStarted = true;
    Serial.println("Monitor: http://" + String(mdnsName) + ".local");
    Serial.println("JSON API: http://" + String(mdnsName) + ".local/data");
  }
}

void startWebServer() {
  startMdns();
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", MONITOR_PAGE);
  });
  server.on("/data", HTTP_OPTIONS, []() {
    sendCors();
    server.send(204);
  });
  server.on("/data", HTTP_GET, handleData);
  server.begin();
  serverStarted = true;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetPinAttenuation(FLEX_A_PIN, ADC_11db);
  analogSetPinAttenuation(FLEX_B_PIN, ADC_11db);

  initializeFlexReadings();
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  Serial.println("Menghubungkan ke WiFi: " + String(ssid));
  WiFi.begin(ssid, password);
}

void loop() {
  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_INTERVAL_MS) {
    lastSensorRead = now;
    updateFlexReadings();
  }

  if (now - lastSerialPrint >= SERIAL_INTERVAL_MS) {
    lastSerialPrint = now;
    Serial.print("FlexA:");
    Serial.print(flexA);
    Serial.print(" FlexB:");
    Serial.println(flexB);
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!serverStarted) {
      Serial.println("WiFi tersambung");
      Serial.println("Monitor IP: http://" + WiFi.localIP().toString());
      startWebServer();
    } else if (!mdnsStarted) {
      Serial.println("WiFi tersambung kembali");
      Serial.println("Monitor IP: http://" + WiFi.localIP().toString());
      startMdns();
    }
    server.handleClient();
  } else {
    if (mdnsStarted) {
      MDNS.end();
      mdnsStarted = false;
    }
    if (now - lastWifiRetry >= WIFI_RETRY_INTERVAL_MS) {
      lastWifiRetry = now;
      Serial.println("WiFi belum tersambung, status: " + String(WiFi.status()));
      WiFi.reconnect();
    }
  }
}`;

function initVirtualEsp32() {
  const $ = (id) => document.getElementById(id);
  const flexAInput = $('flexAInput');
  const flexBInput = $('flexBInput');
  const fpsInput = $('fpsInput');
  const centerInputs = $('centerInputs');
  const flexARead = $('virtualFlexARead');
  const flexBRead = $('virtualFlexBRead');
  const fpsRead = $('fpsRead');
  const payloadPreview = $('payloadPreview');
  const virtualMdnsInput = $('virtualMdnsInput');
  const espSketch = $('espSketch');
  const copySketch = $('copySketch');
  const channel = createChannel(() => { });
  const settings = loadSettings();

  const flexAMinInput = $('flexAMinInput');
  const flexAMaxInput = $('flexAMaxInput');
  const flexBMinInput = $('flexBMinInput');
  const flexBMaxInput = $('flexBMaxInput');

  // Load calibration values directly from settings (synchronized with simulator.html)
  flexAMinInput.value = settings.servo.inMin;
  flexAMaxInput.value = settings.servo.inMax;
  flexBMinInput.value = settings.gripper.gripInMin;
  flexBMaxInput.value = settings.gripper.gripInMax;

  // Listen for settings change in other tabs
  window.addEventListener('storage', (event) => {
    if (event.key === SETTINGS_KEY) {
      try {
        Object.assign(settings, loadSettings());
        flexAMinInput.value = settings.servo.inMin;
        flexAMaxInput.value = settings.servo.inMax;
        flexBMinInput.value = settings.gripper.gripInMin;
        flexBMaxInput.value = settings.gripper.gripInMax;
        updatePayload();
      } catch (e) {}
    }
  });

  espSketch.textContent = sketch;
  copySketch.addEventListener('click', async () => {
    await navigator.clipboard?.writeText(sketch);
    copySketch.textContent = 'Copied';
    window.setTimeout(() => (copySketch.textContent = 'Copy'), 900);
  });

  virtualMdnsInput.value = normalizeMdns(localStorage.getItem(DEVICE_KEY)).replace('.local', '');

  let payload = payloadFromFlex(2048, 0, virtualMdnsInput.value, settings);
  let lastSend = 0;

  function updatePayload() {
    const mdns = normalizeMdns(virtualMdnsInput.value);
    localStorage.setItem(DEVICE_KEY, mdns);

    const minA = Number(flexAMinInput.value);
    const maxA = Number(flexAMaxInput.value);
    const minB = Number(flexBMinInput.value);
    const maxB = Number(flexBMaxInput.value);

    // Save calibration directly to main settings
    settings.servo.inMin = minA;
    settings.servo.inMax = maxA;
    settings.gripper.armInMin = minA;
    settings.gripper.armInMax = maxA;
    settings.gripper.gripInMin = minB;
    settings.gripper.gripInMax = maxB;
    saveSettings(settings);

    // Map percentage slider (0-100) to calibrated ADC range
    const valA = minA + (maxA - minA) * (Number(flexAInput.value) / 100);
    const valB = minB + (maxB - minB) * (Number(flexBInput.value) / 100);

    payload = payloadFromFlex(valA, valB, mdns, settings);
    flexARead.textContent = payload.flexA;
    flexBRead.textContent = payload.flexB;
    fpsRead.textContent = `${fpsInput.value} FPS`;
    payloadPreview.textContent = JSON.stringify(payload, null, 2);
  }

  function sendNow() {
    updatePayload();
    channel.send(payload);
  }

  [flexAInput, flexBInput, fpsInput, virtualMdnsInput, flexAMinInput, flexAMaxInput, flexBMinInput, flexBMaxInput].forEach((input) => {
    input.addEventListener('input', sendNow);
  });

  centerInputs.addEventListener('click', () => {
    flexAInput.value = 50; // Center is 50%
    flexBInput.value = 0;  // Open is 0%
    sendNow();
  });

  function stream(time) {
    const fps = Number(fpsInput.value);
    const frameMs = 1000 / fps;
    if (time - lastSend >= frameMs) {
      lastSend = time;
      sendNow();
    }
    requestAnimationFrame(stream);
  }

  sendNow();
  requestAnimationFrame(stream);
}

if (document.body.dataset.page === 'simulator') initSimulator();
if (document.body.dataset.page === 'virtual') initVirtualEsp32();
