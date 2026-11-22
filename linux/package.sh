#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
root="$(pwd)/linux/portable"
mkdir -p "$root/bin" "$root/lib" "$root/licenses"
packages="$(mktemp)"
trap 'rm -f "$packages"' EXIT HUP INT TERM
printf "%s\n" ffmpeg gtk3 >> "$packages"
cp linux/build/silk-converter "$root/bin/"
cp /usr/bin/ffmpeg "$root/bin/"
for bin in "$root/bin/silk-converter" "$root/bin/ffmpeg"; do
  ldd "$bin" | awk '/=> \// {print $3}' | while IFS= read -r lib; do
    case "$(basename "$lib")" in libc.so.*|libm.so.*|libpthread.so.*|libdl.so.*|librt.so.*|libresolv.so.*|libutil.so.*|ld-linux*) continue;; esac
    cp -L -n "$lib" "$root/lib/"
    pacman -Qo -q "$(readlink -f "$lib")" 2>/dev/null >> "$packages" || true
  done
done
cat > "$root/silk-converter" <<'EOF'
#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export PATH="$root/bin:$PATH"
export LD_LIBRARY_PATH="$root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$root/bin/silk-converter" "$@"
EOF
chmod +x "$root/silk-converter"
cp LICENSE "$root/licenses/project-GPL-3.0.txt"
cp vendor/UPSTREAM-LICENSE.txt "$root/licenses/upstream.txt"
sort -u "$packages" -o "$packages"
: > "$root/licenses/PACKAGES.txt"
while IFS= read -r pkg; do
  pacman -Qi "$pkg" >> "$root/licenses/PACKAGES.txt"
  if [ -d "/usr/share/licenses/$pkg" ]; then cp -R "/usr/share/licenses/$pkg" "$root/licenses/"; fi
done < "$packages"
if [ -d /usr/share/licenses/common ]; then cp -R /usr/share/licenses/common "$root/licenses/common"; fi
ffmpeg -version > "$root/licenses/FFMPEG-BUILD.txt"
printf '%s\n' 'FFmpeg 9.0.2 source: https://ffmpeg.org/releases/ffmpeg-9.0.2.tar.xz' 'Arch build recipes: https://gitlab.archlinux.org/archlinux/packaging/packages/ffmpeg/-/tree/2-9.0.2-3' 'Package metadata and dependency licenses: PACKAGES.txt and subdirectories.' > "$root/licenses/SOURCES.txt"
printf '%s\n' 'Arch x86_64 bundle: run ./silk-converter. Requires compatible system glibc, kernel, graphics drivers and desktop. FFmpeg and detected shared libraries included. See licenses/SOURCES.txt and PACKAGES.txt for source and license information.' > "$root/README.txt"
tar -C linux -czf linux/silk-converter-linux-x86_64.tar.gz portable
echo "Bundle: $(pwd)/linux/silk-converter-linux-x86_64.tar.gz"
