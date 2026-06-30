// Navigation
document.querySelectorAll('[data-page]').forEach(el => {
  el.addEventListener('click', e => {
    e.preventDefault();
    const page = el.dataset.page;
    document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
    document.getElementById(`page-${page}`).classList.add('active');
    document.querySelectorAll('.nav-links a').forEach(a => a.classList.remove('active'));
    if (el.closest('.nav-links')) el.classList.add('active');
    history.pushState(null, '', `#${page}`);
    if (page === 'apps') loadApps();
    if (page === 'resources') loadResources('ir');
  });
});

window.addEventListener('popstate', () => {
  const page = location.hash.slice(1) || 'home';
  document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
  const el = document.getElementById(`page-${page}`);
  if (el) el.classList.add('active');
  document.querySelectorAll('.nav-links a').forEach(a => a.classList.remove('active'));
  const navLink = document.querySelector(`.nav-links a[data-page="${page}"]`);
  if (navLink) navLink.classList.add('active');
});

document.addEventListener('DOMContentLoaded', () => {
  const page = location.hash.slice(1) || 'home';
  if (page !== 'home') {
    document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
    document.getElementById(`page-${page}`).classList.add('active');
    document.querySelectorAll('.nav-links a').forEach(a => a.classList.remove('active'));
    const navLink = document.querySelector(`.nav-links a[data-page="${page}"]`);
    if (navLink) navLink.classList.add('active');
    if (page === 'apps') loadApps();
    if (page === 'resources') loadResources('ir');
  }
});

// App catalog
async function loadApps() {
  const grid = document.getElementById('apps-grid');
  try {
    const res = await fetch('data/apps.json');
    const data = await res.json();
    grid.innerHTML = data.apps.map(app => `
      <div class="app-card">
        <h3>${app.name}</h3>
        <div class="app-desc">${app.description}</div>
        <div class="app-meta">
          <span>${app.category}</span>
          <span>${app.version}</span>
          ${app.source ? `<span><a href="${app.source}" target="_blank">Source</a></span>` : ''}
        </div>
      </div>
    `).join('');
  } catch (e) {
    grid.innerHTML = '<p style="color:var(--red);text-align:center">Failed to load apps.</p>';
  }
}

// Resources
let allResources = [];

async function loadResources(cat) {
  const list = document.getElementById('resource-list');
  try {
    if (!allResources.length) {
      const res = await fetch('data/resources.json');
      allResources = await res.json();
    }
    const items = allResources.filter(r => r.category === cat);
    list.innerHTML = items.length
      ? items.map(r => `
        <div class="resource-item">
          <div class="resource-name">${r.name} <small>${r.size}</small></div>
          <button class="resource-dl" onclick="downloadResource('${r.url}', '${r.name}')">Download</button>
        </div>
      `).join('')
      : '<p style="color:var(--text-dim);text-align:center">No resources available.</p>';
  } catch (e) {
    list.innerHTML = '<p style="color:var(--red);text-align:center">Failed to load resources.</p>';
  }
}

function downloadResource(url, name) {
  const a = document.createElement('a');
  a.href = url;
  a.download = name;
  a.click();
}

// Tab switching
document.addEventListener('click', e => {
  const tab = e.target.closest('.tab');
  if (tab) {
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    tab.classList.add('active');
    loadResources(tab.dataset.cat);
  }
});
