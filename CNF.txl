define program
    [expression]
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


% =================================================================

redefine program
        ...
    |   [expression_lists]
end redefine

define expression_lists
    [repeat expression_chain_and]
end define

define expression_chain_and
    [expression_chain_or] [opt *]
end define

define expression_chain_or
        [expression_element]
    |   ( [repeat expression_element_chain] )
end define

define expression_element_chain
    [expression_element] [opt +]
end define

define expression_element
    [opt !] [id]
end define

% =================================================================

function convertToChains
  replace [program]
    P [expression]
  construct TypeHolder [expression_lists]
    % void
  construct newP [expression_lists]
    TypeHolder [reparse P]
  by
    newP
end function

function convertToTree
  replace [program]
    P [expression_lists]
  construct TypeHolder [expression]
    'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
  construct newP [expression]
    TypeHolder [reparse P]
  by
    newP
end function

% =================================================================

rule sort
  replace [repeat expression_element_chain]
    Ao [opt !] A [id] + Bo [opt !] B [id] Co [opt +] C [repeat expression_element_chain]
  construct A_str [stringlit]
    _ [quote A]
  construct B_str [stringlit]
    _ [quote B]
  where
    B_str [< A_str]
  by
    Bo B + Ao A Co C
end rule

rule simplify
  replace [expression_lists]
    E [expression_lists]
  construct NewE [expression_lists]
    E [collapseSelf]
  where not
    NewE [= E]
  by
    NewE
end rule

rule collapseSelf
  replace [repeat expression_element_chain]
    Ao [opt !] A [id] + Bo [opt !] B [id] Co [opt +] C [repeat expression_element_chain]
  construct A_str [stringlit]
    _ [quote A]
  construct B_str [stringlit]
    _ [quote B]
  where
    A_str [= B_str]
  by
    Bo B Co C
end rule


% =================================================================

function main
  replace [program]
    P [program]
  by
    P [buildCNF]
      [convertToChains]
      [sort]
      [simplify]
      [convertToTree]
end function
