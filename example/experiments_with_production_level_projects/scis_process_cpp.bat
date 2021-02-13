@set SCIS_TXL=../../txl_grammar.txl
@set SCIS_RULESET=../../ruleset_grammar.txl

@set SRC=./_normalized.hpp
@set DST=./_instrumented.hpp
@set RULESET=./rules/cpp.yml
@set ANNOTATION=../lang/cpp/annotation.xml
@set FRAGMENTS=../fragments/cpp/

@echo --src %SRC% --dst %DST% --ruleset %RULESET% --annotation %ANNOTATION% --fragments-dir %FRAGMENTS% --scis-grm-txl %SCIS_TXL% --scis-grm-ruleset %SCIS_RULESET%

@"../../Debug/scis" --src %SRC% --dst %DST% --ruleset %RULESET% --annotation %ANNOTATION% --fragments-dir %FRAGMENTS% --scis-grm-txl %SCIS_TXL% --scis-grm-ruleset %SCIS_RULESET%

@pause
