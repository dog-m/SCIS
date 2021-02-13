@set SCIS_TXL=../../txl_grammar.txl
@set SCIS_RULESET=../../ruleset_grammar.txl

@set SRC=./_normalized.pas
@set DST=./_instrumented.pas
@set RULESET=./rules/delphi.yml
@set ANNOTATION=../lang/delphi/annotation.xml
@set FRAGMENTS=../fragments/delphi/

@"../../Debug/scis" --src %SRC% --dst %DST% --ruleset %RULESET% --annotation %ANNOTATION% --fragments-dir %FRAGMENTS% --scis-grm-txl %SCIS_TXL% --scis-grm-ruleset %SCIS_RULESET%

@pause
