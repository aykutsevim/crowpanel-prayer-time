"""Tests for tools/gen_places.py with canned responses, no network (spec 004 FR-021, OD4-3, research R6, R8).
Run: python -m unittest discover tools"""
import json
import random
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import gen_places as g  # noqa: E402

STATES = [
    {"_id": "539", "name": "İSTANBUL"},
    {"_id": "506", "name": "ANKARA"},
    {"_id": "504", "name": "AĞRI"},
]


def d(did, name, sid):
    return {"district_id": str(did), "diyanet_name": name, "state": {"id": str(sid)}}


DISTRICTS = [
    d(9549, "SULTANBEYLİ", 539), d(9541, "İSTANBUL", 539), d(100059, "KADIKÖY", 539), d(9535, "ARNAVUTKOY", 539),
    d(100060, "KÂĞITHANE", 539), d(9206, "ANKARA", 506), d(100001, "ÇANKAYA", 506), d(9205, "AYAŞ", 506),
    d(9400, "AĞRI", 504), d(9401, "DOĞUBAYAZIT", 504),
]
CENTRES = {100059: 9541, 100060: 9541, 100001: 9206}


def build(states=STATES, districts=DISTRICTS, centres=CENTRES):
    return g.build(states, districts, centres, expected_provinces=len(states), total_range=(1, 100))


class OrderTest(unittest.TestCase):
    def test_turkish_order_of_provinces(self) -> None:
        self.assertEqual([p[1] for p in build()], ["AĞRI", "ANKARA", "İSTANBUL"])  # Ğ after A, before N

    def test_tr_key(self) -> None:
        words = ["ÇORUM", "CİZRE", "IĞDIR", "İZMİR", "ÖDEMİŞ", "OSMANİYE", "ŞİLE", "SİNOP", "ÜSKÜDAR", "USAK", "19 MAYIS"]
        self.assertEqual(sorted(words, key=g.tr_key),
                         ["19 MAYIS", "CİZRE", "ÇORUM", "IĞDIR", "İZMİR", "OSMANİYE", "ÖDEMİŞ", "SİNOP", "ŞİLE",
                          "USAK", "ÜSKÜDAR"])
        self.assertEqual(g.tr_key("KÂĞITHANE")[:2], g.tr_key("KA"))  # Â sorts as A

    def test_centre_first_then_alphabetical_with_aliases_by_name(self) -> None:
        ist = [p for p in build() if p[1] == "İSTANBUL"][0]
        self.assertEqual(ist[0], 539)
        self.assertEqual(ist[2], [[9541, "İSTANBUL"], [9535, "ARNAVUTKOY"], [9541, "KADIKÖY"], [9541, "KÂĞITHANE"],
                                  [9549, "SULTANBEYLİ"]])

    def test_alias_takes_centre_id(self) -> None:
        ank = [p for p in build() if p[1] == "ANKARA"][0]
        self.assertIn([9206, "ÇANKAYA"], ank[2])


class FailureTest(unittest.TestCase):
    def test_wrong_province_count(self) -> None:
        with self.assertRaises(g.PlaceError):
            g.build(STATES[:2], DISTRICTS[:8], CENTRES, expected_provinces=3, total_range=(1, 100))

    def test_empty_province(self) -> None:
        with self.assertRaises(g.PlaceError):
            build(districts=[x for x in DISTRICTS if x["state"]["id"] != "504"])

    def test_alias_centre_in_another_province(self) -> None:
        with self.assertRaises(g.PlaceError):
            build(centres={100059: 9206, 100060: 9541, 100001: 9206})

    def test_alias_without_centre(self) -> None:
        with self.assertRaises(g.PlaceError):
            build(centres={100059: 9541, 100060: 9541})

    def test_duplicate_name(self) -> None:
        with self.assertRaises(g.PlaceError):
            build(districts=DISTRICTS + [d(9999, "AYAŞ", 506)])

    def test_total_out_of_range(self) -> None:
        with self.assertRaises(g.PlaceError):
            g.build(STATES, DISTRICTS, CENTRES, expected_provinces=3, total_range=(900, 1100))


