#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 4 ]; then
  echo "usage: scripts/package-release.sh PLATFORM PACKAGE_NAME PACKAGE_ROOT BUILD_DIR" >&2
  exit 2
fi

platform="$1"
package_name="$2"
package_root="$3"
build_dir="$4"

if [ "$platform" != "macos" ]; then
  echo "unsupported platform for shell packager: $platform" >&2
  exit 2
fi

receive_bundle="$(find "$build_dir" -type d -name 'NozzleReceive.bundle' | sort | head -n 1)"
send_bundle="$(find "$build_dir" -type d -name 'NozzleSend.bundle' | sort | head -n 1)"
if [ -z "$receive_bundle" ] || [ -z "$send_bundle" ]; then
  echo "missing FFGL bundles under $build_dir" >&2
  exit 1
fi

rm -rf package "$package_name" package-contents.txt verify-package
mkdir -p "package/$package_root"
cp -R "$receive_bundle" "package/$package_root/NozzleReceive.bundle"
cp -R "$send_bundle" "package/$package_root/NozzleSend.bundle"
cp README.md "package/$package_root/README.md"
cp LICENSE "package/$package_root/LICENSE"
cp THIRD-PARTY-NOTICES.md "package/$package_root/THIRD-PARTY-NOTICES.md"

scripts/verify-macos-bundle.sh --sign-adhoc --require-adhoc-signature "package/$package_root/NozzleReceive.bundle"
scripts/verify-macos-bundle.sh --sign-adhoc --require-adhoc-signature "package/$package_root/NozzleSend.bundle"

(
  cd package
  zip -r "../$package_name" "$package_root"
)

test -f "$package_name"
unzip -l "$package_name" > package-contents.txt
grep -F "$package_root/NozzleReceive.bundle/Contents/MacOS/NozzleReceive" package-contents.txt
grep -F "$package_root/NozzleSend.bundle/Contents/MacOS/NozzleSend" package-contents.txt
grep -F "$package_root/README.md" package-contents.txt
grep -F "$package_root/LICENSE" package-contents.txt
grep -F "$package_root/THIRD-PARTY-NOTICES.md" package-contents.txt
