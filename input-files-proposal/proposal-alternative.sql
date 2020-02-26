use fragment "imports-instrummentation-fragment-with-dependencies"

-- instrument_some_statements_at_s3_and_s4:
instrument
  print_iteration($node),
  correct_variable_i,
  do_other_stuff
at
  [first] if("x == 10"...) -> [all] for(*) # before_body
where
  { class_name = "Main" } & { method_name = "main" }
