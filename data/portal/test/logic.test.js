// Tests of the setup page's logic (spec 004 FR-014, FR-015, research R7, R8). Run: node --test data/portal/test
'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const NVP = require('../logic.js');

test('Turkish title case, same cases as the device (test_title_glyph.cpp)', () => {
  const cases = {
    'SULTANBEYLİ': 'Sultanbeyli', 'AĞRI': 'Ağrı', 'KÂĞITHANE': 'Kâğıthane', 'IĞDIR': 'Iğdır',
    '19 MAYIS': '19 Mayıs', 'AFYONKARAHİSAR': 'Afyonkarahisar', 'ÇANKAYA': 'Çankaya', 'İSTANBUL': 'İstanbul',
    'MERKEZ-KUZEY (YENİ).A': 'Merkez-Kuzey (Yeni).A'
  };
  for (const [upper, title] of Object.entries(cases)) assert.equal(NVP.titleCaseTr(upper), title);
});

test('validation rules and messages (02 §10)', () => {
  const ok = { ssid: 'Ev', auth: 'secured', password: '12345678', districtIndex: 0 };
  assert.equal(NVP.validate(ok), '');
  assert.equal(NVP.validate({ ...ok, ssid: '' }), 'Bir ağ seç');
  assert.equal(NVP.validate({ ...ok, ssid: 'x'.repeat(33) }), 'Bir ağ seç');
  assert.equal(NVP.validate({ ...ok, ssid: 'ğ'.repeat(16) }), '');       // 32 bytes
  assert.equal(NVP.validate({ ...ok, ssid: 'ğ'.repeat(17) }), 'Bir ağ seç');  // 34 bytes
  assert.equal(NVP.validate({ ...ok, password: '1234567' }), 'Şifre en az 8 karakter olmalı');
  assert.equal(NVP.validate({ ...ok, password: 'x'.repeat(64) }), 'Şifre en az 8 karakter olmalı');
  assert.equal(NVP.validate({ ...ok, auth: 'open', password: '' }), '');
  assert.equal(NVP.validate({ ...ok, auth: 'unknown', password: '' }), '');  // hidden network, may be open
  assert.equal(NVP.validate({ ...ok, auth: 'unknown', password: '123' }), 'Şifre en az 8 karakter olmalı');
  assert.equal(NVP.validate({ ...ok, districtIndex: -1 }), 'İl ve ilçe seç');
});

test('form body encoding (contracts/portal-http.md)', () => {
  const body = NVP.formBody('Ev Ağı 🏠', 'gizli&şifre', [9549, 'SULTANBEYLİ'], [539, 'İSTANBUL', []]);
  assert.equal(body, 'ssid=Ev+A%C4%9F%C4%B1+%F0%9F%8F%A0&password=gizli%26%C5%9Fifre&district_id=9549' +
    '&province=%C4%B0STANBUL&district=SULTANBEYL%C4%B0');
  assert.match(NVP.formBody('Kafe', '', [1, 'A'], [2, 'B', []]), /password=&/);
});

test('preselection by names, with districts sharing their centre id (OD4-3)', () => {
  const list = [[506, 'ANKARA', [[9206, 'ANKARA'], [9206, 'ÇANKAYA']]],
    [539, 'İSTANBUL', [[9541, 'İSTANBUL'], [9541, 'KADIKÖY'], [9549, 'SULTANBEYLİ']]]];
  assert.deepEqual(NVP.findPlace(list, { id: 9541, province: 'İSTANBUL', district: 'KADIKÖY' }), { province: 1, district: 1 });
  assert.deepEqual(NVP.findPlace(list, { id: 9206, province: 'ANKARA', district: 'ÇANKAYA' }), { province: 0, district: 1 });
  assert.deepEqual(NVP.findPlace(list, { id: 1, province: 'ANKARA', district: 'YOK' }), { province: 0, district: -1 });
  assert.equal(NVP.findPlace(list, null), null);
  assert.equal(NVP.findPlace(list, { id: 1, province: 'X', district: 'Y' }), null);
});

test('unsupported networks cannot be chosen (OD4-4)', () => {
  assert.equal(NVP.selectable({ auth: 'unsupported' }), false);
  assert.equal(NVP.selectable({ auth: 'open' }), true);
  assert.equal(NVP.selectable({ auth: 'secured' }), true);
  assert.equal(NVP.T.unsupported, 'desteklenmiyor');
});
