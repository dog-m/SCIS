
define program
        [expression]
    |   [simplified_expression]
end define

define expression
        [expression] [SP] + [SP] [expression]
    |   [term]
end define

define term
        [term] [SP] * [SP] [term]
    |   [inversion]
end define

define inversion
        [SPOFF] ! [inversion] [SPON]
    |   [primary]
end define

define primary
        [id]
    |   ( [expression] )
end define


% =================================================================

function main
  replace [program]
    P [expression]
  by
    P [buildCNF]
      [simplify]
end function

% =================================================================


rule buildCNF
  replace [expression]
    E [expression]
  construct NewE [expression]
    E [demorganD]
      [demorganC]
      [cancelDoubleNegation]
      [resolveBrackets]
      [resolveBracketsSimple]
      [resolveBracketsId]
      [resolveBracketsR]
      [resolveBracketsL]
      [distributeOr]
      [distributeOr2]
      [collapseSelfOr]
      [collapseSelfAnd]
      [collapseSelfOr2]
      [collapseSelfOr3]
      [collapseSelfOr4]
      [collapseSelfOr5]
      [collapseSelfId]
      [collapseSelfId2]
      %[sortOr]
      %[sortOr2]
  where not
    NewE [= E]
  by
    NewE
end rule


rule demorganD
  replace [expression]
    !( A [expression] + B [expression] )
  by
    ( !(A) * !(B) )
end rule


rule demorganC
  replace [term]
    !( A [term] * B [term] )
  by
    ( !(A) + !(B) )
end rule


rule cancelDoubleNegation
  replace [expression]
    !( ! A [inversion] )
  by
    A
end rule


rule resolveBrackets
  replace [expression]
    ( N [expression] )
  by
    N
end rule

rule resolveBracketsSimple
  replace [primary]
    ( N [primary] )
  by
    N
end rule

rule resolveBracketsId
  replace [expression]
    ( N [id] )
  by
    N
end rule


rule resolveBracketsR
  replace [term]
    A [term] * ( B [inversion] * C [inversion] )
  by
    A * B * C
end rule

rule resolveBracketsL
  replace [term]
    ( A [term] * B [inversion] ) * C [inversion]
  by
    A * B * C
end rule


rule distributeOr
  replace [expression]
    A [expression] + B [term] * C [inversion]
  by
    (A + B) * (A + C)
end rule

rule distributeOr2
  replace [expression]
    B [term] * C [inversion] + A [term]
  by
    (B + A) * (C + A)
end rule


rule collapseSelfOr
  replace * [expression]
    A [expression] + B [expression]
  construct A_str [stringlit]
    _ [quote A]
  construct B_str [stringlit]
    _ [quote B]
  where
    A_str [= B_str]
  by
    A
end rule

rule collapseSelfOr2
  replace * [expression]
    A [id] + ! B [id] + C [expression]
  where
    A [= B]
  by
    C
end rule

rule collapseSelfOr3
  replace * [expression]
    ! A [id] + B [id] + C [expression]
  where
    A [= B]
  by
    C
end rule

rule collapseSelfOr4
  replace * [expression]
    A [expression] + B [id] + ! C [id]
  where
    B [= C]
  by
    A
end rule

rule collapseSelfOr5
  replace * [expression]
    A [expression] + ! B [id] + C [id]
  where
    B [= C]
  by
    A
end rule

rule collapseSelfAnd
  replace [term]
    A [term] * B [term]
  where
    A [= B]
  by
    A
end rule

rule collapseSelfId
  replace * [expression]
    A [id] + B [id] + C [expression]
  where
    A [= B]
  by
    A + C
end rule

rule collapseSelfId2
  replace * [expression]
    A [expression] + B [id] + C [id]
  where
    B [= C]
  by
    A + B
end rule


rule sortOr
  replace [expression]
    A [inversion] + B [inversion] + C [expression]
  construct A_str [stringlit]
    _ [quote A]
  construct B_str [stringlit]
    _ [quote B]
  where
    A_str [> B_str]
  by
    B + A + C
end rule

rule sortOr2
  replace [expression]
    A [expression] + B [inversion] + C [inversion]
  construct B_str [stringlit]
    _ [quote B]
  construct C_str [stringlit]
    _ [quote C]
  where
    B_str [> C_str]
  by
    A + C + B
end rule

% =================================================================

define simplified_expression
  [number]
end define

% =================================================================

rule simplify
  match [expression]
    _ [expression]
end rule
