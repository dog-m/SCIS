program test;
uses
    SysUtils;
type
    Main = class
    public
        a: integer;
        b: integer;
        procedure something_else (args: array of string);
        procedure main (args: array of string);
    end;

procedure Main.something_else (args: array of string);
begin
    if a  >=  b then begin
        if a  <  0 then
            System.writeln ('Fizz Buzz!');
    end;
    if a  >=  b then begin
        if a  <  0 then
            System.writeln ('Fizz Buzz!');
    end;
end;

procedure Main.main (args: array of string);
begin
    begin
        begin
            System.WriteLn ('[Pseudo-LOGGER]',  #32,  'before <if_statement> block in [Main.main] method');
            if a  >=  b then begin
                case 1 of 1: begin
                    if a  <  0 then
                        System.writeln ('Hello World!');
                end end;
            end;
        end;
        if a  >=  b then begin
            if a  <  0 then
                System.writeln ('Hello World!');
        end;
    end;
end;
begin
    with Main.Create do begin
        a := -10;
        b := -20;
        main (['text']);
        Free;
    end;
end.
