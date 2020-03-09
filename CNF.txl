compounds
    <T>   <F>
end compounds

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
        [var_or_const]
    |   ( [expression] )
end define

define var_or_const
      [id]
  |   <T>
  |   <F>
end define


% =================================================================

rule buildCNF
  replace [expression]
    E [expression]
  construct NewE [expression]
    E [demorganD]
      [demorganC]
      
      [cancelDoubleNegation]
      [cancelDoubleNegation2]
      
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
      
      [collapseConstant_True]
      [collapseConstant_True2]
      [collapseConstant_True3]
      [collapseConstant_True_or]
      [collapseConstant_True_or2]
      [collapseConstant_True_and]
      [collapseConstant_True_and2]
      
      [collapseConstant_False]
      [collapseConstant_False2]
      [collapseConstant_False3]
      [collapseConstant_False_or]
      [collapseConstant_False_or2]
      [collapseConstant_False_and]
      [collapseConstant_False_and2]
      
      [consumeOr]
      [consumeOr2]
  where not
    NewE [= E]
  by
    NewE
end rule


rule demorganD
  replace [inversion]
    !( A [expression] + B [expression] )
  by
    ( !(A) * !(B) )
end rule


rule demorganC
  replace [inversion]
    !( A [term] * B [term] )
  by
    ( !(A) + !(B) )
end rule


rule consumeOr
  replace [expression]
    A [term] * B [term] + A
  by
    A
end rule

rule consumeOr2
  replace [expression]
    A [term] + A * B [term]
  by
    A
end rule


rule cancelDoubleNegation
  replace [inversion]
    !( ! A [inversion] )
  by
    A
end rule

rule cancelDoubleNegation2
  replace [inversion]
    ! ! A [inversion]
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
    ( N [var_or_const] )
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
    A [var_or_const] + ! B [var_or_const] + C [expression]
  where
    A [= B]
  by
    C
end rule

rule collapseSelfOr3
  replace * [expression]
    ! A [var_or_const] + B [var_or_const] + C [expression]
  where
    A [= B]
  by
    C
end rule

rule collapseSelfOr4
  replace * [expression]
    A [expression] + B [var_or_const] + ! C [var_or_const]
  where
    B [= C]
  by
    A
end rule

rule collapseSelfOr5
  replace * [expression]
    A [expression] + ! B [var_or_const] + C [var_or_const]
  where
    B [= C]
  by
    A
end rule

rule collapseSelfAnd
  replace [term]
    A [term] * A
  by
    A
end rule

rule collapseSelfId
  replace * [expression]
    A [var_or_const] + B [var_or_const] + C [expression]
  where
    A [= B]
  by
    A + C
end rule

rule collapseSelfId2
  replace * [expression]
    A [expression] + B [var_or_const] + C [var_or_const]
  where
    B [= C]
  by
    A + B
end rule


rule collapseConstant_True
  replace * [inversion]
    ! <T>
  by
    <F>
end rule

rule collapseConstant_True2
  replace * [expression]
    ! A [inversion] + A
  by
    <T>
end rule

rule collapseConstant_True3
  replace * [expression]
    A [inversion] + ! A
  by
    <T>
end rule

rule collapseConstant_True_or
  replace * [expression]
    A [expression] + <T>
  by
    <T>
end rule

rule collapseConstant_True_or2
  replace * [expression]
    <T> + A [expression]
  by
    <T>
end rule

rule collapseConstant_True_and
  replace * [term]
    A [term] * <T>
  by
    A
end rule

rule collapseConstant_True_and2
  replace * [term]
    <T> * A [term]
  by
    A
end rule


rule collapseConstant_False
  replace * [inversion]
    ! <F>
  by
    <T>
end rule

rule collapseConstant_False2
  replace * [term]
    ! A [inversion] * A
  by
    <F>
end rule

rule collapseConstant_False3
  replace * [term]
    A [inversion] * ! A
  by
    <F>
end rule

rule collapseConstant_False_or
  replace * [expression]
    A [expression] + <F>
  by
    A
end rule

rule collapseConstant_False_or2
  replace * [expression]
    <F> + A [expression]
  by
    A
end rule

rule collapseConstant_False_and
  replace * [term]
    A [term] * <F>
  by
    <F>
end rule

rule collapseConstant_False_and2
  replace * [term]
    <F> * A [term]
  by
    <F>
end rule


% =================================================================

redefine program
        ...
    |   [expression_lists]
end redefine

define expression_lists
        [repeat expression_chain_and]
    |   [repeat expression_element_chain]
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
    [opt !] [var_or_const]
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
    'SOME_USELESS_RANDOM_IDENTIFIER
  construct newP [expression]
    TypeHolder [reparse P]
  by
    newP
end function

% =================================================================

rule simplify
  replace [expression_lists]
    E [expression_lists]
  construct NewE [expression_lists]
    E [sort]
      [sort2]
      
      [collapseSelf_or]
      [collapseSelf_and]
      
      %[simplify_and]
      
      [fix_tail_of_and_chain]
  where not
    NewE [= E]
  by
    NewE
end rule


rule sort
  replace [repeat expression_element_chain]
    Ao [opt !] A [var_or_const] + Bo [opt !] B [var_or_const] Co [opt +] C [repeat expression_element_chain]
  construct A_str [stringlit]
    _ [quote A]
  construct B_str [stringlit]
    _ [quote B]
  where
    B_str [< A_str]
  by
    Bo B + Ao A Co C
end rule


rule sort2
  replace [repeat expression_chain_and]
    A [expression_chain_or] * B [expression_chain_or] Bo [opt *] C [repeat expression_chain_and]
  construct A_str [stringlit]
    _ [quote A]
  construct B_str [stringlit]
    _ [quote B]
  where
    B_str [< A_str]
  by
    B * A Bo C
end rule


rule collapseSelf_or
  replace [repeat expression_element_chain]
    A [expression_element] + A Bo [opt +] C [repeat expression_element_chain]
  by
    A Bo C
end rule


rule collapseSelf_and
  replace [repeat expression_chain_and]
    A [expression_chain_or] * A Bo [opt *] C [repeat expression_chain_and]
  by
    A Bo C
end rule


rule fix_tail_of_and_chain
  replace [repeat expression_chain_and]
    B [expression_chain_or] *
  by
    B
end rule


% =================================================================

function main
  replace [program]
    P [program]
  by
    P [buildCNF]
      [convertToChains]
      [simplify]
      %[convertToTree]
end function
