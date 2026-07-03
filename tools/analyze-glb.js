// Script to analyze the GLB file and determine correct axes
// Run: node analyze-glb.js

const fs = require('fs');
const path = require('path');

// Minimal GLTF/GLB reader to get node transforms
const glbPath = path.join(__dirname, 'assets', 'models', 'scara-web.glb');
const buf = fs.readFileSync(glbPath);

// GLB header: magic(4) + version(4) + length(4)
const magic = buf.readUInt32LE(0);
if (magic !== 0x46546C67) { console.error('Not a GLB file'); process.exit(1); }

// Chunk 0: JSON
const jsonChunkLen  = buf.readUInt32LE(12);
// chunk type at 16 should be 0x4E4F534A = JSON
const jsonStr = buf.slice(20, 20 + jsonChunkLen).toString('utf8');
const gltf    = JSON.parse(jsonStr);

console.log('\n=== GLTF Nodes ===');
(gltf.nodes || []).forEach((n, i) => {
  const name        = n.name || `(node ${i})`;
  const translation = n.translation ? n.translation.map(v => v.toFixed(4)).join(', ') : 'none';
  const rotation    = n.rotation    ? n.rotation.map(v => v.toFixed(4)).join(', ')    : 'none';
  const scale       = n.scale       ? n.scale.map(v => v.toFixed(4)).join(', ')       : 'none';
  const mesh        = n.mesh !== undefined ? `mesh=${n.mesh}` : '';
  console.log(`[${i}] ${name.padEnd(40)} T:(${translation})  R:(${rotation})  S:(${scale})  ${mesh}`);
});

console.log('\n=== GLTF Meshes ===');
(gltf.meshes || []).forEach((m, i) => {
  console.log(`[${i}] ${m.name || `(mesh ${i})`}`);
});

console.log('\n=== Scene hierarchy ===');
function printNode(idx, indent = '') {
  const n = gltf.nodes[idx];
  if (!n) return;
  const meshName = n.mesh !== undefined ? (gltf.meshes[n.mesh]?.name || `mesh${n.mesh}`) : '';
  console.log(`${indent}[${idx}] ${n.name || '(unnamed)'}  ${meshName}`);
  (n.children || []).forEach(c => printNode(c, indent + '  '));
}
(gltf.scenes || []).forEach((scene, si) => {
  console.log(`Scene ${si}: ${scene.name || ''}`);
  (scene.nodes || []).forEach(ni => printNode(ni, '  '));
});
