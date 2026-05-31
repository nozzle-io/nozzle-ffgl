#!/usr/bin/env bash
set -euo pipefail

sign_adhoc=0
require_adhoc_signature=0
while [ "$#" -gt 0 ]; do
  case "$1" in
    --sign-adhoc)
      sign_adhoc=1
      shift
      ;;
    --require-adhoc-signature)
      require_adhoc_signature=1
      shift
      ;;
    *)
      break
      ;;
  esac
done

if [ "$#" -ne 1 ]; then
  echo "usage: scripts/verify-macos-bundle.sh [--sign-adhoc] [--require-adhoc-signature] PLUGIN.bundle" >&2
  exit 2
fi

bundle_path="$1"
if [ ! -d "$bundle_path" ]; then
  echo "missing bundle: $bundle_path" >&2
  exit 1
fi

info_plist="$bundle_path/Contents/Info.plist"
if [ ! -f "$info_plist" ]; then
  echo "missing Info.plist: $info_plist" >&2
  exit 1
fi

executable="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$info_plist")"
bundle_identifier="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$info_plist")"
if [ -z "$executable" ]; then
  echo "empty CFBundleExecutable in $info_plist" >&2
  exit 1
fi
case "$bundle_identifier" in
  org.nozzle-io.ffgl.*) ;;
  *) echo "unexpected CFBundleIdentifier: $bundle_identifier" >&2; exit 1 ;;
esac

binary_path="$bundle_path/Contents/MacOS/$executable"
if [ ! -f "$binary_path" ]; then
  echo "missing bundle executable: $binary_path" >&2
  exit 1
fi

file "$binary_path" | tee plugin-file.txt
grep -F "Mach-O" plugin-file.txt
grep -F "bundle" plugin-file.txt

lipo -info "$binary_path" | tee plugin-lipo.txt
grep -F "arm64" plugin-lipo.txt
grep -F "x86_64" plugin-lipo.txt

nm -gU "$binary_path" | grep -E '(^| )_plugMain$' | tee plugin-symbols.txt

if [ "$sign_adhoc" -eq 1 ]; then
  codesign --force --sign - "$bundle_path"
fi

if [ "$require_adhoc_signature" -eq 1 ]; then
  codesign --verify --verbose=4 "$bundle_path" 2>&1 | tee plugin-codesign-verify.txt
  codesign --verify --strict --verbose=4 "$bundle_path" 2>&1 | tee plugin-codesign-strict-verify.txt
  codesign -dv --verbose=4 "$bundle_path" 2>&1 | tee plugin-codesign-details.txt
  grep -F "Signature=adhoc" plugin-codesign-details.txt
  grep -F "TeamIdentifier=not set" plugin-codesign-details.txt
fi
