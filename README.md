# KDE Shortcut Popup

A fast, minimalist shortcut reference and search popup tailored specifically for KDE Plasma 6 (Wayland & X11).

![KDE Shortcut Popup Preview](file:///home/hugorobert/.gemini/antigravity/brain/93661854-8682-4074-8d16-28d3ce9fa17f/popup_centered.png)

## Highlights

- **Complete Discovery**: Reads 100% of global shortcuts live via KDE's DBus interface (`org.kde.kglobalaccel`), parses `~/.config/kglobalshortcutsrc`, scans `.desktop` files, and includes standard KDE editing/navigation actions.
- **Smart Minimalist Search**:
  - Search by application or component name (e.g. `kwin`, `spectacle`, `dolphin`, `konsole`).
  - Search by key combinations (e.g. `meta w`, `ctrl alt t`, `super w`, `shift print`).
  - Supports synonyms: `super`/`win`/`cmd` → `meta`, `control` → `ctrl`, `option` → `alt`, `enter` → `return`, `prtscn` → `print`.
  - Contextual keyword search: `screenshot`, `terminal`, `volume`, `tiling`, `sound`, etc.
- **Minimalist & Distraction-Free**:
  - Opens centered as a floating card popup with rounded corners and acrylic glass dark/light theme integration.
  - Automatically matches your KDE Plasma system color scheme (`Darkly` / `Breeze` / custom).
  - Renders keys as stylish, distinct keyboard badges (`[Meta] + [W]`).
  - Distinguishes user-customized shortcuts with a `[Custom]` tag.
- **Effortless Dismissal**:
  - Closes instantly when pressing <kbd>Esc</kbd>.
  - Closes instantly when clicking anywhere outside (auto-dismiss on focus loss).
  - Toggle behavior: invoking the shortcut again closes the popup if already open.
- **Quick Action**:
  - Pressing <kbd>Enter</kbd> or clicking any shortcut copies the key combination directly to your clipboard with a subtle toast notification.

---

## Installation & Setup

To install the popup and register it in KDE Plasma:

```bash
cd /home/hugorobert/Documents/Repos/KDE_Shortcut_popup
./scripts/install.sh
```

This will:
1. Compile the native Qt6 executable to `~/.local/bin/kde-shortcut-popup`.
2. Install the desktop launcher to `~/.local/share/applications/kde-shortcut-popup.desktop`.
3. Pre-assign the default global shortcut: <kbd>Meta</kbd> + <kbd>K</kbd>.

---

## How to Open & Use

- **Global Shortcut**: Press <kbd>Meta</kbd> + <kbd>K</kbd> (or your chosen shortcut) from anywhere on your desktop.
- **Application Launcher**: Search for **KDE Shortcut Popup** in the Application Menu or KRunner.
- **Terminal**: Run `kde-shortcut-popup`.

### Changing the Shortcut
You can easily reassign the shortcut at any time:
1. Open **System Settings**.
2. Navigate to **Keyboard** → **Shortcuts**.
3. Under Applications, click **KDE Shortcut Popup**.
4. Set any custom shortcut you prefer (e.g. <kbd>Meta</kbd> + <kbd>F1</kbd>, <kbd>Ctrl</kbd> + <kbd>Alt</kbd> + <kbd>?</kbd>, etc.).

---

## CLI Options

The binary also provides convenient command-line capabilities:

```bash
# Print all discovered shortcuts to terminal
kde-shortcut-popup --list

# Filter shortcuts directly from terminal
kde-shortcut-popup --search "kwin"
kde-shortcut-popup --search "ctrl alt t"

# Launch the GUI with a pre-filled search query
kde-shortcut-popup --query "spectacle"

# Show help
kde-shortcut-popup --help
```

---

## Building from Source

Dependencies (standard on Arch / CachyOS KDE Plasma):
- `qt6-base`, `qt6-declarative`
- `layer-shell-qt`
- `cmake`, `make`, `g++`

```bash
cmake -B build -S .
make -C build
```
