## CLRadeonExtender Policy versions

Because some things has been developed enough chaotically and these things will be later
cleaned up, the CLRX assembler provides simple mechanism to choice what behaviour
is needed: the policy version.

Currently, policy version controls following things:

* unified SGPRs count (for all formats SGPR count all SGPRs including extra registers)
  for policy version since 0.2 (200).

The policy version can be set by `.policy` pseudo-operation or by command line option
`--policy`. By default, policy version is same as CLRX assembler version.
