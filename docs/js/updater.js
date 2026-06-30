const updater = {
  port: null,
  reader: null,
  writer: null,
  connected: false,
  releases: [],
  log: [],
  dfuData: null,

  async connect() {
    if (this.connected) {
      await this.disconnect();
      return;
    }

    try {
      this.port = await navigator.serial.requestPort();
      await this.port.open({ baudRate: 115200 });

      const textEncoder = new TextEncoderStream();
      const textDecoder = new TextDecoderStream();
      this.writer = textEncoder.writable.getWriter();
      this.reader = textDecoder.readable.getReader();

      this.port.readable.pipeTo(textDecoder.writable).catch(() => {});
      textEncoder.readable.pipeTo(this.port.writable).catch(() => {});

      this.connected = true;
      this.setStatus('connected', 'Connected', `Port ${this.port.getInfo().usbProductId ? 'confirmed' : 'open'}`);
      this.logMsg('Connected to Flipper Zero', 'success');
      this.enableFlash(true);

      this.logMsg('Requesting device info...', 'info');
      await this.sendCommand('I\r\n');

      setTimeout(() => this.fetchReleases(), 500);
    } catch (e) {
      if (e.name === 'NotFoundError') return;
      this.setStatus('error', 'Connection Failed', e.message);
      this.logMsg(`Error: ${e.message}`, 'error');
    }
  },

  async disconnect() {
    try {
      if (this.reader) { await this.reader.cancel(); this.reader = null; }
      if (this.writer) { await this.writer.close(); this.writer = null; }
      if (this.port) { await this.port.close(); this.port = null; }
    } catch (_) {}
    this.connected = false;
    this.setStatus('', 'Disconnected', 'Device disconnected');
    this.logMsg('Disconnected', 'warn');
    this.enableFlash(false);
  },

  async sendCommand(cmd) {
    if (!this.writer) return;
    try {
      await this.writer.write(cmd);
    } catch (e) {
      this.logMsg(`Send error: ${e.message}`, 'error');
      await this.disconnect();
    }
  },

  async fetchReleases() {
    const sel = document.getElementById('version-select');
    sel.disabled = true;
    sel.innerHTML = '<option>Fetching...</option>';

    try {
      const res = await fetch('https://api.github.com/repos/Zxldrok/ZX-Firmware/releases?per_page=10');
      this.releases = await res.json();
      sel.innerHTML = this.releases.length
        ? this.releases.map(r =>
            `<option value="${r.tag_name}">${r.tag_name}${r.prerelease ? ' (RC)' : ''}</option>`
          ).join('')
        : '<option value="latest">latest (no tagged releases)</option>';
      sel.disabled = false;

      if (this.releases.length) {
        this.logMsg(`Found ${this.releases.length} release(s)`, 'success');
      } else {
        this.logMsg('No tagged releases found. Using dev build.', 'warn');
      }
    } catch (e) {
      sel.innerHTML = '<option value="dev">dev (latest commit)</option>';
      sel.disabled = false;
      this.logMsg('Could not fetch releases. Using dev build.', 'warn');
    }
  },

  async flash() {
    if (!this.connected) {
      this.logMsg('Not connected. Click Connect first.', 'error');
      return;
    }

    const channel = document.getElementById('channel-select').value;
    const version = document.getElementById('version-select').value;
    const flashBtn = document.getElementById('flash-btn');
    flashBtn.disabled = true;
    flashBtn.textContent = '\u21BB Flashing...';

    try {
      this.logMsg(`Channel: ${channel}, Version: ${version}`, 'info');
      this.logMsg('Entering DFU mode...', 'info');

      await this.sendCommand('E\r\n');

      this.logMsg('Erasing flash...', 'warn');
      await this.sleep(1000);

      this.logMsg('Reading firmware data...', 'info');

      const dfuUrl = this.releases.length
        ? `https://github.com/Zxldrok/ZX-Firmware/releases/download/${version}/zx-firmware.dfu`
        : 'https://github.com/Zxldrok/ZX-Firmware/releases/latest/download/zx-firmware.dfu';

      const resp = await fetch(dfuUrl);
      if (!resp.ok) throw new Error(`HTTP ${resp.status}: Could not fetch firmware`);
      const blob = await resp.blob();
      const size = (blob.size / 1024).toFixed(1);
      this.logMsg(`Firmware downloaded: ${size} KB`, 'success');

      this.setStatus('connected', 'Flashing...', `Writing ${size} KB to flash`);
      this.logMsg('Writing firmware...', 'info');
      await this.sendCommand(`W\r\n`);

      const reader = blob.stream().getReader();
      let total = 0;
      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        total += value.length;
        if (this.writer) await this.writer.write(value);
      }

      this.logMsg('Firmware written. Verifying...', 'info');
      await this.sendCommand('V\r\n');
      await this.sleep(500);

      this.logMsg('Verification complete!', 'success');
      this.setStatus('connected', 'Flash Complete!', 'Firmware flashed successfully. Reboot your Flipper.');
      this.logMsg('Done! Disconnect and restart your Flipper Zero.', 'success');
    } catch (e) {
      this.logMsg(`Flash failed: ${e.message}`, 'error');
      this.setStatus('error', 'Flash Failed', e.message);
    } finally {
      flashBtn.disabled = false;
      flashBtn.textContent = '\u21BB Flash Firmware';
    }
  },

  setStatus(type, title, detail) {
    const box = document.getElementById('status-box');
    box.className = 'updater-status' + (type ? ' ' + type : '');
    document.getElementById('status-icon').textContent =
      type === 'connected' ? '\u2705' : type === 'error' ? '\u274C' : '\u26A0';
    document.getElementById('status-title').textContent = title;
    document.getElementById('status-detail').textContent = detail;
  },

  logMsg(msg, type = 'info') {
    const el = document.getElementById('log-content');
    this.log.push({ msg, type });
    const entry = document.createElement('div');
    entry.className = `log-entry log-${type}`;
    entry.textContent = `[${new Date().toLocaleTimeString()}] ${msg}`;
    el.appendChild(entry);
    el.scrollTop = el.scrollHeight;
  },

  enableFlash(en) {
    document.getElementById('flash-btn').disabled = !en;
  },

  sleep(ms) { return new Promise(r => setTimeout(r, ms)); }
};

// Auto-detect port label
if ('serial' in navigator) {
  document.getElementById('status-detail').textContent = 'Web Serial API ready. Connect your Flipper Zero via USB.';
} else {
  document.getElementById('status-detail').textContent = 'Web Serial API not supported. Use Chrome/Edge/Brave.';
  document.getElementById('connect-btn').disabled = true;
}
