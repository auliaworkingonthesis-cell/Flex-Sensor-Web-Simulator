import * as THREE from './vendor/three/three.module.js';
import { GLTFLoader } from './vendor/three/GLTFLoader.js';
import { OrbitControls } from './vendor/three/OrbitControls.js';

const host = document.getElementById('scaraCanvasHost');
const fallback = document.getElementById('scaraFallback');
const loading = document.getElementById('scaraLoading');
const requestedPose = { pan: 0, grip: 0 };

function showFallback(message = 'SCARA 3D tidak tersedia') {
  if (host) host.hidden = true;
  if (fallback) fallback.hidden = false;
  if (loading) loading.textContent = message;
}

function findByNamePart(root, part) {
  const normalizedPart = part.toLowerCase().replace(/[^a-z0-9]/g, '');
  let match = null;
  root.traverse((object) => {
    const normalizedName = object.name.toLowerCase().replace(/[^a-z0-9]/g, '');
    if (!match && normalizedName.includes(normalizedPart)) match = object;
  });
  return match;
}

function clamp(value, min, max) { return Math.min(Math.max(value, min), max); }

function dominantAxis(v) { return ['x','y','z'].reduce((a,c) => v[c] > v[a] ? c : a, 'x'); }
function smallestAxis(v) { return ['x','y','z'].reduce((a,c) => v[c] < v[a] ? c : a, 'x'); }

function setRendererSize(renderer, camera) {
  const w = Math.max(host.clientWidth, 1), h = Math.max(host.clientHeight, 1);
  renderer.setSize(w, h, false);
  camera.aspect = w / h;
  camera.updateProjectionMatrix();
}

