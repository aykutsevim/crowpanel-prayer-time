/* The setup page's flow (02-ui-spec.md §10; spec 004 FR-013..FR-015, contracts/portal-http.md).
 * After submitting, the device screen is the source of truth: the page shows one fixed message,
 * polls the status every 3 s for at most 60 s, and re-enables the form if the device is back in
 * setup (the phone may have dropped off the device's network by then). */
(function () {
  'use strict';
  var NVP = window.NVP, UI = window.NVPUI;
  var state = { ssid: '', auth: '', password: '', districtIndex: -1 };
  var provinces = [];
  var nets = [];
  var manual = false;
  var $ = function (id) { return document.getElementById(id); };

  var netSection = UI.section('Wi-Fi ağı');
  var listBox = UI.el('div');
  var refresh = UI.el('button', 'ghost', 'Yenile');
  refresh.type = 'button';
  var notListed = UI.el('button', 'link', 'Ağım listede yok');
  notListed.type = 'button';
  var manualField = UI.el('label', 'field hidden', 'Ağ adı');
  var manualInput = UI.el('input');
  manualInput.maxLength = 32;
  manualInput.autocapitalize = 'off';
  manualField.appendChild(manualInput);
  var pw = UI.passwordField('Wi-Fi şifresi');
  netSection.appendChild(listBox);
  var bar = UI.el('div', 'bar');
  bar.appendChild(refresh);
  bar.appendChild(notListed);
  netSection.appendChild(bar);
  netSection.appendChild(manualField);
  netSection.appendChild(pw.root);

  var placeSection = UI.section('Konum');
  var il = UI.select('İl');
  var ilce = UI.select('İlçe');
  placeSection.appendChild(il.root);
  placeSection.appendChild(ilce.root);

  var submit = UI.primaryButton('Kaydet ve bağlan');
  var problem = UI.message('alert');
  var done = UI.message('done');

  function renderNets() {
    listBox.innerHTML = '';
    listBox.appendChild(UI.networkList(nets, manual ? '' : state.ssid, function (n) {
      manual = false;
      manualField.classList.add('hidden');
      state.ssid = n.ssid;
      state.auth = n.auth;
      update();
    }));
  }

  function update() {
    if (manual) {
      state.ssid = manualInput.value;
      state.auth = 'unknown';  // a hidden network's security is not known: the device tries
    }
    state.password = pw.input.value;
    pw.root.classList.toggle('hidden', state.auth === 'open' || !state.ssid);
    if (state.auth === 'open') state.password = '';
    var why = NVP.validate(state);
    submit.disabled = why !== '';
    problem.textContent = why;
    renderNets();
  }

  function loadScan(again) {
    fetch('/api/scan' + (again ? '?refresh=1' : ''), { cache: 'no-store' }).then(function (r) { return r.json(); })
      .then(function (j) {
        nets = j.networks || [];
        update();
        if (j.scanning) setTimeout(function () { loadScan(false); }, 1500);
      }).catch(function () {});
  }

  function districts(p) { return p < 0 ? [] : provinces[p][2]; }

  il.select.onchange = function () {
    var p = il.select.value === '' ? -1 : Number(il.select.value);
    UI.fill(ilce.select, districts(p).map(function (d) { return NVP.titleCaseTr(d[1]); }), '');
    state.districtIndex = -1;
    update();
  };
  ilce.select.onchange = function () {
    state.districtIndex = ilce.select.value === '' ? -1 : Number(ilce.select.value);
    update();
  };
  pw.input.oninput = update;
  manualInput.oninput = update;
  refresh.onclick = function () { loadScan(true); };
  notListed.onclick = function () {
    manual = true;
    manualField.classList.remove('hidden');
    manualInput.focus();
    update();
  };

  function preselect(location) {
    var at = NVP.findPlace(provinces, location);
    if (!at) return;
    il.select.value = String(at.province);
    il.select.onchange();
    if (at.district >= 0) {
      ilce.select.value = String(at.district);
      state.districtIndex = at.district;
    }
    update();
  }

  function poll(until) {
    if (Date.now() > until) return;
    setTimeout(function () {
      fetch('/api/status', { cache: 'no-store' }).then(function (r) { return r.json(); }).then(function (j) {
        if (j.mode === 'setup') { form.classList.remove('sent'); done.textContent = ''; update(); return; }
        poll(until);
      }).catch(function () { poll(until); });
    }, 3000);
  }

  var form = $('f');
  form.appendChild(netSection);
  form.appendChild(placeSection);
  form.appendChild(submit);
  form.appendChild(problem);
  form.appendChild(done);
  form.onsubmit = function (e) {
    e.preventDefault();
    if (NVP.validate(state) !== '') return;
    var p = Number(il.select.value);
    var body = NVP.formBody(state.ssid, state.password, provinces[p][2][state.districtIndex], provinces[p]);
    submit.disabled = true;
    form.classList.add('sent');
    done.textContent = 'Cihaz Wi-Fi ağına bağlanıyor. Sonucu cihazın ekranından takip et.';
    fetch('/api/setup', { method: 'POST', body: body,
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' } }).catch(function () {});
    poll(Date.now() + 60000);
  };

  fetch('/places.json').then(function (r) { return r.json(); }).then(function (j) {
    provinces = j.provinces;
    UI.fill(il.select, provinces.map(function (p) { return NVP.titleCaseTr(p[1]); }), '');
    UI.fill(ilce.select, [], '');
    return fetch('/api/status', { cache: 'no-store' }).then(function (r) { return r.json(); });
  }).then(function (s) { preselect(s.location); }).catch(function () {});
  loadScan(false);
  update();
})();