class FileTest(unittest.TestCase):
    def test_document_round_trip_and_stable_checksum(self) -> None:
        provinces = build()
        self.assertEqual(g.checksum(provinces), g.checksum(build()))
        text = g.document(provinces, "2026-11-24")
        doc = json.loads(text)
        self.assertEqual(doc["generated"], "2026-11-24")
        self.assertEqual(doc["provinces"], provinces)
        self.assertIn("İSTANBUL", text)  # not \\u-escaped
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "p.json"
            path.write_text(text, encoding="utf-8")
            self.assertEqual(g.verify_file(path), provinces)
            path.write_text(text.replace("AYAŞ", "AYAS"), encoding="utf-8")
            with self.assertRaises(g.PlaceError):
                g.verify_file(path)


class FetchTest(unittest.TestCase):
    def test_fetch_all_and_spot_check_with_canned_answers(self) -> None:
        def fake(url: str):
            if "/api/locations/states" in url:
                return {"data": STATES}
            if "/api/v2/locations/districts" in url:
                page = int(url.split("page=")[1].split("&")[0])
                return {"data": DISTRICTS[:6] if page == 1 else DISTRICTS[6:], "meta": {"pagination": {"pages": 2}}}
            if "/api/v2/times" in url:
                did = int(url.split("district=")[1].split("&")[0])
                return {"data": {"source": {"type": "copied_from_center", "center_district_id": str(CENTRES[did])}}}
            if "/ilceler/" in url:
                pid = url.rsplit("/", 1)[1]
                return [{"IlceID": x["district_id"]} for x in DISTRICTS if x["state"]["id"] == pid]
            raise AssertionError(url)

        states, districts, centres = g.fetch_all(fake)
        self.assertEqual(len(districts), len(DISTRICTS))
        self.assertEqual(centres, CENTRES)
        provinces = build(states, districts, centres)
        g.spot_check(provinces, fake, random.Random(1))  # no exception

        def missing(url: str):
            return [] if "/ilceler/" in url else fake(url)

        with self.assertRaises(g.PlaceError):
            g.spot_check(provinces, missing, random.Random(1))


if __name__ == "__main__":
    unittest.main()


class CoordsTest(unittest.TestCase):
    """spec 007 T031, research R4: coordinates for every bundled entry, in the bundled order."""

    @staticmethod
    def with_coords(confidence="high", lat=39.9, lon=32.8):
        out = []
        for x in DISTRICTS:
            y = dict(x)
            y["coordinates"] = {"lat": lat, "lon": lon, "confidence": confidence}
            out.append(y)
        return out

    def test_one_row_per_entry_with_stored_id(self) -> None:
        provinces = build()
        rows = g.build_coords(provinces, self.with_coords())
        self.assertEqual(sum(len(p[2]) for p in provinces), len(rows))
        kadikoy = [r for r in rows if r[1] == "KADIKÖY"][0]
        self.assertEqual(9541, kadikoy[0])  # the centre's id, as the location stores it (OD4-3)
        self.assertEqual([3990000, 3280000], kadikoy[2:])

    def test_low_confidence_refused(self) -> None:
        with self.assertRaises(g.PlaceError):
            g.build_coords(build(), self.with_coords(confidence="low"))

    def test_outside_turkey_refused(self) -> None:
        with self.assertRaises(g.PlaceError):
            g.build_coords(build(), self.with_coords(lat=8.8888888, lon=7.7777777))

    def test_committed_table_matches_the_list(self) -> None:
        doc = json.loads(g.COORDS_OUT.read_text(encoding="utf-8"))
        listed = sum(len(p[2]) for p in g.verify_file(g.OUT))
        self.assertEqual(listed, len(doc["rows"]))
