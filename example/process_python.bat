@set SCIS_TXL=../txl_grammar.txl
@set SCIS_RULESET=../ruleset_grammar.txl

@set SRC=./test_source.py
@set DST=./test_source_instrumented.py
@set RULESET=./add_logging_to_Main_main.yml
@set ANNOTATION=./lang/python/annotation.xml
@set FRAGMENTS=./fragments/python/

"../Debug/scis" --src %SRC% --dst %DST% --ruleset %RULESET% --annotation %ANNOTATION% --fragments-dir %FRAGMENTS% --scis-grm-txl %SCIS_TXL% --scis-grm-ruleset %SCIS_RULESET%

@pause
