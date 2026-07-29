"""Tests for the Prolog -> Verilog-A netlist compiler."""

import sys, os, tempfile, unittest
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'netlister'))
from parser import parse_prolog, validate_topology
from emit_veriloga import generate_testbench


class TestNetlistGeneration(unittest.TestCase):

    def test_parse_simple(self):
        content = """
        binds(port_1, sensor_a).
        latch_polarity(port_1, p).
        valid_operator(op_x, [path(port_1, p)]).
        operator_gate(op_x, gate_1).
        env_state(port_1, 1.8).
        """
        with tempfile.NamedTemporaryFile(mode='w', suffix='.pl', delete=False) as f:
            f.write(content); f.flush()
            facts = parse_prolog(f.name)
        os.unlink(f.name)
        self.assertIn('valid_operator', facts)

    def test_validate_ok(self):
        facts = {
            'binds': [['port_1', 'a']],
            'latch_polarity': [['port_1', 'p']],
            'valid_operator': [['op_x', '[path(port_1, p)]']],
            'operator_gate': [['op_x', 'g']],
            'env_state': [['port_1', '1.8']],
        }
        self.assertEqual(validate_topology(facts), [])

    def test_validate_missing_polarity(self):
        facts = {
            'binds': [['port_1', 'a']],
            'latch_polarity': [],
            'valid_operator': [['op_x', '[path(port_1, p)]']],
            'operator_gate': [['op_x', 'g']],
            'env_state': [['port_1', '1.8']],
        }
        self.assertTrue(any('polarity' in e for e in validate_topology(facts)))

    def test_validate_missing_gate(self):
        facts = {
            'binds': [['port_1', 'a']],
            'latch_polarity': [['port_1', 'p']],
            'valid_operator': [['op_x', '[path(port_1, p)]']],
            'operator_gate': [],
            'env_state': [['port_1', '1.8']],
        }
        self.assertTrue(any('gate' in e for e in validate_topology(facts)))

    def test_generate_2cell(self):
        facts = {
            'binds': [['p1', 'a'], ['p2', 'b']],
            'latch_polarity': [['p1', 'p'], ['p2', 'pn']],
            'valid_operator': [['op_a', '[path(p1, p)]'], ['op_b', '[path(p2, pn)]']],
            'operator_gate': [['op_a', 'ga'], ['op_b', 'gb']],
            'env_state': [['p1', '1.8'], ['p2', '0.0']],
        }
        out = generate_testbench(facts, os.devnull)
        self.assertIn('exo_mesh_2cell_tb', out)
        self.assertIn('exo_cell CELL_0', out)
        self.assertIn('exo_cell CELL_1', out)
        self.assertIn('sigma_source', out)
        self.assertIn('lossy_bus_segment', out)


if __name__ == '__main__':
    unittest.main()
