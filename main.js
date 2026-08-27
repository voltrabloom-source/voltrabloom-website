/* ============================================================
   VoltraBloom — Main Application Module
   Three.js (ES Module) + GSAP Scroll Animations + Preloader
   ============================================================ */

import * as THREE from 'three';

/* ===================== FEATURE DETECTION ===================== */
const prefersReducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
const isMobile = /Android|iPhone|iPad|iPod/i.test(navigator.userAgent);
const isLowEnd = (navigator.hardwareConcurrency || 4) < 4;
const useSimpleMaterials = isMobile || isLowEnd;

/* ===================== PRELOADER ===================== */
const preloaderEl = document.getElementById('preloader');
const preloaderFill = document.querySelector('.preloader-fill');
let preloaderProgress = 0;
let firstFrameRendered = false;
let fontsLoaded = false;

function updatePreloader(amount) {
  preloaderProgress = Math.min(preloaderProgress + amount, 100);
  if (preloaderFill) preloaderFill.style.width = preloaderProgress + '%';
}

function tryDismissPreloader() {
  if (firstFrameRendered && fontsLoaded && preloaderEl) {
    updatePreloader(100);
    setTimeout(() => {
      preloaderEl.classList.add('loaded');
      // Refresh ScrollTrigger after preloader hides
      if (window.ScrollTrigger) window.ScrollTrigger.refresh();
    }, 300);
  }
}

// Track font loading
document.fonts.ready.then(() => {
  fontsLoaded = true;
  updatePreloader(30);
  tryDismissPreloader();
});

// Fallback: dismiss after 4s regardless
setTimeout(() => {
  fontsLoaded = true;
  firstFrameRendered = true;
  tryDismissPreloader();
}, 4000);

/* ===================== THREE.JS SETUP ===================== */
const canvas = document.getElementById('c3d');
if (!canvas) throw new Error('Canvas #c3d not found');

// --- Renderer ---
const renderer = new THREE.WebGLRenderer({
  canvas,
  antialias: !isMobile,       // Disable AA on mobile for performance
  alpha: true,
  powerPreference: 'high-performance'
});
renderer.setPixelRatio(Math.min(window.devicePixelRatio, 1.5));
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.15;
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;
renderer.shadowMap.autoUpdate = false; // Manual shadow updates only
renderer.setClearColor(0xFAF6EE, 1);
updatePreloader(20);

// --- Scene ---
const scene = new THREE.Scene();

// --- Camera ---
const camera = new THREE.PerspectiveCamera(46, window.innerWidth / window.innerHeight, 0.1, 500);
camera.position.set(0, 14, 60);
const cameraTarget = new THREE.Vector3(0, 7, 0);
camera.lookAt(cameraTarget);

/* ===================== LIGHTING ===================== */
const ambient = new THREE.AmbientLight(0xFFF5E6, 0.45);
scene.add(ambient);

const sunLight = new THREE.DirectionalLight(0xFFF5E6, 1.8);
sunLight.position.set(18, 32, 18);
sunLight.castShadow = true;
sunLight.shadow.mapSize.set(1024, 1024); // Reduced from 2048
sunLight.shadow.camera.near = 0.5;
sunLight.shadow.camera.far = 120;
sunLight.shadow.camera.left = -45;
sunLight.shadow.camera.right = 45;
sunLight.shadow.camera.top = 45;
sunLight.shadow.camera.bottom = -45;
sunLight.shadow.bias = -0.0004;
sunLight.shadow.radius = 4;
scene.add(sunLight);

const fillLight = new THREE.DirectionalLight(0xE8D5C0, 0.4);
fillLight.position.set(-18, 12, -12);
scene.add(fillLight);

const rimLight = new THREE.DirectionalLight(0xC4B9A8, 0.25);
rimLight.position.set(0, 8, -25);
scene.add(rimLight);

/* ===================== MATERIALS ===================== */
// Glass: Use transparent MeshStandardMaterial instead of costly transmission
const glassMat = new THREE.MeshStandardMaterial({
  color: 0xF8F4ED,
  metalness: 0.05,
  roughness: 0.12,
  transparent: true,
  opacity: 0.22,
  side: THREE.DoubleSide,
  envMapIntensity: 0.6
});

