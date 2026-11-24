# Contract: screens, gestures and the harness (FR-004 to FR-009)

`lib/ui` depends only on LVGL and `lib/core` (constitution VI). The same sources render in `sim` and on the device.

## Screens (`lib/ui/src/setup/`)

```cpp
namespace nvp::ui {
lv_obj_t* setup_screen_create();                       // 02 §7.2, reference 15
void      setup_screen_update(const nvp::net::SetupView&);
lv_obj_t* connecting_screen_create();                  // 02 §7.3, reference 16
void      connecting_screen_update(const nvp::net::ConnectingView&);
// Status overlay: drawn on lv_layer_top() over the current main screen (02 §7.4, reference 17).
// Touch is classified by the app (TouchClassifier); overlay_hit() names the button a tap hit.
void      overlay_open(const nvp::net::OverlayView&);
void      overlay_update(const nvp::net::OverlayView&);
void      overlay_show_confirm(bool on);
void      overlay_close();
bool      overlay_is_open();
bool      overlay_confirm_shown();
enum class OverlayHit : uint8_t { None, Cancel, StartOver };
OverlayHit overlay_hit(int32_t x, int32_t y);
}
```

- **Updates are change-driven,** as on the main screen. Unchanged fields are not touched, so no redraw happens.
- **Strings** come from `nvp/text/tr.h` (new keys `kSetup*`, `kConnecting*`, `kOverlay*`, `kConfirm*`), copied byte for byte from 02 §7.2–§7.4.
- **No allocation after creation,** except the QR code's buffer, which is allocated once per screen. On the device LVGL's allocator has four extra 60 kB pools in PSRAM besides its 64 kB internal pool (research R16).

## Gestures (`nvp::screen::TouchClassifier`)

| Mode / screen | Gesture | Result |
|---|---|---|
| main screen | `Tap` | `overlay_open` |
| main screen | `TwoFingerHold5s` | diagnostic mode (Slice 3, unchanged) |
| overlay | `Tap` | `overlay_close` |
| overlay | `Hold5s` | `overlay_show_confirm(true)` |
| confirmation | tap `Vazgeç`, or 15 s | close the confirmation and the overlay; back to the main screen |
| confirmation | tap `Baştan kur` | post `StartOver` to the network worker |
| setup / connecting screens | any | nothing |

The overlay closes after 20 s without touch.

## Harness (`sim/ui_render`)

- **Fixtures 15, 16 and 17** are added to the fixture table and render to `out/snapshots/15-setup-ap.png` and the matching names.
- **`tools/compare_reference.py`** compares them with `docs/design/screens/15…17` within the tolerances of 02 §12. Each comparison also produces a side-by-side image for review.
- **`tools/check_snapshots.py`** gains geometry checks:
  - the QR square at (508, 86), 268×268;
  - the overlay panel at (100, 52), 600×376;
  - the step circles, 36 px;
  - the confirmation buttons, 200×56.
