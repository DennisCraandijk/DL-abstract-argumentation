Pakota : Solver for Extension and Status Enforcement in Abstract Argumentation
==============================================================================

Version    : 2020.07.12

Author     : [Andreas Niskanen](mailto:andreas.niskanen@helsinki.fi), University of Helsinki

This version of Pakota has been implemented using [PySAT](https://pysathq.github.io/),
in particular it makes use of the [RC2](https://pysathq.github.io/docs/html/api/examples/rc2.html) MaxSAT solver.
Please install PySAT by following [the instructions](https://pysathq.github.io/installation.html) before using this software.

Usage
-----

```
python3 pakota.py file mode sem

Arguments:
  file : Input filename for enforcement instance in apx format.
  mode : Enforcement variant. mode={strict|nonstrict|cred|skept}
    strict    : strict extension enforcement
    nonstrict : nonstrict extension enforcement
    cred      : credulous status enforcement
    skept     : skeptical status enforcement
  sem  : Argumentation semantics. sem={adm|com|prf|stb}
    adm       : admissible
    com       : complete
    prf       : preferred
    stb       : stable
```

Input format
------------

For extension enforcement, in the input file both an AF and an enforcement request is specified using the following predicates.

```
arg(X).   ... X is an argument
att(X,Y). ... there is an attack from X to Y
enf(X).   ... enforce argument X
```

For status enforcement, the AF is also represented via the `arg` and `att` predicates.
The arguments to be positively and negatively enforced are represented via the `pos` and `neg` predicates, respectively. 

```
arg(X).   ... X is an argument
att(X,Y). ... there is an attack from X to Y
pos(X).   ... positively enforce argument X
neg(Y).   ... negatively enforce argument Y
```

Contact
-------

Please direct any questions, comments, bug reports etc. directly to [the author](mailto:andreas.niskanen@helsinki.fi).
