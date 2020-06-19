rule add_logging_to_some_program
{
  instrument
    [first] if(*),
    [first] if("x == 10"...) -> [all] for(*)
  by
    CustomRecipie
  where
    class_name = "Main" & method_name = "main"
}
