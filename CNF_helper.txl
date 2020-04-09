define cnf_entry
    [cnf_expression]
end define

define cnf_expression
        [cnf_expression] [SP] '| [SP] [cnf_expression]
    |   [cnf_term]
end define

define cnf_term
        [cnf_term] [SP] '& [SP] [cnf_term]
    |   [cnf_inversion]
end define

define cnf_inversion
        [SPOFF] '! [cnf_inversion] [SPON]
    |   [cnf_primary]
end define

define cnf_primary
        [cnf_var_or_const]
    |   ( [cnf_expression] )
end define

define cnf_var_or_const
      [id]
  |   '@
  |   '0
end define


% =================================================================

rule cnf_buildCNF
  replace [cnf_expression]
    E [cnf_expression]
  construct NewE [cnf_expression]
    E [cnf_demorganD]
      [cnf_demorganC]
      
      [cnf_cancelDoubleNegation]
      [cnf_cancelDoubleNegation2]
      
      [cnf_resolveBrackets]
      [cnf_resolveBracketsSimple]
      [cnf_resolveBracketsId]
      [cnf_resolveBracketsR]
      [cnf_resolveBracketsL]
      
      [cnf_distributeOr]
      [cnf_distributeOr2]
      
      [cnf_collapseSelfOr]
      [cnf_collapseSelfAnd]
      [cnf_collapseSelfOr2]
      [cnf_collapseSelfOr3]
      [cnf_collapseSelfOr4]
      [cnf_collapseSelfOr5]
      [cnf_collapseSelfId]
      [cnf_collapseSelfId2]
      
      [cnf_collapseConstant_True]
      [cnf_collapseConstant_True2]
      [cnf_collapseConstant_True3]
      [cnf_collapseConstant_True_or]
      [cnf_collapseConstant_True_or2]
      [cnf_collapseConstant_True_and]
      [cnf_collapseConstant_True_and2]
      
      [cnf_collapseConstant_False]
      [cnf_collapseConstant_False2]
      [cnf_collapseConstant_False3]
      [cnf_collapseConstant_False_or]
      [cnf_collapseConstant_False_or2]
      [cnf_collapseConstant_False_and]
      [cnf_collapseConstant_False_and2]
      
      [cnf_consumeOr]
      [cnf_consumeOr2]
  where not
    NewE [= E]
  by
    NewE
end rule


rule cnf_demorganD
  replace [cnf_inversion]
    '! '( A [cnf_expression] '| B [cnf_expression] ')
  by
    ( '!(A) '& '!(B) )
end rule


rule cnf_demorganC
  replace [cnf_inversion]
    '!( A [cnf_term] '& B [cnf_term] )
  by
    ( '!(A) '| '!(B) )
end rule


rule cnf_consumeOr
  replace [cnf_expression]
    A [cnf_term] '& B [cnf_term] '| A
  by
    A
end rule

rule cnf_consumeOr2
  replace [cnf_expression]
    A [cnf_term] '| A '& B [cnf_term]
  by
    A
end rule


rule cnf_cancelDoubleNegation
  replace [cnf_inversion]
    '!( '! A [cnf_inversion] )
  by
    A
end rule

rule cnf_cancelDoubleNegation2
  replace [cnf_inversion]
    '! '! A [cnf_inversion]
  by
    A
end rule


rule cnf_resolveBrackets
  replace [cnf_expression]
    ( N [cnf_expression] )
  by
    N
end rule

rule cnf_resolveBracketsSimple
  replace [cnf_primary]
    ( N [cnf_primary] )
  by
    N
end rule

rule cnf_resolveBracketsId
  replace [cnf_expression]
    ( N [cnf_var_or_const] )
  by
    N
end rule


rule cnf_resolveBracketsR
  replace [cnf_term]
    A [cnf_term] '& ( B [cnf_inversion] '& C [cnf_inversion] )
  by
    A '& B '& C
end rule