const bodyMat = useSimpleMaterials
  ? new THREE.MeshStandardMaterial({ color: 0xE8E0D4, metalness: 0.05, roughness: 0.3 })
  : new THREE.MeshPhysicalMaterial({
      color: 0xE8E0D4, metalness: 0.05, roughness: 0.3,
      clearcoat: 0.4, clearcoatRoughness: 0.3, envMapIntensity: 0.8
    });

const darkMat = new THREE.MeshStandardMaterial({
  color: 0x3A3428, metalness: 0.15, roughness: 0.55
});

const accentMat = new THREE.MeshStandardMaterial({
  color: 0xE2A03F, metalness: 0.35, roughness: 0.35,
  emissive: 0xE2A03F, emissiveIntensity: 0.08
});

const solarPanelMat = new THREE.MeshStandardMaterial({
  color: 0x1A2840, metalness: 0.5, roughness: 0.2,
  emissive: 0x0A1428, emissiveIntensity: 0.12
});

const potMat = new THREE.MeshStandardMaterial({ color: 0x8B5E3C, roughness: 0.85 });
const soilMat = new THREE.MeshStandardMaterial({ color: 0x3D2B1F, roughness: 0.95 });

const innerContMat = useSimpleMaterials
  ? new THREE.MeshStandardMaterial({ color: 0xE8E0D4, transparent: true, opacity: 0.55, roughness: 0.2 })
  : new THREE.MeshPhysicalMaterial({
      color: 0xE8E0D4, metalness: 0.05, roughness: 0.2,
      transparent: true, opacity: 0.65,
      clearcoat: 0.6, envMapIntensity: 0.8
    });

const greenGlowMat = new THREE.MeshStandardMaterial({
  color: 0x4D7C0F, emissive: 0x4D7C0F, emissiveIntensity: 0.5,
  metalness: 0.2, roughness: 0.4
});

const copperMat = new THREE.MeshStandardMaterial({
  color: 0xC08840, metalness: 0.7, roughness: 0.3
});

const boardMat = new THREE.MeshStandardMaterial({ color: 0x2A4A08, roughness: 0.6, metalness: 0.3 });

updatePreloader(15);

/* ===================== MODEL CONSTRUCTION ===================== */
const modelGroup = new THREE.Group();
scene.add(modelGroup);

const WT = 0.3; // Wall thickness

/* --- BOX 1: Main Enclosure (22W × 15H × 22D) --- */
const B1W = 22, B1H = 15, B1D = 22;

// Bottom panel
const b1Bottom = new THREE.Mesh(new THREE.BoxGeometry(B1W, WT, B1D), bodyMat);
b1Bottom.position.set(-11, WT / 2, 0);
b1Bottom.receiveShadow = true;
modelGroup.add(b1Bottom);

// Back wall
const b1Back = new THREE.Mesh(new THREE.BoxGeometry(B1W, B1H, WT), glassMat);
b1Back.position.set(-11, B1H / 2, -B1D / 2);
b1Back.castShadow = true;
modelGroup.add(b1Back);

// Left wall
const b1Left = new THREE.Mesh(new THREE.BoxGeometry(WT, B1H, B1D), glassMat);
b1Left.position.set(-B1W, B1H / 2, 0);
b1Left.castShadow = true;
modelGroup.add(b1Left);

// Right wall (shared with Box 2)
const b1Right = new THREE.Mesh(new THREE.BoxGeometry(WT, B1H, B1D), glassMat);
b1Right.position.set(0, B1H / 2, 0);
b1Right.castShadow = true;
modelGroup.add(b1Right);

// Front wall with cutout (3 pieces)
const cutW = 9.6, cutH = 3.9;
const cutCx = -11;
const cutTop = WT + cutH;
const sideW = (B1W - cutW) / 2;

const fLeft = new THREE.Mesh(new THREE.BoxGeometry(sideW, B1H, WT), glassMat);
fLeft.position.set(cutCx - cutW / 2 - sideW / 2, B1H / 2, B1D / 2);
fLeft.castShadow = true;
modelGroup.add(fLeft);

