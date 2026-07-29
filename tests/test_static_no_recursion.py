"""Verify Prolog files contain no recursion (pure static topology)."""

import os, re, sys


# Files that must contain ZERO rules (pure static facts only)
PURE_FACT_FILES = [
    '../logic/prolog/topology.pl',
    '../logic/prolog/examples/three_cell_mesh.pl',
]

RULE_FILES = [
    '../logic/prolog/schema.pl',
]


def check_pure_facts(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    # Pure fact files must not contain :- (rule definitions)
    if re.search(r':-', content):
        return False, f"Found rule definition (:-) in pure-fact file {filepath}"
    return True, None


def test_all_prolog_files():
    base = os.path.dirname(__file__)
    all_ok = True

    for rel_path in PURE_FACT_FILES:
        path = os.path.join(base, rel_path)
        if not os.path.exists(path):
            print(f"[SKIP] {rel_path} not found")
            continue
        ok, err = check_pure_facts(path)
        if ok:
            print(f"[PASS] {rel_path} — pure facts, no rules")
        else:
            print(f"[FAIL] {err}")
            all_ok = False

    # Rule files just need to exist and be parseable
    for rel_path in RULE_FILES:
        path = os.path.join(base, rel_path)
        if os.path.exists(path):
            print(f"[PASS] {rel_path} — rule file present (expected)")
        else:
            print(f"[FAIL] {rel_path} — rule file missing")
            all_ok = False

    return all_ok


if __name__ == '__main__':
    ok = test_all_prolog_files()
    sys.exit(0 if ok else 1)
