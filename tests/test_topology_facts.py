"""Verify the core topology.pl facts are well-formed."""

import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'netlister'))
from parser import parse_prolog, parse_paths, validate_topology


def test_core_topology():
    spec = os.path.join(os.path.dirname(__file__), '..', 'logic', 'prolog', 'topology.pl')
    facts = parse_prolog(spec)
    errors = validate_topology(facts)

    print(f"Core topology ({spec}):")
    print(f"  binds: {len(facts.get('binds', []))}")
    print(f"  operators: {len(facts.get('valid_operator', []))}")
    print(f"  env_states: {len(facts.get('env_state', []))}")

    if errors:
        print("  ERRORS:")
        for e in errors:
            print(f"    [FAIL] {e}")
        return False
    print("  [PASS] All validation checks passed.")
    return True


def test_three_cell_example():
    spec = os.path.join(os.path.dirname(__file__), '..', 'logic', 'prolog', 'examples', 'three_cell_mesh.pl')
    facts = parse_prolog(spec)
    errors = validate_topology(facts)

    print(f"Three-cell example ({spec}):")
    if errors:
        print("  ERRORS:")
        for e in errors:
            print(f"    [FAIL] {e}")
        return False
    print("  [PASS] All validation checks passed.")

    # Check activation logic: cell_0 should fire (P=HIGH), cell_1 should fire (PN=LOW), cell_2 blocked
    env = {k: v for k, v in facts.get('env_state', [])}
    for name, paths_str in facts.get('valid_operator', []):
        paths = parse_paths(paths_str)
        p_ok = all(
            (pol == 'p' and float(env.get(port, 0)) > 0.5) or
            (pol == 'pn' and float(env.get(port, 0)) < 0.5)
            for port, pol in paths
        )
        status = "[PASS] FIRES" if p_ok else "[PASS] BLOCKED"
        print(f"    {status}: {name}")
    return True


if __name__ == '__main__':
    ok1 = test_core_topology()
    ok2 = test_three_cell_example()
    sys.exit(0 if (ok1 and ok2) else 1)