const fRight = new THREE.Mesh(new THREE.BoxGeometry(sideW, B1H, WT), glassMat);
fRight.position.set(cutCx + cutW / 2 + sideW / 2, B1H / 2, B1D / 2);
fRight.castShadow = true;
modelGroup.add(fRight);

const topH = B1H - cutTop;
const fTop = new THREE.Mesh(new THREE.BoxGeometry(cutW, topH, WT), glassMat);
fTop.position.set(cutCx, cutTop + topH / 2, B1D / 2);
fTop.castShadow = true;
modelGroup.add(fTop);

// Top lid with two circular holes
const lidShape = new THREE.Shape();
lidShape.moveTo(-B1W / 2, -B1D / 2);
lidShape.lineTo(B1W / 2, -B1D / 2);
lidShape.lineTo(B1W / 2, B1D / 2);
lidShape.lineTo(-B1W / 2, B1D / 2);
lidShape.lineTo(-B1W / 2, -B1D / 2);

const h1 = new THREE.Path(); h1.absarc(-4, 5, 1.8, 0, Math.PI * 2, true);
lidShape.holes.push(h1);
const h2 = new THREE.Path(); h2.absarc(4, 5, 1.8, 0, Math.PI * 2, true);
lidShape.holes.push(h2);

const lidGeo = new THREE.ExtrudeGeometry(lidShape, { depth: 0.4, bevelEnabled: false });
const lid1 = new THREE.Mesh(lidGeo, bodyMat);
lid1.rotation.x = -Math.PI / 2;
lid1.position.set(-11, B1H, 0);
lid1.castShadow = true;
lid1.receiveShadow = true;
modelGroup.add(lid1);

// Inner container
const icW = 8.5, icH = 12, icD = 8.5;
const innerCont = new THREE.Mesh(new THREE.BoxGeometry(icW, icH, icD), innerContMat);
innerCont.position.set(-B1W + icW / 2, icH / 2, B1D / 2 - icD / 2);
innerCont.castShadow = true;
modelGroup.add(innerCont);

// Inner glow core
const innerCore = new THREE.Mesh(new THREE.BoxGeometry(4, 8, 4), greenGlowMat);
innerCore.position.set(-B1W + icW / 2, icH / 2, B1D / 2 - icD / 2);
modelGroup.add(innerCore);

// Circuit board
const board = new THREE.Mesh(new THREE.BoxGeometry(10, 0.2, 6), boardMat);
board.position.set(-5, WT + 0.1, -3);
board.receiveShadow = true;
modelGroup.add(board);

// Small components on board
for (let ci = 0; ci < 8; ci++) {
  const comp = new THREE.Mesh(
    new THREE.BoxGeometry(0.4 + Math.random() * 0.3, 0.25, 0.4 + Math.random() * 0.3),
    Math.random() > 0.5 ? darkMat : accentMat
  );
  comp.position.set(-8 + ci * 0.8, WT + 0.22, -5 + Math.random() * 4);
  modelGroup.add(comp);
}

/* --- BOX 2: Open Tray (22W × 5H × 22D) --- */
const B2W = 22, B2D = 22;
const fenceH = 3;

const b2Bottom = new THREE.Mesh(new THREE.BoxGeometry(B2W, WT, B2D), bodyMat);
b2Bottom.position.set(B2W / 2, WT / 2, 0);
b2Bottom.receiveShadow = true;
modelGroup.add(b2Bottom);

const b2Front = new THREE.Mesh(new THREE.BoxGeometry(B2W, fenceH, WT), glassMat);
b2Front.position.set(B2W / 2, WT + fenceH / 2, B2D / 2);
b2Front.castShadow = true;
modelGroup.add(b2Front);

const b2Back = new THREE.Mesh(new THREE.BoxGeometry(B2W, fenceH, WT), glassMat);
b2Back.position.set(B2W / 2, WT + fenceH / 2, -B2D / 2);
b2Back.castShadow = true;
modelGroup.add(b2Back);

