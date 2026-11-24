/* Pure logic of the setup page (02-ui-spec.md §10; spec 004 FR-013..FR-015, research R7, R8).
 * No DOM here, so node --test can check it (data/portal/test). Shared with the settings page of
 * Slice 8. Plain ES2017, no framework. */
(function (root) {
  'use strict';

  var T = {
    pickNetwork: 'Bir ağ seç',
    passwordShort: 'Şifre en az 8 karakter olmalı',
    pickPlace: 'İl ve ilçe seç',
    unsupported: 'desteklenmiyor'
  };

  var LOWER = { 'I': 'ı', 'İ': 'i', 'Ç': 'ç', 'Ğ': 'ğ', 'Ö': 'ö', 'Ş': 'ş', 'Ü': 'ü', 'Â': 'â', 'Î': 'î', 'Û': 'û' };
  var UPPER = { 'i': 'İ', 'ı': 'I', 'ç': 'Ç', 'ğ': 'Ğ', 'ö': 'Ö', 'ş': 'Ş', 'ü': 'Ü', 'â': 'Â', 'î': 'Î', 'û': 'Û' };

  // Turkish title case without toLocaleLowerCase('tr'), which older WebViews get wrong (R8):
  // SULTANBEYLİ -> Sultanbeyli, IĞDIR -> Iğdır, 19 MAYIS -> 19 Mayıs.
  function titleCaseTr(name) {
    var out = '';
    var start = true;
    for (var i = 0; i < name.length; i++) {
      var c = name[i];
      c = LOWER[c] || c.toLowerCase();
      if (start) c = UPPER[c] || c.toUpperCase();
      out += c;
      start = c === ' ' || c === '-' || c === '(' || c === '.';
    }
    return out;
  }

  // The form's state -> the first rule it breaks, or '' when it can be submitted.
  // state: { ssid, auth: 'open'|'secured'|'unknown', password, districtIndex }
  function validate(state) {
    if (!state.ssid || utf8Length(state.ssid) > 32) return T.pickNetwork;
    var p = state.password || '';
    if (state.auth === 'secured' && p.length < 8) return T.passwordShort;
    if (state.auth === 'unknown' && p.length > 0 && p.length < 8) return T.passwordShort;
    if (utf8Length(p) > 63) return T.passwordShort;
    if (state.districtIndex == null || state.districtIndex < 0) return T.pickPlace;
    return '';
  }

  function utf8Length(s) {
    var n = 0;
    for (var i = 0; i < s.length; i++) {
      var c = s.charCodeAt(i);
      if (c < 0x80) n += 1;
      else if (c < 0x800) n += 2;
      else if (c >= 0xD800 && c <= 0xDBFF) { n += 4; i++; }
      else n += 3;
    }
    return n;
  }

  // application/x-www-form-urlencoded body of POST /api/setup (contracts/portal-http.md).
  function formBody(ssid, password, district, province) {
    function enc(v) { return encodeURIComponent(v).replace(/%20/g, '+'); }
    return 'ssid=' + enc(ssid) + '&password=' + enc(password || '') +
      '&district_id=' + district[0] + '&province=' + enc(province[1]) + '&district=' + enc(district[1]);
  }

  // Indices of a stored location in the place list, matched by names: two entries can share an
  // id (a district without its own times carries its centre's id, OD4-3).
  function findPlace(provinces, location) {
    if (!location) return null;
    for (var p = 0; p < provinces.length; p++) {
      if (provinces[p][1] !== location.province) continue;
      var ds = provinces[p][2];
      for (var d = 0; d < ds.length; d++) {
        if (ds[d][1] === location.district && ds[d][0] === location.id) return { province: p, district: d };
      }
      return { province: p, district: -1 };
    }
    return null;
  }

  function selectable(net) { return net.auth !== 'unsupported'; }

  var api = { T: T, titleCaseTr: titleCaseTr, validate: validate, formBody: formBody, findPlace: findPlace,
    selectable: selectable, utf8Length: utf8Length };
  root.NVP = api;
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
})(typeof globalThis !== 'undefined' ? globalThis : this);
