"""Generate the bundled place list data/places_tr.json (LOC-02; spec 004 FR-021, OD4-3, research R6, R8).

Sources (04-data-sources.md §9, checked live on 2026-11-24, research R6):
  * İmsakiyem v1 `GET /api/locations/states?countryId=2`: the 81 provinces with Diyanet ids and
    upper-case names.
  * İmsakiyem v2 `GET /api/v2/locations/districts?countryId=2&page=N&limit=500`: 1003 districts.
    Ids below 100000 are Diyanet's. Ids from 100000 up are districts that Diyanet does not
    publish separately ("copied_from_center"); their centre id is read from one v2 `times` call
    each, and they are listed under the centre's id (OD4-3).
  * emushaf `GET /ilceler/{provinceId}`: spot check of five ids against the second source.

Polite use (constitution VIII): one request every 3.1 s (the provider allows 100 per 5 minutes),
User-Agent naming the project, TLS verified. A full run makes about 140 requests and takes about
seven minutes. Run it only to regenerate the committed list:

    python tools/gen_places.py
"""
from __future__ import annotations

import hashlib
import json
import random
import re
import ssl
import sys
import time
import urllib.error
import urllib.request
from datetime import date
from pathlib import Path
from typing import Callable

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "data" / "places_tr.json"
IMSAKIYEM = "https://ezanvakti.imsakiyem.com"
EMUSHAF = "https://ezanvakti.emushaf.net"
PACE_S = 3.1
ALIAS_MIN_ID = 100000
EXPECTED_PROVINCES = 81
TOTAL_RANGE = (900, 1100)
SPOT_FIXED = (9549, 9541, 9206)  # Sultanbeyli, İstanbul centre, Ankara centre (04 §3.2)

# Turkish alphabetical order (research R8). Q, W, X are not Turkish letters but sort where an
# English reader expects them. Â, Î, Û sort as A, İ, U.
_ALPHABET = "ABCÇDEFGĞHIİJKLMNOÖPQRSŞTUÜVWXYZ"
_FOLD = {"Â": "A", "Î": "İ", "Û": "U"}


class PlaceError(Exception):
    """The generated list would be wrong; nothing is written."""


def tr_key(name: str) -> list[int]:
    key = []
    for ch in name:
        ch = _FOLD.get(ch, ch)
        if ch == " ":
            key.append(0)
        elif ch.isdigit():
            key.append(1 + int(ch))
        elif ch in _ALPHABET:
            key.append(20 + _ALPHABET.index(ch))
        else:
            key.append(100 + ord(ch))
    return key


def build(states: list[dict], districts: list[dict], centre_of: dict[int, int],
          expected_provinces: int = EXPECTED_PROVINCES,
          total_range: tuple[int, int] = TOTAL_RANGE) -> list:
    """Compact list [[province_id, NAME, [[district_id, NAME], ...]], ...] in Turkish order."""
    if len(states) != expected_provinces:
        raise PlaceError(f"{len(states)} provinces, expected {expected_provinces}")
    names = {int(s["_id"]): s["name"].strip() for s in states}
    own_ids: dict[int, set[int]] = {pid: set() for pid in names}
    for d in districts:
        pid = int(d["state"]["id"])
        if pid not in names:
            raise PlaceError(f"district {d['district_id']} in unknown province {pid}")
        if int(d["district_id"]) < ALIAS_MIN_ID:
            own_ids[pid].add(int(d["district_id"]))
    groups: dict[int, list[list]] = {pid: [] for pid in names}
    seen: set[tuple[int, str]] = set()
    for d in districts:
        pid = int(d["state"]["id"])
        did = int(d["district_id"])
        name = d["diyanet_name"].strip()
        alias = did >= ALIAS_MIN_ID
        if alias:
            if did not in centre_of:
                raise PlaceError(f"alias {did} {name}: centre unknown")
            did = centre_of[did]
            if did not in own_ids[pid]:
                raise PlaceError(f"alias {name}: centre {did} is not a district of province {names[pid]}")
        if (pid, name) in seen:
            raise PlaceError(f"duplicate district {name} in {names[pid]}")
        seen.add((pid, name))
        groups[pid].append([did, name, alias])
    provinces = []
    for pid, pname in names.items():
        entries = groups[pid]
        if not entries:
            raise PlaceError(f"province {pname} has no districts")
        centre = [e for e in entries if e[1] == pname and not e[2]]
        rest = sorted((e for e in entries if e not in centre), key=lambda e: (tr_key(e[1]), e[0]))
        provinces.append([pid, pname, [[e[0], e[1]] for e in centre + rest]])
    provinces.sort(key=lambda p: tr_key(p[1]))
    total = sum(len(p[2]) for p in provinces)
    if not total_range[0] <= total <= total_range[1]:
        raise PlaceError(f"{total} districts, outside {total_range}")
    return provinces


def canonical(provinces: list) -> str:
    return json.dumps(provinces, ensure_ascii=False, separators=(",", ":"))


def checksum(provinces: list) -> str:
    return hashlib.sha256(canonical(provinces).encode("utf-8")).hexdigest()


