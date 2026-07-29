"""parser.py — Parse Prolog topology facts into a Python dict."""

import re


def parse_prolog(filepath):
    facts = {}
    with open(filepath, 'r') as f:
        content = f.read()
    content = re.sub(r'%.*', '', content)

    i = 0
    while i < len(content):
        while i < len(content) and content[i] in ' \t\n\r':
            i += 1
        if i >= len(content):
            break
        if content[i] == '.':
            i += 1
            continue
        match = re.match(r'(\w+)\s*\(', content[i:])
        if not match:
            i += 1
            continue
        name = match.group(1)
        start = i + match.end()
        depth = 1
        j = start
        while j < len(content) and depth > 0:
            if content[j] == '(':
                depth += 1
            elif content[j] == ')':
                depth -= 1
            j += 1
        if depth == 0:
            args_str = content[start:j-1]
            k = j
            while k < len(content) and content[k] in ' \t\n\r':
                k += 1
            if k < len(content) and content[k] == '.':
                args = _split_top_level(args_str)
                facts.setdefault(name, []).append(args)
                i = k + 1
                continue
        i = j
    return facts


def _split_top_level(s):
    parts, depth_p, depth_b, current = [], 0, 0, []
    for c in s:
        if c == '(':
            depth_p += 1
            current.append(c)
        elif c == ')':
            depth_p -= 1
            current.append(c)
        elif c == '[':
            depth_b += 1
            current.append(c)
        elif c == ']':
            depth_b -= 1
            current.append(c)
        elif c == ',' and depth_p == 0 and depth_b == 0:
            parts.append(''.join(current).strip())
            current = []
        else:
            current.append(c)
    if current:
        parts.append(''.join(current).strip())
    return parts


def parse_paths(paths_str):
    paths = []
    for match in re.finditer(r'path\((\w+),\s*(\w+)\)', paths_str):
        paths.append((match.group(1), match.group(2)))
    return paths


def validate_topology(facts):
    errors = []
    binds_list = facts.get('binds', [])
    polarities = facts.get('latch_polarity', [])
    ops = facts.get('valid_operator', [])
    gates = facts.get('operator_gate', [])

    bound_ports = {b[0] for b in binds_list}
    pol_ports = {p[0] for p in polarities}

    for port in bound_ports - pol_ports:
        errors.append(f"Port {port} bound but has no polarity.")
    for op_name, paths_str in ops:
        for port, _ in parse_paths(paths_str):
            if port not in bound_ports:
                errors.append(f"Operator {op_name} references unbounded port {port}.")
    op_names = {o[0] for o in ops}
    gated = {g[0] for g in gates}
    for op in op_names - gated:
        errors.append(f"Operator {op} has no gate assignment.")
    return errors