const b2Right = new THREE.Mesh(new THREE.BoxGeometry(WT, fenceH, B2D), glassMat);
b2Right.position.set(B2W, WT + fenceH / 2, 0);
b2Right.castShadow = true;
modelGroup.add(b2Right);

/* --- SOLAR FLOWER --- */
const flowerGroup = new THREE.Group();
flowerGroup.position.set(-15, 15.2, -5);
modelGroup.add(flowerGroup);

const stem = new THREE.Mesh(new THREE.CylinderGeometry(0.2, 0.25, 2.5, 12), darkMat);
stem.position.y = 1.25;
stem.castShadow = true;
flowerGroup.add(stem);

const fHub = new THREE.Mesh(new THREE.CylinderGeometry(0.9, 0.9, 0.35, 16), accentMat);
fHub.position.y = 2.75;
fHub.castShadow = true;
flowerGroup.add(fHub);

// 5 solar panel petals
for (let pi = 0; pi < 5; pi++) {
  const pAngle = (pi / 5) * Math.PI * 2;
  const petal = new THREE.Mesh(new THREE.BoxGeometry(4, 0.12, 2), solarPanelMat);
  petal.position.set(Math.cos(pAngle) * 2.2, 2.85, Math.sin(pAngle) * 2.2);
  petal.rotation.y = pAngle;
  petal.rotation.x = -0.28;
  petal.castShadow = true;
  flowerGroup.add(petal);

  const vein = new THREE.Mesh(new THREE.BoxGeometry(3.6, 0.14, 0.08), accentMat);
  vein.position.set(Math.cos(pAngle) * 2.2, 2.86, Math.sin(pAngle) * 2.2);
  vein.rotation.y = pAngle;
  vein.rotation.x = -0.28;
  flowerGroup.add(vein);
}

const centerDisk = new THREE.Mesh(new THREE.CylinderGeometry(0.7, 0.7, 0.08, 16), solarPanelMat);
centerDisk.position.y = 2.98;
flowerGroup.add(centerDisk);

/* --- WIND TURBINE (VAWT) --- */
const turbineGroup = new THREE.Group();
turbineGroup.position.set(-7, 15.2, -5);
modelGroup.add(turbineGroup);

const pole = new THREE.Mesh(new THREE.CylinderGeometry(0.2, 0.25, 5, 12), darkMat);
pole.position.y = 2.5;
pole.castShadow = true;
turbineGroup.add(pole);

const tHub = new THREE.Mesh(new THREE.CylinderGeometry(0.35, 0.35, 0.4, 12), bodyMat);
tHub.position.y = 5.2;
tHub.castShadow = true;
turbineGroup.add(tHub);

const bladeAssembly = new THREE.Group();
bladeAssembly.position.y = 5.8;
turbineGroup.add(bladeAssembly);

for (let bi = 0; bi < 3; bi++) {
  const bAngle = (bi / 3) * Math.PI * 2;
  const bladeRadius = 1.3;

  const blade = new THREE.Mesh(new THREE.BoxGeometry(0.15, 3.8, 1.6), bodyMat);
  blade.position.set(Math.cos(bAngle) * bladeRadius, 0, Math.sin(bAngle) * bladeRadius);
  blade.rotation.y = bAngle;
  blade.castShadow = true;
  bladeAssembly.add(blade);

  const armTop = new THREE.Mesh(new THREE.BoxGeometry(bladeRadius + 0.3, 0.12, 0.12), darkMat);
  armTop.position.set(Math.cos(bAngle) * (bladeRadius / 2), 1.85, Math.sin(bAngle) * (bladeRadius / 2));
  armTop.rotation.y = bAngle;
  bladeAssembly.add(armTop);

  const armBot = armTop.clone();
  armBot.position.y = -1.85;
  bladeAssembly.add(armBot);
}

const tCap = new THREE.Mesh(new THREE.ConeGeometry(0.3, 0.5, 12), bodyMat);
tCap.position.y = 2.1;
tCap.castShadow = true;
bladeAssembly.add(tCap);

/* --- 5 SOIL POTS (dice-5 pattern) --- */
const potPositions = [[5, -5], [15, -5], [10, 0], [5, 5], [15, 5]];

