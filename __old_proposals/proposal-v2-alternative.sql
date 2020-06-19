use fragment "imports-instrummentation-fragment-with-dependencies"

-- name: instrument_some_statements_at_s3_and_s4
instrument
  print_iteration($node),
  correct_variable_i,
  do_other_stuff
at
  [first] if("x == 10"...) -> [all] for(*) # before_body
where
  { class_name = "Main" } & { method_name = "main" }


-- альтернатива
instrument
  [first] if_statement
    :before
      <- init_logging
    :before_body
      <- print_branch("first if")
      <- connect_to_db

  [all] for_statement("i = 10"...)
    :after
      <- print_message($pointcut, $class_name)

in context
  { class_name = "Main" } & { method_name = "main" }