rule cnf_resolveBracketsL
  replace [cnf_term]
    ( A [cnf_term] '& B [cnf_inversion] ) '& C [cnf_inversion]
  by
    A '& B '& C
end rule


rule cnf_distributeOr
  replace [cnf_expression]
    A [cnf_expression] '| B [cnf_term] '& C [cnf_inversion]
  by
    (A '| B) '& (A '| C)
end rule

rule cnf_distributeOr2
  replace [cnf_expression]
    B [cnf_term] '& C [cnf_inversion] '| A [cnf_term]
  by
    (B '| A) '& (C '| A)
end rule


rule cnf_collapseSelfOr
  replace * [cnf_expression]
    A [cnf_expression] '| B [cnf_expression]
  construct A_str [stringlit]
    _ [quote A]
  construct B_str [stringlit]
    _ [quote B]
  where
    A_str [= B_str]
  by
    A
end rule

rule cnf_collapseSelfOr2
  replace * [cnf_expression]
    A [cnf_var_or_const] '| '! B [cnf_var_or_const] '| C [cnf_expression]
  where
    A [= B]
  by
    C
end rule

rule cnf_collapseSelfOr3
  replace * [cnf_expression]
    '! A [cnf_var_or_const] '| B [cnf_var_or_const] '| C [cnf_expression]
  where
    A [= B]
  by
    C
end rule

rule cnf_collapseSelfOr4
  replace * [cnf_expression]
    A [cnf_expression] '| B [cnf_var_or_const] '| '! C [cnf_var_or_const]
  where
    B [= C]
  by
    A
end rule

rule cnf_collapseSelfOr5
  replace * [cnf_expression]
    A [cnf_expression] '| '! B [cnf_var_or_const] '| C [cnf_var_or_const]
  where
    B [= C]
  by
    A
end rule

rule cnf_collapseSelfAnd
  replace [cnf_term]
    A [cnf_term] '& A
  by
    A
end rule

rule cnf_collapseSelfId
  replace * [cnf_expression]
    A [cnf_var_or_const] '| B [cnf_var_or_const] '| C [cnf_expression]
  where
    A [= B]
  by
    A '| C
end rule

rule cnf_collapseSelfId2
  replace * [cnf_expression]
    A [cnf_expression] '| B [cnf_var_or_const] '| C [cnf_var_or_const]
  where
    B [= C]
  by
    A '| B
end rule


rule cnf_collapseConstant_True
  replace * [cnf_inversion]
    '! '@
  by
    '0
end rule

rule cnf_collapseConstant_True2
  replace * [cnf_expression]
    '! A [cnf_inversion] '| A
  by
    '@
end rule

rule cnf_collapseConstant_True3
  replace * [cnf_expression]
    A [cnf_inversion] '| '! A
  by
    '@
end rule

rule cnf_collapseConstant_True_or
  replace * [cnf_expression]
    A [cnf_expression] '| '@
  by
    '@
end rule

rule cnf_collapseConstant_True_or2
  replace * [cnf_expression]
    '@ '| A [cnf_expression]
  by
    '@
end rule

rule cnf_collapseConstant_True_and
  replace * [cnf_term]
    A [cnf_term] '& '@
  by
    A
end rule

rule cnf_collapseConstant_True_and2
  replace * [cnf_term]
    '@ '& A [cnf_term]
  by
    A
end rule


rule cnf_collapseConstant_False
  replace * [cnf_inversion]
    '! '0
  by
    '@
end rule

rule cnf_collapseConstant_False2
  replace * [cnf_term]
    '! A [cnf_inversion] '& A
  by
    '0
end rule

rule cnf_collapseConstant_False3
  replace * [cnf_term]
    A [cnf_inversion] '& '! A
  by
    '0
end rule

rule cnf_collapseConstant_False_or
  replace * [cnf_expression]
    A [cnf_expression] '| '0
  by
    A
end rule

rule cnf_collapseConstant_False_or2
  replace * [cnf_expression]
    '0 '| A [cnf_expression]
  by
    A
end rule

rule cnf_collapseConstant_False_and
  replace * [cnf_term]
    A [cnf_term] '& '0
  by
    '0
end rule

rule cnf_collapseConstant_False_and2
  replace * [cnf_term]
    '0 '& A [cnf_term]
  by
    '0
end rule


% =================================================================

redefine cnf_entry
        ...
    |   [cnf_expression_lists]
end redefine

define cnf_expression_lists
        [repeat cnf_expression_chain_and]
    |   [repeat cnf_expression_element_chain]
end define

define cnf_expression_chain_and
    [cnf_expression_chain_or] [opt '&]
end define

define cnf_expression_chain_or
        [cnf_expression_element]
    |   ( [repeat cnf_expression_element_chain] )
end define

define cnf_expression_element_chain
    [cnf_expression_element] [opt '|]
end define

define cnf_expression_element
    [opt '!] [cnf_var_or_const]
end define

% =================================================================

function cnf_convertToChains
  replace [cnf_entry]
    P [cnf_expression]
  construct TypeHolder [cnf_expression_lists]
    % void
  construct newP [cnf_expression_lists]
    TypeHolder [reparse P]
  by
    newP
end function

function cnf_convertToTree
  replace [cnf_entry]
    P [cnf_expression_lists]
  construct TypeHolder [cnf_expression]
    'SOME_USELESS_RANDOM_IDENTIFIER
  construct newP [cnf_expression]
    TypeHolder [reparse P]
  by
    newP
end function

% =================================================================

rule cnf_simplify
  replace [cnf_expression_lists]
    E [cnf_expression_lists]
  construct NewE [cnf_expression_lists]
    E [cnf_sort]
      [cnf_sort2]
      
      [cnf_collapseSelf_or]
      [cnf_collapseSelf_and]
      
      %[cnf_simplify_and]
      
      [cnf_fix_tail_of_and_chain]
  where not
    NewE [= E]
  by
    NewE
end rule


rule cnf_sort
  replace [repeat cnf_expression_element_chain]
    Ao [opt '!] A [cnf_var_or_const] '| Bo [opt '!] B [cnf_var_or_const] Co [opt '|] C [repeat cnf_expression_element_chain]
  construct A_str [stringlit]
    _ [quote A]
  construct B_str [stringlit]
    _ [quote B]
  where
    B_str [< A_str]
  by
    Bo B '| Ao A Co C
end rule


rule cnf_sort2
  replace [repeat cnf_expression_chain_and]
    A [cnf_expression_chain_or] '& B [cnf_expression_chain_or] Bo [opt '&] C [repeat cnf_expression_chain_and]
  construct A_str [stringlit]
    _ [quote A]
  construct B_str [stringlit]
    _ [quote B]
  where
    B_str [< A_str]
  by
    B '& A Bo C
end rule


rule cnf_collapseSelf_or
  replace [repeat cnf_expression_element_chain]
    A [cnf_expression_element] '| A Bo [opt '|] C [repeat cnf_expression_element_chain]
  by
    A Bo C
end rule


rule cnf_collapseSelf_and
  replace [repeat cnf_expression_chain_and]
    A [cnf_expression_chain_or] '& A Bo [opt '&] C [repeat cnf_expression_chain_and]
  by
    A Bo C
end rule


rule cnf_fix_tail_of_and_chain
  replace [repeat cnf_expression_chain_and]
    B [cnf_expression_chain_or] '&
  by
    B
end rule


% =================================================================

rule do_cnf
  replace $ [cnf_entry]
    P [cnf_entry]
  by
    P [cnf_buildCNF]
      [cnf_convertToChains]
      [cnf_simplify]
      %[cnf_convertToTree]
end rule