potPositions.forEach(pos => {
  const pg = new THREE.Group();
  pg.position.set(pos[0], WT, pos[1]);
  modelGroup.add(pg);

  const pot = new THREE.Mesh(new THREE.CylinderGeometry(1.3, 1.0, 2.2, 16), potMat);
  pot.position.y = 1.1;
  pot.castShadow = true;
  pot.receiveShadow = true;
  pg.add(pot);

  const rim = new THREE.Mesh(new THREE.TorusGeometry(1.3, 0.08, 8, 24), potMat);
  rim.position.y = 2.2;
  rim.rotation.x = Math.PI / 2;
  pg.add(rim);

  const soil = new THREE.Mesh(new THREE.CylinderGeometry(1.25, 1.25, 0.15, 16), soilMat);
  soil.position.y = 2.2;
  pg.add(soil);

  const electrode = new THREE.Mesh(new THREE.CylinderGeometry(0.06, 0.06, 0.9, 8), copperMat);
  electrode.position.set(0.4, 2.65, 0);
  pg.add(electrode);

  const sprout = new THREE.Mesh(new THREE.SphereGeometry(0.22, 8, 6), greenGlowMat);
  sprout.position.set(-0.35, 2.4, 0.2);
  sprout.scale.y = 0.7;
  pg.add(sprout);

  const wire = new THREE.Mesh(new THREE.CylinderGeometry(0.03, 0.03, 0.4, 6), copperMat);
  wire.position.set(0.05, 2.5, 0.1);
  wire.rotation.z = 0.5;
  pg.add(wire);
});

/* --- Ground Plane --- */
const ground = new THREE.Mesh(
  new THREE.PlaneGeometry(300, 300),
  new THREE.MeshStandardMaterial({ color: 0xF0EAD8, roughness: 0.9 })
);
ground.rotation.x = -Math.PI / 2;
ground.position.y = -0.01;
ground.receiveShadow = true;
scene.add(ground);

updatePreloader(15);

// Trigger initial shadow render
renderer.shadowMap.needsUpdate = true;

/* ===================== SCROLL ANIMATION (GSAP) ===================== */
const { gsap, ScrollTrigger } = window;
gsap.registerPlugin(ScrollTrigger);

const cameraSlides = [
  { pos: [0, 14, 60],    target: [0, 7, 0] },      // Slide 1: Hero
  { pos: [-36, 28, 24],  target: [-18, 17, -5] },   // Slide 2: Solar
  { pos: [16, 28, 24],   target: [-5, 19, -5] },    // Slide 3: Wind
  { pos: [-14, 9, 32],   target: [6, 2, 0] },       // Slide 4: SMFC
  { pos: [0, 16, 65],    target: [0, 7, 0] },       // Slide 5: Gallery
];

let needsShadowUpdate = false;

if (!prefersReducedMotion) {
  // Master scroll-driven camera timeline
  const camTimeline = gsap.timeline({
    scrollTrigger: {
      trigger: '.content-layer',
      start: 'top top',
      end: 'bottom bottom',
      scrub: 1.8,
      invalidateOnRefresh: true,
    }
  });

  // Build transitions between each pair of slides
  // With 5 slides over 100% scroll, each occupies ~20%
  // Transitions happen in the first half of each section
  const transitionPoints = [0.12, 0.32, 0.52, 0.72];
  const transitionDuration = 0.12;

  for (let i = 1; i < cameraSlides.length; i++) {
    const slide = cameraSlides[i];
    const startPoint = transitionPoints[i - 1];

    camTimeline.to(camera.position, {
      x: slide.pos[0], y: slide.pos[1], z: slide.pos[2],
      duration: transitionDuration,
      ease: 'power2.inOut',
      onUpdate: () => { needsShadowUpdate = true; }
    }, startPoint);

    camTimeline.to(cameraTarget, {
      x: slide.target[0], y: slide.target[1], z: slide.target[2],
      duration: transitionDuration,
      ease: 'power2.inOut',
    }, startPoint);
  }

  // Canvas fade for Slide 5
  ScrollTrigger.create({
    trigger: '#slide-5',
    start: 'top 65%',
    end: 'top 15%',
    scrub: 1.5,
    onUpdate: (self) => {
      canvas.style.opacity = String(Math.max(0, 1 - self.progress * 1.05));
    },
    onLeaveBack: () => { canvas.style.opacity = '1'; }
  });
} else {
  // Reduced motion: instant camera positions on section enter
  cameraSlides.forEach((slide, index) => {
    ScrollTrigger.create({
      trigger: '#slide-' + (index + 1),
      start: 'top 55%',
      onEnter: () => {
        camera.position.set(...slide.pos);
        cameraTarget.set(...slide.target);
        needsShadowUpdate = true;
      },
      onEnterBack: () => {
        camera.position.set(...slide.pos);
        cameraTarget.set(...slide.target);
        needsShadowUpdate = true;
      }
    });
  });
}

