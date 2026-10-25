#!/usr/bin/env bash
# Install only the user-facing CLI and Plasma widget. The existing system
# service is installed separately using Owly's normal packaging.
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="$project_dir/target/plasma"
target_dir="${CARGO_TARGET_DIR:-$project_dir/target}"
data_dir="${XDG_DATA_HOME:-$HOME/.local/share}"
widget_id=io.github.eirom16.owlykde

for tool in cargo cmake kpackagetool6; do
    command -v "$tool" >/dev/null || { echo "Falta instalar: $tool" >&2; exit 1; }
done

cd "$project_dir"
cargo build --release --locked -p owly-cli
cmake -S "$project_dir/plasma" -B "$build_dir" -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --parallel 2
install -Dm755 "$target_dir/release/owly" "$HOME/.local/bin/owly"
mkdir -p "$data_dir/plasma/plasmoids"
if [[ -f "$data_dir/plasma/plasmoids/$widget_id/metadata.json" ]]; then
    kpackagetool6 --type Plasma/Applet --upgrade "$build_dir/package"
else
    kpackagetool6 --type Plasma/Applet --install "$build_dir/package"
fi
echo 'Instalado. Abre «Añadir elementos gráficos», busca «Owly» y arrástralo al panel.'
echo 'El servicio owlyd debe estar instalado mediante el procedimiento habitual del proyecto.'
