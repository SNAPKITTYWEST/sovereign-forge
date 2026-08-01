# Makefile — DSSSL + Verilog Synthesis Pipeline
# sovereign-forge/dsssl/

.PHONY: all verilog dsssl clean install-deps

all: verilog dsssl

# ── Verilog hardware simulation ───────────────────────────────────────────────
verilog: refinement_eval.v
	@echo "[+] Compiling Verilog RTL..."
	@if command -v iverilog &>/dev/null; then \
	    iverilog -o /tmp/sim_refinement refinement_eval.v && \
	    vvp /tmp/sim_refinement; \
	else \
	    echo "  iverilog not found — run: brew install icarus-verilog"; \
	    echo "  or: sudo apt install iverilog"; \
	fi

# ── DSSSL + grove synthesis ───────────────────────────────────────────────────
dsssl: synthesis.dsl grove.sgml
	@echo "[+] Running DSSSL synthesis pipeline..."
	@chmod +x dsssl-verilog-synth && ./dsssl-verilog-synth --verbose

# ── JS kernel (from clojure-lisp-bridge) ─────────────────────────────────────
dsssl-js: grove.sgml
	@echo "[+] Running JS DSSSL kernel..."
	@if command -v node &>/dev/null; then \
	    node ../../snapkitty-clojure-lisp-bridge/dsssl-synthesis/dsssl-kernel.mjs grove.sgml; \
	else \
	    echo "  node not found"; \
	fi

clean:
	rm -f /tmp/sim_refinement

install-deps-mac:
	brew install icarus-verilog node

install-deps-linux:
	sudo apt install -y iverilog nodejs
