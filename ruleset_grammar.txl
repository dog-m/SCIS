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
end keys

compounds 
    ->  <-  ...  <=  >=  !=
end compounds

comments 
    //
    /*  */
end comments



define program 
    [repeat use_fragment_statement] [NL]
    [repeat context_definition] [NL]
    [rules]
end define



define use_fragment_statement
    'use 'fragment [stringlit] [NL]
end define



define context_definition
    'context [SP] [SPOFF] [context_name] ': [SPON] [NL] [IN]
        [basic_context_or_expression_context] [NL] [EX] [NL]
end define

define context_name
        [id]
    |   [global_context]
end define

define global_context
    '@
end define

define basic_context_or_expression_context
        [basic_context]
    |   [compound_context]
end define


define basic_context
    '{ [SP] [list basic_context_constraint+] [SP] '}
end define

define basic_context_constraint
        [context_property] [context_op] [SP] [string_template]
    |   [context_property] [context_op] [number]
end define

define string_template
    [SPOFF] [opt '...] [stringlit] [opt '...] [SPON]
end define

define context_op
    '= | '< | '<= | '> | '>= | '!=
end define

define context_property
    [SPOFF] [context_property_variant] [SPON]
end define

define context_property_variant
        [point_of_interest]
    |   [something]
end define

define point_of_interest
    poi ': [id]
end define

define something
    [id]
end define


define compound_context
    [context_expression]
end define

define context_expression
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
end define




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
    '( [opt '...] [stringlit] [opt '...] ')
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
        [action_list] [NL] [EX]
end define

define action_list
        [action_id], [NL] [action_list]
    |   [action_id]
end define

define action_id
    [id] [opt template_params]
end define

define template_params
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
