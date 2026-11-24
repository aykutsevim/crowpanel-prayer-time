/* Page components shared by the setup page and, from Slice 8, the settings page (02 §10;
 * spec 004 FR-013, research R7). Plain DOM, no framework, no external requests. */
(function (root) {
  'use strict';
  var NVP = root.NVP;

  function el(tag, cls, text) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (text != null) e.textContent = text;
    return e;
  }

  function section(title) {
    var s = el('section');
    s.appendChild(el('h2', null, title));
    return s;
  }

  var LOCK = '<svg class="lock" viewBox="0 0 12 14" width="11" height="13" aria-label="şifreli">' +
    '<path d="M3 6V4a3 3 0 0 1 6 0v2" fill="none" stroke="currentColor" stroke-width="1.6"/>' +
    '<rect x="1" y="6" width="10" height="7.5" rx="1.5" fill="currentColor"/></svg>';

  // Network list: one row per network, strongest first (the device sorts). onPick(net).
  function networkList(nets, chosen, onPick) {
    var list = el('div', 'nets');
    nets.forEach(function (n) {
      var row = el('button', 'net');
      row.type = 'button';
      var ok = NVP.selectable(n);
      row.disabled = !ok;
      if (chosen && chosen === n.ssid) row.className += ' chosen';
      row.appendChild(el('span', 'name', n.ssid));
      var meta = el('span', 'meta');
      meta.textContent = ok ? n.signal : NVP.T.unsupported;
      if (ok && n.auth === 'secured') meta.insertAdjacentHTML('beforeend', ' ' + LOCK);
      row.appendChild(meta);
      row.onclick = function () { if (ok) onPick(n); };
      list.appendChild(row);
    });
    return list;
  }

  // Password field with a "Göster" toggle.
  function passwordField(label) {
    var wrap = el('div', 'field');
    var l = el('label', null, label);
    var row = el('div', 'pw');
    var input = el('input');
    input.type = 'password';
    input.maxLength = 63;
    input.autocomplete = 'off';
    input.autocapitalize = 'off';
    input.spellcheck = false;
    var show = el('button', 'ghost', 'Göster');
    show.type = 'button';
    show.onclick = function () { input.type = input.type === 'password' ? 'text' : 'password'; };
    row.appendChild(input);
    row.appendChild(show);
    l.appendChild(row);
    wrap.appendChild(l);
    return { root: wrap, input: input };
  }

  function select(label) {
    var l = el('label', 'field', label);
    var s = el('select');
    l.appendChild(s);
    return { root: l, select: s };
  }

  function fill(sel, items, placeholder) {
    sel.innerHTML = '';
    var first = el('option', null, placeholder);
    first.value = '';
    sel.appendChild(first);
    items.forEach(function (text, i) {
      var o = el('option', null, text);
      o.value = String(i);
      sel.appendChild(o);
    });
  }

  function primaryButton(text) {
    var b = el('button', 'primary', text);
    b.type = 'submit';
    return b;
  }

  function message(cls) { return el('p', 'msg ' + (cls || '')); }

  root.NVPUI = { el: el, section: section, networkList: networkList, passwordField: passwordField,
    select: select, fill: fill, primaryButton: primaryButton, message: message };
})(typeof globalThis !== 'undefined' ? globalThis : this);
