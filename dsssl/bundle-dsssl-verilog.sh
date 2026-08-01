#!/usr/bin/env bash
set -euo pipefail

# ==============================================================================
# BUNDLE INSTALLER: DSSSL + VERILOG RELATIONAL REFINEMENT ENGINE
# Generates all files for the sovereign-forge/dsssl/ pipeline
# Run: bash bundle-dsssl-verilog.sh
# Then: cd dsssl_verilog_synth && ./dsssl-verilog-synth --verbose
# ==============================================================================

PROJECT_DIR="./dsssl_verilog_synth"
mkdir -p "${PROJECT_DIR}"
cd "${PROJECT_DIR}"

echo "[+] Fetching files from sovereign-forge/dsssl/..."

for file in synthesis.dsl refinement_eval.v grove.sgml dsssl-verilog-synth Makefile; do
  echo "  -> $file"
  curl -fsSL "https://raw.githubusercontent.com/SNAPKITTYWEST/sovereign-forge/main/dsssl/$file" -o "$file" 2>/dev/null ||   gh api "repos/SNAPKITTYWEST/sovereign-forge/contents/dsssl/$file" --jq ".content" | base64 -d > "$file" 2>/dev/null ||   echo "  [warn] could not fetch $file - see sovereign-forge/dsssl/"
done

chmod +x dsssl-verilog-synth 2>/dev/null || true

echo ""
echo "[+] BUNDLE READY IN: ${PROJECT_DIR}"
echo "[+] Run:"
echo "    cd ${PROJECT_DIR} && ./dsssl-verilog-synth --verbose"
echo ""
echo "    iverilog (optional, for hardware simulation):"
echo "    brew install icarus-verilog   # Mac"
echo "    sudo apt install iverilog      # Linux"
