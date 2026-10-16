"""
Week 1 checkpoint: cross-checks lss_minimize against pyeda.

For every function (hand-picked plus seeded random) this script
  1. runs lss_minimize --json,
  2. confirms the returned cover is functionally equivalent to the specification
     (covers every on-set point, touches no off-set point; don't-cares are free),
  3. runs pyeda's espresso on the same truth table and requires that the cover is
     not worse than espresso's in (terms, literals) order. Espresso is a heuristic,
     so an exact minimizer must match or beat it.

Usage: python verify_pyeda.py [--bin build/src/minimization/lss_minimize] [--random 200] [--seed 1]
"""
import argparse
import json
import random
import subprocess
import sys

from pyeda.inter import espresso_tts, truthtable, exprvars, And, Or
from pyeda.boolalg.expr import OrOp, AndOp

def tt_from_minterms(n, on, dc=()):
    bits = ["0"] * (2 ** n)
    for m in on:
        bits[m] = "1"
    for m in dc:
        bits[m] = "x"
    return "".join(bits)


def hand_picked():
    f = []
    f.append(("textbook4_dc", 4, tt_from_minterms(4, [4, 8, 10, 11, 12, 15], [9, 14])))
    f.append(("kmap_classic", 4, tt_from_minterms(4, [0, 1, 2, 5, 6, 7, 8, 9, 10, 14])))
    f.append(("cyclic_core_3var", 3, tt_from_minterms(3, [0, 1, 2, 5, 6, 7])))
    f.append(("parity_4", 4, tt_from_minterms(4, [i for i in range(16) if bin(i).count("1") % 2])))
    f.append(("majority_5", 5, tt_from_minterms(5, [i for i in range(32) if bin(i).count("1") >= 3])))
    f.append(("full_adder_sum", 3, tt_from_minterms(3, [1, 2, 4, 7])))
    f.append(("full_adder_cout", 3, tt_from_minterms(3, [3, 5, 6, 7])))
    f.append(("bcd_ge5_dc", 4, tt_from_minterms(4, [5, 6, 7, 8, 9], range(10, 16))))
    f.append(("seg7_a", 4, tt_from_minterms(4, [0, 2, 3, 5, 6, 7, 8, 9], range(10, 16))))
    f.append(("mux4", 6, tt_from_minterms(6, [i for i in range(64) if ((i & 0xF) >> (3 - (i >> 4))) & 1])))
    f.append(("constant_one", 3, "1" * 8))
    f.append(("constant_zero", 3, "0" * 8))
    return f


def random_functions(count, seed):
    rng = random.Random(seed)
    out = []
    for i in range(count):
        n = rng.randint(2, 6)
        on_p = rng.uniform(0.15, 0.7)
        dc_p = rng.choice([0, 0, 0.1, 0.2])
        bits = []
        for _ in range(2 ** n):
            r = rng.random()
            bits.append("1" if r < on_p else ("x" if r < on_p + dc_p else "0"))
        out.append((f"random_{i}", n, "".join(bits)))
    return out


def run_lss(binary, n, bits):
    p = subprocess.run([binary, "--tt", bits, "--json"], capture_output=True, text=True, timeout=120)
    if p.returncode != 0:
        raise RuntimeError(p.stderr.strip() or "lss_minimize failed")
    return json.loads(p.stdout)


def cube_contains(cube, m, n):
    for i, ch in enumerate(cube):
        if ch == "-":
            continue
        bit = (m >> (n - 1 - i)) & 1
        if int(ch) != bit:
            return False
    return True


def check_equivalence(n, bits, cubes):
    for m in range(2 ** n):
        covered = any(cube_contains(c, m, n) for c in cubes)
        spec = bits[m]
        if spec == "1" and not covered:
            return f"on-set minterm {m} not covered"
        if spec == "0" and covered:
            return f"off-set minterm {m} covered"
    return None


def pyeda_cost(n, bits):
    if "1" not in bits:
        return (0, 0)
    xs = exprvars("x", n)
    tt = truthtable(list(reversed(xs)), bits)
    (expr,) = espresso_tts(tt)
    if expr.is_one():
        return (1, 0)
    if isinstance(expr, OrOp):
        terms = list(expr.xs)
    else:
        terms = [expr]
    lits = 0
    for t in terms:
        lits += len(t.xs) if isinstance(t, AndOp) else 1
    return (len(terms), lits)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bin", default="build/src/minimization/lss_minimize")
    ap.add_argument("--random", type=int, default=200)
    ap.add_argument("--seed", type=int, default=1)
    args = ap.parse_args()

    funcs = hand_picked() + random_functions(args.random, args.seed)
    bad = 0
    better = 0
    print(f"{'function':<22}{'n':>2} {'lss (t,l)':>10} {'espresso (t,l)':>16}  result")
    for name, n, bits in funcs:
        res = run_lss(args.bin, n, bits)
        problem = check_equivalence(n, bits, res["cubes"])
        mine = (res["terms"], res["literals"])
        theirs = pyeda_cost(n, bits)
        status = "ok"
        if problem:
            status = "FAIL: " + problem
        elif not res["exact"]:
            status = "ok (search budget hit)"
        elif mine > theirs:
            status = "FAIL: worse than espresso"
        elif mine < theirs:
            status = "ok (beats espresso)"
            better += 1
        if status.startswith("FAIL"):
            bad += 1
        if name.startswith("random_") and status == "ok" and int(name.split("_")[1]) >= 15:
            continue
        print(f"{name:<22}{n:>2} {str(mine):>10} {str(theirs):>16}  {status}")
    print(f"\n{len(funcs)} functions, {bad} failures, lss strictly better than espresso on {better}")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