async function initialize() {
  if (!host || !fallback || !window.WebGLRenderingContext) { showFallback(); return; }

  const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 1.6));
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.setClearColor(0x090a0b, 1);
  host.append(renderer.domElement);

  const scene    = new THREE.Scene();
  const camera   = new THREE.PerspectiveCamera(34, 1, 0.01, 100);
  const controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;
  controls.enablePan     = true;
  controls.minPolarAngle = Math.PI * 0.16;
  controls.maxPolarAngle = Math.PI * 0.49;

  scene.add(new THREE.HemisphereLight(0xdcefff, 0x111216, 2.25));
  const keyLight = new THREE.DirectionalLight(0xffffff, 2.8);
  keyLight.position.set(4, 7, 4);
  scene.add(keyLight);
  const edgeLight = new THREE.DirectionalLight(0x38d5e8, 1.25);
  edgeLight.position.set(-4, 3, -3);
  scene.add(edgeLight);

  const p = new URLSearchParams(window.location.search);
  const modelUrl = p.get('scaraModel') || 'assets/models/scara-web.glb';
  const gltf  = await new GLTFLoader().loadAsync(modelUrl);
  const model = gltf.scene;
  model.rotation.z = -Math.PI / 2;  // GLB is Z-up; rotate to Y-up
  model.rotation.y = Math.PI;       // face correct direction
  scene.add(model);

  // ── Find model parts ────────────────────────────────────────────────────
  const rack     = findByNamePart(model, 'cremagliera:1');
  const pinion   = findByNamePart(model, 'pignone_17mm:1');
  const carriage = findByNamePart(model, 'assieme_pinza:1');
  const jaw      = findByNamePart(model, 'dito_pinza_mg90s:1');

  const rackStart      = rack?.position.clone();
  if (rack && rackStart) {
    rackStart.y -= 0.007; // Shift rack closer to pinion in local Y (depth alignment)
    rackStart.x += 0.0008; // Lower rack slightly in local X (vertical height alignment)
    rack.position.y = rackStart.y;
    rack.position.x = rackStart.x;
  }
  const pinionRotStart = pinion?.rotation.clone();
  const carriageStart  = carriage?.position.clone();
  if (carriage && carriageStart) {
    carriageStart.y -= 0.007; // Shift carriage closer to pinion in local Y (depth alignment)
    carriageStart.x += 0.0008; // Lower carriage slightly in local X (vertical height alignment)
    carriage.position.y = carriageStart.y;
    carriage.position.x = carriageStart.x;
  }
  const horn           = carriage ? findByNamePart(carriage, 'RCXAZ_9g singolo:1') : null;
  const screw          = carriage ? findByNamePart(carriage, 'Vite ISO 14580:2011(E) M2x8-4.8:1') : null;
  if (horn && screw) {
    horn.attach(screw);
    screw.position.y -= 0.0022; // Screw it in deeper (backward) by 2.2mm to make it flush
  }
  const jawQuatStart   = jaw?.quaternion.clone();
  const hornQuatStart  = horn?.quaternion.clone();

  // ── Set jaw pivot center to match servo shaft ───────────────────────────
  if (jaw) {
    // The exact screw axis in local jaw space (derived from Inventor constraint: [-0.65, 0, 0.13] cm)
    const pivotOffset = new THREE.Vector3(-0.650000, 0.000000, 0.130000);
    
    // Shift geometry vertices so local origin is exactly at the center of the hole
    jaw.traverse((child) => {
      if (child.isMesh && child.geometry) {
        child.geometry.translate(-pivotOffset.x, -pivotOffset.y, -pivotOffset.z);
      }
    });
    
    // Place the jaw node's origin (which is now the hole center) exactly on the screw axis
    const servoShaftCenter = new THREE.Vector3(-0.005650, -0.005164, 0.021122);
    servoShaftCenter.z = 0.0196;  // Keep it slightly backward to cover the horn
    jaw.position.copy(servoShaftCenter);
  }

  // ── Axis analysis (derived from GLB quaternion math) ────────────────────
  // rack local X maps to world Z → rack slides along LOCAL X
  // pinion local X is the rotation axis (perpendicular to gear face)
  // jaw local Y is the hinge pin axis (perpendicular to flat face)

  const rackSize   = rack   ? new THREE.Box3().setFromObject(rack).getSize(new THREE.Vector3())   : new THREE.Vector3();
  const pinionSize = pinion ? new THREE.Box3().setFromObject(pinion).getSize(new THREE.Vector3()) : new THREE.Vector3();

  // Revert sliding axis to model local Z
  const rackAxis   = 'z';
  const rackTravel = rackSize[rackAxis] * 0.35;

  // Pinion shaft axis = local 'z'  (confirmed by GLB quaternion: localZ → world-X)
  // world-X is perpendicular to rack-motion(world-Z) and vertical(world-Y) = correct shaft
  const pinionAxis   = 'z';
  const pinionPitchR = 0.0090; // Exact pitch radius for Module 1.0 gear with 18 teeth (9mm)

  // Jaw: rotate around local Z axis (cylinder axis of the mounting hole)
  const localHingeAxis = new THREE.Vector3(0, 0, 1);
  const jawHingeDir  = 1;
  const jawMaxAngle  = Math.PI / 2;  // 90° max (from vertical down to horizontal right)

  // ── Scale & position model ───────────────────────────────────────────────
  const initialBox    = new THREE.Box3().setFromObject(model);
  const initialSize   = initialBox.getSize(new THREE.Vector3());
  const initialCenter = initialBox.getCenter(new THREE.Vector3());
  const scale         = 3.7 / Math.max(initialSize.x, initialSize.y, initialSize.z);
  model.scale.setScalar(scale);
  model.position.set(-initialCenter.x * scale, -initialBox.min.y * scale, -initialCenter.z * scale);

  const modelBox    = new THREE.Box3().setFromObject(model);
  const modelSize   = modelBox.getSize(new THREE.Vector3());
  const modelCenter = modelBox.getCenter(new THREE.Vector3());
  const frameSize   = Math.max(modelSize.x, modelSize.y, modelSize.z);

  const ground = new THREE.Mesh(
    new THREE.CircleGeometry(frameSize * 0.78, 48),
    new THREE.MeshStandardMaterial({ color: 0x121416, roughness: 0.9, metalness: 0.08 }),
  );
  ground.rotation.x = -Math.PI / 2;
  ground.position.y = -0.014;
  scene.add(ground);

  // ── Camera ───────────────────────────────────────────────────────────────
  const sphere         = modelBox.getBoundingSphere(new THREE.Sphere());
  const cameraDistance = sphere.radius / Math.sin(THREE.MathUtils.degToRad(camera.fov / 2));
  const cameraDir      = new THREE.Vector3(-0.8, 1.16, -1.5).normalize(); // front view: neg-X, neg-Z
  const rackCenter     = rack ? new THREE.Box3().setFromObject(rack).getCenter(new THREE.Vector3()) : modelCenter;
  const visualFocus    = modelCenter.clone().lerp(rackCenter, 0.28);

  const savedCameraState = localStorage.getItem('scara-camera-state');
  let cameraRestored = false;
  if (savedCameraState) {
    try {
      const parsed = JSON.parse(savedCameraState);
      if (parsed.position && parsed.target) {
        camera.position.set(parsed.position.x, parsed.position.y, parsed.position.z);
        controls.target.set(parsed.target.x, parsed.target.y, parsed.target.z);
        cameraRestored = true;
      }
    } catch(e) {
      console.warn('Failed to load camera state', e);
    }
  }

  if (!cameraRestored) {
    camera.position.copy(visualFocus).add(cameraDir.multiplyScalar(cameraDistance));
    controls.target.copy(visualFocus);
  }

  controls.minDistance = cameraDistance * 0.05; // Allow zooming in extremely close to see details
  controls.maxDistance = cameraDistance * 1.75;
  controls.update();

  // Save camera view state when user interacts
  controls.addEventListener('change', () => {
    const state = {
      position: { x: camera.position.x, y: camera.position.y, z: camera.position.z },
      target: { x: controls.target.x, y: controls.target.y, z: controls.target.z }
    };
    localStorage.setItem('scara-camera-state', JSON.stringify(state));
  });

  window.scaraViewer = {
    setPose(pan, grip) {
      requestedPose.pan  = clamp(Number(pan)  || 0, -100, 100);
      requestedPose.grip = clamp(Number(grip) || 0,    0, 100);
    },
    getPose()      { return { ...requestedPose }; },
    getMechanics() {
      return {
        rackAxis, pinionAxis,
        rackPos:    rack?.position[rackAxis],
        pinionRot:  pinion?.rotation[pinionAxis],
        jawRot:     jaw?.rotation.x,
        carriagePos: carriage?.position[rackAxis],
      };
    },
  };

  if (loading) loading.hidden = true;
  const ro = new ResizeObserver(() => setRendererSize(renderer, camera));
  ro.observe(host);
  setRendererSize(renderer, camera);

  renderer.domElement.addEventListener('webglcontextlost', (e) => {
    e.preventDefault(); ro.disconnect(); showFallback('WebGL terputus - memakai fallback');
  });

  function animate() {
    if (host.hidden) return;
    const rackPosition = -(requestedPose.pan  / 100);  // Negated to reverse left/right direction
    const opening      = requestedPose.grip / 100;
    const rackOffset   = rackTravel * rackPosition;

    // Rack + carriage slide along dominant axis; startOffset nudges initial pos to align with pinion
    const rackStartOffset = -rackTravel * 0.15;
    if (rack && rackStart)
      rack.position[rackAxis] = rackStart[rackAxis] + rackOffset + rackStartOffset;

    // Pinion rotates around its shaft axis (opposite to rack travel direction)
    // Added +0.07 radians starting offset to match the teeth spacing and prevent collision
    if (pinion && pinionRotStart && pinionPitchR)
      pinion.rotation[pinionAxis] = pinionRotStart[pinionAxis] - rackOffset / pinionPitchR + 0.07;

    if (carriage && carriageStart)
      carriage.position[rackAxis] = carriageStart[rackAxis] + rackOffset + rackStartOffset;

    // Jaw rotates around physical servo shaft axis
    if (jaw && jawQuatStart) {
      jaw.quaternion.copy(jawQuatStart);
      jaw.rotateOnAxis(localHingeAxis, jawHingeDir * jawMaxAngle * opening);
    }

    // Horn rotates together with the jaw
    if (horn && hornQuatStart) {
      horn.quaternion.copy(hornQuatStart);
      horn.rotateOnAxis(new THREE.Vector3(0, 1, 0), jawHingeDir * jawMaxAngle * opening);
    }

    controls.update();
    renderer.render(scene, camera);
    requestAnimationFrame(animate);
  }

  animate();
}

initialize().catch((err) => { console.error('SCARA 3D gagal dimuat:', err); showFallback(); });
