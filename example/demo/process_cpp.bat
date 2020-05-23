@set SCIS_TXL=../../txl_grammar.txl
@set SCIS_RULESET=../../ruleset_grammar.txl

@set SRC=./test_source.cpp
@set DST=./test_source_instrumented.cpp
@set RULESET=./demo_refiner_modifiers.yml
@set ANNOTATION=../lang/cpp/annotation.xml
@set FRAGMENTS=./fragments/

"../../Debug/scis" --src %SRC% --dst %DST% --ruleset %RULESET% --annotation %ANNOTATION% --fragments-dir %FRAGMENTS% --scis-grm-txl %SCIS_TXL% --scis-grm-ruleset %SCIS_RULESET%

@pause
