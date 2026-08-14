# Mini-program tests

Mini-program tests complement the instruction-pure fixtures under `tests/isa`.
Each `x86/<case>/` directory contains:

- `program.asm`: a complete NASM x86-64 program with `_start`;
- `expected.txt`: exact expected native stdout;
- `translation.xfail`: optional marker while whole-program translation is
  expected to fail;
- `README.md`: program intent and the reason for an expected failure.

The build assembles and links a native ELF. CTest first executes that ELF and
compares stdout byte-for-byte, then invokes `sbt` on the same ELF. A case with
`translation.xfail` passes only when `sbt` returns nonzero. Remove that marker
when the required translator/runtime support lands; the translation test will
then require successful object emission.

Run only this suite with:

```sh
cmake --build build -j 8
ctest --test-dir build --output-on-failure -L mini-program
```
