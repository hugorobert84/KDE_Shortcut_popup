#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_TARGET="$HOME/.local/bin/kde-shortcut-popup"
APP_DIR="$HOME/.local/share/applications"
DESKTOP_TARGET="$APP_DIR/kde-shortcut-popup.desktop"

echo "=== Installing KDE Shortcut Popup ==="

# 1. Build if binary does not exist
if [ ! -f "$SCRIPT_DIR/build/kde-shortcut-popup" ]; then
    echo "Building binary..."
    cmake -B "$SCRIPT_DIR/build" -S "$SCRIPT_DIR"
    make -C "$SCRIPT_DIR/build"
fi

# 2. Install binary to ~/.local/bin
mkdir -p "$HOME/.local/bin"
cp -f "$SCRIPT_DIR/build/kde-shortcut-popup" "$BIN_TARGET"
chmod +x "$BIN_TARGET"
echo "✓ Installed binary to $BIN_TARGET"

# 3. Install desktop entry
mkdir -p "$APP_DIR"
cp -f "$SCRIPT_DIR/kde-shortcut-popup.desktop" "$DESKTOP_TARGET"
chmod +x "$DESKTOP_TARGET"
echo "✓ Installed desktop entry to $DESKTOP_TARGET"

# 4. Update desktop database
if command -v update-desktop-database &>/dev/null; then
    update-desktop-database "$APP_DIR" 2>/dev/null || true
fi

# 5. Register global shortcut (Default: Meta+K)
SHORTCUT_KEY="${1:-Meta+K}"
echo "Registering global shortcut: $SHORTCUT_KEY"

# Unregister any old/stale entry in kglobalaccel
if command -v qdbus6 &>/dev/null; then
    qdbus6 org.kde.kglobalaccel /kglobalaccel org.kde.KGlobalAccel.unregister "kde-shortcut-popup.desktop" "_launch" &>/dev/null || true
fi

if command -v kwriteconfig6 &>/dev/null; then
    kwriteconfig6 --file kglobalshortcutsrc --group "services" --group "kde-shortcut-popup.desktop" --key "_launch" "$SHORTCUT_KEY,$SHORTCUT_KEY,Launch KDE Shortcut Popup" --notify
fi

# Reload kglobalaccel and KWin shortcuts
if command -v qdbus6 &>/dev/null; then
    qdbus6 org.kde.KWin /KWin reconfigure &>/dev/null || true
    qdbus6 org.kde.KGlobalAccel /kglobalaccel org.freedesktop.DBus.Peer.Ping &>/dev/null || true
fi

echo ""
echo "=== Installation Complete! ==="
echo "You can now launch it by:"
echo "  1. Pressing $SHORTCUT_KEY (or your customized global shortcut)"
echo "  2. Running 'kde-shortcut-popup' in any terminal or KRunner"
echo "  3. Launching 'KDE Shortcut Popup' from the Application Menu"
echo ""
echo "To change or assign a different shortcut anytime, open:"
echo "  KDE System Settings > Keyboard > Shortcuts > KDE Shortcut Popup"