/* --- Text Reveal (IntersectionObserver) --- */
const slideObs = new IntersectionObserver(entries => {
  entries.forEach(e => {
    if (e.isIntersecting) e.target.classList.add('visible');
  });
}, { threshold: 0.15 });

document.querySelectorAll('.slide-content').forEach(el => slideObs.observe(el));

/* --- Content fade-in --- */
gsap.fromTo('.content-layer',
  { opacity: 0 },
  { opacity: 1, duration: prefersReducedMotion ? 0 : 1.2, ease: 'power2.out', delay: 0.2 }
);

/* ===================== RENDER LOOP ===================== */
const clock = new THREE.Clock();
let isCanvasVisible = true;
let renderPaused = false;

// Pause rendering when canvas is off-screen or invisible
const canvasObserver = new IntersectionObserver(entries => {
  entries.forEach(e => { isCanvasVisible = e.isIntersecting; });
}, { threshold: 0 });
canvasObserver.observe(canvas);

function animate() {
  requestAnimationFrame(animate);

  // Skip rendering when canvas is hidden (opacity 0) or off-screen
  const canvasOpacity = parseFloat(canvas.style.opacity || '1');
  if (!isCanvasVisible || canvasOpacity <= 0.01) {
    renderPaused = true;
    return;
  }

  // If we were paused, trigger a shadow update on resume
  if (renderPaused) {
    needsShadowUpdate = true;
    renderPaused = false;
  }

  const elapsed = clock.getElapsedTime();

  if (!prefersReducedMotion) {
    // Wind turbine continuous rotation
    bladeAssembly.rotation.y += 0.014;

    // Solar flower subtle sway
    flowerGroup.rotation.y = Math.sin(elapsed * 0.25) * 0.06;
    flowerGroup.position.y = 15.2 + Math.sin(elapsed * 0.6) * 0.08;

    // Inner core glow pulse
    greenGlowMat.emissiveIntensity = 0.45 + Math.sin(elapsed * 1.2) * 0.15;
  }

  // Update shadows only when camera has moved
  if (needsShadowUpdate) {
    renderer.shadowMap.needsUpdate = true;
    needsShadowUpdate = false;
  }

  camera.lookAt(cameraTarget);
  renderer.render(scene, camera);

  // Signal first frame rendered for preloader
  if (!firstFrameRendered) {
    firstFrameRendered = true;
    updatePreloader(20);
    tryDismissPreloader();
  }
}

animate();

/* ===================== RESIZE HANDLER ===================== */
let resizeTimeout;
window.addEventListener('resize', () => {
  clearTimeout(resizeTimeout);
  resizeTimeout = setTimeout(() => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
    needsShadowUpdate = true;
    ScrollTrigger.refresh();
  }, 150); // Debounced
});

/* ===================== CLEANUP (for SPA navigation) ===================== */
window.__voltraCleanup = function() {
  renderer.dispose();
  scene.traverse(obj => {
    if (obj.geometry) obj.geometry.dispose();
    if (obj.material) {
      if (Array.isArray(obj.material)) {
        obj.material.forEach(m => m.dispose());
      } else {
        obj.material.dispose();
      }
    }
  });
  canvasObserver.disconnect();
  slideObs.disconnect();
  ScrollTrigger.getAll().forEach(t => t.kill());
};