def document(provinces: list, generated: str) -> str:
    """The file: one province per line, so a diff of a regenerated list stays readable."""
    head = {
        "generated": generated,
        "source": f"{IMSAKIYEM}/api/locations/states, /api/v2/locations/districts, /api/v2/times (centre ids)",
        "sha256": checksum(provinces),
    }
    lines = [json.dumps(head, ensure_ascii=False, separators=(",", ":"))[:-1] + ',"provinces":[']
    lines += [json.dumps(p, ensure_ascii=False, separators=(",", ":")) + "," for p in provinces]
    lines[-1] = lines[-1][:-1]
    lines.append("]}")
    return "\n".join(lines) + "\n"


def verify_file(path: Path) -> list:
    """Recomputes the checksum of a committed list (tools/check.py)."""
    doc = json.loads(path.read_text(encoding="utf-8"))
    if checksum(doc["provinces"]) != doc["sha256"]:
        raise PlaceError(f"{path}: checksum does not match its provinces")
    return doc["provinces"]


# --- network ---

def _tls() -> ssl.SSLContext:
    """Certificate verification stays on (constitution VIII); certifi's bundle when available."""
    try:
        import certifi  # type: ignore

        return ssl.create_default_context(cafile=certifi.where())
    except ImportError:
        return ssl.create_default_context()


def _version() -> str:
    text = (ROOT / "lib" / "core" / "src" / "nvp" / "version.h").read_text(encoding="utf-8")
    m = re.search(r'NVP_VERSION_STRING\s+"([^"]+)"', text)
    return m.group(1) if m else "dev"


class Fetcher:
    def __init__(self) -> None:
        self.ctx = _tls()
        self.ua = f"NamazVaktiPanel/{_version()} (place-list generator)"
        self.last = 0.0
        self.count = 0

    def __call__(self, url: str):
        wait = self.last + PACE_S - time.monotonic()
        if wait > 0:
            time.sleep(wait)
        req = urllib.request.Request(url, headers={"User-Agent": self.ua, "Accept": "application/json"})
        for attempt in range(3):  # a name lookup on this network fails now and then
            try:
                with urllib.request.urlopen(req, timeout=30, context=self.ctx) as r:
                    body = r.read()
                break
            except urllib.error.URLError as e:
                if attempt == 2 or isinstance(e, urllib.error.HTTPError):
                    raise
                print(f"  retry in 10 s after: {e.reason}", flush=True)
                time.sleep(10)
        self.last = time.monotonic()
        self.count += 1
        return json.loads(body.decode("utf-8"))


def fetch_all(fetch: Callable[[str], dict]) -> tuple[list, list, dict[int, int]]:
    states = fetch(f"{IMSAKIYEM}/api/locations/states?countryId=2")["data"]
    districts: list[dict] = []
    page = 1
    while True:
        doc = fetch(f"{IMSAKIYEM}/api/v2/locations/districts?countryId=2&page={page}&limit=500")
        districts += doc["data"]
        if page >= doc["meta"]["pagination"]["pages"]:
            break
        page += 1
    today = date.today().isoformat()
    centre_of: dict[int, int] = {}
    aliases = [d for d in districts if int(d["district_id"]) >= ALIAS_MIN_ID]
    for i, d in enumerate(aliases, 1):
        doc = fetch(f"{IMSAKIYEM}/api/v2/times?district={d['district_id']}&date={today}&period=daily")
        src = doc["data"]["source"]
        if src.get("type") != "copied_from_center":
            raise PlaceError(f"alias {d['district_id']}: source type {src.get('type')}")
        centre_of[int(d["district_id"])] = int(src["center_district_id"])
        print(f"  alias {i}/{len(aliases)}: {d['diyanet_name']} -> {src['center_district_id']}", flush=True)
    return states, districts, centre_of


def spot_check(provinces: list, fetch: Callable[[str], list], rng: random.Random) -> None:
    where = {}
    for pid, _, ds in provinces:
        for did, _ in ds:
            where.setdefault(did, pid)
    pool = sorted(set(where) - set(SPOT_FIXED))
    ids = list(SPOT_FIXED) + rng.sample(pool, 2)
    for did in ids:
        listed = fetch(f"{EMUSHAF}/ilceler/{where[did]}")
        found = any(int(x["IlceID"]) == did for x in listed)
        print(f"  spot check {did} in province {where[did]}: {'ok' if found else 'MISSING'}")
        if not found:
            raise PlaceError(f"spot check: district {did} not listed by emushaf for province {where[did]}")


def main() -> int:
    fetch = Fetcher()
    print("fetching provinces, districts and centre ids (one request every 3.1 s)...")
    states, districts, centre_of = fetch_all(fetch)
    provinces = build(states, districts, centre_of)
    spot_check(provinces, fetch, random.Random())
    OUT.write_text(document(provinces, date.today().isoformat()), encoding="utf-8", newline="\n")
    total = sum(len(p[2]) for p in provinces)
    print(f"{len(provinces)} provinces, {total} entries ({total - len(centre_of)} Diyanet + {len(centre_of)} "
          f"centre-mapped), {fetch.count} requests, sha256 {checksum(provinces)}")
    print(f"wrote {OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except PlaceError as e:
        sys.exit(f"gen_places: {e}")
