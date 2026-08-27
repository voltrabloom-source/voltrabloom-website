---
trigger: model_decision
description: Best practices for Three.js 3D parametric models, procedural geometry, OrbitControls, and interactive HTML dashboards.
---

# Three.js 3D Visualizer Rules

1. **CDN Script Verification**:
   - Verify that all script CDN URLs point to explicit `.js` library endpoints (e.g., `three.min.js`, `OrbitControls.js`).
2. **Procedural Geometry Integrity**:
   - Ensure all `THREE.Shape()` paths call `.closePath()` and create corresponding extruded meshes attached to parent scene groups.
3. **Scene Lifecycle & Lifecycle Hooking**:
   - All sub-component builders must be explicitly called in `window.onload` or `DOMContentLoaded`.
4. **Clean Web File Format**:
   - Ensure HTML template exports are clean HTML5 and never wrapped in markdown code fences (`` ```html ``).
