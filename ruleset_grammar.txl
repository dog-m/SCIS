%#pragma -newline -id "-"
#pragma -in 2

tokens
    % Python string forms - shortstrings are already captured by [stringlit] and [charlit]
    longstringlit	"\"\"\"#[(\"\"\")]*\"\"\""
    longcharlit		"'''#[(''')]*'''"
end tokens

keys 
    'with        'from
    'use         'fragment
    'context
    'rules
    
    'has         'matches
end keys

compounds 
    ->  <-  ...  <=  >=  ~=
end compounds

comments 
    //
    /*  */
end comments

include "cnf_helper.txl"



define program 
    [repeat use_fragment_statement+] [NL]
    [repeat context_definition] [NL]
    [rules]
end define



define use_fragment_statement
    'use 'fragment [stringlit] [NL]
end define



define context_definition
    'context [SP] [SPOFF] [context_name] ': [SPON] [NL] [IN]
        [basic_context_or_compound_context] [NL] [EX] [NL]
end define

define context_name
        [id]
    |   [global_context]
end define

define global_context
    '@
end define

define basic_context_or_compound_context
        [basic_context]
    |   [compound_context]
end define


define basic_context
    '{ [SP] [list basic_context_constraint+] [SP] '}
end define

define basic_context_constraint
    [SPOFF] [context_property] [SPON] [context_op] [SP] [SPOFF] [string_template] [SPON]
end define

define string_template
    [stringlit]
end define

define context_op
    '= | '< | '<= | '> | '>= | '~= | 'has | 'matches
end define

define context_property
        [id_with_group]
    |   [id]
end define

define id_with_group
    [group_id] ': [id]
end define

define group_id
    [id]
end define



define compound_context
    %[context_expression]
    [cnf_entry]
end define

%{define context_expression
        [context_expression] '| [context_expression]
    |   [context_expression] '- [context_expression]
    |   [context_term]
end define

define context_term
        [context_term] '& [context_term]
    |   [context_primary]
end define

define context_primary
        [context_name]
    |   ( [context_expression] )
end define}%




define rules
    [SPOFF] 'rules ': [SPON] [NL] [IN]
        [repeat single_rule+] [EX]
end define

define single_rule
    [SPOFF] [id] ': [SPON] [NL] [IN]
        [repeat rule_statement+] [EX]
end define

define rule_statement
    [rule_path] [NL] [IN]
        [rule_actions] [NL] [EX]
end define




define rule_path
    [SPOFF] '@ [context_name] [SPON] [repeat path_item_with_arrow+] '# [SP] [SPOFF] [pointcut] ': [SPON]
end define

define pointcut
    [id]
end define

define path_item_with_arrow
    '-> [path_item]
end define

define path_item
    [SP] [SPOFF] '[ [modifier] '] [SPON] [statement_name] [opt param_template]
end define

define modifier
    [id]
end define

define statement_name
    [id]
end define

define param_template
    '( [string_template] ')
end define



define rule_actions
    [opt action_make]
    [action_add]
end define

define action_make
    [SPOFF] 'make ': [SPON] [NL] [IN]
      [repeat action_make_item] [NL] [EX]
end define

define action_make_item
    [id] '<- [string_chain] ';
end define

define action_add
    [SPOFF] 'add ': [SPON] [NL] [IN]
        [list action_id+] [EX] [NL]
end define

define action_id
    [id] [opt template_args]
end define

define template_args
    '( [list id+] ')
end define



define string_chain
    [stringlit_or_constant] [repeat string_chain_node]
end define

define string_chain_node
    '+ [stringlit_or_constant]
end define

define stringlit_or_constant
        [string_constant]
    |   [stringlit]
end define

define string_constant
    [SP] [SPOFF] '$ [id] [SPON]
end define



% ==============================================

function main
  replace * [program]
    P [program]
  by
    P [do_cnf]
end function
