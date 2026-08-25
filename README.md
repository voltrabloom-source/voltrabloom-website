# VoltraBloom — Hybrid Energy Harvesting System Website

Production-ready static website showcasing the VoltraBloom hybrid energy harvesting system.

## Tech Stack

- **Three.js** (ES Module via import map) — 3D product visualization
- **GSAP + ScrollTrigger** — Scroll-driven camera animations
- **Vanilla CSS** — No runtime dependencies (replaces Tailwind CDN)
- **Inline SVG icons** — No icon library dependency

## Project Structure

```
├── index.html      — Main page (SEO, accessibility, preloader)
├── style.css       — Design system & all styles
├── main.js         — Three.js scene + GSAP scroll logic
├── vercel.json     — Deployment config (Vercel)
├── PAPER_VOLTRA.pdf — Research poster (add your file here)
└── README.md
```

## Local Development

Serve with any static file server:

```bash
# Python
python3 -m http.server 8080

# Node.js
npx serve .

# VS Code Live Server extension also works
```

> **Note:** ES modules require a proper HTTP server. Opening `index.html` directly from the filesystem will fail due to CORS restrictions on `import` statements.

## Deployment

### Vercel
```bash
npx vercel --prod
```

### Netlify
Drag-and-drop the project folder, or connect your Git repo.

### GitHub Pages
Push to a repo and enable Pages from Settings → Pages → Source: main branch.

## Production Checklist

- [ ] Replace gallery image URLs with locally hosted images (current CDN URLs have expiring auth tokens)
- [ ] Add `PAPER_VOLTRA.pdf` to the project root
- [ ] Update `og:url` in `index.html` with your actual domain
- [ ] Add `og:image` meta tag with a social sharing screenshot
- [ ] Update the "Live Data" button URL to your specific ThingsBoard dashboard

## Performance Optimizations

- Glass materials use `MeshStandardMaterial` with opacity instead of costly `transmission` (8× faster)
- Shadow map reduced to 1024×1024 with manual updates only on camera movement
- Render loop pauses when canvas is off-screen or invisible
- Pixel ratio capped at 1.5 for mobile GPU safety
- Mobile devices automatically get simplified materials
- Tailwind CDN runtime replaced with ~3KB hand-written CSS
- Lucide icon library replaced with inline SVGs (~5 icons)
- Debounced resize handler prevents layout thrashing

## Accessibility

- `prefers-reduced-motion` respected — all animations disabled
- Semantic HTML with proper heading hierarchy
- ARIA labels on interactive elements and 3D canvas
- Keyboard-navigable links and buttons
