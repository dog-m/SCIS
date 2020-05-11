program test;

{$APPTYPE CONSOLE}

uses
  SysUtils;

type
  Main = class
  public
    a: integer;
    b: integer;

    procedure something_else(args: array of string);
    procedure main(args: array of string);
  end;

procedure Main.something_else(args: array of string);
begin
  if a >= b then
  begin
    if a < 0 then
      System.writeln('Fizz Buzz!');
  end;

  if a >= b then
  begin
    if a < 0 then
      System.writeln('Fizz Buzz!');
  end;
end;

procedure Main.main(args: array of string);
begin
  if a >= b then
  begin
    if a < 0 then
      System.writeln('Hello World!');
  end;

  if a >= b then
  begin
    if a < 0 then
      System.writeln('Hello World!');
  end;
end;

begin
  with Main.Create do
  begin
    a := 10;
    b := 20;
    main(['text']);

    Free;
  end;
end.
